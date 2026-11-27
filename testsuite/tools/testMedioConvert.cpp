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

// Tests of the command line tool medioConvert and of the names <source>2<target> it is
// also installed with: the programs are run on the test datasets, and the files they
// write are read with the library and compared with their sources.

#include <QtTest/QtTest>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QScopedPointer>
#include <QTemporaryDir>

#include <cmath>

#include "CHDF5File.h"
#include "CMedIOData.h"
#include "CMedIODataFactory.h"
#include "CMedIOVolume.h"
#include "MedIOTestData.h"

namespace {

struct Format
{
  const char* name;
  const char* extension;
  CMedIOData::Format format;
  const char* source;   // a dynamic test dataset of the format
};

const Format Formats[] = {
  { "ecat", ".v", CMedIOData::ECAT, "ecat/pet-dynamic.v" },
  { "nifti", ".nii", CMedIOData::NIFTI, "nifti/pet-dynamic.nii.gz" },
  { "philips", ".img", CMedIOData::Philips, "philips/pet-dynamic.img" },
  { "interfile", ".hv", CMedIOData::Interfile, "interfile/pet-bladder-dynamic.siemens/pet-bladder-dynamic.mhdr" },
  { "hdf5", ".h5", CMedIOData::HDF5, NULL }   // made from the ECAT7 dataset
};
const int FormatCount = sizeof(Formats) / sizeof(Formats[0]);

// a program of the build tree by its name without version
QString tool(const QString& name)
{
  return QString(MEDIO_TOOL_DIR) + "/" + name + "-" + MEDIO_TOOL_VERSION;
}

struct Result
{
  int exitCode;
  QString output;
  QString error;
};

Result run(const QString& program, const QStringList& arguments)
{
  Result result = { -1, QString(), QString() };
  QProcess process;
  process.start(program, arguments);
  if(process.waitForFinished(120000) && process.exitStatus() == QProcess::NormalExit)
    result.exitCode = process.exitCode();
  result.output = QString::fromLocal8Bit(process.readAllStandardOutput());
  result.error = QString::fromLocal8Bit(process.readAllStandardError());
  return result;
}

QByteArray content(const QString& fileName)
{
  QFile file(fileName);
  return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
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

// empty if the file holds the frames of the source (values, positions, timing), else what differs
QString difference(const QString& sourceName, const QString& targetName, CMedIOData::Format format)
{
  QScopedPointer<CMedIOData> source(openFile(sourceName));
  QScopedPointer<CMedIOData> target(openFile(targetName));
  if(source.isNull() || target.isNull())
    return "cannot open " + (source.isNull() ? sourceName : targetName);
  if(target->dataFormat() != format)
    return QString("the format is %1").arg(CMedIODataFactory::formatName(target->dataFormat()));

  const QList<short> frames = source->frameNumbers();
  const QList<short> written = target->frameNumbers();
  if(frames.isEmpty() || written.size() != frames.size())
    return QString("%1 instead of %2 frames").arg(written.size()).arg(frames.size());

  for(int f = 0; f < frames.size(); f++)
  {
    CMedIOVolume v, w;
    if(source->readVolume(v, frames[f]) == false || target->readVolume(w, written[f]) == false ||
       w.reorient(v.orientation()) == false)
      return QString("cannot read frame %1").arg(f + 1);
    if(w.voxelCount() != v.voxelCount())
      return QString("frame %1 has another size").arg(f + 1);
    // Philips files store no position within the planes
    if(format != CMedIOData::Philips && w.hasSameGeometry(v) == false)
      return QString("the voxels of frame %1 have other positions").arg(f + 1);

    double maxDifference = 0.0, maxValue = 0.0;
    for(qint64 i = 0; i < v.voxelCount(); i++)
    {
      maxDifference = qMax(maxDifference, std::fabs(v.value(i) - w.value(i)));
      maxValue = qMax(maxValue, std::fabs(v.value(i)));
    }
    if(maxDifference > 1e-6 * maxValue)
      return QString("the values of frame %1 differ by %2 (max. value %3)").arg(f + 1).arg(maxDifference).arg(maxValue);
    if(std::fabs(w.frameStart() - v.frameStart()) > 1e-3 || std::fabs(w.frameDuration() - v.frameDuration()) > 1e-3)
      return QString("the timing of frame %1 differs").arg(f + 1);
  }

  // the decay factors of the frames, in the formats that store them
  if(format == CMedIOData::ECAT || format == CMedIOData::NIFTI || format == CMedIOData::HDF5)
  {
    QJsonObject from, to;
    if(source->readAttributes(from) == false || target->readAttributes(to) == false)
      return "cannot read the attributes";
    const QJsonArray sourceFrames = from.value("Frames").toArray();
    const QJsonArray targetFrames = to.value("Frames").toArray();
    for(int f = 0; f < sourceFrames.size(); f++)
    {
      const QJsonValue factor = sourceFrames.at(f).toObject().value("DecayFactor");
      if(factor.isDouble() && targetFrames.at(f).toObject().value("DecayFactor") != factor)
        return QString("DecayFactor of frame %1 differs").arg(f + 1);
    }
  }

  // an attribute every format stores
  QJsonObject a, b;
  if(source->readAttributes(a) == false || target->readAttributes(b) == false)
    return "cannot read the attributes";
  if(a.contains("PatientWeight") && a.value("PatientWeight") != b.value("PatientWeight"))
    return "PatientWeight differs";

  return QString();
}

} // namespace

class TestMedioConvert : public QObject
{
  Q_OBJECT

  private slots:
    void initTestCase();
    void help();
    void conversions_data();
    void conversions();
    void niftiOptions_data();
    void niftiOptions();
    void niftiOptionsOfOtherConversions();
    void defaultOutput();
    void formatOption();
    void existingOutput();
    void errors();
    void inputIsKept();
    void withoutHdf5Support();

  private:
    QString sourceOf(const Format& format) const;

    QTemporaryDir m_tmp;
    QString m_hdf5;   // the ECAT7 dataset converted into HDF5 (with HDF5 support)
    bool m_data;
};

QString TestMedioConvert::sourceOf(const Format& format) const
{
  return format.source ? medioTestDataDir() + "/" + format.source : m_hdf5;
}

void TestMedioConvert::initTestCase()
{
  QVERIFY(m_tmp.isValid());
  QVERIFY2(QFileInfo::exists(tool("medioConvert")), qPrintable(tool("medioConvert")));
  m_data = QFileInfo::exists(sourceOf(Formats[0]));

  if(m_data && CHDF5File::hasSupport())
  {
    m_hdf5 = m_tmp.filePath("source.h5");
    const Result result = run(tool("medioConvert"), QStringList() << sourceOf(Formats[0]) << m_hdf5);
    QVERIFY2(result.exitCode == 0, qPrintable(result.error));
  }
}

void TestMedioConvert::help()
{
  foreach(const QString& option, QStringList() << "-h" << "--help")
  {
    const Result result = run(tool("medioConvert"), QStringList() << option);
    QCOMPARE(result.exitCode, 0);
    QVERIFY(result.output.contains("<input> <output>") && result.output.contains("--format"));
    QCOMPARE(result.output.contains("(this one was)"), CHDF5File::hasSupport());
  }

  // the names <source>2<target> describe their conversion
  const Result alias = run(tool("philips2ecat"), QStringList() << "--help");
  QCOMPARE(alias.exitCode, 0);
  QVERIFY(alias.output.contains("<input> [output]"));
  QVERIFY(alias.output.contains("format philips into the format ecat"));
  QVERIFY(alias.output.contains("extension .v"));

  // without files: the usage and an error status
  QCOMPARE(run(tool("medioConvert"), QStringList()).exitCode, 1);
  QCOMPARE(run(tool("philips2ecat"), QStringList()).exitCode, 1);
}

// every format into every other format, with the program named after the conversion
// and with medioConvert itself
void TestMedioConvert::conversions_data()
{
  QTest::addColumn<int>("source");
  QTest::addColumn<int>("target");

  for(int s = 0; s < FormatCount; s++)
  {
    for(int t = 0; t < FormatCount; t++)
    {
      if(s != t)
        QTest::newRow(qPrintable(QString("%1 -> %2").arg(Formats[s].name, Formats[t].name))) << s << t;
    }
  }
}

void TestMedioConvert::conversions()
{
  QFETCH(int, source);
  QFETCH(int, target);
  if(m_data == false)
    QSKIP("no test data");
  const Format& s = Formats[source];
  const Format& t = Formats[target];
  const bool hdf5 = (s.format == CMedIOData::HDF5 || t.format == CMedIOData::HDF5);
  const QString alias = QString("%1 2%2").arg(s.name, t.name).remove(' ');

  // the names with hdf5 exist with HDF5 support only
  if(hdf5 && CHDF5File::hasSupport() == false)
  {
    QVERIFY(QFileInfo::exists(tool(alias)) == false);
    QSKIP("no HDF5 support");
  }

  const QString input = sourceOf(s);
  const QString output = m_tmp.filePath(alias + t.extension);
  Result result = run(tool(alias), QStringList() << input << output);
  QVERIFY2(result.exitCode == 0, qPrintable(result.error));
  QVERIFY(result.output.contains(" -> "));
  QCOMPARE(difference(input, output, t.format), QString());

  // medioConvert writes the same files
  const QString general = m_tmp.filePath(alias + "-general" + t.extension);
  result = run(tool("medioConvert"), QStringList() << input << general);
  QVERIFY2(result.exitCode == 0, qPrintable(result.error));
  QCOMPARE(difference(input, general, t.format), QString());
  if(t.format != CMedIOData::HDF5 && t.format != CMedIOData::Interfile)   // these store a time or the file name
    QCOMPARE(content(general), content(output));
}

// the options for NIfTI files of ecat2nifti and philips2nifti, which medioConvert takes
// as well: both write the same files
void TestMedioConvert::niftiOptions_data()
{
  QTest::addColumn<int>("source");
  QTest::addColumn<QStringList>("options");

  for(int s = 0; s <= 2; s += 2)
  {
    const QString name = Formats[s].name;
    QTest::newRow(qPrintable(name + ": none")) << s << QStringList();
    QTest::newRow(qPrintable(name + ": -2")) << s << (QStringList() << "-2");
    QTest::newRow(qPrintable(name + ": --float")) << s << (QStringList() << "--float");
    QTest::newRow(qPrintable(name + ": --normalize")) << s << (QStringList() << "--normalize");
    QTest::newRow(qPrintable(name + ": --sidecar")) << s << (QStringList() << "--sidecar");
    QTest::newRow(qPrintable(name + ": --no-extension")) << s << (QStringList() << "--no-extension");
    QTest::newRow(qPrintable(name + ": all")) << s << (QStringList() << "-2" << "--float" << "--sidecar" << "--no-extension");
  }
}

void TestMedioConvert::niftiOptions()
{
  QFETCH(int, source);
  QFETCH(QStringList, options);
  if(m_data == false)
    QSKIP("no test data");

  const Format& s = Formats[source];
  const QString input = sourceOf(s);
  const QString base = m_tmp.filePath(QString("options-%1-%2").arg(s.name, QString(QTest::currentDataTag()).section(' ', 1).remove('-')));
  const QString named = base + "-named.nii";
  const QString general = base + "-general.nii";
  QVERIFY(QDir().mkpath(base + "-default"));
  const QString copy = base + "-default/image" + s.extension;
  QVERIFY(QFile::copy(input, copy));

  QCOMPARE(run(tool(QString(s.name) + "2nifti"), options + (QStringList() << input << named)).exitCode, 0);
  QCOMPARE(run(tool("medioConvert"), options + (QStringList() << input << general)).exitCode, 0);

  const QByteArray file = content(named);
  QVERIFY(file.size() > 348);
  QCOMPARE(content(general), file);

  // the default output of the named program: the name of the input with .nii (the
  // metadata of the file names its source, so the same source is converted for that)
  QCOMPARE(run(tool(QString(s.name) + "2nifti"), options + (QStringList() << copy)).exitCode, 0);
  QCOMPARE(run(tool("medioConvert"), options + (QStringList() << copy << base + "-default/named.nii")).exitCode, 0);
  QVERIFY(content(base + "-default/image.nii").size() > 348);
  QCOMPARE(content(base + "-default/image.nii"), content(base + "-default/named.nii"));

  // what the options change: the NIfTI version (size of the header), the data type,
  // the sidecar file and the header extensions (the offset of the data)
  const bool nifti2 = options.contains("-2");
  const qint32 headerSize = *reinterpret_cast<const qint32*>(file.constData());
  QCOMPARE(headerSize, nifti2 ? 540 : 348);
  const qint16 datatype = *reinterpret_cast<const qint16*>(file.constData() + (nifti2 ? 12 : 70));
  if(options.contains("--float") || options.contains("--normalize"))
    QCOMPARE(int(datatype), 16);   // float32
  const QString sidecar = base + "-general.json";
  QCOMPARE(QFileInfo::exists(sidecar), options.contains("--sidecar"));
  QCOMPARE(QFileInfo::exists(base + "-named.json"), options.contains("--sidecar"));
  if(options.contains("--sidecar"))
    QCOMPARE(content(sidecar), content(base + "-named.json"));
  const qint64 offset = nifti2 ? *reinterpret_cast<const qint64*>(file.constData() + 168)
                               : qint64(*reinterpret_cast<const float*>(file.constData() + 108));
  if(options.contains("--no-extension"))
    QCOMPARE(offset, qint64(headerSize + 4));
  else
    QVERIFY(offset > headerSize + 4);

  // the file is converted back by both programs alike
  const QString back = base + "-back" + s.extension;
  const QString generalBack = base + "-general-back" + s.extension;
  QCOMPARE(run(tool(QString("nifti2") + s.name), QStringList() << named << back).exitCode, 0);
  QCOMPARE(run(tool("medioConvert"), QStringList() << named << generalBack).exitCode, 0);
  QVERIFY(content(back).isEmpty() == false);
  QCOMPARE(content(generalBack), content(back));
  // the frame timing is part of the metadata, which --no-extension leaves out
  if(options.contains("--no-extension") == false || options.contains("--sidecar"))
    QCOMPARE(difference(input, back, s.format), QString());
}

// the options are for ECAT7 and Philips files converted into NIfTI only
void TestMedioConvert::niftiOptionsOfOtherConversions()
{
  if(m_data == false)
    QSKIP("no test data");

  QDir(m_tmp.path()).mkdir("other");
  const QDir dir(m_tmp.filePath("other"));
  const QStringList options = QStringList() << "-2" << "--float" << "--sidecar" << "--no-extension";
  foreach(const QString& option, options)
  {
    // into another format, and from another format into NIfTI
    Result result = run(tool("medioConvert"), QStringList() << option << sourceOf(Formats[0]) << dir.filePath("a.img"));
    QCOMPARE(result.exitCode, 1);
    QVERIFY2(result.error.contains("for ECAT7 and Philips files converted into NIfTI only"), qPrintable(result.error));
    result = run(tool("medioConvert"), QStringList() << option << sourceOf(Formats[3]) << dir.filePath("a.nii"));
    QCOMPARE(result.exitCode, 1);
    QVERIFY2(result.error.contains("for ECAT7 and Philips files converted into NIfTI only"), qPrintable(result.error));
    // the names of the other conversions do not know them
    QCOMPARE(run(tool("nifti2ecat"), QStringList() << option << sourceOf(Formats[1]) << dir.filePath("a.v")).exitCode, 1);
    result = run(tool("interfile2nifti"), QStringList() << option << sourceOf(Formats[3]) << dir.filePath("a.nii"));
    QCOMPARE(result.exitCode, 1);
    QVERIFY2(result.error.contains("unknown option"), qPrintable(result.error));
  }
  QCOMPARE(dir.entryList(QDir::Files), QStringList());

  // the input is not replaced by its own conversion
  const QString file = dir.filePath("same.nii");
  QCOMPARE(run(tool("ecat2nifti"), QStringList() << sourceOf(Formats[0]) << file).exitCode, 0);
  const QByteArray before = content(file);
  const Result result = run(tool("medioConvert"), QStringList() << "-f" << "ecat" << file << file);
  QCOMPARE(result.exitCode, 1);
  QVERIFY2(result.error.contains("is a file of the input"), qPrintable(result.error));
  QCOMPARE(content(file), before);
}

// the output of <source>2<target> defaults to the name of the input with the extension of the target
void TestMedioConvert::defaultOutput()
{
  if(m_data == false)
    QSKIP("no test data");

  QDir(m_tmp.path()).mkdir("default");
  const QString input = m_tmp.filePath("default/image.nii.gz");
  QVERIFY(QFile::copy(sourceOf(Formats[1]), input));

  QCOMPARE(run(tool("nifti2interfile"), QStringList() << input).exitCode, 0);
  QCOMPARE(difference(input, m_tmp.filePath("default/image.hv"), CMedIOData::Interfile), QString());
  QVERIFY(QFileInfo::exists(m_tmp.filePath("default/image.v")));   // the data file of the header

  if(CHDF5File::hasSupport())
  {
    QCOMPARE(run(tool("nifti2hdf5"), QStringList() << input).exitCode, 0);
    QCOMPARE(difference(input, m_tmp.filePath("default/image.h5"), CMedIOData::HDF5), QString());
    QVERIFY(QFile::remove(m_tmp.filePath("default/image.v")) && QFile::remove(m_tmp.filePath("default/image.hv")));
    QCOMPARE(run(tool("hdf52ecat"), QStringList() << m_tmp.filePath("default/image.h5")).exitCode, 0);
    QCOMPARE(difference(input, m_tmp.filePath("default/image.v"), CMedIOData::ECAT), QString());
  }
}

// --format gives the format of a name without known extension
void TestMedioConvert::formatOption()
{
  if(m_data == false)
    QSKIP("no test data");

  const QString input = sourceOf(Formats[2]);
  const QStringList options = QStringList() << "-f" << "--format" << "--format=ecat";
  for(int i = 0; i < options.size(); i++)
  {
    const QString output = m_tmp.filePath(QString("format-%1.dat").arg(i));
    QStringList arguments = QStringList() << options[i];
    if(options[i].contains('=') == false)
      arguments << "ecat";
    const Result result = run(tool("medioConvert"), arguments << input << output);
    QVERIFY2(result.exitCode == 0, qPrintable(result.error));
    QCOMPARE(difference(input, output, CMedIOData::ECAT), QString());
  }

  // the option wins over the extension
  const QString output = m_tmp.filePath("format.nii");
  QCOMPARE(run(tool("medioConvert"), QStringList() << "-f" << "ecat" << input << output).exitCode, 0);
  QCOMPARE(CMedIODataFactory::identify(output), CMedIOData::ECAT);
}

// an existing output file is replaced, as by the other converters
void TestMedioConvert::existingOutput()
{
  if(m_data == false)
    QSKIP("no test data");

  foreach(const QString& name, QStringList() << "existing.v" << "existing.h5")
  {
    if(name.endsWith(".h5") && CHDF5File::hasSupport() == false)
      continue;
    const QString output = m_tmp.filePath(name);
    QFile file(output);
    QVERIFY(file.open(QIODevice::WriteOnly) && file.write("existing") == 8);
    file.close();
    const QString input = sourceOf(Formats[1]);
    QCOMPARE(run(tool("medioConvert"), QStringList() << input << output).exitCode, 0);
    QCOMPARE(difference(input, output, CMedIODataFactory::formatOfFileName(output)), QString());
  }
}

// what cannot be converted fails with a message and an error status, and writes no file
void TestMedioConvert::errors()
{
  if(m_data == false)
    QSKIP("no test data");

  QDir(m_tmp.path()).mkdir("errors");
  const QDir dir(m_tmp.filePath("errors"));
  const QString ecat = sourceOf(Formats[0]);
  const QString text = dir.filePath("text.v");
  QFile file(text);
  QVERIFY(file.open(QIODevice::WriteOnly) && file.write("no image") == 8);
  file.close();

  struct Case { const char* program; QStringList arguments; const char* message; };
  const Case cases[] = {
    { "medioConvert", QStringList() << dir.filePath("missing.v") << dir.filePath("a.nii"), "does not exist" },
    { "medioConvert", QStringList() << text << dir.filePath("a.nii"), "not a supported image file" },
    { "medioConvert", QStringList() << ecat << dir.filePath("a.xyz"), "unknown file format" },
    { "medioConvert", QStringList() << "-f" << "dicom" << ecat << dir.filePath("a.dat"), "is no format to write" },
    { "medioConvert", QStringList() << "-f" << "micropet" << ecat << dir.filePath("a.dat"), "is no format to write" },
    { "medioConvert", QStringList() << ecat << dir.filePath("a.nii") << "-f", "needs a format" },
    { "medioConvert", QStringList() << "--unknown" << ecat << dir.filePath("a.nii"), "unknown option" },
    { "medioConvert", QStringList() << ecat << dir.filePath("missing/a.nii"), "unable to open" },
    { "medioConvert", QStringList() << ecat << dir.filePath("missing/a.hv"), "unable to create" },
    // the input has to be a file of the source format of the name
    { "philips2ecat", QStringList() << ecat << dir.filePath("a.v"), "is no philips file (it is a ecat file)" },
    { "philips2ecat", QStringList() << "--float" << ecat, "unknown option" },
    { "philips2ecat", QStringList() << dir.filePath("missing.img"), "does not exist" }
  };
  for(unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
  {
    const Result result = run(tool(cases[i].program), cases[i].arguments);
    QVERIFY2(result.exitCode == 1, qPrintable(cases[i].arguments.join(' ')));
    QVERIFY2(result.error.contains(cases[i].message), qPrintable(result.error));
  }
  QCOMPARE(dir.entryList(QDir::Files), QStringList() << "text.v");

  // too many or too few files
  QCOMPARE(run(tool("medioConvert"), QStringList() << ecat).exitCode, 1);
  QCOMPARE(run(tool("medioConvert"), QStringList() << ecat << dir.filePath("a.nii") << dir.filePath("b.nii")).exitCode, 1);
  QCOMPARE(dir.entryList(QDir::Files), QStringList() << "text.v");
}

// the output must not be a file of the input: the default output of an Interfile header
// .hv converted into ECAT7 would be its own data file .v
void TestMedioConvert::inputIsKept()
{
  if(m_data == false)
    QSKIP("no test data");

  QDir(m_tmp.path()).mkdir("kept");
  const QString header = m_tmp.filePath("kept/image.hv");
  const QString data = m_tmp.filePath("kept/image.v");
  QCOMPARE(run(tool("medioConvert"), QStringList() << sourceOf(Formats[0]) << header).exitCode, 0);
  QFile file(data);
  QVERIFY(file.open(QIODevice::ReadOnly));
  const QByteArray content = file.readAll();
  file.close();

  Result result = run(tool("interfile2ecat"), QStringList() << header);
  QCOMPARE(result.exitCode, 1);
  QVERIFY2(result.error.contains("is a file of the input"), qPrintable(result.error));
  result = run(tool("medioConvert"), QStringList() << "-f" << "nifti" << header << header);
  QCOMPARE(result.exitCode, 1);
  QVERIFY2(result.error.contains("is a file of the input"), qPrintable(result.error));

  QVERIFY(file.open(QIODevice::ReadOnly));
  QCOMPARE(file.readAll(), content);
  QCOMPARE(difference(sourceOf(Formats[0]), header, CMedIOData::Interfile), QString());
}

// without HDF5 support, HDF5 files are refused with a message saying so
void TestMedioConvert::withoutHdf5Support()
{
  if(CHDF5File::hasSupport())
    QSKIP("HDF5 support");
  if(m_data == false)
    QSKIP("no test data");

  const QString output = m_tmp.filePath("none.h5");
  Result result = run(tool("medioConvert"), QStringList() << sourceOf(Formats[0]) << output);
  QCOMPARE(result.exitCode, 1);
  QVERIFY2(result.error.contains("without HDF5 support"), qPrintable(result.error));
  QVERIFY(QFileInfo::exists(output) == false);

  // an HDF5 file (its signature) as input
  QFile file(output);
  QVERIFY(file.open(QIODevice::WriteOnly) && file.write(QByteArray("\x89HDF\r\n\x1a\n", 8) + QByteArray(504, '\0')) == 512);
  file.close();
  result = run(tool("medioConvert"), QStringList() << output << m_tmp.filePath("none.v"));
  QCOMPARE(result.exitCode, 1);
  QVERIFY2(result.error.contains("is an HDF5 file") && result.error.contains("without HDF5 support"), qPrintable(result.error));
  QVERIFY(QFileInfo::exists(m_tmp.filePath("none.v")) == false);
}

QTEST_GUILESS_MAIN(TestMedioConvert)
#include "testMedioConvert.moc"
