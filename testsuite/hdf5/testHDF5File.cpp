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
// build, with and without HDF5 support. With HDF5 support, images of the layout
// of docs/hdf5.md are read; the fixtures of testsuite/hdf5/data and the values
// expected from them (expected.json) are made with h5py (generate_fixtures.py).
// Writing is not implemented yet.
//
// The files of the tests are written byte by byte: the signature of the HDF5
// file format specification ("\211HDF\r\n\032\n") at the start of the file or,
// after a user block, at offset 512, 1024, 2048, ...

#include <QtTest/QtTest>
#include <QScopedPointer>
#include <QTemporaryDir>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <cstring>

#include "CHDF5File.h"
#include "CHDF5FrameHeader.h"
#include "CHDF5MainHeader.h"
#include "CMedIODataFactory.h"
#include "CMedIOHeaderDictionary.h"
#include "CMedIOVolume.h"
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
    void fixtures_data();
    void fixtures();
    void genericInterface();
    void headers();
    void closedFile();
};

// the fixtures and the values expected from them
static QString fixture(const QString& name)
{
  return QString(MEDIO_HDF5_FIXTURE_DIR) + "/" + name;
}

static QJsonObject expectedFiles()
{
  QFile file(fixture("expected.json"));
  if(file.open(QIODevice::ReadOnly) == false)
    return QJsonObject();
  return QJsonDocument::fromJson(file.readAll()).object().value("files").toObject();
}

// the values of a matrix as double
static QList<double> valuesOf(const QByteArray& data, CMedIOVolume::DataType type)
{
  QList<double> values;
  if(type == CMedIOVolume::Float64)
  {
    for(int i = 0; i + 8 <= data.size(); i += 8)
    {
      double v;
      memcpy(&v, data.constData() + i, 8);
      values << v;
    }
  }
  else
  {
    for(int i = 0; i + 4 <= data.size(); i += 4)
    {
      float v;
      memcpy(&v, data.constData() + i, 4);
      values << static_cast<double>(v);
    }
  }
  return values;
}

static QList<double> numbersOf(const QJsonArray& array)
{
  QList<double> values;
  foreach(const QJsonValue& v, array)
    values << v.toDouble();
  return values;
}

static QByteArray jsonText(const QJsonObject& object)
{
  return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

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

// files that are no images and writing, which is not implemented yet: open() fails
// with a reason and leaves no file behind
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

// every fixture: the valid ones are read as h5py reads them, the others are refused
// with the reason; without HDF5 support no file can be opened
void TestHDF5File::fixtures_data()
{
  QTest::addColumn<QString>("name");
  QTest::addColumn<QJsonObject>("expected");

  const QJsonObject files = expectedFiles();
  for(QJsonObject::const_iterator it = files.begin(); it != files.end(); ++it)
    QTest::newRow(qPrintable(it.key())) << it.key() << it.value().toObject();
}

void TestHDF5File::fixtures()
{
  QFETCH(QString, name);
  QFETCH(QJsonObject, expected);

  // identified in every build, by the factory as well
  QVERIFY(CHDF5File::isOfType(fixture(name)));
  QScopedPointer<CMedIOData> created(CMedIODataFactory::create(fixture(name)));
  QVERIFY(created.isNull() == false && created->dataFormat() == CMedIOData::HDF5);

  CHDF5File file(fixture(name));
  const bool opened = file.open(QIODevice::ReadOnly);

  if(CHDF5File::hasSupport() == false)
  {
    QVERIFY(opened == false && file.isOpen() == false);
    QVERIFY(file.openError().contains("without HDF5 support"));
    return;
  }

  if(expected.value("valid").toBool() == false)
  {
    QVERIFY(opened == false && file.isOpen() == false);
    QVERIFY2(file.openError().contains(expected.value("reason").toString()), qPrintable(file.openError()));
    QCOMPARE(file.numVolumes(), 0);
    return;
  }

  QVERIFY2(opened, qPrintable(file.openError()));
  QVERIFY(file.isOpen());
  QCOMPARE(file.openError(), QString());

  const QJsonArray volumes = expected.value("volumes").toArray();
  const QJsonArray dimensions = expected.value("dimensions").toArray();
  const CMedIOVolume::DataType type = expected.value("datatype").toString() == "float64" ? CMedIOVolume::Float64
                                                                                         : CMedIOVolume::Float32;
  QCOMPARE(file.numVolumes(), volumes.size());

  QList<short> frames;
  foreach(const QJsonValue& frame, expected.value("first_gate_frames").toArray())
    frames << static_cast<short>(frame.toInt());
  QCOMPARE(file.firstGateFrames(), frames);
  QCOMPARE(file.frameNumbers(), frames);

  // the main header
  CHDF5MainHeader* mainHeader = NULL;
  QVERIFY(file.readMainHeader(mainHeader) && mainHeader != NULL);
  QScopedPointer<CHDF5MainHeader> mainGuard(mainHeader);
  QCOMPARE(mainHeader->formatVersion(), expected.value("version").toString());
  QCOMPARE(mainHeader->numVolumes(), volumes.size());
  QCOMPARE(mainHeader->numFrames(), frames.size());
  QVERIFY(mainHeader->medIOData() == &file);
  QCOMPARE(jsonText(mainHeader->attributes()), jsonText(expected.value("attributes").toObject()));

  // every volume
  for(int k = 0; k < volumes.size(); k++)
  {
    const QJsonObject volume = volumes[k].toObject();
    QByteArray* data = NULL;
    CHDF5FrameHeader* header = NULL;
    QVERIFY(file.readVolumeMatrix(data, header, k) && data != NULL && header != NULL);
    QScopedPointer<QByteArray> dataGuard(data);
    QScopedPointer<CHDF5FrameHeader> headerGuard(header);

    QCOMPARE(header->volume(), k);
    QCOMPARE(static_cast<int>(header->frame()), volume.value("frame").toInt());
    QCOMPARE(static_cast<int>(header->gate()), volume.value("gate").toInt());
    QCOMPARE(static_cast<int>(header->bed()), volume.value("bed").toInt());
    for(int axis = 0; axis < 3; axis++)
      QCOMPARE(header->dimension(axis), dimensions[axis].toInt());
    QCOMPARE(header->dataType(), type);
    QCOMPARE(header->units(), expected.value("units").toString());
    QCOMPARE(header->frameStart(), volume.value("start").toDouble());
    QCOMPARE(header->frameDuration(), volume.value("duration").toDouble());
    double m[3][4];
    header->affine(m);
    for(int i = 0; i < 3; i++)
    {
      for(int j = 0; j < 4; j++)
        QCOMPARE(m[i][j], volume.value("affine").toArray()[i].toArray()[j].toDouble());
    }
    QCOMPARE(jsonText(header->attributes()), jsonText(volume.value("attributes").toObject()));

    // the values, x running fastest
    QCOMPARE(data->size(), dimensions[0].toInt() * dimensions[1].toInt() * dimensions[2].toInt() *
                           CMedIOVolume::bytesPerVoxel(type));
    QCOMPARE(valuesOf(*data, type), numbersOf(volume.value("values").toArray()));

    // the header alone
    CHDF5FrameHeader* only = NULL;
    QVERIFY(file.readVolumeHeader(only, k) && only != NULL);
    QScopedPointer<CHDF5FrameHeader> onlyGuard(only);
    QCOMPARE(only->volume(), k);
    QCOMPARE(jsonText(only->attributes()), jsonText(header->attributes()));
  }

  // no volumes outside of the file
  QByteArray* data = NULL;
  CHDF5FrameHeader* header = NULL;
  QVERIFY(file.readVolumeMatrix(data, header, volumes.size()) == false && data == NULL && header == NULL);
  QVERIFY(file.readVolumeMatrix(data, header, -1) == false && data == NULL && header == NULL);
  QVERIFY(file.readVolumeHeader(header, volumes.size()) == false && header == NULL);
}

// the format independent interface gives the frames of the first gate and bed position
void TestHDF5File::genericInterface()
{
  if(CHDF5File::hasSupport() == false)
    QSKIP("no HDF5 support");

  const QJsonObject expected = expectedFiles().value("gated.h5").toObject();
  const QJsonArray volumes = expected.value("volumes").toArray();
  QVERIFY(volumes.size() == 4);

  QScopedPointer<CMedIOData> file(CMedIODataFactory::create(fixture("gated.h5")));
  QVERIFY(file.isNull() == false && file->open(QIODevice::ReadOnly));
  QCOMPARE(file->frameNumbers(), QList<short>() << 1 << 3);

  // frame 3 is the third volume, the default frame the first one
  CHDF5File* hdf5 = static_cast<CHDF5File*>(file.data());
  QCOMPARE(hdf5->volumeOfFrame(3), 2);
  QCOMPARE(hdf5->volumeOfFrame(-1), 0);
  QCOMPARE(hdf5->volumeOfFrame(2), -1);

  const short frames[] = { -1, 1, 3 };
  const int volumeOf[] = { 0, 0, 2 };
  for(int k = 0; k < 3; k++)
  {
    const QJsonObject volume = volumes[volumeOf[k]].toObject();
    CMedIOVolume v;
    QVERIFY(file->readVolume(v, frames[k]));
    QCOMPARE(v.dimension(0), 4);
    QCOMPARE(v.dimension(1), 3);
    QCOMPARE(v.dimension(2), 2);
    QCOMPARE(v.dataType(), CMedIOVolume::Float32);
    QCOMPARE(v.frameStart(), volume.value("start").toDouble());
    QCOMPARE(v.frameDuration(), volume.value("duration").toDouble());
    QCOMPARE(v.scaleFactor(), 1.0);
    QCOMPARE(v.intercept(), 0.0);
    const QList<double> values = numbersOf(volume.value("values").toArray());
    for(int i = 0; i < values.size(); i++)
      QCOMPARE(v.value(i), values[i]);
    for(int i = 0; i < 3; i++)
    {
      for(int j = 0; j < 4; j++)
        QCOMPARE(v.affine(i, j), volume.value("affine").toArray()[i].toArray()[j].toDouble());
    }

    // the matrix and header of the file itself
    QByteArray* data = NULL;
    CMedIOHeader* header = NULL;
    QVERIFY(file->readMatrix(data, header, frames[k]) && data != NULL && header != NULL);
    QScopedPointer<QByteArray> dataGuard(data);
    QScopedPointer<CMedIOHeader> headerGuard(header);
    QCOMPARE(header->headerFormat(), CMedIOHeader::HDF5FrameHeader);
    QCOMPARE(static_cast<CHDF5FrameHeader*>(header)->volume(), volumeOf[k]);
    QCOMPARE(valuesOf(*data, CMedIOVolume::Float32), values);
  }
  CMedIOVolume none;
  QVERIFY(file->readVolume(none, 2) == false);

  // the main header of the file and of another format
  CMedIOHeader* mainHeader = NULL;
  QVERIFY(file->readMainHeader(mainHeader) && mainHeader != NULL);
  QScopedPointer<CMedIOHeader> mainGuard(mainHeader);
  QCOMPARE(mainHeader->headerFormat(), CMedIOHeader::HDF5MainHeader);
  QCOMPARE(mainHeader->attribute("PatientWeight").toDouble(), 72.5);
  QCOMPARE(mainHeader->attribute("(0010,1030)").toDouble(), 72.5);

  CMedIOHeader* ecatHeader = NULL;
  QVERIFY(file->readMainHeader(ecatHeader, CMedIOData::ECAT) && ecatHeader != NULL);
  QScopedPointer<CMedIOHeader> ecatGuard(ecatHeader);
  QCOMPARE(ecatHeader->headerFormat(), CMedIOHeader::ECATMainHeader);
  QCOMPARE(ecatHeader->attribute("PatientWeight").toDouble(), 72.5);
  QCOMPARE(ecatHeader->attribute("PatientName").toString(), QString::fromUtf8("M\xc3\xbcller^Hans"));

  // a frame in the representation of another format: the same physical values
  QByteArray* ecatData = NULL;
  CMedIOHeader* ecatSubHeader = NULL;
  QVERIFY(file->readMatrix(ecatData, ecatSubHeader, CMedIOData::ECAT, 3) && ecatData != NULL && ecatSubHeader != NULL);
  QScopedPointer<QByteArray> ecatDataGuard(ecatData);
  QScopedPointer<CMedIOHeader> ecatSubGuard(ecatSubHeader);
  QCOMPARE(ecatSubHeader->headerFormat(), CMedIOHeader::ECATSubHeader);
  QCOMPARE(ecatSubHeader->attribute("DecayFactor").toDouble(), 1.05);

  // the attributes of the file with those of the frames of the first gate
  QJsonObject attributes;
  QVERIFY(file->readAttributes(attributes));
  QCOMPARE(attributes.value("PatientWeight").toDouble(), 72.5);
  QCOMPARE(attributes.value("Units").toString(), QString("BQML"));
  QCOMPARE(attributes.value("DecayCorrectionDateTime").toString(), QString("2026-06-03T17:04:56+02:00"));
  const QJsonArray frameAttributes = attributes.value(CMedIOHeaderDictionary::FramesKey).toArray();
  QCOMPARE(frameAttributes.size(), 2);
  QCOMPARE(frameAttributes[0].toObject().value("DecayFactor").toDouble(), 1.01);
  QCOMPARE(frameAttributes[1].toObject().value("DecayFactor").toDouble(), 1.05);
  QCOMPARE(frameAttributes[1].toObject().value("ActualFrameDuration").toDouble(), 60250.0);
  QCOMPARE(frameAttributes[1].toObject().value("FrameReferenceTime").toDouble(), 120100.0);

  // nothing can be written
  QVERIFY(file->writeVolume(none, 1) == false);
}

// the header classes: load(), copies and conversions
void TestHDF5File::headers()
{
  if(CHDF5File::hasSupport() == false)
    QSKIP("no HDF5 support");

  CHDF5File file(fixture("gated.h5"));
  QVERIFY(file.open(QIODevice::ReadOnly));

  CHDF5MainHeader mainHeader(&file);
  QCOMPARE(mainHeader.headerFormat(), CMedIOHeader::HDF5MainHeader);
  QCOMPARE(mainHeader.numVolumes(), 0);
  QVERIFY(mainHeader.load());
  QCOMPARE(mainHeader.numVolumes(), 4);
  QCOMPARE(mainHeader.numFrames(), 2);
  QCOMPARE(mainHeader.formatVersion(), QString("0.1"));
  QCOMPARE(mainHeader.attribute("SeriesNumber").toDouble(), 4.0);
  QVERIFY(mainHeader.medIOData() == &file);
  QVERIFY(mainHeader.save() == false);

  CHDF5MainHeader copy(mainHeader);
  QCOMPARE(copy.numVolumes(), 4);
  QCOMPARE(jsonText(copy.attributes()), jsonText(mainHeader.attributes()));
  QScopedPointer<CMedIOHeader> clone(mainHeader.clone());
  QCOMPARE(clone->headerFormat(), CMedIOHeader::HDF5MainHeader);
  QCOMPARE(jsonText(clone->attributes()), jsonText(mainHeader.attributes()));
  CHDF5MainHeader converted;
  QVERIFY(converted.convertFrom(&mainHeader));
  QCOMPARE(converted.numFrames(), 2);
  copy.clear();
  QCOMPARE(copy.numVolumes(), 0);

  // frame 3 of the first gate
  CHDF5FrameHeader frameHeader(&file, 3);
  QCOMPARE(frameHeader.headerFormat(), CMedIOHeader::HDF5FrameHeader);
  QVERIFY(frameHeader.load());
  QCOMPARE(frameHeader.volume(), 2);
  QCOMPARE(static_cast<int>(frameHeader.frame()), 3);
  QCOMPARE(static_cast<int>(frameHeader.gate()), 1);
  QCOMPARE(frameHeader.dimension(0), 4);
  QCOMPARE(frameHeader.units(), QString("BQML"));
  QCOMPARE(frameHeader.frameStart(), 90.5);
  QCOMPARE(frameHeader.attribute("DecayFactor").toDouble(), 1.05);
  QVERIFY(frameHeader.save() == false);

  CHDF5FrameHeader frameCopy(frameHeader);
  QCOMPARE(frameCopy.volume(), 2);
  QCOMPARE(frameCopy.frameDuration(), 60.25);
  QVERIFY(converted.convertFrom(&frameHeader) == false);
  CHDF5FrameHeader frameConverted;
  QVERIFY(frameConverted.convertFrom(&frameHeader));
  QVERIFY(frameConverted.convertFrom(&mainHeader) == false);
  QCOMPARE(frameConverted.volume(), 2);

  // no frame 2 in the file, no file
  CHDF5FrameHeader missing(&file, 2);
  QVERIFY(missing.load() == false);
  CHDF5FrameHeader unlinked;
  QVERIFY(unlinked.load() == false);
  CHDF5MainHeader unlinkedMain;
  QVERIFY(unlinkedMain.load() == false);
}

// after close() nothing can be read; the file can be opened again
void TestHDF5File::closedFile()
{
  if(CHDF5File::hasSupport() == false)
    QSKIP("no HDF5 support");

  CHDF5File file(fixture("gated.h5"));
  for(int run = 0; run < 2; run++)
  {
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.numVolumes(), 4);
    file.close();

    QVERIFY(file.isOpen() == false);
    QCOMPARE(file.numVolumes(), 0);
    QVERIFY(file.firstGateFrames().isEmpty());
    QCOMPARE(file.volumeOfFrame(1), -1);
    CHDF5MainHeader* mainHeader = NULL;
    QVERIFY(file.readMainHeader(mainHeader) == false && mainHeader == NULL);
    QByteArray* data = NULL;
    CHDF5FrameHeader* header = NULL;
    QVERIFY(file.readMatrix(data, header, 1) == false && data == NULL && header == NULL);
    CMedIOVolume v;
    QVERIFY(file.readVolume(v, 1) == false);
  }

  // opening an open file again reads it anew; a failed open() closes the file
  QVERIFY(file.open(QIODevice::ReadOnly));
  QVERIFY(file.open(QIODevice::ReadOnly));
  QCOMPARE(file.numVolumes(), 4);
  QVERIFY(file.open(QIODevice::WriteOnly) == false);
  QVERIFY(file.isOpen() == false);
  QCOMPARE(file.numVolumes(), 0);

  // two objects of the same file at once
  CHDF5File other(fixture("gated.h5"));
  QVERIFY(file.open(QIODevice::ReadOnly) && other.open(QIODevice::ReadOnly));
  CMedIOVolume a, b;
  QVERIFY(file.readVolume(a, 3) && other.readVolume(b, 3));
  QVERIFY(a.data() == b.data());
}

QTEST_GUILESS_MAIN(TestHDF5File)
#include "testHDF5File.moc"
