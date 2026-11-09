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

#include <QtTest/QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#ifndef METADATA_TEST_DATA_DIR
#error "METADATA_TEST_DATA_DIR has to be defined"
#endif

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

QTEST_GUILESS_MAIN(TestMedIOMetadata)
#include "testMedIOMetadata.moc"
