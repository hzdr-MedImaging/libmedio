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

//! @file CNIFTIExtensions.cpp
//! @brief private: header extensions holding the metadata of NIfTI files (see CNIFTIExtensions.h)

#include "CNIFTIExtensions.h"
#include "CMedIOHeaderDictionary.h"
#include "CMedIOHeaderMapping.h"
#include "CNIFTIFile.h"
#include "CNIFTIMainHeader.h"
#include "CNIFTI1MainHeader.h"
#include "CNIFTI2MainHeader.h"
#include "CNIFTIGeometry.h"
#include "bswap.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMap>
#include <QRegExp>
#include <QStringList>
#include <QtEndian>

#include <cmath>
#include <cstring>

#include <rtdebug.h>

namespace NIFTIExtensions {

namespace {

// DICOM tags used besides those of the dictionary
const quint32 SpecificCharacterSet = 0x00080005;     // (0008,0005)
const quint32 CodeValue = 0x00080100;                // (0008,0100) SH
const quint32 CodingSchemeDesignator = 0x00080102;   // (0008,0102) SH
const quint32 CodingSchemeVersion = 0x00080103;      // (0008,0103) SH
const quint32 CodeMeaning = 0x00080104;              // (0008,0104) LO
const quint32 PmodGroup = 0x0055;                    // private group of PMOD
const char* const PmodCreator = "PMOD_1";
const quint32 Item = 0xFFFEE000;
const quint32 ItemDelimitation = 0xFFFEE00D;
const quint32 SequenceDelimitation = 0xFFFEE0DD;
const quint32 DataSetTrailingPadding = 0xFFFCFFFC;   // (FFFC,FFFC) OB

// value representations of DICOM (PS3.5 6.2)
const char* const knownVRs[] = { "AE", "AS", "AT", "CS", "DA", "DS", "DT", "FD", "FL", "IS", "LO", "LT",
                                 "OB", "OD", "OF", "OL", "OV", "OW", "PN", "SH", "SL", "SQ", "SS", "ST",
                                 "SV", "TM", "UC", "UI", "UL", "UN", "UR", "US", "UT", "UV" };

// value representations with a 4 byte value length in explicit VR
bool longVR(const QByteArray& vr)
{
  static const char* const vrs[] = { "OB", "OD", "OF", "OL", "OV", "OW", "SQ", "SV", "UC", "UN", "UR", "UT", "UV" };
  for(unsigned i=0; i < sizeof(vrs) / sizeof(vrs[0]); i++) {
    if(vr == vrs[i])
      return true;
  }
  return false;
}

bool knownVR(const QByteArray& vr)
{
  for(unsigned i=0; i < sizeof(knownVRs) / sizeof(knownVRs[0]); i++) {
    if(vr == knownVRs[i])
      return true;
  }
  return false;
}

// maximum length of a value (PS3.5 6.2), 0 if not limited here
int maxLength(const QByteArray& vr)
{
  if(vr == "AS") return 4;
  if(vr == "CS" || vr == "SH" || vr == "DS") return 16;
  if(vr == "IS") return 12;
  if(vr == "LO" || vr == "PN" || vr == "UI") return 64;
  return 0;
}

void setIfMissing(QJsonObject& json, const QString& key, const QJsonValue& value)
{
  if(json.contains(key) == false && value.isUndefined() == false && value.isNull() == false)
    json[key] = value;
}

//---------------------------------------------------------------------------------------------
// encoding (explicit VR)

class Encoder
{
  public:
    explicit Encoder(bool bigEndian) : m_big(bigEndian), m_nonAscii(false) { }

    bool nonAscii() const { return m_nonAscii; }

    void append16(QByteArray& out, quint16 v) const
    {
      uchar b[2];
      if(m_big) qToBigEndian(v, b); else qToLittleEndian(v, b);
      out.append(reinterpret_cast<const char*>(b), 2);
    }

    void append32(QByteArray& out, quint32 v) const
    {
      uchar b[4];
      if(m_big) qToBigEndian(v, b); else qToLittleEndian(v, b);
      out.append(reinterpret_cast<const char*>(b), 4);
    }

    // element with a value (padded to an even length); empty if the value is
    // too long for its value length field
    QByteArray element(quint32 tag, const QByteArray& vr, QByteArray value) const
    {
      if(value.size() % 2 != 0)
        value.append(vr == "UI" || vr == "OB" ? '\0' : ' ');

      QByteArray out;
      append16(out, static_cast<quint16>(tag >> 16));
      append16(out, static_cast<quint16>(tag & 0xFFFF));
      out.append(vr);
      if(longVR(vr)) {
        append16(out, 0);
        append32(out, static_cast<quint32>(value.size()));
      } else {
        if(value.size() > 0xFFFF)
          return QByteArray();
        append16(out, static_cast<quint16>(value.size()));
      }
      out.append(value);
      return out;
    }

    // sequence with one item holding the data set
    QByteArray sequence(quint32 tag, const QByteArray& dataSet) const
    {
      QByteArray item;
      append16(item, static_cast<quint16>(Item >> 16));
      append16(item, static_cast<quint16>(Item & 0xFFFF));
      append32(item, static_cast<quint32>(dataSet.size()));
      item.append(dataSet);
      return element(tag, "SQ", item);
    }

    // text of a string value (Latin-1, ISO_IR 100), shortened to the maximum
    // length of the VR; a backslash separates values in DICOM
    QByteArray text(const QString& value, const QByteArray& vr, bool multiple = false)
    {
      QString s = value;
      if(multiple == false)
        s.replace(QChar('\\'), QChar(' '));
      for(int i=0; i < s.size(); i++) {
        if(s[i].unicode() > 127)
          m_nonAscii = true;
      }
      QByteArray b = s.toLatin1();
      const int max = maxLength(vr);
      if(max > 0 && b.size() > max)
        b = b.left(max);
      return b;
    }

    // value of an attribute in the encoding of its VR, empty if not possible
    QByteArray value(const CMedIOHeaderDictionary::Entry& entry, const QJsonValue& v)
    {
      const QByteArray vr(entry.vr);

      switch(entry.type) {
        case CMedIOHeaderDictionary::String:
          return text(v.toString(), vr);

        case CMedIOHeaderDictionary::Number: {
          const double d = v.toDouble();
          if(std::isfinite(d) == false)
            return QByteArray();
          if(vr == "IS") {
            const double r = std::floor(d + 0.5);
            if(r < -2147483648.0 || r > 2147483647.0)
              return QByteArray();
            return QByteArray::number(static_cast<qint64>(r));
          }
          // DS: at most 16 characters
          for(int precision=15; precision > 0; precision--) {
            const QByteArray s = QByteArray::number(d, 'g', precision);
            if(s.size() <= 16)
              return s;
          }
          return QByteArray();
        }

        case CMedIOHeaderDictionary::Date:
          return v.toString().remove(QChar('-')).toLatin1();

        case CMedIOHeaderDictionary::Time:
          return v.toString().remove(QChar(':')).toLatin1();

        case CMedIOHeaderDictionary::DateTime: {
          QRegExp iso("^(\\d{4})-(\\d{2})-(\\d{2})T(\\d{2}):(\\d{2})(?::(\\d{2})(\\.\\d{1,6})?)?(Z|[+-]\\d{2}:\\d{2})?$");
          if(iso.exactMatch(v.toString()) == false)
            return QByteArray();
          QString s = iso.cap(1) + iso.cap(2) + iso.cap(3) + iso.cap(4) + iso.cap(5) + iso.cap(6) + iso.cap(7);
          const QString zone = iso.cap(8);
          if(zone == "Z")
            s += "+0000";
          else if(zone.isEmpty() == false)
            s += QString(zone).remove(QChar(':'));
          return s.toLatin1();
        }

        case CMedIOHeaderDictionary::Strings: {
          const QJsonArray array = v.toArray();
          QList<QByteArray> values;
          for(int i=0; i < array.size(); i++)
            values.append(text(array[i].toString().replace(QChar('\\'), QChar(' ')), vr, true));
          QByteArray joined;
          for(int i=0; i < values.size(); i++) {
            if(i > 0)
              joined.append('\\');
            joined.append(values[i]);
          }
          return joined;
        }

        case CMedIOHeaderDictionary::Code:
          break; // a sequence, see codeItem()
      }

      return QByteArray();
    }

    // data set of a code sequence item
    QByteArray codeItem(const QJsonObject& code)
    {
      static const struct { const char* key; quint32 tag; const char* vr; } fields[] = {
        { "CodeValue", CodeValue, "SH" },
        { "CodingSchemeDesignator", CodingSchemeDesignator, "SH" },
        { "CodingSchemeVersion", CodingSchemeVersion, "SH" },
        { "CodeMeaning", CodeMeaning, "LO" },
      };
      QByteArray dataSet;
      for(unsigned i=0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        const QString s = code.value(QLatin1String(fields[i].key)).toString();
        if(s.isEmpty() == false)
          dataSet.append(element(fields[i].tag, fields[i].vr, text(s, fields[i].vr)));
      }
      return dataSet;
    }

    // FD values
    QByteArray doubles(const QJsonArray& array, double factor) const
    {
      QByteArray out;
      for(int i=0; i < array.size(); i++) {
        double d = array[i].toDouble() * factor;
        quint64 bits = 0;
        memcpy(&bits, &d, sizeof(bits));
        uchar b[8];
        if(m_big) qToBigEndian(bits, b); else qToLittleEndian(bits, b);
        out.append(reinterpret_cast<const char*>(b), 8);
      }
      return out;
    }

  private:
    bool m_big;
    bool m_nonAscii;
};

bool numbers(const QJsonArray& array)
{
  for(int i=0; i < array.size(); i++) {
    if(array[i].isDouble() == false || std::isfinite(array[i].toDouble()) == false)
      return false;
  }
  return true;
}

//---------------------------------------------------------------------------------------------
// decoding (explicit or implicit VR)

struct Element {
  quint32 tag;
  QByteArray vr;
  QByteArray value;
  QList<QList<Element> > items;
};

class Decoder
{
  public:
    Decoder(const QByteArray& data, bool bigEndian) : m_d(data), m_big(bigEndian), m_explicit(true)
    {
      // explicit VR if the bytes after the first tag are a VR (as nibabel does it)
      m_explicit = data.size() >= 6 && knownVR(data.mid(4, 2));
    }

    bool parse(QList<Element>& elements)
    {
      qint64 pos = 0;
      return dataSet(pos, m_d.size(), false, 0, elements);
    }

  private:
    quint16 u16(qint64 pos) const
    {
      const uchar* p = reinterpret_cast<const uchar*>(m_d.constData()) + pos;
      return m_big ? qFromBigEndian<quint16>(p) : qFromLittleEndian<quint16>(p);
    }

    quint32 u32(qint64 pos) const
    {
      const uchar* p = reinterpret_cast<const uchar*>(m_d.constData()) + pos;
      return m_big ? qFromBigEndian<quint32>(p) : qFromLittleEndian<quint32>(p);
    }

    quint32 tagAt(qint64 pos) const
    {
      return (static_cast<quint32>(u16(pos)) << 16) | u16(pos + 2);
    }

    bool zeros(qint64 pos, qint64 end) const
    {
      for(qint64 i=pos; i < end; i++) {
        if(m_d[static_cast<int>(i)] != '\0')
          return false;
      }
      return true;
    }

    // VR of an element in implicit VR
    static QByteArray implicitVR(quint32 tag)
    {
      const CMedIOHeaderDictionary::Entry* entry = CMedIOHeaderDictionary::entry(tag);
      if(entry != NULL)
        return QByteArray(entry->vr);
      if(tag == 0x00540016 || tag == 0x00540013)
        return "SQ";
      if(tag == SpecificCharacterSet) return "CS";
      if(tag == CodeValue || tag == CodingSchemeDesignator || tag == CodingSchemeVersion) return "SH";
      if(tag == CodeMeaning) return "LO";
      if((tag >> 16) == PmodGroup && (tag & 0xFF00) == 0)
        return "LO"; // private creator
      return "UN";
    }

    // elements of a data set up to end (or the item delimitation of an item
    // of undefined length)
    bool dataSet(qint64& pos, qint64 end, bool undefinedLength, int depth, QList<Element>& elements)
    {
      if(depth > 8)
        return false;

      while(pos + 8 <= end) {
        // the padding of the extension ends the data set
        if(depth == 0 && zeros(pos, end))
          break;

        const quint32 tag = tagAt(pos);
        if(tag == ItemDelimitation) {
          pos += 8;
          return undefinedLength;
        }
        if((tag >> 16) == 0xFFFE)
          return false;

        Element e;
        e.tag = tag;
        quint32 length = 0;
        if(m_explicit) {
          e.vr = m_d.mid(static_cast<int>(pos + 4), 2);
          if(knownVR(e.vr) == false)
            return false;
          if(longVR(e.vr)) {
            if(pos + 12 > end)
              return false;
            length = u32(pos + 8);
            pos += 12;
          } else {
            length = u16(pos + 6);
            pos += 8;
          }
        } else {
          e.vr = implicitVR(tag);
          length = u32(pos + 4);
          pos += 8;
        }

        if(e.vr == "SQ" || length == 0xFFFFFFFF) {
          const bool undefinedSequence = (length == 0xFFFFFFFF);
          const qint64 sequenceEnd = undefinedSequence ? end : pos + length;
          if(sequenceEnd > end || items(pos, sequenceEnd, undefinedSequence, depth + 1, e.items) == false)
            return false;
          if(undefinedSequence == false)
            pos = sequenceEnd;
        } else {
          if(pos + length > end)
            return false;
          e.value = m_d.mid(static_cast<int>(pos), static_cast<int>(length));
          pos += length;
        }
        elements.append(e);
      }

      // the rest of a data set is padding (the size of an extension is a multiple of 16 bytes)
      if(undefinedLength)
        return false;
      pos = end;
      return true;
    }

    // items of a sequence
    bool items(qint64& pos, qint64 end, bool undefinedLength, int depth, QList<QList<Element> >& list)
    {
      while(pos + 8 <= end) {
        const quint32 tag = tagAt(pos);
        const quint32 length = u32(pos + 4);
        pos += 8;
        if(tag == SequenceDelimitation)
          return undefinedLength;
        if(tag != Item)
          return false;

        QList<Element> item;
        if(length == 0xFFFFFFFF) {
          if(dataSet(pos, end, true, depth, item) == false)
            return false;
        } else {
          const qint64 itemEnd = pos + length;
          if(itemEnd > end || dataSet(pos, itemEnd, false, depth, item) == false)
            return false;
          pos = itemEnd;
        }
        list.append(item);
      }
      return undefinedLength == false && pos == end;
    }

    const QByteArray& m_d;
    bool m_big;
    bool m_explicit;
};

const Element* find(const QList<Element>& elements, quint32 tag)
{
  for(int i=0; i < elements.size(); i++) {
    if(elements[i].tag == tag)
      return &elements[i];
  }
  return NULL;
}

// text of a string value
QString decodedText(const QByteArray& value, bool utf8)
{
  QByteArray b = value;
  while(b.endsWith('\0') || b.endsWith(' '))
    b.chop(1);
  return (utf8 ? QString::fromUtf8(b) : QString::fromLatin1(b)).trimmed();
}

// JSON value of a DICOM value, Undefined if it cannot be converted
QJsonValue decodedValue(const CMedIOHeaderDictionary::Entry& entry, const Element& e, bool utf8)
{
  const QString s = decodedText(e.value, utf8);

  switch(entry.type) {
    case CMedIOHeaderDictionary::String:
      if(s.isEmpty() == false)
        return QJsonValue(s);
    break;

    case CMedIOHeaderDictionary::Number: {
      bool ok = false;
      const double d = s.section(QChar('\\'), 0, 0).trimmed().toDouble(&ok);
      if(ok && std::isfinite(d))
        return MedIOHeaderMapping::number(d);
    }
    break;

    case CMedIOHeaderDictionary::Date: {
      const QString digits = QString(s).remove(QChar('.')).remove(QChar('-'));
      if(QRegExp("^\\d{8}$").exactMatch(digits))
        return QJsonValue(digits.left(4) + "-" + digits.mid(4, 2) + "-" + digits.mid(6, 2));
    }
    break;

    case CMedIOHeaderDictionary::Time: {
      QRegExp tm("^(\\d{2}):?(\\d{2})(?::?(\\d{2})(\\.\\d{1,6})?)?$");
      if(tm.exactMatch(s)) {
        QString t = tm.cap(1) + ":" + tm.cap(2);
        if(tm.cap(3).isEmpty() == false)
          t += ":" + tm.cap(3) + tm.cap(4);
        return QJsonValue(t);
      }
    }
    break;

    case CMedIOHeaderDictionary::DateTime: {
      QRegExp dt("^(\\d{4})(\\d{2})(\\d{2})(\\d{2})(\\d{2})(\\d{2})?(\\.\\d{1,6})?([+-]\\d{4})?$");
      if(dt.exactMatch(s)) {
        QString iso = dt.cap(1) + "-" + dt.cap(2) + "-" + dt.cap(3) + "T" + dt.cap(4) + ":" + dt.cap(5);
        if(dt.cap(6).isEmpty() == false)
          iso += ":" + dt.cap(6) + dt.cap(7);
        const QString zone = dt.cap(8);
        if(zone == "+0000" || zone == "-0000")
          iso += "Z";
        else if(zone.isEmpty() == false)
          iso += zone.left(3) + ":" + zone.mid(3);
        return QJsonValue(iso);
      }
    }
    break;

    case CMedIOHeaderDictionary::Strings: {
      QJsonArray array;
      foreach(const QString& part, s.split(QChar('\\'))) {
        if(part.trimmed().isEmpty() == false)
          array.append(part.trimmed());
      }
      if(array.isEmpty() == false)
        return QJsonValue(array);
    }
    break;

    case CMedIOHeaderDictionary::Code: {
      if(e.items.isEmpty())
        break;
      static const struct { const char* key; quint32 tag; } fields[] = {
        { "CodeValue", CodeValue }, { "CodingSchemeDesignator", CodingSchemeDesignator },
        { "CodingSchemeVersion", CodingSchemeVersion }, { "CodeMeaning", CodeMeaning },
      };
      QJsonObject code;
      for(unsigned i=0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        const Element* field = find(e.items.first(), fields[i].tag);
        if(field != NULL && decodedText(field->value, utf8).isEmpty() == false)
          code[QLatin1String(fields[i].key)] = decodedText(field->value, utf8);
      }
      if(code.isEmpty() == false)
        return QJsonValue(code);
    }
    break;
  }

  return QJsonValue(QJsonValue::Undefined);
}

// attributes of the elements of a data set (sequence: tag of the sequence
// holding it, 0 for the top level)
void decodeAttributes(const QList<Element>& elements, quint32 sequence, bool utf8, QJsonObject& json)
{
  for(int i=0; i < elements.size(); i++) {
    const Element& e = elements[i];
    const CMedIOHeaderDictionary::Entry* entry = CMedIOHeaderDictionary::entry(e.tag);
    if(entry != NULL && entry->sequence == sequence && entry->scope == CMedIOHeaderDictionary::MainHeader) {
      const QJsonValue value = decodedValue(*entry, e, utf8);
      if(value.isUndefined() == false && CMedIOHeaderDictionary::isValid(*entry, value))
        json[QLatin1String(entry->keyword)] = value;
    } else if(sequence == 0 && e.vr == "SQ" && e.items.isEmpty() == false) {
      // sequences holding attributes (first item)
      decodeAttributes(e.items.first(), e.tag, utf8, json);
    }
  }
}

// FD values
QJsonArray decodedDoubles(const QByteArray& value, bool bigEndian, double factor)
{
  QJsonArray array;
  if(value.size() % 8 != 0)
    return array;
  for(int i=0; i < value.size(); i += 8) {
    const uchar* p = reinterpret_cast<const uchar*>(value.constData()) + i;
    const quint64 bits = bigEndian ? qFromBigEndian<quint64>(p) : qFromLittleEndian<quint64>(p);
    double d = 0.0;
    memcpy(&d, &bits, sizeof(d));
    if(std::isfinite(d) == false)
      return QJsonArray();
    array.append(MedIOHeaderMapping::number(d * factor));
  }
  return array;
}

// a header extension of esize bytes (esize, ecode, data and zero padding)
QByteArray extension(qint32 code, const QByteArray& data, qint64 esize, bool swap)
{
  qint32 size = static_cast<qint32>(esize);
  qint32 ecode = code;
  if(swap) {
    BSWAP_32(size);
    BSWAP_32(ecode);
  }

  QByteArray out;
  out.append(reinterpret_cast<const char*>(&size), sizeof(size));
  out.append(reinterpret_cast<const char*>(&ecode), sizeof(ecode));
  out.append(data);
  out.append(QByteArray(static_cast<int>(esize - out.size()), '\0'));
  return out;
}

qint64 paddedSize(qint64 dataSize)
{
  const qint64 esize = 8 + dataSize;
  return esize + (16 - esize % 16) % 16;
}

} // namespace

//=============================================================================================

QList<Extension> read(QIODevice& file, qint64 headerSize, qint64 end, bool swap, bool* present, qint64* used)
{
  ENTER();
  QList<Extension> list;
  char extender[4] = { 0, 0, 0, 0 };

  const bool any = file.seek(headerSize) && file.read(extender, 4) == 4 && extender[0] != 0;
  if(present)
    *present = any;
  if(used)
    *used = 0;
  if(any == false) {
    LEAVE();
    return list;
  }

  qint64 pos = headerSize + 4;
  while(pos + 8 <= end && file.seek(pos)) {
    qint32 esize = 0;
    qint32 ecode = 0;
    if(file.read(reinterpret_cast<char*>(&esize), 4) != 4 ||
       file.read(reinterpret_cast<char*>(&ecode), 4) != 4)
      break;
    if(swap) {
      BSWAP_32(esize);
      BSWAP_32(ecode);
    }
    if(esize < 8 || pos + esize > end) {
      W("invalid NIfTI header extension size %d at offset %lld", esize, pos);
      break;
    }

    Extension e;
    e.code = ecode;
    e.data = file.read(esize - 8);
    if(e.data.size() != esize - 8)
      break;
    list.append(e);
    pos += esize;
    if(used)
      *used = pos - headerSize - 4;
  }

  LEAVE();
  return list;
}

QByteArray dicomDataSet(const QJsonObject& json)
{
  ENTER();

  // the attributes of the metadata (e.g. Units "BQML" for the BIDS "Bq/mL")
  const QJsonObject attributes = MedIOHeaderMapping::attributesOfNiftiJson(json);
  Encoder encoder(false);

  // data sets of the top level and of the sequences, ordered by tag
  QMap<quint32, QByteArray> top;
  QMap<quint32, QMap<quint32, QByteArray> > sequences;

  foreach(const CMedIOHeaderDictionary::Entry* entry, CMedIOHeaderDictionary::entries()) {
    if(entry->scope != CMedIOHeaderDictionary::MainHeader)
      continue;
    const QJsonValue value = attributes.value(QLatin1String(entry->keyword));
    if(value.isUndefined() || value.isNull() || CMedIOHeaderDictionary::isValid(*entry, value) == false)
      continue;

    QByteArray element;
    if(entry->type == CMedIOHeaderDictionary::Code) {
      const QByteArray item = encoder.codeItem(value.toObject());
      if(item.isEmpty() == false)
        element = encoder.sequence(entry->tag, item);
    } else {
      const QByteArray v = encoder.value(*entry, value);
      if(v.isEmpty() == false)
        element = encoder.element(entry->tag, entry->vr, v);
    }
    if(element.isEmpty())
      continue;

    if(entry->sequence == 0)
      top[entry->tag] = element;
    else
      sequences[entry->sequence][entry->tag] = element;
  }

  for(QMap<quint32, QMap<quint32, QByteArray> >::const_iterator s = sequences.constBegin(); s != sequences.constEnd(); ++s) {
    QByteArray item;
    foreach(const QByteArray& element, s.value())
      item.append(element);
    top[s.key()] = encoder.sequence(s.key(), item);
  }

  // frame timing in the private tags of PMOD (at most 8191 values fit into an FD element)
  const QJsonArray starts = json.value("FrameTimesStart").toArray();
  const QJsonArray durations = json.value("FrameDuration").toArray();
  if(starts.isEmpty() == false && starts.size() == durations.size() && starts.size() <= 0xFFFF / 8 &&
     numbers(starts) && numbers(durations)) {
    top[(PmodGroup << 16) | 0x0010] = encoder.element((PmodGroup << 16) | 0x0010, "LO", encoder.text(PmodCreator, "LO"));
    top[(PmodGroup << 16) | 0x1001] = encoder.element((PmodGroup << 16) | 0x1001, "FD", encoder.doubles(starts, 1.0));
    top[(PmodGroup << 16) | 0x1004] = encoder.element((PmodGroup << 16) | 0x1004, "FD", encoder.doubles(durations, 1000.0));
  }

  if(top.isEmpty()) {
    LEAVE();
    return QByteArray();
  }

  if(encoder.nonAscii())
    top[SpecificCharacterSet] = encoder.element(SpecificCharacterSet, "CS", "ISO_IR 100");

  QByteArray dataSet;
  foreach(const QByteArray& element, top)
    dataSet.append(element);

  // readers like nibabel remove the trailing zero bytes of an extension (its
  // padding): a data set ending with a zero byte is completed by padding
  if(dataSet.endsWith('\0'))
    dataSet.append(encoder.element(DataSetTrailingPadding, "OB", QByteArray("  ")));

  LEAVE();
  return dataSet;
}

QJsonObject metadataOfDicomDataSet(const QByteArray& data, bool* pmod)
{
  ENTER();
  QJsonObject json;
  if(pmod)
    *pmod = false;

  // little endian (as written by libmedio and PMOD and read by nibabel),
  // big endian (nibabel writes it for big endian files if asked to)
  bool bigEndian = false;
  QList<Element> elements;
  if(Decoder(data, false).parse(elements) == false) {
    QList<Element> big;
    if(Decoder(data, true).parse(big)) {
      elements = big;
      bigEndian = true;
    } else {
      W("unable to read the DICOM header extension of the NIfTI file");
      if(elements.isEmpty()) {
        LEAVE();
        return json;
      }
    }
  }

  const Element* charset = find(elements, SpecificCharacterSet);
  const bool utf8 = charset != NULL && charset->value.contains("ISO_IR 192");
  decodeAttributes(elements, 0, utf8, json);

  // frame timing of PMOD: the private block of the creator "PMOD_1"
  for(quint32 block=0x10; block <= 0xFF; block++) {
    const Element* creator = find(elements, (PmodGroup << 16) | block);
    if(creator == NULL || decodedText(creator->value, false) != QLatin1String(PmodCreator))
      continue;
    if(pmod)
      *pmod = true;

    const Element* starts = find(elements, (PmodGroup << 16) | (block << 8) | 0x01);
    const Element* durations = find(elements, (PmodGroup << 16) | (block << 8) | 0x04);
    if(starts != NULL && durations != NULL) {
      const QJsonArray s = decodedDoubles(starts->value, bigEndian, 1.0);
      const QJsonArray d = decodedDoubles(durations->value, bigEndian, 0.001);
      if(s.isEmpty() == false && s.size() == d.size()) {
        json["FrameTimesStart"] = s;
        json["FrameDuration"] = d;
      }
    }
    break;
  }

  LEAVE();
  return json;
}

QByteArray metadataExtensions(const QJsonObject& json, bool swap, qint64 size)
{
  ENTER();
  QByteArray block;
  if(json.isEmpty()) {
    LEAVE();
    return block;
  }

  const QByteArray dicom = dicomDataSet(json);
  const QByteArray text = QJsonDocument(json).toJson(QJsonDocument::Compact);

  // the JSON extension first (as written by earlier versions), the last
  // extension fills the room
  const qint64 jsonSize = paddedSize(text.size());
  qint64 dicomSize = dicom.isEmpty() ? 0 : paddedSize(dicom.size());
  qint64 lastSize = dicom.isEmpty() ? jsonSize : dicomSize;
  if(size > jsonSize + dicomSize)
    lastSize += size - jsonSize - dicomSize;

  if(dicom.isEmpty()) {
    block.append(extension(JsonCode, text, lastSize, swap));
  } else {
    block.append(extension(JsonCode, text, jsonSize, swap));
    block.append(extension(DicomCode, dicom, lastSize, swap));
  }

  LEAVE();
  return block;
}

qint64 reservedSize(const QJsonObject& json, int volumes)
{
  // every number of the JSON timing may take up to 24 characters (17
  // significant digits, sign, point and exponent), the DICOM data set may
  // need its trailing padding (16 bytes)
  QJsonArray widest;
  for(int i=0; i < volumes; i++)
    widest.append(-1.2345678901234567e-300);
  QJsonObject timing = json;
  timing["FrameTimesStart"] = widest;
  timing["FrameDuration"] = widest;
  return metadataExtensions(timing, false).size() + 16;
}

bool prepareHeaderMetadata(QIODevice& file, bool pair, const QJsonObject& attributes, const QByteArray& descrip,
                           qint64 headerSize, qint64 voxOffset, bool swap, HeaderMetadata& metadata)
{
  ENTER();
  metadata.setDescrip = false;
  metadata.descrip.clear();
  metadata.voxOffset = voxOffset;
  metadata.block.clear();
  metadata.fileSize = -1;

  const QJsonObject json = MedIOHeaderMapping::niftiJson(attributes);

  // descrip as written by PMOD ("<patient name> [<study type>] <study
  // description>"), unless it was set to something else than libmedio's
  // default or a descrip of this form (e.g. of attributes changed since)
  const QByteArray current = descrip.trimmed();
  const QByteArray description = niftiDescription(attributes).toLatin1().left(79);  // descrip holds 79 characters
  const bool replaceable = current.isEmpty() || current == "converted by libmedio" ||
                           attributesOfNiftiDescription(QString::fromLatin1(current)).isEmpty() == false;
  if(replaceable && description != current && (description.isEmpty() == false || current.isEmpty() == false)) {
    metadata.setDescrip = true;
    metadata.descrip = description;
  }

  // extensions present (files opened for reading and writing): those of the
  // metadata are replaced as a whole, so that no removed value is left behind
  const qint64 dataStart = qMax(voxOffset, headerSize + 4);
  const qint64 fileSize = file.size();
  bool present = false;
  bool own = true;
  QList<Extension> existing;
  if(file.isReadable() && fileSize >= headerSize + 4)
    existing = read(file, headerSize, pair ? fileSize : qMin(dataStart, fileSize), swap, &present);
  foreach(const Extension& e, existing)
    own = own && (e.code == JsonCode || e.code == DicomCode);

  if(json.isEmpty()) {
    // no attributes: remove the metadata extensions of the file
    if(present && own) {
      if(pair) {
        metadata.block = QByteArray(4, '\0');
        metadata.fileSize = headerSize + 4;
      } else {
        metadata.block = QByteArray(static_cast<int>(qMin(dataStart, fileSize) - headerSize), '\0');
      }
    }
    RETURN(true);
    return true;
  }

  const QByteArray indicator("\x01\0\0\0", 4);
  const QByteArray extensions = metadataExtensions(json, swap);

  if(pair) {
    // the extensions end the header file
    metadata.block = indicator + extensions;
    metadata.fileSize = headerSize + metadata.block.size();
    RETURN(true);
    return true;
  }

  // single file: the extensions fill the room up to vox_offset
  qint64 offset = voxOffset;
  const qint64 required = headerSize + 4 + extensions.size();
  if(offset < required) {
    // the voxel data must not be moved
    if(offset > 0 && fileSize > offset) {
      E("no space for the attributes before the voxel data");
      RETURN(false);
      return false;
    }
    offset = required;
    metadata.voxOffset = offset;
  }
  const qint64 room = offset - headerSize - 4;
  metadata.block = indicator + metadataExtensions(json, swap, room - room % 16) +
                   QByteArray(static_cast<int>(room % 16), '\0');

  RETURN(true);
  return true;
}

//=============================================================================================

QString niftiDescription(const QJsonObject& attributes)
{
  const QString name = attributes.value("PatientName").toString().trimmed();
  const QString type = attributes.value("ECAT7").toObject().value("Study_Type").toString().trimmed();
  QString description = attributes.value("StudyDescription").toString().trimmed();
  if(description.isEmpty())
    description = attributes.value("SeriesDescription").toString().trimmed();
  if(name.isEmpty() && type.isEmpty() && description.isEmpty())
    return QString();

  return QString("%1 [%2] %3").arg(name, type, description).trimmed();
}

QJsonObject attributesOfNiftiDescription(const QString& descrip)
{
  QJsonObject attributes;
  const QString s = descrip.trimmed();

  // "<name> [<type>] <description>"
  const int open = s.indexOf(QChar('['));
  const int close = (open >= 0) ? s.indexOf(QChar(']'), open) : -1;
  if(open < 0 || close < 0 ||
     (open > 0 && s[open - 1] != QChar(' ')) ||
     (close + 1 < s.size() && s[close + 1] != QChar(' ')))
    return attributes;

  const QString name = s.left(open).trimmed();
  const QString type = s.mid(open + 1, close - open - 1).trimmed();
  const QString description = s.mid(close + 1).trimmed();
  if(name.isEmpty() == false)
    attributes["PatientName"] = name;
  if(description.isEmpty() == false)
    attributes["StudyDescription"] = description;
  if(type.isEmpty() == false)
    attributes["ECAT7"] = QJsonObject{{"Study_Type", type}};
  return attributes;
}

//=============================================================================================

QJsonObject fileMetadata(CNIFTIFile& file, const CNIFTIMainHeader* header, const QString& fileName)
{
  ENTER();

  const bool nifti1 = (header->mainHeaderType() == CNIFTIMainHeader::NIFTI1MainHeader);
  const CNIFTI1MainHeader* h1 = nifti1 ? static_cast<const CNIFTI1MainHeader*>(header) : NULL;
  const CNIFTI2MainHeader* h2 = nifti1 ? NULL : static_cast<const CNIFTI2MainHeader*>(header);

  // JSON metadata: header extension or sidecar file
  QJsonObject json = nifti1 ? h1->readHeaderExtension(file) : h2->readHeaderExtension(file);
  if(json.isEmpty()) {
    QFile sidecar(niftiSidecarFileName(fileName));
    if(sidecar.open(QIODevice::ReadOnly))
      json = QJsonDocument::fromJson(sidecar.readAll()).object();
  }
  const bool metadata = (json.isEmpty() == false);

  // values of a DICOM extension the JSON metadata does not have
  const QSysInfo::Endian order = nifti1 ? h1->byteOrder() : h2->byteOrder();
  const bool swap = (order != QSysInfo::ByteOrder);
  const qint64 end = file.isPair() ? file.size() : (nifti1 ? static_cast<qint64>(h1->vox_Offset()) : h2->vox_Offset());
  bool pmod = false;
  foreach(const Extension& e, read(file, header->rawDataSize(), end, swap)) {
    if(e.code != DicomCode)
      continue;
    const QJsonObject dicom = metadataOfDicomDataSet(e.data, &pmod);
    for(QJsonObject::const_iterator it = dicom.begin(); it != dicom.end(); ++it)
      setIfMissing(json, it.key(), it.value());
    break;
  }

  // the descrip of files written by PMOD
  if(pmod && metadata == false) {
    const char* descrip = nifti1 ? h1->descrip() : h2->descrip();
    const QJsonObject described = attributesOfNiftiDescription(QString::fromLatin1(descrip, static_cast<int>(qstrnlen(descrip, 80))));
    for(QJsonObject::const_iterator it = described.begin(); it != described.end(); ++it)
      setIfMissing(json, it.key(), it.value());
  }

  LEAVE();
  return json;
}

} // namespace NIFTIExtensions
