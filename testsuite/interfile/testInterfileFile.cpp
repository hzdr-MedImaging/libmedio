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
#include "CInterfileMainHeader.h"

#include "MedIOTestData.h"

Q_DECLARE_METATYPE(MedIOTestFile)

namespace {

// the file to open: the file itself, the main header (.mhdr) or the header (.hdr) of
// a directory
QString fileToOpen(const MedIOTestFile& file)
{
  if(QFileInfo(file.path).isDir() == false)
    return file.path;

  const QDir dir(file.path);
  QStringList names = dir.entryList(QStringList() << "*.mhdr", QDir::Files);
  if(names.isEmpty())
    names = dir.entryList(QStringList() << "*.hdr", QDir::Files);
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

// the factor of the units of the expected values into Bq/ml
double unitsFactor(const QString& units)
{
  return units.startsWith("kBq", Qt::CaseInsensitive) ? 1000.0 : 1.0;
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
    void sameAsEcat_data();
    void sameAsEcat();
    void landmarks_data();
    void landmarks();
    void attributes_data();
    void attributes();
    void toNifti_data();
    void toNifti();

    void readOnly();
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

    const double factor = unitsFactor(expected.value("units").toString());
    QVERIFY(std::fabs(v.scaleFactor() - expected.value("rescale_slope").toDouble() * factor) <=
            1e-12 * std::fabs(v.scaleFactor()));
    QCOMPARE(v.intercept() + 0.0, expected.value("rescale_offset").toDouble() * factor + 0.0);

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
    const int frames = file.expected.value("frames").toArray().size();
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
      QTest::newRow(qPrintable(QDir(medioTestDataDir()).relativeFilePath(path))) << path << frames;
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
// PMOD converted the ECAT7 files into Interfile: the same physical values at the same
// positions (ECAT7 voxel order "LPI")

void TestInterfileFile::sameAsEcat_data()
{
  addInterfileRows(m_data);
}

void TestInterfileFile::sameAsEcat()
{
  QFETCH(MedIOTestFile, file);
  if(file.path.isEmpty())
    QSKIP("no Interfile test datasets");

  const MedIOTestFile* ecat = sameDataset(m_data, QString("ecat/%1.v").arg(file.dataset));
  if(ecat == NULL)
    QSKIP("no ECAT7 file of the dataset");

  QScopedPointer<CMedIOData> interfile(openFile(fileToOpen(file)));
  QScopedPointer<CMedIOData> reference(openFile(ecat->path));
  QVERIFY(interfile.isNull() == false && reference.isNull() == false);

  const QList<short> numbers = interfile->frameNumbers();
  QCOMPARE(numbers.size(), reference->frameNumbers().size());

  for(int f = 0; f < numbers.size(); f++)
  {
    CMedIOVolume v;
    CMedIOVolume e;
    QVERIFY(interfile->readVolume(v, numbers.at(f)));
    QVERIFY(reference->readVolume(e, reference->frameNumbers().at(f)));

    // PMOD writes the planes from inferior to superior
    QCOMPARE(v.orientation(), QString("LPS"));
    QVERIFY(v.reorient("LPI"));
    QCOMPARE(e.orientation(), QString("LPI"));
    for(int i = 0; i < 3; i++)
    {
      QCOMPARE(v.dimension(i), e.dimension(i));
      QVERIFY(std::fabs(v.voxelSize(i) - e.voxelSize(i)) < 1e-4);
    }

    QCOMPARE(v.frameStart(), e.frameStart());
    QCOMPARE(v.frameDuration(), e.frameDuration());

    // stored values identical, scale factors in the precision of ECAT7 (float32)
    double maxDifference = 0.0;
    double maxValue = 0.0;
    for(qint64 i = 0; i < v.voxelCount(); i++)
    {
      QCOMPARE(v.storedValue(i), e.storedValue(i));
      maxDifference = qMax(maxDifference, std::fabs(v.value(i) - e.value(i)));
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
    keys << "Manufacturer" << "ManufacturerModelName" << "PatientPosition" << "ReconstructionMethod"
         << "RadionuclidePositronFraction";

    // the Siemens header gives the injection time in UTC, the DICOM series in the local
    // time of the scanner (CET, UTC+1)
    const QDateTime utc = QDateTime::fromString(attributes.value("RadiopharmaceuticalStartDateTime").toString(),
                                                Qt::ISODate);
    const QDateTime local = QDateTime::fromString(reference.value("RadiopharmaceuticalStartDateTime").toString(),
                                                  "yyyy-MM-ddThh:mm:ss");
    QVERIFY(utc.isValid() && local.isValid());
    QCOMPARE(utc.timeSpec(), Qt::UTC);
    QCOMPARE(QDateTime(local.date(), local.time(), Qt::UTC).toMSecsSinceEpoch() - utc.toMSecsSinceEpoch(),
             qint64(3600 * 1000));
  }
  else
    QSKIP("no reference of the dataset");

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

void TestInterfileFile::readOnly()
{
  QCOMPARE(CMedIODataFactory::formatName(CMedIOData::Interfile), "interfile");
  CMedIOData::Format format = CMedIOData::Unknown;
  QVERIFY(CMedIODataFactory::formatFromName("Interfile", format));
  QCOMPARE(format, CMedIOData::Interfile);
  QVERIFY(CMedIODataFactory::createForWriting(m_tmp.filePath("new.hv"), CMedIOData::Interfile) == NULL);

  // a valid header cannot be opened for writing
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

  CInterfileFile interfile(header);
  QVERIFY(interfile.open(QIODevice::WriteOnly) == false);
  QVERIFY(interfile.openError().contains("read only"));
  QVERIFY2(interfile.open(QIODevice::ReadOnly), qPrintable(interfile.openError()));
  QCOMPARE(interfile.numFrames(), 1);
  interfile.close();

  // the data file is found from the data file name, big endian by default
  QCOMPARE(CMedIODataFactory::identify(m_tmp.filePath("image.v")), CMedIOData::Interfile);
  CInterfileFile byData(m_tmp.filePath("image.v"));
  QVERIFY(byData.open(QIODevice::ReadOnly));
  QCOMPARE(byData.headerFileName(), header);

  CInterfileMainHeader* mainHeader = NULL;
  QVERIFY(byData.readMainHeader(mainHeader));
  QVERIFY(mainHeader->save() == false);
  delete mainHeader;
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
