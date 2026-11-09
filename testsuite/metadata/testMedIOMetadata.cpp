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


// Tests of the format independent metadata (CMedIOMetadata, CMedIOHeader::metadata()).
//
// The dictionary of the metadata keys is compared with the DICOM data
// dictionary of pydicom (data/dicom_dictionary.json, written by
// data/generate_dictionary.py).

#include <CMedIOMetadata.h>
#include <CMedIOHeader.h>
#include <CECAT7SubHeaderNorm.h>
#include <CECATFile.h>
#include <CECAT7MainHeader.h>
#include <CECAT7SubHeaderImage.h>
#include <CMedIOVolume.h>
#include <CPhilipsFile.h>
#include <CPhilipsMainHeader.h>
#include <CPhilipsSubHeader.h>
#include <CConcordeImage.h>
#include <CNIFTIFile.h>
#include <CNIFTIMainHeader.h>
#include <CNIFTI1MainHeader.h>
#include <CNIFTI2MainHeader.h>
#include <CConcordeMainHeader.h>
#include <CConcordeFrameHeader.h>

#include "MedIOTestData.h"

#include <QtTest/QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#ifndef NIFTI_TEST_DATA_DIR
#error "NIFTI_TEST_DATA_DIR has to be defined"
#endif

#ifndef METADATA_TEST_DATA_DIR
#error "METADATA_TEST_DATA_DIR has to be defined"
#endif

//---------------------------------------------------------------------------------------------
// helpers

// expected values of a test data file (<file>.json, see testsuite/data)
static QJsonObject expectedValues(const QString& file)
{
  QString error;
  return medioReadJson(medioTestDataDir() + "/" + file + ".json", &error);
}

// true if the metadata keys of actual are the expected ones (numbers with a
// relative tolerance); keys: all metadata keys, or the given ones
static bool sameMetadata(const QJsonObject& actual, const QJsonObject& expected,
                         QString* message, QStringList keys = QStringList())
{
  if(keys.isEmpty()) {
    foreach(const CMedIOMetadata::Key* k, CMedIOMetadata::keys())
      keys.append(QString::fromLatin1(k->keyword));
  }

  foreach(const QString& key, keys) {
    const QJsonValue a = actual.value(key);
    const QJsonValue e = expected.value(key);
    bool same = (a == e);
    if(a.isDouble() && e.isDouble())
      same = qAbs(a.toDouble() - e.toDouble()) <= 1e-9 * qMax(qAbs(e.toDouble()), 1.0);
    if(same == false) {
      if(message)
        *message = QString("%1: %2 instead of %3").arg(key)
                     .arg(QString(QJsonDocument(QJsonObject{{"v", a}}).toJson(QJsonDocument::Compact)))
                     .arg(QString(QJsonDocument(QJsonObject{{"v", e}}).toJson(QJsonDocument::Compact)));
      return false;
    }
  }
  return true;
}

#define COMPARE_METADATA(actual, expected, ...) \
  do { QString message; \
       QVERIFY2(sameMetadata(actual, expected, &message, ##__VA_ARGS__), qPrintable(message)); } while(0)

//=============================================================================================

class TestMedIOMetadata : public QObject
{
  Q_OBJECT

  private slots:
    void dictionary();
    void tags_data();
    void tags();
    void keyLookup();
    void values_data();
    void values();

    void headerMetadata();
    void headerMetadataErrors();
    void headerMetadataCopies();

    void ecatMetadata_data();
    void ecatMetadata();
    void ecatWrite();
    void ecatBirthDate();

    void philipsMetadata_data();
    void philipsMetadata();
    void philipsWrite();

    void concordeMetadata();

    void niftiEarlierMetadata();
    void niftiWrite_data();
    void niftiWrite();
    void niftiBids();

  private:
    QTemporaryDir m_tmp;
};

//---------------------------------------------------------------------------------------------
// every key of libmedio has the tag, VR and VM of the DICOM data dictionary

void TestMedIOMetadata::dictionary()
{
  QFile file(QString(METADATA_TEST_DATA_DIR) + "/dicom_dictionary.json");
  QVERIFY(file.open(QIODevice::ReadOnly));
  const QJsonObject json = QJsonDocument::fromJson(file.readAll()).object();
  const QJsonObject expected = json["keys"].toObject();
  const QJsonObject sequences = json["sequences"].toObject();
  QVERIFY(expected.isEmpty() == false);

  const QList<const CMedIOMetadata::Key*> keys = CMedIOMetadata::keys();
  QCOMPARE(keys.size(), expected.size());

  QSet<quint32> sequenceTags;
  foreach(const QJsonValue& s, sequences) {
    quint32 tag = 0;
    QVERIFY(CMedIOMetadata::parseTag(s.toObject()["tag"].toString(), tag));
    QCOMPARE(s.toObject()["vr"].toString(), QString("SQ"));
    sequenceTags.insert(tag);
  }

  foreach(const CMedIOMetadata::Key* key, keys) {
    const QString keyword = QString::fromLatin1(key->keyword);
    QVERIFY2(expected.contains(keyword), qPrintable(keyword));
    const QJsonObject e = expected[keyword].toObject();
    QCOMPARE(CMedIOMetadata::tagString(key->tag), e["tag"].toString());
    QCOMPARE(QString::fromLatin1(key->vr), e["vr"].toString());
    QCOMPARE(QString::fromLatin1(key->vm), e["vm"].toString());
    QVERIFY(key->sequence == 0 || sequenceTags.contains(key->sequence));

    // JSON value of the VR
    const QString vr = QString::fromLatin1(key->vr);
    if(vr == "DS" || vr == "IS")
      QCOMPARE(key->type, CMedIOMetadata::Number);
    else if(vr == "DA")
      QCOMPARE(key->type, CMedIOMetadata::Date);
    else if(vr == "DT")
      QCOMPARE(key->type, CMedIOMetadata::DateTime);
    else if(vr == "SQ")
      QCOMPARE(key->type, CMedIOMetadata::Code);
    else if(QString::fromLatin1(key->vm) == "1-n")
      QCOMPARE(key->type, CMedIOMetadata::Strings);
    else
      QCOMPARE(key->type, CMedIOMetadata::String);
  }
}

//---------------------------------------------------------------------------------------------
// tags as number and as string

void TestMedIOMetadata::tags_data()
{
  QTest::addColumn<QString>("text");
  QTest::addColumn<bool>("valid");
  QTest::addColumn<uint>("tag");

  QTest::newRow("tuple")            << "(0010,1030)" << true  << 0x00101030u;
  QTest::newRow("without brackets") << "0010,1030"   << true  << 0x00101030u;
  QTest::newRow("hex digits")       << "00101030"    << true  << 0x00101030u;
  QTest::newRow("lower case")       << "(0029,10ab)" << true  << 0x002910ABu;
  QTest::newRow("spaces")           << " (0008,103E) " << true << 0x0008103Eu;
  QTest::newRow("one bracket")      << "(0010,1030"  << false << 0u;
  QTest::newRow("too short")        << "(010,1030)"  << false << 0u;
  QTest::newRow("no hex")           << "(00G0,1030)" << false << 0u;
  QTest::newRow("keyword")          << "PatientWeight" << false << 0u;
  QTest::newRow("empty")            << ""            << false << 0u;
}

void TestMedIOMetadata::tags()
{
  QFETCH(QString, text);
  QFETCH(bool, valid);
  QFETCH(uint, tag);

  quint32 parsed = 0;
  QCOMPARE(CMedIOMetadata::parseTag(text, parsed), valid);
  if(valid) {
    QCOMPARE(parsed, static_cast<quint32>(tag));
    QCOMPARE(CMedIOMetadata::parseTag(CMedIOMetadata::tagString(parsed), parsed), true);
    QCOMPARE(parsed, static_cast<quint32>(tag));
  }
}

//---------------------------------------------------------------------------------------------

void TestMedIOMetadata::keyLookup()
{
  QCOMPARE(CMedIOMetadata::tagString(0x00101030), QString("(0010,1030)"));
  QCOMPARE(CMedIOMetadata::tagString(0x0008103E), QString("(0008,103E)"));

  // by keyword, tag and tag string
  const CMedIOMetadata::Key* weight = CMedIOMetadata::key("PatientWeight");
  QVERIFY(weight != NULL);
  QCOMPARE(weight->tag, 0x00101030u);
  QCOMPARE(QString(weight->unit), QString("kg"));
  QVERIFY(CMedIOMetadata::key(0x00101030) == weight);
  QVERIFY(CMedIOMetadata::key("(0010,1030)") == weight);
  QVERIFY(CMedIOMetadata::key("00101030") == weight);
  QCOMPARE(CMedIOMetadata::tag("PatientWeight"), 0x00101030u);
  QCOMPARE(CMedIOMetadata::tag("(0010,1030)"), 0x00101030u);

  // attribute of a sequence
  const CMedIOMetadata::Key* dose = CMedIOMetadata::key("(0018,1074)");
  QVERIFY(dose != NULL);
  QCOMPARE(QString(dose->keyword), QString("RadionuclideTotalDose"));
  QCOMPARE(CMedIOMetadata::tagString(dose->sequence), QString("(0054,0016)"));
  QCOMPARE(QString(dose->unit), QString("Bq"));

  // frame keys
  QCOMPARE(CMedIOMetadata::key("DecayFactor")->scope, CMedIOMetadata::FrameHeader);
  QCOMPARE(CMedIOMetadata::key("PatientName")->scope, CMedIOMetadata::MainHeader);

  // unknown keys and tags
  QVERIFY(CMedIOMetadata::key("PatientWeigth") == NULL);
  QVERIFY(CMedIOMetadata::key(0x00291010) == NULL);
  QCOMPARE(CMedIOMetadata::tag("(0029,1010)"), 0x00291010u);
  QCOMPARE(CMedIOMetadata::tag("Unknown"), 0u);

  // normalized keys: keyword of known tags, tag string of unknown tags
  QCOMPARE(CMedIOMetadata::normalizedKey("(0010,1030)"), QString("PatientWeight"));
  QCOMPARE(CMedIOMetadata::normalizedKey("00101030"), QString("PatientWeight"));
  QCOMPARE(CMedIOMetadata::normalizedKey("PatientWeight"), QString("PatientWeight"));
  QCOMPARE(CMedIOMetadata::normalizedKey("00291010"), QString("(0029,1010)"));
  QCOMPARE(CMedIOMetadata::normalizedKey("ECAT7"), QString("ECAT7"));
}

//---------------------------------------------------------------------------------------------
// JSON values of the keys

void TestMedIOMetadata::values_data()
{
  QTest::addColumn<QString>("key");
  QTest::addColumn<QString>("json");   // {"v": value}
  QTest::addColumn<bool>("valid");

  QTest::newRow("string")             << "PatientName" << "\"Doe^John\"" << true;
  QTest::newRow("string as number")   << "PatientName" << "1" << false;
  QTest::newRow("number")             << "PatientWeight" << "8.63" << true;
  QTest::newRow("number as string")   << "PatientWeight" << "\"8.63\"" << false;
  QTest::newRow("date")               << "PatientBirthDate" << "\"1926-06-03\"" << true;
  QTest::newRow("date DICOM")         << "PatientBirthDate" << "\"19260603\"" << false;
  QTest::newRow("date invalid")       << "PatientBirthDate" << "\"1926-13-03\"" << false;
  QTest::newRow("date time UTC")      << "AcquisitionDateTime" << "\"2026-06-03T17:04:56Z\"" << true;
  QTest::newRow("date time offset")   << "AcquisitionDateTime" << "\"2026-06-03T19:04:56+02:00\"" << true;
  QTest::newRow("date time fraction") << "AcquisitionDateTime" << "\"2026-06-03T19:04:56.837\"" << true;
  QTest::newRow("date time no zone")  << "AcquisitionDateTime" << "\"2026-06-03T19:04:56\"" << true;
  QTest::newRow("date time DICOM")    << "AcquisitionDateTime" << "\"20260603190456\"" << false;
  QTest::newRow("date only")          << "AcquisitionDateTime" << "\"2026-06-03\"" << false;
  QTest::newRow("strings")            << "CorrectedImage" << "[\"ATTN\", \"DECY\"]" << true;
  QTest::newRow("strings single")     << "CorrectedImage" << "\"ATTN\"" << false;
  QTest::newRow("strings mixed")      << "CorrectedImage" << "[\"ATTN\", 1]" << false;
  QTest::newRow("code")               << "RadionuclideCodeSequence"
                                      << "{\"CodeValue\": \"C-111A1\", \"CodingSchemeDesignator\": \"SRT\", \"CodeMeaning\": \"^18^Fluorine\"}" << true;
  QTest::newRow("code unknown field") << "RadionuclideCodeSequence" << "{\"Value\": \"C-111A1\"}" << false;
  QTest::newRow("code number")        << "RadionuclideCodeSequence" << "{\"CodeValue\": 1}" << false;
}

void TestMedIOMetadata::values()
{
  QFETCH(QString, key);
  QFETCH(QString, json);
  QFETCH(bool, valid);

  const QJsonObject object = QJsonDocument::fromJson(QString("{\"v\": %1}").arg(json).toUtf8()).object();
  QVERIFY(object.contains("v"));
  QCOMPARE(CMedIOMetadata::isValid(*CMedIOMetadata::key(key), object["v"]), valid);
}

//---------------------------------------------------------------------------------------------
// metadata kept in the header object (a header type without metadata of its
// own format: everything is kept in the object)

void TestMedIOMetadata::headerMetadata()
{
  CECAT7SubHeaderNorm header;
  QVERIFY(header.metadata().isEmpty());

  // by keyword, tag string and tag number
  QVERIFY(header.setMetadataValue("PatientWeight", 8.63));
  QVERIFY(header.setMetadataValue("(0010,0010)", QString("Doe^John")));
  QVERIFY(header.setMetadataValue(0x00181074, 60780000.0));   // (0018,1074) RadionuclideTotalDose

  QCOMPARE(header.metadataValue("PatientWeight").toDouble(), 8.63);
  QCOMPARE(header.metadataValue("(0010,1030)").toDouble(), 8.63);
  QCOMPARE(header.metadataValue(0x00101030).toDouble(), 8.63);
  QCOMPARE(header.metadataValue("PatientName").toString(), QString("Doe^John"));
  QCOMPARE(header.metadataValue("RadionuclideTotalDose").toDouble(), 60780000.0);
  QVERIFY(header.metadataValue("PatientID").isUndefined());

  // stored under the keyword
  QJsonObject json = header.metadata();
  QCOMPARE(json.size(), 3);
  QVERIFY(json.contains("PatientName") && json.contains("PatientWeight") && json.contains("RadionuclideTotalDose"));

  // unknown keys and tags of private attributes (stored as tag string)
  QVERIFY(header.setMetadataValue("00291010", QString("private")));
  QVERIFY(header.setMetadataValue("MyKey", QJsonObject{{"a", 1}}));
  QCOMPARE(header.metadataValue("(0029,1010)").toString(), QString("private"));
  QCOMPARE(header.metadataValue(0x00291010).toString(), QString("private"));
  QVERIFY(header.metadata().contains("(0029,1010)"));
  QCOMPARE(header.metadataValue("MyKey").toObject()["a"].toInt(), 1);

  // the whole object changed and given back; null removes a key
  json = header.metadata();
  json["PatientWeight"] = 9.5;
  json["PatientName"] = QJsonValue(QJsonValue::Null);
  QVERIFY(header.updateMetadata(json));
  QCOMPARE(header.metadataValue("PatientWeight").toDouble(), 9.5);
  QVERIFY(header.metadataValue("PatientName").isUndefined());
  QCOMPARE(header.metadataValue("RadionuclideTotalDose").toDouble(), 60780000.0);

  // frame metadata of all frames
  QJsonArray frames;
  frames.append(QJsonObject{{"DecayFactor", 1.00158}});
  QVERIFY(header.setMetadataValue(CMedIOMetadata::FramesKey, frames));
  QCOMPARE(header.metadataValue("Frames").toArray().size(), 1);
}

void TestMedIOMetadata::headerMetadataErrors()
{
  CECAT7SubHeaderNorm header;
  QVERIFY(header.setMetadataValue("PatientWeight", 8.63));
  const QJsonObject before = header.metadata();

  // invalid values: nothing is changed, also not the valid keys of the same call
  QJsonObject changes;
  changes["PatientName"] = QString("Doe^John");
  changes["PatientWeight"] = QString("heavy");
  QVERIFY(header.updateMetadata(changes) == false);
  QCOMPARE(header.metadata(), before);

  QVERIFY(header.setMetadataValue("PatientBirthDate", QString("19260603")) == false);
  QVERIFY(header.setMetadataValue("AcquisitionDateTime", QString("2026-06-03")) == false);
  QVERIFY(header.setMetadataValue("CorrectedImage", QString("ATTN")) == false);
  QVERIFY(header.setMetadataValue("Frames", QJsonArray{1, 2}) == false);
  QCOMPARE(header.metadata(), before);

  // a key given as keyword and as tag
  changes = QJsonObject();
  changes["PatientWeight"] = 1.0;
  changes["(0010,1030)"] = 2.0;
  QVERIFY(header.updateMetadata(changes) == false);
  QCOMPARE(header.metadata(), before);
}

void TestMedIOMetadata::headerMetadataCopies()
{
  CECAT7SubHeaderNorm header;
  QVERIFY(header.setMetadataValue("PatientWeight", 8.63));

  CECAT7SubHeaderNorm copy(header);
  QCOMPARE(copy.metadata(), header.metadata());

  QScopedPointer<CMedIOHeader> clone(header.clone());
  QCOMPARE(clone->metadata(), header.metadata());

  CECAT7SubHeaderNorm assigned;
  assigned = header;
  QCOMPARE(assigned.metadata(), header.metadata());

  // the copies are independent
  QVERIFY(copy.setMetadataValue("PatientWeight", 1.0));
  QCOMPARE(header.metadataValue("PatientWeight").toDouble(), 8.63);
}

//---------------------------------------------------------------------------------------------
// ECAT7: metadata of the test datasets, compared with the values of the
// nibabel header (<file>.json) and with the DICOM series the files were
// exported from (dicom/<name>.json, pydicom)

void TestMedIOMetadata::ecatMetadata_data()
{
  QTest::addColumn<QString>("file");
  QTest::addColumn<QString>("dicom");

  QTest::newRow("pet-static")  << "ecat/pet-static.v" << "dicom/pet-static";
  QTest::newRow("pet-dynamic") << "ecat/pet-dynamic.v" << "dicom/pet-dynamic";
}

void TestMedIOMetadata::ecatMetadata()
{
  QFETCH(QString, file);
  QFETCH(QString, dicom);

  const QJsonObject expected = expectedValues(file)["metadata"].toObject();
  const QJsonObject original = expectedValues(dicom)["metadata"].toObject();
  QVERIFY(expected.isEmpty() == false && original.isEmpty() == false);

  CECATFile ecat(medioTestDataDir() + "/" + file);
  QVERIFY(ecat.open(QIODevice::ReadOnly));
  CECATMainHeader* mh = NULL;
  QVERIFY(ecat.readMainHeader(mh));
  QScopedPointer<CECATMainHeader> mhGuard(mh);

  const QJsonObject main = mh->metadata();
  COMPARE_METADATA(main, expected["main"].toObject());

  // the values that ECAT7 stores like DICOM, as far as the file has them (the
  // times are UTC in ECAT7 and local time without time zone in the DICOM
  // series, PatientSex O is no ECAT7 value)
  QStringList same;
  foreach(const QString& key, QStringList() << "PatientName" << "PatientID" << "PatientBirthDate" << "PatientSize"
                                            << "PatientWeight" << "Radiopharmaceutical" << "RadionuclideTotalDose"
                                            << "RadionuclideHalfLife" << "EnergyWindowLowerLimit"
                                            << "EnergyWindowUpperLimit" << "Units") {
    if(main.contains(key))
      same.append(key);
  }
  QVERIFY(same.size() >= 9);
  COMPARE_METADATA(main, original["main"].toObject(), same);

  // native fields of the main header, without the fields describing the image data
  const QJsonObject native = main["ECAT7"].toObject();
  QCOMPARE(native["Patient_Weight"].toDouble(), main["PatientWeight"].toDouble());
  QCOMPARE(native["Patient_Height"].toDouble(), 100.0);
  QCOMPARE(native["Data_Units"].toString(), expectedValues(file)["main_header"].toObject()["data_units"].toString());
  QCOMPARE(native["Patient_Sex"].toString(), QString("U"));
  foreach(const QString& field, QStringList() << "Magic_Number" << "File_Type" << "Num_Planes" << "Num_Frames"
                                              << "Calibration_Factor" << "Plane_Separation" << "Init_Bed_Position")
    QVERIFY2(native.contains(field) == false, qPrintable(field));

  // frames
  const QJsonArray expectedFrames = expected["frames"].toArray();
  const QJsonArray originalFrames = original["frames"].toArray();
  const QList<short> frames = ecat.frameNumbers();
  QCOMPARE(frames.size(), expectedFrames.size());
  QCOMPARE(frames.size(), originalFrames.size());
  for(int i=0; i < frames.size(); i++) {
    CECATSubHeader* sh = NULL;
    QVERIFY(ecat.readSubHeader(sh, frames[i]));
    QScopedPointer<CECATSubHeader> shGuard(sh);
    const QJsonObject frame = sh->metadata();
    COMPARE_METADATA(frame, expectedFrames[i].toObject());
    COMPARE_METADATA(frame, originalFrames[i].toObject(), QStringList() << "ActualFrameDuration" << "DecayFactor");
    QVERIFY(frame["ECAT7"].toObject().contains("Annotation"));
    QVERIFY(frame["ECAT7"].toObject().contains("Scale_Factor") == false);
  }
}

//---------------------------------------------------------------------------------------------
// ECAT7: metadata written into a file

void TestMedIOMetadata::ecatWrite()
{
  const QString path = m_tmp.filePath("metadata.v");

  CECAT7MainHeader header(NULL, CECATMainHeader::ECAT7_Volume16);
  QJsonObject changes;
  changes["PatientName"] = QString("Doe^Jane");
  changes["(0010,0020)"] = QString("P-123");                  // PatientID as tag string
  changes["PatientBirthDate"] = QString("1980-02-29");
  changes["PatientSex"] = QString("O");                       // no ECAT7 value
  changes["PatientAge"] = QString("046Y");
  changes["PatientSize"] = 1.82;
  changes["PatientWeight"] = 71.5;
  changes["PatientPosition"] = QString("HFS");
  changes["AcquisitionDateTime"] = QString("2026-06-03T19:04:56+02:00");
  changes["RadiopharmaceuticalStartDateTime"] = QString("2026-06-03T15:40:00Z");
  changes["RadionuclideTotalDose"] = 250.5e6;
  changes["RadionuclideHalfLife"] = 6586.2;
  changes["EnergyWindowLowerLimit"] = 435.0;
  changes["Units"] = QString("BQML");
  changes["SeriesInstanceUID"] = QString("1.2.3.4");          // no ECAT7 field
  changes["Philips"] = QJsonObject{{"weight", 71500}};         // other format
  QVERIFY(header.updateMetadata(changes));

  // the native fields in the units of ECAT7
  QCOMPARE(QString(header.patient_ID()), QString("P-123"));
  QCOMPARE(header.patient_Height(), 182.0f);
  QCOMPARE(header.patient_Weight(), 71.5f);
  QCOMPARE(static_cast<qint64>(header.scan_Start_Time()), Q_INT64_C(1780506296));   // 17:04:56 UTC
  QCOMPARE(header.patient_Sex(), CECAT7MainHeader::Sex_Unknown);
  QCOMPARE(header.patient_Orientation(), CECAT7MainHeader::HF_Supine);
  QCOMPARE(QString(header.data_Units()), QString("Bq/ml"));
  QCOMPARE(header.dosage(), 250.5e6f);

  // values ECAT7 cannot store are kept in the object
  QJsonObject json = header.metadata();
  QCOMPARE(json["PatientSex"].toString(), QString("O"));
  QCOMPARE(json["SeriesInstanceUID"].toString(), QString("1.2.3.4"));
  QCOMPARE(json["Philips"].toObject()["weight"].toInt(), 71500);
  QCOMPARE(json["AcquisitionDateTime"].toString(), QString("2026-06-03T17:04:56Z"));
  QCOMPARE(json["PatientSize"].toDouble(), 1.82);

  // native fields: changed through the section of the format
  QVERIFY(header.updateMetadata(QJsonObject{{"ECAT7", QJsonObject{{"Patient_Weight", 70.0}, {"Isotope_Name", "F-18"}}}}));
  QCOMPARE(header.metadataValue("PatientWeight").toDouble(), 70.0);
  QCOMPARE(QString(header.isotope_Name()), QString("F-18"));
  QVERIFY(header.updateMetadata(QJsonObject{{"ECAT7", QJsonObject{{"Num_Planes", 2}}}}) == false);
  QVERIFY(header.updateMetadata(QJsonObject{{"ECAT7", QJsonObject{{"Lwr_True_Thres", 70000}}}}) == false);
  QVERIFY(header.updateMetadata(QJsonObject{{"PatientPosition", "XYZ"}}));   // kept in the object
  QCOMPARE(header.patient_Orientation(), CECAT7MainHeader::Orient_Unknown);
  QVERIFY(header.updateMetadata(QJsonObject{{"PatientPosition", "HFS"}}));

  // the whole object changed and given back: the changed metadata key wins
  json = header.metadata();
  json["PatientWeight"] = 72.0;
  QVERIFY(header.updateMetadata(json));
  QCOMPARE(header.patient_Weight(), 72.0f);

  // write and read back: the keys ECAT7 stores
  json = header.metadata();
  {
    CECATFile file(path, CECATMainHeader::ECAT7_Volume16);
    QVERIFY(file.open(QIODevice::WriteOnly));
    header.setMedIOData(&file);
    QVERIFY(file.writeMainHeader(header));
    header.setMedIOData(NULL);

    // a frame, so that the file can be read
    CMedIOVolume volume;
    QVERIFY(volume.setData(2, 2, 2, CMedIOVolume::Int16, QByteArray(16, 0)));
    QVERIFY(file.writeVolume(volume, 1));
    file.close();
  }
  CECATFile file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));
  CECATMainHeader* mh = NULL;
  QVERIFY(file.readMainHeader(mh));
  QScopedPointer<CECATMainHeader> mhGuard(mh);
  const QJsonObject written = mh->metadata();
  COMPARE_METADATA(written, json, QStringList() << "PatientName" << "PatientID" << "PatientBirthDate" << "PatientAge"
                                                << "PatientSize" << "PatientWeight" << "PatientPosition"
                                                << "AcquisitionDateTime" << "RadiopharmaceuticalStartDateTime"
                                                << "RadionuclideTotalDose" << "RadionuclideHalfLife"
                                                << "EnergyWindowLowerLimit" << "Units");
  QVERIFY(written.contains("PatientSex") == false);
  QVERIFY(written.contains("SeriesInstanceUID") == false);
  QVERIFY(written.contains("Philips") == false);
}

//---------------------------------------------------------------------------------------------
// ECAT7 birth dates: Unix time of midnight in the time zone of the writer;
// the date is the same on every machine

void TestMedIOMetadata::ecatBirthDate()
{
  CECAT7MainHeader header;
  const qint64 midnightUTC = static_cast<qint64>(QDate(1970, 1, 1).daysTo(QDate(1926, 6, 3))) * 86400;

  // written at midnight UTC, Europe/Berlin (+01:00), Asia/Tokyo (+09:00), Pacific/Honolulu (-10:00)
  foreach(int offsetHours, QList<int>() << 0 << 1 << 9 << -10) {
    header.setPatient_Birth_Date(static_cast<time_t>(midnightUTC - offsetHours * 3600));
    QCOMPARE(header.metadataValue("PatientBirthDate").toString(), QString("1926-06-03"));
  }

  // written by libmedio: midnight UTC
  CECAT7MainHeader written;
  QVERIFY(written.setMetadataValue("PatientBirthDate", QString("1926-06-03")));
  QCOMPARE(static_cast<qint64>(written.patient_Birth_Date()), midnightUTC);

  // an unchanged date is not written again
  QVERIFY(header.setMetadataValue("PatientBirthDate", QString("1926-06-03")));
  QCOMPARE(static_cast<qint64>(header.patient_Birth_Date()), midnightUTC + 10 * 3600);

  QVERIFY(header.setMetadataValue("PatientBirthDate", QJsonValue(QJsonValue::Null)));
  QVERIFY(header.metadataValue("PatientBirthDate").isUndefined());
}

//---------------------------------------------------------------------------------------------
// Philips: metadata of the test datasets (exported by ROVER from the DICOM
// series), compared with the DICOM series (pydicom) and with the times of the
// ECAT7 file of the same export (nibabel; both UTC)

void TestMedIOMetadata::philipsMetadata_data()
{
  QTest::addColumn<QString>("file");
  QTest::addColumn<QString>("ecat");
  QTest::addColumn<QString>("dicom");

  QTest::newRow("pet-static.img")  << "philips/pet-static.img" << "ecat/pet-static.v" << "dicom/pet-static";
  QTest::newRow("pet-static.syn")  << "philips/pet-static.syn" << "ecat/pet-static.v" << "dicom/pet-static";
  QTest::newRow("pet-dynamic.img") << "philips/pet-dynamic.img" << "ecat/pet-dynamic.v" << "dicom/pet-dynamic";
  QTest::newRow("pet-dynamic.syn") << "philips/pet-dynamic.syn" << "ecat/pet-dynamic.v" << "dicom/pet-dynamic";
}

void TestMedIOMetadata::philipsMetadata()
{
  QFETCH(QString, file);
  QFETCH(QString, ecat);
  QFETCH(QString, dicom);

  const QJsonObject original = expectedValues(dicom)["metadata"].toObject();
  const QJsonObject ecatValues = expectedValues(ecat)["metadata"].toObject();
  QVERIFY(original.isEmpty() == false && ecatValues.isEmpty() == false);

  CPhilipsFile philips(medioTestDataDir() + "/" + file);
  QVERIFY(philips.open(QIODevice::ReadOnly));
  CPhilipsMainHeader* mh = NULL;
  QVERIFY(philips.readMainHeader(mh));
  QScopedPointer<CPhilipsMainHeader> mhGuard(mh);

  const QJsonObject main = mh->metadata();
  COMPARE_METADATA(main, original["main"].toObject(),
                   QStringList() << "PatientName" << "PatientID" << "PatientBirthDate" << "PatientSex"
                                 << "PatientSize" << "PatientWeight" << "Radiopharmaceutical"
                                 << "RadionuclideTotalDose" << "RadionuclideHalfLife");
  COMPARE_METADATA(main, ecatValues["main"].toObject(),
                   QStringList() << "AcquisitionDateTime" << "RadiopharmaceuticalStartDateTime");

  // native fields in the units of Philips, without the fields describing the image data
  const QJsonObject native = main["Philips"].toObject();
  QCOMPARE(native["weight"].toDouble(), main["PatientWeight"].toDouble() * 1000.0);           // g
  QCOMPARE(native["height"].toDouble(), main["PatientSize"].toDouble() * 1000.0);             // mm
  QVERIFY(qAbs(native["activity"].toDouble() * 1.0e6 / main["RadionuclideTotalDose"].toDouble() - 1.0) < 1e-6);  // MBq
  QVERIFY(qAbs(native["halfLife"].toDouble() * 60.0 / main["RadionuclideHalfLife"].toDouble() - 1.0) < 1e-6);    // min
  foreach(const QString& field, QStringList() << "file_fmt" << "filtyp" << "nslice" << "nframe" << "dmax" << "slcthk")
    QVERIFY2(native.contains(field) == false, qPrintable(field));

  // frames (first slice)
  const QJsonArray originalFrames = original["frames"].toArray();
  const QList<short> frames = philips.frameNumbers();
  QCOMPARE(frames.size(), originalFrames.size());
  for(int i=0; i < frames.size(); i++) {
    CPhilipsSubHeader* sh = NULL;
    QVERIFY(philips.readSubHeader(sh, frames[i], -1));
    QScopedPointer<CPhilipsSubHeader> shGuard(sh);
    const QJsonObject frame = sh->metadata();
    COMPARE_METADATA(frame, originalFrames[i].toObject(), QStringList() << "ActualFrameDuration");
    QVERIFY(frame["Philips"].toObject().contains("suvscl") == false);
  }
}

//---------------------------------------------------------------------------------------------
// Philips: metadata written into a main header

void TestMedIOMetadata::philipsWrite()
{
  CPhilipsMainHeader header(NULL, CPhilipsMainHeader::Image);
  QJsonObject changes;
  changes["PatientName"] = QString("Doe^Jane");
  changes["PatientBirthDate"] = QString("1980-02-29");
  changes["PatientSex"] = QString("O");
  changes["PatientSize"] = 1.82;
  changes["PatientWeight"] = 71.5;
  changes["PatientPosition"] = QString("FFP");
  changes["AcquisitionDateTime"] = QString("2026-06-03T19:04:56+02:00");
  changes["RadiopharmaceuticalRoute"] = QString("intravenous");
  changes["RadionuclideTotalDose"] = 250.5e6;
  changes["RadionuclideHalfLife"] = 6586.2;
  changes["RadionuclideCodeSequence"] = QJsonObject{{"CodeValue", "C-111A1"}, {"CodingSchemeDesignator", "SRT"},
                                                    {"CodeMeaning", "^18^Fluorine"}};
  changes["SeriesNumber"] = 3;
  QVERIFY(header.updateMetadata(changes));

  // the native fields in the units of Philips
  QCOMPARE(header.height(), static_cast<short>(1820));
  QCOMPARE(header.weight(), 71500);
  QCOMPARE(header.activity(), 250.5f);
  QCOMPARE(header.halfLife(), 109.77f);
  QCOMPARE(header.sex(), 'O');
  QCOMPARE(header.bthyr(), static_cast<short>(1980));
  QCOMPARE(header.bthmo(), static_cast<short>(2));
  QCOMPARE(header.bthday(), static_cast<short>(29));
  QCOMPARE(header.orient_hf(), CPhilipsMainHeader::Feet_First);
  QCOMPARE(header.orient_ps(), CPhilipsMainHeader::Prone);
  QCOMPARE(header.route(), CPhilipsMainHeader::Intravenous);
  QCOMPARE(static_cast<qint64>(header.acq_date_time()), Q_INT64_C(1780506296));
  QCOMPARE(QString(header.isotope_code_value()), QString("C-111A1"));
  QCOMPARE(QString(header.Dpat_name()), QString("Doe^Jane"));

  // read back: DICOM units and texts
  const QJsonObject json = header.metadata();
  QCOMPARE(json["PatientSize"].toDouble(), 1.82);
  QCOMPARE(json["PatientWeight"].toDouble(), 71.5);
  QCOMPARE(json["RadionuclideTotalDose"].toDouble(), 250.5e6);
  QCOMPARE(json["RadionuclideHalfLife"].toDouble(), 6586.2);
  QCOMPARE(json["RadiopharmaceuticalRoute"].toString(), QString("Intravenous"));
  QCOMPARE(json["AcquisitionDateTime"].toString(), QString("2026-06-03T17:04:56Z"));
  QCOMPARE(json["RadionuclideCodeSequence"].toObject(), changes["RadionuclideCodeSequence"].toObject());

  // values Philips cannot store are kept in the object
  QVERIFY(header.setMetadataValue("RadiopharmaceuticalRoute", QString("Intranasal")));
  QCOMPARE(header.route(), CPhilipsMainHeader::UndefinedRoute);
  QCOMPARE(header.metadataValue("RadiopharmaceuticalRoute").toString(), QString("Intranasal"));
  QVERIFY(header.setMetadataValue("PatientWeight", 1.0e9));   // does not fit into the gram field
  QCOMPARE(header.weight(), 0);
  QCOMPARE(header.metadataValue("PatientWeight").toDouble(), 1.0e9);

  // native fields
  QVERIFY(header.updateMetadata(QJsonObject{{"Philips", QJsonObject{{"dep_acq_date", 20260603}}}}));
  QCOMPARE(header.dep_acq_date(), 20260603);
  QVERIFY(header.updateMetadata(QJsonObject{{"Philips", QJsonObject{{"nslice", 3}}}}) == false);
}

//---------------------------------------------------------------------------------------------
// microPET: metadata of a synthetic header (the units are given by the header)

static bool writeConcordeHeader(const QString& path, int doseUnits)
{
  QFile header(path + ".hdr");
  if(header.open(QIODevice::WriteOnly | QIODevice::Text) == false)
    return false;

  QTextStream h(&header);
  h << "# synthetic microPET header (libmedio test)\n"
    << "model 2000\n"
    << "institution Test Institute\n"
    << "file_type 5\n"
    << "total_frames 2\n"
    << "isotope F-18\n"
    << "isotope_half_life 6586.2\n"
    << "isotope_branching_fraction 0.9673\n"
    << "lld 350\n"
    << "uld 650\n"
    << "data_type 2\n"
    << "x_dimension 4\n"
    << "y_dimension 3\n"
    << "z_dimension 2\n"
    << "calibration_units 2\n"
    << "calibration_factor 1000\n"
    << "operator Operator Name\n"
    << "study_identifier S-1\n"
    << "scan_time Wed Jun 3 19:04:56 2026\n"
    << "injected_compound FDG\n"
    << "dose_units " << doseUnits << "\n"
    << "dose 2\n"
    << "injection_time Wed Jun 3 17:40:00 2026\n"
    << "subject_identifier mouse-7\n"
    << "subject_orientation 4\n"
    << "subject_length_units 2\n"
    << "subject_length 9\n"
    << "subject_weight_units 1\n"
    << "subject_weight 25\n"
    << "end_of_header\n";
  for(int f=0; f < 2; f++) {
    h << "frame " << f << "\n"
      << "event_type 1\n"
      << "data_file_pointer 0 " << f * 48 << "\n"
      << "frame_start " << f * 60 << "\n"
      << "frame_duration 60\n"
      << "scale_factor 1\n"
      << "deadtime_correction 1.05\n"
      << "decay_correction " << 1.2 + 0.1 * f << "\n"
      << "end_of_header\n";
  }
  header.close();

  QFile data(path);
  return data.open(QIODevice::WriteOnly) && data.write(QByteArray(96, 0)) == 96;
}

void TestMedIOMetadata::concordeMetadata()
{
  const QString path = m_tmp.filePath("mpet.img");
  QVERIFY(writeConcordeHeader(path, 1));   // dose in mCi

  CConcordeImage file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));
  CConcordeMainHeader* mh = NULL;
  QVERIFY(file.readMainHeader(mh));
  QScopedPointer<CConcordeMainHeader> mhGuard(mh);

  QJsonObject expected;
  expected["PatientID"] = QString("mouse-7");
  expected["PatientSize"] = 0.09;                 // 9 cm
  expected["PatientWeight"] = 0.025;              // 25 g
  expected["PatientPosition"] = QString("HFS");
  expected["StudyID"] = QString("S-1");
  expected["InstitutionName"] = QString("Test Institute");
  expected["OperatorsName"] = QJsonArray{QString("Operator Name")};
  expected["ManufacturerModelName"] = QString("2000");
  expected["AcquisitionDateTime"] = QString("2026-06-03T19:04:56");                 // local time, no time zone
  expected["RadiopharmaceuticalStartDateTime"] = QString("2026-06-03T17:40:00");
  expected["Radiopharmaceutical"] = QString("FDG");
  expected["RadionuclideTotalDose"] = 7.4e7;      // 2 mCi
  expected["RadionuclideHalfLife"] = 6586.2;
  expected["RadionuclidePositronFraction"] = 0.9673;
  expected["EnergyWindowLowerLimit"] = 350.0;
  expected["EnergyWindowUpperLimit"] = 650.0;
  expected["Units"] = QString("BQML");

  const QJsonObject main = mh->metadata();
  COMPARE_METADATA(main, expected);
  const QJsonObject native = main["microPET"].toObject();
  QCOMPARE(native["dose"].toDouble(), 2.0);
  QCOMPARE(native["subject_weight"].toDouble(), 25.0);
  QCOMPARE(native["isotope"].toString(), QString("F-18"));
  QVERIFY(native.contains("calibration_factor") == false);
  QVERIFY(native.contains("x_dimension") == false);

  for(int f=1; f <= 2; f++) {
    CConcordeFrameHeader* fh = NULL;
    QVERIFY(file.readSubHeader(fh, f));
    QScopedPointer<CConcordeFrameHeader> fhGuard(fh);
    const QJsonObject frame = fh->metadata();
    QJsonObject expectedFrame;
    expectedFrame["ActualFrameDuration"] = 60000.0;
    expectedFrame["DecayFactor"] = 1.2 + 0.1 * (f - 1);
    expectedFrame["DeadTimeFactor"] = 1.05;
    COMPARE_METADATA(frame, expectedFrame);
    QVERIFY(frame["microPET"].toObject().contains("scale_factor") == false);
  }
  file.close();

  // dose with unknown units: only in the native section
  const QString unknown = m_tmp.filePath("mpet-units.img");
  QVERIFY(writeConcordeHeader(unknown, 0));
  CConcordeImage file2(unknown);
  QVERIFY(file2.open(QIODevice::ReadOnly));
  CConcordeMainHeader* mh2 = NULL;
  QVERIFY(file2.readMainHeader(mh2));
  QScopedPointer<CConcordeMainHeader> mh2Guard(mh2);
  QVERIFY(mh2->metadataValue("RadionuclideTotalDose").isUndefined());
  QCOMPARE(mh2->metadata()["microPET"].toObject()["dose"].toDouble(), 2.0);

  // set in DICOM units: converted into the units of the header
  QVERIFY(mh2->setMetadataValue("RadionuclideTotalDose", 3.7e7));
  QCOMPARE(mh2->doseUnits(), CConcordeMainHeader::MBq);
  QCOMPARE(mh2->dose(), 37.0f);
  QVERIFY(mh2->setMetadataValue("PatientWeight", 0.03));
  QCOMPARE(mh2->subjectWeight(), 30.0f);   // g, the units of the header
}

//---------------------------------------------------------------------------------------------
// NIfTI: JSON written by earlier versions of CNIFTIConverter (ECAT7 main header
// fields at the top level; fixture written with nibabel)

// JSON header extension of a NIfTI file as stored
static QJsonObject niftiExtension(const QString& path)
{
  CNIFTIFile file(path);
  CNIFTIMainHeader* h = NULL;
  QJsonObject json;
  if(file.open(QIODevice::ReadOnly) && file.readMainHeader(h)) {
    json = (h->mainHeaderType() == CNIFTIMainHeader::NIFTI1MainHeader) ? static_cast<CNIFTI1MainHeader*>(h)->readHeaderExtension(file)
                                                                      : static_cast<CNIFTI2MainHeader*>(h)->readHeaderExtension(file);
  }
  delete h;
  return json;
}

void TestMedIOMetadata::niftiEarlierMetadata()
{
  CNIFTIFile file(QString(NIFTI_TEST_DATA_DIR) + "/n1_le_int16_ext.nii");
  QVERIFY(file.open(QIODevice::ReadOnly));
  CNIFTIMainHeader* h = NULL;
  QVERIFY(file.readMainHeader(h));
  QScopedPointer<CNIFTIMainHeader> hGuard(h);

  // {"Isotope_Name": "F-18", "Dosage": 300.5, "Frame_Duration": 600}
  const QJsonObject json = h->metadata();
  QCOMPARE(json["ECAT7"].toObject(), QJsonObject({{"Isotope_Name", "F-18"}, {"Dosage", 300.5}}));
  QCOMPARE(json["RadionuclideTotalDose"].toDouble(), 300.5);
  QCOMPARE(json["Frame_Duration"].toInt(), 600);
  QVERIFY(json.contains("NIfTI"));

  // no metadata keys of ECAT7 defaults (e.g. the energy window of a new ECAT7 header)
  QStringList keys;
  foreach(const CMedIOMetadata::Key* k, CMedIOMetadata::keys()) {
    if(json.contains(QString::fromLatin1(k->keyword)))
      keys.append(QString::fromLatin1(k->keyword));
  }
  QCOMPARE(keys, QStringList() << "RadionuclideTotalDose");
}

//---------------------------------------------------------------------------------------------
// NIfTI: metadata written as JSON header extension and read back

void TestMedIOMetadata::niftiWrite_data()
{
  QTest::addColumn<QString>("name");
  QTest::addColumn<int>("version");

  QTest::newRow("NIfTI-1")      << "meta1.nii" << 1;
  QTest::newRow("NIfTI-2")      << "meta2.nii" << 2;
  QTest::newRow("compressed")   << "meta3.nii.gz" << 1;
  QTest::newRow("pair")         << "meta4.hdr" << 1;
}

void TestMedIOMetadata::niftiWrite()
{
  QFETCH(QString, name);
  QFETCH(int, version);

  const QString path = m_tmp.filePath(name);
  QByteArray data(4 * 3 * 2 * 2, 0);
  for(int i=0; i < data.size() / 2; i++)
    reinterpret_cast<qint16*>(data.data())[i] = static_cast<qint16>(i * 7 - 50);

  QJsonObject metadata;
  metadata["PatientName"] = QString("Doe^Jane");
  metadata["PatientWeight"] = 71.5;
  metadata["AcquisitionDateTime"] = QString("2026-06-03T17:04:56Z");
  metadata["Radiopharmaceutical"] = QString("FDG");
  metadata["RadionuclideTotalDose"] = 2.505e8;
  metadata["DecayCorrection"] = QString("START");
  metadata["CorrectedImage"] = QJsonArray{QString("ATTN"), QString("DECY")};
  metadata["RadionuclideCodeSequence"] = QJsonObject{{"CodeValue", "C-111A1"}, {"CodeMeaning", "^18^Fluorine"}};
  metadata["(0029,1010)"] = QString("private");
  metadata["ECAT7"] = QJsonObject{{"Patient_Dexterity", "R"}, {"Bin_Size", 0.225}};
  metadata["Frames"] = QJsonArray{QJsonObject{{"DecayFactor", 1.00158}, {"ECAT7", QJsonObject{{"Annotation", "f1"}}}}};

  {
    CNIFTIFile file(path, version == 2 ? CNIFTIMainHeader::NIFTI2 : CNIFTIMainHeader::NIFTI1);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QScopedPointer<CNIFTIMainHeader> header(file.createEmptyHeader());
    if(version == 2) {
      CNIFTI2MainHeader* h = static_cast<CNIFTI2MainHeader*>(header.data());
      h->setDim(0, 3); h->setDim(1, 4); h->setDim(2, 3); h->setDim(3, 2);
      h->setDataType(4); h->setBit_Pix(16);
      h->setDescrip("metadata test");
    } else {
      CNIFTI1MainHeader* h = static_cast<CNIFTI1MainHeader*>(header.data());
      h->setDim(0, 3); h->setDim(1, 4); h->setDim(2, 3); h->setDim(3, 2);
      h->setDataType(4); h->setBit_Pix(16);
      h->setDescrip("metadata test");
    }
    QVERIFY(header->updateMetadata(metadata));
    QVERIFY(file.writeMainHeader(*header));
    QVERIFY(file.writeMatrix(data));
    file.close();
  }

  // the metadata and the BIDS keys derived from them in the header extension
  const QJsonObject extension = niftiExtension(path);
  QCOMPARE(extension["PatientWeight"].toDouble(), 71.5);
  QCOMPARE(extension["TracerName"].toString(), QString("FDG"));
  QCOMPARE(extension["InjectedRadioactivity"].toDouble(), 250.5);
  QCOMPARE(extension["InjectedRadioactivityUnits"].toString(), QString("MBq"));
  QCOMPARE(extension["ImageDecayCorrected"].toBool(), true);
  QCOMPARE(extension["DecayCorrectionFactor"].toArray(), QJsonArray{1.00158});
  QVERIFY(extension.contains("NIfTI") == false);

  // read back: the metadata as written, the voxel data unchanged
  CNIFTIFile file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));
  CNIFTIMainHeader* h = NULL;
  QVERIFY(file.readMainHeader(h));
  QScopedPointer<CNIFTIMainHeader> hGuard(h);
  QJsonObject json = h->metadata();
  QCOMPARE(json["NIfTI"].toObject()["descrip"].toString(), QString("metadata test"));
  json.remove("NIfTI");
  QCOMPARE(json, metadata);

  QByteArray* read = NULL;
  QVERIFY(file.readMatrix(read));
  QVERIFY(read->left(data.size()) == data);
  delete read;
}

//---------------------------------------------------------------------------------------------
// NIfTI: BIDS keys of other tools (JSON sidecar file)

void TestMedIOMetadata::niftiBids()
{
  const QString path = m_tmp.filePath("bids.nii");
  {
    CNIFTIFile file(path, CNIFTIMainHeader::NIFTI1);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QScopedPointer<CNIFTIMainHeader> header(file.createEmptyHeader());
    CNIFTI1MainHeader* h = static_cast<CNIFTI1MainHeader*>(header.data());
    h->setDim(0, 3); h->setDim(1, 2); h->setDim(2, 2); h->setDim(3, 2);
    h->setDataType(4); h->setBit_Pix(16);
    QVERIFY(file.writeMainHeader(*header));
    QVERIFY(file.writeMatrix(QByteArray(16, 0)));
    file.close();
  }

  QFile sidecar(m_tmp.filePath("bids.json"));
  QVERIFY(sidecar.open(QIODevice::WriteOnly));
  sidecar.write("{\"TracerName\": \"FDG\", \"InjectedRadioactivity\": 370, \"InjectedRadioactivityUnits\": \"MBq\","
                " \"ManufacturersModelName\": \"Biograph\", \"Units\": \"Bq/mL\", \"DecayCorrectionFactor\": [1.01, 1.02],"
                " \"FrameTimesStart\": [0, 60], \"FrameDuration\": [60, 60], \"PatientWeight\": 70}");
  sidecar.close();

  CNIFTIFile file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));
  CNIFTIMainHeader* h = NULL;
  QVERIFY(file.readMainHeader(h));
  QScopedPointer<CNIFTIMainHeader> hGuard(h);
  const QJsonObject json = h->metadata();

  QCOMPARE(json["Radiopharmaceutical"].toString(), QString("FDG"));
  QCOMPARE(json["RadionuclideTotalDose"].toDouble(), 3.7e8);
  QCOMPARE(json["ManufacturerModelName"].toString(), QString("Biograph"));
  QCOMPARE(json["Units"].toString(), QString("BQML"));
  QCOMPARE(json["PatientWeight"].toDouble(), 70.0);
  QCOMPARE(json["Frames"].toArray().size(), 2);
  QCOMPARE(json["Frames"].toArray()[1].toObject()["DecayFactor"].toDouble(), 1.02);
  // the frame timing of NIfTI files is kept, the derived BIDS keys are not
  QVERIFY(json.contains("FrameTimesStart") && json.contains("FrameDuration"));
  QVERIFY(json.contains("TracerName") == false && json.contains("InjectedRadioactivity") == false);
}

QTEST_GUILESS_MAIN(TestMedIOMetadata)
#include "testMedIOMetadata.moc"
