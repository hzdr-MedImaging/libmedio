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
//! kept in the main header object. Only the descriptive NIfTI fields form the
//! native section "NIfTI".

#include "CMedIOHeaderMapping.h"
#include "CMedIOHeaderDictionary.h"
#include "CNIFTIFile.h"
#include "CNIFTIMainHeader.h"
#include "CNIFTI1MainHeader.h"
#include "CNIFTI2MainHeader.h"
#include "CNIFTIGeometry.h"
#include "CECAT7MainHeader.h"

#include <QJsonArray>

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
                                 "DecayCorrectionFactor" };

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
