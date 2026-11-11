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

// Tests of CNIFTIConverter (ECAT7 <-> NIfTI) using the datasets in
// testsuite/data (see testsuite/data/README.md) and synthetic NIfTI files.
//
// The expected values of the dataset files are generated with nibabel/numpy
// (make_expected.py), the geometry is checked against the convention
// documented in CNIFTIConverter.h:
//
//   ECAT7 voxel (i, j, k) is located at (LPS, mm)
//     x = x_offset + (i - (nx-1)/2) * px
//     y = y_offset + (j - (ny-1)/2) * py
//     z = bed position + z_offset - k * pz
//   and is stored as NIfTI voxel (i, j, nz-1-k).

#include <CNIFTIConverter.h>
#include <CNIFTIFile.h>
#include <CNIFTIMainHeader.h>
#include <CNIFTI1MainHeader.h>
#include <CNIFTI2MainHeader.h>
#include <CECATFile.h>
#include <CECATMainHeader.h>
#include <CECATSubHeader.h>
#include <CECAT7MainHeader.h>
#include <CECAT7SubHeaderImage.h>
#include <CPhilipsFile.h>
#include <CPhilipsMainHeader.h>
#include <CPhilipsSubHeader.h>
#include <CPhilipsSubHeaderImage.h>
#include <CMedIODataFactory.h>

#include "MedIOTestData.h"

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QJsonArray>
#include <QJsonDocument>

#include <cmath>

//=============================================================================================
// ECAT7 files

struct EcatFrame
{
  int dataType;
  double scale;
  unsigned int startMs;
  unsigned int durationMs;
  double reconZoom;
};

struct EcatVolume
{
  int nx, ny, nz, frames;
  double px, py, pz;          // mm
  double xOffset, yOffset;    // mm
  double zPosition;           // bed position + z offset (mm)
  double planeSeparation;     // mm
  double calibration;
  int patientOrientation;
  QString patientName, isotope, dataUnits;
  double dosage;
  QList<EcatFrame> frameInfo;
  QVector<double> values;     // stored x scale x calibration, frame by frame

  double value(int i, int j, int k, int f) const
  { return values[i + nx * (j + static_cast<qint64>(ny) * (k + static_cast<qint64>(nz) * f))]; }

  // LPS position of voxel (i, j, k) following the documented convention
  void position(double i, double j, double k, double p[3]) const
  {
    p[0] = xOffset + (i - (nx - 1) / 2.0) * px;
    p[1] = yOffset + (j - (ny - 1) / 2.0) * py;
    p[2] = zPosition - k * pz;
  }
};

static double ecatValue(const char* data, qint64 i, int type)
{
  switch(type) {
    case CECATSubHeader::ByteData:  return reinterpret_cast<const quint8*>(data)[i];
    case CECATSubHeader::SunLong:   return reinterpret_cast<const qint32*>(data)[i];
    case CECATSubHeader::IEEEFloat: return reinterpret_cast<const float*>(data)[i];
  }
  return reinterpret_cast<const qint16*>(data)[i];
}

static bool readEcat(const QString& fileName, EcatVolume& v)
{
  CECATFile file(fileName);
  if(file.open(QIODevice::ReadOnly) == false)
    return false;

  CECATMainHeader* mh = NULL;
  if(file.readMainHeader(mh) == false || mh == NULL)
    return false;
  QScopedPointer<CECATMainHeader> mhGuard(mh);
  if(mh->mainHeaderType() != CECATMainHeader::ECAT7MainHeader)
    return false;
  const CECAT7MainHeader* main = static_cast<const CECAT7MainHeader*>(mh);

  v.frames = qMax(1, static_cast<int>(main->num_Frames()));
  v.calibration = main->calibration_Factor() > 0 ? main->calibration_Factor() : 1.0;
  v.planeSeparation = main->plane_Separation() * 10.0;
  v.patientOrientation = main->patient_Orientation();
  v.patientName = main->patient_Name();
  v.isotope = main->isotope_Name();
  v.dataUnits = main->data_Units();
  v.dosage = main->dosage();
  v.values.clear();
  v.frameInfo.clear();

  for(int f=1; f <= v.frames; f++) {
    QByteArray* data = NULL;
    CECATSubHeader* sh = NULL;
    if(file.readMatrix(data, sh, f) == false || data == NULL || sh == NULL)
      return false;
    QScopedPointer<QByteArray> dataGuard(data);
    QScopedPointer<CECATSubHeader> shGuard(sh);
    if(sh->subHeaderType() != CECATSubHeader::ECAT7_Image)
      return false;
    const CECAT7SubHeaderImage* s = static_cast<const CECAT7SubHeaderImage*>(sh);

    if(f == 1) {
      v.nx = s->x_Dimension(); v.ny = s->y_Dimension(); v.nz = s->z_Dimension();
      v.px = s->x_Pixel_Size() * 10.0; v.py = s->y_Pixel_Size() * 10.0; v.pz = s->z_Pixel_Size() * 10.0;
      v.xOffset = s->x_Offset() * 10.0; v.yOffset = s->y_Offset() * 10.0;
      v.zPosition = (main->init_Bed_Position() + s->z_Offset()) * 10.0;
    }

    EcatFrame info = { s->data_Type(), s->scale_Factor(), s->frame_Start_Time(), s->frame_Duration(), s->recon_Zoom() };
    v.frameInfo.append(info);

    const double scale = (info.scale != 0.0 ? info.scale : 1.0) * v.calibration;
    const qint64 n = static_cast<qint64>(v.nx) * v.ny * v.nz;
    for(qint64 i=0; i < n; i++)
      v.values.append(ecatValue(data->constData(), i, info.dataType) * scale);
  }

  return true;
}

//=============================================================================================
// NIfTI files

struct NiftiVolume
{
  int version;
  qint64 dim[8];
  double pixdim[8];
  int datatype, xyztUnits;
  double slope, inter, toffset;
  int qformCode, sformCode;
  double srow[3][4];
  double voxOffset;
  QJsonObject json;
  QVector<double> values;     // stored x slope + inter

  double value(qint64 i, qint64 j, qint64 k, qint64 f) const
  { return values[i + dim[1] * (j + dim[2] * (k + dim[3] * f))]; }

  // RAS position of voxel (i, j, k) using the sform
  void position(double i, double j, double k, double p[3]) const
  {
    for(int r=0; r < 3; r++)
      p[r] = srow[r][0] * i + srow[r][1] * j + srow[r][2] * k + srow[r][3];
  }
};

static double niftiValue(const char* data, qint64 i, int type)
{
  switch(type) {
    case 2:   return reinterpret_cast<const quint8*>(data)[i];
    case 256: return reinterpret_cast<const qint8*>(data)[i];
    case 4:   return reinterpret_cast<const qint16*>(data)[i];
    case 512: return reinterpret_cast<const quint16*>(data)[i];
    case 8:   return reinterpret_cast<const qint32*>(data)[i];
    case 16:  return reinterpret_cast<const float*>(data)[i];
    case 64:  return reinterpret_cast<const double*>(data)[i];
  }
  return std::nan("");
}

template <class H>
static void niftiFields(H* h, CNIFTIFile& file, NiftiVolume& v)
{
  for(short i=0; i < 8; i++) {
    v.dim[i] = h->dim(i);
    v.pixdim[i] = h->pix_Dim(i);
  }
  v.datatype = h->dataType();
  v.xyztUnits = h->xyzt_Units();
  v.slope = h->scl_Slope();
  v.inter = h->scl_Inter();
  if(v.slope == 0.0) {
    v.slope = 1.0;
    v.inter = 0.0;
  }
  v.toffset = h->toffset();
  v.qformCode = h->qform_Code();
  v.sformCode = h->sform_Code();
  for(short i=0; i < 4; i++) {
    v.srow[0][i] = h->srow_X(i);
    v.srow[1][i] = h->srow_Y(i);
    v.srow[2][i] = h->srow_Z(i);
  }
  v.voxOffset = h->vox_Offset();
  v.json = h->readHeaderExtension(file);
}

static bool readNifti(const QString& fileName, NiftiVolume& v)
{
  CNIFTIFile file(fileName);
  if(file.open(QIODevice::ReadOnly) == false)
    return false;

  CNIFTIMainHeader* h = NULL;
  if(file.readMainHeader(h) == false || h == NULL)
    return false;
  QScopedPointer<CNIFTIMainHeader> guard(h);

  QByteArray* data = NULL;
  if(file.readMatrix(data) == false || data == NULL)
    return false;
  QScopedPointer<QByteArray> dataGuard(data);

  if(h->mainHeaderType() == CNIFTIMainHeader::NIFTI1MainHeader) {
    v.version = 1;
    niftiFields(static_cast<CNIFTI1MainHeader*>(h), file, v);
  } else {
    v.version = 2;
    niftiFields(static_cast<CNIFTI2MainHeader*>(h), file, v);
  }

  qint64 n = 1;
  for(int i=1; i <= v.dim[0]; i++)
    n *= v.dim[i];
  v.values.resize(n);
  for(qint64 i=0; i < n; i++)
    v.values[i] = niftiValue(data->constData(), i, v.datatype) * v.slope + v.inter;

  return true;
}

// maximum absolute value of a frame
static double maxAbs(const QVector<double>& values, qint64 offset, qint64 count)
{
  double m = 0.0;
  for(qint64 i=0; i < count; i++)
    m = qMax(m, std::fabs(values[offset + i]));
  return m;
}

// the file to open for a test data entry: the .hdr file of a pair directory
static QString niftiPath(const MedIOTestFile& file)
{
  if(QFileInfo(file.path).isDir() == false)
    return file.path;

  const QStringList headers = QDir(file.path).entryList(QStringList() << "*.hdr" << "*.HDR" << "*.hdr.gz", QDir::Files);
  return headers.size() == 1 ? QDir(file.path).filePath(headers.first()) : QString();
}

// header/image pairs are identified by the file name
static bool pairName(const QString& name)
{
  QString n = name.toLower();
  if(n.endsWith(".gz"))
    n.chop(3);
  return n.endsWith(".hdr") || n.endsWith(".img");
}

static QJsonArray landmarks(const MedIOTestFile& file)
{
  return file.datasetInfo.value("landmarks").toArray();
}

static bool landmarkVoxel(const QJsonObject& landmark, const QString& file, int v[3])
{
  const QJsonArray a = landmark.value("voxel").toObject().value(file).toArray();
  if(a.size() != 3)
    return false;
  for(int i=0; i < 3; i++)
    v[i] = a[i].toInt();
  return true;
}

//=============================================================================================

class TestNIFTIConversion : public QObject
{
  Q_OBJECT

  private:
    QTemporaryDir m_tmp;
    MedIOTestData m_data;

    QString tmpFile(const QString& name) const { return m_tmp.filePath(name); }
    void addEcatRows(bool withVariants);
    QString writeSyntheticNifti(const QString& name, const double affine[3][4], bool useQform, int units,
                                int frames, double pixdim4, double toffset);

  private slots:
    void initTestCase();

    void ecatToNifti_data();
    void ecatToNifti();

    void ecatToNiftiRaw_data();
    void ecatToNiftiRaw();

    void roundTrip_data();
    void roundTrip();

    void niftiToEcatReference_data();
    void niftiToEcatReference();

    void niftiToEcatOrientation_data();
    void niftiToEcatOrientation();

    void philipsToNifti_data();
    void philipsToNifti();

    void philipsRoundTrip_data();
    void philipsRoundTrip();
    void philipsAttributesRoundTrip_data();
    void philipsAttributesRoundTrip();

    void philipsSliceScales_data();
    void philipsSliceScales();
    void philipsHeaderConversion();

    void metadataOptions();
    void errors();
};

void TestNIFTIConversion::initTestCase()
{
  QVERIFY(m_tmp.isValid());
  m_data = medioTestData();
  foreach(const QString& pending, m_data.pending)
    qInfo("pending test data file: %s", qPrintable(pending));
}

void TestNIFTIConversion::addEcatRows(bool withVariants)
{
  QTest::addColumn<QString>("ecat");
  QTest::addColumn<int>("version");
  QTest::addColumn<bool>("floatData");
  QTest::addColumn<QString>("suffix");

  bool any = false;
  foreach(const MedIOTestFile& file, m_data.files) {
    if(file.format != "ecat")
      continue;
    any = true;

    QTest::newRow(qPrintable(file.relativePath + " nifti1")) << file.path << 1 << false << ".nii";
    if(withVariants) {
      QTest::newRow(qPrintable(file.relativePath + " nifti2 gz")) << file.path << 2 << false << ".nii.gz";
      QTest::newRow(qPrintable(file.relativePath + " nifti1 float gz")) << file.path << 1 << true << ".nii.gz";
      QTest::newRow(qPrintable(file.relativePath + " nifti2 float")) << file.path << 2 << true << ".nii";
      QTest::newRow(qPrintable(file.relativePath + " nifti1 pair")) << file.path << 1 << false << ".hdr";
      QTest::newRow(qPrintable(file.relativePath + " nifti2 float pair gz")) << file.path << 2 << true << ".img.gz";
    }
  }

  if(any == false)
    QTest::newRow("no ECAT7 test data") << QString() << 1 << false << ".nii";
}

//---------------------------------------------------------------------------------------------
// ECAT7 -> NIfTI: header, values, geometry and metadata

void TestNIFTIConversion::ecatToNifti_data()
{
  addEcatRows(true);
}

void TestNIFTIConversion::ecatToNifti()
{
  QFETCH(QString, ecat);
  QFETCH(int, version);
  QFETCH(bool, floatData);
  QFETCH(QString, suffix);
  if(ecat.isEmpty())
    QSKIP("no ECAT7 files in the test data");

  const MedIOTestFile* testFile = NULL;
  foreach(const MedIOTestFile& f, m_data.files)
    if(f.path == ecat)
      testFile = &f;
  QVERIFY(testFile);

  EcatVolume e;
  QVERIFY(readEcat(ecat, e));

  CNIFTIConverter::Options options;
  options.niftiVersion = version;
  options.floatData = floatData;

  const QString out = tmpFile(QFileInfo(ecat).fileName() + QString(".v%1%2").arg(version).arg(floatData ? "f" : "") + suffix);
  QString error;
  QVERIFY2(CNIFTIConverter::ecatToNifti(ecat, out, options, &error), qPrintable(error));

  NiftiVolume n;
  QVERIFY(readNifti(out, n));

  // header
  QCOMPARE(n.version, version);
  QCOMPARE(n.dim[0], qint64(e.frames > 1 ? 4 : 3));
  QCOMPARE(n.dim[1], qint64(e.nx));
  QCOMPARE(n.dim[2], qint64(e.ny));
  QCOMPARE(n.dim[3], qint64(e.nz));
  if(e.frames > 1)
    QCOMPARE(n.dim[4], qint64(e.frames));
  QCOMPARE(n.xyztUnits, 10);
  QCOMPARE(n.qformCode, 1);
  QCOMPARE(n.sformCode, 1);

  bool sameScale = true;
  foreach(const EcatFrame& f, e.frameInfo)
    sameScale = sameScale && f.scale == e.frameInfo.first().scale && f.dataType == e.frameInfo.first().dataType;

  if(floatData || sameScale == false) {
    QCOMPARE(n.datatype, 16);
    QCOMPARE(n.slope, 1.0);
  } else {
    QCOMPARE(n.datatype, 4);
    QVERIFY(std::fabs(n.slope - e.frameInfo.first().scale * e.calibration) <= 1e-6 * n.slope);
  }
  QCOMPARE(n.inter, 0.0);

  // values: identical to the ECAT7 values and to the independently computed sums
  const qint64 volume = static_cast<qint64>(e.nx) * e.ny * e.nz;
  const QJsonArray matrices = testFile->expected.value("matrices").toArray();
  QCOMPARE(matrices.size(), e.frames);

  for(int f=0; f < e.frames; f++) {
    const double tol = 1e-6 * maxAbs(e.values, f * volume, volume);
    double sum = 0.0;
    for(int k=0; k < e.nz; k++)
      for(int j=0; j < e.ny; j++)
        for(int i=0; i < e.nx; i++) {
          const double ev = e.value(i, j, k, f);
          const double nv = n.value(i, j, e.nz - 1 - k, f);
          if(std::fabs(ev - nv) > tol)
            QFAIL(qPrintable(QString("frame %1 voxel (%2,%3,%4): ECAT %5, NIfTI %6").arg(f+1).arg(i).arg(j).arg(k).arg(ev).arg(nv)));
          sum += nv;
        }

    const QJsonObject m = matrices[f].toObject();
    const double expectedSum = m.value("raw_sum").toDouble() * m.value("scale_factor").toDouble() *
                               testFile->expected.value("main_header").toObject().value("calibration_factor").toDouble(1.0);
    QVERIFY2(std::fabs(sum - expectedSum) <= 1e-5 * std::fabs(expectedSum) + tol,
             qPrintable(QString("frame %1: sum %2, expected %3").arg(f+1).arg(sum, 0, 'g', 12).arg(expectedSum, 0, 'g', 12)));
  }

  // geometry: affine of the documented convention
  for(int i=0; i < 3; i++)
    for(int j=0; j < 3; j++) {
      const double diag[3] = { -e.px, -e.py, e.pz };
      QVERIFY(std::fabs(n.srow[i][j] - (i == j ? diag[i] : 0.0)) < 1e-4);
    }

  for(int k=0; k < e.nz; k += e.nz - 1)
    for(int j=0; j < e.ny; j += e.ny - 1)
      for(int i=0; i < e.nx; i += e.nx - 1) {
        double lps[3], ras[3];
        e.position(i, j, k, lps);
        n.position(i, j, e.nz - 1 - k, ras);
        QVERIFY(std::fabs(-ras[0] - lps[0]) < 1e-3);
        QVERIFY(std::fabs(-ras[1] - lps[1]) < 1e-3);
        QVERIFY(std::fabs( ras[2] - lps[2]) < 1e-3);
      }

  // geometry: distances between the landmarks (the ECAT7 files of the test
  // data do not store the absolute position of the cropped volume)
  const QJsonArray lm = landmarks(*testFile);
  for(int a=0; a < lm.size(); a++)
    for(int b=a+1; b < lm.size(); b++) {
      int va[3], vb[3];
      if(landmarkVoxel(lm[a].toObject(), testFile->relativePath, va) == false ||
         landmarkVoxel(lm[b].toObject(), testFile->relativePath, vb) == false)
        continue;
      double pa[3], pb[3];
      n.position(va[0], va[1], e.nz - 1 - va[2], pa);
      n.position(vb[0], vb[1], e.nz - 1 - vb[2], pb);
      const QJsonArray wa = lm[a].toObject().value("world_mm").toArray();
      const QJsonArray wb = lm[b].toObject().value("world_mm").toArray();
      const double sign[3] = { -1.0, -1.0, 1.0 }; // RAS -> LPS
      for(int i=0; i < 3; i++) {
        const double d = sign[i] * (pb[i] - pa[i]);
        const double expected = wb[i].toDouble() - wa[i].toDouble();
        QVERIFY2(std::fabs(d - expected) < 0.1,
                 qPrintable(QString("landmark distance axis %1: %2 mm, expected %3 mm").arg(i).arg(d).arg(expected)));
      }
    }

  // metadata in the JSON header extension
  QVERIFY(n.json.isEmpty() == false);
  if(pairName(suffix)) {
    // the extension ends the header file, the voxel data starts the image file
    QCOMPARE(n.voxOffset, 0.0);
  } else {
    QVERIFY(n.voxOffset > (version == 2 ? 544 : 352));
    QCOMPARE(std::fmod(n.voxOffset, 16.0), 0.0);
  }
  // the attributes (expected values converted from the nibabel header, see
  // make_expected.py) and the native fields of the ECAT7 main header
  const QJsonObject expectedAttributes = testFile->expected.value("attributes").toObject();
  const QJsonObject expectedMain = expectedAttributes.value("main").toObject();
  QVERIFY(expectedMain.isEmpty() == false);
  foreach(const QString& key, expectedMain.keys()) {
    if(expectedMain[key].isDouble())
      QVERIFY2(std::fabs(n.json.value(key).toDouble() - expectedMain[key].toDouble()) <= 1e-9 * std::fabs(expectedMain[key].toDouble()), qPrintable(key));
    else
      QVERIFY2(n.json.value(key) == expectedMain[key], qPrintable(key));
  }
  const QJsonObject ecatFields = n.json.value("ECAT7").toObject();
  QVERIFY(ecatFields.contains("Patient_Name"));
  QVERIFY(ecatFields.contains("Isotope_Name"));
  QCOMPARE(ecatFields.value("Data_Units").toString(), testFile->expected.value("main_header").toObject().value("data_units").toString());

  // frame attributes of all frames (sub headers)
  const QJsonArray expectedFrames = expectedAttributes.value("frames").toArray();
  const QJsonArray frameAttributes = n.json.value("Frames").toArray();
  QCOMPARE(frameAttributes.size(), e.frames);
  for(int f=0; f < e.frames; f++) {
    QCOMPARE(frameAttributes[f].toObject().value("DecayFactor"), expectedFrames[f].toObject().value("DecayFactor"));
    QVERIFY(frameAttributes[f].toObject().value("ECAT7").toObject().contains("Annotation"));
  }

  const QJsonArray start = n.json.value("FrameTimesStart").toArray();
  const QJsonArray duration = n.json.value("FrameDuration").toArray();
  QCOMPARE(start.size(), e.frames);
  QCOMPARE(duration.size(), e.frames);
  for(int f=0; f < e.frames; f++) {
    const QJsonObject m = matrices[f].toObject();
    QCOMPARE(qRound(start[f].toDouble() * 1000.0), m.value("frame_start_time_ms").toInt());
    QCOMPARE(qRound(duration[f].toDouble() * 1000.0), m.value("frame_duration_ms").toInt());
  }

  bool sameDuration = true;
  foreach(const EcatFrame& f, e.frameInfo)
    sameDuration = sameDuration && f.durationMs == e.frameInfo.first().durationMs;
  if(e.frames > 1)
    QCOMPARE(n.pixdim[4], sameDuration ? e.frameInfo.first().durationMs / 1000.0 : 0.0);
}

//---------------------------------------------------------------------------------------------
// ECAT7 -> NIfTI: voxel by voxel against the raw files of the dataset

void TestNIFTIConversion::ecatToNiftiRaw_data()
{
  QTest::addColumn<QString>("ecat");
  QTest::addColumn<QString>("raw");

  bool any = false;
  foreach(const MedIOTestFile& ecat, m_data.files) {
    if(ecat.format != "ecat")
      continue;
    foreach(const MedIOTestFile& raw, m_data.files) {
      if(raw.format == "raw" && raw.dataset == ecat.dataset) {
        QTest::newRow(qPrintable(ecat.relativePath + " vs " + raw.relativePath)) << ecat.relativePath << raw.relativePath;
        any = true;
      }
    }
  }

  if(any == false)
    QTest::newRow("no ECAT7/raw test data") << QString() << QString();
}

void TestNIFTIConversion::ecatToNiftiRaw()
{
  QFETCH(QString, ecat);
  QFETCH(QString, raw);
  if(ecat.isEmpty())
    QSKIP("no ECAT7 and raw files of the same dataset in the test data");

  const MedIOTestFile *ecatFile = NULL, *rawFile = NULL;
  foreach(const MedIOTestFile& f, m_data.files) {
    if(f.relativePath == ecat) ecatFile = &f;
    if(f.relativePath == raw) rawFile = &f;
  }
  QVERIFY(ecatFile && rawFile);

  const QJsonObject layout = rawFile->fileInfo.value("layout").toObject();
  if(layout.value("data_type").toString() != "float32" || layout.value("byte_order").toString() != "little")
    QSKIP("only little endian float32 raw files are supported by this test");

  // the raw files must use the voxel order of the ECAT7 file
  foreach(const QJsonValue& l, landmarks(*ecatFile)) {
    int ve[3], vr[3];
    if(landmarkVoxel(l.toObject(), ecat, ve) && landmarkVoxel(l.toObject(), raw, vr))
      QVERIFY2(ve[0] == vr[0] && ve[1] == vr[1] && ve[2] == vr[2], "raw file does not use the ECAT7 voxel order");
  }

  CNIFTIConverter::Options options;
  options.floatData = true;
  const QString out = tmpFile(QFileInfo(ecat).fileName() + ".raw.nii");
  QString error;
  QVERIFY2(CNIFTIConverter::ecatToNifti(ecatFile->path, out, options, &error), qPrintable(error));

  NiftiVolume n;
  QVERIFY(readNifti(out, n));

  const QJsonArray dims = layout.value("dims").toArray();
  QCOMPARE(dims.size(), 3);
  QCOMPARE(qint64(dims[0].toInt()), n.dim[1]);
  QCOMPARE(qint64(dims[1].toInt()), n.dim[2]);
  QCOMPARE(qint64(dims[2].toInt()), n.dim[3]);

  const int frame = rawFile->fileInfo.value("frame").toInt(1) - 1;
  QVERIFY(frame >= 0 && (frame == 0 || frame < n.dim[4]));

  QFile file(rawFile->path);
  QVERIFY(file.open(QIODevice::ReadOnly));
  const QByteArray bytes = file.readAll();
  const qint64 volume = n.dim[1] * n.dim[2] * n.dim[3];
  QCOMPARE(qint64(bytes.size()), volume * 4);
  QVERIFY(QSysInfo::ByteOrder == QSysInfo::LittleEndian);
  const float* values = reinterpret_cast<const float*>(bytes.constData());

  double m = 0.0;
  for(qint64 i=0; i < volume; i++)
    m = qMax(m, std::fabs(double(values[i])));

  for(qint64 k=0; k < n.dim[3]; k++)
    for(qint64 j=0; j < n.dim[2]; j++)
      for(qint64 i=0; i < n.dim[1]; i++) {
        const double r = values[i + n.dim[1] * (j + n.dim[2] * k)];
        const double v = n.value(i, j, n.dim[3] - 1 - k, frame);
        if(std::fabs(r - v) > 1e-6 * m)
          QFAIL(qPrintable(QString("voxel (%1,%2,%3): raw %4, NIfTI %5").arg(i).arg(j).arg(k).arg(r).arg(v)));
      }
}

//---------------------------------------------------------------------------------------------
// ECAT7 -> NIfTI -> ECAT7

void TestNIFTIConversion::roundTrip_data()
{
  addEcatRows(true);
}

void TestNIFTIConversion::roundTrip()
{
  QFETCH(QString, ecat);
  QFETCH(int, version);
  QFETCH(bool, floatData);
  QFETCH(QString, suffix);
  if(ecat.isEmpty())
    QSKIP("no ECAT7 files in the test data");

  CNIFTIConverter::Options options;
  options.niftiVersion = version;
  options.floatData = floatData;

  const QString base = tmpFile(QFileInfo(ecat).fileName() + QString(".rt%1%2").arg(version).arg(floatData ? "f" : ""));
  QString error;
  QVERIFY2(CNIFTIConverter::ecatToNifti(ecat, base + suffix, options, &error), qPrintable(error));
  QVERIFY2(CNIFTIConverter::niftiToEcat(base + suffix, base + ".v", &error), qPrintable(error));

  EcatVolume a, b;
  QVERIFY(readEcat(ecat, a));
  QVERIFY(readEcat(base + ".v", b));

  // main header
  QCOMPARE(b.frames, a.frames);
  QCOMPARE(b.calibration, 1.0);
  QCOMPARE(b.patientOrientation, a.patientOrientation);
  QCOMPARE(b.patientName, a.patientName);
  QCOMPARE(b.isotope, a.isotope);
  QCOMPARE(b.dataUnits, a.dataUnits);
  QCOMPARE(b.dosage, a.dosage);
  QVERIFY(std::fabs(b.planeSeparation - a.planeSeparation) < 1e-4);

  // geometry
  QCOMPARE(b.nx, a.nx);
  QCOMPARE(b.ny, a.ny);
  QCOMPARE(b.nz, a.nz);
  QVERIFY(std::fabs(b.px - a.px) < 1e-4);
  QVERIFY(std::fabs(b.py - a.py) < 1e-4);
  QVERIFY(std::fabs(b.pz - a.pz) < 1e-4);
  QVERIFY(std::fabs(b.xOffset - a.xOffset) < 1e-3);
  QVERIFY(std::fabs(b.yOffset - a.yOffset) < 1e-3);
  QVERIFY(std::fabs(b.zPosition - a.zPosition) < 1e-3);

  // frames and values
  const qint64 volume = static_cast<qint64>(a.nx) * a.ny * a.nz;
  for(int f=0; f < a.frames; f++) {
    QCOMPARE(b.frameInfo[f].startMs, a.frameInfo[f].startMs);
    QCOMPARE(b.frameInfo[f].durationMs, a.frameInfo[f].durationMs);
    QCOMPARE(b.frameInfo[f].reconZoom, a.frameInfo[f].reconZoom);
    QCOMPARE(b.frameInfo[f].dataType, int(CECATSubHeader::SunShort));

    // int16 data is kept, float data is quantized to int16 again
    const double m = maxAbs(a.values, f * volume, volume);
    const double tol = 0.5 * b.frameInfo[f].scale + 1e-6 * m;
    for(qint64 i=0; i < volume; i++) {
      const double d = std::fabs(a.values[f * volume + i] - b.values[f * volume + i]);
      if(d > tol)
        QFAIL(qPrintable(QString("frame %1 voxel %2: %3 instead of %4").arg(f+1).arg(i).arg(b.values[f * volume + i]).arg(a.values[f * volume + i])));
    }
  }
}

//---------------------------------------------------------------------------------------------
// NIfTI files of other tools -> ECAT7, compared with the ECAT7 file of the dataset

void TestNIFTIConversion::niftiToEcatReference_data()
{
  QTest::addColumn<QString>("nifti");
  QTest::addColumn<QString>("ecat");

  bool any = false;
  foreach(const MedIOTestFile& nifti, m_data.files) {
    if(nifti.format != "nifti")
      continue;
    foreach(const MedIOTestFile& ecat, m_data.files) {
      if(ecat.format == "ecat" && ecat.dataset == nifti.dataset) {
        QTest::newRow(qPrintable(nifti.relativePath)) << nifti.relativePath << ecat.relativePath;
        any = true;
      }
    }
  }

  if(any == false)
    QTest::newRow("no NIfTI/ECAT7 test data") << QString() << QString();
}

void TestNIFTIConversion::niftiToEcatReference()
{
  QFETCH(QString, nifti);
  QFETCH(QString, ecat);
  if(nifti.isEmpty())
    QSKIP("no NIfTI and ECAT7 files of the same dataset in the test data");

  const MedIOTestFile *niftiFile = NULL, *ecatFile = NULL;
  foreach(const MedIOTestFile& f, m_data.files) {
    if(f.relativePath == nifti) niftiFile = &f;
    if(f.relativePath == ecat) ecatFile = &f;
  }
  QVERIFY(niftiFile && ecatFile);

  const QString in = niftiPath(*niftiFile);
  QVERIFY2(in.isEmpty() == false, "a NIfTI pair directory has to contain exactly one .hdr file");

  const QString out = tmpFile(QFileInfo(nifti).fileName() + ".v");
  QString error;
  QVERIFY2(CNIFTIConverter::niftiToEcat(in, out, &error), qPrintable(error));

  EcatVolume ref, conv;
  QVERIFY(readEcat(ecatFile->path, ref));
  QVERIFY(readEcat(out, conv));
  NiftiVolume n;
  QVERIFY(readNifti(in, n));

  QCOMPARE(conv.frames, ref.frames);
  QCOMPARE(conv.nx, ref.nx);
  QCOMPARE(conv.ny, ref.ny);
  QCOMPARE(conv.nz, ref.nz);
  QVERIFY(std::fabs(conv.px - ref.px) < 1e-3);
  QVERIFY(std::fabs(conv.py - ref.py) < 1e-3);
  QVERIFY(std::fabs(conv.pz - ref.pz) < 1e-3);

  // The tools may write other units (e.g. without the ECAT7 calibration
  // factor), so the values have to agree up to a common factor. A wrong
  // voxel order (e.g. a flipped axis) breaks this.
  const qint64 volume = static_cast<qint64>(ref.nx) * ref.ny * ref.nz;
  double factor = 0.0;
  for(int f=0; f < ref.frames; f++) {
    double sxy = 0.0, sxx = 0.0;
    for(qint64 i=0; i < volume; i++) {
      sxy += conv.values[f * volume + i] * ref.values[f * volume + i];
      sxx += ref.values[f * volume + i] * ref.values[f * volume + i];
    }
    const double c = sxy / sxx;
    QVERIFY(c > 0.0);
    if(f == 0)
      factor = c;
    QVERIFY2(std::fabs(c - factor) <= 1e-3 * factor, "the values of the frames differ by different factors");

    const double m = maxAbs(conv.values, f * volume, volume);
    for(qint64 i=0; i < volume; i++) {
      if(std::fabs(conv.values[f * volume + i] - c * ref.values[f * volume + i]) > 1e-3 * m)
        QFAIL(qPrintable(QString("frame %1 voxel %2: %3 instead of %4").arg(f+1).arg(i).arg(conv.values[f * volume + i]).arg(c * ref.values[f * volume + i])));
    }
  }

  // the landmark voxels of the NIfTI file end up at the landmark voxels of the ECAT7 file
  foreach(const QJsonValue& l, landmarks(*niftiFile)) {
    int vn[3], ve[3];
    if(landmarkVoxel(l.toObject(), nifti, vn) == false || landmarkVoxel(l.toObject(), ecat, ve) == false)
      continue;
    for(int f=0; f < conv.frames; f++) {
      const double expected = n.value(vn[0], vn[1], vn[2], f);
      const double value = conv.value(ve[0], ve[1], ve[2], f);
      QVERIFY2(std::fabs(value - expected) <= conv.frameInfo[f].scale + 1e-6 * std::fabs(expected),
               qPrintable(QString("landmark %1 frame %2: %3 instead of %4").arg(l.toObject().value("name").toString()).arg(f+1).arg(value).arg(expected)));
    }
  }
}

//---------------------------------------------------------------------------------------------
// NIfTI -> ECAT7 with synthetic orientations

QString TestNIFTIConversion::writeSyntheticNifti(const QString& name, const double affine[3][4], bool useQform,
                                                 int units, int frames, double pixdim4, double toffset)
{
  const int nx = 4, ny = 3, nz = 2;
  const QString path = tmpFile(name);

  CNIFTIFile file(path, CNIFTIMainHeader::NIFTI1);
  if(file.open(QIODevice::WriteOnly) == false)
    return QString();

  QScopedPointer<CNIFTIMainHeader> header(file.createEmptyHeader());
  CNIFTI1MainHeader* h = static_cast<CNIFTI1MainHeader*>(header.data());

  // voxel sizes = column norms of the affine (in file units)
  double size[3];
  for(int j=0; j < 3; j++)
    size[j] = std::sqrt(affine[0][j]*affine[0][j] + affine[1][j]*affine[1][j] + affine[2][j]*affine[2][j]);

  h->setDim(0, frames > 1 ? 4 : 3);
  h->setDim(1, nx); h->setDim(2, ny); h->setDim(3, nz); h->setDim(4, frames);
  for(short i=5; i < 8; i++) h->setDim(i, 1);
  h->setPix_Dim(0, 1.0);
  for(short i=1; i <= 3; i++) h->setPix_Dim(i, size[i-1]);
  h->setPix_Dim(4, pixdim4);
  h->setToffset(toffset);
  h->setXyzt_Units(units);
  h->setDataType(4);
  h->setBit_Pix(16);
  h->setScl_Slope(1.0);
  h->setScl_Inter(0.0);

  if(useQform) {
    // the affines used with useQform are rotations about z by 180 degrees
    // (quaternion (0, 0, 1)) with qfac = sign of affine[2][2]
    h->setSform_Code(0);
    h->setQform_Code(1);
    h->setQuatern_B(0.0); h->setQuatern_C(0.0); h->setQuatern_D(1.0);
    h->setPix_Dim(0, affine[2][2] < 0 ? -1.0 : 1.0);
    h->setQoffset_X(affine[0][3]); h->setQoffset_Y(affine[1][3]); h->setQoffset_Z(affine[2][3]);
  } else {
    h->setQform_Code(0);
    h->setSform_Code(2);
    for(short i=0; i < 4; i++) {
      h->setSrow_X(i, affine[0][i]);
      h->setSrow_Y(i, affine[1][i]);
      h->setSrow_Z(i, affine[2][i]);
    }
  }

  QByteArray data(nx * ny * nz * frames * 2, '\0');
  qint16* v = reinterpret_cast<qint16*>(data.data());
  for(int i=0; i < nx * ny * nz * frames; i++)
    v[i] = static_cast<qint16>(i + 1);

  if(file.writeMainHeader(*h) == false || file.writeMatrix(data) == false)
    return QString();
  file.close();

  return path;
}

void TestNIFTIConversion::niftiToEcatOrientation_data()
{
  QTest::addColumn<QVector<double> >("affine");
  QTest::addColumn<bool>("useQform");
  QTest::addColumn<int>("units");
  QTest::addColumn<double>("unitScale");

  // RAS affines (rows), voxel sizes 2, 2.5 and 3 mm
  QTest::newRow("RAS (dcm2niix, nibabel)") << (QVector<double>() << 2 << 0 << 0 << 10   << 0 << 2.5 << 0 << -20 << 0 << 0 << 3 << -30) << false << 10 << 1.0;
  QTest::newRow("LPS (libmedio)")          << (QVector<double>() << -2 << 0 << 0 << 10  << 0 << -2.5 << 0 << -20 << 0 << 0 << 3 << -30) << false << 10 << 1.0;
  QTest::newRow("LAI")                     << (QVector<double>() << -2 << 0 << 0 << 10  << 0 << 2.5 << 0 << -20 << 0 << 0 << -3 << -30) << false << 10 << 1.0;
  QTest::newRow("permuted axes (A, L, S)") << (QVector<double>() << 0 << -2.5 << 0 << 10 << 2 << 0 << 0 << -20 << 0 << 0 << 3 << -30) << false << 10 << 1.0;
  QTest::newRow("sagittal (S, P, L)")      << (QVector<double>() << 0 << 0 << -3 << 10  << 0 << -2.5 << 0 << -20 << 2 << 0 << 0 << -30) << false << 10 << 1.0;
  QTest::newRow("qform qfac 1")            << (QVector<double>() << -2 << 0 << 0 << 10  << 0 << -2.5 << 0 << -20 << 0 << 0 << 3 << -30) << true << 10 << 1.0;
  QTest::newRow("qform qfac -1")           << (QVector<double>() << -2 << 0 << 0 << 10  << 0 << -2.5 << 0 << -20 << 0 << 0 << -3 << -30) << true << 10 << 1.0;
  QTest::newRow("units m and ms")          << (QVector<double>() << 0.002 << 0 << 0 << 0.01 << 0 << 0.0025 << 0 << -0.02 << 0 << 0 << 0.003 << -0.03) << false << (1 | 16) << 1000.0;
}

void TestNIFTIConversion::niftiToEcatOrientation()
{
  QFETCH(QVector<double>, affine);
  QFETCH(bool, useQform);
  QFETCH(int, units);
  QFETCH(double, unitScale);

  double a[3][4];
  for(int i=0; i < 12; i++)
    a[i / 4][i % 4] = affine[i];

  // 2 frames of 500 time units each, starting at 1000 time units
  const double timeScale = (units & 0x38) == 16 ? 0.001 : 1.0;
  const QString name = QString("orientation-%1").arg(QTest::currentDataTag()).replace(QRegExp("[^a-zA-Z0-9-]"), "_");
  const QString in = writeSyntheticNifti(name + ".nii", a, useQform, units, 2, 500.0, 1000.0);
  QVERIFY(in.isEmpty() == false);

  QString error;
  QVERIFY2(CNIFTIConverter::niftiToEcat(in, tmpFile(name + ".v"), &error), qPrintable(error));

  EcatVolume e;
  QVERIFY(readEcat(tmpFile(name + ".v"), e));
  QCOMPARE(e.frames, 2);
  QCOMPARE(e.nx * e.ny * e.nz, 24);
  QCOMPARE(e.frameInfo[0].startMs, static_cast<unsigned int>(qRound(1000.0 * timeScale * 1000.0)));
  QCOMPARE(e.frameInfo[0].durationMs, static_cast<unsigned int>(qRound(500.0 * timeScale * 1000.0)));
  QCOMPARE(e.frameInfo[1].startMs, static_cast<unsigned int>(qRound(1500.0 * timeScale * 1000.0)));

  // inverse of the 3x3 part of the affine (in mm)
  double m[3][3], inv[3][3];
  for(int i=0; i < 3; i++)
    for(int j=0; j < 3; j++)
      m[i][j] = a[i][j] * unitScale;
  const double det = m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1]) - m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0]) + m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]);
  for(int i=0; i < 3; i++)
    for(int j=0; j < 3; j++) {
      const int i1 = (j + 1) % 3, i2 = (j + 2) % 3, j1 = (i + 1) % 3, j2 = (i + 2) % 3;
      inv[i][j] = (m[i1][j1] * m[i2][j2] - m[i1][j2] * m[i2][j1]) / det;
    }

  // every ECAT7 voxel holds the value of the NIfTI voxel at the same position
  const int nifti[3] = { 4, 3, 2 };
  for(int f=0; f < e.frames; f++)
    for(int k=0; k < e.nz; k++)
      for(int j=0; j < e.ny; j++)
        for(int i=0; i < e.nx; i++) {
          double lps[3];
          e.position(i, j, k, lps);
          const double ras[3] = { -lps[0] - a[0][3] * unitScale, -lps[1] - a[1][3] * unitScale, lps[2] - a[2][3] * unitScale };
          double v[3];
          for(int r=0; r < 3; r++) {
            v[r] = inv[r][0] * ras[0] + inv[r][1] * ras[1] + inv[r][2] * ras[2];
            QVERIFY2(std::fabs(v[r] - qRound(v[r])) < 1e-3 && qRound(v[r]) >= 0 && qRound(v[r]) < nifti[r],
                     qPrintable(QString("ECAT voxel (%1,%2,%3) maps to NIfTI (%4,%5,%6)").arg(i).arg(j).arg(k).arg(v[0]).arg(v[1]).arg(v[2])));
          }
          const double expected = 1 + qRound(v[0]) + 4 * (qRound(v[1]) + 3 * (qRound(v[2]) + 2 * f));
          QCOMPARE(e.value(i, j, k, f), expected);
        }

  // and back: same positions in the NIfTI file written from the ECAT7 file
  QVERIFY2(CNIFTIConverter::ecatToNifti(tmpFile(name + ".v"), tmpFile(name + ".back.nii"), CNIFTIConverter::Options(), &error), qPrintable(error));
  NiftiVolume n;
  QVERIFY(readNifti(tmpFile(name + ".back.nii"), n));
  for(qint64 k=0; k < n.dim[3]; k++)
    for(qint64 j=0; j < n.dim[2]; j++)
      for(qint64 i=0; i < n.dim[1]; i++) {
        // original NIfTI voxel of this value
        const int value = qRound(n.value(i, j, k, 0)) - 1;
        const int o[3] = { value % 4, (value / 4) % 3, value / 12 };
        double p[3], q[3];
        n.position(i, j, k, p);
        for(int r=0; r < 3; r++)
          q[r] = (a[r][0] * o[0] + a[r][1] * o[1] + a[r][2] * o[2] + a[r][3]) * unitScale;
        for(int r=0; r < 3; r++)
          QVERIFY(std::fabs(p[r] - q[r]) < 1e-3);
      }
}

//---------------------------------------------------------------------------------------------
// Philips -> NIfTI, compared with the NIfTI file converted from the ECAT7 file
// of the same dataset (the Philips and ECAT7 files hold the same data)

void TestNIFTIConversion::philipsToNifti_data()
{
  QTest::addColumn<QString>("philips");
  QTest::addColumn<QString>("ecat");

  bool any = false;
  foreach(const MedIOTestFile& philips, m_data.files) {
    if(philips.format != "philips")
      continue;
    foreach(const MedIOTestFile& ecat, m_data.files) {
      if(ecat.format == "ecat" && ecat.dataset == philips.dataset) {
        QTest::newRow(qPrintable(philips.relativePath)) << philips.path << ecat.path;
        any = true;
      }
    }
  }

  if(any == false)
    QTest::newRow("no Philips/ECAT7 test data") << QString() << QString();
}

void TestNIFTIConversion::philipsToNifti()
{
  QFETCH(QString, philips);
  QFETCH(QString, ecat);
  if(philips.isEmpty())
    QSKIP("no Philips and ECAT7 files of the same dataset in the test data");

  const QString base = tmpFile(QFileInfo(philips).fileName());
  QString error;
  QVERIFY2(CNIFTIConverter::philipsToNifti(philips, base + ".nii", CNIFTIConverter::Options(), &error), qPrintable(error));
  QVERIFY2(CNIFTIConverter::toNifti(philips, base + ".auto.nii", CNIFTIConverter::Options(), &error), qPrintable(error));
  QVERIFY2(CNIFTIConverter::ecatToNifti(ecat, base + ".ecat.nii", CNIFTIConverter::Options(), &error), qPrintable(error));

  NiftiVolume p, a, e;
  QVERIFY(readNifti(base + ".nii", p));
  QVERIFY(readNifti(base + ".auto.nii", a));
  QVERIFY(readNifti(base + ".ecat.nii", e));
  QCOMPARE(a.values, p.values);

  // header and geometry
  for(int i=0; i <= 4; i++)
    QCOMPARE(p.dim[i], e.dim[i]);
  for(int r=0; r < 3; r++)
    for(int c=0; c < 4; c++)
      QVERIFY2(std::fabs(p.srow[r][c] - e.srow[r][c]) < 1e-3, qPrintable(QString("srow[%1][%2]").arg(r).arg(c)));

  // values per frame
  const qint64 volume = e.dim[1] * e.dim[2] * e.dim[3];
  const int frames = e.dim[0] >= 4 ? int(e.dim[4]) : 1;
  QCOMPARE(qint64(p.values.size()), volume * frames);
  for(int f=0; f < frames; f++) {
    const double tol = 1e-6 * maxAbs(e.values, f * volume, volume);
    for(qint64 i=0; i < volume; i++) {
      if(std::fabs(p.values[f * volume + i] - e.values[f * volume + i]) > tol)
        QFAIL(qPrintable(QString("frame %1 voxel %2: %3 instead of %4").arg(f+1).arg(i).arg(p.values[f * volume + i]).arg(e.values[f * volume + i])));
    }
  }

  // frame timing and metadata
  QCOMPARE(p.json.value("FrameTimesStart").toArray(), e.json.value("FrameTimesStart").toArray());
  QCOMPARE(p.json.value("FrameDuration").toArray(), e.json.value("FrameDuration").toArray());
  QCOMPARE(p.json.value("PatientName").toString(), e.json.value("PatientName").toString());
  QCOMPARE(p.json.value("RadionuclideTotalDose").toDouble(), e.json.value("RadionuclideTotalDose").toDouble());
  QVERIFY(std::fabs(p.json.value("PatientWeight").toDouble() - e.json.value("PatientWeight").toDouble()) < 1e-3);

  // the native fields of the Philips main header
  CPhilipsFile philipsFile(philips);
  QVERIFY(philipsFile.open(QIODevice::ReadOnly));
  CPhilipsMainHeader* mainHeader = NULL;
  QVERIFY(philipsFile.readMainHeader(mainHeader));
  QScopedPointer<CPhilipsMainHeader> guard(mainHeader);
  QCOMPARE(p.json.value("Philips").toObject(), mainHeader->attributes().value("Philips").toObject());
}

//---------------------------------------------------------------------------------------------
// Philips -> NIfTI -> Philips: the attributes of the Philips main header and of
// the first slice of every frame (native fields and attributes) are kept

void TestNIFTIConversion::philipsAttributesRoundTrip_data()
{
  QTest::addColumn<QString>("philips");

  bool any = false;
  foreach(const MedIOTestFile& file, m_data.files) {
    if(file.format == "philips") {
      QTest::newRow(qPrintable(file.relativePath)) << file.path;
      any = true;
    }
  }

  if(any == false)
    QTest::newRow("no Philips test data") << QString();
}

void TestNIFTIConversion::philipsAttributesRoundTrip()
{
  QFETCH(QString, philips);
  if(philips.isEmpty())
    QSKIP("no Philips files in the test data");

  const QString suffix = philips.endsWith(".syn") ? ".syn" : ".img";
  const QString base = tmpFile("attributes-" + QFileInfo(philips).completeBaseName() + suffix.mid(1));
  QString error;
  QVERIFY2(CNIFTIConverter::philipsToNifti(philips, base + ".nii", CNIFTIConverter::Options(), &error), qPrintable(error));
  QVERIFY2(CNIFTIConverter::niftiToPhilips(base + ".nii", base + suffix, &error), qPrintable(error));

  CPhilipsFile source(philips);
  CPhilipsFile written(base + suffix);
  QVERIFY(source.open(QIODevice::ReadOnly));
  QVERIFY(written.open(QIODevice::ReadOnly));

  CPhilipsMainHeader* a = NULL;
  CPhilipsMainHeader* b = NULL;
  QVERIFY(source.readMainHeader(a));
  QVERIFY(written.readMainHeader(b));
  QScopedPointer<CPhilipsMainHeader> aGuard(a), bGuard(b);

  const QJsonObject ma = a->attributes();
  const QJsonObject mb = b->attributes();
  QStringList different;
  foreach(const QString& key, ma.keys() + mb.keys()) {
    if(ma.value(key) != mb.value(key) && key != "Philips" && different.contains(key) == false)
      different.append(key);
  }
  const QJsonObject na = ma.value("Philips").toObject();
  const QJsonObject nb = mb.value("Philips").toObject();
  foreach(const QString& field, na.keys() + nb.keys()) {
    if(na.value(field) != nb.value(field) && different.contains("Philips." + field) == false)
      different.append("Philips." + field);
  }
  QVERIFY2(different.isEmpty(), qPrintable(different.join(", ")));

  // frames: the first slice
  const QList<short> frames = source.frameNumbers();
  QCOMPARE(written.frameNumbers().size(), frames.size());
  for(int k=0; k < frames.size(); k++) {
    CPhilipsSubHeader* sa = NULL;
    CPhilipsSubHeader* sb = NULL;
    QVERIFY(source.readSubHeader(sa, frames[k], -1));
    QVERIFY(written.readSubHeader(sb, written.frameNumbers()[k], -1));
    QScopedPointer<CPhilipsSubHeader> saGuard(sa), sbGuard(sb);
    QCOMPARE(sb->attributes().value("Philips").toObject(), sa->attributes().value("Philips").toObject());
  }
}

//---------------------------------------------------------------------------------------------
// ECAT7 -> NIfTI -> Philips (image and Syntegra) -> NIfTI

void TestNIFTIConversion::philipsRoundTrip_data()
{
  QTest::addColumn<QString>("ecat");
  QTest::addColumn<QString>("suffix");

  bool any = false;
  foreach(const MedIOTestFile& file, m_data.files) {
    if(file.format == "ecat") {
      QTest::newRow(qPrintable(file.relativePath + " image")) << file.path << ".img";
      QTest::newRow(qPrintable(file.relativePath + " syntegra")) << file.path << ".syn";
      any = true;
    }
  }

  if(any == false)
    QTest::newRow("no ECAT7 test data") << QString() << QString();
}

void TestNIFTIConversion::philipsRoundTrip()
{
  QFETCH(QString, ecat);
  QFETCH(QString, suffix);
  if(ecat.isEmpty())
    QSKIP("no ECAT7 files in the test data");

  const QString base = tmpFile(QFileInfo(ecat).fileName() + ".prt");
  QString error;
  QVERIFY2(CNIFTIConverter::ecatToNifti(ecat, base + ".nii", CNIFTIConverter::Options(), &error), qPrintable(error));
  QVERIFY2(CNIFTIConverter::niftiToPhilips(base + ".nii", base + suffix, &error), qPrintable(error));
  QVERIFY2(CNIFTIConverter::philipsToNifti(base + suffix, base + ".back.nii", CNIFTIConverter::Options(), &error), qPrintable(error));

  NiftiVolume a, b;
  QVERIFY(readNifti(base + ".nii", a));
  QVERIFY(readNifti(base + ".back.nii", b));

  // the Philips file
  QCOMPARE(CMedIODataFactory::identify(base + suffix), CMedIOData::Philips);
  {
    CPhilipsFile philipsFile(base + suffix);
    QVERIFY(philipsFile.open(QIODevice::ReadOnly));
    CPhilipsMainHeader* mainHeader = NULL;
    QVERIFY(philipsFile.readMainHeader(mainHeader));
    QScopedPointer<CPhilipsMainHeader> guard(mainHeader);
    QCOMPARE(int(mainHeader->filtyp()), int(suffix == ".syn" ? CPhilipsMainHeader::Syntegra : CPhilipsMainHeader::Image));
    QCOMPARE(qint64(philipsFile.numFrames()), a.dim[0] >= 4 ? a.dim[4] : qint64(1));
    QCOMPARE(qint64(philipsFile.numSlices(philipsFile.minFrame())), a.dim[3]);
    QCOMPARE(int(philipsFile.minSlice()), suffix == ".syn" ? 1 : 0);
    QCOMPARE(QString(mainHeader->patient_name()), a.json.value("PatientName").toString());
  }

  // header and geometry
  for(int i=0; i <= 4; i++)
    QCOMPARE(b.dim[i], a.dim[i]);
  for(int r=0; r < 3; r++)
    for(int c=0; c < 4; c++)
      QVERIFY2(std::fabs(b.srow[r][c] - a.srow[r][c]) < 1e-3, qPrintable(QString("srow[%1][%2]").arg(r).arg(c)));

  // values (16 bit integers with a scale factor per frame)
  const qint64 volume = a.dim[1] * a.dim[2] * a.dim[3];
  const int frames = a.dim[0] >= 4 ? int(a.dim[4]) : 1;
  for(int f=0; f < frames; f++) {
    const double m = maxAbs(a.values, f * volume, volume);
    const double tol = 0.5 * m / 32767.0 + 1e-6 * m;
    for(qint64 i=0; i < volume; i++) {
      if(std::fabs(b.values[f * volume + i] - a.values[f * volume + i]) > tol)
        QFAIL(qPrintable(QString("frame %1 voxel %2: %3 instead of %4").arg(f+1).arg(i).arg(b.values[f * volume + i]).arg(a.values[f * volume + i])));
    }
  }

  // frame timing and metadata
  QCOMPARE(b.json.value("FrameTimesStart").toArray(), a.json.value("FrameTimesStart").toArray());
  QCOMPARE(b.json.value("FrameDuration").toArray(), a.json.value("FrameDuration").toArray());
  QCOMPARE(b.json.value("PatientName").toString(), a.json.value("PatientName").toString());
  QCOMPARE(b.json.value("Philips").toObject().value("isotope_name").toString(),
           a.json.value("ECAT7").toObject().value("Isotope_Name").toString());
  QCOMPARE(b.json.value("RadionuclideTotalDose").toDouble(), a.json.value("RadionuclideTotalDose").toDouble());
  QVERIFY(std::fabs(b.json.value("PatientWeight").toDouble() - a.json.value("PatientWeight").toDouble()) < 1e-3);
}

//---------------------------------------------------------------------------------------------
// Philips file with a different scale factor per slice (written by means of
// the NIfTI -> Philips header conversion) -> float32 NIfTI

void TestNIFTIConversion::philipsSliceScales_data()
{
  QTest::addColumn<int>("dataType");
  QTest::addColumn<double>("offset");  // stored value of voxel i of slice k: i + 1 + 100 k + offset

  QTest::newRow("signed short")   << int(CPhilipsSubHeader::SignedShort) << -50.0;
  QTest::newRow("unsigned short") << int(CPhilipsSubHeader::UnsignedShort) << 40000.0;
  QTest::newRow("byte")           << int(CPhilipsSubHeader::ByteData) << 0.0;
  QTest::newRow("float")          << int(CPhilipsSubHeader::Float) << 0.25;
}

void TestNIFTIConversion::philipsSliceScales()
{
  QFETCH(int, dataType);
  QFETCH(double, offset);

  const double affine[3][4] = { { -2, 0, 0, 0 }, { 0, -2.5, 0, 0 }, { 0, 0, 3, 0 } };
  const QString name = QString("slices%1").arg(dataType);
  const QString nifti = writeSyntheticNifti(name + ".nii", affine, false, 10, 1, 0.0, 0.0);
  QVERIFY(nifti.isEmpty() == false);

  CNIFTIFile niftiFile(nifti);
  QVERIFY(niftiFile.open(QIODevice::ReadOnly));
  CNIFTIMainHeader* niftiHeader = NULL;
  QVERIFY(niftiFile.readMainHeader(niftiHeader));
  QScopedPointer<CNIFTIMainHeader> niftiGuard(niftiHeader);

  const double scale[2] = { 0.5, 2.0 };
  {
    CPhilipsFile philipsFile(tmpFile(name + ".img"), CPhilipsMainHeader::Image);
    QVERIFY(philipsFile.open(QIODevice::WriteOnly));

    QScopedPointer<CPhilipsMainHeader> mainHeader(philipsFile.createEmptyMainHeader());
    QVERIFY(mainHeader->convertFrom(niftiHeader));
    QCOMPARE(int(mainHeader->nslice()), 2);
    QCOMPARE(int(mainHeader->nframe()), 1);
    QVERIFY(std::fabs(mainHeader->Dslice_thick() - 3.0) < 1e-4);

    QScopedPointer<CPhilipsSubHeader> subHeaderPtr(philipsFile.createEmptySubHeader());
    CPhilipsSubHeaderImage* subHeader = static_cast<CPhilipsSubHeaderImage*>(subHeaderPtr.data());
    QVERIFY(subHeader->convertFrom(NULL, niftiHeader));
    QCOMPARE(int(subHeader->xdim()), 4);
    QCOMPARE(int(subHeader->ydim()), 3);
    QVERIFY(std::fabs(subHeader->pix_spacing_x() - 2.0) < 1e-4);
    QVERIFY(std::fabs(subHeader->pix_spacing_y() - 2.5) < 1e-4);

    subHeader->setDatype(static_cast<CPhilipsSubHeader::Data_Type>(dataType));
    for(int k=0; k < 2; k++) {
      QByteArray plane;
      for(int i=0; i < 12; i++) {
        const double value = i + 1 + 100 * k + offset;
        if(dataType == CPhilipsSubHeader::SignedShort) {
          const qint16 v = static_cast<qint16>(value);
          plane.append(reinterpret_cast<const char*>(&v), sizeof(v));
        } else if(dataType == CPhilipsSubHeader::UnsignedShort) {
          const quint16 v = static_cast<quint16>(value);
          plane.append(reinterpret_cast<const char*>(&v), sizeof(v));
        } else if(dataType == CPhilipsSubHeader::ByteData) {
          plane.append(static_cast<char>(static_cast<quint8>(value)));
        } else {
          const float v = static_cast<float>(value);
          plane.append(reinterpret_cast<const char*>(&v), sizeof(v));
        }
      }

      subHeader->setSuvscl(0.0f);
      subHeader->setImgscl(scale[k]);
      subHeader->setSlcnum(k);
      QVERIFY(philipsFile.writeMatrix(plane.constData(), plane.size(), *subHeader, 1, k));
    }

    QVERIFY(philipsFile.writeMainHeader(*mainHeader));
    philipsFile.close();
  }

  QString error;
  QVERIFY2(CNIFTIConverter::philipsToNifti(tmpFile(name + ".img"), tmpFile(name + ".back.nii"), CNIFTIConverter::Options(), &error), qPrintable(error));

  NiftiVolume n;
  QVERIFY(readNifti(tmpFile(name + ".back.nii"), n));
  QCOMPARE(n.datatype, 16);
  QCOMPARE(n.dim[1], qint64(4));
  QCOMPARE(n.dim[2], qint64(3));
  QCOMPARE(n.dim[3], qint64(2));
  QVERIFY(std::fabs(n.pixdim[1] - 2.0) < 1e-4);
  QVERIFY(std::fabs(n.pixdim[2] - 2.5) < 1e-4);
  QVERIFY(std::fabs(n.pixdim[3] - 3.0) < 1e-4);

  // Philips (ECAT7) plane k is NIfTI slice nz-1-k
  for(int k=0; k < 2; k++)
    for(int j=0; j < 3; j++)
      for(int i=0; i < 4; i++)
        QCOMPARE(n.value(i, j, 1 - k, 0), (i + 4 * j + 1 + 100 * k + offset) * scale[k]);
}

//---------------------------------------------------------------------------------------------
// header conversion Philips -> NIfTI (CNIFTI1/2MainHeader::convertFrom())

void TestNIFTIConversion::philipsHeaderConversion()
{
  QString philips;
  foreach(const MedIOTestFile& f, m_data.files)
    if(f.format == "philips" && philips.isEmpty())
      philips = f.path;
  if(philips.isEmpty())
    QSKIP("no Philips files in the test data");

  CPhilipsFile philipsFile(philips);
  QVERIFY(philipsFile.open(QIODevice::ReadOnly));
  CPhilipsMainHeader* mainHeader = NULL;
  QVERIFY(philipsFile.readMainHeader(mainHeader));
  QScopedPointer<CPhilipsMainHeader> mainGuard(mainHeader);
  CPhilipsSubHeader* subHeader = NULL;
  QVERIFY(philipsFile.readSubHeader(subHeader, philipsFile.minFrame(), philipsFile.minSlice()));
  QScopedPointer<CPhilipsSubHeader> subGuard(subHeader);
  const CPhilipsSubHeaderImage* s = static_cast<const CPhilipsSubHeaderImage*>(subHeader);

  bool ok = false;
  const double scale = s->ecat_Scale_Factor(ok, mainHeader);
  QVERIFY(ok);

  for(int version=1; version <= 2; version++) {
    CNIFTIFile niftiFile(tmpFile(QString("header%1.nii").arg(version)), version == 1 ? CNIFTIMainHeader::NIFTI1 : CNIFTIMainHeader::NIFTI2);
    QVERIFY(niftiFile.open(QIODevice::WriteOnly));
    QScopedPointer<CNIFTIMainHeader> header(niftiFile.createEmptyHeader());
    QVERIFY(header->convertFrom(mainHeader, subHeader));

    double slope = 0.0, pixdim[4];
    qint64 dim[5];
    int sformCode = 0;
    if(version == 1) {
      const CNIFTI1MainHeader* h = static_cast<const CNIFTI1MainHeader*>(header.data());
      slope = h->scl_Slope(); sformCode = h->sform_Code();
      for(int i=0; i < 5; i++) dim[i] = h->dim(i);
      for(int i=1; i < 4; i++) pixdim[i] = h->pix_Dim(i);
    } else {
      const CNIFTI2MainHeader* h = static_cast<const CNIFTI2MainHeader*>(header.data());
      slope = h->scl_Slope(); sformCode = h->sform_Code();
      for(int i=0; i < 5; i++) dim[i] = h->dim(i);
      for(int i=1; i < 4; i++) pixdim[i] = h->pix_Dim(i);
    }

    QCOMPARE(dim[0], qint64(philipsFile.numFrames() > 1 ? 4 : 3));
    QCOMPARE(dim[1], qint64(s->xdim()));
    QCOMPARE(dim[2], qint64(s->ydim()));
    QCOMPARE(dim[3], qint64(philipsFile.numSlices(philipsFile.minFrame())));
    QVERIFY(std::fabs(pixdim[1] - s->pix_spacing_x()) < 1e-4);
    QVERIFY(std::fabs(pixdim[2] - s->pix_spacing_y()) < 1e-4);
    QVERIFY(std::fabs(pixdim[3] - mainHeader->Dslice_thick()) < 1e-4);
    QVERIFY(std::fabs(slope - scale) <= 1e-6 * scale);
    QCOMPARE(sformCode, 1);
    niftiFile.close();
  }
}

//---------------------------------------------------------------------------------------------
// metadata options and NIfTI files of other tools without JSON metadata

void TestNIFTIConversion::metadataOptions()
{
  QString ecat;
  foreach(const MedIOTestFile& f, m_data.files)
    if(f.format == "ecat" && ecat.isEmpty())
      ecat = f.path;
  if(ecat.isEmpty())
    QSKIP("no ECAT7 files in the test data");

  QString error;

  // sidecar file next to the NIfTI file, no header extension
  CNIFTIConverter::Options options;
  options.jsonExtension = false;
  options.jsonSidecar = true;
  QVERIFY2(CNIFTIConverter::ecatToNifti(ecat, tmpFile("sidecar.nii.gz"), options, &error), qPrintable(error));
  QVERIFY(QFile::exists(tmpFile("sidecar.json")));

  NiftiVolume n;
  QVERIFY(readNifti(tmpFile("sidecar.nii.gz"), n));
  QVERIFY(n.json.isEmpty());
  QCOMPARE(n.voxOffset, 352.0);

  QFile sidecar(tmpFile("sidecar.json"));
  QVERIFY(sidecar.open(QIODevice::ReadOnly));
  const QJsonObject json = QJsonDocument::fromJson(sidecar.readAll()).object();
  QVERIFY(json.contains("PatientName"));
  QVERIFY(json.value("ECAT7").toObject().contains("Patient_Name"));
  QVERIFY(json.contains("FrameDuration"));

  // the metadata is restored from the sidecar file
  QVERIFY2(CNIFTIConverter::niftiToEcat(tmpFile("sidecar.nii.gz"), tmpFile("sidecar.v"), &error), qPrintable(error));
  EcatVolume a, b;
  QVERIFY(readEcat(ecat, a));
  QVERIFY(readEcat(tmpFile("sidecar.v"), b));
  QCOMPARE(b.patientName, a.patientName);
  QCOMPARE(b.isotope, a.isotope);
  for(int f=0; f < a.frames; f++)
    QCOMPARE(b.frameInfo[f].durationMs, a.frameInfo[f].durationMs);

  // sidecar file of a header/image pair (written via the image file name)
  QVERIFY2(CNIFTIConverter::ecatToNifti(ecat, tmpFile("sidecarpair.img"), options, &error), qPrintable(error));
  QVERIFY(QFile::exists(tmpFile("sidecarpair.hdr")));
  QVERIFY(QFile::exists(tmpFile("sidecarpair.json")));
  QVERIFY2(CNIFTIConverter::niftiToEcat(tmpFile("sidecarpair.hdr"), tmpFile("sidecarpair.v"), &error), qPrintable(error));
  EcatVolume p;
  QVERIFY(readEcat(tmpFile("sidecarpair.v"), p));
  QCOMPARE(p.patientName, a.patientName);
  for(int f=0; f < a.frames; f++)
    QCOMPARE(p.frameInfo[f].durationMs, a.frameInfo[f].durationMs);

  // without any metadata: no frame timing for 3D data (pixdim[4] is ignored)
  const double affine[3][4] = { { -2, 0, 0, 0 }, { 0, -2.5, 0, 0 }, { 0, 0, 3, 0 } };
  const QString plain = writeSyntheticNifti("plain.nii", affine, false, 10, 1, 1.0, 0.0);
  QVERIFY(plain.isEmpty() == false);
  QVERIFY2(CNIFTIConverter::niftiToEcat(plain, tmpFile("plain.v"), &error), qPrintable(error));
  EcatVolume c;
  QVERIFY(readEcat(tmpFile("plain.v"), c));
  QCOMPARE(c.frames, 1);
  QCOMPARE(c.frameInfo[0].durationMs, 0u);
  QCOMPARE(c.frameInfo[0].startMs, 0u);
}

//---------------------------------------------------------------------------------------------
// errors: no output file is left behind

void TestNIFTIConversion::errors()
{
  QString error;

  QVERIFY(CNIFTIConverter::ecatToNifti(tmpFile("missing.v"), tmpFile("missing.nii"), CNIFTIConverter::Options(), &error) == false);
  QVERIFY(error.isEmpty() == false);
  QVERIFY(QFile::exists(tmpFile("missing.nii")) == false);

  error.clear();
  QVERIFY(CNIFTIConverter::niftiToEcat(tmpFile("missing.nii"), tmpFile("missing.v"), &error) == false);
  QVERIFY(error.isEmpty() == false);
  QVERIFY(QFile::exists(tmpFile("missing.v")) == false);

  // a NIfTI file is no ECAT7 file and vice versa
  const double affine[3][4] = { { -2, 0, 0, 0 }, { 0, -2.5, 0, 0 }, { 0, 0, 3, 0 } };
  const QString nifti = writeSyntheticNifti("error.nii", affine, false, 10, 1, 0.0, 0.0);
  QVERIFY(nifti.isEmpty() == false);
  QVERIFY(CNIFTIConverter::ecatToNifti(nifti, tmpFile("error.out.nii"), CNIFTIConverter::Options(), &error) == false);
  QVERIFY(QFile::exists(tmpFile("error.out.nii")) == false);

  QVERIFY2(CNIFTIConverter::niftiToEcat(nifti, tmpFile("error.v"), &error), qPrintable(error));
  QVERIFY(CNIFTIConverter::niftiToEcat(tmpFile("error.v"), tmpFile("error.out.v"), &error) == false);
  QVERIFY(QFile::exists(tmpFile("error.out.v")) == false);

  // output directory does not exist
  QVERIFY(CNIFTIConverter::niftiToEcat(nifti, tmpFile("no/such/dir/error.v"), &error) == false);
  QVERIFY(CNIFTIConverter::ecatToNifti(tmpFile("error.v"), tmpFile("no/such/dir/error.nii"), CNIFTIConverter::Options(), &error) == false);
  QVERIFY(CNIFTIConverter::ecatToNifti(tmpFile("error.v"), tmpFile("no/such/dir/error.nii.gz"), CNIFTIConverter::Options(), &error) == false);
  QVERIFY(QFile::exists(tmpFile("no/such/dir/error.nii.gz")) == false);

  // Philips conversions
  QVERIFY(CNIFTIConverter::philipsToNifti(nifti, tmpFile("error.p.nii"), CNIFTIConverter::Options(), &error) == false);
  QVERIFY(QFile::exists(tmpFile("error.p.nii")) == false);
  QVERIFY(CNIFTIConverter::philipsToNifti(tmpFile("missing.img"), tmpFile("missing.p.nii"), CNIFTIConverter::Options(), &error) == false);
  QVERIFY(CNIFTIConverter::toNifti(nifti, tmpFile("error.t.nii"), CNIFTIConverter::Options(), &error) == false);
  QVERIFY(QFile::exists(tmpFile("error.t.nii")) == false);
  QVERIFY(CNIFTIConverter::niftiToPhilips(nifti, tmpFile("no/such/dir/error.img"), &error) == false);
  QVERIFY(CNIFTIConverter::niftiToPhilips(tmpFile("missing.nii"), tmpFile("missing.img"), &error) == false);
  QVERIFY(QFile::exists(tmpFile("missing.img")) == false);

  // the image file of a pair cannot be created: no header file is left behind
  QVERIFY(QDir(m_tmp.path()).mkdir("blocked.img"));
  QVERIFY(CNIFTIConverter::ecatToNifti(tmpFile("error.v"), tmpFile("blocked.hdr"), CNIFTIConverter::Options(), &error) == false);
  QVERIFY(QFile::exists(tmpFile("blocked.hdr")) == false);

  // the compressed file cannot be created (a directory of that name): no
  // sidecar file is left behind (a compressed file that cannot be written
  // when it is closed is tested in testNIFTIFormat)
  QVERIFY(QDir(m_tmp.path()).mkdir("blocked.nii.gz"));
  CNIFTIConverter::Options sidecarOptions;
  sidecarOptions.jsonSidecar = true;
  QVERIFY(CNIFTIConverter::ecatToNifti(tmpFile("error.v"), tmpFile("blocked.nii.gz"), sidecarOptions, &error) == false);
  QVERIFY(QFileInfo(tmpFile("blocked.nii.gz")).isDir());
  QVERIFY(QFile::exists(tmpFile("blocked.json")) == false);

  // NIfTI pair without image file
  QVERIFY2(CNIFTIConverter::ecatToNifti(tmpFile("error.v"), tmpFile("noimage.hdr"), CNIFTIConverter::Options(), &error), qPrintable(error));
  QVERIFY(QFile::remove(tmpFile("noimage.img")));
  QVERIFY(CNIFTIConverter::niftiToEcat(tmpFile("noimage.hdr"), tmpFile("noimage.v"), &error) == false);
  QVERIFY(QFile::exists(tmpFile("noimage.v")) == false);

  // the ECAT7 main header defines more frames (num_Frames) than the file
  // contains: no incomplete NIfTI file is written
  {
    CECATFile file(tmpFile("frames.v"), CECATMainHeader::ECAT7_Volume16);
    QVERIFY(file.open(QIODevice::WriteOnly));

    CECAT7MainHeader mainHeader(&file, CECATMainHeader::ECAT7_Volume16);
    mainHeader.setNum_Frames(3);
    mainHeader.setNum_Planes(2);
    mainHeader.setCalibration_Factor(1.0f);
    QVERIFY(file.writeMainHeader(mainHeader));

    QByteArray matrix(4 * 3 * 2 * sizeof(qint16), '\1');
    for(short f=1; f <= 2; f++) {
      CECAT7SubHeaderImage subHeader(&file);
      subHeader.setData_Type(CECATSubHeader::SunShort);
      subHeader.setNum_Dimensions(3);
      subHeader.setX_Dimension(4);
      subHeader.setY_Dimension(3);
      subHeader.setZ_Dimension(2);
      subHeader.setScale_Factor(f);
      QVERIFY(file.writeSubHeader(subHeader, f));
      QVERIFY(file.writeMatrix(matrix, f));
    }
    file.close();
  }
  {
    // CECATFile keeps num_Frames in sync with the frames: set it in the file
    // (big endian short at offset 354 of the ECAT7 main header)
    QFile file(tmpFile("frames.v"));
    const char numFrames[2] = { 0, 3 };
    QVERIFY(file.open(QIODevice::ReadWrite) && file.seek(354) && file.write(numFrames, 2) == 2);
    file.close();

    CECATFile ecat(tmpFile("frames.v"));
    CECATMainHeader* mainHeader = NULL;
    QVERIFY(ecat.open(QIODevice::ReadOnly) && ecat.readMainHeader(mainHeader) && mainHeader != NULL);
    QCOMPARE(static_cast<int>(static_cast<CECAT7MainHeader*>(mainHeader)->num_Frames()), 3);
    QCOMPARE(ecat.frameNumbers().size(), 2);
    delete mainHeader;
    ecat.close();
  }
  QVERIFY(CNIFTIConverter::ecatToNifti(tmpFile("frames.v"), tmpFile("frames.nii"), CNIFTIConverter::Options(), &error) == false);
  QVERIFY(QFile::exists(tmpFile("frames.nii")) == false);
}

QTEST_GUILESS_MAIN(TestNIFTIConversion)
#include "testNIFTIConversion.moc"
