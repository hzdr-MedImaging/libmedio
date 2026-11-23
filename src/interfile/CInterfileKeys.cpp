/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio
 
  Copyright (C) 2004-2026 hzdr.de and contributors
 
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
    http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

//! @file CInterfileKeys.cpp
//! @brief private: the "key := value" lines of an Interfile header (see CInterfileKeys.h)

#include "CInterfileKeys.h"

#include <QFile>
#include <QLocale>
#include <QRegularExpression>
#include <QTextCodec>

#include <rtdebug.h>

namespace {

// the largest header read by CInterfileKeys::read(): headers of scanners with many
// sinogram segments have some 10 kB, files holding header and data are read up to
// the end of the header
const qint64 MaxHeaderSize = 16 * 1024 * 1024;

// a line of the header as text: UTF-8 if it is valid UTF-8, otherwise Latin-1 (the
// 8 bit character set of older headers)
QString decodeLine(const QByteArray& line)
{
  QTextCodec* codec = QTextCodec::codecForName("UTF-8");
  QTextCodec::ConverterState state;
  const QString text = codec->toUnicode(line.constData(), line.size(), &state);
  if(state.invalidChars == 0)
    return text;

  return QString::fromLatin1(line);
}

// the next line of text from position pos on (without CR/LF), pos moved to the
// start of the following line; lines end with LF, CR LF or CR
QByteArray nextLine(const QByteArray& text, int& pos)
{
  int end = pos;
  while(end < text.size() && text.at(end) != '\n' && text.at(end) != '\r')
    end++;

  const QByteArray line = text.mid(pos, end - pos);
  if(end < text.size())
  {
    // CR LF is one line end
    if(text.at(end) == '\r' && end + 1 < text.size() && text.at(end + 1) == '\n')
      end++;
    end++;
  }
  pos = end;

  return line;
}

} // namespace

//-----------------------------------------------------------------------------------------

QString CInterfileKeys::normalizedName(const QString& key, QList<int>* indices, bool* required, bool* vendor)
{
  QString name = key.trimmed();
  bool isRequired = false;
  bool isVendor = false;

  // the prefixes '!' (required key) and '%' (vendor key)
  while(name.startsWith('!') || name.startsWith('%'))
  {
    if(name.startsWith('!'))
      isRequired = true;
    else
      isVendor = true;
    name = name.mid(1).trimmed();
  }

  // the indices at the end: "matrix size [1]", "image duration (sec)[2]", "key[1][2]"
  static const QRegularExpression indexExpression("\\[\\s*(\\d+)\\s*\\]\\s*$");
  QList<int> keyIndices;
  for(QRegularExpressionMatch match = indexExpression.match(name); match.hasMatch();
      match = indexExpression.match(name))
  {
    keyIndices.prepend(match.captured(1).toInt());
    name.truncate(match.capturedStart());
  }

  name = name.toLower();
  name.replace('_', ' ');
  static const QRegularExpression openExpression("\\s*\\(\\s*");
  static const QRegularExpression closeExpression("\\s*\\)");
  name.replace(openExpression, " (");
  name.replace(closeExpression, ")");
  name = name.simplified();

  if(indices)
    *indices = keyIndices;
  if(required)
    *required = isRequired;
  if(vendor)
    *vendor = isVendor;

  return name;
}

bool CInterfileKeys::parse(const QByteArray& text, QString* error, qint64* headerSize)
{
  ENTER();

  QList<CInterfileKey> keys;
  bool started = false;
  int pos = 0;
  QString reason = "no \"!END OF INTERFILE\" line";

  while(pos < text.size())
  {
    const QByteArray rawLine = nextLine(text, pos);
    const QString line = decodeLine(rawLine).trimmed();

    if(line.isEmpty())
      continue;

    if(line.startsWith(';'))
    {
      // comments before "!INTERFILE" are not part of the header
      if(started)
      {
        CInterfileKey comment;
        comment.comment = true;
        comment.text = line;
        keys.append(comment);
      }
      continue;
    }

    const int separator = line.indexOf(":=");
    if(separator < 0)
    {
      if(started == false || rawLine.contains('\0'))
      {
        reason = started ? "binary data before \"!END OF INTERFILE\"" : "no \"!INTERFILE\" line at the start";
        break;
      }

      // a line without ":=" inside the header: kept as comment
      W("Interfile: line without \":=\" kept as comment: '%s'", qPrintable(line));
      CInterfileKey comment;
      comment.comment = true;
      comment.text = "; " + line;
      keys.append(comment);
      continue;
    }

    CInterfileKey key;
    key.text = line.left(separator).trimmed();
    key.value = line.mid(separator + 2).trimmed();
    key.name = normalizedName(key.text, &key.indices, &key.required, &key.vendor);

    if(started == false)
    {
      if(key.name != "interfile")
      {
        reason = "no \"!INTERFILE\" line at the start";
        break;
      }
      started = true;
    }

    keys.append(key);

    if(key.name == "end of interfile")
    {
      m_keys = keys;
      if(headerSize)
        *headerSize = pos;
      RETURN(true);
      return true;
    }
  }

  if(error)
    *error = reason;

  RETURN(false);
  return false;

  return false;
}

bool CInterfileKeys::read(const QString& fileName, QString* error, qint64* headerSize)
{
  ENTER();

  QFile file(fileName);
  if(file.open(QIODevice::ReadOnly) == false)
  {
    if(error)
      *error = QString("cannot open '%1': %2").arg(fileName, file.errorString());
    RETURN(false);
    return false;
  }

  // read until the end of the header: the file may hold the image data after it
  QByteArray text;
  while(file.atEnd() == false && text.size() < MaxHeaderSize)
  {
    const QByteArray chunk = file.read(64 * 1024);
    if(chunk.isEmpty())
      break;
    text += chunk;

    const QByteArray upper = text.toUpper();
    const int end = upper.indexOf("END OF INTERFILE");
    if(end >= 0 && upper.indexOf('\n', end) >= 0)
      break;
    // a file starting without "!INTERFILE" is no Interfile header
    if(text.size() >= 64 * 1024 && upper.indexOf("INTERFILE") < 0)
      break;
  }

  QString reason;
  if(parse(text, &reason, headerSize) == false)
  {
    if(error)
      *error = QString("'%1' is no Interfile header: %2").arg(fileName, reason);
    RETURN(false);
    return false;
  }

  RETURN(true);
  return true;

  return true;
}

bool CInterfileKeys::isInterfile(const QString& fileName)
{
  ENTER();

  QFile file(fileName);
  if(file.open(QIODevice::ReadOnly) == false)
  {
    RETURN(false);
    return false;
  }

  // the first line that is neither empty nor a comment has to be "!INTERFILE :="
  const QByteArray text = file.read(4096);
  int pos = 0;
  if(text.startsWith("\xEF\xBB\xBF"))
    pos = 3;
  while(pos < text.size())
  {
    const QString line = decodeLine(nextLine(text, pos)).trimmed();
    if(line.isEmpty() || line.startsWith(';'))
      continue;

    const int separator = line.indexOf(":=");
    RETURN(separator > 0 && normalizedName(line.left(separator)) == "interfile");
    return separator > 0 && normalizedName(line.left(separator)) == "interfile";
  }

  RETURN(false);
  return false;

  return false;
}

QByteArray CInterfileKeys::toText() const
{
  QString text;
  foreach(const CInterfileKey& key, m_keys)
  {
    if(key.comment)
      text += key.text;
    else if(key.value.isEmpty())
      text += key.text + " :=";
    else
      text += key.text + " := " + key.value;
    text += "\r\n";
  }

  // ASCII/Latin-1 as the headers of other programs, UTF-8 for other characters
  bool latin1 = true;
  for(int i = 0; i < text.size() && latin1; i++)
    latin1 = text.at(i).unicode() < 256;

  return latin1 ? text.toLatin1() : text.toUtf8();
}

void CInterfileKeys::clear()
{
  m_keys.clear();
}

int CInterfileKeys::count() const
{
  return m_keys.size();
}

const CInterfileKey& CInterfileKeys::at(int i) const
{
  return m_keys.at(i);
}

int CInterfileKeys::indexOf(const QString& name, const QList<int>& indices, int from) const
{
  QList<int> nameIndices;
  const QString normalized = normalizedName(name, &nameIndices);
  const QList<int>& wanted = indices.isEmpty() ? nameIndices : indices;

  for(int i = qMax(from, 0); i < m_keys.size(); i++)
  {
    const CInterfileKey& key = m_keys.at(i);
    if(key.comment == false && key.name == normalized && key.indices == wanted)
      return i;
  }

  return -1;
}

bool CInterfileKeys::contains(const QString& name, const QList<int>& indices) const
{
  return indexOf(name, indices) >= 0;
}

QString CInterfileKeys::value(const QString& name, const QList<int>& indices, const QString& defaultValue) const
{
  const int i = indexOf(name, indices);
  return (i >= 0) ? m_keys.at(i).value : defaultValue;
}

void CInterfileKeys::append(const QString& key, const QString& value)
{
  CInterfileKey line;
  line.text = key.trimmed();
  line.value = value.trimmed();
  line.name = normalizedName(line.text, &line.indices, &line.required, &line.vendor);
  m_keys.append(line);
}

void CInterfileKeys::appendComment(const QString& text)
{
  CInterfileKey line;
  line.comment = true;
  line.text = "; " + text.trimmed();
  m_keys.append(line);
}

void CInterfileKeys::setValue(const QString& key, const QString& value)
{
  const int i = indexOf(key);
  if(i >= 0)
    m_keys[i].value = value.trimmed();
  else
    append(key, value);
}

int CInterfileKeys::remove(const QString& name, const QList<int>& indices)
{
  int removed = 0;
  for(int i = indexOf(name, indices); i >= 0; i = indexOf(name, indices, i))
  {
    m_keys.removeAt(i);
    removed++;
  }

  return removed;
}

bool CInterfileKeys::toInt(const QString& value, qint64* result)
{
  bool ok = false;
  const qint64 number = value.trimmed().toLongLong(&ok);
  if(ok && result)
    *result = number;

  return ok;
}

bool CInterfileKeys::toDouble(const QString& value, double* result)
{
  bool ok = false;
  const double number = QLocale::c().toDouble(value.trimmed(), &ok);
  if(ok && result)
    *result = number;

  return ok;
}

QStringList CInterfileKeys::toList(const QString& value)
{
  const QString text = value.trimmed();
  if(text.startsWith('{') == false || text.endsWith('}') == false)
    return QStringList() << text;

  const QString inner = text.mid(1, text.size() - 2).trimmed();
  QStringList elements;
  if(inner.isEmpty())
    return elements;

  foreach(const QString& element, inner.split(','))
    elements << element.trimmed();

  return elements;
}
