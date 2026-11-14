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

//! @file CNIFTIHeaderMapping.cpp
//! @brief private: format independent header attributes of NIfTI files (see CMedIOHeaderMapping.h)
//!
//! NIfTI headers have no fields for the attributes: all attributes of a NIfTI
//! file are stored in its JSON header extension (or a JSON sidecar file) and
//! kept in the main header object; a DICOM header extension holds them for
//! other tools (see CNIFTIExtensions.h). Only the descriptive NIfTI fields
//! form the native section "NIfTI".

#include "CMedIOHeaderMapping.h"
#include "CMedIOHeaderDictionary.h"
#include "CNIFTIFile.h"
#include "CNIFTIMainHeader.h"
#include "CNIFTI1MainHeader.h"
#include "CNIFTI2MainHeader.h"
#include "CNIFTIGeometry.h"
#include "CECAT7MainHeader.h"

#include <QDateTime>
#include <QJsonArray>
#include <QRegExp>
#include <QTime>

#include <cmath>

#include <rtdebug.h>

namespace MedIOHeaderMapping {

namespace {

typedef CNIFTI1MainHeader H1;
typedef CNIFTI2MainHeader H2;

// native fields (without the fields describing the image data: dimensions,
// data type, scaling, display range, geometry, units, slice timing)
const Field nifti1Native[] = {
  MEDIO_NATIVE_TEXT(H1, "descrip", descrip, setDescrip),
  MEDIO_NATIVE_TEXT(H1, "aux_file", aux_File, setAux_File),
  MEDIO_NATIVE_TEXT(H1, "intent_name", intent_Name, setIntent_Name),
  MEDIO_NATIVE(H1, "intent_code", intent_Code, setIntent_Code, short),
  MEDIO_NATIVE(H1, "intent_p1", intent_P1, setIntent_P1, float),
  MEDIO_NATIVE(H1, "intent_p2", intent_P2, setIntent_P2, float),
  MEDIO_NATIVE(H1, "intent_p3", intent_P3, setIntent_P3, float),
};

const Field nifti2Native[] = {
  MEDIO_NATIVE_TEXT(H2, "descrip", descrip, setDescrip),
  MEDIO_NATIVE_TEXT(H2, "aux_file", aux_File, setAux_File),
  MEDIO_NATIVE_TEXT(H2, "intent_name", intent_Name, setIntent_Name),
  MEDIO_NATIVE(H2, "intent_code", intent_Code, setIntent_Code, int),
  MEDIO_NATIVE(H2, "intent_p1", intent_P1, setIntent_P1, double),
  MEDIO_NATIVE(H2, "intent_p2", intent_P2, setIntent_P2, double),
  MEDIO_NATIVE(H2, "intent_p3", intent_P3, setIntent_P3, double),
};

const Adapter nifti1Adapter = { "NIfTI", NULL, 0, nifti1Native, sizeof(nifti1Native) / sizeof(nifti1Native[0]) };
const Adapter nifti2Adapter = { "NIfTI", NULL, 0, nifti2Native, sizeof(nifti2Native) / sizeof(nifti2Native[0]) };

// BIDS keys derived from the attributes (written for other tools, not read back
// from files that have the attributes)
const char* const bidsKeys[] = { "TracerName", "InjectedRadioactivity", "InjectedRadioactivityUnits",
                                 "ManufacturersModelName", "ReconMethodName", "ImageDecayCorrected",
                                 "DecayCorrectionFactor", "TracerRadionuclide", "TimeZero", "ScanStart",
                                 "InjectionStart", "ImageDecayCorrectionTime" };

// chemical symbols of the radionuclides used in PET and SPECT, for names like "^18^Fluorine"
const struct { const char* name; const char* symbol; } elements[] = {
  { "Arsenic", "As" }, { "Bromine", "Br" }, { "Carbon", "C" }, { "Cobalt", "Co" }, { "Copper", "Cu" },
  { "Fluorine", "F" }, { "Gallium", "Ga" }, { "Indium", "In" }, { "Iodine", "I" }, { "Lanthanum", "La" },
  { "Lutetium", "Lu" },
  { "Manganese", "Mn" }, { "Nitrogen", "N" }, { "Oxygen", "O" }, { "Rubidium", "Rb" },
  { "Scandium", "Sc" }, { "Technetium", "Tc" }, { "Terbium", "Tb" }, { "Thallium", "Tl" },
  { "Titanium", "Ti" }, { "Yttrium", "Y" }, { "Zirconium", "Zr" },
};

// BIDS radionuclide ("F18", "Cu64", "Tc99m") of a name like "F-18", "18F",
// "Cu-64", "^18^Fluorine" or "Fluorine-18", empty if unknown
QString bidsRadionuclide(const QString& name)
{
  const QString s = QString(name).remove(QChar('^')).trimmed();
  QRegExp symbolFirst("^([A-Za-z]+)[\\s_-]*(\\d{1,3})(m?)$");
  QRegExp massFirst("^(\\d{1,3})(m?)[\\s_-]*([A-Za-z]+)$");

  QString element, mass, metastable;
  if(symbolFirst.exactMatch(s)) {
    element = symbolFirst.cap(1);
    mass = symbolFirst.cap(2);
    metastable = symbolFirst.cap(3);
  } else if(massFirst.exactMatch(s)) {
    mass = massFirst.cap(1);
    metastable = massFirst.cap(2);
    element = massFirst.cap(3);
  } else {
    return QString();
  }

  QString symbol;
  if(element.size() <= 2) {
    symbol = element.left(1).toUpper() + element.mid(1).toLower();
  } else {
    for(unsigned i=0; i < sizeof(elements) / sizeof(elements[0]); i++) {
      if(element.compare(QLatin1String(elements[i].name), Qt::CaseInsensitive) == 0)
        symbol = QString::fromLatin1(elements[i].symbol);
    }
  }
  if(symbol.isEmpty())
    return QString();

  return symbol + QString::number(mass.toInt()) + metastable;
}

// half-lives (s) of PET radionuclides, to recognize the radionuclide of
// metadata that has only the half-life (e.g. the BIDS sidecars of dcm2niix)
const struct { const char* nuclide; double halfLife; } halfLives[] = {
  { "O15", 122.24 },     { "N13", 597.9 },      { "C11", 1221.8 },    { "Cu62", 584.4 },
  { "Rb82", 75.45 },     { "Ga68", 4062.6 },    { "F18", 6586.2 },    { "Ti45", 11088.0 },
  { "Cu61", 12020.4 },   { "La133", 14083.2 },  { "Sc44", 14551.2 },  { "Cu64", 45720.0 },
  { "Y86", 53064.0 },    { "Br76", 58320.0 },   { "Zr89", 282276.0 }, { "I124", 360806.4 },
};

// BIDS radionuclide of a half-life (s): the only radionuclide of the table
// within 1 %, empty otherwise
QString radionuclideOfHalfLife(const QJsonValue& value)
{
  const double halfLife = value.toDouble();
  if(value.isDouble() == false || halfLife <= 0.0)
    return QString();

  QString nuclide;
  for(unsigned i=0; i < sizeof(halfLives) / sizeof(halfLives[0]); i++) {
    if(std::fabs(halfLife - halfLives[i].halfLife) <= 0.01 * halfLives[i].halfLife) {
      if(nuclide.isEmpty() == false)
        return QString(); // ambiguous
      nuclide = QString::fromLatin1(halfLives[i].nuclide);
    }
  }
  return nuclide;
}

// ECAT7 isotope name ("F-18", "Tc-99m") and DICOM code meaning
// ("^18^Fluorine") of a BIDS radionuclide ("F18")
void radionuclideNames(const QString& nuclide, QString& isotope, QString& meaning)
{
  isotope.clear();
  meaning.clear();
  QRegExp parts("^([A-Z][a-z]?)(\\d{1,3})(m?)$");
  if(parts.exactMatch(nuclide) == false)
    return;

  isotope = parts.cap(1) + "-" + parts.cap(2) + parts.cap(3);
  for(unsigned i=0; i < sizeof(elements) / sizeof(elements[0]); i++) {
    if(parts.cap(1) == QLatin1String(elements[i].symbol))
      meaning = "^" + parts.cap(2) + parts.cap(3) + "^" + QString::fromLatin1(elements[i].name);
  }
}

// BIDS radionuclide of the attributes: the isotope of the native sections or
// the radionuclide code
QString radionuclideOf(const QJsonObject& json)
{
  const QJsonValue candidates[] = {
    json.value("ECAT7").toObject().value("Isotope_Name"),
    json.value("Philips").toObject().value("isotope_name"),
    json.value("microPET").toObject().value("isotope"),
    json.value("RadionuclideCodeSequence").toObject().value("CodeMeaning"),
  };
  for(unsigned i=0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
    const QString nuclide = bidsRadionuclide(candidates[i].toString());
    if(nuclide.isEmpty() == false)
      return nuclide;
  }
  return QString();
}

// "hh:mm:ss" local time of day of an ISO 8601 date and time (BIDS TimeZero:
// the clock time of the scanner, see unixTimeOfDateTime()), empty if it has no time
QString timeOfDay(const QJsonValue& dateTime)
{
  qint64 seconds = 0;
  if(QRegExp(".*T\\d{2}:\\d{2}.*").exactMatch(dateTime.toString()) == false ||
     unixTimeOfDateTime(dateTime, seconds) == false || seconds == 0)
    return QString();
  return QDateTime::fromMSecsSinceEpoch(seconds * 1000, Qt::LocalTime).time().toString("hh:mm:ss");
}

// factor of the BIDS radioactivity units to Bq
double radioactivityFactor(const QString& units)
{
  if(units.isEmpty() || units == "MBq") return 1.0e6;
  if(units == "kBq") return 1.0e3;
  if(units == "GBq") return 1.0e9;
  if(units == "Bq")  return 1.0;
  if(units == "mCi") return 3.7e7;
  if(units == "uCi") return 3.7e4;
  return 0.0;
}

void setIfMissing(QJsonObject& json, const QString& key, const QJsonValue& value)
{
  if(json.contains(key) == false && value.isUndefined() == false && value.isNull() == false)
    json[key] = value;
}

} // namespace

const Adapter* nifti1MainHeaderAdapter()
{
  return &nifti1Adapter;
}

const Adapter* nifti2MainHeaderAdapter()
{
  return &nifti2Adapter;
}

//=============================================================================================

QJsonObject attributesOfNiftiJson(const QJsonObject& source)
{
  ENTER();
  QJsonObject json = source;

  // ECAT7 main header fields of earlier versions of CNIFTIConverter
  const Adapter* ecat = ecat7MainHeaderAdapter();
  if(json.contains("ECAT7") == false) {
    QJsonObject fields;
    for(int i=0; i < ecat->nativeCount; i++) {
      const QString name = QString::fromLatin1(ecat->native[i].key);
      if(json.contains(name)) {
        QJsonValue value = json.take(name);
        // characters were written as number
        if((name == "Patient_Sex" || name == "Patient_Dexterity") && value.isDouble())
          value = QJsonValue(QString(QChar(value.toInt())));
        fields[name] = value;
      }
    }

    if(fields.isEmpty() == false) {
      json["ECAT7"] = fields;

      // the attributes of these fields: an ECAT7 header without values
      // (no defaults), set to the given fields
      CECAT7MainHeader header;
      QJsonObject cleared;
      for(int i=0; i < ecat->nativeCount; i++)
        cleared[QString::fromLatin1(ecat->native[i].key)] = QJsonValue(QJsonValue::Null);
      cleared["Patient_Orientation"] = static_cast<int>(CECAT7MainHeader::Orient_Unknown);   // 0 is a valid orientation
      header.updateAttributes(QJsonObject{{"ECAT7", cleared}});
      transfer(QJsonObject{{"ECAT7", fields}}, header);

      const QJsonObject derived = header.attributes();
      for(int i=0; i < ecat->keyCount; i++)
        setIfMissing(json, QString::fromLatin1(ecat->keys[i].key), derived.value(QString::fromLatin1(ecat->keys[i].key)));
    }
  }

  // BIDS keys of other tools
  setIfMissing(json, "Radiopharmaceutical", json.value("TracerName"));
  setIfMissing(json, "ManufacturerModelName", json.value("ManufacturersModelName"));
  setIfMissing(json, "ReconstructionMethod", json.value("ReconMethodName"));
  if(json.value("InjectedRadioactivity").isDouble()) {
    const double factor = radioactivityFactor(json.value("InjectedRadioactivityUnits").toString());
    if(factor > 0.0)
      setIfMissing(json, "RadionuclideTotalDose", number(json.value("InjectedRadioactivity").toDouble() * factor));
  }
  if(json.value("Units").toString().compare("Bq/mL", Qt::CaseInsensitive) == 0)
    json["Units"] = QString("BQML");

  // the radionuclide of other tools (no native section of a format written by
  // libmedio): BIDS TracerRadionuclide or the half-life of a PET radionuclide
  // (as ECAT7 isotope name and radionuclide code)
  // (of the JSON as given: the ECAT7 fields of earlier versions, incl. the
  // isotope name, are moved into the "ECAT7" section above)
  const bool otherTool = source.contains("ECAT7") == false && source.contains("Philips") == false &&
                         source.contains("microPET") == false && source.contains("Isotope_Name") == false;
  if(otherTool && radionuclideOf(json).isEmpty()) {
    QString nuclide = bidsRadionuclide(json.value("TracerRadionuclide").toString());
    if(nuclide.isEmpty())
      nuclide = radionuclideOfHalfLife(json.value("RadionuclideHalfLife"));
    QString isotope, meaning;
    radionuclideNames(nuclide, isotope, meaning);
    if(isotope.isEmpty() == false) {
      QJsonObject ecat = json.value("ECAT7").toObject();
      ecat["Isotope_Name"] = isotope;
      json["ECAT7"] = ecat;
    }
    if(meaning.isEmpty() == false)
      setIfMissing(json, "RadionuclideCodeSequence", QJsonObject{{"CodeMeaning", meaning}});
  }

  // the injection of BIDS PET metadata of other tools: InjectionStart (s)
  // relative to TimeZero (local clock time, on the day of the scan start
  // AcquisitionDateTime - ScanStart)
  qint64 scan = 0;
  const QTime timeZero = QTime::fromString(json.value("TimeZero").toString().left(8), "hh:mm:ss");
  if(json.contains("RadiopharmaceuticalStartDateTime") == false && timeZero.isValid() &&
     json.value("InjectionStart").isDouble() && json.value("AcquisitionDateTime").isString() &&
     unixTimeOfDateTime(json.value("AcquisitionDateTime"), scan) && scan != 0) {
    const QDateTime reference = QDateTime::fromMSecsSinceEpoch(scan * 1000, Qt::LocalTime)
                                .addMSecs(qRound64(-json.value("ScanStart").toDouble() * 1000.0));
    QDateTime zero(reference.date(), timeZero, Qt::LocalTime);
    if(zero.secsTo(reference) > 12 * 3600)
      zero = QDateTime(reference.date().addDays(1), timeZero, Qt::LocalTime);
    else if(reference.secsTo(zero) > 12 * 3600)
      zero = QDateTime(reference.date().addDays(-1), timeZero, Qt::LocalTime);
    const qint64 injection = zero.toMSecsSinceEpoch() / 1000 + qRound64(json.value("InjectionStart").toDouble());
    json["RadiopharmaceuticalStartDateTime"] = dateTimeOfUnixTime(injection);
  }

  const QJsonArray decay = json.value("DecayCorrectionFactor").toArray();
  if(decay.isEmpty() == false && json.contains(CMedIOHeaderDictionary::FramesKey) == false) {
    QJsonArray frames;
    for(int i=0; i < decay.size(); i++)
      frames.append(QJsonObject{{"DecayFactor", decay[i]}});
    json[CMedIOHeaderDictionary::FramesKey] = frames;
  }

  for(unsigned i=0; i < sizeof(bidsKeys) / sizeof(bidsKeys[0]); i++)
    json.remove(QString::fromLatin1(bidsKeys[i]));

  LEAVE();
  return json;
}

QJsonObject niftiJson(const QJsonObject& attributes)
{
  ENTER();
  QJsonObject json = attributes;
  json.remove("NIfTI");
  for(unsigned i=0; i < sizeof(bidsKeys) / sizeof(bidsKeys[0]); i++)
    json.remove(QString::fromLatin1(bidsKeys[i]));

  if(json.isEmpty()) {
    LEAVE();
    return json;
  }

  // BIDS keys for other tools
  setIfMissing(json, "TracerName", json.value("Radiopharmaceutical"));
  setIfMissing(json, "ManufacturersModelName", json.value("ManufacturerModelName"));
  setIfMissing(json, "ReconMethodName", json.value("ReconstructionMethod"));
  if(json.value("RadionuclideTotalDose").isDouble()) {
    json["InjectedRadioactivity"] = number(json.value("RadionuclideTotalDose").toDouble() / 1.0e6);
    json["InjectedRadioactivityUnits"] = QString("MBq");
  }
  if(json.value("DecayCorrection").isString())
    json["ImageDecayCorrected"] = (json.value("DecayCorrection").toString() != "NONE");

  // BIDS PET: units, radionuclide and the times relative to TimeZero (the scan start)
  if(json.value("Units").toString() == "BQML")
    json["Units"] = QString("Bq/mL");
  const QString nuclide = radionuclideOf(json);
  if(nuclide.isEmpty() == false)
    json["TracerRadionuclide"] = nuclide;
  const QString timeZero = timeOfDay(json.value("AcquisitionDateTime"));
  if(timeZero.isEmpty() == false) {
    json["TimeZero"] = timeZero;
    json["ScanStart"] = 0;

    qint64 scan = 0, injection = 0;
    const bool injectionKnown = json.value("RadiopharmaceuticalStartDateTime").isString() &&
                                unixTimeOfDateTime(json.value("AcquisitionDateTime"), scan) &&
                                unixTimeOfDateTime(json.value("RadiopharmaceuticalStartDateTime"), injection);
    if(injectionKnown)
      json["InjectionStart"] = static_cast<double>(injection - scan);

    const QString decay = json.value("DecayCorrection").toString();
    if(decay == "START")
      json["ImageDecayCorrectionTime"] = 0;
    else if(decay == "ADMIN" && injectionKnown)
      json["ImageDecayCorrectionTime"] = static_cast<double>(injection - scan);
  }

  const QJsonArray frames = json.value(CMedIOHeaderDictionary::FramesKey).toArray();
  QJsonArray decay;
  for(int i=0; i < frames.size(); i++) {
    const QJsonValue factor = frames[i].toObject().value("DecayFactor");
    if(factor.isDouble() == false) {
      decay = QJsonArray();
      break;
    }
    decay.append(factor);
  }
  if(decay.isEmpty() == false)
    json["DecayCorrectionFactor"] = decay;

  LEAVE();
  return json;
}

void readNiftiAttributes(CNIFTIFile& file, CNIFTIMainHeader& header)
{
  ENTER();
  // the name of the header file (fileName() is a temporary file while compressed files are opened)
  const QJsonObject json = niftiMetadata(file, &header, file.headerFileName());
  if(json.isEmpty() == false)
    transfer(attributesOfNiftiJson(json), header);
  LEAVE();
}

} // namespace MedIOHeaderMapping
