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

// Conversions between all writable formats with the format independent interface:
// every image test file is written into every other writable format (ECAT7, NIfTI,
// Philips, Interfile) and read again, with readVolume()/writeVolume() and with
// readMatrix()/writeMatrix(). The physical values, the positions of the voxels,
// the frame timing, the stored data type and the attributes have to be those of the
// source, except where a format cannot store them (see the exceptions below). Chains
// of conversions through all formats keep them as well.

#include <QtTest/QtTest>
#include <QJsonArray>
#include <QJsonObject>
#include <QScopedPointer>
#include <QTemporaryDir>

#include <cmath>

#include "CMedIOData.h"
#include "CHDF5File.h"
#include "CMedIODataFactory.h"
#include "CMedIOHeader.h"
#include "CMedIOVolume.h"

#include "MedIOTestData.h"

Q_DECLARE_METATYPE(MedIOTestFile)

namespace {

struct Target
{
  const char* name;
  const char* suffix;
  CMedIOData::Format format;
};

const Target Targets[] = {
  { "ECAT7", ".v", CMedIOData::ECAT },
  { "NIfTI", ".nii", CMedIOData::NIFTI },
  { "Philips", ".img", CMedIOData::Philips },
  { "Interfile", ".hv", CMedIOData::Interfile },
  { "HDF5", ".h5", CMedIOData::HDF5 }   // only with HDF5 support
};

// the attributes compared, and those a format cannot store (they are not in its files)
const char* const AttributeKeys[] = {
  "PatientName", "PatientID", "PatientBirthDate", "PatientWeight", "PatientSize", "AcquisitionDateTime",
  "ManufacturerModelName", "Radiopharmaceutical", "RadiopharmaceuticalStartDateTime", "RadionuclideTotalDose",
  "RadionuclideHalfLife", "EnergyWindowLowerLimit", "EnergyWindowUpperLimit", "Units", "ReconstructionMethod"
};

bool storable(CMedIOData::Format format, const QString& key)
{
  // Philips files have no field for the units and the reconstruction method, ECAT7 files
  // none for the reconstruction method
  if(format == CMedIOData::Philips && (key == "Units" || key == "ReconstructionMethod"))
    return false;
  if(format == CMedIOData::ECAT && key == "ReconstructionMethod")
    return false;
  return true;
}

// the file to open: the file itself, the main header (.mhdr) or the header of a directory
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

// how the frames of a file are transferred into the target
enum Transfer
{
  Volumes,  // readVolume()/writeVolume()
  Matrices  // readMatrix() in the representation of the target and writeMatrix()
};

// write all frames of a file into a new file of another format: the main header of the
// source in the representation of the target, every frame as volume or as matrix with
// its sub header in the representation of the target
bool convert(const QString& source, const QString& target, CMedIOData::Format format, QString* error,
             Transfer transfer = Volumes)
{
  QScopedPointer<CMedIOData> in(openFile(source));
  QScopedPointer<CMedIOData> out(CMedIODataFactory::createForWriting(target, format));
  if(in.isNull() || out.isNull())
  {
    *error = "cannot open the source or create the target";
    return false;
  }

  CMedIOHeader* header = NULL;
  if(in->readMainHeader(header, format) == false || header == NULL)
  {
    delete header;
    *error = "cannot convert the main header";
    return false;
  }
  QScopedPointer<CMedIOHeader> mainHeader(header);

  if(out->open(QIODevice::WriteOnly) == false || out->writeMainHeader(*mainHeader) == false)
  {
    *error = "cannot write the main header";
    return false;
  }
  const QList<short> numbers = in->frameNumbers();
  for(int f = 0; f < numbers.size(); f++)
  {
    bool written = false;
    if(transfer == Matrices)
    {
      QByteArray* data = NULL;
      CMedIOHeader* subHeader = NULL;
      written = in->readMatrix(data, subHeader, format, numbers.at(f)) && data != NULL && subHeader != NULL &&
                out->writeMatrix(*data, *subHeader, short(f + 1), mainHeader.data());
      delete data;
      delete subHeader;
    }
    else
    {
      CMedIOVolume v;
      written = in->readVolume(v, numbers.at(f)) && out->writeVolume(v, short(f + 1));
    }
    if(written == false)
    {
      *error = QString("cannot write frame %1").arg(f + 1);
      return false;
    }
  }
  out->close();

  return true;
}

// compare the frames and attributes of a file with those of its source; through: the
// formats the image was written into on its way (an attribute one of them cannot store
// is lost)
void compareFiles(const QString& sourceName, const QString& targetName, CMedIOData::Format format,
                  const QList<CMedIOData::Format>& through)
{
  QScopedPointer<CMedIOData> source(openFile(sourceName));
  QScopedPointer<CMedIOData> target(openFile(targetName));
  QVERIFY(source.isNull() == false && target.isNull() == false);
  QCOMPARE(target->dataFormat(), format);

  const QList<short> numbers = source->frameNumbers();
  QCOMPARE(target->frameNumbers().size(), numbers.size());

  for(int f = 0; f < numbers.size(); f++)
  {
    CMedIOVolume v;
    CMedIOVolume w;
    QVERIFY(source->readVolume(v, numbers.at(f)));
    QVERIFY(target->readVolume(w, target->frameNumbers().at(f)));
    QVERIFY(w.reorient(v.orientation()));
    for(int i = 0; i < 3; i++)
      QCOMPARE(w.dimension(i), v.dimension(i));

    // positions: Philips files store no position within the planes (they are centered),
    // only the axes and the position along z
    if(format == CMedIOData::Philips)
    {
      double a[3][4];
      double b[3][4];
      v.affine(a);
      w.affine(b);
      for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
          QVERIFY2(std::fabs(a[i][j] - b[i][j]) < 1e-4, "the voxel axes differ");
      QVERIFY2(std::fabs(a[2][3] - b[2][3]) < 1e-2, "the z position differs");
    }
    else
      QVERIFY2(w.hasSameGeometry(v), "the positions of the voxels differ");

    double maxDifference = 0.0;
    double maxValue = 0.0;
    for(qint64 i = 0; i < v.voxelCount(); i++)
    {
      maxDifference = qMax(maxDifference, std::fabs(v.value(i) - w.value(i)));
      maxValue = qMax(maxValue, std::fabs(v.value(i)));
    }
    QVERIFY2(maxDifference <= 1e-6 * maxValue,
             qPrintable(QString("frame %1: max. difference %2 (max. value %3)").arg(f + 1).arg(maxDifference).arg(maxValue)));

    // 16 bit integers stay integers (NIfTI files of several frames: the physical values);
    // HDF5 files always hold the physical values as float32 (docs/hdf5.md), so an image
    // that was stored as HDF5 on its way has no integers anymore
    const bool niftiFrames = (format == CMedIOData::NIFTI && numbers.size() > 1);
    if(format == CMedIOData::HDF5)
    {
      QCOMPARE(w.dataType(), v.dataType() == CMedIOVolume::Float64 ? CMedIOVolume::Float64 : CMedIOVolume::Float32);
      QCOMPARE(w.scaleFactor(), 1.0);
    }
    else if(v.dataType() == CMedIOVolume::Int16 && niftiFrames == false && through.contains(CMedIOData::HDF5) == false)
      QCOMPARE(w.dataType(), CMedIOVolume::Int16);

    QVERIFY2(std::fabs(w.frameStart() - v.frameStart()) < 1e-3, qPrintable(QString("frame %1: start").arg(f + 1)));
    QVERIFY2(std::fabs(w.frameDuration() - v.frameDuration()) < 1e-3, qPrintable(QString("frame %1: duration").arg(f + 1)));
  }

  QJsonObject expected;
  QJsonObject actual;
  QVERIFY(source->readAttributes(expected));
  QVERIFY(target->readAttributes(actual));
  for(unsigned k = 0; k < sizeof(AttributeKeys) / sizeof(AttributeKeys[0]); k++)
  {
    const QString key = QString::fromLatin1(AttributeKeys[k]);
    bool kept = expected.contains(key);
    foreach(CMedIOData::Format f, through)
      kept = kept && storable(f, key);
    if(kept == false)
      continue;
    // times: the same time, in UTC if they have a zone
    if(key.endsWith("DateTime"))
    {
      const QDateTime a = QDateTime::fromString(expected.value(key).toString(), Qt::ISODateWithMs);
      const QDateTime b = QDateTime::fromString(actual.value(key).toString(), Qt::ISODateWithMs);
      QVERIFY2(a.isValid() && a == b, qPrintable(QString("%1: %2 instead of %3").arg(key, actual.value(key).toString(),
                                                                                    expected.value(key).toString())));
      continue;
    }
    const QJsonValue a = expected.value(key);
    const QJsonValue b = actual.value(key);
    const bool same = (a.isDouble() && b.isDouble()) ? std::fabs(a.toDouble() - b.toDouble()) <= 1e-6 * std::fabs(a.toDouble())
                                                      : a == b;
    QVERIFY2(same, qPrintable(QString("%1: %2 instead of %3").arg(key, b.toVariant().toString(), a.toVariant().toString())));
  }
}

} // namespace

class TestConversions : public QObject
{
  Q_OBJECT

  private:
    MedIOTestData m_data;
    QTemporaryDir m_tmp;

  private slots:
    void initTestCase();
    void allFormats_data();
    void allFormats();
    void allFormatsMatrices_data();
    void allFormatsMatrices();
    void hdf5Matrices_data();
    void hdf5Matrices();
    void chains_data();
    void chains();
    void volumeAttributes_data();
    void volumeAttributes();
    void volumeAttributesOfFiles_data();
    void volumeAttributesOfFiles();
};

void TestConversions::initTestCase()
{
  m_data = medioTestData();
  QVERIFY(m_tmp.isValid());
}

void TestConversions::allFormats_data()
{
  QTest::addColumn<MedIOTestFile>("file");
  QTest::addColumn<int>("target");

  bool any = false;
  foreach(const MedIOTestFile& file, m_data.files)
  {
    // the image files (one per format and dataset: the others hold the same images)
    if(file.format == "dicom" || file.format == "raw" || file.relativePath.endsWith(".pair") ||
       file.relativePath.endsWith(".syn"))
      continue;
    for(unsigned t = 0; t < sizeof(Targets) / sizeof(Targets[0]); t++)
    {
      if(file.format == QString(Targets[t].name).toLower().remove(QRegularExpression("[0-9]")))
        continue;
      QTest::newRow(qPrintable(file.relativePath + " -> " + Targets[t].name)) << file << int(t);
      any = true;
    }
  }

  if(any == false)
    QTest::newRow("no files") << MedIOTestFile() << 0;
}

void TestConversions::allFormats()
{
  QFETCH(MedIOTestFile, file);
  QFETCH(int, target);
  if(file.path.isEmpty())
    QSKIP("no test data");

  const Target& t = Targets[target];
  if(t.format == CMedIOData::HDF5 && CHDF5File::hasSupport() == false)
    QSKIP("no HDF5 support");
  const QString source = fileToOpen(file);
  const QString name = m_tmp.filePath(QString(file.relativePath).replace('/', '_') + t.suffix);
  QString error;
  QVERIFY2(convert(source, name, t.format, &error), qPrintable(error));
  compareFiles(source, name, t.format, QList<CMedIOData::Format>() << t.format);
}

// The same conversions with readMatrix()/writeMatrix(): the matrix of every frame is read
// in the representation of the target, with a sub header that is converted from the
// source independently of the main header. The frame timing, the positions and the values
// of the written files have to be those of the source as well.
void TestConversions::allFormatsMatrices_data()
{
  allFormats_data();
}

void TestConversions::allFormatsMatrices()
{
  QFETCH(MedIOTestFile, file);
  QFETCH(int, target);
  if(file.path.isEmpty())
    QSKIP("no test data");

  const Target& t = Targets[target];
  if(t.format == CMedIOData::HDF5 && CHDF5File::hasSupport() == false)
    QSKIP("no HDF5 support");
  const QString source = fileToOpen(file);
  const QString name = m_tmp.filePath("matrix_" + QString(file.relativePath).replace('/', '_') + t.suffix);
  QString error;
  QVERIFY2(convert(source, name, t.format, &error, Matrices), qPrintable(error));
  compareFiles(source, name, t.format, QList<CMedIOData::Format>() << t.format);
}

// The test data holds no HDF5 images: HDF5 files written from other formats are the
// sources of the conversions with readMatrix()/writeMatrix() into every other format
void TestConversions::hdf5Matrices_data()
{
  QTest::addColumn<QString>("source");
  QTest::addColumn<int>("target");

  const char* const sources[] = { "ecat/pet-dynamic.v", "interfile/pet-bladder-dynamic.siemens/pet-bladder-dynamic.mhdr" };
  for(unsigned s = 0; s < sizeof(sources) / sizeof(sources[0]); s++)
  {
    for(unsigned t = 0; t < sizeof(Targets) / sizeof(Targets[0]); t++)
    {
      if(Targets[t].format != CMedIOData::HDF5)
        QTest::newRow(qPrintable(QString("%1 -> HDF5 -> %2").arg(sources[s], Targets[t].name))) << QString(sources[s]) << int(t);
    }
  }
}

void TestConversions::hdf5Matrices()
{
  QFETCH(QString, source);
  QFETCH(int, target);

  const QString first = medioTestDataDir() + "/" + source;
  if(QFileInfo::exists(first) == false)
    QSKIP("no test data");
  if(CHDF5File::hasSupport() == false)
    QSKIP("no HDF5 support");

  const Target& t = Targets[target];
  const QString base = m_tmp.filePath("hdf5_" + QString(source).replace('/', '_'));
  const QString hdf5 = base + ".h5";
  const QString name = base + t.suffix;
  QString error;
  QVERIFY2(convert(first, hdf5, CMedIOData::HDF5, &error), qPrintable(error));
  QVERIFY2(convert(hdf5, name, t.format, &error, Matrices), qPrintable(error));
  compareFiles(first, name, t.format, QList<CMedIOData::Format>() << CMedIOData::HDF5 << t.format);
}

void TestConversions::chains_data()
{
  QTest::addColumn<QString>("source");
  QTest::addColumn<QStringList>("suffixes");

  QTest::newRow("ECAT7 through all formats") << "ecat/pet-static.v"
                                             << (QStringList() << ".hv" << ".nii" << ".img" << ".hv" << ".v");
  QTest::newRow("ECAT7 dynamic and back") << "ecat/pet-dynamic.v" << (QStringList() << ".hv" << ".v");
  QTest::newRow("Siemens Interfile and back")
      << "interfile/pet-bladder-dynamic.siemens/pet-bladder-dynamic.mhdr" << (QStringList() << ".v" << ".hv");
  QTest::newRow("ECAT7 dynamic through HDF5") << "ecat/pet-dynamic.v" << (QStringList() << ".h5" << ".nii" << ".hdf5" << ".v");
  QTest::newRow("Siemens Interfile through HDF5")
      << "interfile/pet-bladder-dynamic.siemens/pet-bladder-dynamic.mhdr" << (QStringList() << ".h5" << ".hv" << ".hdf");
}

void TestConversions::chains()
{
  QFETCH(QString, source);
  QFETCH(QStringList, suffixes);

  const QString first = medioTestDataDir() + "/" + source;
  if(QFileInfo::exists(first) == false)
    QSKIP("no test data");
  if(suffixes.contains(".h5") && CHDF5File::hasSupport() == false)
    QSKIP("no HDF5 support");

  // every file of the chain has the values, positions and timing of the source
  QString current = first;
  QList<CMedIOData::Format> through;
  for(int s = 0; s < suffixes.size(); s++)
  {
    const QString next = m_tmp.filePath(QString("chain-%1-%2%3").arg(QTest::currentDataTag()).arg(s).arg(suffixes[s])
                                            .replace(' ', '_'));
    const CMedIOData::Format format = CMedIODataFactory::formatOfFileName(next);
    QString error;
    QVERIFY2(convert(current, next, format, &error), qPrintable(error));
    through << format;
    compareFiles(first, next, format, through);
    if(QTest::currentTestFailed())
      return;
    current = next;
  }
}

// the attributes of a volume are stored with the frame written, as far as the format has
// a place for them, and the volume itself wins over attributes that describe it
void TestConversions::volumeAttributes_data()
{
  QTest::addColumn<int>("target");
  QTest::addColumn<QStringList>("stored");

  // DecayFactor: ECAT7 image sub header (decay_corr_fctr); DeadTimeFactor: Philips sub
  // header (deadtimefac); HDF5: datasets per volume for both and FrameReferenceTime.
  // Interfile headers as STIR writes them have no key for them, and NIfTI files store
  // the frame attributes with their main header, not with the volumes written.
  QTest::newRow("ECAT7") << 0 << (QStringList() << "DecayFactor");
  QTest::newRow("NIfTI") << 1 << QStringList();
  QTest::newRow("Philips") << 2 << (QStringList() << "DeadTimeFactor");
  QTest::newRow("Interfile") << 3 << QStringList();
  QTest::newRow("HDF5") << 4 << (QStringList() << "DecayFactor" << "DeadTimeFactor" << "FrameReferenceTime");
}

void TestConversions::volumeAttributes()
{
  QFETCH(int, target);
  QFETCH(QStringList, stored);
  const Target& t = Targets[target];
  if(t.format == CMedIOData::HDF5 && CHDF5File::hasSupport() == false)
    QSKIP("no HDF5 support");

  // two frames with different attributes
  const int nx = 4, ny = 3, nz = 2;
  QByteArray data(nx * ny * nz * int(sizeof(float)), '\0');
  float* values = reinterpret_cast<float*>(data.data());
  for(int i = 0; i < nx * ny * nz; i++)
    values[i] = float(i + 1);
  const double affine[3][4] = { { 2.0, 0.0, 0.0, -3.0 }, { 0.0, 2.0, 0.0, -2.0 }, { 0.0, 0.0, -3.0, 30.0 } };

  CMedIOVolume volumes[2];
  for(int f = 0; f < 2; f++)
  {
    CMedIOVolume& v = volumes[f];
    QVERIFY(v.setData(nx, ny, nz, CMedIOVolume::Float32, data));
    v.setAffine(affine);
    v.setFrameStart(10.0 + 20.0 * f);
    v.setFrameDuration(20.0);
    QVERIFY(v.attributes().isEmpty());
    QJsonObject attributes;
    attributes["DecayFactor"] = 1.25 + f;
    attributes["DeadTimeFactor"] = 1.5 + f;
    attributes["FrameReferenceTime"] = 19000.0 + 20000.0 * f;
    // what describes the volume is not taken from the attributes: the frame duration
    // and the native fields of every format
    attributes["ActualFrameDuration"] = 99000;
    attributes["ECAT7"] = QJsonObject{{"X_Dimension", 99}, {"Frame_Duration", 99000}, {"Scale_Factor", 99.0}};
    attributes["Philips"] = QJsonObject{{"xdim", 99}, {"scale_factor", 99.0}};
    attributes["Interfile"] = QJsonObject{{"matrix size [1]", 99}};
    attributes["NIfTI"] = QJsonObject{{"scl_slope", 99.0}};
    v.setAttributes(attributes);
    QCOMPARE(v.attributes(), attributes);

    // copies have the attributes, and changes of the volume keep them
    CMedIOVolume copy(v);
    QCOMPARE(copy.attributes(), attributes);
    CMedIOVolume assigned;
    assigned = v;
    QCOMPARE(assigned.attributes(), attributes);
    QVERIFY(copy.reorient("RAS") && copy.convertToPhysicalValues());
    QCOMPARE(copy.attributes(), attributes);
    copy.clear();
    QVERIFY(copy.attributes().isEmpty());
  }

  const QString name = m_tmp.filePath(QString("attributes-%1%2").arg(t.name, t.suffix));
  {
    QScopedPointer<CMedIOData> out(CMedIODataFactory::createForWriting(name, t.format));
    QVERIFY(out.isNull() == false);
    CMedIOHeader* header = NULL;
    QVERIFY(CMedIOData::mainHeaderFromVolume(volumes[0], t.format, 2, header) && header != NULL);
    QScopedPointer<CMedIOHeader> mainHeader(header);
    QVERIFY(out->open(QIODevice::WriteOnly) && out->writeMainHeader(*mainHeader));
    QVERIFY(out->writeVolume(volumes[0], 1));
    QVERIFY(out->writeVolume(volumes[1], 2));
    out->close();
  }

  QScopedPointer<CMedIOData> in(openFile(name));
  QVERIFY(in.isNull() == false);
  QCOMPARE(in->frameNumbers().size(), 2);
  QJsonObject fileAttributes;
  QVERIFY(in->readAttributes(fileAttributes));
  const QJsonArray frames = fileAttributes.value("Frames").toArray();
  for(int f = 0; f < 2; f++)
  {
    CMedIOVolume v;
    QVERIFY(in->readVolume(v, in->frameNumbers().at(f)));

    // the volume as written
    QCOMPARE(v.dimension(0), nx);
    QCOMPARE(v.dimension(1), ny);
    QCOMPARE(v.dimension(2), nz);
    QVERIFY(v.reorient(volumes[f].orientation()));
    for(int i = 0; i < nx * ny * nz; i++)
      QCOMPARE(v.value(i), double(i + 1));
    QCOMPARE(v.frameStart(), 10.0 + 20.0 * f);
    QCOMPARE(v.frameDuration(), 20.0);

    // its attributes: those of the frame in the file, with the stored ones of the volume
    const QJsonObject attributes = v.attributes();
    QCOMPARE(attributes, frames.at(f).toObject());
    // (NIfTI files give the durations of all frames with their main header: FrameDuration)
    if(t.format != CMedIOData::NIFTI)
      QCOMPARE(attributes.value("ActualFrameDuration").toDouble(), 20000.0);
    const double expected[] = { 1.25 + f, 1.5 + f, 19000.0 + 20000.0 * f };
    const char* const keys[] = { "DecayFactor", "DeadTimeFactor", "FrameReferenceTime" };
    for(int k = 0; k < 3; k++)
    {
      if(stored.contains(keys[k]))
        QVERIFY2(qAbs(attributes.value(keys[k]).toDouble() - expected[k]) < 1e-6 * expected[k], keys[k]);
      else
        QVERIFY2(attributes.contains(keys[k]) == false ||
                 qAbs(attributes.value(keys[k]).toDouble() - expected[k]) > 1e-6 * expected[k], keys[k]);
    }
  }
}

// a frame read as volume has the attributes of its sub header, and keeps those the target
// stores when it is written into a file of another format
void TestConversions::volumeAttributesOfFiles_data()
{
  allFormats_data();
}

void TestConversions::volumeAttributesOfFiles()
{
  QFETCH(MedIOTestFile, file);
  QFETCH(int, target);
  if(file.path.isEmpty())
    QSKIP("no test data");
  const Target& t = Targets[target];
  if(t.format == CMedIOData::HDF5 && CHDF5File::hasSupport() == false)
    QSKIP("no HDF5 support");

  const QString sourceName = fileToOpen(file);
  const QString name = m_tmp.filePath(QString("volume-") + QString(file.relativePath).replace('/', '_') + t.suffix);
  QString error;
  QVERIFY2(convert(sourceName, name, t.format, &error), qPrintable(error));

  QScopedPointer<CMedIOData> source(openFile(sourceName));
  QScopedPointer<CMedIOData> written(openFile(name));
  QVERIFY(source.isNull() == false && written.isNull() == false);
  QJsonObject sourceAttributes;
  QVERIFY(source->readAttributes(sourceAttributes));
  const QJsonArray sourceFrames = sourceAttributes.value("Frames").toArray();
  const QList<short> numbers = source->frameNumbers();
  const QList<short> writtenNumbers = written->frameNumbers();
  QCOMPARE(writtenNumbers.size(), numbers.size());

  for(int f = 0; f < numbers.size(); f++)
  {
    CMedIOVolume v, w;
    QVERIFY(source->readVolume(v, numbers.at(f)) && written->readVolume(w, writtenNumbers.at(f)));
    // the attributes of the frame as readAttributes() gives them
    QCOMPARE(v.attributes(), f < sourceFrames.size() ? sourceFrames.at(f).toObject() : QJsonObject());

    // ECAT7 and HDF5 files store the decay factor, Philips and HDF5 files the dead time factor
    const bool decay = (t.format == CMedIOData::ECAT || t.format == CMedIOData::NIFTI || t.format == CMedIOData::HDF5);
    const bool deadTime = (t.format == CMedIOData::Philips || t.format == CMedIOData::NIFTI || t.format == CMedIOData::HDF5);
    if(decay && v.attributes().contains("DecayFactor"))
      QCOMPARE(w.attributes().value("DecayFactor"), v.attributes().value("DecayFactor"));
    if(deadTime && v.attributes().contains("DeadTimeFactor"))
      QCOMPARE(w.attributes().value("DeadTimeFactor"), v.attributes().value("DeadTimeFactor"));
  }
}

QTEST_GUILESS_MAIN(TestConversions)
#include "testConversions.moc"
