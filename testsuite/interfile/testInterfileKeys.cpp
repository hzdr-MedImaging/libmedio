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

// Tests of CInterfileKeys: the syntax of Interfile headers (Interfile 3.3, the PET
// proposal/STIR and the keys of Siemens and PMOD), independent of the image data.

#include <QtTest/QtTest>
#include <QTemporaryDir>

#include "CInterfileKeys.h"

class TestInterfileKeys : public QObject
{
  Q_OBJECT

  private slots:
    void normalizedName_data();
    void normalizedName();
    void parse();
    void lineEndings_data();
    void lineEndings();
    void commentsAndUnknownLines();
    void dataAfterHeader();
    void invalidHeaders_data();
    void invalidHeaders();
    void encoding();
    void lookup();
    void modify();
    void roundTrip();
    void values();
    void fileAccess();
};

static QList<int> indices(int a = 0, int b = 0)
{
  QList<int> result;
  if(a)
    result << a;
  if(b)
    result << b;
  return result;
}

void TestInterfileKeys::normalizedName_data()
{
  QTest::addColumn<QString>("key");
  QTest::addColumn<QString>("name");
  QTest::addColumn<QList<int> >("indices");
  QTest::addColumn<bool>("required");
  QTest::addColumn<bool>("vendor");

  QTest::newRow("plain") << "imaging modality" << "imaging modality" << indices() << false << false;
  QTest::newRow("required") << "!INTERFILE" << "interfile" << indices() << true << false;
  QTest::newRow("case and spaces") << "  !Imaging   MODALITY " << "imaging modality" << indices() << true << false;
  QTest::newRow("underscore") << "study_time" << "study time" << indices() << false << false;
  QTest::newRow("tab") << "number\tformat" << "number format" << indices() << false << false;
  QTest::newRow("index") << "!matrix size [1]" << "matrix size" << indices(1) << true << false;
  QTest::newRow("index without space") << "image duration (sec)[12]" << "image duration (sec)" << indices(12) << false << false;
  QTest::newRow("index with spaces") << "data offset in bytes [ 3 ]" << "data offset in bytes" << indices(3) << false << false;
  QTest::newRow("two indices") << "total prompts[2][1]" << "total prompts" << indices(2, 1) << false << false;
  QTest::newRow("unit") << "scaling factor (mm/pixel) [2]" << "scaling factor (mm/pixel)" << indices(2) << false << false;
  QTest::newRow("unit without space") << "Image_Duration(Sec)" << "image duration (sec)" << indices() << false << false;
  QTest::newRow("unit with spaces") << "image duration ( sec )" << "image duration (sec)" << indices() << false << false;
  QTest::newRow("vendor") << "%SMS-MI header name space" << "sms-mi header name space" << indices() << false << true;
  QTest::newRow("vendor with index") << "%energy window lower level (keV) [1]" << "energy window lower level (kev)"
                                     << indices(1) << false << true;
  QTest::newRow("required and vendor") << "!%total number of sinograms" << "total number of sinograms" << indices()
                                       << true << true;
}

void TestInterfileKeys::normalizedName()
{
  QFETCH(QString, key);
  QFETCH(QString, name);
  QFETCH(QList<int>, indices);
  QFETCH(bool, required);
  QFETCH(bool, vendor);

  QList<int> keyIndices;
  bool keyRequired = !required;
  bool keyVendor = !vendor;
  QCOMPARE(CInterfileKeys::normalizedName(key, &keyIndices, &keyRequired, &keyVendor), name);
  QCOMPARE(keyIndices, indices);
  QCOMPARE(keyRequired, required);
  QCOMPARE(keyVendor, vendor);
}

void TestInterfileKeys::parse()
{
  const QByteArray text =
      "!INTERFILE :=\n"
      "!imaging modality := nucmed\n"
      "!version of keys := 3.3\n"
      "!name of data file := image.img\n"
      "patient name := Doe^Jane\n"
      "energy window [1] := \n"
      "!matrix size [1] := 128\n"
      "minimum ring difference per segment := { -4, -1, 2 }\n"
      "%axial compression := 11\n"
      "!END OF INTERFILE :=\n";

  CInterfileKeys keys;
  QString error;
  qint64 headerSize = 0;
  QVERIFY2(keys.parse(text, &error, &headerSize), qPrintable(error));
  QCOMPARE(headerSize, qint64(text.size()));
  QCOMPARE(keys.count(), 10);

  QCOMPARE(keys.at(0).name, QString("interfile"));
  QVERIFY(keys.at(0).required);
  QCOMPARE(keys.at(0).value, QString(""));
  QCOMPARE(keys.at(4).text, QString("patient name"));
  QCOMPARE(keys.at(4).value, QString("Doe^Jane"));
  QVERIFY(keys.at(4).required == false);
  QCOMPARE(keys.at(5).name, QString("energy window"));
  QCOMPARE(keys.at(5).indices, indices(1));
  QVERIFY(keys.at(5).value.isEmpty());
  QCOMPARE(keys.at(6).text, QString("!matrix size [1]"));
  QCOMPARE(keys.at(8).name, QString("axial compression"));
  QVERIFY(keys.at(8).vendor);
  QCOMPARE(keys.at(9).name, QString("end of interfile"));
}

void TestInterfileKeys::lineEndings_data()
{
  QTest::addColumn<QByteArray>("lineEnd");
  QTest::newRow("LF") << QByteArray("\n");
  QTest::newRow("CR LF") << QByteArray("\r\n");
  QTest::newRow("CR") << QByteArray("\r");
}

void TestInterfileKeys::lineEndings()
{
  QFETCH(QByteArray, lineEnd);

  const QByteArray text = "!INTERFILE :=" + lineEnd + lineEnd +
                          "!matrix size [1] := 64" + lineEnd +
                          "!END OF INTERFILE :=" + lineEnd;
  CInterfileKeys keys;
  qint64 headerSize = 0;
  QVERIFY(keys.parse(text, 0, &headerSize));
  QCOMPARE(keys.count(), 3);
  QCOMPARE(keys.value("matrix size [1]"), QString("64"));
  QCOMPARE(headerSize, qint64(text.size()));
}

void TestInterfileKeys::commentsAndUnknownLines()
{
  const QByteArray text =
      "; written by a test\n"
      "\n"
      "!INTERFILE :=\n"
      ";only first peak levels are specified\n"
      "a line without separator\n"
      "pmod patient weight := 8.63\n"
      "!END OF INTERFILE :=\n";

  CInterfileKeys keys;
  QString error;
  QVERIFY2(keys.parse(text, &error), qPrintable(error));
  // the comment before "!INTERFILE" is not part of the header
  QCOMPARE(keys.count(), 5);
  QVERIFY(keys.at(1).comment);
  QCOMPARE(keys.at(1).text, QString(";only first peak levels are specified"));
  // a line without ":=" is kept as comment
  QVERIFY(keys.at(2).comment);
  QCOMPARE(keys.at(2).text, QString("; a line without separator"));
  // unknown keys are kept with their values
  QCOMPARE(keys.value("PMOD patient weight"), QString("8.63"));
  // comments are no keys
  QCOMPARE(keys.indexOf(";only first peak levels are specified"), -1);
}

void TestInterfileKeys::dataAfterHeader()
{
  // a file holding header and data (Interfile 3.3 single file, e.g. PMOD): the
  // header ends with ^Z, the binary data follows
  QByteArray text = "!INTERFILE :=\r\n!data offset in bytes := 64\r\n!END OF INTERFILE :=\r\n";
  const qint64 size = text.size();
  text += QByteArray("\x1a\x00\xff\x7f\r\n:=\n", 10) + QByteArray(100, '\0');

  CInterfileKeys keys;
  qint64 headerSize = 0;
  QVERIFY(keys.parse(text, 0, &headerSize));
  QCOMPARE(headerSize, size);
  QCOMPARE(keys.count(), 3);
}

void TestInterfileKeys::invalidHeaders_data()
{
  QTest::addColumn<QByteArray>("text");
  QTest::newRow("empty") << QByteArray();
  QTest::newRow("no INTERFILE") << QByteArray("!imaging modality := nucmed\n!END OF INTERFILE :=\n");
  QTest::newRow("no END") << QByteArray("!INTERFILE :=\n!imaging modality := nucmed\n");
  QTest::newRow("binary data") << QByteArray("\x00\x01\x02\x03 binary", 12);
  QTest::newRow("NIfTI header") << (QByteArray("\x5c\x01\x00\x00", 4) + QByteArray(344, '\0'));
  QTest::newRow("binary before END") << (QByteArray("!INTERFILE :=\n") + QByteArray("\x00\x01\x02\n", 4) +
                                         QByteArray("!END OF INTERFILE :=\n"));
}

void TestInterfileKeys::invalidHeaders()
{
  QFETCH(QByteArray, text);

  CInterfileKeys keys;
  QString error;
  QVERIFY(keys.parse(text, &error) == false);
  QVERIFY(error.isEmpty() == false);
}

void TestInterfileKeys::encoding()
{
  // UTF-8 and Latin-1 (older headers) patient names
  CInterfileKeys keys;
  QVERIFY(keys.parse("!INTERFILE :=\npatient name := M\xc3\xbcller\n!END OF INTERFILE :=\n"));
  QCOMPARE(keys.value("patient name"), QString::fromUtf8("M\xc3\xbcller"));
  QVERIFY(keys.parse("!INTERFILE :=\npatient name := M\xfcller\n!END OF INTERFILE :=\n"));
  QCOMPARE(keys.value("patient name"), QString::fromUtf8("M\xc3\xbcller"));
}

void TestInterfileKeys::lookup()
{
  const QByteArray text =
      "!INTERFILE :=\n"
      "!matrix size [1] := 31\n"
      "image duration (sec) := 30\n"
      "image duration (sec)[1] := 60\n"
      "!frame group number := 1\n"
      "!image duration (sec) := 90\n"
      "!END OF INTERFILE :=\n";

  CInterfileKeys keys;
  QVERIFY(keys.parse(text));

  // the name is compared normalized, the indices exactly
  QCOMPARE(keys.value("Matrix_Size", indices(1)), QString("31"));
  QCOMPARE(keys.value("!matrix size [1]"), QString("31"));
  QVERIFY(keys.contains("matrix size") == false);
  QCOMPARE(keys.value("matrix size", indices(2), "none"), QString("none"));
  QCOMPARE(keys.value("image duration (sec)"), QString("30"));
  QCOMPARE(keys.value("image duration (sec)", indices(1)), QString("60"));

  // repeated keys: the next one from a position on
  const int first = keys.indexOf("image duration (sec)");
  QCOMPARE(first, 2);
  QCOMPARE(keys.indexOf("image duration (sec)", QList<int>(), first + 1), 5);
  QCOMPARE(keys.indexOf("image duration (sec)", QList<int>(), 6), -1);
}

void TestInterfileKeys::modify()
{
  CInterfileKeys keys;
  keys.append("!INTERFILE", "");
  keys.append("!matrix size [1]", "128");
  keys.appendComment("a comment");
  keys.setValue("!matrix size [1]", " 64 ");
  keys.setValue("scaling factor (mm/pixel) [1]", "2.5");
  keys.append("!END OF INTERFILE", "");

  QCOMPARE(keys.count(), 5);
  QCOMPARE(keys.value("matrix size", indices(1)), QString("64"));
  QCOMPARE(keys.value("scaling factor (mm/pixel)", indices(1)), QString("2.5"));
  QVERIFY(keys.at(1).required);
  QVERIFY(keys.at(2).comment);

  QCOMPARE(keys.remove("matrix size", indices(1)), 1);
  QCOMPARE(keys.count(), 4);
  QCOMPARE(keys.remove("matrix size", indices(1)), 0);

  keys.clear();
  QCOMPARE(keys.count(), 0);
}

void TestInterfileKeys::roundTrip()
{
  const QByteArray text =
      "!INTERFILE :=\n"
      "!imaging modality := nucmed\n"
      ";a comment\n"
      "patient name := Doe^Jane\n"
      "energy window [1] := \n"
      "%axial compression := 11\n"
      "matrix size [3] := {95, 85, 85}\n"
      "!END OF INTERFILE :=\n";

  CInterfileKeys keys;
  QVERIFY(keys.parse(text));

  // written with CR LF and the keys as they were written
  const QByteArray written = keys.toText();
  QVERIFY(written.startsWith("!INTERFILE :=\r\n!imaging modality := nucmed\r\n;a comment\r\n"));
  QVERIFY(written.contains("\r\nenergy window [1] :=\r\n"));
  QVERIFY(written.endsWith("!END OF INTERFILE :=\r\n"));

  CInterfileKeys again;
  QVERIFY(again.parse(written));
  QCOMPARE(again.count(), keys.count());
  for(int i = 0; i < keys.count(); i++)
  {
    QCOMPARE(again.at(i).text, keys.at(i).text);
    QCOMPARE(again.at(i).name, keys.at(i).name);
    QCOMPARE(again.at(i).indices, keys.at(i).indices);
    QCOMPARE(again.at(i).value, keys.at(i).value);
    QCOMPARE(again.at(i).comment, keys.at(i).comment);
  }

  // characters beyond Latin-1 are written as UTF-8
  keys.setValue("patient name", QString::fromUtf8("\xe5\xb1\xb1\xe7\x94\xb0"));
  QVERIFY(keys.toText().contains("patient name := \xe5\xb1\xb1\xe7\x94\xb0\r\n"));
}

void TestInterfileKeys::values()
{
  qint64 i = 0;
  QVERIFY(CInterfileKeys::toInt(" 3370 ", &i));
  QCOMPARE(i, qint64(3370));
  QVERIFY(CInterfileKeys::toInt("-7", &i));
  QCOMPARE(i, qint64(-7));
  QVERIFY(CInterfileKeys::toInt("1.5", &i) == false);
  QVERIFY(CInterfileKeys::toInt("", &i) == false);

  double d = 0.0;
  QVERIFY(CInterfileKeys::toDouble("1.6455695000000001", &d));
  QCOMPARE(d, 1.6455695000000001);
  QVERIFY(CInterfileKeys::toDouble(" -0.0 ", &d));
  QCOMPARE(d, 0.0);
  QVERIFY(CInterfileKeys::toDouble("2.5e-3", &d));
  QCOMPARE(d, 2.5e-3);
  QVERIFY(CInterfileKeys::toDouble("1,5", &d) == false);
  QVERIFY(CInterfileKeys::toDouble("kBq/cc", &d) == false);

  QCOMPARE(CInterfileKeys::toList("{ -4, -1 ,2}"), QStringList() << "-4" << "-1" << "2");
  QCOMPARE(CInterfileKeys::toList("{}"), QStringList());
  QCOMPARE(CInterfileKeys::toList(" 128 "), QStringList() << "128");
  QCOMPARE(CInterfileKeys::toList("{arc correction}"), QStringList() << "arc correction");
}

void TestInterfileKeys::fileAccess()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  // a header file, a file holding header and data, and files that are no Interfile
  const QString header = dir.filePath("image.hv");
  const QString single = dir.filePath("single.hdr");
  const QString nifti = dir.filePath("image.hdr");
  const QString other = dir.filePath("other.txt");
  QFile file(header);
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write("\xEF\xBB\xBF; comment\r\n!INTERFILE :=\r\n!matrix size [1] := 8\r\n!END OF INTERFILE :=\r\n");
  file.close();
  file.setFileName(single);
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write("!INTERFILE :=\r\n!END OF INTERFILE :=\r\n\x1a");
  file.write(QByteArray(200 * 1024, '\x01'));
  file.close();
  file.setFileName(nifti);
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write(QByteArray("\x5c\x01\x00\x00", 4) + QByteArray(344, '\0'));
  file.close();
  file.setFileName(other);
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write("imaging modality := nucmed\n");
  file.close();

  QVERIFY(CInterfileKeys::isInterfile(header));
  QVERIFY(CInterfileKeys::isInterfile(single));
  QVERIFY(CInterfileKeys::isInterfile(nifti) == false);
  QVERIFY(CInterfileKeys::isInterfile(other) == false);
  QVERIFY(CInterfileKeys::isInterfile(dir.filePath("missing.hv")) == false);

  CInterfileKeys keys;
  QString error;
  qint64 headerSize = 0;
  QVERIFY2(keys.read(header, &error, &headerSize), qPrintable(error));
  QCOMPARE(keys.value("matrix size", indices(1)), QString("8"));

  QVERIFY2(keys.read(single, &error, &headerSize), qPrintable(error));
  QCOMPARE(headerSize, qint64(37));

  QVERIFY(keys.read(nifti, &error) == false);
  QVERIFY(error.contains("image.hdr"));
  QVERIFY(keys.read(dir.filePath("missing.hv"), &error) == false);
  QVERIFY(error.contains("cannot open"));
}

QTEST_GUILESS_MAIN(TestInterfileKeys)
#include "testInterfileKeys.moc"
