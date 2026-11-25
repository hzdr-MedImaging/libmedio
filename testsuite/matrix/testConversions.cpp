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
// Philips, Interfile) and read again. The physical values, the positions of the voxels,
// the frame timing, the stored data type and the attributes have to be those of the
// source, except where a format cannot store them (see the exceptions below). Chains
// of conversions through all formats keep them as well.

#include <QtTest/QtTest>
#include <QJsonObject>
#include <QScopedPointer>
#include <QTemporaryDir>

#include <cmath>

#include "CMedIOData.h"
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
  { "Interfile", ".hv", CMedIOData::Interfile }
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

// write all frames of a file into a new file of another format: the main header of the
// source in the representation of the target, every frame as volume
bool convert(const QString& source, const QString& target, CMedIOData::Format format, QString* error)
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
    CMedIOVolume v;
    if(in->readVolume(v, numbers.at(f)) == false || out->writeVolume(v, short(f + 1)) == false)
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

    // 16 bit integers stay integers (NIfTI files of several frames: the physical values)
    const bool niftiFrames = (format == CMedIOData::NIFTI && numbers.size() > 1);
    if(v.dataType() == CMedIOVolume::Int16 && niftiFrames == false)
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
    void chains_data();
    void chains();
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
  const QString source = fileToOpen(file);
  const QString name = m_tmp.filePath(QString(file.relativePath).replace('/', '_') + t.suffix);
  QString error;
  QVERIFY2(convert(source, name, t.format, &error), qPrintable(error));
  compareFiles(source, name, t.format, QList<CMedIOData::Format>() << t.format);
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
}

void TestConversions::chains()
{
  QFETCH(QString, source);
  QFETCH(QStringList, suffixes);

  const QString first = medioTestDataDir() + "/" + source;
  if(QFileInfo::exists(first) == false)
    QSKIP("no test data");

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

QTEST_GUILESS_MAIN(TestConversions)
#include "testConversions.moc"
