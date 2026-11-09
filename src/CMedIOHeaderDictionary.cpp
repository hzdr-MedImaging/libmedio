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

//! @file CMedIOHeaderDictionary.cpp
//! @brief contains the implementation of the class CMedIOHeaderDictionary

#include "CMedIOHeaderDictionary.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegExp>

const char* const CMedIOHeaderDictionary::FramesKey = "Frames";

namespace {

// sequences holding some of the attributes below
const quint32 RadiopharmaceuticalInformationSequence = 0x00540016; // (0054,0016)
const quint32 EnergyWindowRangeSequence = 0x00540013;              // (0054,0013)

#define M CMedIOHeaderDictionary::MainHeader
#define F CMedIOHeaderDictionary::FrameHeader
#define STR CMedIOHeaderDictionary::String
#define NUM CMedIOHeaderDictionary::Number
#define DAT CMedIOHeaderDictionary::Date
#define DTM CMedIOHeaderDictionary::DateTime
#define STRS CMedIOHeaderDictionary::Strings
#define CODE CMedIOHeaderDictionary::Code

// the keys of the attributes: DICOM keyword, tag, VR, VM, sequence, scope,
// JSON value, unit (checked against the DICOM data dictionary of pydicom by
// the attributes test, see testsuite/attributes)
const CMedIOHeaderDictionary::Entry entryTable[] = {
  // patient
  { "PatientName",                      0x00100010, "PN", "1",   0, M, STR,  ""   }, // (0010,0010)
  { "PatientID",                        0x00100020, "LO", "1",   0, M, STR,  ""   }, // (0010,0020)
  { "PatientBirthDate",                 0x00100030, "DA", "1",   0, M, DAT,  ""   }, // (0010,0030)
  { "PatientSex",                       0x00100040, "CS", "1",   0, M, STR,  ""   }, // (0010,0040) M, F, O
  { "PatientAge",                       0x00101010, "AS", "1",   0, M, STR,  ""   }, // (0010,1010) e.g. "045Y"
  { "PatientSize",                      0x00101020, "DS", "1",   0, M, NUM,  "m"  }, // (0010,1020)
  { "PatientWeight",                    0x00101030, "DS", "1",   0, M, NUM,  "kg" }, // (0010,1030)
  { "PatientPosition",                  0x00185100, "CS", "1",   0, M, STR,  ""   }, // (0018,5100) e.g. HFS

  // study and series
  { "StudyInstanceUID",                 0x0020000D, "UI", "1",   0, M, STR,  ""   }, // (0020,000D)
  { "SeriesInstanceUID",                0x0020000E, "UI", "1",   0, M, STR,  ""   }, // (0020,000E)
  { "FrameOfReferenceUID",              0x00200052, "UI", "1",   0, M, STR,  ""   }, // (0020,0052)
  { "StudyID",                          0x00200010, "SH", "1",   0, M, STR,  ""   }, // (0020,0010)
  { "SeriesNumber",                     0x00200011, "IS", "1",   0, M, NUM,  ""   }, // (0020,0011)
  { "AccessionNumber",                  0x00080050, "SH", "1",   0, M, STR,  ""   }, // (0008,0050)
  { "AcquisitionDateTime",              0x0008002A, "DT", "1",   0, M, DTM,  ""   }, // (0008,002A)
  { "StudyDescription",                 0x00081030, "LO", "1",   0, M, STR,  ""   }, // (0008,1030)
  { "SeriesDescription",                0x0008103E, "LO", "1",   0, M, STR,  ""   }, // (0008,103E)
  { "InstitutionName",                  0x00080080, "LO", "1",   0, M, STR,  ""   }, // (0008,0080)
  { "ReferringPhysicianName",           0x00080090, "PN", "1",   0, M, STR,  ""   }, // (0008,0090)
  { "RequestingPhysician",              0x00321032, "PN", "1",   0, M, STR,  ""   }, // (0032,1032)
  { "OperatorsName",                    0x00081070, "PN", "1-n", 0, M, STRS, ""   }, // (0008,1070)

  // equipment
  { "Manufacturer",                     0x00080070, "LO", "1",   0, M, STR,  ""   }, // (0008,0070)
  { "ManufacturerModelName",            0x00081090, "LO", "1",   0, M, STR,  ""   }, // (0008,1090)
  { "DeviceSerialNumber",               0x00181000, "LO", "1",   0, M, STR,  ""   }, // (0018,1000)
  { "SoftwareVersions",                 0x00181020, "LO", "1-n", 0, M, STRS, ""   }, // (0018,1020)

  // radiopharmaceutical, in the RadiopharmaceuticalInformationSequence (0054,0016)
  { "Radiopharmaceutical",              0x00180031, "LO", "1",   RadiopharmaceuticalInformationSequence, M, STR,  ""   }, // (0018,0031)
  { "RadiopharmaceuticalRoute",         0x00181070, "LO", "1",   RadiopharmaceuticalInformationSequence, M, STR,  ""   }, // (0018,1070)
  { "RadiopharmaceuticalStartDateTime", 0x00181078, "DT", "1",   RadiopharmaceuticalInformationSequence, M, DTM,  ""   }, // (0018,1078)
  { "RadionuclideTotalDose",            0x00181074, "DS", "1",   RadiopharmaceuticalInformationSequence, M, NUM,  "Bq" }, // (0018,1074)
  { "RadionuclideHalfLife",             0x00181075, "DS", "1",   RadiopharmaceuticalInformationSequence, M, NUM,  "s"  }, // (0018,1075)
  { "RadionuclidePositronFraction",     0x00181076, "DS", "1",   RadiopharmaceuticalInformationSequence, M, NUM,  ""   }, // (0018,1076)
  { "RadionuclideCodeSequence",         0x00540300, "SQ", "1",   RadiopharmaceuticalInformationSequence, M, CODE, ""   }, // (0054,0300)
  { "RadiopharmaceuticalCodeSequence",  0x00540304, "SQ", "1",   RadiopharmaceuticalInformationSequence, M, CODE, ""   }, // (0054,0304)

  // acquisition, in the EnergyWindowRangeSequence (0054,0013)
  { "EnergyWindowLowerLimit",           0x00540014, "DS", "1",   EnergyWindowRangeSequence, M, NUM, "keV" }, // (0054,0014)
  { "EnergyWindowUpperLimit",           0x00540015, "DS", "1",   EnergyWindowRangeSequence, M, NUM, "keV" }, // (0054,0015)

  // image
  { "Units",                            0x00541001, "CS", "1",   0, M, STR,  ""   }, // (0054,1001) e.g. BQML
  { "DecayCorrection",                  0x00541102, "CS", "1",   0, M, STR,  ""   }, // (0054,1102) NONE, START, ADMIN
  { "CorrectedImage",                   0x00280051, "CS", "1-n", 0, M, STRS, ""   }, // (0028,0051) e.g. ATTN, DECY
  { "ReconstructionMethod",             0x00541103, "LO", "1",   0, M, STR,  ""   }, // (0054,1103)
  { "AttenuationCorrectionMethod",      0x00541101, "LO", "1",   0, M, STR,  ""   }, // (0054,1101)
  { "ScatterCorrectionMethod",          0x00541105, "LO", "1",   0, M, STR,  ""   }, // (0054,1105)

  // frame
  { "ActualFrameDuration",              0x00181242, "IS", "1",   0, F, NUM,  "ms" }, // (0018,1242)
  { "DecayFactor",                      0x00541321, "DS", "1",   0, F, NUM,  ""   }, // (0054,1321)
  { "DeadTimeFactor",                   0x00541324, "DS", "1",   0, F, NUM,  ""   }, // (0054,1324)
};

#undef M
#undef F
#undef STR
#undef NUM
#undef DAT
#undef DTM
#undef STRS
#undef CODE

const int entryCount = sizeof(entryTable) / sizeof(entryTable[0]);

// strings of the code sequence items
const char* const codeFields[] = { "CodeValue", "CodingSchemeDesignator", "CodingSchemeVersion", "CodeMeaning" };

bool isStringArray(const QJsonValue& value)
{
  if(value.isArray() == false)
    return false;
  const QJsonArray array = value.toArray();
  for(int i=0; i < array.size(); i++) {
    if(array[i].isString() == false)
      return false;
  }
  return true;
}

} // namespace

//=============================================================================================

const CMedIOHeaderDictionary::Entry* CMedIOHeaderDictionary::entry(const QString& keywordOrTag)
{
  for(int i=0; i < entryCount; i++) {
    if(keywordOrTag == QLatin1String(entryTable[i].keyword))
      return &entryTable[i];
  }

  quint32 t = 0;
  return parseTag(keywordOrTag, t) ? entry(t) : NULL;
}

const CMedIOHeaderDictionary::Entry* CMedIOHeaderDictionary::entry(quint32 tag)
{
  for(int i=0; i < entryCount; i++) {
    if(entryTable[i].tag == tag)
      return &entryTable[i];
  }
  return NULL;
}

QList<const CMedIOHeaderDictionary::Entry*> CMedIOHeaderDictionary::entries()
{
  QList<const Entry*> list;
  for(int i=0; i < entryCount; i++)
    list.append(&entryTable[i]);
  return list;
}

QString CMedIOHeaderDictionary::normalizedKey(const QString& keywordOrTag)
{
  const Entry* k = entry(keywordOrTag);
  if(k != NULL)
    return QString::fromLatin1(k->keyword);

  quint32 t = 0;
  if(parseTag(keywordOrTag, t))
    return tagString(t);

  return keywordOrTag;
}

QString CMedIOHeaderDictionary::tagString(quint32 tag)
{
  return QString("(%1,%2)").arg(tag >> 16, 4, 16, QChar('0'))
                           .arg(tag & 0xFFFF, 4, 16, QChar('0')).toUpper();
}

bool CMedIOHeaderDictionary::parseTag(const QString& text, quint32& tag)
{
  // "(gggg,eeee)", "gggg,eeee" or "ggggeeee" with hexadecimal digits
  QRegExp pattern("^\\(?([0-9A-Fa-f]{4}),?([0-9A-Fa-f]{4})\\)?$");
  const QString s = text.trimmed();
  if(pattern.exactMatch(s) == false)
    return false;

  // brackets only as pair
  if(s.startsWith('(') != s.endsWith(')'))
    return false;

  bool ok1 = false;
  bool ok2 = false;
  const quint32 group = pattern.cap(1).toUInt(&ok1, 16);
  const quint32 element = pattern.cap(2).toUInt(&ok2, 16);
  if(ok1 == false || ok2 == false)
    return false;

  tag = (group << 16) | element;
  return true;
}

quint32 CMedIOHeaderDictionary::tag(const QString& keywordOrTag)
{
  const Entry* k = entry(keywordOrTag);
  if(k != NULL)
    return k->tag;

  quint32 t = 0;
  return parseTag(keywordOrTag, t) ? t : 0;
}

bool CMedIOHeaderDictionary::isValid(const Entry& entry, const QJsonValue& value)
{
  switch(entry.type) {
    case String:
      return value.isString();

    case Number:
      return value.isDouble();

    case Date:
      return value.isString() &&
             QRegExp("^\\d{4}-\\d{2}-\\d{2}$").exactMatch(value.toString()) &&
             QDate::fromString(value.toString(), Qt::ISODate).isValid();

    case Time:
      return value.isString() &&
             QRegExp("^\\d{2}:\\d{2}(:\\d{2}(\\.\\d{1,6})?)?$").exactMatch(value.toString());

    case DateTime:
      return value.isString() &&
             QRegExp("^\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}(:\\d{2}(\\.\\d{1,6})?)?(Z|[+-]\\d{2}:\\d{2})?$").exactMatch(value.toString()) &&
             QDateTime::fromString(value.toString(), Qt::ISODate).isValid();

    case Strings:
      return isStringArray(value);

    case Code: {
      if(value.isObject() == false)
        return false;
      const QJsonObject object = value.toObject();
      for(QJsonObject::const_iterator it = object.begin(); it != object.end(); ++it) {
        bool known = false;
        for(unsigned i=0; i < sizeof(codeFields) / sizeof(codeFields[0]); i++)
          known = known || (it.key() == QLatin1String(codeFields[i]));
        if(known == false || it.value().isString() == false)
          return false;
      }
      return true;
    }
  }

  return false;
}
