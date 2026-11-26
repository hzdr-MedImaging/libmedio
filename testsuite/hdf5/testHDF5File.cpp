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

// Tests of CHDF5File: HDF5 files are identified by their signature in every
// build, with and without HDF5 support. Reading and writing them is not
// implemented yet.
//
// The files of the tests are written byte by byte: the signature of the HDF5
// file format specification ("\211HDF\r\n\032\n") at the start of the file or,
// after a user block, at offset 512, 1024, 2048, ...

#include <QtTest/QtTest>
#include <QScopedPointer>
#include <QTemporaryDir>

#include "CHDF5File.h"
#include "CMedIODataFactory.h"
#include "MedIOTestData.h"

class TestHDF5File : public QObject
{
  Q_OBJECT

  private slots:
    void signature_data();
    void signature();
    void missingFile();
    void factory();
    void formatNames();
    void support();
    void openFails();
    void otherFormats();
};

static const QByteArray hdf5Signature("\x89HDF\r\n\x1a\n", 8);

static bool writeFile(const QString& fileName, const QByteArray& content)
{
  QFile file(fileName);
  return file.open(QIODevice::WriteOnly) && file.write(content) == content.size();
}

// a file of the given size with the signature at an offset
static QByteArray fileWithSignature(int offset, int size)
{
  QByteArray content(size, '\0');
  content.replace(offset, hdf5Signature.size(), hdf5Signature);
  return content;
}

void TestHDF5File::signature_data()
{
  QTest::addColumn<QByteArray>("content");
  QTest::addColumn<bool>("hdf5");

  QTest::newRow("signature only") << hdf5Signature << true;
  QTest::newRow("at the start") << fileWithSignature(0, 2048) << true;
  QTest::newRow("user block of 512 bytes") << fileWithSignature(512, 4096) << true;
  QTest::newRow("user block of 1024 bytes") << fileWithSignature(1024, 4096) << true;
  QTest::newRow("user block of 2048 bytes") << fileWithSignature(2048, 4096) << true;
  QTest::newRow("user block of 1 MiB") << fileWithSignature(1 << 20, (1 << 20) + 8) << true;
  QTest::newRow("user block with text") << QByteArray("#!/bin/sh\n").leftJustified(512, ' ') + hdf5Signature << true;

  QTest::newRow("empty file") << QByteArray() << false;
  QTest::newRow("zeros") << QByteArray(4096, '\0') << false;
  QTest::newRow("text") << QByteArray("no HDF5 file\n") << false;
  QTest::newRow("truncated signature") << hdf5Signature.left(7) << false;
  QTest::newRow("truncated signature after a user block") << fileWithSignature(512, 520).left(519) << false;
  QTest::newRow("wrong last byte") << QByteArray("\x89HDF\r\n\x1a\r", 8) << false;
  QTest::newRow("without the first byte") << QByteArray("HDF\r\n\x1a\n\0", 8) << false;
  // no valid sizes of a user block
  QTest::newRow("at offset 8") << fileWithSignature(8, 4096) << false;
  QTest::newRow("at offset 256") << fileWithSignature(256, 4096) << false;
  QTest::newRow("at offset 1536") << fileWithSignature(1536, 4096) << false;
}

void TestHDF5File::signature()
{
  QFETCH(QByteArray, content);
  QFETCH(bool, hdf5);

  QTemporaryDir tmpDir;
  QVERIFY(tmpDir.isValid());
  // the name of the file does not matter
  const QString fileName = tmpDir.filePath(hdf5 ? "image.dat" : "image.h5");
  QVERIFY(writeFile(fileName, content));

  QCOMPARE(CHDF5File::isOfType(fileName), hdf5);
  QCOMPARE(CMedIODataFactory::identify(fileName) == CMedIOData::HDF5, hdf5);

  QScopedPointer<CMedIOData> file(CHDF5File::createFromFile(fileName));
  QCOMPARE(file.isNull(), hdf5 == false);
}

void TestHDF5File::missingFile()
{
  QTemporaryDir tmpDir;
  QVERIFY(tmpDir.isValid());
  const QString fileName = tmpDir.filePath("missing.h5");

  QVERIFY(CHDF5File::isOfType(fileName) == false);
  QVERIFY(CHDF5File::createFromFile(fileName) == NULL);
  QVERIFY(CHDF5File::isOfType(tmpDir.path()) == false);   // a directory
  QCOMPARE(CMedIODataFactory::identify(fileName), CMedIOData::Unknown);
}

void TestHDF5File::factory()
{
  QTemporaryDir tmpDir;
  QVERIFY(tmpDir.isValid());
  const QString fileName = tmpDir.filePath("image.h5");
  QVERIFY(writeFile(fileName, fileWithSignature(0, 2048)));

  QScopedPointer<CMedIOData> file(CMedIODataFactory::create(fileName));
  QVERIFY(file.isNull() == false);
  QCOMPARE(file->dataFormat(), CMedIOData::HDF5);
  QVERIFY(dynamic_cast<CHDF5File*>(file.data()) != NULL);
  QCOMPARE(file->fileName(), fileName);
  QVERIFY(file->isOpen() == false);
}

void TestHDF5File::formatNames()
{
  QCOMPARE(QString(CMedIODataFactory::formatName(CMedIOData::HDF5)), QString("hdf5"));

  foreach(const QString& name, QStringList() << "hdf5" << "HDF5" << "h5" << "hdf")
  {
    CMedIOData::Format format = CMedIOData::Unknown;
    QVERIFY2(CMedIODataFactory::formatFromName(name, format), qPrintable(name));
    QCOMPARE(format, CMedIOData::HDF5);
  }

  // the names of the other formats are unchanged
  const QList<CMedIOData::Format> formats = QList<CMedIOData::Format>()
      << CMedIOData::Unknown << CMedIOData::ConcordeMicropet << CMedIOData::ECAT << CMedIOData::Philips
      << CMedIOData::PhilipsBin << CMedIOData::NIFTI << CMedIOData::Interfile << CMedIOData::HDF5;
  foreach(CMedIOData::Format expected, formats)
  {
    CMedIOData::Format format = CMedIOData::HDF5;
    QVERIFY(CMedIODataFactory::formatFromName(CMedIODataFactory::formatName(expected), format));
    QCOMPARE(format, expected);
  }
}

// the HDF5 support the build was configured with (MEDIO_WITH_HDF5)
void TestHDF5File::support()
{
#ifdef MEDIO_TEST_HAVE_HDF5
  QVERIFY(CHDF5File::hasSupport());
  QCOMPARE(CHDF5File::libraryVersion(), QString(MEDIO_TEST_HDF5_VERSION));
#else
  QVERIFY(CHDF5File::hasSupport() == false);
  QCOMPARE(CHDF5File::libraryVersion(), QString());
#endif
}

// not implemented yet: open() fails with a reason and leaves no file behind
void TestHDF5File::openFails()
{
  QTemporaryDir tmpDir;
  QVERIFY(tmpDir.isValid());
  const QString existing = tmpDir.filePath("image.h5");
  const QByteArray content = fileWithSignature(0, 2048);
  QVERIFY(writeFile(existing, content));

  CHDF5File file(existing);
  QCOMPARE(file.openError(), QString());
  QVERIFY(file.open(QIODevice::ReadOnly) == false);
  QVERIFY(file.isOpen() == false);
  QVERIFY(file.openError().isEmpty() == false);
  QCOMPARE(file.openError().contains("without HDF5 support"), CHDF5File::hasSupport() == false);
  file.close();

  // the generic interface fails on a file that is not open
  CMedIOHeader* header = NULL;
  QVERIFY(file.readMainHeader(header) == false);
  QVERIFY(header == NULL);
  QVERIFY(file.frameNumbers().isEmpty());

  // writing: nothing is created, an existing file is not changed
  const QString created = tmpDir.filePath("new.h5");
  CHDF5File out(created);
  QVERIFY(out.open(QIODevice::WriteOnly) == false);
  QVERIFY(QFile::exists(created) == false);
  CHDF5File overwrite(existing);
  QVERIFY(overwrite.open(QIODevice::WriteOnly) == false);
  QFile check(existing);
  QVERIFY(check.open(QIODevice::ReadOnly));
  QCOMPARE(check.readAll(), content);
}

// the test datasets of the other formats are no HDF5 files
void TestHDF5File::otherFormats()
{
  const MedIOTestData data = medioTestData();
  if(data.files.isEmpty())
    QSKIP("no test data");

  foreach(const MedIOTestFile& file, data.files)
  {
    QVERIFY2(CHDF5File::isOfType(file.path) == false, qPrintable(file.path));
    QVERIFY2(CMedIODataFactory::identify(file.path) != CMedIOData::HDF5, qPrintable(file.path));
  }
}

QTEST_GUILESS_MAIN(TestHDF5File)
#include "testHDF5File.moc"
