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
// Files written by libmedio are read back here; the Python tests check them with
// h5py and the checker of the layout (testsuite/hdf5/tools/check_hdf5.py).
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
    void writeNewFile();
    void writeErrors();
    void writeGeneric();
    void fileNames();
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

  // a new file without content: nothing is created, an existing file is not changed
  // (without HDF5 support it cannot even be opened)
  const QString created = tmpDir.filePath("new.h5");
  CHDF5File out(created);
  QCOMPARE(out.open(QIODevice::WriteOnly), CHDF5File::hasSupport());
  QVERIFY(QFile::exists(created) == false);
  QVERIFY(out.finish() == false || CHDF5File::hasSupport() == false);
  QVERIFY(QFile::exists(created) == false);
  CHDF5File overwrite(existing);
  QCOMPARE(overwrite.open(QIODevice::WriteOnly), CHDF5File::hasSupport());
  overwrite.close();
  QFile check(existing);
  QVERIFY(check.open(QIODevice::ReadOnly));
  QCOMPARE(check.readAll(), content);
  QCOMPARE(QDir(tmpDir.path()).entryList(QDir::Files), QStringList() << "image.h5");

  // other modes
  QVERIFY(out.open(QIODevice::ReadWrite) == false);
  QVERIFY(out.open(QIODevice::Append) == false);
  QCOMPARE(QDir(tmpDir.path()).entryList(QDir::Files), QStringList() << "image.h5");
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

  // nothing can be written into a file opened for reading
  CMedIOVolume first;
  QVERIFY(file->readVolume(first, 1));
  QVERIFY(file->writeVolume(first, 1) == false);
  CHDF5MainHeader newHeader;
  QVERIFY(hdf5->writeMainHeader(newHeader) == false);
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
  QVERIFY(file.open(QIODevice::ReadWrite) == false);
  QVERIFY(file.isOpen() == false);
  QCOMPARE(file.numVolumes(), 0);

  // two objects of the same file at once
  CHDF5File other(fixture("gated.h5"));
  QVERIFY(file.open(QIODevice::ReadOnly) && other.open(QIODevice::ReadOnly));
  CMedIOVolume a, b;
  QVERIFY(file.readVolume(a, 3) && other.readVolume(b, 3));
  QVERIFY(a.data() == b.data());
}

// a volume of the test images: nx * ny * nz values counting from first
static QByteArray floatVolume(int count, float first)
{
  QByteArray data(count * 4, '\0');
  for(int i = 0; i < count; i++)
  {
    const float value = first + 0.25f * i;
    memcpy(data.data() + i * 4, &value, 4);
  }
  return data;
}

static CHDF5FrameHeader frameHeaderOf(short frame, short gate, short bed, double start, double duration, double z)
{
  const double m[3][4] = { { 2.0, 0.0, 0.0, -3.0 }, { 0.0, 2.5, 0.0, -2.5 }, { 0.0, 0.0, -3.0, z } };
  CHDF5FrameHeader header;
  header.setIndex(frame, gate, bed);
  header.setDimensions(4, 3, 2);
  header.setDataType(CMedIOVolume::Float32);
  header.setAffine(m);
  header.setTiming(start, duration);
  return header;
}

// a new file with frames, gates and attributes, read again
void TestHDF5File::writeNewFile()
{
  if(CHDF5File::hasSupport() == false)
    QSKIP("no HDF5 support");

  QTemporaryDir tmpDir;
  QVERIFY(tmpDir.isValid());
  const QString fileName = tmpDir.filePath(QString::fromUtf8("bild \xc3\xa4.h5"));
  QVERIFY(writeFile(fileName, "an existing file"));

  QJsonObject attributes;
  attributes["PatientName"] = QString::fromUtf8("M\xc3\xbcller^Hans");
  attributes["PatientWeight"] = 72.5;
  attributes["SeriesNumber"] = 4;
  attributes["AcquisitionDateTime"] = QString("2026-06-03T17:04:56+02:00");
  attributes["DecayCorrectionDateTime"] = QString("2026-06-03T17:04:50+02:00");
  attributes["Units"] = QString("BQML");
  attributes["CorrectedImage"] = QJsonArray{QString("DECY"), QString("ATTN")};
  attributes["RadionuclideCodeSequence"] = QJsonObject{{"CodeValue", "C-111A1"}, {"CodingSchemeDesignator", "SRT"},
                                                       {"CodeMeaning", "^18^Fluorine"}};
  attributes["(0029,1010)"] = QString("private");
  attributes["ECAT7"] = QJsonObject{{"Study_Type", "GATED"}};

  const short frames[] = { 1, 1, 3, 3 };
  const short gates[] = { 1, 2, 1, 2 };
  const double starts[] = { 0.0, 0.0, 90.5, 90.5 };
  const double durations[] = { 30.0, 30.0, 60.25, 60.25 };
  {
    CHDF5File file(fileName);
    QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(file.openError()));
    QVERIFY(file.isOpen() && file.isWritable());
    QCOMPARE(file.fileName(), fileName);

    // the QFile of the object is not the file written: its own methods fail
    QVERIFY(file.write("x", 1) < 0);
    char c = 0;
    QVERIFY(file.read(&c, 1) < 0);

    CHDF5MainHeader mainHeader;
    QVERIFY(mainHeader.updateAttributes(attributes));
    QVERIFY(file.writeMainHeader(mainHeader));
    QVERIFY(file.writeMainHeader(mainHeader) == false);   // once

    for(int k = 0; k < 4; k++)
    {
      CHDF5FrameHeader header = frameHeaderOf(frames[k], gates[k], 0, starts[k], durations[k], 10.0);
      QVERIFY(header.updateAttributes(QJsonObject{{"DecayFactor", 1.0 + 0.01 * frames[k]}, {"TriggerTime", 400.0 * (gates[k] - 1)},
                                                  {"ECAT7", QJsonObject{{"Annotation", QString("volume %1").arg(k)}}}}));
      // the first volume is written twice: the second matrix replaces the first one
      if(k == 0)
        QVERIFY(file.writeMatrix(floatVolume(24, -1.0f), header));
      QVERIFY(file.writeMatrix(floatVolume(24, 1000.0f * k), header));
      QCOMPARE(file.numVolumes(), k + 1);
    }

    // nothing can be read from a new file, and it has not got its name yet
    CHDF5MainHeader* none = NULL;
    QVERIFY(file.readMainHeader(none) == false && none == NULL);
    QFile existing(fileName);
    QVERIFY(existing.open(QIODevice::ReadOnly));
    QCOMPARE(existing.readAll(), QByteArray("an existing file"));
    existing.close();
    QCOMPARE(QDir(tmpDir.path()).entryList(QDir::Files).size(), 2);

    QVERIFY2(file.finish(), qPrintable(file.openError()));
    QVERIFY(file.isOpen() == false);
    QCOMPARE(file.openError(), QString());
    QVERIFY(file.finish());   // nothing to do
  }
  QCOMPARE(QDir(tmpDir.path()).entryList(QDir::Files), QStringList() << QFileInfo(fileName).fileName());
  QVERIFY(CHDF5File::isOfType(fileName));

  CHDF5File file(fileName);
  QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.openError()));
  QCOMPARE(file.numVolumes(), 4);
  QCOMPARE(file.firstGateFrames(), QList<short>() << 1 << 3);

  CHDF5MainHeader* mainHeader = NULL;
  QVERIFY(file.readMainHeader(mainHeader) && mainHeader != NULL);
  QScopedPointer<CHDF5MainHeader> mainGuard(mainHeader);
  QCOMPARE(mainHeader->formatVersion(), QString("0.1"));
  QCOMPARE(jsonText(mainHeader->attributes()), jsonText(attributes));

  for(int k = 0; k < 4; k++)
  {
    QByteArray* data = NULL;
    CHDF5FrameHeader* header = NULL;
    QVERIFY(file.readVolumeMatrix(data, header, k) && data != NULL && header != NULL);
    QScopedPointer<QByteArray> dataGuard(data);
    QScopedPointer<CHDF5FrameHeader> headerGuard(header);
    QCOMPARE(*data, floatVolume(24, 1000.0f * k));
    QCOMPARE(static_cast<int>(header->frame()), static_cast<int>(frames[k]));
    QCOMPARE(static_cast<int>(header->gate()), static_cast<int>(gates[k]));
    QCOMPARE(static_cast<int>(header->bed()), 0);
    QCOMPARE(header->dimension(0), 4);
    QCOMPARE(header->dimension(1), 3);
    QCOMPARE(header->dimension(2), 2);
    QCOMPARE(header->dataType(), CMedIOVolume::Float32);
    QCOMPARE(header->units(), QString("BQML"));
    QCOMPARE(header->frameStart(), starts[k]);
    QCOMPARE(header->frameDuration(), durations[k]);
    double m[3][4];
    header->affine(m);
    QCOMPARE(m[0][0], 2.0);
    QCOMPARE(m[1][1], 2.5);
    QCOMPARE(m[2][2], -3.0);
    QCOMPARE(m[2][3], 10.0);
    QCOMPARE(header->attribute("DecayFactor").toDouble(), 1.0 + 0.01 * frames[k]);
    QCOMPARE(header->attribute("TriggerTime").toDouble(), 400.0 * (gates[k] - 1));
    QCOMPARE(header->attribute("ActualFrameDuration").toDouble(), durations[k] * 1000.0);
    QCOMPARE(header->attributes().value("ECAT7").toObject().value("Annotation").toString(), QString("volume %1").arg(k));
  }
}

// what cannot be written is refused; a file that was not written completely does not exist
void TestHDF5File::writeErrors()
{
  if(CHDF5File::hasSupport() == false)
    QSKIP("no HDF5 support");

  QTemporaryDir tmpDir;
  QVERIFY(tmpDir.isValid());
  const QString fileName = tmpDir.filePath("image.h5");
  const QByteArray data = floatVolume(24, 0.0f);
  const CHDF5FrameHeader header = frameHeaderOf(1, 1, 0, 0.0, 60.0, 10.0);
  CHDF5MainHeader mainHeader;

  {
    // no matrix before the main header
    CHDF5File file(fileName);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.writeMatrix(data, header) == false);
    QVERIFY(file.writeMainHeader(mainHeader));

    // invalid matrices and headers
    QVERIFY(file.writeMatrix(data.left(92), header) == false);
    CHDF5FrameHeader invalid = header;
    invalid.setDataType(CMedIOVolume::Int16);
    QVERIFY(file.writeMatrix(data.left(48), invalid) == false);
    invalid = header;
    invalid.setDimensions(4, 3, 0);
    QVERIFY(file.writeMatrix(QByteArray(), invalid) == false);
    invalid = header;
    invalid.setTiming(0.0, -1.0);
    QVERIFY(file.writeMatrix(data, invalid) == false);
    invalid = header;
    invalid.setIndex(1, 0, 0);
    QVERIFY(file.writeMatrix(data, invalid) == false);
    invalid.setIndex(1, 1, -1);
    QVERIFY(file.writeMatrix(data, invalid) == false);
    invalid.setIndex(0, 1, 0);
    QVERIFY(file.writeMatrix(data, invalid, 0) == false);
    QCOMPARE(file.numVolumes(), 0);

    // all volumes have the dimensions and data type of the first one
    QVERIFY(file.writeMatrix(data, header));
    CHDF5FrameHeader other = frameHeaderOf(2, 1, 0, 60.0, 60.0, 10.0);
    other.setDimensions(2, 3, 4);
    QVERIFY(file.writeMatrix(data, other) == false);
    other.setDimensions(4, 3, 2);
    other.setDataType(CMedIOVolume::Float64);
    QVERIFY(file.writeMatrix(floatVolume(48, 0.0f), other) == false);
    QCOMPARE(file.numVolumes(), 1);

    // the refused matrices did not spoil the file
    QVERIFY(file.finish());
  }
  {
    CHDF5File file(fileName);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.numVolumes(), 1);
  }
  QVERIFY(QFile::remove(fileName));

  // a file without main header, or without a volume, is not written
  for(int run = 0; run < 2; run++)
  {
    CHDF5File file(fileName);
    QVERIFY(file.open(QIODevice::WriteOnly));
    if(run == 1)
      QVERIFY(file.writeMainHeader(mainHeader));
    QVERIFY(file.finish() == false);
    QVERIFY(file.openError().isEmpty() == false);
    QVERIFY(file.errorStatus() != CMedIOData::MERR_Ok);
    QVERIFY(QDir(tmpDir.path()).entryList(QDir::Files).isEmpty());
  }

  // a file that is not closed is discarded with its object
  {
    CHDF5File file(fileName);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.writeMainHeader(mainHeader));
  }
  QVERIFY(QDir(tmpDir.path()).entryList(QDir::Files).isEmpty());

  // no directory to write into
  CHDF5File nowhere(tmpDir.filePath("missing/image.h5"));
  QVERIFY(nowhere.open(QIODevice::WriteOnly) == false);
  QVERIFY(nowhere.isOpen() == false);
  QVERIFY(nowhere.openError().isEmpty() == false);

  // opening for writing again completes the file, as close() does, and starts a new one;
  // the new one is not written without content and leaves the first one as it is
  CHDF5File again(fileName);
  QVERIFY(again.open(QIODevice::WriteOnly));
  QVERIFY(again.writeMainHeader(mainHeader) && again.writeMatrix(data, header));
  QVERIFY(again.open(QIODevice::WriteOnly));
  QCOMPARE(again.numVolumes(), 0);
  QVERIFY(QFile::exists(fileName));
  QCOMPARE(QDir(tmpDir.path()).entryList(QDir::Files).size(), 2);   // and the temporary file
  again.close();
  QCOMPARE(QDir(tmpDir.path()).entryList(QDir::Files), QStringList() << "image.h5");
  CHDF5File first(fileName);
  QVERIFY(first.open(QIODevice::ReadOnly));
  QCOMPARE(first.numVolumes(), 1);
}

// a new file written with the format independent interface: integers with scale factors
// per plane become the physical values
void TestHDF5File::writeGeneric()
{
  if(CHDF5File::hasSupport() == false)
    QSKIP("no HDF5 support");

  QTemporaryDir tmpDir;
  QVERIFY(tmpDir.isValid());
  const QString fileName = tmpDir.filePath("image.hdf5");

  const int nx = 4, ny = 3, nz = 2;
  QByteArray stored(nx * ny * nz * 2, '\0');
  for(int i = 0; i < nx * ny * nz; i++)
  {
    const qint16 value = static_cast<qint16>(i * 7 - 50);
    memcpy(stored.data() + i * 2, &value, 2);
  }
  const double m[3][4] = { { 2.0, 0.0, 0.0, -3.0 }, { 0.0, 2.5, 0.0, -2.5 }, { 0.0, 0.0, -3.0, 10.0 } };
  CMedIOVolume volumes[2];
  for(int k = 0; k < 2; k++)
  {
    QVERIFY(volumes[k].setData(nx, ny, nz, CMedIOVolume::Int16, stored));
    volumes[k].setScaleFactor(0, 0.5 * (k + 1));
    volumes[k].setScaleFactor(1, 0.25);
    volumes[k].setAffine(m);
    volumes[k].setFrameStart(60.0 * k);
    volumes[k].setFrameDuration(60.0);
  }

  {
    QScopedPointer<CMedIOData> out(CMedIODataFactory::createForWriting(fileName));
    QVERIFY(out.isNull() == false);
    QCOMPARE(out->dataFormat(), CMedIOData::HDF5);
    QCOMPARE(out->fileNames(), QStringList() << fileName);
    CMedIOHeader* header = NULL;
    QVERIFY(CMedIOData::mainHeaderFromVolume(volumes[0], CMedIOData::HDF5, 2, header) && header != NULL);
    QScopedPointer<CMedIOHeader> headerGuard(header);
    QCOMPARE(header->headerFormat(), CMedIOHeader::HDF5MainHeader);
    QCOMPARE(CMedIOData::requiredFrames(*header), 0);
    QVERIFY(header->updateAttributes(QJsonObject{{"PatientWeight", 72.5}, {"Units", "CNTS"}}));

    QVERIFY(out->open(QIODevice::WriteOnly));
    QVERIFY(out->writeVolume(volumes[0], 1) == false);   // no main header yet
    QVERIFY(out->writeMainHeader(*header));
    QVERIFY(out->writeVolume(volumes[0], 1));
    QVERIFY(out->writeVolume(volumes[1], 2));
    out->close();
    QCOMPARE(out->errorStatus(), static_cast<int>(CMedIOData::MERR_Ok));
  }

  QScopedPointer<CMedIOData> in(CMedIODataFactory::create(fileName));
  QVERIFY(in.isNull() == false && in->dataFormat() == CMedIOData::HDF5 && in->open(QIODevice::ReadOnly));
  QCOMPARE(in->frameNumbers(), QList<short>() << 1 << 2);
  for(int k = 0; k < 2; k++)
  {
    CMedIOVolume v;
    QVERIFY(in->readVolume(v, static_cast<short>(k + 1)));
    QCOMPARE(v.dataType(), CMedIOVolume::Float32);
    QCOMPARE(v.scaleFactor(), 1.0);
    QVERIFY(v.hasSameGeometry(volumes[k]));
    QCOMPARE(v.frameStart(), 60.0 * k);
    QCOMPARE(v.frameDuration(), 60.0);
    for(int i = 0; i < nx * ny * nz; i++)
      QCOMPARE(v.value(i), volumes[k].value(i));
  }
  QJsonObject attributes;
  QVERIFY(in->readAttributes(attributes));
  QCOMPARE(attributes.value("PatientWeight").toDouble(), 72.5);
  QCOMPARE(attributes.value("Units").toString(), QString("CNTS"));
  in->close();
  QVERIFY(in->removeFiles());
  QVERIFY(QDir(tmpDir.path()).entryList(QDir::Files).isEmpty());
}

// the names of HDF5 files
void TestHDF5File::fileNames()
{
  foreach(const QString& name, QStringList() << "image.h5" << "image.hdf5" << "image.hdf" << "IMAGE.H5" << "/a/b.c/image.Hdf5")
    QCOMPARE(CMedIODataFactory::formatOfFileName(name), CMedIOData::HDF5);
  foreach(const QString& name, QStringList() << "image.h5.gz" << "image.h" << "h5" << "image.hdf5.v")
    QVERIFY2(CMedIODataFactory::formatOfFileName(name) != CMedIOData::HDF5, qPrintable(name));

  QScopedPointer<CMedIOData> file(CMedIODataFactory::createForWriting("image.h5"));
  QVERIFY(file.isNull() == false && file->dataFormat() == CMedIOData::HDF5);
  QScopedPointer<CMedIOData> explicitFormat(CMedIODataFactory::createForWriting("image.dat", CMedIOData::HDF5));
  QVERIFY(explicitFormat.isNull() == false && explicitFormat->dataFormat() == CMedIOData::HDF5);
}

QTEST_GUILESS_MAIN(TestHDF5File)
#include "testHDF5File.moc"
