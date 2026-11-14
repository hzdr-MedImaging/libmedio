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

//! @file CMedIOHeaderMapping.cpp
//! @brief private: format independent header attributes of the headers (see CMedIOHeaderMapping.h)

#include "CMedIOHeaderMapping.h"
#include "CMedIOHeaderDictionary.h"
#include "CMedIOHeader.h"
#include "CECATMainHeader.h"
#include "CECATSubHeader.h"
#include "CNIFTIMainHeader.h"

#include <QDateTime>
#include <QJsonArray>
#include <QRegExp>
#include <QScopedPointer>

#include <rtdebug.h>

namespace MedIOHeaderMapping {

//=============================================================================================
// adapters

const Adapter* adapter(const CMedIOHeader& header)
{
  switch(header.headerFormat()) {
    case CMedIOHeader::ECATMainHeader:
      if(static_cast<const CECATMainHeader&>(header).mainHeaderType() == CECATMainHeader::ECAT7MainHeader)
        return ecat7MainHeaderAdapter();
    break;

    case CMedIOHeader::ECATSubHeader:
      if(static_cast<const CECATSubHeader&>(header).subHeaderType() == CECATSubHeader::ECAT7_Image)
        return ecat7ImageSubHeaderAdapter();
    break;

    case CMedIOHeader::PhilipsMainHeader:
      return philipsMainHeaderAdapter();

    case CMedIOHeader::PhilipsSubHeader:
      return philipsSubHeaderAdapter();

    case CMedIOHeader::ConcordeMicroPetMainHeader:
      return concordeMainHeaderAdapter();

    case CMedIOHeader::ConcordeMicroPetFrameHeader:
      return concordeFrameHeaderAdapter();

    case CMedIOHeader::NIFTIMainHeader:
      if(static_cast<const CNIFTIMainHeader&>(header).mainHeaderType() == CNIFTIMainHeader::NIFTI1MainHeader)
        return nifti1MainHeaderAdapter();
      if(static_cast<const CNIFTIMainHeader&>(header).mainHeaderType() == CNIFTIMainHeader::NIFTI2MainHeader)
        return nifti2MainHeaderAdapter();
    break;

    default:
    break;
  }
  return NULL;
}

namespace {

const Field* findField(const Field* fields, int count, const QString& key)
{
  for(int i=0; i < count; i++) {
    if(key == QLatin1String(fields[i].key))
      return &fields[i];
  }
  return NULL;
}

bool isArrayOfObjects(const QJsonValue& value)
{
  if(value.isArray() == false)
    return false;
  const QJsonArray array = value.toArray();
  for(int i=0; i < array.size(); i++) {
    if(array[i].isObject() == false)
      return false;
  }
  return true;
}

} // namespace

//=============================================================================================
// attributes of a header

QJsonObject attributes(const CMedIOHeader& header, const QJsonObject& other)
{
  QJsonObject json = other;

  const Adapter* a = adapter(header);
  if(a != NULL) {
    // the keys stored by the format: the values of its fields replace
    // values kept for keys the format could not store before
    for(int i=0; i < a->keyCount; i++) {
      const QJsonValue value = a->keys[i].get(header);
      if(value.isUndefined() == false)
        json[QString::fromLatin1(a->keys[i].key)] = value;
    }

    QJsonObject native;
    for(int i=0; i < a->nativeCount; i++) {
      const QJsonValue value = a->native[i].get(header);
      if(value.isUndefined() == false)
        native[QString::fromLatin1(a->native[i].key)] = value;
    }
    json[QString::fromLatin1(a->section)] = native;
  }

  return json;
}

bool update(CMedIOHeader& header, QJsonObject& other, const QJsonObject& changes)
{
  ENTER();

  const Adapter* a = adapter(header);
  const QString section = a ? QString::fromLatin1(a->section) : QString();
  const QJsonObject before = attributes(header, other);

  // keys given as tag are stored by their keyword ("(0010,1030)" -> "PatientWeight")
  QJsonObject keys;
  for(QJsonObject::const_iterator it = changes.begin(); it != changes.end(); ++it) {
    const QString key = CMedIOHeaderDictionary::normalizedKey(it.key());
    if(keys.contains(key)) {
      E("attribute %s given twice", key.toLatin1().constData());
      RETURN(false);
      return false;
    }
    keys[key] = it.value();
  }

  // native fields of the format of the header (only changed fields, so that
  // a changed attribute wins over the unchanged field in the same object)
  if(a != NULL && keys.contains(section)) {
    const QJsonValue value = keys.take(section);
    if(value.isObject()) {
      const QJsonObject fields = value.toObject();
      const QJsonObject old = before.value(section).toObject();
      for(QJsonObject::const_iterator it = fields.begin(); it != fields.end(); ++it) {
        const Field* field = findField(a->native, a->nativeCount, it.key());
        if(field == NULL) {
          E("%s has no field %s", section.toLatin1().constData(), it.key().toLatin1().constData());
          RETURN(false);
          return false;
        }
        if(it.value() == old.value(it.key()))
          continue;
        if(field->set(header, it.value()) == false) {
          E("invalid value of %s.%s", section.toLatin1().constData(), it.key().toLatin1().constData());
          RETURN(false);
          return false;
        }
      }

      // setters can change other fields (e.g. Philips date and time fields
      // set the deprecated fields of the same time): set them again
      for(int pass=0; pass < 2; pass++) {
        for(QJsonObject::const_iterator it = fields.begin(); it != fields.end(); ++it) {
          const Field* field = findField(a->native, a->nativeCount, it.key());
          if(field->get(header) != it.value())
            field->set(header, it.value());
        }
      }
    } else if(value.isNull() == false) {
      E("the native fields of %s have to be an object", section.toLatin1().constData());
      RETURN(false);
      return false;
    }
  }

  // attributes, other formats and unknown keys
  for(QJsonObject::const_iterator it = keys.begin(); it != keys.end(); ++it) {
    const QString key = it.key();
    const QJsonValue value = it.value();

    const CMedIOHeaderDictionary::Entry* k = CMedIOHeaderDictionary::entry(key);
    if(value.isNull() == false && value.isUndefined() == false) {
      if((k != NULL && CMedIOHeaderDictionary::isValid(*k, value) == false) ||
         (key == QLatin1String(CMedIOHeaderDictionary::FramesKey) && isArrayOfObjects(value) == false)) {
        E("invalid value of attribute %s", key.toLatin1().constData());
        RETURN(false);
        return false;
      }
    }

    // unchanged
    if(value == before.value(key) || (value.isNull() && before.contains(key) == false))
      continue;

    const Field* field = a ? findField(a->keys, a->keyCount, key) : NULL;
    if(field != NULL) {
      other.remove(key);
      if(field->set(header, value) == false) {
        // the format cannot store the value: keep it in the header object
        field->set(header, QJsonValue(QJsonValue::Null));
        other[key] = value;
      }
    } else if(value.isNull()) {
      other.remove(key);
    } else {
      other[key] = value;
    }
  }

  RETURN(true);
  return true;
}

void transfer(const QJsonObject& source, CMedIOHeader& target, const QStringList& skip)
{
  ENTER();

  const Adapter* a = adapter(target);
  const QString section = a ? QString::fromLatin1(a->section) : QString();

  for(QJsonObject::const_iterator it = source.begin(); it != source.end(); ++it) {
    if(skip.contains(it.key()))
      continue;

    // native fields of the target format: the fields that can be set (a
    // field that cannot, e.g. a read only one, does not stop the others),
    // set together (setters can change other fields, see update())
    if(a != NULL && it.key() == section && it.value().isObject()) {
      const QJsonObject fields = it.value().toObject();
      QJsonObject valid;
      for(QJsonObject::const_iterator field = fields.begin(); field != fields.end(); ++field) {
        QScopedPointer<CMedIOHeader> copy(target.clone());
        QJsonObject change;
        change[section] = QJsonObject{{field.key(), field.value()}};
        if(copy.isNull() == false && copy->updateAttributes(change))
          valid[field.key()] = field.value();
        else
          W("native field %s.%s not copied", section.toLatin1().constData(), field.key().toLatin1().constData());
      }
      QJsonObject change;
      change[section] = valid;
      if(target.updateAttributes(change) == false)
        W("native fields of %s not copied", section.toLatin1().constData());
      continue;
    }

    QJsonObject change;
    change[it.key()] = it.value();
    if(target.updateAttributes(change) == false)
      W("attribute %s not copied", it.key().toLatin1().constData());
  }

  LEAVE();
}

QStringList frameKeys()
{
  return QStringList() << QString::fromLatin1(CMedIOHeaderDictionary::FramesKey)
                       << "FrameTimesStart" << "FrameDuration";
}

//=============================================================================================
// values

QJsonValue number(double value, int digits)
{
  if(std::isfinite(value) == false)
    return QJsonValue(value);
  return QJsonValue(QString::number(value, 'g', digits).toDouble());
}

QJsonValue floatNumber(float value)
{
  if(std::isfinite(value) == false)
    return QJsonValue(static_cast<double>(value));

  // the shortest decimal number giving the float
  for(int digits = 6; digits < 9; digits++) {
    const double d = QString::number(static_cast<double>(value), 'g', digits).toDouble();
    if(static_cast<float>(d) == value)
      return QJsonValue(d);
  }
  return number(static_cast<double>(value), 9);
}

QJsonValue floatNumber(float value, double factor)
{
  // float precision: 7 significant digits
  return number(floatNumber(value).toDouble() * factor, 7);
}

QJsonValue text(const char* latin1)
{
  return text(QString::fromLatin1(latin1));
}

QJsonValue text(const QString& string)
{
  const QString s = string.trimmed();
  return s.isEmpty() ? QJsonValue(QJsonValue::Undefined) : QJsonValue(s);
}

QByteArray latin1(const QJsonValue& value)
{
  return value.isString() ? value.toString().toLatin1() : QByteArray();
}

QJsonValue dateTimeOfUnixTime(qint64 seconds)
{
  if(seconds == 0)
    return QJsonValue(QJsonValue::Undefined);
  return QJsonValue(QDateTime::fromMSecsSinceEpoch(seconds * 1000, Qt::UTC).toString(Qt::ISODate));
}

bool unixTimeOfDateTime(const QJsonValue& value, qint64& seconds)
{
  seconds = 0;
  if(value.isNull())
    return true;
  if(value.isString() == false)
    return false;

  const QString s = value.toString();
  QDateTime dt = QDateTime::fromString(s, Qt::ISODate);
  if(dt.isValid() == false)
    return false;

  // without time zone: local time of this machine (as in DICOM, where date
  // and time are the local time of the scanner)
  if(QRegExp(".*(Z|[+-]\\d{2}:\\d{2})$").exactMatch(s) == false)
    dt = QDateTime(dt.date(), dt.time(), Qt::LocalTime);

  seconds = dt.toMSecsSinceEpoch() / 1000;
  return true;
}

QJsonValue dateOfUnixTime(qint64 seconds)
{
  if(seconds == 0)
    return QJsonValue(QJsonValue::Undefined);

  // nearest midnight UTC
  const qint64 days = static_cast<qint64>(std::floor((static_cast<double>(seconds) + 43200.0) / 86400.0));
  return QJsonValue(QDate(1970, 1, 1).addDays(days).toString(Qt::ISODate));
}

bool unixTimeOfDate(const QJsonValue& value, qint64& seconds)
{
  seconds = 0;
  if(value.isNull())
    return true;
  const QDate d = date(value);
  if(d.isValid() == false)
    return false;
  seconds = static_cast<qint64>(QDate(1970, 1, 1).daysTo(d)) * 86400;
  return true;
}

QDate date(const QJsonValue& value)
{
  return value.isString() ? QDate::fromString(value.toString(), Qt::ISODate) : QDate();
}

QJsonValue strings(const QJsonValue& string)
{
  if(string.isUndefined())
    return string;
  QJsonArray array;
  array.append(string);
  return QJsonValue(array);
}

QJsonValue firstString(const QJsonValue& strings)
{
  const QJsonArray array = strings.toArray();
  return array.isEmpty() ? QJsonValue(QJsonValue::Null) : array.first();
}

QJsonValue code(const QString& value, const QString& scheme, const QString& version, const QString& meaning)
{
  QJsonObject item;
  if(value.trimmed().isEmpty() == false)   item["CodeValue"] = value.trimmed();
  if(scheme.trimmed().isEmpty() == false)  item["CodingSchemeDesignator"] = scheme.trimmed();
  if(version.trimmed().isEmpty() == false) item["CodingSchemeVersion"] = version.trimmed();
  if(meaning.trimmed().isEmpty() == false) item["CodeMeaning"] = meaning.trimmed();
  return item.isEmpty() ? QJsonValue(QJsonValue::Undefined) : QJsonValue(item);
}

} // namespace MedIOHeaderMapping
