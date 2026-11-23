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

//! @file CInterfileHeaderMapping.cpp
//! @brief private: the format independent header attributes of Interfile headers
//!        (see CInterfileHeaderMapping.h)

#include "CInterfileHeaderMapping.h"
#include "CInterfileKeys.h"

#include <QDate>
#include <QDateTime>
#include <QJsonArray>
#include <QRegularExpression>
#include <QStringList>
#include <QTime>

#include <cmath>

namespace InterfileMapping {

namespace {

// the name of the section of the native keys
const char* const SectionName = "Interfile";

// a source of keys: the keys [begin, end) of a header
struct Source
{
  const CInterfileKeys* keys;
  int begin;
  int end;
};

// the value of the first of the keys (normalized names, without indices) in the
// sources, in the order of the sources; null if none is given with a value
QString lookup(const QList<Source>& sources, const QStringList& names, QString* foundName = NULL)
{
  foreach(const Source& source, sources)
  {
    foreach(const QString& name, names)
    {
      for(int i = source.begin; i < source.end; i++)
      {
        const CInterfileKey& key = source.keys->at(i);
        if(key.comment == false && key.name == name && key.value.isEmpty() == false)
        {
          if(foundName)
            *foundName = key.name;
          return key.value;
        }
      }
    }
  }

  return QString();
}

// the value of the first key whose name starts with one of the prefixes (e.g. "study
// date" for "study date (yyyy:mm:dd)"), with the name of that key
QString lookupPrefix(const QList<Source>& sources, const QStringList& prefixes, QString* foundName)
{
  foreach(const Source& source, sources)
  {
    foreach(const QString& prefix, prefixes)
    {
      for(int i = source.begin; i < source.end; i++)
      {
        const CInterfileKey& key = source.keys->at(i);
        if(key.comment == false && key.value.isEmpty() == false &&
           (key.name == prefix || key.name.startsWith(prefix + " (")))
        {
          *foundName = key.name;
          return key.value;
        }
      }
    }
  }

  return QString();
}

bool toNumber(const QString& value, double* number)
{
  return CInterfileKeys::toDouble(value, number) && std::isfinite(*number);
}

void putString(QJsonObject& json, const char* key, const QString& value)
{
  const QString text = value.trimmed();
  if(text.isEmpty() == false)
    json[QString::fromLatin1(key)] = text;
}

void putNumber(QJsonObject& json, const char* key, const QString& value, double factor = 1.0)
{
  double number = 0.0;
  if(value.isEmpty() == false && toNumber(value, &number))
    json[QString::fromLatin1(key)] = number * factor;
}

// the time zone given in the name of a key, e.g. "study time (hh:mm:ss gmt+00:00)":
// true and the offset from UTC (s) if there is one
bool zoneOfName(const QString& name, int* offset)
{
  static const QRegularExpression expression("gmt\\s*([+-])(\\d{1,2}):?(\\d{2})");
  const QRegularExpressionMatch match = expression.match(name);
  if(match.hasMatch() == false)
    return false;

  *offset = (match.captured(2).toInt() * 3600 + match.captured(3).toInt() * 60) * (match.captured(1) == "-" ? -1 : 1);
  return true;
}

QDate dateOf(const QString& value)
{
  // Interfile writes dates as yyyy:mm:dd
  const QStringList parts = value.trimmed().split(QRegularExpression("[:\\-/.]"));
  if(parts.size() != 3)
    return QDate();

  return QDate(parts[0].toInt(), parts[1].toInt(), parts[2].toInt());
}

QTime timeOf(const QString& value)
{
  const QStringList parts = value.trimmed().split(':');
  if(parts.size() < 2 || parts.size() > 3)
    return QTime();

  double seconds = 0.0;
  if(parts.size() == 3 && toNumber(parts[2], &seconds) == false)
    return QTime();

  return QTime(parts[0].toInt(), parts[1].toInt(), int(seconds), int(std::floor((seconds - std::floor(seconds)) * 1000.0 + 0.5)) % 1000);
}

// date and time as DICOM date time (ISO 8601): with the zone of the name of the time
// key (Siemens: GMT+00:00), otherwise without zone
QString dateTime(const QList<Source>& sources, const QString& dateKey, const QString& timeKey)
{
  QString dateName;
  QString timeName;
  const QDate date = dateOf(lookupPrefix(sources, QStringList() << dateKey, &dateName));
  const QTime time = timeOf(lookupPrefix(sources, QStringList() << timeKey, &timeName));
  if(date.isValid() == false || time.isValid() == false)
    return QString();

  QString text = date.toString("yyyy-MM-dd") + "T" + time.toString(time.msec() ? "hh:mm:ss.zzz" : "hh:mm:ss");
  int offset = 0;
  if(zoneOfName(timeName, &offset))
  {
    if(offset == 0)
      text += "Z";
    else
      text += QString("%1%2:%3").arg(offset < 0 ? '-' : '+').arg(std::abs(offset) / 3600, 2, 10, QChar('0'))
                                .arg(std::abs(offset) % 3600 / 60, 2, 10, QChar('0'));
  }

  return text;
}

// PMOD writes times as milliseconds since 1970 (UTC)
QString unixTime(const QString& value)
{
  double ms = 0.0;
  if(value.isEmpty() || toNumber(value, &ms) == false || ms <= 0.0)
    return QString();

  return QDateTime::fromMSecsSinceEpoch(qint64(ms), Qt::UTC).toString(Qt::ISODate);
}

// DICOM Patient Position of the Siemens (DICOM terms) or STIR (head_in/feet_in,
// supine/prone) keys
QString patientPosition(const QList<Source>& sources)
{
  const QString orientation = lookup(sources, QStringList() << "patient orientation").trimmed();
  static const QStringList dicom = QStringList() << "HFS" << "FFS" << "HFP" << "FFP" << "HFDR" << "HFDL" << "FFDR"
                                                 << "FFDL";
  if(dicom.contains(orientation.toUpper()))
    return orientation.toUpper();

  const QString rotation = lookup(sources, QStringList() << "patient rotation").trimmed().toLower();
  QString position;
  if(orientation.toLower() == "head_in")
    position = "HF";
  else if(orientation.toLower() == "feet_in")
    position = "FF";
  if(position.isEmpty() == false && (rotation == "supine" || rotation == "prone"))
    return position + (rotation == "supine" ? "S" : "P");

  return QString();
}

} // namespace

QString units(const QString& units, double* factor)
{
  *factor = 1.0;
  QString text = units.trimmed().toLower();
  text.remove(' ');

  if(text == "bqml" || text == "bq/ml" || text == "bq/cc" || text == "bq/cm3" || text == "bq/cm^3")
    return "BQML";
  if(text == "kbq/ml" || text == "kbq/cc" || text == "kbq/cm3")
  {
    *factor = 1e3;
    return "BQML";
  }
  if(text == "mbq/ml" || text == "mbq/cc" || text == "mbq/cm3")
  {
    *factor = 1e6;
    return "BQML";
  }

  return units.trimmed();
}

bool isLayoutKey(const QString& name)
{
  static const QStringList names = QStringList()
    // structure of the header
    << "interfile" << "end of interfile" << "data set" << "total number of data sets"
    // layout of the data
    << "name of data file" << "data offset in bytes" << "data starting block" << "imagedata byte order"
    << "image data byte order" << "number format" << "number of bytes per pixel" << "number of dimensions"
    << "matrix size" << "matrix axis label" << "number of slices" << "slice thickness (pixels)"
    << "total number of images" << "number of images/energy window" << "number of time frames"
    << "number of frame groups" << "frame group number" << "number of images this frame group"
    // geometry
    << "scaling factor (mm/pixel)" << "scale factor (mm/pixel)" << "first pixel offset (mm)" << "origin (mm)"
    << "pmod volume position" << "image info" << "slice sensitivity"
    // scaling
    << "data rescale slope" << "data rescale offset" << "image scaling factor" << "image slope"
    << "image intercept" << "quantification units" << "units" << "minimum pixel count" << "maximum pixel count"
    // timing
    << "image duration (sec)" << "image relative start time (sec)" << "pause between images (sec)"
    << "pause between frame groups (sec)";

  return names.contains(name);
}

QJsonObject section(const CInterfileKeys& keys, int begin, int end)
{
  QJsonObject json;

  for(int i = qMax(begin, 0); i < end && i < keys.count(); i++)
  {
    const CInterfileKey& key = keys.at(i);
    if(key.comment || key.value.isEmpty() || isLayoutKey(key.name))
      continue;

    const QString text = key.text.trimmed();
    if(json.contains(text))
    {
      QJsonArray values = json.value(text).isArray() ? json.value(text).toArray()
                                                     : (QJsonArray() << json.value(text));
      values.append(key.value);
      json[text] = values;
    }
    else
      json[text] = key.value;
  }

  return json;
}

QJsonObject mainAttributes(const CInterfileKeys& keys, int begin, int end, const CInterfileKeys* dataSet,
                           const QString& units)
{
  QList<Source> sources;
  const Source main = { &keys, begin, end };
  sources << main;
  if(dataSet)
  {
    const Source set = { dataSet, 0, dataSet->count() };
    sources << set;
  }
  const bool siemens = lookup(sources, QStringList() << "sms-mi header name space").isEmpty() == false;

  QJsonObject json;

  // patient
  putString(json, "PatientName", lookup(sources, QStringList() << "patient name"));
  putString(json, "PatientID", lookup(sources, QStringList() << "patient id"));
  const QDate birthDate = dateOf(lookup(sources, QStringList() << "patient dob" << "patient birth date"));
  if(birthDate.isValid())
    json["PatientBirthDate"] = birthDate.toString("yyyy-MM-dd");
  const QString sex = lookup(sources, QStringList() << "patient sex").trimmed().toUpper();
  if(sex.isEmpty() == false && QString("MFO").contains(sex.at(0)))
    json["PatientSex"] = QString(sex.at(0));
  putNumber(json, "PatientWeight", lookup(sources, QStringList() << "patient weight (kg)" << "patient weight"
                                                                 << "pmod patient weight"));
  const QString height = lookup(sources, QStringList() << "patient height (cm)");
  if(height.isEmpty() == false)
    putNumber(json, "PatientSize", height, 0.01);
  else
    putNumber(json, "PatientSize", lookup(sources, QStringList() << "pmod patient size"));
  putString(json, "PatientPosition", patientPosition(sources));

  // study
  QString studyId = lookup(sources, QStringList() << "study id");
  while(studyId.endsWith('\\'))
    studyId.chop(1);
  putString(json, "StudyID", studyId);
  QString acquisition = unixTime(lookup(sources, QStringList() << "pmod scan datetime utc"));
  if(acquisition.isEmpty())
    acquisition = dateTime(sources, "study date", "study time");
  putString(json, "AcquisitionDateTime", acquisition);

  // equipment
  if(siemens)
    json["Manufacturer"] = QString("SIEMENS");
  putString(json, "ManufacturerModelName", lookup(sources, QStringList() << "originating system"));

  // radiopharmaceutical
  putString(json, "Radiopharmaceutical", lookup(sources, QStringList() << "radiopharmaceutical"));
  putNumber(json, "RadionuclideHalfLife", lookup(sources, QStringList() << "isotope gamma halflife (sec)"
                                                                        << "radionuclide halflife (sec)"
                                                                        << "isotope halflife (sec)"
                                                                        << "isotope halflife"));
  putNumber(json, "RadionuclidePositronFraction", lookup(sources, QStringList() << "isotope branching factor"
                                                                                << "branching factor"));
  const QString doseBq = lookup(sources, QStringList() << "tracer activity at time of injection (bq)");
  const QString doseMBq = lookup(sources, QStringList() << "tracer activity at time of injection (mbq)"
                                                        << "pmod tracer activity");
  if(doseBq.isEmpty() == false)
    putNumber(json, "RadionuclideTotalDose", doseBq);
  else
    putNumber(json, "RadionuclideTotalDose", doseMBq, 1e6);
  QString injection = unixTime(lookup(sources, QStringList() << "pmod administration datetime utc"));
  if(injection.isEmpty())
    injection = dateTime(sources, "tracer injection date", "tracer injection time");
  putString(json, "RadiopharmaceuticalStartDateTime", injection);

  // energy window
  putNumber(json, "EnergyWindowLowerLimit", lookup(sources, QStringList() << "energy window lower level (kev)"
                                                                          << "energy window lower level"));
  putNumber(json, "EnergyWindowUpperLimit", lookup(sources, QStringList() << "energy window upper level (kev)"
                                                                          << "energy window upper level"));

  // image
  if(units == "BQML")
    json["Units"] = units;
  QString method = lookup(sources, QStringList() << "method of reconstruction");
  const QString iterations = lookup(sources, QStringList() << "number of iterations");
  const QString subsets = lookup(sources, QStringList() << "number of subsets");
  if(siemens && method.isEmpty() == false && iterations.isEmpty() == false && subsets.isEmpty() == false)
    method += QString(" %1i %2s").arg(iterations.trimmed(), subsets.trimmed()); // as in the DICOM series
  putString(json, "ReconstructionMethod", method);
  putString(json, "AttenuationCorrectionMethod", lookup(sources, QStringList() << "method of attenuation correction"));
  putString(json, "ScatterCorrectionMethod", lookup(sources, QStringList() << "method of scatter correction"));

  // the keys of the header
  json[QString::fromLatin1(SectionName)] = section(keys, begin, end);

  return json;
}

QJsonObject frameAttributes(const CInterfileKeys& keys, int begin, int end, double duration)
{
  QJsonObject json;

  if(duration > 0.0)
    json["ActualFrameDuration"] = std::floor(duration * 1000.0 + 0.5);

  const QJsonObject keysOfFrame = section(keys, begin, end);
  if(keysOfFrame.isEmpty() == false)
    json[QString::fromLatin1(SectionName)] = keysOfFrame;

  return json;
}

} // namespace InterfileMapping
