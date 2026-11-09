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

//! @file CMedIOMetadataMap.h
//! @brief private: mapping between the headers of the formats and the
//!        format independent metadata (CMedIOHeader::metadata())
//!
//! Every header type with metadata has an adapter: a table of the metadata
//! keys (DICOM keywords, see CMedIOMetadata) it stores in its own fields,
//! and a table of its native fields (the section named after the format).
//!
//! - A field reads its value from the header (QJsonValue::Undefined if it is
//!   not set) and writes it (QJsonValue::Null: not set). The value has been
//!   checked against the key before (CMedIOMetadata::isValid()).
//! - A metadata key setter returns false if the format cannot store the value
//!   (e.g. PatientSex "O" in ECAT7): the field is cleared then and the value
//!   is kept in the metadata of the header object, like keys of other formats.
//! - A native field setter returns false if the value does not fit.
//! - The native sections only hold fields describing the data: fields that
//!   describe the size, data type, scaling, geometry or layout of the image
//!   data are not part of the metadata.
//!
//! Units and times:
//! - the values of the metadata keys have the units of their DICOM attributes
//! - Unix times of the formats (seconds since 1970, UTC) are written as
//!   ISO 8601 with "Z"; date and time without time zone are taken as UTC
//! - dates stored as Unix time (ECAT7 birth date) are the date of the nearest
//!   midnight UTC, so that a birth date written at local midnight of any time
//!   zone up to 12 hours from UTC gives the same date on every machine

#ifndef CMEDIOMETADATAMAP_H
#define CMEDIOMETADATAMAP_H

#include <QByteArray>
#include <QtGlobal>
#include <QDate>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>

#include <cmath>
#include <limits>

class CMedIOHeader;
class CNIFTIFile;
class CNIFTIMainHeader;

namespace MedIOMetadata {

//---------------------------------------------------------------------------------------------
// adapters

typedef QJsonValue (*Getter)(const CMedIOHeader& header);
typedef bool (*Setter)(CMedIOHeader& header, const QJsonValue& value);

struct Field {
  const char* key;   //!< DICOM keyword or name of the native field
  Getter get;
  Setter set;
};

struct Adapter {
  const char* section;        //!< name of the section of the native fields, e.g. "ECAT7"
  const Field* keys;          //!< metadata keys stored by the format
  int keyCount;
  const Field* native;        //!< native fields
  int nativeCount;
};

//! adapter of a header (NULL if its type has none)
const Adapter* adapter(const CMedIOHeader& header);

// adapters of the formats (defined next to the formats)
const Adapter* ecat7MainHeaderAdapter();
const Adapter* ecat7ImageSubHeaderAdapter();
const Adapter* philipsMainHeaderAdapter();
const Adapter* philipsSubHeaderAdapter();
const Adapter* concordeMainHeaderAdapter();
const Adapter* concordeFrameHeaderAdapter();
const Adapter* nifti1MainHeaderAdapter();
const Adapter* nifti2MainHeaderAdapter();

//---------------------------------------------------------------------------------------------
// NIfTI: the metadata of a NIfTI file are stored in its JSON header extension
// (or a JSON sidecar file, see CNIFTIGeometry.h)

//! metadata of the JSON of a NIfTI file: the keys written by earlier versions of
//! CNIFTIConverter (ECAT7 main header fields at the top level) are moved into
//! the "ECAT7" section and give the metadata keys they store; BIDS keys of
//! other tools give the metadata keys that are not set (see niftiJson())
QJsonObject metadataOfNiftiJson(const QJsonObject& json);

//! JSON to store in a NIfTI file: the metadata (without the native NIfTI
//! fields) plus BIDS keys derived from them (TracerName, InjectedRadioactivity,
//! InjectedRadioactivityUnits, ManufacturersModelName, ReconMethodName,
//! ImageDecayCorrected, DecayCorrectionFactor), empty if there is nothing to store
QJsonObject niftiJson(const QJsonObject& metadata);

//! read the metadata of an opened NIfTI file into its main header
void readNiftiMetadata(CNIFTIFile& file, CNIFTIMainHeader& header);

//---------------------------------------------------------------------------------------------
// metadata of a header (implementation of CMedIOHeader::metadata()/updateMetadata())

//! metadata of a header: its native fields and the keys it stores, plus the
//! metadata kept in the header object (other)
QJsonObject metadata(const CMedIOHeader& header, const QJsonObject& other);

//! apply changes (see CMedIOHeader::updateMetadata())
bool update(CMedIOHeader& header, QJsonObject& other, const QJsonObject& changes);

//! copy metadata into another header, e.g. after a conversion with convertFrom():
//! every key on its own, keys that are invalid for the target are skipped
//! (keys in skip are not copied)
void transfer(const QJsonObject& source, CMedIOHeader& target, const QStringList& skip = QStringList());

//! keys that are not copied by transfer() between main headers: the frame
//! metadata and the frame timing of NIfTI files, which belong to the frames
QStringList frameKeys();

//---------------------------------------------------------------------------------------------
// values

//! number rounded to the given number of significant digits (removes the
//! noise of unit conversions, e.g. 109.77 min * 60 = 6586.2 s)
QJsonValue number(double value, int digits = 15);

//! number of a float field: the shortest decimal number giving the float
QJsonValue floatNumber(float value);

//! value of a float field converted with a factor, rounded to the precision
//! of a float (7 significant digits)
QJsonValue floatNumber(float value, double factor);

//! string of a Latin-1 text field, Undefined if empty
QJsonValue text(const char* latin1);
QJsonValue text(const QString& string);

//! Latin-1 text of a value (empty for Null)
QByteArray latin1(const QJsonValue& value);

//! ISO 8601 date and time (UTC) of a Unix time, Undefined for 0
QJsonValue dateTimeOfUnixTime(qint64 seconds);

//! Unix time of an ISO 8601 date and time (without time zone: UTC), 0 for Null
bool unixTimeOfDateTime(const QJsonValue& value, qint64& seconds);

//! date of the nearest midnight UTC of a Unix time, Undefined for 0
QJsonValue dateOfUnixTime(qint64 seconds);

//! Unix time of midnight UTC of a date, 0 for Null
bool unixTimeOfDate(const QJsonValue& value, qint64& seconds);

//! date of a value (invalid date for Null)
QDate date(const QJsonValue& value);

//! array of strings with one string, Undefined if empty
QJsonValue strings(const QJsonValue& string);

//! first string of an array of strings (Null for Null or an empty array)
QJsonValue firstString(const QJsonValue& strings);

//! code sequence item with the given strings (empty ones are left out), Undefined if all are empty
QJsonValue code(const QString& value, const QString& scheme, const QString& version, const QString& meaning);

//---------------------------------------------------------------------------------------------
// native fields: conversion between the field types and JSON values

inline QJsonValue toJson(const char* value)  { return QJsonValue(QString::fromLatin1(value)); }
inline QJsonValue toJson(const QString& value) { return QJsonValue(value); }
inline QJsonValue toJson(bool value)         { return QJsonValue(value); }
inline QJsonValue toJson(char value)         { return QJsonValue(QString(QChar::fromLatin1(value)).remove(QChar('\0'))); }
inline QJsonValue toJson(float value)        { return floatNumber(value); }
inline QJsonValue toJson(double value)       { return QJsonValue(value); }
inline QJsonValue toJson(short value)        { return QJsonValue(static_cast<double>(value)); }
inline QJsonValue toJson(unsigned short value) { return QJsonValue(static_cast<double>(value)); }
inline QJsonValue toJson(int value)          { return QJsonValue(static_cast<double>(value)); }
inline QJsonValue toJson(unsigned int value) { return QJsonValue(static_cast<double>(value)); }
inline QJsonValue toJson(long value)         { return QJsonValue(static_cast<double>(value)); }
inline QJsonValue toJson(long long value)    { return QJsonValue(static_cast<double>(value)); }
inline QJsonValue toJson(unsigned char value) { return QJsonValue(static_cast<double>(value)); }

inline bool fromJson(const QJsonValue& v, QByteArray& out)
{
  if(v.isNull()) { out.clear(); return true; }
  if(v.isString() == false) return false;
  out = v.toString().toLatin1();
  return true;
}

inline bool fromJson(const QJsonValue& v, QString& out)
{
  if(v.isNull()) { out.clear(); return true; }
  if(v.isString() == false) return false;
  out = v.toString();
  return true;
}

inline bool fromJson(const QJsonValue& v, bool& out)
{
  if(v.isNull()) { out = false; return true; }
  if(v.isBool() == false) return false;
  out = v.toBool();
  return true;
}

inline bool fromJson(const QJsonValue& v, char& out)
{
  if(v.isNull()) { out = 0; return true; }
  if(v.isString() == false || v.toString().toLatin1().size() > 1) return false;
  const QByteArray s = v.toString().toLatin1();
  out = s.isEmpty() ? 0 : s[0];
  return true;
}

inline bool fromJson(const QJsonValue& v, float& out)
{
  if(v.isNull()) { out = 0.0f; return true; }
  if(v.isDouble() == false) return false;
  out = static_cast<float>(v.toDouble());
  return true;
}

inline bool fromJson(const QJsonValue& v, double& out)
{
  if(v.isNull()) { out = 0.0; return true; }
  if(v.isDouble() == false) return false;
  out = v.toDouble();
  return true;
}

//! integers: whole numbers in the range of the type
template<typename T>
inline bool fromJsonInteger(const QJsonValue& v, T& out)
{
  if(v.isNull()) { out = 0; return true; }
  if(v.isDouble() == false) return false;
  const double d = v.toDouble();
  if(d != std::floor(d) ||
     d < static_cast<double>(std::numeric_limits<T>::min()) ||
     d > static_cast<double>(std::numeric_limits<T>::max()))
    return false;
  out = static_cast<T>(d);
  return true;
}

inline bool fromJson(const QJsonValue& v, short& out)          { return fromJsonInteger(v, out); }
inline bool fromJson(const QJsonValue& v, unsigned short& out) { return fromJsonInteger(v, out); }
inline bool fromJson(const QJsonValue& v, int& out)            { return fromJsonInteger(v, out); }
inline bool fromJson(const QJsonValue& v, unsigned int& out)   { return fromJsonInteger(v, out); }
inline bool fromJson(const QJsonValue& v, long& out)           { return fromJsonInteger(v, out); }
inline bool fromJson(const QJsonValue& v, long long& out)      { return fromJsonInteger(v, out); }
inline bool fromJson(const QJsonValue& v, unsigned char& out)  { return fromJsonInteger(v, out); }

} // namespace MedIOMetadata

//---------------------------------------------------------------------------------------------
// macros for the field tables of the adapters (H: header class)

//! native field with a getter and setter of a simple type T
#define MEDIO_NATIVE(H, name, getter, setter, T) \
  { name, \
    [](const CMedIOHeader& header) -> QJsonValue { \
      return MedIOMetadata::toJson(static_cast<const H&>(header).getter()); }, \
    [](CMedIOHeader& header, const QJsonValue& value) -> bool { \
      T v; if(MedIOMetadata::fromJson(value, v) == false) return false; \
      static_cast<H&>(header).setter(v); return true; } }

//! native text field (const char* getter and setter)
#define MEDIO_NATIVE_TEXT(H, name, getter, setter) \
  { name, \
    [](const CMedIOHeader& header) -> QJsonValue { \
      return MedIOMetadata::toJson(static_cast<const H&>(header).getter()); }, \
    [](CMedIOHeader& header, const QJsonValue& value) -> bool { \
      QByteArray v; if(MedIOMetadata::fromJson(value, v) == false) return false; \
      static_cast<H&>(header).setter(v.constData()); return true; } }

//! native enumeration field (stored as number)
#define MEDIO_NATIVE_ENUM(H, name, getter, setter, E) \
  { name, \
    [](const CMedIOHeader& header) -> QJsonValue { \
      return MedIOMetadata::toJson(static_cast<int>(static_cast<const H&>(header).getter())); }, \
    [](CMedIOHeader& header, const QJsonValue& value) -> bool { \
      int v; if(MedIOMetadata::fromJson(value, v) == false) return false; \
      static_cast<H&>(header).setter(static_cast<E>(v)); return true; } }

//! metadata key (getter: expression of h, setter: statements with h and value)
#define MEDIO_KEY(H, keyword, getExpression, setStatements) \
  { keyword, \
    [](const CMedIOHeader& header) -> QJsonValue { \
      const H& h = static_cast<const H&>(header); Q_UNUSED(h); return getExpression; }, \
    [](CMedIOHeader& header, const QJsonValue& value) -> bool { \
      H& h = static_cast<H&>(header); Q_UNUSED(h); Q_UNUSED(value); setStatements } }

#endif // CMEDIOMETADATAMAP_H
