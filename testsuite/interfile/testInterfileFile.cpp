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

// Reading Interfile files (CInterfileFile) with the Interfile test datasets. The
// expected values come from independent sources: the own parser of
// testsuite/data/tools/make_expected.py, the ECAT7 file of the same dataset (PMOD
// converted it into Interfile), the DICOM series of the same reconstruction
// (Siemens) and the landmarks of testsuite/data/datasets.json.

#include <QtTest/QtTest>
#include <QJsonArray>
#include <QJsonObject>
#include <QScopedPointer>
#include <QTemporaryDir>

#include <cmath>

#include "CMedIOData.h"
#include "CMedIODataFactory.h"
#include "CMedIOHeader.h"
#include "CMedIOVolume.h"
#include "CInterfileFile.h"
#include "CInterfileFrameHeader.h"
#include "CInterfileKeys.h"
#include "CInterfileMainHeader.h"

#include "MedIOTestData.h"

Q_DECLARE_METATYPE(MedIOTestFile)

namespace {

// the file to open: the file itself, the main header (.mhdr) or the header (.hdr, STIR:
// .hv) of a directory
QString fileToOpen(const MedIOTestFile& file)
{
  if(QFileInfo(file.path).isDir() == false)
    return file.path;

  const QDir dir(file.path);
  QStringList names = dir.entryList(QStringList() << "*.mhdr", QDir::Files);
  if(names.isEmpty())
    names = dir.entryList(QStringList() << "*.hdr" << "*.hv", QDir::Files);
  return names.size() == 1 ? dir.filePath(names.first()) : QString();
}

// the test file of another format of the same dataset (NULL if there is none)
const MedIOTestFile* sameDataset(const MedIOTestData& data, const QString& relativePath)
{
  foreach(const MedIOTestFile& file, data.files)
  {
    if(file.relativePath == relativePath)
      return &file;
  }
  return NULL;
}

// a file opened for reading by the factory
CMedIOData* openFile(const QString& fileName)
{
  CMedIOData* file = CMedIODataFactory::create(fileName);
  if(file != NULL && file->open(QIODevice::ReadOnly) == false)
  {
    delete file;
    file = NULL;
  }
  return file;
}

} // namespace

class TestInterfileFile : public QObject
{
  Q_OBJECT

  private:
    MedIOTestData m_data;
    QTemporaryDir m_tmp;

  private slots:
    void initTestCase();

    void datasets_data();
    void datasets();
    void identify_data();
    void identify();
    void sameAsSource_data();
    void sameAsSource();
    void landmarks_data();
    void landmarks();
    void attributes_data();
    void attributes();
    void toNifti_data();
    void toNifti();

    void openModes();
    void siemensPlaneOrder_data();
    void siemensPlaneOrder();
    void write_data();
    void write();
    void writePatientPositions_data();
    void writePatientPositions();
    void writeErrors();
    void notInterfile();
};

void TestInterfileFile::initTestCase()
{
  m_data = medioTestData();
  QVERIFY(m_tmp.isValid());
}

static void addInterfileRows(const MedIOTestData& data)
{
  QTest::addColumn<MedIOTestFile>("file");

  bool any = false;
  foreach(const MedIOTestFile& file, data.files)
  {
    if(file.format != "interfile")
      continue;
    QTest::newRow(qPrintable(file.relativePath)) << file;
    any = true;
  }

  if(any == false)
    QTest::newRow("no files") << MedIOTestFile();
}

//-----------------------------------------------------------------------------------------
// the frames of the datasets with the expected values of make_expected.py

void TestInterfileFile::datasets_data()
{
  addInterfileRows(m_data);
}

void TestInterfileFile::datasets()
{
  QFETCH(MedIOTestFile, file);
  if(file.path.isEmpty())
    QSKIP("no Interfile test datasets");

  const QString name = fileToOpen(file);
  QCOMPARE(CMedIODataFactory::identify(name), CMedIOData::Interfile);
  QScopedPointer<CMedIOData> data(openFile(name));
  QVERIFY(data.isNull() == false);
  QCOMPARE(data->dataFormat(), CMedIOData::Interfile);

  const QJsonArray frames = file.expected.value("frames").toArray();
  const QList<short> numbers = data->frameNumbers();
  QCOMPARE(numbers.size(), frames.size());

  CMedIOHeader* header = NULL;
  QVERIFY(data->readMainHeader(header));
  QScopedPointer<CMedIOHeader> mainHeader(header);
  QCOMPARE(mainHeader->headerFormat(), CMedIOHeader::InterfileMainHeader);
  QCOMPARE(static_cast<CInterfileMainHeader*>(header)->numFrames(), frames.size());

  for(int f = 0; f < frames.size(); f++)
  {
    const QJsonObject expected = frames.at(f).toObject();
    QCOMPARE(numbers.at(f), short(f + 1));

    CMedIOVolume v;
    QVERIFY(data->readVolume(v, numbers.at(f)));
    const QJsonArray dims = expected.value("dims").toArray();
    for(int i = 0; i < 3; i++)
    {
      QCOMPARE(v.dimension(i), dims.at(i).toInt());
      QVERIFY(std::fabs(v.voxelSize(i) - expected.value("pixel_size_mm").toArray().at(i).toDouble()) < 1e-6);
    }

    // stored values in the order of the file and their scaling
    double minimum = HUGE_VAL;
    double maximum = -HUGE_VAL;
    double sum = 0.0;
    for(qint64 i = 0; i < v.voxelCount(); i++)
    {
      const double value = v.storedValue(i);
      minimum = qMin(minimum, value);
      maximum = qMax(maximum, value);
      sum += value;
    }
    QCOMPARE(minimum, expected.value("raw_min").toDouble());
    QCOMPARE(maximum, expected.value("raw_max").toDouble());
    QCOMPARE(sum, expected.value("raw_sum").toDouble());

    QCOMPARE(v.scaleFactor(), expected.value("rescale_slope").toDouble());
    QCOMPARE(v.intercept() + 0.0, expected.value("rescale_offset").toDouble() + 0.0);

    QCOMPARE(v.frameDuration(), expected.value("duration_s").toDouble());
    if(expected.contains("start_s"))
      QCOMPARE(v.frameStart(), expected.value("start_s").toDouble());

    // the native matrix and its description
    QByteArray* matrix = NULL;
    CMedIOHeader* subHeader = NULL;
    QVERIFY(data->readMatrix(matrix, subHeader, numbers.at(f)));
    QScopedPointer<QByteArray> matrixData(matrix);
    QScopedPointer<CMedIOHeader> frameHeader(subHeader);
    QCOMPARE(subHeader->headerFormat(), CMedIOHeader::InterfileFrameHeader);
    QVERIFY(*matrix == v.data());
    QCOMPARE(static_cast<CInterfileFrameHeader*>(subHeader)->frame(), numbers.at(f));
  }
}

//-----------------------------------------------------------------------------------------
// the factory identifies every file of a dataset: headers, main header and data files

void TestInterfileFile::identify_data()
{
  QTest::addColumn<QString>("path");
  QTest::addColumn<int>("frames");

  bool any = false;
  foreach(const MedIOTestFile& file, m_data.files)
  {
    if(file.format != "interfile")
      continue;
    // the image header and data file of a data set of a Siemens main header are one
    // frame of it (one image header per frame)
    const int frames = file.expected.value("frames").toArray().size();
    const int dataSets = file.expected.value("header").toObject().value("data_sets").toArray().size();
    QStringList paths;
    if(QFileInfo(file.path).isDir())
    {
      foreach(const QString& name, QDir(file.path).entryList(QDir::Files))
        paths << QDir(file.path).filePath(name);
    }
    else
      paths << file.path;
    foreach(const QString& path, paths)
    {
      const int pathFrames = (dataSets > 0 && path.endsWith(".mhdr") == false) ? frames / dataSets : frames;
      QTest::newRow(qPrintable(QDir(medioTestDataDir()).relativeFilePath(path))) << path << pathFrames;
      any = true;
    }
  }

  if(any == false)
    QTest::newRow("no files") << QString() << 0;
}

void TestInterfileFile::identify()
{
  QFETCH(QString, path);
  QFETCH(int, frames);
  if(path.isEmpty())
    QSKIP("no Interfile test datasets");

  QCOMPARE(CMedIODataFactory::identify(path), CMedIOData::Interfile);
  QVERIFY(CInterfileFile::isOfType(path));

  CInterfileFile file(path);
  QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.openError()));
  // an image header of a Siemens data set is opened on its own: its frames
  QCOMPARE(file.numFrames(), frames);
  QVERIFY(file.fileNames().contains(path));
  foreach(const QString& name, file.allFileNames())
    QVERIFY2(QFileInfo(name).isFile(), qPrintable(name));
}

//-----------------------------------------------------------------------------------------
// the files converted from a file of another format (datasets.json "source"): the same
// physical values at the same positions. PMOD converted the ECAT7 files (identical stored
// values; ECAT7 stores no absolute position), STIR the NIfTI file (physical values as
// float32, the positions of the NIfTI file)

void TestInterfileFile::sameAsSource_data()
{
  addInterfileRows(m_data);
}

void TestInterfileFile::sameAsSource()
{
  QFETCH(MedIOTestFile, file);
  if(file.path.isEmpty())
    QSKIP("no Interfile test datasets");

  const QString sourcePath = file.fileInfo.value("source").toString().section(' ', 0, 0);
  const MedIOTestFile* source = sameDataset(m_data, sourcePath);
  if(source == NULL)
    QSKIP("not converted from a test file of another format");
  const bool ecat = (source->format == "ecat");

  QScopedPointer<CMedIOData> interfile(openFile(fileToOpen(file)));
  QScopedPointer<CMedIOData> reference(openFile(QFileInfo(source->path).isDir() ? fileToOpen(*source) : source->path));
  QVERIFY(interfile.isNull() == false && reference.isNull() == false);

  const QList<short> numbers = interfile->frameNumbers();
  QVERIFY(numbers.size() <= reference->frameNumbers().size());

  // PMOD wrote pet-dynamic in kBq/cc, the ECAT7 file (calibration factor 1000) gives Bq/cc
  QJsonObject attributes;
  QJsonObject referenceAttributes;
  QVERIFY(interfile->readAttributes(attributes) && reference->readAttributes(referenceAttributes));
  const double factor = (attributes.value("Units").toString() == "KBQCC" &&
                         referenceAttributes.value("Units").toString() == "BQML") ? 1000.0 : 1.0;

  for(int f = 0; f < numbers.size(); f++)
  {
    CMedIOVolume v;
    CMedIOVolume e;
    QVERIFY(interfile->readVolume(v, numbers.at(f)));
    QVERIFY(reference->readVolume(e, reference->frameNumbers().at(f)));

    QVERIFY(v.reorient(e.orientation()));
    for(int i = 0; i < 3; i++)
    {
      QCOMPARE(v.dimension(i), e.dimension(i));
      QVERIFY(std::fabs(v.voxelSize(i) - e.voxelSize(i)) < 1e-4);
    }
    if(ecat)
    {
      QCOMPARE(v.frameStart(), e.frameStart());
      QCOMPARE(v.frameDuration(), e.frameDuration());
    }
    else
      QVERIFY2(v.hasSameGeometry(e), "the positions of the voxels differ from the source");

    double maxDifference = 0.0;
    double maxValue = 0.0;
    for(qint64 i = 0; i < v.voxelCount(); i++)
    {
      if(ecat)
        QCOMPARE(v.storedValue(i), e.storedValue(i));
      maxDifference = qMax(maxDifference, std::fabs(v.value(i) * factor - e.value(i)));
      maxValue = qMax(maxValue, std::fabs(e.value(i)));
    }
    QVERIFY2(maxDifference <= 1e-6 * maxValue, qPrintable(QString("max. difference %1").arg(maxDifference)));
  }
}

//-----------------------------------------------------------------------------------------
// positions of the landmarks of datasets.json: absolute (Siemens, compared with the
// DICOM series), otherwise their distances (the files of PMOD store no position)

void TestInterfileFile::landmarks_data()
{
  addInterfileRows(m_data);
}

void TestInterfileFile::landmarks()
{
  QFETCH(MedIOTestFile, file);
  if(file.path.isEmpty())
    QSKIP("no Interfile test datasets");

  QList<QVector<double> > voxels;
  QList<QVector<double> > worlds;
  foreach(const QJsonValue& value, file.datasetInfo.value("landmarks").toArray())
  {
    const QJsonObject landmark = value.toObject();
    const QJsonArray voxel = landmark.value("voxel").toObject().value(file.relativePath).toArray();
    const QJsonArray world = landmark.value("world_mm").toArray();
    if(voxel.size() != 3 || world.size() != 3)
      continue;
    voxels << (QVector<double>() << voxel.at(0).toDouble() << voxel.at(1).toDouble() << voxel.at(2).toDouble());
    worlds << (QVector<double>() << world.at(0).toDouble() << world.at(1).toDouble() << world.at(2).toDouble());
  }
  if(voxels.size() < 2)
    QSKIP("no landmarks of the file");

  QScopedPointer<CMedIOData> data(openFile(fileToOpen(file)));
  QVERIFY(data.isNull() == false);
  CMedIOVolume v;
  QVERIFY(data->readVolume(v, -1));

  // the first voxel of STIR files: the position given by the expected values
  const QJsonObject expected = file.expected.value("frames").toArray().at(0).toObject();
  if(expected.contains("first_voxel_lps_mm"))
  {
    double p[3];
    v.position(0, 0, 0, p);
    for(int i = 0; i < 3; i++)
      QVERIFY(std::fabs(p[i] - expected.value("first_voxel_lps_mm").toArray().at(i).toDouble()) < 1e-3);
  }

  const bool absolute = file.fileInfo.value("origin").toString() == "original";
  for(int a = 0; a < voxels.size(); a++)
  {
    double pa[3];
    v.position(voxels[a][0], voxels[a][1], voxels[a][2], pa);
    for(int i = 0; absolute && i < 3; i++)
      QVERIFY2(std::fabs(pa[i] - worlds[a][i]) < 0.01,
               qPrintable(QString("landmark %1 axis %2: %3 mm, expected %4 mm").arg(a).arg(i).arg(pa[i]).arg(worlds[a][i])));

    for(int b = a + 1; b < voxels.size(); b++)
    {
      double pb[3];
      v.position(voxels[b][0], voxels[b][1], voxels[b][2], pb);
      for(int i = 0; i < 3; i++)
        QVERIFY2(std::fabs((pb[i] - pa[i]) - (worlds[b][i] - worlds[a][i])) < 0.1,
                 qPrintable(QString("distance of landmarks %1 and %2, axis %3").arg(a).arg(b).arg(i)));
    }
  }
}

//-----------------------------------------------------------------------------------------
// attributes compared with the ECAT7 file (PMOD) or the DICOM series (Siemens) of the
// same dataset

void TestInterfileFile::attributes_data()
{
  addInterfileRows(m_data);
}

void TestInterfileFile::attributes()
{
  QFETCH(MedIOTestFile, file);
  if(file.path.isEmpty())
    QSKIP("no Interfile test datasets");

  QScopedPointer<CMedIOData> data(openFile(fileToOpen(file)));
  QVERIFY(data.isNull() == false);
  QJsonObject attributes;
  QVERIFY(data->readAttributes(attributes));
  QVERIFY(attributes.value("Interfile").toObject().isEmpty() == false);
  if(file.fileInfo.value("source").toString().startsWith("nifti/"))
    QSKIP("STIR writes no attributes of the NIfTI file");

  // the keys the reference stores the same way
  QStringList keys = QStringList() << "Radiopharmaceutical" << "RadionuclideHalfLife" << "RadionuclideTotalDose"
                                   << "EnergyWindowLowerLimit" << "EnergyWindowUpperLimit" << "Units";
  QJsonObject reference;
  QJsonArray referenceFrames;
  if(const MedIOTestFile* ecat = sameDataset(m_data, QString("ecat/%1.v").arg(file.dataset)))
  {
    reference = ecat->expected.value("attributes").toObject().value("main").toObject();
    referenceFrames = ecat->expected.value("attributes").toObject().value("frames").toArray();
    keys << "PatientName" << "PatientID" << "PatientBirthDate" << "PatientSize" << "PatientWeight"
         << "AcquisitionDateTime" << "RadiopharmaceuticalStartDateTime";
  }
  else if(const MedIOTestFile* dicom = sameDataset(m_data, QString("dicom/%1").arg(file.dataset)))
  {
    reference = dicom->expected.value("attributes").toObject().value("main").toObject();
    referenceFrames = dicom->expected.value("attributes").toObject().value("frames").toArray();
    keys << "Manufacturer" << "ManufacturerModelName" << "PatientPosition" << "ReconstructionMethod"
         << "RadionuclidePositronFraction";

    // the Siemens headers give the times in UTC, the DICOM series in the local time of the
    // scanner: the acquisition gives the offset of the local time (in steps of 15 minutes),
    // the injection has to have the same
    const auto msecs = [](const QJsonObject& json, const char* key, bool utc) -> qint64 {
      QDateTime time = QDateTime::fromString(json.value(key).toString(), Qt::ISODateWithMs);
      if(utc == false)
        time = QDateTime(time.date(), time.time(), Qt::UTC);
      return time.isValid() ? time.toMSecsSinceEpoch() : 0;
    };
    const qint64 quarter = 15 * 60 * 1000;
    const qint64 offset = qRound64(double(msecs(reference, "AcquisitionDateTime", false) -
                                          msecs(attributes, "AcquisitionDateTime", true)) / quarter) * quarter;
    QVERIFY(msecs(attributes, "AcquisitionDateTime", true) != 0 && qAbs(offset) <= 14 * 3600 * 1000);
    QVERIFY(attributes.value("RadiopharmaceuticalStartDateTime").toString().endsWith('Z'));
    QCOMPARE(msecs(reference, "RadiopharmaceuticalStartDateTime", false) -
             msecs(attributes, "RadiopharmaceuticalStartDateTime", true), offset);
  }
  else
    QSKIP("no reference of the dataset");

  // the units as given (PMOD: kBq/cc for pet-dynamic, whose ECAT7 file gives Bq/cc)
  if(attributes.value("Units").toString() == "KBQCC")
  {
    QCOMPARE(reference.value("Units").toString(), QString("BQML"));
    keys.removeAll("Units");
  }

  int compared = 0;
  foreach(const QString& key, keys)
  {
    if(reference.contains(key) == false)
      continue;
    QVERIFY2(attributes.value(key) == reference.value(key),
             qPrintable(QString("%1: %2 instead of %3").arg(key, QString(QJsonDocument(QJsonObject{{"v", attributes.value(key)}}).toJson(QJsonDocument::Compact)),
                                                      QString(QJsonDocument(QJsonObject{{"v", reference.value(key)}}).toJson(QJsonDocument::Compact)))));
    compared++;
  }
  QVERIFY(compared >= 5);

  // the frames
  const QJsonArray frames = attributes.value("Frames").toArray();
  QCOMPARE(frames.size(), data->frameNumbers().size());
  for(int f = 0; f < referenceFrames.size() && f < frames.size(); f++)
    QCOMPARE(frames.at(f).toObject().value("ActualFrameDuration").toDouble(),
             referenceFrames.at(f).toObject().value("ActualFrameDuration").toDouble());
}

//-----------------------------------------------------------------------------------------
// conversion into NIfTI with the format independent API: the same values at the same positions

void TestInterfileFile::toNifti_data()
{
  addInterfileRows(m_data);
}

void TestInterfileFile::toNifti()
{
  QFETCH(MedIOTestFile, file);
  if(file.path.isEmpty())
    QSKIP("no Interfile test datasets");

  QScopedPointer<CMedIOData> source(openFile(fileToOpen(file)));
  QVERIFY(source.isNull() == false);

  const QString target = m_tmp.filePath(file.dataset + "-" + QFileInfo(file.path).fileName() + ".nii");
  QScopedPointer<CMedIOData> nifti(CMedIODataFactory::createForWriting(target, CMedIOData::NIFTI));
  QVERIFY(nifti.isNull() == false);

  CMedIOHeader* header = NULL;
  QVERIFY(source->readMainHeader(header, CMedIOData::NIFTI));
  QScopedPointer<CMedIOHeader> mainHeader(header);
  QVERIFY(nifti->open(QIODevice::WriteOnly));
  QVERIFY(nifti->writeMainHeader(*mainHeader));
  const QList<short> numbers = source->frameNumbers();
  foreach(short frame, numbers)
  {
    CMedIOVolume v;
    QVERIFY(source->readVolume(v, frame));
    QVERIFY(v.convertToPhysicalValues());
    QVERIFY(nifti->writeVolume(v, frame));
  }
  nifti->close();

  QScopedPointer<CMedIOData> written(openFile(target));
  QVERIFY(written.isNull() == false);
  QCOMPARE(written->frameNumbers().size(), numbers.size());
  for(int f = 0; f < numbers.size(); f++)
  {
    CMedIOVolume v;
    CMedIOVolume n;
    QVERIFY(source->readVolume(v, numbers.at(f)));
    QVERIFY(written->readVolume(n, written->frameNumbers().at(f)));
    QVERIFY(n.reorient(v.orientation()));
    QVERIFY(v.hasSameGeometry(n));
    for(qint64 i = 0; i < v.voxelCount(); i += 97)
      QVERIFY(std::fabs(v.value(i) - n.value(i)) <= 1e-5 * std::fabs(v.value(i)) + 1e-6);
    QVERIFY(std::fabs(n.frameStart() - v.frameStart()) < 1e-3);
    QVERIFY(std::fabs(n.frameDuration() - v.frameDuration()) < 1e-3);
  }

  // the attributes go with the image
  QJsonObject sourceAttributes;
  QJsonObject writtenAttributes;
  QVERIFY(source->readAttributes(sourceAttributes));
  QVERIFY(written->readAttributes(writtenAttributes));
  foreach(const QString& key, QStringList() << "Radiopharmaceutical" << "RadionuclideHalfLife" << "Units")
    QCOMPARE(writtenAttributes.value(key), sourceAttributes.value(key));
}

//-----------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
// Siemens: the planes of the data file are from the highest to the lowest z, "%image
// info[n]" lists them in the order of the scan; without "%image info" the bed positions
// give z. The values of the samples of docs/interfile.md: continuous bed motion towards
// higher z (scan direction "out"), step and shoot towards lower z ("in").

void TestInterfileFile::siemensPlaneOrder_data()
{
  QTest::addColumn<bool>("imageInfo");
  QTest::addColumn<double>("start");
  QTest::addColumn<double>("end");
  QTest::addColumn<double>("firstZ");

  // 4 planes of 1.64557 mm
  QTest::newRow("image info, z rising with n") << true << -718.542 << 239.003 << -718.542 + 3 * 1.64557;
  QTest::newRow("image info, z falling with n") << true << -766.002 << -1026.0 << -766.002;
  QTest::newRow("bed positions, rising") << false << -718.542 << 239.003 << -718.542 + 3 * 1.64557;
  QTest::newRow("bed positions, falling") << false << -766.002 << -1026.0 << -766.002;
}

void TestInterfileFile::siemensPlaneOrder()
{
  QFETCH(bool, imageInfo);
  QFETCH(double, start);
  QFETCH(double, end);
  QFETCH(double, firstZ);

  const int planes = 4;
  const double dz = 1.64557;
  const QString name = m_tmp.filePath(QString("order-%1.v.hdr").arg(QTest::currentDataTag()).replace(' ', '_').replace(',', ""));
  QByteArray text = "!INTERFILE:=\r\n%SMS-MI header name space:=image subheader\r\nimage data byte order:=LITTLEENDIAN\r\n"
                    "number format:=float\r\n!number of bytes per pixel:=4\r\nnumber of dimensions:=3\r\n"
                    "matrix size[1]:=2\r\nmatrix size[2]:=3\r\nmatrix size[3]:=4\r\n"
                    "scale factor (mm/pixel) [1]:=22.6875\r\nscale factor (mm/pixel) [2]:=22.6875\r\n"
                    "scale factor (mm/pixel) [3]:=1.64557\r\n%gantry offset (mm) [1]:=-0.858\r\n"
                    "%gantry offset (mm) [2]:=0.255\r\nstart vertical bed position (mm):=178\r\n";
  text += QString("start horizontal bed position (mm):=%1\r\nend horizontal bed position (mm):=%2\r\n")
              .arg(start, 0, 'g', 10).arg(end, 0, 'g', 10).toLatin1();
  text += "!name of data file:=" + QFileInfo(name).completeBaseName().toLatin1() + "\r\n%image orientation:={1,0,0,0,1,0}\r\n";
  // the entries in the order of the scan: from the start position towards the end
  const double step = (end > start) ? dz : -dz;
  for(int n = 1; imageInfo && n <= planes; n++)
    text += QString("%image info[%1]:={%2,%3,-0.858,-541.255,0,0,0,1,0.0,1,0,0.0,60,1,no slice}\r\n")
                .arg(n).arg(n - 1).arg(start + (n - 1) * step, 0, 'g', 10).toLatin1();
  QFile header(name);
  QVERIFY(header.open(QIODevice::WriteOnly));
  header.write(text);
  header.close();
  QFile data(QFileInfo(name).dir().filePath(QFileInfo(name).completeBaseName()));
  QVERIFY(data.open(QIODevice::WriteOnly));
  data.write(QByteArray(2 * 3 * planes * 4, '\0'));
  data.close();

  QScopedPointer<CMedIOData> file(openFile(name));
  QVERIFY(file.isNull() == false);
  CMedIOVolume v;
  QVERIFY(file->readVolume(v, 1));
  QCOMPARE(v.orientation(), QString("LPI"));
  double p[3];
  v.position(0, 0, 0, p);
  QVERIFY2(std::fabs(p[2] - firstZ) < 1e-3, qPrintable(QString("z of the first plane %1").arg(p[2])));
  v.position(0, 0, planes - 1, p);
  QVERIFY(std::fabs(p[2] - (firstZ - (planes - 1) * dz)) < 1e-3);
  if(imageInfo)
    QVERIFY(std::fabs(p[0] + 0.858) < 1e-6 && std::fabs(p[1] + 541.255) < 1e-6);
  else
  {
    // x: -nx / 2 * dx - gantry offset x, y: -ny / 2 * dy - vertical bed - gantry offset y
    QVERIFY(std::fabs(p[0] - (-1.0 * 22.6875 + 0.858)) < 1e-6);
    QVERIFY(std::fabs(p[1] - (-1.5 * 22.6875 - 178.0 - 0.255)) < 1e-6);
  }
}

void TestInterfileFile::openModes()
{
  QCOMPARE(CMedIODataFactory::formatName(CMedIOData::Interfile), "interfile");
  CMedIOData::Format format = CMedIOData::Unknown;
  QVERIFY(CMedIODataFactory::formatFromName("Interfile", format));
  QCOMPARE(format, CMedIOData::Interfile);
  QCOMPARE(CMedIODataFactory::formatOfFileName("image.HV"), CMedIOData::Interfile);

  const QString header = m_tmp.filePath("image.hv");
  QFile file(header);
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write("!INTERFILE :=\n!name of data file := image.v\n!matrix size [1] := 2\n!matrix size [2] := 2\n"
             "!matrix size [3] := 1\n!number format := float\n!number of bytes per pixel := 4\n"
             "!END OF INTERFILE :=\n");
  file.close();
  QFile dataFile(m_tmp.filePath("image.v"));
  QVERIFY(dataFile.open(QIODevice::WriteOnly));
  dataFile.write(QByteArray(16, '\0'));
  dataFile.close();

  // existing files are read, or replaced as new files: they cannot be changed
  CInterfileFile interfile(header);
  QVERIFY(interfile.open(QIODevice::ReadWrite) == false);
  QVERIFY(interfile.open(QIODevice::Append) == false);
  QVERIFY2(interfile.open(QIODevice::ReadOnly), qPrintable(interfile.openError()));
  QCOMPARE(interfile.numFrames(), 1);
  QCOMPARE(interfile.fileNames(), QStringList() << header << m_tmp.filePath("image.v"));
  interfile.close();

  // the data file is found from the data file name, big endian by default
  QCOMPARE(CMedIODataFactory::identify(m_tmp.filePath("image.v")), CMedIOData::Interfile);
  CInterfileFile byData(m_tmp.filePath("image.v"));
  QVERIFY(byData.open(QIODevice::ReadOnly));
  QCOMPARE(byData.headerFileName(), header);

  // a header read from a file cannot be written into it
  CInterfileMainHeader* mainHeader = NULL;
  QVERIFY(byData.readMainHeader(mainHeader));
  QVERIFY(mainHeader->save() == false);
  QVERIFY(byData.writeMainHeader(*mainHeader) == false);
  delete mainHeader;

  // the names of new files: the data file of STIR
  QCOMPARE(CInterfileFile::dataFileNameOf("/a/image.hv"), QString("/a/image.v"));
  QCOMPARE(CInterfileFile::dataFileNameOf("/a/image.stir.hv"), QString("/a/image.stir.v"));
  QCOMPARE(CInterfileFile::dataFileNameOf("/a/image"), QString("/a/image.v"));
  QCOMPARE(CInterfileFile::dataFileNameOf("/a/image.v"), QString("/a/image.v.v"));
  QScopedPointer<CMedIOData> created(CMedIODataFactory::createForWriting(m_tmp.filePath("new.hv")));
  QVERIFY(created.isNull() == false);
  QCOMPARE(created->dataFormat(), CMedIOData::Interfile);
  QCOMPARE(created->fileNames(), QStringList() << m_tmp.filePath("new.hv") << m_tmp.filePath("new.v"));
}

//-----------------------------------------------------------------------------------------
// writing: every image test file written as Interfile and read again

void TestInterfileFile::write_data()
{
  QTest::addColumn<MedIOTestFile>("file");

  bool any = false;
  foreach(const MedIOTestFile& file, m_data.files)
  {
    // one file per format and dataset (the other files hold the same images)
    if(file.format == "dicom" || file.format == "raw" || file.relativePath.endsWith(".pair") ||
       file.relativePath.endsWith(".syn"))
      continue;
    QTest::newRow(qPrintable(file.relativePath)) << file;
    any = true;
  }

  if(any == false)
    QTest::newRow("no files") << MedIOTestFile();
}

void TestInterfileFile::write()
{
  QFETCH(MedIOTestFile, file);
  if(file.path.isEmpty())
    QSKIP("no test data");

  QScopedPointer<CMedIOData> source(openFile(fileToOpen(file)));
  QVERIFY(source.isNull() == false);
  const QList<short> numbers = source->frameNumbers();

  const QString name = m_tmp.filePath(QString(file.relativePath).replace('/', '_') + ".hv");
  QScopedPointer<CMedIOData> target(CMedIODataFactory::createForWriting(name));
  QVERIFY(target.isNull() == false);
  QCOMPARE(target->dataFormat(), CMedIOData::Interfile);

  CMedIOHeader* header = NULL;
  QVERIFY(source->readMainHeader(header, CMedIOData::Interfile));
  QScopedPointer<CMedIOHeader> mainHeader(header);
  QCOMPARE(header->headerFormat(), CMedIOHeader::InterfileMainHeader);
  QCOMPARE(CMedIOData::requiredFrames(*header), numbers.size());

  QVERIFY(target->open(QIODevice::WriteOnly));
  QVERIFY(target->writeMainHeader(*mainHeader));
  for(int f = 0; f < numbers.size(); f++)
  {
    CMedIOVolume v;
    QVERIFY(source->readVolume(v, numbers.at(f)));
    QVERIFY(target->writeVolume(v, short(f + 1)));
  }
  target->close();

  // read again: the same values at the same positions, the same timing and attributes
  QJsonObject sourceAttributes;
  QVERIFY(source->readAttributes(sourceAttributes));
  QCOMPARE(CMedIODataFactory::identify(name), CMedIOData::Interfile);
  QScopedPointer<CMedIOData> written(openFile(name));
  QVERIFY(written.isNull() == false);
  QCOMPARE(written->frameNumbers().size(), numbers.size());
  for(int f = 0; f < numbers.size(); f++)
  {
    CMedIOVolume v;
    CMedIOVolume w;
    QVERIFY(source->readVolume(v, numbers.at(f)));
    QVERIFY(written->readVolume(w, short(f + 1)));
    // HFS (or no patient position): the axes of STIR are "LPI"
    QCOMPARE(w.orientation(), QString("LPI"));
    QVERIFY(v.reorient(w.orientation()));
    QVERIFY2(v.hasSameGeometry(w), "the positions of the voxels differ");
    double maxDifference = 0.0;
    double maxValue = 0.0;
    for(qint64 i = 0; i < v.voxelCount(); i++)
    {
      maxDifference = qMax(maxDifference, std::fabs(v.value(i) - w.value(i)));
      maxValue = qMax(maxValue, std::fabs(v.value(i)));
    }
    QVERIFY2(maxDifference <= 1e-6 * maxValue, qPrintable(QString("max. difference %1").arg(maxDifference)));
    // integers are kept as integers with their scale factor
    if(v.dataType() == CMedIOVolume::Int16 && v.hasUniformScaleFactor() && v.intercept() == 0.0)
    {
      QCOMPARE(w.dataType(), CMedIOVolume::Int16);
      QCOMPARE(w.scaleFactor(), v.scaleFactor());
    }
    QVERIFY(std::fabs(w.frameStart() - v.frameStart()) < 1e-6);
    QVERIFY(std::fabs(w.frameDuration() - v.frameDuration()) < 1e-6);
  }

  QJsonObject writtenAttributes;
  QVERIFY(written->readAttributes(writtenAttributes));
  foreach(const QString& key, QStringList() << "PatientName" << "PatientID" << "PatientBirthDate" << "PatientSex"
                                            << "PatientWeight" << "PatientSize" << "PatientPosition"
                                            << "AcquisitionDateTime" << "ManufacturerModelName"
                                            << "Radiopharmaceutical" << "RadiopharmaceuticalStartDateTime"
                                            << "RadionuclideTotalDose" << "RadionuclideHalfLife"
                                            << "RadionuclidePositronFraction" << "EnergyWindowLowerLimit"
                                            << "EnergyWindowUpperLimit" << "Units" << "AttenuationCorrectionMethod"
                                            << "ScatterCorrectionMethod")
  {
    const QJsonValue value = sourceAttributes.value(key);
    if(value.isUndefined() || (key == "PatientPosition" && value.toString() != "HFS"))
      continue;
    if(key == "AcquisitionDateTime" || key == "RadiopharmaceuticalStartDateTime")
    {
      // the same time, in UTC if it has a zone
      const QDateTime expected = QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
      const QDateTime actual = QDateTime::fromString(writtenAttributes.value(key).toString(), Qt::ISODateWithMs);
      QVERIFY2(actual.isValid() && actual == expected, qPrintable(key + ": " + writtenAttributes.value(key).toString()));
      continue;
    }
    QVERIFY2(writtenAttributes.value(key) == value, qPrintable(key));
  }
}

// the patient positions of STIR: the voxel axes and keys of the header
void TestInterfileFile::writePatientPositions_data()
{
  QTest::addColumn<QString>("position");
  QTest::addColumn<QString>("orientation");
  QTest::addColumn<QString>("patientOrientation");
  QTest::addColumn<QString>("patientRotation");

  QTest::newRow("HFS") << "HFS" << "LPI" << "head_in" << "supine";
  QTest::newRow("FFS") << "FFS" << "RPS" << "feet_in" << "supine";
  QTest::newRow("HFP") << "HFP" << "RAI" << "head_in" << "prone";
  QTest::newRow("FFP") << "FFP" << "LAS" << "feet_in" << "prone";
  QTest::newRow("none") << "" << "LPI" << "head_in" << "supine";
}

void TestInterfileFile::writePatientPositions()
{
  QFETCH(QString, position);
  QFETCH(QString, orientation);
  QFETCH(QString, patientOrientation);
  QFETCH(QString, patientRotation);

  // a volume with distinct values in "RAS" order, not axis aligned with all formats
  const int nx = 4, ny = 3, nz = 5;
  QVector<float> values(nx * ny * nz);
  for(int i = 0; i < values.size(); i++)
    values[i] = 0.5f * float(i) - 3.0f;
  CMedIOVolume v;
  QVERIFY(v.setData(nx, ny, nz, CMedIOVolume::Float32,
                    QByteArray(reinterpret_cast<const char*>(values.constData()), int(values.size() * sizeof(float)))));
  const double m[3][4] = { { -2.0, 0.0, 0.0, 10.5 }, { 0.0, -3.0, 0.0, -20.25 }, { 0.0, 0.0, 4.0, 37.0 } };
  v.setAffine(m);
  v.setFrameStart(12.0);
  v.setFrameDuration(30.0);

  CMedIOHeader* header = NULL;
  QVERIFY(CMedIOData::mainHeaderFromVolume(v, CMedIOData::Interfile, 2, header));
  QScopedPointer<CMedIOHeader> mainHeader(header);
  if(position.isEmpty() == false)
    QVERIFY(mainHeader->updateAttributes(QJsonObject{{"PatientPosition", position}, {"PatientName", "Test^Name"}}));

  const QString name = m_tmp.filePath("position-" + QString(QTest::currentDataTag()) + ".hv");
  QScopedPointer<CMedIOData> target(CMedIODataFactory::createForWriting(name));
  QVERIFY(target->open(QIODevice::WriteOnly));
  QVERIFY(target->writeMainHeader(*mainHeader));
  QVERIFY(target->writeVolume(v, 2));
  v.setFrameStart(42.0);
  QVERIFY(target->writeVolume(v, 1));
  target->close();

  CInterfileKeys keys;
  QVERIFY(keys.read(name));
  QCOMPARE(keys.value("patient orientation"), patientOrientation);
  QCOMPARE(keys.value("patient rotation"), patientRotation);
  QCOMPARE(keys.value("number of time frames"), QString("2"));
  QCOMPARE(keys.value("!number format"), QString("float"));
  QCOMPARE(keys.value("name of data file"), QString("position-%1.v").arg(QTest::currentDataTag()));
  QCOMPARE(keys.value("data offset in bytes[2]"), QString::number(nx * ny * nz * 4));

  QScopedPointer<CMedIOData> written(openFile(name));
  QVERIFY(written.isNull() == false);
  CMedIOVolume w;
  QVERIFY(written->readVolume(w, 2));
  QCOMPARE(w.orientation(), orientation);
  QCOMPARE(w.frameStart(), 12.0);
  QVERIFY(written->readVolume(w, 1));
  QCOMPARE(w.frameStart(), 42.0);
  QCOMPARE(w.frameDuration(), 30.0);
  QVERIFY(w.reorient(v.orientation()));
  QVERIFY(v.hasSameGeometry(w));
  QVERIFY(w.data() == v.data());

  QJsonObject attributes;
  QVERIFY(written->readAttributes(attributes));
  if(position.isEmpty() == false)
  {
    QCOMPARE(attributes.value("PatientPosition").toString(), position);
    QCOMPARE(attributes.value("PatientName").toString(), QString("Test^Name"));
  }
}

void TestInterfileFile::writeErrors()
{
  const int nx = 3, ny = 2, nz = 2;
  CMedIOVolume v;
  QVERIFY(v.setData(nx, ny, nz, CMedIOVolume::Int16, QByteArray(nx * ny * nz * 2, '\1')));
  v.setScaleFactor(0.5);
  CMedIOVolume f;
  QVERIFY(f.setData(nx, ny, nz, CMedIOVolume::Float32, QByteArray(nx * ny * nz * 4, '\0')));
  CMedIOVolume larger;
  QVERIFY(larger.setData(nx + 1, ny, nz, CMedIOVolume::Int16, QByteArray((nx + 1) * ny * nz * 2, '\0')));

  CMedIOHeader* header = NULL;
  QVERIFY(CMedIOData::mainHeaderFromVolume(v, CMedIOData::Interfile, 2, header));
  QScopedPointer<CMedIOHeader> mainHeader(header);

  const QString name = m_tmp.filePath("errors.hv");
  QScopedPointer<CMedIOData> target(CMedIODataFactory::createForWriting(name));
  QVERIFY(target->open(QIODevice::WriteOnly));
  QVERIFY(target->writeVolume(v, 1) == false);           // no main header yet
  QVERIFY(target->writeMainHeader(*mainHeader));
  QVERIFY(target->writeMainHeader(*mainHeader) == false); // only once
  QVERIFY(target->writeVolume(v, 3) == false);           // 2 frames
  QVERIFY(target->writeVolume(v, 1));
  QVERIFY(target->writeVolume(f, 2) == false);           // float32 values in a file of integers
  QVERIFY(target->writeVolume(larger, 2) == false);      // another matrix
  target->close();

  // the file without its second frame is incomplete
  QCOMPARE(CMedIODataFactory::identify(name), CMedIOData::Unknown);
  QVERIFY(target->removeFiles());
  QVERIFY(QFile::exists(name) == false && QFile::exists(m_tmp.filePath("errors.v")) == false);
}

void TestInterfileFile::notInterfile()
{
  // files of the other formats are not Interfile
  foreach(const MedIOTestFile& file, m_data.files)
  {
    if(file.format == "interfile")
      continue;
    QString path = file.path;
    if(QFileInfo(path).isDir())
    {
      const QStringList names = QDir(path).entryList(QDir::Files);
      if(names.isEmpty())
        continue;
      path = QDir(path).filePath(names.first());
    }
    QVERIFY2(CInterfileFile::isOfType(path) == false, qPrintable(file.relativePath));
  }

  // a data file of the size described, but too short
  const QString header = m_tmp.filePath("short.hv");
  QFile file(header);
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write("!INTERFILE :=\n!name of data file := short.v\n!matrix size [1] := 4\n!matrix size [2] := 4\n"
             "!matrix size [3] := 1\n!number format := float\n!number of bytes per pixel := 4\n"
             "!END OF INTERFILE :=\n");
  file.close();
  QFile dataFile(m_tmp.filePath("short.v"));
  QVERIFY(dataFile.open(QIODevice::WriteOnly));
  dataFile.write(QByteArray(60, '\0'));
  dataFile.close();

  CInterfileFile interfile(header);
  QVERIFY(interfile.open(QIODevice::ReadOnly) == false);
  QVERIFY2(interfile.openError().contains("60 bytes"), qPrintable(interfile.openError()));
  QCOMPARE(CMedIODataFactory::identify(header), CMedIOData::Unknown);
}

QTEST_GUILESS_MAIN(TestInterfileFile)
#include "testInterfileFile.moc"
