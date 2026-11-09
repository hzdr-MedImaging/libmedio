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

// Tests of the format independent matrix access of CMedIOData
// (readMatrix()/writeMatrix() in the representation of other formats,
// readVolume()/writeVolume()).
//
// The expected values are independent of the conversion:
// - the values of testsuite/data/<format>/<file>.json generated with nibabel,
// - the files of the same dataset in different formats, written by other
//   tools (ROVER, PMOD, ecat2nifti), see testsuite/data/datasets.json,
// - positions computed in the test from the documented geometry:
//
//   ECAT7 voxel (i, j, k) is located at (LPS, mm)
//     x = x_offset + (i - (nx-1)/2) * px
//     y = y_offset + (j - (ny-1)/2) * py
//     z = bed position + z_offset - k * pz

#include <CMedIOData.h>
#include <CMedIOHeader.h>
#include <CMedIOVolume.h>
#include <CMedIODataFactory.h>
#include <CECATFile.h>
#include <CECATMainHeader.h>
#include <CECATSubHeader.h>
#include <CECAT7MainHeader.h>
#include <CECAT7SubHeaderImage.h>
#include <CNIFTIFile.h>
#include <CNIFTIMainHeader.h>
#include <CNIFTI1MainHeader.h>
#include <CNIFTI2MainHeader.h>
#include <CPhilipsFile.h>
#include <CPhilipsMainHeader.h>
#include <CPhilipsSubHeader.h>
#include <CPhilipsSubHeaderImage.h>
#include <CConcordeFile.h>
#include <CConcordeMainHeader.h>
#include <CConcordeFrameHeader.h>
#include <CNIFTIConverter.h>

#include "MedIOTestData.h"

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QJsonArray>
#include <QTextStream>
#include <QtEndian>

#include <cmath>
#include <cstring>
#include <limits>

#ifndef NIFTI_TEST_DATA_DIR
#error "NIFTI_TEST_DATA_DIR has to be defined"
#endif

//=============================================================================================
// helpers

static bool approx(double a, double b, double relative, double absolute = 0.0)
{
  return std::fabs(a - b) <= qMax(absolute, relative * qMax(std::fabs(a), std::fabs(b)));
}

static double ecatValue(const char* data, qint64 i, int type)
{
  switch(type) {
    case CECATSubHeader::ByteData:  return reinterpret_cast<const quint8*>(data)[i];
    case CECATSubHeader::SunLong:   return reinterpret_cast<const qint32*>(data)[i];
    case CECATSubHeader::IEEEFloat: return reinterpret_cast<const float*>(data)[i];
  }
  return reinterpret_cast<const qint16*>(data)[i];
}

static QJsonObject expectedValues(const QString& file)
{
  QString error;
  return medioReadJson(medioTestDataDir() + "/" + file + ".json", &error);
}

// ECAT7 image file read with CECATFile
struct EcatFile
{
  CECAT7MainHeader* mainHeader;
  QList<CECAT7SubHeaderImage*> subHeaders;
  QList<QByteArray> matrices;   // without padding

  EcatFile() : mainHeader(NULL) { }
  ~EcatFile() { delete mainHeader; qDeleteAll(subHeaders); }

  int frames() const { return subHeaders.size(); }
  qint64 voxels() const
  { return static_cast<qint64>(subHeaders[0]->x_Dimension()) * subHeaders[0]->y_Dimension() * subHeaders[0]->z_Dimension(); }

  // physical value of voxel i of frame f (0-based)
  double value(int f, qint64 i) const
  {
    const double scale = subHeaders[f]->scale_Factor() != 0.0 ? subHeaders[f]->scale_Factor() : 1.0;
    const double calibration = mainHeader->calibration_Factor() > 0.0 ? mainHeader->calibration_Factor() : 1.0;
    return ecatValue(matrices[f].constData(), i, subHeaders[f]->data_Type()) * scale * calibration;
  }

  // LPS position of voxel (i, j, k) following the documented convention
  void position(int f, double i, double j, double k, double p[3]) const
  {
    const CECAT7SubHeaderImage* s = subHeaders[f];
    p[0] = s->x_Offset() * 10.0 + (i - (s->x_Dimension() - 1) / 2.0) * s->x_Pixel_Size() * 10.0;
    p[1] = s->y_Offset() * 10.0 + (j - (s->y_Dimension() - 1) / 2.0) * s->y_Pixel_Size() * 10.0;
    p[2] = (mainHeader->init_Bed_Position() + s->z_Offset()) * 10.0 - k * s->z_Pixel_Size() * 10.0;
  }

  bool read(const QString& path)
  {
    CECATFile file(path);
    if(file.open(QIODevice::ReadOnly) == false)
      return false;
    CECATMainHeader* mh = NULL;
    if(file.readMainHeader(mh) == false || mh == NULL || mh->mainHeaderType() != CECATMainHeader::ECAT7MainHeader) {
      delete mh;
      return false;
    }
    mainHeader = static_cast<CECAT7MainHeader*>(mh);

    for(int f=1; f <= qMax(1, static_cast<int>(mainHeader->num_Frames())); f++) {
      QByteArray* data = NULL;
      CECATSubHeader* sh = NULL;
      if(file.readMatrix(data, sh, f) == false || data == NULL || sh == NULL || sh->subHeaderType() != CECATSubHeader::ECAT7_Image) {
        delete data;
        delete sh;
        return false;
      }
      subHeaders.append(static_cast<CECAT7SubHeaderImage*>(sh));
      const int size = ecatSize(subHeaders.last()->data_Type()) * voxels();
      matrices.append(data->left(size));
      delete data;
    }
    return true;
  }

  static int ecatSize(int type)
  {
    switch(type) {
      case CECATSubHeader::ByteData:  return 1;
      case CECATSubHeader::SunLong:
      case CECATSubHeader::IEEEFloat: return 4;
    }
    return 2;
  }
};

// create an ECAT7 file and write the given main header (linked to the file)
static bool createEcat(CECATFile& file, const CECAT7MainHeader& mainHeader)
{
  if(file.open(QIODevice::WriteOnly) == false)
    return false;
  CECAT7MainHeader mh(mainHeader);
  mh.setMedIOData(&file);
  return file.writeMainHeader(mh);
}

// matrix of frame f of an ECAT7 file (without padding), empty if it cannot be read
static QByteArray ecatMatrix(const QString& path, short f)
{
  CECATFile file(path);
  QByteArray* data = NULL;
  CECATSubHeader* sh = NULL;
  QByteArray result;
  if(file.open(QIODevice::ReadOnly) && file.readMatrix(data, sh, f) && data && sh &&
     sh->subHeaderType() == CECATSubHeader::ECAT7_Image) {
    const CECAT7SubHeaderImage* s = static_cast<const CECAT7SubHeaderImage*>(sh);
    const qint64 voxels = static_cast<qint64>(s->x_Dimension()) * s->y_Dimension() * s->z_Dimension();
    result = data->left(EcatFile::ecatSize(s->data_Type()) * voxels);
  }
  delete data;
  delete sh;
  return result;
}


//---------------------------------------------------------------------------------------------
// NIfTI helpers

static double niftiValue(const char* data, qint64 i, int datatype)
{
  switch(datatype) {
    case 2:   return reinterpret_cast<const quint8*>(data)[i];
    case 256: return reinterpret_cast<const qint8*>(data)[i];
    case 4:   return reinterpret_cast<const qint16*>(data)[i];
    case 512: return reinterpret_cast<const quint16*>(data)[i];
    case 8:   return reinterpret_cast<const qint32*>(data)[i];
    case 768: return reinterpret_cast<const quint32*>(data)[i];
    case 16:  return reinterpret_cast<const float*>(data)[i];
    case 64:  return reinterpret_cast<const double*>(data)[i];
  }
  return std::nan("");
}

static int niftiSize(int datatype)
{
  switch(datatype) {
    case 2: case 256: return 1;
    case 4: case 512: return 2;
    case 8: case 16: case 768: return 4;
    case 64: return 8;
  }
  return 0;
}

static void setNiftiValue(char* data, qint64 i, int datatype, double v)
{
  switch(datatype) {
    case 2:   reinterpret_cast<quint8*>(data)[i] = static_cast<quint8>(v); break;
    case 256: reinterpret_cast<qint8*>(data)[i] = static_cast<qint8>(v); break;
    case 4:   reinterpret_cast<qint16*>(data)[i] = static_cast<qint16>(v); break;
    case 512: reinterpret_cast<quint16*>(data)[i] = static_cast<quint16>(v); break;
    case 8:   reinterpret_cast<qint32*>(data)[i] = static_cast<qint32>(v); break;
    case 768: reinterpret_cast<quint32*>(data)[i] = static_cast<quint32>(v); break;
    case 16:  reinterpret_cast<float*>(data)[i] = static_cast<float>(v); break;
    case 64:  reinterpret_cast<double*>(data)[i] = v; break;
  }
}

// voxel value of linear index i in the nibabel reference files (see testsuite/nifti/data/generate_fixtures.py)
static double patternValue(qint64 i, int datatype)
{
  switch(datatype) {
    case 16:
    case 64:  return i * 0.25 - 2.0;
    case 2:   return (i * 7) % 50;
    default:  return (i * 7) % 50 - 10;
  }
}

// header values of both NIfTI versions
struct NiftiHeader
{
  qint64 dim[8];
  int datatype;
  double slope, inter;
  double srow[3][4];
  int sformCode;
};

template <class H>
static NiftiHeader niftiHeaderOf(const H* h)
{
  NiftiHeader n;
  for(short i=0; i < 8; i++)
    n.dim[i] = h->dim(i);
  n.datatype = h->dataType();
  n.slope = h->scl_Slope();
  n.inter = h->scl_Inter();
  n.sformCode = h->sform_Code();
  for(short i=0; i < 4; i++) {
    n.srow[0][i] = h->srow_X(i);
    n.srow[1][i] = h->srow_Y(i);
    n.srow[2][i] = h->srow_Z(i);
  }
  return n;
}

static NiftiHeader niftiHeader(const CMedIOHeader* h)
{
  const CNIFTIMainHeader* n = static_cast<const CNIFTIMainHeader*>(h);
  if(n->mainHeaderType() == CNIFTIMainHeader::NIFTI1MainHeader)
    return niftiHeaderOf(static_cast<const CNIFTI1MainHeader*>(n));
  return niftiHeaderOf(static_cast<const CNIFTI2MainHeader*>(n));
}

// LPS position (mm) of NIfTI voxel v using a RAS affine (mm)
static void niftiPosition(const double a[3][4], const double v[3], double p[3])
{
  for(int r=0; r < 3; r++)
    p[r] = a[r][0] * v[0] + a[r][1] * v[1] + a[r][2] * v[2] + a[r][3];
  p[0] = -p[0];
  p[1] = -p[1];
}

// NIfTI voxel (possibly fractional) at LPS position p (mm) using a RAS affine (mm)
static void niftiVoxel(const double a[3][4], const double p[3], double v[3])
{
  const double ras[3] = { -p[0] - a[0][3], -p[1] - a[1][3], p[2] - a[2][3] };
  const double det = a[0][0]*(a[1][1]*a[2][2]-a[1][2]*a[2][1]) - a[0][1]*(a[1][0]*a[2][2]-a[1][2]*a[2][0]) + a[0][2]*(a[1][0]*a[2][1]-a[1][1]*a[2][0]);
  for(int i=0; i < 3; i++) {
    v[i] = 0.0;
    for(int j=0; j < 3; j++) {
      const int i1 = (j + 1) % 3, i2 = (j + 2) % 3, j1 = (i + 1) % 3, j2 = (i + 2) % 3;
      v[i] += (a[i1][j1] * a[i2][j2] - a[i1][j2] * a[i2][j1]) / det * ras[j];
    }
  }
}

// description of a synthetic NIfTI file
struct Synthetic
{
  Synthetic() : version(1), nx(4), ny(3), nz(2), frames(1), dim5(1), useQform(false), units(10),
                datatype(4), slope(1.0), inter(0.0), pixdim4(0.0), toffset(0.0),
                byteOrder(QSysInfo::ByteOrder)
  {
    const double lps[3][4] = { { -2, 0, 0, 10 }, { 0, -2.5, 0, -20 }, { 0, 0, 3, -30 } };
    memcpy(affine, lps, sizeof(affine));
  }

  int version, nx, ny, nz, frames, dim5;
  double affine[3][4]; // RAS, in file units
  bool useQform;       // affine is a rotation about z by 180 degrees (see setHeader())
  int units, datatype;
  double slope, inter, pixdim4, toffset;
  QSysInfo::Endian byteOrder;

  qint64 voxels() const { return static_cast<qint64>(nx) * ny * nz; }
};

template <class H>
static void setHeader(H* h, const Synthetic& s)
{
  double size[3];
  for(int j=0; j < 3; j++)
    size[j] = std::sqrt(s.affine[0][j]*s.affine[0][j] + s.affine[1][j]*s.affine[1][j] + s.affine[2][j]*s.affine[2][j]);

  h->setDim(0, s.dim5 > 1 ? 5 : (s.frames > 1 ? 4 : 3));
  h->setDim(1, s.nx); h->setDim(2, s.ny); h->setDim(3, s.nz); h->setDim(4, s.frames); h->setDim(5, s.dim5);
  for(short i=6; i < 8; i++) h->setDim(i, 1);
  for(short i=0; i < 8; i++) h->setPix_Dim(i, 1.0);
  for(short i=1; i <= 3; i++) h->setPix_Dim(i, size[i-1]);
  h->setPix_Dim(4, s.pixdim4);
  h->setToffset(s.toffset);
  h->setXyzt_Units(s.units);
  h->setDataType(s.datatype);
  h->setBit_Pix(s.datatype == 32 ? 64 : niftiSize(s.datatype) * 8);
  h->setScl_Slope(s.slope);
  h->setScl_Inter(s.inter);
  h->setByteOrder(s.byteOrder);

  if(s.useQform) {
    // rotation by 180 degrees about z (quaternion (0, 0, 1)), qfac = sign of affine[2][2]
    h->setSform_Code(0);
    h->setQform_Code(1);
    h->setQuatern_B(0.0); h->setQuatern_C(0.0); h->setQuatern_D(1.0);
    h->setPix_Dim(0, s.affine[2][2] < 0 ? -1.0 : 1.0);
    h->setQoffset_X(s.affine[0][3]); h->setQoffset_Y(s.affine[1][3]); h->setQoffset_Z(s.affine[2][3]);
  } else {
    h->setQform_Code(0);
    h->setSform_Code(2);
    for(short i=0; i < 4; i++) {
      h->setSrow_X(i, s.affine[0][i]);
      h->setSrow_Y(i, s.affine[1][i]);
      h->setSrow_Z(i, s.affine[2][i]);
    }
  }
}

// open a NIfTI file for writing and write the main header of s
static bool createNifti(CNIFTIFile& file, const Synthetic& s)
{
  if(file.open(QIODevice::WriteOnly) == false)
    return false;

  QScopedPointer<CNIFTIMainHeader> header(file.createEmptyHeader());
  if(header.data() == NULL)
    return false;

  if(s.version == 2)
    setHeader(static_cast<CNIFTI2MainHeader*>(header.data()), s);
  else
    setHeader(static_cast<CNIFTI1MainHeader*>(header.data()), s);

  return file.writeMainHeader(*header);
}

// write a synthetic NIfTI file with the given stored values (machine byte order)
static bool writeNifti(const QString& path, const Synthetic& s, const QByteArray& data)
{
  CNIFTIFile file(path, s.version == 2 ? CNIFTIMainHeader::NIFTI2 : CNIFTIMainHeader::NIFTI1);
  bool result = createNifti(file, s) && file.writeMatrix(data);
  file.close();
  return result;
}

// create a NIfTI file with a main header converted from another one
static bool createNifti(CNIFTIFile& file, const CMedIOHeader* header)
{
  if(file.open(QIODevice::WriteOnly) == false)
    return false;
  QScopedPointer<CNIFTIMainHeader> h(file.createEmptyHeader());
  return h.data() && h->convertFrom(header) && file.writeMainHeader(*h);
}

// complete voxel data of a NIfTI file (machine byte order) and its header
static QByteArray readAll(const QString& path, NiftiHeader* header = NULL)
{
  CNIFTIFile file(path);
  QByteArray* data = NULL;
  CNIFTIMainHeader* h = NULL;
  if(file.open(QIODevice::ReadOnly) == false || file.readMatrix(data) == false || data == NULL || file.readMainHeader(h) == false)
    return QByteArray();
  if(header)
    *header = niftiHeader(h);
  delete h;
  QByteArray result(*data);
  delete data;
  return result;
}

//=============================================================================================

class TestMedIOMatrix : public QObject
{
  Q_OBJECT

  private:
    QTemporaryDir m_tmp;
    QString tmpFile(const QString& name) const { return m_tmp.filePath(name); }

  private slots:
    void initTestCase();

    void ecatNative_data();
    void ecatNative();
    void ecatVolume_data();
    void ecatVolume();
    void ecatWriteVolume();
    void ecatDefaultFrame();
    void writeReorientedVolume();

    void niftiReferenceFiles_data();
    void niftiReferenceFiles();
    void niftiOrientations_data();
    void niftiOrientations();
    void niftiDataTypes_data();
    void niftiDataTypes();
    void niftiDatasets_data();
    void niftiDatasets();
    void ecatToNiftiFile_data();
    void ecatToNiftiFile();
    void niftiToEcatFile();
    void niftiRequantized();
    void niftiErrors();

    void philipsDatasets_data();
    void philipsDatasets();
    void writePhilips_data();
    void writePhilips();
    void philipsSliceScales_data();
    void philipsSliceScales();
    void philipsErrors();

    void concorde();

    void frameNumbersRead_data();
    void frameNumbersRead();
    void frameNumbersCount_data();
    void frameNumbersCount();
    void frameNumbersToNifti_data();
    void frameNumbersToNifti();

    void nativeFormat_data();
    void nativeFormat();

    void writeWithFormat();
    void writeMainHeaderFormat();

    void errors();
};

void TestMedIOMatrix::initTestCase()
{
  QVERIFY(m_tmp.isValid());
}

//---------------------------------------------------------------------------------------------
// ECAT7 matrices requested in the ECAT7 format: unchanged

void TestMedIOMatrix::ecatNative_data()
{
  QTest::addColumn<QString>("file");
  QTest::newRow("pet-static.v")  << "ecat/pet-static.v";
  QTest::newRow("pet-dynamic.v") << "ecat/pet-dynamic.v";
}

void TestMedIOMatrix::ecatNative()
{
  QFETCH(QString, file);

  CECATFile ecat(medioTestDataDir() + "/" + file);
  QVERIFY(ecat.open(QIODevice::ReadOnly));

  CMedIOHeader* mh = NULL;
  QVERIFY(ecat.readMainHeader(mh, CMedIOData::ECAT));
  QScopedPointer<CMedIOHeader> mhGuard(mh);
  QCOMPARE(static_cast<int>(mh->headerFormat()), static_cast<int>(CMedIOHeader::ECATMainHeader));
  CECATMainHeader* native = NULL;
  QVERIFY(ecat.readMainHeader(native));
  QCOMPARE(static_cast<CECAT7MainHeader*>(mh)->calibration_Factor(), static_cast<CECAT7MainHeader*>(native)->calibration_Factor());
  QCOMPARE(static_cast<CECAT7MainHeader*>(mh)->num_Frames(), static_cast<CECAT7MainHeader*>(native)->num_Frames());
  delete native;

  for(int f=1; f <= static_cast<CECAT7MainHeader*>(mh)->num_Frames(); f++) {
    QByteArray* data = NULL;
    CMedIOHeader* sh = NULL;
    QVERIFY(ecat.readMatrix(data, sh, CMedIOData::ECAT, f));
    QByteArray* nativeData = NULL;
    CECATSubHeader* nativeHeader = NULL;
    QVERIFY(ecat.readMatrix(nativeData, nativeHeader, f));

    QCOMPARE(*data, *nativeData);
    QCOMPARE(static_cast<int>(sh->headerFormat()), static_cast<int>(CMedIOHeader::ECATSubHeader));
    QCOMPARE(static_cast<CECAT7SubHeaderImage*>(sh)->scale_Factor(), static_cast<CECAT7SubHeaderImage*>(nativeHeader)->scale_Factor());
    delete data; delete sh; delete nativeData; delete nativeHeader;
  }
}

//---------------------------------------------------------------------------------------------
// readVolume() of ECAT7 files: compared with the values of nibabel

void TestMedIOMatrix::ecatVolume_data()
{
  ecatNative_data();
}

void TestMedIOMatrix::ecatVolume()
{
  QFETCH(QString, file);

  const QJsonObject expected = expectedValues(file);
  QVERIFY(expected.isEmpty() == false);
  const double calibration = expected["main_header"].toObject()["calibration_factor"].toDouble();
  const QJsonArray matrices = expected["matrices"].toArray();

  EcatFile reference;
  QVERIFY(reference.read(medioTestDataDir() + "/" + file));

  QScopedPointer<CMedIOData> data(CMedIODataFactory::create(medioTestDataDir() + "/" + file));
  QVERIFY(data.data() != NULL);
  QVERIFY(data->open(QIODevice::ReadOnly));

  for(int f=0; f < matrices.size(); f++) {
    const QJsonObject m = matrices[f].toObject();
    CMedIOVolume v;
    QVERIFY(data->readVolume(v, f + 1));

    const QJsonArray dims = m["dims"].toArray();
    for(int a=0; a < 3; a++)
      QCOMPARE(v.dimension(a), dims[a].toInt());
    QCOMPARE(v.dataType(), CMedIOVolume::Int16);
    QCOMPARE(v.orientation(), QString("LPI"));
    QVERIFY(v.hasUniformScaleFactor());
    QVERIFY(approx(v.scaleFactor(), m["scale_factor"].toDouble() * calibration, 1e-6));
    QCOMPARE(v.frameStart(), m["frame_start_time_ms"].toDouble() / 1000.0);
    QCOMPARE(v.frameDuration(), m["frame_duration_ms"].toDouble() / 1000.0);

    double sum = 0.0, lo = 0.0, hi = 0.0;
    for(qint64 i=0; i < v.voxelCount(); i++) {
      const double s = v.storedValue(i);
      sum += s;
      lo = (i == 0) ? s : qMin(lo, s);
      hi = (i == 0) ? s : qMax(hi, s);
    }
    QCOMPARE(sum, m["raw_sum"].toDouble());
    QCOMPARE(lo, m["raw_min"].toDouble());
    QCOMPARE(hi, m["raw_max"].toDouble());

    const QJsonArray size = m["pixel_size_cm"].toArray();
    for(int a=0; a < 3; a++)
      QVERIFY(approx(v.voxelSize(a), size[a].toDouble() * 10.0, 1e-6));

    // geometry of the documented convention
    const int corners[2][3] = { { 0, 0, 0 }, { v.dimension(0) - 1, v.dimension(1) - 1, v.dimension(2) - 1 } };
    for(int c=0; c < 2; c++) {
      double p[3], q[3];
      v.position(corners[c][0], corners[c][1], corners[c][2], p);
      reference.position(f, corners[c][0], corners[c][1], corners[c][2], q);
      for(int r=0; r < 3; r++)
        QVERIFY(approx(p[r], q[r], 0.0, 1e-3));
    }
  }
}

//---------------------------------------------------------------------------------------------
// writeVolume() into ECAT7 files with the same and with another calibration factor

void TestMedIOMatrix::ecatWriteVolume()
{
  EcatFile reference;
  QVERIFY(reference.read(medioTestDataDir() + "/ecat/pet-dynamic.v"));
  QCOMPARE(static_cast<double>(reference.mainHeader->calibration_Factor()), 1000.0);

  for(int pass=0; pass < 2; pass++) {
    const QString path = tmpFile(QString("volume-%1.v").arg(pass));
    {
      QScopedPointer<CMedIOData> in(CMedIODataFactory::create(medioTestDataDir() + "/ecat/pet-dynamic.v"));
      QVERIFY(in->open(QIODevice::ReadOnly));

      CECAT7MainHeader mainHeader(*reference.mainHeader);
      if(pass == 1)
        mainHeader.setCalibration_Factor(1.0f);
      CECATFile out(path, CECATMainHeader::ECAT7_Volume16);
      QVERIFY(createEcat(out, mainHeader));

      for(int f=reference.frames(); f >= 1; f--) {
        CMedIOVolume v;
        QVERIFY(in->readVolume(v, f));
        QVERIFY(out.writeVolume(v, f));
      }
      out.close();
    }

    EcatFile written;
    QVERIFY(written.read(path));
    QCOMPARE(written.frames(), reference.frames());
    for(int f=0; f < reference.frames(); f++) {
      // same stored values, scale factor adjusted to the calibration factor
      QVERIFY(written.matrices[f] == reference.matrices[f]);
      QCOMPARE(static_cast<int>(written.subHeaders[f]->data_Type()), static_cast<int>(CECATSubHeader::SunShort));
      QVERIFY(approx(written.subHeaders[f]->scale_Factor() * written.mainHeader->calibration_Factor(),
                     reference.subHeaders[f]->scale_Factor() * reference.mainHeader->calibration_Factor(), 1e-6));
      QCOMPARE(written.subHeaders[f]->frame_Start_Time(), reference.subHeaders[f]->frame_Start_Time());
      QCOMPARE(written.subHeaders[f]->frame_Duration(), reference.subHeaders[f]->frame_Duration());

      double p[3], q[3];
      written.position(f, 3, 5, 7, p);
      reference.position(f, 3, 5, 7, q);
      for(int r=0; r < 3; r++)
        QVERIFY(approx(p[r], q[r], 0.0, 1e-3));
    }
  }
}

//---------------------------------------------------------------------------------------------
// frame -1 written into an ECAT7 file is frame 1, as for NIfTI and Philips,
// also into a file without frames (CECATFile itself takes frame 0 there)

void TestMedIOMatrix::ecatDefaultFrame()
{
  const QString source = medioTestDataDir() + "/ecat/pet-static.v";
  EcatFile reference;
  QVERIFY(reference.read(source));

  CECATFile in(source);
  QVERIFY(in.open(QIODevice::ReadOnly));
  CMedIOHeader* mh = NULL;
  QVERIFY(in.readMainHeader(mh, CMedIOData::ECAT));
  QScopedPointer<CMedIOHeader> mhGuard(mh);

  // pass 0: ECAT7 matrix (written unchanged), 1: NIfTI matrix (converted), 2: volume
  for(int pass=0; pass < 3; pass++) {
    const QString path = tmpFile(QString("default-frame-%1.v").arg(pass));
    {
      CECATFile out(path, CECATMainHeader::ECAT7_Volume16);
      QVERIFY(createEcat(out, *static_cast<CECAT7MainHeader*>(mh)));
      if(pass < 2) {
        QByteArray* data = NULL;
        CMedIOHeader* sh = NULL;
        QVERIFY(in.readMatrix(data, sh, pass == 0 ? CMedIOData::ECAT : CMedIOData::NIFTI, 1));
        QScopedPointer<QByteArray> dataGuard(data);
        QScopedPointer<CMedIOHeader> shGuard(sh);
        QVERIFY(out.writeMatrix(*data, *sh));
      } else {
        CMedIOVolume volume;
        QVERIFY(in.readVolume(volume, 1));
        QVERIFY(out.writeVolume(volume));
      }
      out.close();
    }
    QVERIFY(ecatMatrix(path, 0).isEmpty());
    const QByteArray written = ecatMatrix(path, 1);
    QVERIFY(written.isEmpty() == false);
    if(pass == 0)
      QVERIFY(written == reference.matrices[0]);
  }
}

//---------------------------------------------------------------------------------------------
// a volume in RAS voxel order written into an ECAT7 file is reordered; every
// value keeps its position

void TestMedIOMatrix::writeReorientedVolume()
{
  // 4 x 3 x 2 voxels, axes towards right, anterior, superior (LPS affine), value = 1 + linear index
  const double ras[3][4] = { { -2, 0, 0, 10 }, { 0, -2.5, 0, -20 }, { 0, 0, 3, -30 } };
  QByteArray values(24 * 4, '\0');
  for(int i=0; i < 24; i++)
    reinterpret_cast<qint32*>(values.data())[i] = 1 + i;
  CMedIOVolume volume;
  QVERIFY(volume.setData(4, 3, 2, CMedIOVolume::Int32, values));
  volume.setAffine(ras);
  volume.setScaleFactor(0.5);
  volume.setFrameStart(30.0);
  volume.setFrameDuration(60.0);

  const QString path = tmpFile("reoriented.v");
  {
    CECATFile out(path, CECATMainHeader::ECAT7_Volume16);
    CECAT7MainHeader mainHeader(NULL, CECATMainHeader::ECAT7_Volume16);
    mainHeader.setInit_Bed_Position(-5.0f);
    mainHeader.setCalibration_Factor(1.0f);
    QVERIFY(createEcat(out, mainHeader));
    QVERIFY(out.writeVolume(volume, 1));
    out.close();
  }

  EcatFile written;
  QVERIFY(written.read(path));
  const CECAT7SubHeaderImage* s = written.subHeaders[0];
  QCOMPARE(static_cast<int>(s->data_Type()), static_cast<int>(CECATSubHeader::SunLong));
  QCOMPARE(static_cast<double>(s->scale_Factor()), 0.5);
  QCOMPARE(s->frame_Start_Time(), 30000u);
  QCOMPARE(s->frame_Duration(), 60000u);
  QCOMPARE(s->x_Dimension() * s->y_Dimension() * s->z_Dimension(), 24);

  for(int k=0; k < s->z_Dimension(); k++)
    for(int j=0; j < s->y_Dimension(); j++)
      for(int i=0; i < s->x_Dimension(); i++) {
        double p[3];
        written.position(0, i, j, k, p);
        // voxel of the volume at this position: inverse of the diagonal affine above
        const double v[3] = { (p[0] - 10.0) / -2.0, (p[1] + 20.0) / -2.5, (p[2] + 30.0) / 3.0 };
        for(int r=0; r < 3; r++)
          QVERIFY2(approx(v[r], qRound(v[r]), 0.0, 1e-4), qPrintable(QString("ECAT voxel (%1,%2,%3)").arg(i).arg(j).arg(k)));
        const double expected = (1 + qRound(v[0]) + 4 * (qRound(v[1]) + 3 * qRound(v[2]))) * 0.5;
        QCOMPARE(written.value(0, i + s->x_Dimension() * (j + s->y_Dimension() * k)), expected);
      }
}


//=============================================================================================
// NIfTI

//---------------------------------------------------------------------------------------------
// nibabel reference files: affine diag(-2, 2.5, 3) + (10, -20, -30) (RAS) for
// all files, scl_slope 0.5, scl_inter 1, i.e. NIfTI voxel (i, j, k) is ECAT7
// voxel (i, ny-1-j, nz-1-k) (y towards posterior, planes from superior to inferior)

void TestMedIOMatrix::niftiReferenceFiles_data()
{
  QTest::addColumn<QString>("file");
  QTest::addColumn<int>("frames");

  QTest::newRow("n1_le_int16.nii")          << "n1_le_int16.nii" << 1;
  QTest::newRow("n1_be_float32_4d.nii")     << "n1_be_float32_4d.nii" << 2;
  QTest::newRow("n1_le_uint8.nii.gz")       << "n1_le_uint8.nii.gz" << 1;
  QTest::newRow("n2_le_float64.nii")        << "n2_le_float64.nii" << 1;
  QTest::newRow("n2_be_int32_ext.nii")      << "n2_be_int32_ext.nii" << 1;
  QTest::newRow("n2_le_int16_4d.nii.gz")    << "n2_le_int16_4d.nii.gz" << 2;
  QTest::newRow("p1_le_int16_ext.hdr")      << "p1_le_int16_ext.hdr" << 1;
  QTest::newRow("p2_be_float32_4d.hdr")     << "p2_be_float32_4d.hdr" << 2;
  QTest::newRow("p1_le_uint8.hdr.gz")       << "p1_le_uint8.hdr.gz" << 1;
  QTest::newRow("p1_le_int16_offset.img")   << "p1_le_int16_offset.img" << 1;
}

void TestMedIOMatrix::niftiReferenceFiles()
{
  QFETCH(QString, file);
  QFETCH(int, frames);

  const int nx = 4, ny = 3, nz = 2;
  const qint64 voxels = nx * ny * nz;
  const double affine[3][4] = { { -2, 0, 0, 10 }, { 0, 2.5, 0, -20 }, { 0, 0, 3, -30 } };

  QScopedPointer<CMedIOData> nifti(CMedIODataFactory::create(QString(NIFTI_TEST_DATA_DIR) + "/" + file));
  QVERIFY(nifti.data() != NULL);
  QCOMPARE(static_cast<int>(nifti->dataFormat()), static_cast<int>(CMedIOData::NIFTI));
  QVERIFY(nifti->open(QIODevice::ReadOnly));

  CMedIOHeader* mh = NULL;
  QVERIFY(nifti->readMainHeader(mh, CMedIOData::ECAT));
  QScopedPointer<CMedIOHeader> mhGuard(mh);
  QCOMPARE(static_cast<int>(static_cast<CECAT7MainHeader*>(mh)->num_Frames()), frames);
  const double bed = static_cast<CECAT7MainHeader*>(mh)->init_Bed_Position() * 10.0;

  for(int f=1; f <= frames; f++) {
    const qint64 first = (f - 1) * voxels;

    // NIfTI representation of the file itself: one volume in the voxel order and datatype of the file
    QByteArray* native = NULL;
    CMedIOHeader* nativeHeader = NULL;
    QVERIFY(nifti->readMatrix(native, nativeHeader, CMedIOData::NIFTI, f));
    QScopedPointer<QByteArray> nativeGuard(native);
    QScopedPointer<CMedIOHeader> nativeHeaderGuard(nativeHeader);
    const NiftiHeader n = niftiHeader(nativeHeader);
    QCOMPARE(native->size(), static_cast<int>(voxels * niftiSize(n.datatype)));
    for(qint64 i=0; i < voxels; i++)
      QCOMPARE(niftiValue(native->constData(), i, n.datatype), patternValue(first + i, n.datatype));

    // volume in the voxel order of the file
    CMedIOVolume volume;
    QVERIFY(nifti->readVolume(volume, f));
    QCOMPARE(volume.orientation(), QString("LAS"));
    QCOMPARE(volume.scaleFactor(), 0.5);
    QCOMPARE(volume.intercept(), 1.0);
    for(qint64 i=0; i < voxels; i++)
      QCOMPARE(volume.storedValue(i), patternValue(first + i, n.datatype));

    // ECAT7 representation: scl_inter -> physical values as float32
    QByteArray* data = NULL;
    CMedIOHeader* sh = NULL;
    QVERIFY(nifti->readMatrix(data, sh, CMedIOData::ECAT, f));
    QScopedPointer<QByteArray> dataGuard(data);
    QScopedPointer<CMedIOHeader> shGuard(sh);
    QCOMPARE(static_cast<int>(sh->headerFormat()), static_cast<int>(CMedIOHeader::ECATSubHeader));
    const CECAT7SubHeaderImage* s = static_cast<const CECAT7SubHeaderImage*>(sh);
    QCOMPARE(static_cast<int>(s->data_Type()), static_cast<int>(CECATSubHeader::IEEEFloat));
    QCOMPARE(static_cast<double>(s->scale_Factor()), 1.0);
    QCOMPARE(s->x_Dimension(), nx); QCOMPARE(s->y_Dimension(), ny); QCOMPARE(s->z_Dimension(), nz);

    for(int k=0; k < nz; k++)
      for(int j=0; j < ny; j++)
        for(int i=0; i < nx; i++) {
          const qint64 e = i + nx * (j + ny * k);
          const qint64 o = i + nx * ((ny - 1 - j) + ny * (nz - 1 - k));
          QVERIFY(approx(ecatValue(data->constData(), e, CECATSubHeader::IEEEFloat),
                         static_cast<float>(patternValue(first + o, n.datatype) * 0.5 + 1.0), 1e-6));

          // same position in both representations
          const double v[3] = { static_cast<double>(i), static_cast<double>(ny - 1 - j), static_cast<double>(nz - 1 - k) };
          double pn[3];
          niftiPosition(affine, v, pn);
          const double pe[3] = { s->x_Offset() * 10.0 + (i - (nx - 1) / 2.0) * s->x_Pixel_Size() * 10.0,
                                 s->y_Offset() * 10.0 + (j - (ny - 1) / 2.0) * s->y_Pixel_Size() * 10.0,
                                 bed + s->z_Offset() * 10.0 - k * s->z_Pixel_Size() * 10.0 };
          for(int r=0; r < 3; r++)
            QVERIFY2(approx(pe[r], pn[r], 0.0, 1e-3), qPrintable(QString("voxel (%1,%2,%3) axis %4").arg(i).arg(j).arg(k).arg(r)));
        }

    // char* variant
    char* raw = NULL;
    unsigned int len = 0;
    CMedIOHeader* sh2 = NULL;
    QVERIFY(nifti->readMatrix(raw, len, sh2, CMedIOData::ECAT, f));
    QCOMPARE(QByteArray(raw, len), *data);
    delete[] raw;
    delete sh2;
  }

  QByteArray* none = NULL;
  CMedIOHeader* noHeader = NULL;
  QVERIFY(nifti->readMatrix(none, noHeader, CMedIOData::ECAT, frames + 1) == false);
  QVERIFY(nifti->readMatrix(none, noHeader, CMedIOData::NIFTI, frames + 1) == false);
  QVERIFY(none == NULL && noHeader == NULL);
}

//---------------------------------------------------------------------------------------------
// synthetic NIfTI files with different orientations (2 frames, values 1 +
// linear index): ECAT7 representation, frame timing and writing back

void TestMedIOMatrix::niftiOrientations_data()
{
  QTest::addColumn<QVector<double> >("affine");
  QTest::addColumn<bool>("useQform");
  QTest::addColumn<int>("units");
  QTest::addColumn<double>("unitScale");
  QTest::addColumn<int>("version");

  // RAS affines (rows), voxel sizes 2, 2.5 and 3 mm
  QTest::newRow("RAS (dcm2niix, nibabel)") << (QVector<double>() << 2 << 0 << 0 << 10   << 0 << 2.5 << 0 << -20 << 0 << 0 << 3 << -30) << false << 10 << 1.0 << 1;
  QTest::newRow("LPS (libmedio)")          << (QVector<double>() << -2 << 0 << 0 << 10  << 0 << -2.5 << 0 << -20 << 0 << 0 << 3 << -30) << false << 10 << 1.0 << 2;
  QTest::newRow("LAI")                     << (QVector<double>() << -2 << 0 << 0 << 10  << 0 << 2.5 << 0 << -20 << 0 << 0 << -3 << -30) << false << 10 << 1.0 << 1;
  QTest::newRow("permuted axes (A, L, S)") << (QVector<double>() << 0 << -2.5 << 0 << 10 << 2 << 0 << 0 << -20 << 0 << 0 << 3 << -30) << false << 10 << 1.0 << 2;
  QTest::newRow("sagittal (S, P, L)")      << (QVector<double>() << 0 << 0 << -3 << 10  << 0 << -2.5 << 0 << -20 << 2 << 0 << 0 << -30) << false << 10 << 1.0 << 1;
  QTest::newRow("qform qfac 1")            << (QVector<double>() << -2 << 0 << 0 << 10  << 0 << -2.5 << 0 << -20 << 0 << 0 << 3 << -30) << true << 10 << 1.0 << 1;
  QTest::newRow("qform qfac -1")           << (QVector<double>() << -2 << 0 << 0 << 10  << 0 << -2.5 << 0 << -20 << 0 << 0 << -3 << -30) << true << 10 << 1.0 << 2;
  QTest::newRow("units m and ms")          << (QVector<double>() << 0.002 << 0 << 0 << 0.01 << 0 << 0.0025 << 0 << -0.02 << 0 << 0 << 0.003 << -0.03) << false << (1 | 16) << 1000.0 << 1;
}

void TestMedIOMatrix::niftiOrientations()
{
  QFETCH(QVector<double>, affine);
  QFETCH(bool, useQform);
  QFETCH(int, units);
  QFETCH(double, unitScale);
  QFETCH(int, version);

  Synthetic s;
  s.version = version;
  s.frames = 2;
  s.useQform = useQform;
  s.units = units;
  s.pixdim4 = 500.0;  // time units
  s.toffset = 1000.0;
  for(int i=0; i < 12; i++)
    s.affine[i / 4][i % 4] = affine[i];

  double a[3][4]; // in mm
  for(int i=0; i < 3; i++)
    for(int j=0; j < 4; j++)
      a[i][j] = s.affine[i][j] * unitScale;
  const double timeScale = (units & 0x38) == 16 ? 0.001 : 1.0;

  const qint64 voxels = s.voxels();
  QByteArray values(voxels * s.frames * 2, '\0');
  for(qint64 i=0; i < voxels * s.frames; i++)
    reinterpret_cast<qint16*>(values.data())[i] = static_cast<qint16>(i + 1);

  const QString name = QString("orientation-%1").arg(QTest::currentDataTag()).replace(QRegExp("[^a-zA-Z0-9-]"), "_");
  QVERIFY(writeNifti(tmpFile(name + ".nii"), s, values));

  CNIFTIFile nifti(tmpFile(name + ".nii"));
  QVERIFY(nifti.open(QIODevice::ReadOnly));
  CMedIOHeader* mh = NULL;
  QVERIFY(nifti.readMainHeader(mh, CMedIOData::ECAT));
  QScopedPointer<CMedIOHeader> mhGuard(mh);
  const double bed = static_cast<CECAT7MainHeader*>(mh)->init_Bed_Position() * 10.0;

  QList<QByteArray> ecatData, niftiData;
  QList<CMedIOHeader*> ecatHeaders, niftiHeaders;

  for(int f=1; f <= s.frames; f++) {
    QByteArray* data = NULL;
    CMedIOHeader* sh = NULL;
    QVERIFY(nifti.readMatrix(data, sh, CMedIOData::ECAT, f));
    ecatData.append(*data);
    ecatHeaders.append(sh);
    const CECAT7SubHeaderImage* sub = static_cast<const CECAT7SubHeaderImage*>(sh);

    QCOMPARE(static_cast<int>(sub->data_Type()), static_cast<int>(CECATSubHeader::SunShort));
    QCOMPARE(static_cast<double>(sub->scale_Factor()), 1.0);
    QCOMPARE(sub->frame_Start_Time(), static_cast<unsigned int>(qRound((1000.0 + (f - 1) * 500.0) * timeScale * 1000.0)));
    QCOMPARE(sub->frame_Duration(), static_cast<unsigned int>(qRound(500.0 * timeScale * 1000.0)));

    const int nx = sub->x_Dimension(), ny = sub->y_Dimension(), nz = sub->z_Dimension();
    QCOMPARE(static_cast<qint64>(nx) * ny * nz, voxels);

    // every ECAT7 voxel holds the value of the NIfTI voxel at the same position
    for(int k=0; k < nz; k++)
      for(int j=0; j < ny; j++)
        for(int i=0; i < nx; i++) {
          const double p[3] = { sub->x_Offset() * 10.0 + (i - (nx - 1) / 2.0) * sub->x_Pixel_Size() * 10.0,
                                sub->y_Offset() * 10.0 + (j - (ny - 1) / 2.0) * sub->y_Pixel_Size() * 10.0,
                                bed + sub->z_Offset() * 10.0 - k * sub->z_Pixel_Size() * 10.0 };
          double v[3];
          niftiVoxel(a, p, v);
          const int n[3] = { s.nx, s.ny, s.nz };
          for(int r=0; r < 3; r++)
            QVERIFY2(std::fabs(v[r] - qRound(v[r])) < 1e-3 && qRound(v[r]) >= 0 && qRound(v[r]) < n[r],
                     qPrintable(QString("ECAT voxel (%1,%2,%3) maps to NIfTI (%4,%5,%6)").arg(i).arg(j).arg(k).arg(v[0]).arg(v[1]).arg(v[2])));
          const double expected = 1 + qRound(v[0]) + s.nx * (qRound(v[1]) + s.ny * (qRound(v[2]) + s.nz * (f - 1)));
          QCOMPARE(ecatValue(data->constData(), i + nx * (j + ny * k), CECATSubHeader::SunShort), expected);
        }
    delete data;

    QVERIFY(nifti.readMatrix(data, sh, CMedIOData::NIFTI, f));
    niftiData.append(*data);
    niftiHeaders.append(sh);
    delete data;
  }

  // ECAT7 representation written into a NIfTI file with the same header (frames in any order)
  {
    CNIFTIFile out(tmpFile(name + ".ecat.nii.gz"), version == 2 ? CNIFTIMainHeader::NIFTI2 : CNIFTIMainHeader::NIFTI1);
    QVERIFY(createNifti(out, s));
    for(int f=s.frames; f >= 1; f--)
      QVERIFY(out.writeMatrix(ecatData[f-1].constData(), ecatData[f-1].size(), *ecatHeaders[f-1], f));
    out.close();
    QCOMPARE(readAll(tmpFile(name + ".ecat.nii.gz")), values);
  }

  // NIfTI representation written into an ECAT7 file: the same as the ECAT7 representation
  {
    CECATFile out(tmpFile(name + ".v"), CECATMainHeader::ECAT7_Volume16);
    QVERIFY(createEcat(out, *static_cast<CECAT7MainHeader*>(mh)));
    for(int f=1; f <= s.frames; f++)
      QVERIFY(out.writeMatrix(niftiData[f-1], *niftiHeaders[f-1], f));
    out.close();

    EcatFile written;
    QVERIFY(written.read(tmpFile(name + ".v")));
    QCOMPARE(written.frames(), s.frames);
    for(int f=0; f < s.frames; f++) {
      const CECAT7SubHeaderImage* e = static_cast<const CECAT7SubHeaderImage*>(ecatHeaders[f]);
      QCOMPARE(written.matrices[f], ecatData[f]);
      QCOMPARE(written.subHeaders[f]->frame_Start_Time(), e->frame_Start_Time());
      QCOMPARE(written.subHeaders[f]->frame_Duration(), e->frame_Duration());
      QCOMPARE(written.subHeaders[f]->x_Offset(), e->x_Offset());
      QVERIFY(approx(written.subHeaders[f]->z_Offset(), e->z_Offset(), 0.0, 1e-5));
    }
  }

  qDeleteAll(ecatHeaders);
  qDeleteAll(niftiHeaders);
}

//---------------------------------------------------------------------------------------------
// NIfTI datatypes and scaling -> ECAT7 data type and scale factor

void TestMedIOMatrix::niftiDataTypes_data()
{
  QTest::addColumn<int>("datatype");
  QTest::addColumn<double>("slope");
  QTest::addColumn<double>("inter");
  QTest::addColumn<double>("lo");      // stored values from lo to hi
  QTest::addColumn<double>("hi");
  QTest::addColumn<int>("ecatType");
  QTest::addColumn<double>("scale");   // 0: physical values (scale factor 1)

  QTest::newRow("uint8")              << 2   << 2.0 << 0.0 << 0.0 << 255.0 << static_cast<int>(CECATSubHeader::ByteData) << 2.0;
  QTest::newRow("int8")               << 256 << 2.0 << 0.0 << -128.0 << 127.0 << static_cast<int>(CECATSubHeader::SunShort) << 2.0;
  QTest::newRow("int16")              << 4   << 0.25 << 0.0 << -32768.0 << 32767.0 << static_cast<int>(CECATSubHeader::SunShort) << 0.25;
  QTest::newRow("uint16")             << 512 << 0.5 << 0.0 << 0.0 << 65535.0 << static_cast<int>(CECATSubHeader::SunLong) << 0.5;
  QTest::newRow("int32")              << 8   << 3.0 << 0.0 << -2147483648.0 << 2147483647.0 << static_cast<int>(CECATSubHeader::SunLong) << 3.0;
  QTest::newRow("uint32 int32 range") << 768 << 1.5 << 0.0 << 0.0 << 2147483647.0 << static_cast<int>(CECATSubHeader::SunLong) << 1.5;
  QTest::newRow("uint32 large")       << 768 << 1.5 << 0.0 << 0.0 << 4294967295.0 << static_cast<int>(CECATSubHeader::IEEEFloat) << 0.0;
  QTest::newRow("float32")            << 16  << 2.0 << 0.0 << -2.5 << 1e6 << static_cast<int>(CECATSubHeader::IEEEFloat) << 2.0;
  QTest::newRow("float64")            << 64  << 2.0 << 0.0 << -1e-3 << 1e10 << static_cast<int>(CECATSubHeader::IEEEFloat) << 0.0;
  QTest::newRow("int16 scl_inter")    << 4   << 0.5 << 5.0 << -100.0 << 100.0 << static_cast<int>(CECATSubHeader::IEEEFloat) << 0.0;
  QTest::newRow("uint8 scl_inter")    << 2   << 1.0 << -1.0 << 0.0 << 255.0 << static_cast<int>(CECATSubHeader::IEEEFloat) << 0.0;
  QTest::newRow("scl_slope 0")        << 4   << 0.0 << 7.0 << -100.0 << 100.0 << static_cast<int>(CECATSubHeader::SunShort) << 1.0;
}

void TestMedIOMatrix::niftiDataTypes()
{
  QFETCH(int, datatype);
  QFETCH(double, slope);
  QFETCH(double, inter);
  QFETCH(double, lo);
  QFETCH(double, hi);
  QFETCH(int, ecatType);
  QFETCH(double, scale);

  Synthetic s;
  s.datatype = datatype;
  s.slope = slope;
  s.inter = inter;
  s.version = 2; // double scl_slope/scl_inter
  s.byteOrder = (QSysInfo::ByteOrder == QSysInfo::LittleEndian) ? QSysInfo::BigEndian : QSysInfo::LittleEndian;

  // stored values from lo to hi; the affine is the one of libmedio: NIfTI voxel (i, j, nz-1-k) is ECAT7 voxel (i, j, k)
  const qint64 voxels = s.voxels();
  QByteArray values(voxels * niftiSize(datatype), '\0');
  QVector<double> stored(voxels);
  for(qint64 n=0; n < voxels; n++) {
    double v = lo + (hi - lo) * n / (voxels - 1);
    if(datatype != 16 && datatype != 64)
      v = std::floor(v);
    if(n == voxels - 1)
      v = hi;
    setNiftiValue(values.data(), n, datatype, v);
    stored[n] = niftiValue(values.constData(), n, datatype);
  }

  const QString name = QString("datatype-%1.nii").arg(QTest::currentDataTag()).replace(' ', '_');
  QVERIFY(writeNifti(tmpFile(name), s, values));

  CNIFTIFile nifti(tmpFile(name));
  QVERIFY(nifti.open(QIODevice::ReadOnly));

  // the volume keeps the datatype of the file
  CMedIOVolume volume;
  QVERIFY(nifti.readVolume(volume, 1));
  QCOMPARE(CMedIOVolume::bytesPerVoxel(volume.dataType()), niftiSize(datatype));
  for(qint64 n=0; n < voxels; n++)
    QCOMPARE(volume.storedValue(n), stored[n]);

  QByteArray* data = NULL;
  CMedIOHeader* sh = NULL;
  QVERIFY(nifti.readMatrix(data, sh, CMedIOData::ECAT, 1));
  QScopedPointer<QByteArray> dataGuard(data);
  QScopedPointer<CMedIOHeader> shGuard(sh);
  const CECAT7SubHeaderImage* sub = static_cast<const CECAT7SubHeaderImage*>(sh);

  QCOMPARE(static_cast<int>(sub->data_Type()), ecatType);
  QCOMPARE(data->size(), static_cast<int>(voxels * EcatFile::ecatSize(ecatType)));
  QCOMPARE(static_cast<double>(sub->scale_Factor()), scale == 0.0 ? 1.0 : scale);

  // physical values: stored x scl_slope + scl_inter (scl_slope 0: stored values)
  const double niftiSlope = (slope == 0.0) ? 1.0 : slope;
  const double niftiInter = (slope == 0.0) ? 0.0 : inter;
  for(int k=0; k < s.nz; k++)
    for(int j=0; j < s.ny; j++)
      for(int i=0; i < s.nx; i++) {
        const qint64 n = i + s.nx * (j + s.ny * (s.nz - 1 - k));
        const double value = ecatValue(data->constData(), i + s.nx * (j + s.ny * k), ecatType);
        if(scale != 0.0)
          QCOMPARE(value, stored[n]); // stored values unchanged
        else
          QVERIFY2(approx(value, stored[n] * niftiSlope + niftiInter, 1e-6),
                   qPrintable(QString("voxel %1: %2 != %3").arg(n).arg(value).arg(stored[n] * niftiSlope + niftiInter)));
      }

  if(ecatType == CECATSubHeader::SunShort || ecatType == CECATSubHeader::ByteData) {
    QCOMPARE(static_cast<double>(sub->image_Min()), lo);
    QCOMPARE(static_cast<double>(sub->image_Max()), hi);
  }
}

//---------------------------------------------------------------------------------------------
// datasets of testsuite/data: the NIfTI files were written by other tools
// (PMOD, ecat2nifti) from the ECAT7 files

void TestMedIOMatrix::niftiDatasets_data()
{
  QTest::addColumn<QString>("ecat");
  QTest::addColumn<QString>("nifti");
  QTest::addColumn<int>("ecatType");      // ECAT7 data type of the NIfTI file
  QTest::addColumn<bool>("calibrated");   // the NIfTI values include the ECAT7 calibration factor
  QTest::addColumn<bool>("requantized");  // the NIfTI file has one scale factor for all frames

  QTest::newRow("pet-static.pair")     << "ecat/pet-static.v" << "nifti/pet-static.pair/pet-static.hdr" << static_cast<int>(CECATSubHeader::SunShort) << true << false;
  QTest::newRow("pet-static.nii.gz")   << "ecat/pet-static.v" << "nifti/pet-static.nii.gz" << static_cast<int>(CECATSubHeader::IEEEFloat) << true << false;
  QTest::newRow("pet-dynamic.pair")    << "ecat/pet-dynamic.v" << "nifti/pet-dynamic.pair/pet-dynamic.hdr" << static_cast<int>(CECATSubHeader::SunShort) << false << true;
  QTest::newRow("pet-dynamic.nii.gz")  << "ecat/pet-dynamic.v" << "nifti/pet-dynamic.nii.gz" << static_cast<int>(CECATSubHeader::SunShort) << false << true;
}

void TestMedIOMatrix::niftiDatasets()
{
  QFETCH(QString, ecat);
  QFETCH(QString, nifti);
  QFETCH(int, ecatType);
  QFETCH(bool, calibrated);
  QFETCH(bool, requantized);

  EcatFile reference;
  QVERIFY(reference.read(medioTestDataDir() + "/" + ecat));
  const double calibration = reference.mainHeader->calibration_Factor();

  QScopedPointer<CMedIOData> niftiFile(CMedIODataFactory::create(medioTestDataDir() + "/" + nifti));
  QVERIFY(niftiFile->open(QIODevice::ReadOnly));
  QScopedPointer<CMedIOData> ecatFile(CMedIODataFactory::create(medioTestDataDir() + "/" + ecat));
  QVERIFY(ecatFile->open(QIODevice::ReadOnly));

  // NIfTI file written by the other tool
  const QByteArray niftiValues = readAll(medioTestDataDir() + "/" + nifti);
  NiftiHeader n;
  readAll(medioTestDataDir() + "/" + nifti, &n);

  double niftiScale = 0.0;
  for(int f=1; f <= reference.frames(); f++) {
    const CECAT7SubHeaderImage* e = reference.subHeaders[f-1];
    const qint64 voxels = reference.voxels();

    // NIfTI file -> ECAT7 representation
    QByteArray* data = NULL;
    CMedIOHeader* sh = NULL;
    QVERIFY(niftiFile->readMatrix(data, sh, CMedIOData::ECAT, f));
    QScopedPointer<QByteArray> dataGuard(data);
    QScopedPointer<CMedIOHeader> shGuard(sh);
    const CECAT7SubHeaderImage* s = static_cast<const CECAT7SubHeaderImage*>(sh);

    QCOMPARE(static_cast<int>(s->data_Type()), ecatType);
    QCOMPARE(s->x_Dimension(), e->x_Dimension());
    QCOMPARE(s->y_Dimension(), e->y_Dimension());
    QCOMPARE(s->z_Dimension(), e->z_Dimension());
    QVERIFY(approx(s->x_Pixel_Size(), e->x_Pixel_Size(), 1e-5));
    QVERIFY(approx(s->z_Pixel_Size(), e->z_Pixel_Size(), 1e-5));

    if(f == 1)
      niftiScale = s->scale_Factor();
    QCOMPARE(static_cast<qint64>(data->size()), voxels * EcatFile::ecatSize(ecatType));

    if(ecatType == CECATSubHeader::SunShort && (requantized == false || f == 1)) {
      // same stored values and scale factor
      QCOMPARE(static_cast<double>(s->scale_Factor()), static_cast<double>(e->scale_Factor()));
      QVERIFY(*data == reference.matrices[f-1]);
    } else {
      // same physical values (one quantization step of requantized frames)
      const double tolerance = requantized ? niftiScale : 0.0;
      for(qint64 i=0; i < voxels; i++) {
        const double expected = reference.value(f - 1, i) / (calibrated ? 1.0 : calibration);
        const double value = ecatValue(data->constData(), i, ecatType) * s->scale_Factor();
        QVERIFY2(approx(value, expected, 1e-6, tolerance), qPrintable(QString("frame %1 voxel %2: %3 != %4").arg(f).arg(i).arg(value).arg(expected)));
      }
    }

    // ECAT7 file -> NIfTI representation: the voxel order of the NIfTI files of the other tools
    if(requantized == false || f == 1) {
      QByteArray* niftiData = NULL;
      CMedIOHeader* niftiHeaderPtr = NULL;
      QVERIFY(ecatFile->readMatrix(niftiData, niftiHeaderPtr, CMedIOData::NIFTI, f));
      QScopedPointer<QByteArray> niftiDataGuard(niftiData);
      QScopedPointer<CMedIOHeader> niftiHeaderGuard(niftiHeaderPtr);
      const NiftiHeader h = niftiHeader(niftiHeaderPtr);
      QCOMPARE(h.datatype, 4); // int16 with the scale factor (x calibration factor)
      QVERIFY(approx(h.slope, e->scale_Factor() * calibration, 1e-6));
      for(int a=1; a <= 4; a++)
        QCOMPARE(h.dim[a], a < 4 ? n.dim[a] : static_cast<qint64>(reference.frames()));

      for(qint64 i=0; i < voxels; i++) {
        const double value = niftiValue(niftiData->constData(), i, 4) * h.slope;
        const double expected = niftiValue(niftiValues.constData(), i + voxels * (f - 1), n.datatype) * (n.slope != 0.0 ? n.slope : 1.0) *
                                (calibrated ? 1.0 : calibration);
        QVERIFY2(approx(value, expected, 1e-5), qPrintable(QString("frame %1 voxel %2: %3 != %4").arg(f).arg(i).arg(value).arg(expected)));
      }
    }
  }
}

//---------------------------------------------------------------------------------------------
// ECAT7 file -> NIfTI file: main header in the NIfTI representation, frames
// written in the ECAT7 representation and in the NIfTI representation

void TestMedIOMatrix::ecatToNiftiFile_data()
{
  QTest::addColumn<QString>("ecat");
  QTest::addColumn<QString>("nifti");
  QTest::addColumn<int>("datatype");  // expected NIfTI datatype of the main header

  QTest::newRow("pet-static .nii")       << "ecat/pet-static.v" << "static.nii" << 4;
  QTest::newRow("pet-static .hdr")       << "ecat/pet-static.v" << "static.hdr" << 4;
  QTest::newRow("pet-dynamic .nii.gz")   << "ecat/pet-dynamic.v" << "dynamic.nii.gz" << 16; // scale factor per frame
}

void TestMedIOMatrix::ecatToNiftiFile()
{
  QFETCH(QString, ecat);
  QFETCH(QString, nifti);
  QFETCH(int, datatype);

  EcatFile reference;
  QVERIFY(reference.read(medioTestDataDir() + "/" + ecat));
  const CECAT7SubHeaderImage* first = reference.subHeaders.first();
  const int nx = first->x_Dimension(), ny = first->y_Dimension(), nz = first->z_Dimension();
  const qint64 voxels = reference.voxels();

  CECATFile in(medioTestDataDir() + "/" + ecat);
  QVERIFY(in.open(QIODevice::ReadOnly));
  CMedIOHeader* mh = NULL;
  QVERIFY(in.readMainHeader(mh, CMedIOData::NIFTI));
  QScopedPointer<CMedIOHeader> mhGuard(mh);
  QCOMPARE(niftiHeader(mh).datatype, datatype);
  QCOMPARE(niftiHeader(mh).dim[4], static_cast<qint64>(reference.frames()));

  for(int pass=0; pass < 2; pass++) {
    const QString path = tmpFile(QString::number(pass) + "-" + nifti);
    {
      // pass 0: NIfTI-1, header linked to the file; pass 1: NIfTI-2, header converted
      CNIFTIFile out(path, pass == 0 ? CNIFTIMainHeader::NIFTI1 : CNIFTIMainHeader::NIFTI2);
      if(pass == 0) {
        QVERIFY(out.open(QIODevice::WriteOnly));
        mh->setMedIOData(&out);
        QVERIFY(out.writeMainHeader(*static_cast<CNIFTIMainHeader*>(mh)));
        mh->setMedIOData(NULL);
      } else {
        QVERIFY(createNifti(out, mh));
      }

      for(int f=reference.frames(); f >= 1; f--) {
        if(pass == 0) {
          // ECAT7 representation (the matrix of CECATFile, padded to 512 byte blocks)
          QByteArray* data = NULL;
          CECATSubHeader* sh = NULL;
          QVERIFY(in.readMatrix(data, sh, f));
          QVERIFY(out.writeMatrix(*data, *static_cast<CMedIOHeader*>(sh), f, reference.mainHeader));
          delete data;
          delete sh;
        } else {
          // NIfTI representation
          QByteArray* data = NULL;
          CMedIOHeader* sh = NULL;
          QVERIFY(in.readMatrix(data, sh, CMedIOData::NIFTI, f));
          QVERIFY(out.writeMatrix(*data, *sh, f));
          delete data;
          delete sh;
        }
      }
      out.close();
    }

    NiftiHeader h;
    const QByteArray all = readAll(path, &h);
    QCOMPARE(h.datatype, datatype);
    QCOMPARE(static_cast<qint64>(all.size()), voxels * reference.frames() * niftiSize(datatype));

    for(int f=0; f < reference.frames(); f++)
      for(int k=0; k < nz; k++)
        for(int j=0; j < ny; j++)
          for(int i=0; i < nx; i++) {
            const qint64 e = i + static_cast<qint64>(nx) * (j + static_cast<qint64>(ny) * k);
            const qint64 n = i + static_cast<qint64>(nx) * (j + static_cast<qint64>(ny) * (nz - 1 - k));
            const double value = niftiValue(all.constData(), n + voxels * f, datatype);

            if(datatype == 4)
              QCOMPARE(value, ecatValue(reference.matrices[f].constData(), e, CECATSubHeader::SunShort));
            else
              QVERIFY(approx(value * h.slope, reference.value(f, e), 1e-6));

            if(f == 0 && (i == 0 || i == nx - 1) && (j == 0 || j == ny - 1)) {
              const double v[3] = { static_cast<double>(i), static_cast<double>(j), static_cast<double>(nz - 1 - k) };
              double pn[3], pe[3];
              niftiPosition(h.srow, v, pn);
              reference.position(f, i, j, k, pe);
              for(int r=0; r < 3; r++)
                QVERIFY(approx(pn[r], pe[r], 0.0, 1e-3));
            }
          }
  }
}

//---------------------------------------------------------------------------------------------
// NIfTI file of PMOD -> ECAT7 file with the stored values of the original ECAT7 file

void TestMedIOMatrix::niftiToEcatFile()
{
  const QString path = tmpFile("pet-static-from-nifti.v");
  {
    QScopedPointer<CMedIOData> in(CMedIODataFactory::create(medioTestDataDir() + "/nifti/pet-static.pair/pet-static.hdr"));
    QVERIFY(in->open(QIODevice::ReadOnly));

    CMedIOHeader* mh = NULL;
    QVERIFY(in->readMainHeader(mh, CMedIOData::ECAT));
    QScopedPointer<CMedIOHeader> mhGuard(mh);

    CECATFile out(path, CECATMainHeader::ECAT7_Volume16);
    QVERIFY(createEcat(out, *static_cast<CECAT7MainHeader*>(mh)));

    // the NIfTI representation of the file itself
    QByteArray* data = NULL;
    CMedIOHeader* sh = NULL;
    QVERIFY(in->readMatrix(data, sh, CMedIOData::NIFTI, 1));
    QVERIFY(out.writeMatrix(*data, *sh, 1));
    delete data;
    delete sh;
    out.close();
  }

  EcatFile written, reference;
  QVERIFY(written.read(path));
  QVERIFY(reference.read(medioTestDataDir() + "/ecat/pet-static.v"));
  QCOMPARE(written.frames(), 1);
  QCOMPARE(static_cast<int>(written.subHeaders[0]->data_Type()), static_cast<int>(CECATSubHeader::SunShort));
  QCOMPARE(written.subHeaders[0]->scale_Factor(), reference.subHeaders[0]->scale_Factor());
  QVERIFY(written.matrices[0] == reference.matrices[0]);
}

//---------------------------------------------------------------------------------------------
// ECAT7 frames with different scale factors and calibration factor 1000
// written into an int16 NIfTI file with the scaling of frame 1: frame 1 keeps
// its stored values, the others are requantized; values exceeding int16 are rejected

void TestMedIOMatrix::niftiRequantized()
{
  EcatFile reference;
  QVERIFY(reference.read(medioTestDataDir() + "/ecat/pet-dynamic.v"));
  QCOMPARE(reference.frames(), 3);
  const CECAT7SubHeaderImage* first = reference.subHeaders.first();
  const int nx = first->x_Dimension(), ny = first->y_Dimension(), nz = first->z_Dimension();
  const qint64 voxels = reference.voxels();

  for(int pass=0; pass < 2; pass++) {
    const QString path = tmpFile(QString("requantized-%1.nii").arg(pass));
    CNIFTIFile file(path, CNIFTIMainHeader::NIFTI1);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QScopedPointer<CNIFTIMainHeader> header(file.createEmptyHeader());
    QVERIFY(header->convertFrom(reference.mainHeader, first));
    CNIFTI1MainHeader* h = static_cast<CNIFTI1MainHeader*>(header.data());
    QVERIFY(approx(h->scl_Slope(), first->scale_Factor() * 1000.0, 1e-6));
    if(pass == 1)
      h->setScl_Slope(h->scl_Slope() / 4.0); // frame 1 does not fit
    QVERIFY(file.writeMainHeader(*header));
    const double slope = h->scl_Slope();

    if(pass == 1) {
      QVERIFY(file.writeMatrix(reference.matrices[0], *reference.subHeaders[0], 1, reference.mainHeader) == false);
      file.close();
      continue;
    }

    for(int f=1; f <= 3; f++)
      QVERIFY(file.writeMatrix(reference.matrices[f-1], *reference.subHeaders[f-1], f, reference.mainHeader));
    file.close();

    const QByteArray all = readAll(path);
    QCOMPARE(static_cast<qint64>(all.size()), voxels * 3 * 2);
    for(int f=0; f < 3; f++)
      for(int k=0; k < nz; k++)
        for(qint64 ij=0; ij < static_cast<qint64>(nx) * ny; ij++) {
          const double stored = ecatValue(reference.matrices[f].constData(), ij + nx * ny * k, CECATSubHeader::SunShort);
          const double value = niftiValue(all.constData(), ij + nx * ny * (nz - 1 - k) + voxels * f, 4);
          if(f == 0)
            QCOMPARE(value, stored);
          else
            QVERIFY(std::fabs(value * slope - reference.value(f, ij + nx * ny * k)) <= slope * (0.5 + 1e-6));
        }
  }
}

//---------------------------------------------------------------------------------------------

void TestMedIOMatrix::niftiErrors()
{
  Synthetic s;
  const qint64 voxels = s.voxels();
  QByteArray values(voxels * 2, '\0');

  CNIFTIFile file(tmpFile("errors.nii"), CNIFTIMainHeader::NIFTI1);
  QVERIFY(createNifti(file, s));

  // ECAT7 sub header not matching the NIfTI main header
  CECAT7SubHeaderImage sub;
  sub.setData_Type(CECATSubHeader::SunShort);
  sub.setScale_Factor(1.0f);
  sub.setX_Dimension(s.nx); sub.setY_Dimension(s.ny); sub.setZ_Dimension(s.nz);
  QVERIFY(file.writeMatrix(values, sub, 1));
  QVERIFY(file.writeMatrix(values, sub, 2) == false);         // frame
  sub.setZ_Dimension(s.nz + 1);
  QVERIFY(file.writeMatrix(QByteArray(voxels * 2 + s.nx * s.ny * 2, '\0'), sub, 1) == false); // dimensions
  sub.setZ_Dimension(s.nz);
  QVERIFY(file.writeMatrix(values.left(values.size() - 2), sub, 1) == false);              // size
  sub.setData_Type(CECATSubHeader::IEEEFloat);
  QByteArray floats(voxels * 4, '\0');
  reinterpret_cast<float*>(floats.data())[3] = 40000.0f;
  QVERIFY(file.writeMatrix(floats, sub, 1) == false); // exceeds int16
  reinterpret_cast<float*>(floats.data())[3] = std::numeric_limits<float>::quiet_NaN();
  QVERIFY(file.writeMatrix(floats, sub, 1) == false); // NaN in int16
  reinterpret_cast<float*>(floats.data())[3] = 2.6f;
  QVERIFY(file.writeMatrix(floats, sub, 1));          // rounded to 3
  file.close();

  // the rejected writes did not change the data
  const QByteArray written = readAll(tmpFile("errors.nii"));
  QCOMPARE(written.size(), values.size());
  for(qint64 n=0; n < voxels; n++) {
    // ECAT7 voxel 3 = (3, 0, 0) is NIfTI voxel (3, 0, nz-1)
    const double expected = (n == 3 + s.nx * s.ny * (s.nz - 1)) ? 3.0 : 0.0;
    QCOMPARE(niftiValue(written.constData(), n, 4), expected);
  }

  // complex data: neither volume nor other representations
  {
    Synthetic c;
    c.datatype = 32; // DT_COMPLEX64
    QVERIFY(writeNifti(tmpFile("complex.nii"), c, QByteArray(voxels * 8, '\0')));

    CNIFTIFile in(tmpFile("complex.nii"));
    QVERIFY(in.open(QIODevice::ReadOnly));
    QByteArray* data = NULL;
    QVERIFY(in.readMatrix(data, 1));  // native volume
    QCOMPARE(data->size(), static_cast<int>(voxels * 8));
    delete data;
    data = NULL;

    CMedIOHeader* sh = NULL;
    QVERIFY(in.readMatrix(data, sh, CMedIOData::ECAT, 1) == false);
    QVERIFY(data == NULL && sh == NULL);
    CMedIOVolume v;
    QVERIFY(in.readVolume(v, 1) == false);
  }

  // 5D data: frames of the file, but no ECAT7 representation
  {
    Synthetic d;
    d.frames = 2;
    d.dim5 = 3;
    QByteArray data5(voxels * 6 * 2, '\0');
    for(qint64 n=0; n < voxels * 6; n++)
      reinterpret_cast<qint16*>(data5.data())[n] = static_cast<qint16>(n);
    QVERIFY(writeNifti(tmpFile("5d.nii"), d, data5));

    CNIFTIFile in(tmpFile("5d.nii"));
    QVERIFY(in.open(QIODevice::ReadOnly));
    QByteArray* data = NULL;
    QVERIFY(in.readMatrix(data, 6));
    QCOMPARE(*data, data5.right(voxels * 2));
    delete data;
    QVERIFY(in.readMatrix(data, 7) == false);
    CMedIOHeader* sh = NULL;
    QVERIFY(in.readMatrix(data, sh, CMedIOData::ECAT, 1) == false);
  }

  // the main header has to be written first
  {
    CNIFTIFile out(tmpFile("noheader.nii"), CNIFTIMainHeader::NIFTI1);
    QVERIFY(out.open(QIODevice::WriteOnly));
    QVERIFY(out.writeMatrix(values, sub, 1) == false);
    out.close();
  }
}

//=============================================================================================
// Philips

// maximum absolute physical value of frame f
static double maxAbs(const EcatFile& e, int f)
{
  double m = 0.0;
  for(qint64 i=0; i < e.voxels(); i++)
    m = qMax(m, std::fabs(e.value(f, i)));
  return m;
}

// physical values of a Philips matrix (all slices of a frame) and its scale factor
static bool philipsValues(const QByteArray& data, const CMedIOHeader* header, const CMedIOHeader* mainHeader, QVector<double>& values)
{
  const CPhilipsSubHeaderImage* s = dynamic_cast<const CPhilipsSubHeaderImage*>(header);
  const CPhilipsMainHeader* m = dynamic_cast<const CPhilipsMainHeader*>(mainHeader);
  if(s == NULL || m == NULL)
    return false;
  bool ok = false;
  const double scale = s->ecat_Scale_Factor(ok, m);
  if(ok == false)
    return false;

  values.clear();
  switch(s->datype()) {
    case CPhilipsSubHeader::SignedShort:
      for(int i=0; i < data.size() / 2; i++)
        values.append(reinterpret_cast<const qint16*>(data.constData())[i] * scale);
      break;
    case CPhilipsSubHeader::Float:
      for(int i=0; i < data.size() / 4; i++)
        values.append(reinterpret_cast<const float*>(data.constData())[i] * scale);
      break;
    default:
      return false;
  }
  return true;
}

//---------------------------------------------------------------------------------------------
// Philips files of the datasets (ROVER export with the same stored values and
// voxel order as the ECAT7 files) in all representations

void TestMedIOMatrix::philipsDatasets_data()
{
  QTest::addColumn<QString>("philips");
  QTest::addColumn<QString>("ecat");

  QTest::newRow("pet-static.img")  << "philips/pet-static.img" << "ecat/pet-static.v";
  QTest::newRow("pet-static.syn")  << "philips/pet-static.syn" << "ecat/pet-static.v";
  QTest::newRow("pet-dynamic.img") << "philips/pet-dynamic.img" << "ecat/pet-dynamic.v";
  QTest::newRow("pet-dynamic.syn") << "philips/pet-dynamic.syn" << "ecat/pet-dynamic.v";
}

void TestMedIOMatrix::philipsDatasets()
{
  QFETCH(QString, philips);
  QFETCH(QString, ecat);

  EcatFile reference;
  QVERIFY(reference.read(medioTestDataDir() + "/" + ecat));
  const qint64 voxels = reference.voxels();

  QScopedPointer<CMedIOData> file(CMedIODataFactory::create(medioTestDataDir() + "/" + philips));
  QVERIFY(file.data() != NULL);
  QCOMPARE(static_cast<int>(file->dataFormat()), static_cast<int>(CMedIOData::Philips));
  QVERIFY(file->open(QIODevice::ReadOnly));
  CECATFile ecatFile(medioTestDataDir() + "/" + ecat);
  QVERIFY(ecatFile.open(QIODevice::ReadOnly));

  CMedIOHeader* philipsMain = NULL;
  QVERIFY(file->readMainHeader(philipsMain, CMedIOData::Philips));
  QScopedPointer<CMedIOHeader> philipsMainGuard(philipsMain);
  CMedIOHeader* ecatAsPhilipsMain = NULL;
  QVERIFY(ecatFile.readMainHeader(ecatAsPhilipsMain, CMedIOData::Philips));
  QScopedPointer<CMedIOHeader> ecatAsPhilipsMainGuard(ecatAsPhilipsMain);
  CMedIOHeader* ecatMain = NULL;
  QVERIFY(file->readMainHeader(ecatMain, CMedIOData::ECAT));
  QScopedPointer<CMedIOHeader> ecatMainGuard(ecatMain);
  QCOMPARE(static_cast<int>(static_cast<CECAT7MainHeader*>(ecatMain)->num_Frames()), reference.frames());

  const short firstFrame = static_cast<CPhilipsFile*>(file.data())->minFrame();
  for(int f=0; f < reference.frames(); f++) {
    const double tolerance = 1e-6 * maxAbs(reference, f);

    // volume: same stored values and voxel order, physical values and positions of the ECAT7 file
    CMedIOVolume v;
    QVERIFY(file->readVolume(v, firstFrame + f));
    QCOMPARE(v.dataType(), CMedIOVolume::Int16);
    QCOMPARE(v.voxelCount(), voxels);
    QCOMPARE(v.orientation(), QString("LPI"));
    QCOMPARE(v.frameStart() * 1000.0, static_cast<double>(reference.subHeaders[f]->frame_Start_Time()));
    for(qint64 i=0; i < voxels; i++) {
      QCOMPARE(v.storedValue(i), ecatValue(reference.matrices[f].constData(), i, CECATSubHeader::SunShort));
      QVERIFY(approx(v.value(i), reference.value(f, i), 0.0, tolerance));
    }
    const int last[3] = { v.dimension(0) - 1, v.dimension(1) - 1, v.dimension(2) - 1 };
    double p[3], q[3];
    v.position(last[0], last[1], last[2], p);
    reference.position(f, last[0], last[1], last[2], q);
    for(int r=0; r < 3; r++)
      QVERIFY(approx(p[r], q[r], 0.0, 1e-3));

    // ECAT7 representation: int16 with one scale factor if the slices share it, float32 otherwise
    QByteArray* data = NULL;
    CMedIOHeader* sh = NULL;
    QVERIFY(file->readMatrix(data, sh, CMedIOData::ECAT, firstFrame + f));
    QScopedPointer<QByteArray> dataGuard(data);
    QScopedPointer<CMedIOHeader> shGuard(sh);
    const CECAT7SubHeaderImage* s = static_cast<const CECAT7SubHeaderImage*>(sh);
    QCOMPARE(static_cast<int>(s->data_Type()), static_cast<int>(v.hasUniformScaleFactor() ? CECATSubHeader::SunShort : CECATSubHeader::IEEEFloat));
    QCOMPARE(s->z_Dimension(), reference.subHeaders[f]->z_Dimension());
    for(qint64 i=0; i < voxels; i++)
      QVERIFY(approx(ecatValue(data->constData(), i, s->data_Type()) * s->scale_Factor(), reference.value(f, i), 0.0, tolerance));

    // NIfTI representation: the same values as the one of the ECAT7 file
    QByteArray* niftiData = NULL;
    CMedIOHeader* niftiHeaderPtr = NULL;
    QVERIFY(file->readMatrix(niftiData, niftiHeaderPtr, CMedIOData::NIFTI, firstFrame + f));
    QScopedPointer<QByteArray> niftiDataGuard(niftiData);
    QScopedPointer<CMedIOHeader> niftiHeaderGuard(niftiHeaderPtr);
    QByteArray* ecatNiftiData = NULL;
    CMedIOHeader* ecatNiftiHeader = NULL;
    QVERIFY(ecatFile.readMatrix(ecatNiftiData, ecatNiftiHeader, CMedIOData::NIFTI, f + 1));
    QScopedPointer<QByteArray> ecatNiftiDataGuard(ecatNiftiData);
    QScopedPointer<CMedIOHeader> ecatNiftiHeaderGuard(ecatNiftiHeader);
    const NiftiHeader a = niftiHeader(niftiHeaderPtr), b = niftiHeader(ecatNiftiHeader);
    for(int r=0; r < 3; r++)
      for(int c=0; c < 4; c++)
        QVERIFY(approx(a.srow[r][c], b.srow[r][c], 0.0, 1e-3));
    for(qint64 i=0; i < voxels; i++)
      QVERIFY(approx(niftiValue(niftiData->constData(), i, a.datatype) * a.slope + a.inter,
                     niftiValue(ecatNiftiData->constData(), i, b.datatype) * b.slope + b.inter, 0.0, tolerance));

    // Philips representation of the Philips file and of the ECAT7 file: all slices, one scale factor
    QByteArray* philipsData = NULL;
    CMedIOHeader* philipsHeader = NULL;
    QVERIFY(file->readMatrix(philipsData, philipsHeader, CMedIOData::Philips, firstFrame + f));
    QScopedPointer<QByteArray> philipsDataGuard(philipsData);
    QScopedPointer<CMedIOHeader> philipsHeaderGuard(philipsHeader);
    QCOMPARE(static_cast<int>(philipsHeader->headerFormat()), static_cast<int>(CMedIOHeader::PhilipsSubHeader));
    QVector<double> values;
    QVERIFY(philipsValues(*philipsData, philipsHeader, philipsMain, values));
    QCOMPARE(static_cast<qint64>(values.size()), voxels);
    for(qint64 i=0; i < voxels; i++)
      QVERIFY(approx(values[i], reference.value(f, i), 0.0, tolerance));

    QVERIFY(ecatFile.readMatrix(philipsData, philipsHeader, CMedIOData::Philips, f + 1));
    QScopedPointer<QByteArray> philipsData2Guard(philipsData);
    QScopedPointer<CMedIOHeader> philipsHeader2Guard(philipsHeader);
    QCOMPARE(static_cast<int>(static_cast<CPhilipsSubHeader*>(philipsHeader)->datype()), static_cast<int>(CPhilipsSubHeader::SignedShort));
    QVERIFY(*philipsData == reference.matrices[f]); // stored values of the ECAT7 file
    QVERIFY(philipsValues(*philipsData, philipsHeader, ecatAsPhilipsMain, values));
    for(qint64 i=0; i < voxels; i++)
      QVERIFY(approx(values[i], reference.value(f, i), 0.0, tolerance));
  }
}

//---------------------------------------------------------------------------------------------
// Philips image and Syntegra files written from ECAT7 matrices and from NIfTI matrices

void TestMedIOMatrix::writePhilips_data()
{
  QTest::addColumn<QString>("source");
  QTest::addColumn<QString>("ecat");     // ECAT7 file of the same dataset
  QTest::addColumn<int>("type");
  QTest::addColumn<bool>("exact");       // the stored values of the ECAT7 file are kept
  QTest::addColumn<bool>("timing");      // the source holds the frame timing

  QTest::newRow("ECAT7 -> image")      << "ecat/pet-dynamic.v" << "ecat/pet-dynamic.v" << static_cast<int>(CPhilipsMainHeader::Image) << true << true;
  QTest::newRow("ECAT7 -> Syntegra")   << "ecat/pet-static.v" << "ecat/pet-static.v" << static_cast<int>(CPhilipsMainHeader::Syntegra) << true << true;
  QTest::newRow("NIfTI -> image")      << "nifti/pet-static.pair/pet-static.hdr" << "ecat/pet-static.v" << static_cast<int>(CPhilipsMainHeader::Image) << true << true;
  QTest::newRow("NIfTI -> Syntegra")   << "nifti/pet-static.nii.gz" << "ecat/pet-static.v" << static_cast<int>(CPhilipsMainHeader::Syntegra) << false << false;
  QTest::newRow("ECAT7 -> image, other main header") << "ecat/pet-dynamic.v" << "ecat/pet-dynamic.v" << static_cast<int>(CPhilipsMainHeader::Image) << true << true;
}

void TestMedIOMatrix::writePhilips()
{
  QFETCH(QString, source);
  QFETCH(QString, ecat);
  QFETCH(int, type);
  QFETCH(bool, exact);
  QFETCH(bool, timing);

  EcatFile reference;
  QVERIFY(reference.read(medioTestDataDir() + "/" + ecat));
  const qint64 voxels = reference.voxels();

  const QString path = tmpFile(QString(QTest::currentDataTag()).replace(QRegExp("[^a-zA-Z0-9]"), "_") +
                               (type == CPhilipsMainHeader::Syntegra ? ".syn" : ".img"));
  {
    QScopedPointer<CMedIOData> in(CMedIODataFactory::create(medioTestDataDir() + "/" + source));
    QVERIFY(in->open(QIODevice::ReadOnly));

    CMedIOHeader* mh = NULL;
    QVERIFY(in->readMainHeader(mh, CMedIOData::Philips));
    QScopedPointer<CMedIOHeader> mhGuard(mh);
    CPhilipsMainHeader* philipsMain = static_cast<CPhilipsMainHeader*>(mh);
    philipsMain->setFiltyp(static_cast<CPhilipsMainHeader::File_Type>(type));
    if(QString(QTest::currentDataTag()).contains("other main header")) {
      // the scale factors of the slices must not depend on the patient data of the source
      philipsMain->setWeight(philipsMain->weight() * 2 + 10000);
      philipsMain->setActivity(philipsMain->activity() * 3.0f + 1.0f);
    }

    CPhilipsFile out(path, static_cast<CPhilipsMainHeader::File_Type>(type));
    QVERIFY(out.open(QIODevice::WriteOnly));
    philipsMain->setMedIOData(&out);
    QVERIFY(out.writeMainHeader(*philipsMain));

    CMedIOHeader* sourceMain = NULL;
    QVERIFY(in->readMainHeader(sourceMain, in->dataFormat()));
    QScopedPointer<CMedIOHeader> sourceMainGuard(sourceMain);

    for(int f=reference.frames(); f >= 1; f--) {
      QByteArray* data = NULL;
      CMedIOHeader* sh = NULL;
      QVERIFY(in->readMatrix(data, sh, in->dataFormat(), f));
      QVERIFY(out.writeMatrix(*data, *sh, f, sourceMain));
      delete data;
      delete sh;
    }
    out.close();
  }

  // read back: values and slice numbering
  CPhilipsFile written(path);
  QVERIFY(written.open(QIODevice::ReadOnly));
  QCOMPARE(static_cast<int>(written.fileType()), type);
  QCOMPARE(static_cast<int>(written.numFrames()), reference.frames());
  QCOMPARE(static_cast<int>(written.minSlice()), type == CPhilipsMainHeader::Syntegra ? 1 : 0);

  for(int f=0; f < reference.frames(); f++) {
    CMedIOVolume v;
    QVERIFY(written.readVolume(v, written.minFrame() + f));
    QCOMPARE(v.voxelCount(), voxels);
    QCOMPARE(v.dataType(), exact ? CMedIOVolume::Int16 : CMedIOVolume::Float32);
    const double tolerance = 1e-6 * maxAbs(reference, f);
    for(qint64 i=0; i < voxels; i++) {
      if(exact)
        QCOMPARE(v.storedValue(i), ecatValue(reference.matrices[f].constData(), i, CECATSubHeader::SunShort));
      QVERIFY(approx(v.value(i), reference.value(f, i), 0.0, tolerance));
    }
    if(timing) {
      QCOMPARE(v.frameStart() * 1000.0, static_cast<double>(reference.subHeaders[f]->frame_Start_Time()));
      QCOMPARE(v.frameDuration() * 1000.0, static_cast<double>(reference.subHeaders[f]->frame_Duration()));
    }
  }
}

//---------------------------------------------------------------------------------------------
// Philips file with a scale factor per slice (2 slices of 4 x 3 voxels,
// stored value 1 + i + 100 * slice, written with CPhilipsFile)

void TestMedIOMatrix::philipsSliceScales_data()
{
  QTest::addColumn<int>("dataType");
  QTest::addColumn<double>("scale0");
  QTest::addColumn<double>("scale1");
  QTest::addColumn<int>("ecatType");     // ECAT7 representation
  QTest::addColumn<int>("niftiType");    // NIfTI representation

  QTest::newRow("int16, different scales")  << static_cast<int>(CPhilipsSubHeader::SignedShort) << 0.5 << 2.0
                                            << static_cast<int>(CECATSubHeader::IEEEFloat) << 16;
  QTest::newRow("uint16, different scales") << static_cast<int>(CPhilipsSubHeader::UnsignedShort) << 0.5 << 2.0
                                            << static_cast<int>(CECATSubHeader::IEEEFloat) << 16;
  QTest::newRow("uint16, same scale")       << static_cast<int>(CPhilipsSubHeader::UnsignedShort) << 2.0 << 2.0
                                            << static_cast<int>(CECATSubHeader::SunLong) << 512;
}

void TestMedIOMatrix::philipsSliceScales()
{
  QFETCH(int, dataType);
  QFETCH(double, scale0);
  QFETCH(double, scale1);
  QFETCH(int, ecatType);
  QFETCH(int, niftiType);

  const double scales[2] = { scale0, scale1 };
  const QString path = tmpFile(QString("slices-%1-%2.img").arg(dataType).arg(scale0));
  {
    // NIfTI header of the geometry (libmedio orientation)
    Synthetic s;
    QVERIFY(writeNifti(tmpFile("slices-geometry.nii"), s, QByteArray(s.voxels() * 2, '\0')));
    CNIFTIFile niftiFile(tmpFile("slices-geometry.nii"));
    QVERIFY(niftiFile.open(QIODevice::ReadOnly));
    CNIFTIMainHeader* niftiHeaderPtr = NULL;
    QVERIFY(niftiFile.readMainHeader(niftiHeaderPtr));
    QScopedPointer<CNIFTIMainHeader> niftiGuard(niftiHeaderPtr);

    CPhilipsFile philipsFile(path, CPhilipsMainHeader::Image);
    QVERIFY(philipsFile.open(QIODevice::WriteOnly));
    QScopedPointer<CPhilipsMainHeader> mainHeader(philipsFile.createEmptyMainHeader());
    QVERIFY(mainHeader->convertFrom(niftiHeaderPtr));
    QScopedPointer<CPhilipsSubHeader> subHeaderPtr(philipsFile.createEmptySubHeader());
    CPhilipsSubHeaderImage* subHeader = static_cast<CPhilipsSubHeaderImage*>(subHeaderPtr.data());
    QVERIFY(subHeader->convertFrom(NULL, niftiHeaderPtr));
    subHeader->setDatype(static_cast<CPhilipsSubHeader::Data_Type>(dataType));

    for(int k=0; k < 2; k++) {
      QByteArray plane;
      for(int i=0; i < 12; i++) {
        const qint16 v = static_cast<qint16>(i + 1 + 100 * k);
        plane.append(reinterpret_cast<const char*>(&v), sizeof(v));
      }
      subHeader->setSuvscl(0.0f);
      subHeader->setImgscl(scales[k]);
      subHeader->setSlcnum(k);
      QVERIFY(philipsFile.writeMatrix(plane.constData(), plane.size(), *subHeader, 1, k));
    }
    QVERIFY(philipsFile.writeMainHeader(*mainHeader));
    philipsFile.close();
  }

  CPhilipsFile file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));

  CMedIOVolume v;
  QVERIFY(file.readVolume(v, 1));
  QCOMPARE(v.dataType(), dataType == CPhilipsSubHeader::SignedShort ? CMedIOVolume::Int16 : CMedIOVolume::UInt16);
  QCOMPARE(v.scaleFactor(0), scale0);
  QCOMPARE(v.scaleFactor(1), scale1);
  for(int k=0; k < 2; k++)
    for(int i=0; i < 12; i++) {
      QCOMPARE(v.storedValue(i + 12 * k), 1.0 + i + 100 * k);
      QCOMPARE(v.value(i + 12 * k), (1.0 + i + 100 * k) * scales[k]);
    }

  // ECAT7 representation
  QByteArray* data = NULL;
  CMedIOHeader* sh = NULL;
  QVERIFY(file.readMatrix(data, sh, CMedIOData::ECAT, 1));
  QScopedPointer<QByteArray> dataGuard(data);
  QScopedPointer<CMedIOHeader> shGuard(sh);
  const CECAT7SubHeaderImage* e = static_cast<const CECAT7SubHeaderImage*>(sh);
  QCOMPARE(static_cast<int>(e->data_Type()), ecatType);
  for(int k=0; k < 2; k++)
    for(int i=0; i < 12; i++)
      QCOMPARE(ecatValue(data->constData(), i + 12 * k, ecatType) * e->scale_Factor(), (1.0 + i + 100 * k) * scales[k]);

  // NIfTI representation (planes reversed)
  QVERIFY(file.readMatrix(data, sh, CMedIOData::NIFTI, 1));
  QScopedPointer<QByteArray> niftiDataGuard(data);
  QScopedPointer<CMedIOHeader> niftiHeaderGuard(sh);
  const NiftiHeader n = niftiHeader(sh);
  QCOMPARE(n.datatype, niftiType);
  for(int k=0; k < 2; k++)
    for(int i=0; i < 12; i++)
      QCOMPARE(niftiValue(data->constData(), i + 12 * (1 - k), niftiType) * n.slope, (1.0 + i + 100 * k) * scales[k]);

  // Philips representation: one scale factor
  CMedIOHeader* mh = NULL;
  QVERIFY(file.readMainHeader(mh, CMedIOData::Philips));
  QScopedPointer<CMedIOHeader> mhGuard(mh);
  QVERIFY(file.readMatrix(data, sh, CMedIOData::Philips, 1));
  QScopedPointer<QByteArray> philipsDataGuard(data);
  QScopedPointer<CMedIOHeader> philipsHeaderGuard(sh);
  QCOMPARE(static_cast<int>(static_cast<CPhilipsSubHeader*>(sh)->datype()),
           static_cast<int>(scale0 == scale1 ? dataType : CPhilipsSubHeader::Float));
  bool ok = false;
  const double scale = static_cast<CPhilipsSubHeaderImage*>(sh)->ecat_Scale_Factor(ok, static_cast<CPhilipsMainHeader*>(mh));
  QVERIFY(ok);
  for(int k=0; k < 2; k++)
    for(int i=0; i < 12; i++) {
      const double stored = (scale0 == scale1) ? (dataType == CPhilipsSubHeader::SignedShort ? reinterpret_cast<const qint16*>(data->constData())[i + 12 * k]
                                                                                             : reinterpret_cast<const quint16*>(data->constData())[i + 12 * k])
                                               : reinterpret_cast<const float*>(data->constData())[i + 12 * k];
      QCOMPARE(stored * scale, (1.0 + i + 100 * k) * scales[k]);
    }
}

//---------------------------------------------------------------------------------------------

void TestMedIOMatrix::philipsErrors()
{
  CPhilipsFile philips(medioTestDataDir() + "/philips/pet-static.img");
  QVERIFY(philips.open(QIODevice::ReadOnly));
  QByteArray* data = NULL;
  CMedIOHeader* sh = NULL;
  QVERIFY(philips.readMatrix(data, sh, CMedIOData::Philips, -1));
  QScopedPointer<QByteArray> dataGuard(data);
  QScopedPointer<CMedIOHeader> shGuard(sh);
  CMedIOHeader* mh = NULL;
  QVERIFY(philips.readMainHeader(mh, CMedIOData::Philips));
  QScopedPointer<CMedIOHeader> mhGuard(mh);

  CECATFile out(tmpFile("philips-errors.v"), CECATMainHeader::ECAT7_Volume16);
  QVERIFY(createEcat(out, CECAT7MainHeader(NULL, CECATMainHeader::ECAT7_Volume16)));
  QVERIFY(out.writeMatrix(*data, *sh, 1) == false);                        // no Philips main header
  QVERIFY(out.writeMatrix(data->left(data->size() - 2), *sh, 1, mh) == false); // not a multiple of a slice
  QVERIFY(out.writeMatrix(*data, *sh, 1, mh));
  out.close();

  // the frames of Philips files can be written in any order and overwritten with the same slices
  {
    CPhilipsFile philipsOut(tmpFile("philips-order.img"), CPhilipsMainHeader::Image);
    QVERIFY(philipsOut.open(QIODevice::WriteOnly));
    mh->setMedIOData(&philipsOut);
    QVERIFY(philipsOut.writeMainHeader(*static_cast<CPhilipsMainHeader*>(mh)));
    mh->setMedIOData(NULL);
    QVERIFY(philipsOut.writeMatrix(*data, *sh, 2, mh));
    QVERIFY(philipsOut.writeMatrix(*data, *sh, 1, mh));
    QVERIFY(philipsOut.writeMatrix(*data, *sh, 3, mh));

    // frame 2 overwritten with values that compress worse
    QByteArray other(*data);
    for(int i=0; i < other.size() / 2; i++)
      reinterpret_cast<qint16*>(other.data())[i] = static_cast<qint16>((i * 7919) % 30011);
    QVERIFY(philipsOut.writeMatrix(other, *sh, 2, mh));
    // other slices than the frame holds
    const int sliceBytes = static_cast<CPhilipsSubHeaderImage*>(sh)->xdim() * static_cast<CPhilipsSubHeaderImage*>(sh)->ydim() * 2;
    QVERIFY(philipsOut.writeMatrix(data->left(data->size() - sliceBytes), *sh, 2, mh) == false);
    philipsOut.close();

    CPhilipsFile in(tmpFile("philips-order.img"));
    QVERIFY(in.open(QIODevice::ReadOnly));
    QCOMPARE(static_cast<int>(in.numFrames()), 3);
    for(int f=1; f <= 3; f++) {
      QByteArray* frameData = NULL;
      CMedIOHeader* frameHeader = NULL;
      QVERIFY(in.readMatrix(frameData, frameHeader, CMedIOData::Philips, f));
      QVERIFY(*frameData == (f == 2 ? other : *data));
      delete frameData;
      delete frameHeader;
    }
  }

  // frames that do not exist
  QByteArray* none = NULL;
  QVERIFY(philips.readMatrix(none, sh, CMedIOData::ECAT, 99) == false);
  QVERIFY(none == NULL);
}

//=============================================================================================
// microPET (read only)

// synthetic microPET image: 4 x 3 x 2 voxels, 2 frames, int16 (Intel byte
// order), stored value 1 + i + 100 * frame, frame f with scale factor 0.5 / (f + 1)
static bool writeConcorde(const QString& path)
{
  QFile header(path + ".hdr");
  if(header.open(QIODevice::WriteOnly | QIODevice::Text) == false)
    return false;

  QTextStream h(&header);
  h << "# synthetic microPET header (libmedio test)\n"
    << "model 2000\n"
    << "file_type 5\n"
    << "acquisition_mode 3\n"
    << "total_frames 2\n"
    << "isotope F-18\n"
    << "isotope_half_life 6586.2\n"
    << "isotope_branching_fraction 0.8\n"
    << "data_type 2\n"
    << "number_of_dimensions 3\n"
    << "x_dimension 4\n"
    << "y_dimension 3\n"
    << "z_dimension 2\n"
    << "pixel_size 0.2\n"
    << "axial_crystal_pitch 0.6\n"
    << "calibration_units 2\n"
    << "calibration_factor 1000\n"
    << "subject_identifier test\n"
    << "end_of_header\n";
  for(int f=0; f < 2; f++) {
    h << "frame " << f << "\n"
      << "event_type 1\n"
      << "gate 0\n"
      << "bed 0\n"
      << "bed_offset 0\n"
      << "data_file_pointer 0 " << f * 48 << "\n"
      << "frame_start " << f * 60 << "\n"
      << "frame_duration " << 60 * (f + 1) << "\n"
      << "scale_factor " << 0.5 / (f + 1) << "\n"
      << "minimum " << 1 + 100 * f << "\n"
      << "maximum " << 24 + 100 * f << "\n"
      << "end_of_header\n";
  }
  header.close();

  QFile data(path);
  if(data.open(QIODevice::WriteOnly) == false)
    return false;
  for(int f=0; f < 2; f++)
    for(int i=0; i < 24; i++) {
      const qint16 v = qToLittleEndian(static_cast<qint16>(1 + i + 100 * f));
      data.write(reinterpret_cast<const char*>(&v), 2);
    }
  return true;
}

void TestMedIOMatrix::concorde()
{
  const QString path = tmpFile("mpet_image");
  QVERIFY(writeConcorde(path));

  QScopedPointer<CMedIOData> file(CMedIODataFactory::create(path));
  QVERIFY(file.data() != NULL);
  QCOMPARE(static_cast<int>(file->dataFormat()), static_cast<int>(CMedIOData::ConcordeMicropet));
  QVERIFY(file->open(QIODevice::ReadOnly));

  CMedIOHeader* ecatMain = NULL;
  QVERIFY(file->readMainHeader(ecatMain, CMedIOData::ECAT));
  QScopedPointer<CMedIOHeader> ecatMainGuard(ecatMain);
  QCOMPARE(static_cast<int>(static_cast<CECAT7MainHeader*>(ecatMain)->num_Frames()), 2);
  CMedIOHeader* concordeMain = NULL;
  QVERIFY(file->readMainHeader(concordeMain, CMedIOData::ConcordeMicropet));
  QScopedPointer<CMedIOHeader> concordeMainGuard(concordeMain);
  QCOMPARE(static_cast<int>(concordeMain->headerFormat()), static_cast<int>(CMedIOHeader::ConcordeMicroPetMainHeader));

  for(int f=1; f <= 2; f++) {
    // physical value = stored x scale_factor / branching fraction x calibration factor (as mp2ecat)
    const double scale = 0.5 / f / 0.8 * 1000.0;

    // the voxels of the file in ECAT7 voxel order (as mp2ecat)
    CMedIOVolume v;
    QVERIFY(file->readVolume(v, f));
    QCOMPARE(v.dataType(), CMedIOVolume::Int16);
    QCOMPARE(v.dimension(0), 4); QCOMPARE(v.dimension(1), 3); QCOMPARE(v.dimension(2), 2);
    QCOMPARE(v.orientation(), QString("LPI"));
    QVERIFY(approx(v.scaleFactor(), scale, 1e-6));
    QVERIFY(approx(v.voxelSize(0), 2.0, 1e-6));
    QVERIFY(approx(v.voxelSize(2), 3.0, 1e-6));
    QCOMPARE(v.frameStart(), (f - 1) * 60.0);
    QCOMPARE(v.frameDuration(), f * 60.0);
    for(int i=0; i < 24; i++)
      QCOMPARE(v.storedValue(i), 1.0 + i + 100 * (f - 1));

    // ECAT7 representation
    QByteArray* data = NULL;
    CMedIOHeader* sh = NULL;
    QVERIFY(file->readMatrix(data, sh, CMedIOData::ECAT, f));
    QScopedPointer<QByteArray> dataGuard(data);
    QScopedPointer<CMedIOHeader> shGuard(sh);
    const CECAT7SubHeaderImage* s = static_cast<const CECAT7SubHeaderImage*>(sh);
    QCOMPARE(static_cast<int>(s->data_Type()), static_cast<int>(CECATSubHeader::SunShort));
    QVERIFY(approx(s->scale_Factor(), scale, 1e-6));
    QCOMPARE(s->frame_Duration(), static_cast<unsigned int>(f * 60000));
    for(int i=0; i < 24; i++)
      QCOMPARE(ecatValue(data->constData(), i, CECATSubHeader::SunShort), 1.0 + i + 100 * (f - 1));

    // NIfTI representation: planes reversed
    QVERIFY(file->readMatrix(data, sh, CMedIOData::NIFTI, f));
    QScopedPointer<QByteArray> niftiDataGuard(data);
    QScopedPointer<CMedIOHeader> niftiHeaderGuard(sh);
    const NiftiHeader n = niftiHeader(sh);
    QCOMPARE(n.datatype, 4);
    QVERIFY(approx(n.slope, scale, 1e-6));
    for(int k=0; k < 2; k++)
      for(int i=0; i < 12; i++)
        QCOMPARE(niftiValue(data->constData(), i + 12 * (1 - k), 4), 1.0 + i + 12 * k + 100 * (f - 1));

    // microPET representation of the file itself: unchanged
    QVERIFY(file->readMatrix(data, sh, CMedIOData::ConcordeMicropet, f));
    QScopedPointer<QByteArray> nativeGuard(data);
    QScopedPointer<CMedIOHeader> nativeHeaderGuard(sh);
    QCOMPARE(static_cast<int>(sh->headerFormat()), static_cast<int>(CMedIOHeader::ConcordeMicroPetFrameHeader));
    QCOMPARE(data->size(), 48);
    QCOMPARE(static_cast<double>(static_cast<CConcordeFrameHeader*>(sh)->scaleFactor()), 0.5 / f);

    // ... which can be written into an ECAT7 file with the microPET main header
    CECATFile out(tmpFile(QString("mpet-%1.v").arg(f)), CECATMainHeader::ECAT7_Volume16);
    QVERIFY(createEcat(out, *static_cast<CECAT7MainHeader*>(ecatMain)));
    QVERIFY(out.writeMatrix(*data, *sh, 1) == false);   // main header missing
    QVERIFY(out.writeMatrix(*data, *sh, 1, concordeMain));
    out.close();
    EcatFile written;
    QVERIFY(written.read(tmpFile(QString("mpet-%1.v").arg(f))));
    for(int i=0; i < 24; i++)
      QVERIFY(approx(written.value(0, i), (1.0 + i + 100 * (f - 1)) * scale, 1e-6));
  }

  // microPET files cannot be written
  CECATFile ecat(medioTestDataDir() + "/ecat/pet-static.v");
  QVERIFY(ecat.open(QIODevice::ReadOnly));
  QByteArray* data = NULL;
  CMedIOHeader* sh = NULL;
  QVERIFY(ecat.readMatrix(data, sh, CMedIOData::ConcordeMicropet, 1) == false);
  QVERIFY(ecat.readMainHeader(sh, CMedIOData::ConcordeMicropet) == false);
  CMedIOVolume v;
  QVERIFY(file->readVolume(v, 1));
  QVERIFY(file->writeVolume(v, 1) == false);
  QVERIFY(file->readVolume(v, 3) == false);
}

//=============================================================================================
// readMainHeader()/readMatrix() without format: the format of the file itself

//---------------------------------------------------------------------------------------------
// frame numbering: a file with three frames yields exactly these three frames
// in every representation and conversion, whatever numbers the format gives
// them. ECAT7 and Philips files address their frames by the frame numbers
// stored in the file, which do not have to start with 1 and can have gaps
// (both formats allow frame 0, see frameNumberRows()); NIfTI volumes and microPET frame sections are
// addressed by their position (1 = first). The frames of pet-dynamic (three
// frames with different scale factors) are written with other frame numbers;
// the expected values are those of ecat/pet-dynamic.v (nibabel), for
// microPET synthetic values.

// stored and physical value of voxel i of frame k (0-based) of writeConcordeFrames()
static double concordeStored(int k, qint64 i) { return 1.0 + i + 100.0 * k; }
static double concordeValue(int k, qint64 i) { return concordeStored(k, i) * 0.5 / (k + 1) / 0.8 * 1000.0; }

// microPET image (4 x 3 x 2 voxels) with one frame section per entry of
// sectionNumbers (the frame numbers given in the header)
static bool writeConcordeFrames(const QString& path, const QVector<int>& sectionNumbers)
{
  QFile header(path + ".hdr");
  if(header.open(QIODevice::WriteOnly | QIODevice::Text) == false)
    return false;

  QTextStream h(&header);
  h << "# synthetic microPET header (libmedio test)\n"
    << "model 2000\n"
    << "file_type 5\n"
    << "acquisition_mode 3\n"
    << "total_frames " << sectionNumbers.size() << "\n"
    << "isotope F-18\n"
    << "isotope_half_life 6586.2\n"
    << "isotope_branching_fraction 0.8\n"
    << "data_type 2\n"
    << "number_of_dimensions 3\n"
    << "x_dimension 4\n"
    << "y_dimension 3\n"
    << "z_dimension 2\n"
    << "pixel_size 0.2\n"
    << "axial_crystal_pitch 0.6\n"
    << "calibration_units 2\n"
    << "calibration_factor 1000\n"
    << "subject_identifier test\n"
    << "end_of_header\n";
  for(int k=0; k < sectionNumbers.size(); k++) {
    h << "frame " << sectionNumbers[k] << "\n"
      << "event_type 1\n"
      << "gate 0\n"
      << "bed 0\n"
      << "bed_offset 0\n"
      << "data_file_pointer 0 " << k * 48 << "\n"
      << "frame_start " << k * 60 << "\n"
      << "frame_duration " << 60 * (k + 1) << "\n"
      << "scale_factor " << 0.5 / (k + 1) << "\n"
      << "minimum " << concordeStored(k, 0) << "\n"
      << "maximum " << concordeStored(k, 23) << "\n"
      << "end_of_header\n";
  }
  header.close();

  QFile data(path);
  if(data.open(QIODevice::WriteOnly) == false)
    return false;
  for(int k=0; k < sectionNumbers.size(); k++)
    for(int i=0; i < 24; i++) {
      const qint16 v = qToLittleEndian(static_cast<qint16>(concordeStored(k, i)));
      data.write(reinterpret_cast<const char*>(&v), 2);
    }
  return true;
}

// write the three frames of source (ECAT7 or Philips) as the frames numbers[0..2] of a new file
static bool writeRenumbered(const QString& source, const QString& path, CMedIOData::Format format,
                            int philipsType, const QVector<int>& numbers)
{
  QScopedPointer<CMedIOData> in(CMedIODataFactory::create(source));
  if(in.data() == NULL || in->open(QIODevice::ReadOnly) == false)
    return false;

  CMedIOHeader* mh = NULL;
  if(in->readMainHeader(mh, format) == false || mh == NULL)
    return false;
  QScopedPointer<CMedIOHeader> mhGuard(mh);

  QScopedPointer<CMedIOData> out;
  bool result = false;
  if(format == CMedIOData::ECAT) {
    CECATFile* ecat = new CECATFile(path, CECATMainHeader::ECAT7_Volume16);
    out.reset(ecat);
    result = createEcat(*ecat, *static_cast<CECAT7MainHeader*>(mh));
  } else {
    CPhilipsFile* philips = new CPhilipsFile(path, static_cast<CPhilipsMainHeader::File_Type>(philipsType));
    out.reset(philips);
    CPhilipsMainHeader* philipsMain = static_cast<CPhilipsMainHeader*>(mh);
    result = philips->open(QIODevice::WriteOnly);
    philipsMain->setMedIOData(philips);
    result = result && philips->writeMainHeader(*philipsMain);
    philipsMain->setMedIOData(NULL);
  }

  for(int k=0; k < numbers.size() && result; k++) {
    QByteArray* data = NULL;
    CMedIOHeader* sh = NULL;
    result = in->readMatrix(data, sh, format, k + 1) &&
             out->writeMatrix(*data, *sh, numbers[k], mh);
    delete data;
    delete sh;
  }
  out->close();
  return result;
}

// rows: kind of file and the frame numbers stored in it
static void frameNumberRows()
{
  QTest::addColumn<QString>("kind");
  QTest::addColumn<QVector<int> >("numbers");

  const QVector<int> first1 = QVector<int>() << 1 << 2 << 3;
  const QVector<int> first3 = QVector<int>() << 3 << 4 << 5;
  const QVector<int> first0 = QVector<int>() << 0 << 1 << 2;
  const QVector<int> gaps   = QVector<int>() << 1 << 3 << 5;

  QTest::newRow("ECAT7 frames 1-3")    << "ecat" << first1;
  QTest::newRow("ECAT7 frames 3-5")    << "ecat" << first3;
  QTest::newRow("ECAT7 frames 0-2")    << "ecat" << first0;
  QTest::newRow("ECAT7 frames 1,3,5")  << "ecat" << gaps;
  QTest::newRow("Philips frames 1-3")  << "philips" << first1;
  QTest::newRow("Philips frames 3-5")  << "philips" << first3;
  QTest::newRow("Philips frames 1,3,5") << "philips" << gaps;
  // Philips image files cannot hold frame 0: frame 0, slice 0 has the
  // directory ID 0 of an unused entry. Syntegra files number their slices from 1.
  QTest::newRow("Syntegra frames 3-5") << "syntegra" << first3;
  QTest::newRow("Syntegra frames 0-2") << "syntegra" << first0;
  QTest::newRow("NIfTI volumes 1-3")   << "nifti" << first1;
  QTest::newRow("microPET sections 0-2") << "micropet" << first0;
  QTest::newRow("microPET sections 3-5") << "micropet" << first3;
}

// test file of a row: its path, the frame numbers to read its frames with and the expected values
struct FrameNumberFile
{
  QString path;
  QVector<int> address;   // frame number of the k-th frame for readMatrix()/readVolume()
  QVector<int> missing;   // frame numbers without a frame
  EcatFile reference;     // pet-dynamic (not for microPET)
  bool micropet;
  double divisor;         // NIfTI file of PMOD: without the ECAT7 calibration factor
  double step;            // NIfTI file of PMOD: one quantization step (requantized frames)

  double expected(int k, qint64 i) const { return micropet ? concordeValue(k, i) : reference.value(k, i) / divisor; }
  qint64 voxels() const { return micropet ? 24 : reference.voxels(); }
  double tolerance(int k) const { return micropet ? 1e-9 : qMax(step, 1e-6 * maxAbs(reference, k) / divisor); }
};

static bool frameNumberFile(const QString& tag, const QString& kind, const QVector<int>& numbers,
                            const QString& tmpDir, FrameNumberFile& f)
{
  const QString dataDir = medioTestDataDir();
  const QString base = QDir(tmpDir).filePath(QString(tag).replace(QRegExp("[^a-zA-Z0-9]"), "_"));

  f.micropet = (kind == "micropet");
  f.divisor = 1.0;
  f.step = 0.0;
  if(f.micropet == false && f.reference.read(dataDir + "/ecat/pet-dynamic.v") == false)
    return false;

  bool result = false;
  if(kind == "ecat") {
    f.path = base + ".v";
    result = writeRenumbered(dataDir + "/ecat/pet-dynamic.v", f.path, CMedIOData::ECAT, 0, numbers);
  } else if(kind == "philips" || kind == "syntegra") {
    const bool syntegra = (kind == "syntegra");
    f.path = base + (syntegra ? ".syn" : ".img");
    result = writeRenumbered(dataDir + (syntegra ? "/philips/pet-dynamic.syn" : "/philips/pet-dynamic.img"), f.path,
                             CMedIOData::Philips, syntegra ? CPhilipsMainHeader::Syntegra : CPhilipsMainHeader::Image, numbers);
  } else if(kind == "nifti") {
    // PMOD: one scale factor for all frames, without the calibration factor (see datasets.json)
    f.path = dataDir + "/nifti/pet-dynamic.nii.gz";
    NiftiHeader n;
    result = (readAll(f.path, &n).isEmpty() == false);
    f.divisor = f.reference.mainHeader->calibration_Factor();
    f.step = n.slope;
  } else if(kind == "micropet") {
    f.path = base;
    result = writeConcordeFrames(f.path, numbers);
  }

  // ECAT7 and Philips: frame numbers of the file, NIfTI and microPET: position
  const bool positional = (kind == "nifti" || kind == "micropet");
  f.address.clear();
  for(int k=0; k < numbers.size(); k++)
    f.address.append(positional ? k + 1 : numbers[k]);

  f.missing.clear();
  if(f.address.first() > 0)
    f.missing.append(f.address.first() - 1);
  for(int n=f.address.first() + 1; n < f.address.last(); n++)
    if(f.address.contains(n) == false)
      f.missing.append(n);
  f.missing.append(f.address.last() + 1);

  return result;
}

// physical values of an ECAT7 matrix of a file whose ECAT7 main header has the calibration factor
static bool compareEcatMatrix(const QByteArray& data, const CMedIOHeader* sh, double calibration,
                              const FrameNumberFile& f, int k, QString& message)
{
  if(sh == NULL || sh->headerFormat() != CMedIOHeader::ECATSubHeader ||
     static_cast<const CECATSubHeader*>(sh)->subHeaderType() != CECATSubHeader::ECAT7_Image) {
    message = "no ECAT7 image sub header";
    return false;
  }
  const CECAT7SubHeaderImage* s = static_cast<const CECAT7SubHeaderImage*>(sh);
  const double scale = s->scale_Factor() != 0.0 ? s->scale_Factor() : 1.0;
  if(static_cast<qint64>(s->x_Dimension()) * s->y_Dimension() * s->z_Dimension() != f.voxels()) {
    message = "wrong dimensions";
    return false;
  }
  const double tolerance = f.tolerance(k);
  for(qint64 i=0; i < f.voxels(); i++) {
    const double value = ecatValue(data.constData(), i, s->data_Type()) * scale * calibration;
    if(approx(value, f.expected(k, i), 1e-5, tolerance) == false) {
      message = QString("voxel %1: %2 instead of %3 (values of another frame?)").arg(i).arg(value).arg(f.expected(k, i));
      return false;
    }
  }
  return true;
}

static double ecatCalibration(const CMedIOHeader* mainHeader)
{
  const double c = static_cast<const CECAT7MainHeader*>(mainHeader)->calibration_Factor();
  return c > 0.0 ? c : 1.0;
}

void TestMedIOMatrix::frameNumbersRead_data()  { frameNumberRows(); }
void TestMedIOMatrix::frameNumbersCount_data() { frameNumberRows(); }
void TestMedIOMatrix::frameNumbersToNifti_data() { frameNumberRows(); }

// frameNumbers() lists the frames, every frame can be read with its frame
// number, -1 is the first frame, other numbers fail
void TestMedIOMatrix::frameNumbersRead()
{
  QFETCH(QString, kind);
  QFETCH(QVector<int>, numbers);
  FrameNumberFile f;
  QVERIFY(frameNumberFile(QTest::currentDataTag(), kind, numbers, m_tmp.path(), f));

  QScopedPointer<CMedIOData> file(CMedIODataFactory::create(f.path));
  QVERIFY(file.data() != NULL);
  QVERIFY(file->frameNumbers().isEmpty()); // not open
  QVERIFY(file->open(QIODevice::ReadOnly));

  QList<short> expectedNumbers;
  foreach(int n, f.address)
    expectedNumbers.append(static_cast<short>(n));
  QCOMPARE(file->frameNumbers(), expectedNumbers);

  CMedIOHeader* ecatMain = NULL;
  QVERIFY(file->readMainHeader(ecatMain, CMedIOData::ECAT));
  QScopedPointer<CMedIOHeader> ecatMainGuard(ecatMain);
  const double calibration = ecatCalibration(ecatMain);

  QByteArray firstData;
  for(int k=0; k < f.address.size(); k++) {
    QByteArray* data = NULL;
    CMedIOHeader* sh = NULL;
    QVERIFY2(file->readMatrix(data, sh, CMedIOData::ECAT, f.address[k]), qPrintable(QString("frame %1").arg(f.address[k])));
    QScopedPointer<QByteArray> dataGuard(data);
    QScopedPointer<CMedIOHeader> shGuard(sh);
    QString message;
    QVERIFY2(compareEcatMatrix(*data, sh, calibration, f, k, message), qPrintable(QString("frame %1: %2").arg(f.address[k]).arg(message)));
    if(k == 0)
      firstData = *data;

    CMedIOVolume v;
    QVERIFY2(file->readVolume(v, f.address[k]), qPrintable(QString("volume of frame %1").arg(f.address[k])));
    QCOMPARE(v.voxelCount(), f.voxels());
  }

  // -1: the first frame
  QByteArray* data = NULL;
  CMedIOHeader* sh = NULL;
  QVERIFY(file->readMatrix(data, sh, CMedIOData::ECAT, -1));
  QScopedPointer<QByteArray> dataGuard(data);
  QScopedPointer<CMedIOHeader> shGuard(sh);
  QVERIFY(*data == firstData);

  // frame numbers without a frame
  foreach(int n, f.missing) {
    QByteArray* none = NULL;
    CMedIOHeader* noneHeader = NULL;
    QVERIFY2(file->readMatrix(none, noneHeader, CMedIOData::ECAT, n) == false, qPrintable(QString("frame %1 does not exist").arg(n)));
    QVERIFY(none == NULL && noneHeader == NULL);
  }
}

// the main headers in the representation of every format give the number of
// frames (ECAT7 and Philips: also the main headers written by libmedio)
void TestMedIOMatrix::frameNumbersCount()
{
  QFETCH(QString, kind);
  QFETCH(QVector<int>, numbers);
  FrameNumberFile f;
  QVERIFY(frameNumberFile(QTest::currentDataTag(), kind, numbers, m_tmp.path(), f));

  QScopedPointer<CMedIOData> file(CMedIODataFactory::create(f.path));
  QVERIFY(file.data() != NULL);
  QVERIFY(file->open(QIODevice::ReadOnly));

  CMedIOHeader* ecatMain = NULL;
  QVERIFY(file->readMainHeader(ecatMain, CMedIOData::ECAT));
  QScopedPointer<CMedIOHeader> ecatMainGuard(ecatMain);
  QCOMPARE(static_cast<int>(static_cast<CECAT7MainHeader*>(ecatMain)->num_Frames()), numbers.size());

  CMedIOHeader* niftiMain = NULL;
  QVERIFY(file->readMainHeader(niftiMain, CMedIOData::NIFTI));
  QScopedPointer<CMedIOHeader> niftiMainGuard(niftiMain);
  QCOMPARE(niftiHeader(niftiMain).dim[4], static_cast<qint64>(numbers.size()));

  // Philips: main header written by libmedio
  if(kind == "philips" || kind == "syntegra") {
    CMedIOHeader* philipsMain = NULL;
    QVERIFY(file->readMainHeader(philipsMain, CMedIOData::Philips));
    QScopedPointer<CMedIOHeader> philipsMainGuard(philipsMain);
    QCOMPARE(static_cast<int>(static_cast<CPhilipsMainHeader*>(philipsMain)->nframe()), numbers.size());
  }

  // NIfTI header of a file holding the frames, as delivered with a frame
  QByteArray* data = NULL;
  CMedIOHeader* sh = NULL;
  QVERIFY(file->readMatrix(data, sh, CMedIOData::NIFTI, f.address.first()));
  QScopedPointer<QByteArray> dataGuard(data);
  QScopedPointer<CMedIOHeader> shGuard(sh);
  QCOMPARE(niftiHeader(sh).dim[4], static_cast<qint64>(numbers.size()));
}

// NIfTI file with exactly the frames of f, in their order
static bool checkNiftiFrames(const QString& path, const FrameNumberFile& f, QString& message)
{
  const QString name = QFileInfo(path).fileName();
  CNIFTIFile nifti(path);
  CMedIOHeader* niftiMain = NULL;
  CMedIOHeader* ecatMain = NULL;
  if(nifti.open(QIODevice::ReadOnly) == false || nifti.readMainHeader(niftiMain, CMedIOData::NIFTI) == false ||
     nifti.readMainHeader(ecatMain, CMedIOData::ECAT) == false) {
    delete niftiMain;
    message = name + ": cannot be read";
    return false;
  }
  QScopedPointer<CMedIOHeader> niftiMainGuard(niftiMain), ecatMainGuard(ecatMain);

  const qint64 volumes = niftiHeader(niftiMain).dim[4];
  if(volumes != f.address.size()) {
    message = QString("%1: %2 volumes instead of %3").arg(name).arg(volumes).arg(f.address.size());
    return false;
  }

  for(int k=0; k < f.address.size(); k++) {
    QByteArray* data = NULL;
    CMedIOHeader* sh = NULL;
    const bool read = nifti.readMatrix(data, sh, CMedIOData::ECAT, k + 1);
    QScopedPointer<QByteArray> dataGuard(data);
    QScopedPointer<CMedIOHeader> shGuard(sh);
    QString error = "cannot be read";
    if(read == false || compareEcatMatrix(*data, sh, ecatCalibration(ecatMain), f, k, error) == false) {
      message = QString("%1, volume %2: %3").arg(name).arg(k + 1).arg(error);
      return false;
    }
  }
  return true;
}

// conversions into NIfTI contain every frame once, in their order
void TestMedIOMatrix::frameNumbersToNifti()
{
  QFETCH(QString, kind);
  QFETCH(QVector<int>, numbers);
  FrameNumberFile f;
  QVERIFY(frameNumberFile(QTest::currentDataTag(), kind, numbers, m_tmp.path(), f));
  const QString tag = QString(QTest::currentDataTag()).replace(QRegExp("[^a-zA-Z0-9]"), "_");
  QString message;

  // CNIFTIConverter (ECAT7 and Philips files)
  if(kind != "nifti" && kind != "micropet") {
    const QString converted = tmpFile(tag + "-converted.nii");
    QString error;
    QVERIFY2(CNIFTIConverter::toNifti(f.path, converted, CNIFTIConverter::Options(), &error), qPrintable(error));
    QVERIFY2(checkNiftiFrames(converted, f, message), qPrintable(message));
  }

  // format independent matrix access: main header and frames in the NIfTI representation
  QScopedPointer<CMedIOData> file(CMedIODataFactory::create(f.path));
  QVERIFY(file->open(QIODevice::ReadOnly));
  CMedIOHeader* mh = NULL;
  QVERIFY(file->readMainHeader(mh, CMedIOData::NIFTI));
  QScopedPointer<CMedIOHeader> mhGuard(mh);

  const QString copied = tmpFile(tag + "-copied.nii");
  {
    CNIFTIFile out(copied, CNIFTIMainHeader::NIFTI1);
    QVERIFY(out.open(QIODevice::WriteOnly));
    mh->setMedIOData(&out);
    QVERIFY(out.writeMainHeader(*static_cast<CNIFTIMainHeader*>(mh)));
    mh->setMedIOData(NULL);

    for(int k=0; k < f.address.size(); k++) {
      QByteArray* data = NULL;
      CMedIOHeader* sh = NULL;
      QVERIFY2(file->readMatrix(data, sh, CMedIOData::NIFTI, f.address[k]), qPrintable(QString("frame %1").arg(f.address[k])));
      QScopedPointer<QByteArray> dataGuard(data);
      QScopedPointer<CMedIOHeader> shGuard(sh);
      QVERIFY2(out.writeMatrix(*data, *sh, k + 1), qPrintable(QString("frame %1 as volume %2 of %3").arg(f.address[k]).arg(k + 1).arg(niftiHeader(mh).dim[4])));
    }
    out.close();
  }
  QVERIFY2(checkNiftiFrames(copied, f, message), qPrintable(message));
}

void TestMedIOMatrix::nativeFormat_data()
{
  QTest::addColumn<QString>("file");
  QTest::addColumn<int>("headerFormat");   // CMedIOHeader::Format of the sub header
  QTest::addColumn<int>("frame");          // frame to read in addition to the default one

  QTest::newRow("ECAT7")    << medioTestDataDir() + "/ecat/pet-dynamic.v" << static_cast<int>(CMedIOHeader::ECATSubHeader) << 3;
  QTest::newRow("NIfTI")    << medioTestDataDir() + "/nifti/pet-dynamic.nii.gz" << static_cast<int>(CMedIOHeader::NIFTIMainHeader) << 2;
  QTest::newRow("Philips")  << medioTestDataDir() + "/philips/pet-dynamic.syn" << static_cast<int>(CMedIOHeader::PhilipsSubHeader) << 2;
  QTest::newRow("microPET") << tmpFile("mpet_native") << static_cast<int>(CMedIOHeader::ConcordeMicroPetFrameHeader) << 2;
}

void TestMedIOMatrix::nativeFormat()
{
  QFETCH(QString, file);
  QFETCH(int, headerFormat);
  QFETCH(int, frame);

  if(file.endsWith("mpet_native"))
    QVERIFY(writeConcorde(file));

  QScopedPointer<CMedIOData> data(CMedIODataFactory::create(file));
  QVERIFY(data.data() != NULL);
  QVERIFY(data->open(QIODevice::ReadOnly));

  // main header
  CMedIOHeader* mh = NULL;
  CMedIOHeader* mhFormat = NULL;
  QVERIFY(data->readMainHeader(mh));
  QVERIFY(data->readMainHeader(mhFormat, data->dataFormat()));
  QCOMPARE(static_cast<int>(mh->headerFormat()), static_cast<int>(mhFormat->headerFormat()));
  delete mh;
  delete mhFormat;

  // default frame and given frame, QByteArray and char* variants
  for(int pass=0; pass < 2; pass++) {
    QByteArray* matrix = NULL;
    QByteArray* matrixFormat = NULL;
    CMedIOHeader* sh = NULL;
    CMedIOHeader* shFormat = NULL;
    if(pass == 0) {
      QVERIFY(data->readMatrix(matrix, sh));
      QVERIFY(data->readMatrix(matrixFormat, shFormat, data->dataFormat()));
    } else {
      QVERIFY(data->readMatrix(matrix, sh, frame));
      QVERIFY(data->readMatrix(matrixFormat, shFormat, data->dataFormat(), frame));
    }
    QCOMPARE(static_cast<int>(sh->headerFormat()), headerFormat);
    QCOMPARE(static_cast<int>(shFormat->headerFormat()), headerFormat);
    QVERIFY(*matrix == *matrixFormat);

    char* raw = NULL;
    unsigned int len = 0;
    CMedIOHeader* shRaw = NULL;
    QVERIFY(data->readMatrix(raw, len, shRaw, pass == 0 ? -1 : frame));
    QVERIFY(QByteArray(raw, len) == *matrix);
    delete[] raw;

    delete matrix; delete matrixFormat; delete sh; delete shFormat; delete shRaw;
  }

  // the frames differ
  QByteArray* first = NULL;
  QByteArray* other = NULL;
  CMedIOHeader* sh = NULL;
  QVERIFY(data->readMatrix(first, sh));
  delete sh;
  QVERIFY(data->readMatrix(other, sh, frame));
  delete sh;
  QVERIFY(*first != *other);
  delete first;
  delete other;

  QByteArray* none = NULL;
  QVERIFY(data->readMatrix(none, sh, 99) == false);
  QVERIFY(none == NULL && sh == NULL);
}

//---------------------------------------------------------------------------------------------
// writeMatrix() with the format of the matrix: the same result as without a
// format, also through CECATFile and CPhilipsFile with their own sub header
// types; a format that does not match the sub header is rejected without writing

void TestMedIOMatrix::writeWithFormat()
{
  const QString source = medioTestDataDir() + "/ecat/pet-dynamic.v";
  EcatFile reference;
  QVERIFY(reference.read(source));
  QVERIFY(reference.frames() >= 2);

  CECATFile in(source);
  QVERIFY(in.open(QIODevice::ReadOnly));
  CMedIOHeader* ecatMain = NULL;
  CMedIOHeader* niftiMain = NULL;
  QVERIFY(in.readMainHeader(ecatMain, CMedIOData::ECAT));
  QVERIFY(in.readMainHeader(niftiMain, CMedIOData::NIFTI));
  QScopedPointer<CMedIOHeader> ecatMainGuard(ecatMain), niftiMainGuard(niftiMain);

  QByteArray* m[2] = { NULL, NULL };
  CMedIOHeader* s[2] = { NULL, NULL };
  QVERIFY(in.readMatrix(m[0], s[0], CMedIOData::ECAT, 1));
  QVERIFY(in.readMatrix(m[1], s[1], CMedIOData::ECAT, 2));
  QScopedPointer<QByteArray> m0(m[0]), m1(m[1]);
  QScopedPointer<CMedIOHeader> s0(s[0]), s1(s[1]);
  const CECATSubHeader& ecatSub = static_cast<const CECATSubHeader&>(*s[1]);

  // ECAT7 file
  const QString ecatPath = tmpFile("format.v");
  {
    CECATFile out(ecatPath, CECATMainHeader::ECAT7_Volume16);
    QVERIFY(createEcat(out, *static_cast<CECAT7MainHeader*>(ecatMain)));
    CMedIOData& data = out;

    // rejected: the format does not match the sub header
    QVERIFY(data.writeMatrix(*m[1], *s[1], CMedIOData::NIFTI, 2) == false);
    QVERIFY(out.errorStatus() != CMedIOData::MERR_Ok);
    QVERIFY(data.writeMatrix(m[1]->constData(), m[1]->size(), *s[1], CMedIOData::Philips, 2) == false);
    QVERIFY(out.writeMatrix(*m[1], ecatSub, CMedIOData::Unknown, 2) == false);
    QVERIFY(out.writeMatrix(m[1]->constData(), m[1]->size(), ecatSub, CMedIOData::NIFTI, 2) == false);

    // the format is not taken as frame number: frame -1 is the first frame
    QVERIFY(data.writeMatrix(*m[0], *s[0], CMedIOData::ECAT));
    QVERIFY(out.errorStatus() == CMedIOData::MERR_Ok);
    // CECATFile with its own sub header type and a main header, char* variant
    QVERIFY(out.writeMatrix(m[1]->constData(), m[1]->size(), ecatSub, CMedIOData::ECAT, 2, ecatMain));
    out.close();
  }
  QVERIFY(ecatMatrix(ecatPath, 1) == reference.matrices[0]);
  QVERIFY(ecatMatrix(ecatPath, 2) == reference.matrices[1]);
  QVERIFY(ecatMatrix(ecatPath, 3).isEmpty());

  // NIfTI file: the same file as written without a format
  QByteArray niftiFile[2];
  for(int pass=0; pass < 2; pass++) {
    const QString path = tmpFile(QString("format-%1.nii").arg(pass));
    {
      CNIFTIFile out(path, CNIFTIMainHeader::NIFTI1);
      QVERIFY(createNifti(out, niftiMain));
      if(pass == 0) {
        QVERIFY(out.writeMatrix(*m[0], *s[0], 1, ecatMain));
        QVERIFY(out.writeMatrix(*m[1], *s[1], 2, ecatMain));
      } else {
        QVERIFY(out.writeMatrix(*m[1], ecatSub, CMedIOData::NIFTI, 3, ecatMain) == false);
        QVERIFY(out.writeMatrix(*m[1], *s[1], CMedIOData::ECAT, 2, ecatMain));
        QVERIFY(out.writeMatrix(*m[0], *s[0], CMedIOData::ECAT, -1, ecatMain));
      }
      out.close();
    }
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    niftiFile[pass] = file.readAll();

    CNIFTIFile written(path);
    QVERIFY(written.open(QIODevice::ReadOnly));
    for(short f=1; f <= 2; f++) {
      QByteArray* data = NULL;
      QVERIFY(written.readMatrix(data, f));
      QVERIFY(data->count('\0') < data->size());
      delete data;
    }
  }
  QVERIFY(niftiFile[0] == niftiFile[1]);

  // Philips file: CPhilipsFile with its own sub header type
  CPhilipsFile philipsIn(medioTestDataDir() + "/philips/pet-dynamic.syn");
  QVERIFY(philipsIn.open(QIODevice::ReadOnly));
  CMedIOHeader* philipsMain = NULL;
  QVERIFY(philipsIn.readMainHeader(philipsMain, CMedIOData::Philips));
  QScopedPointer<CMedIOHeader> philipsMainGuard(philipsMain);
  CPhilipsMainHeader* pmh = static_cast<CPhilipsMainHeader*>(philipsMain);

  // the same file as written without a format
  QByteArray philipsFile[2];
  for(int pass=0; pass < 2; pass++) {
    const QString path = tmpFile(QString("format-%1.syn").arg(pass));
    {
      CPhilipsFile out(path, pmh->filtyp());
      QVERIFY(out.open(QIODevice::WriteOnly));
      pmh->setMedIOData(&out);
      QVERIFY(out.writeMainHeader(*pmh));
      pmh->setMedIOData(NULL);

      for(short f=philipsIn.maxFrame(); f >= philipsIn.minFrame(); f--) {
        QByteArray* data = NULL;
        CMedIOHeader* sh = NULL;
        QVERIFY(philipsIn.readMatrix(data, sh, CMedIOData::Philips, f));
        QScopedPointer<QByteArray> dataGuard(data);
        QScopedPointer<CMedIOHeader> shGuard(sh);
        const CPhilipsSubHeader& philipsSub = static_cast<const CPhilipsSubHeader&>(*sh);
        if(pass == 0) {
          QVERIFY(out.writeMatrix(*data, *sh, f, philipsMain));
        } else {
          QVERIFY(out.writeMatrix(*data, philipsSub, CMedIOData::ECAT, f, philipsMain) == false);
          QVERIFY(out.writeMatrix(data->constData(), data->size(), philipsSub, CMedIOData::NIFTI, f, philipsMain) == false);
          QVERIFY(out.writeMatrix(*data, philipsSub, CMedIOData::Philips, f, philipsMain));
        }
      }
      out.close();
    }
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    philipsFile[pass] = file.readAll();

    CPhilipsFile written(path);
    QVERIFY(written.open(QIODevice::ReadOnly));
    QCOMPARE(written.numFrames(), philipsIn.numFrames());
  }
  QVERIFY(philipsFile[0] == philipsFile[1]);
}

//---------------------------------------------------------------------------------------------
// writeMatrix(): a main header that does not belong to the format of the
// matrix is rejected without writing, also where it would not be used (NIfTI
// matrices, ECAT7 matrices written unchanged into an ECAT7 file)

void TestMedIOMatrix::writeMainHeaderFormat()
{
  const QString source = medioTestDataDir() + "/ecat/pet-static.v";
  EcatFile reference;
  QVERIFY(reference.read(source));

  CECATFile in(source);
  QVERIFY(in.open(QIODevice::ReadOnly));
  CMedIOHeader* ecatMain = NULL;
  CMedIOHeader* niftiMain = NULL;
  CMedIOHeader* philipsMain = NULL;
  QVERIFY(in.readMainHeader(ecatMain, CMedIOData::ECAT));
  QVERIFY(in.readMainHeader(niftiMain, CMedIOData::NIFTI));
  QVERIFY(in.readMainHeader(philipsMain, CMedIOData::Philips));
  QScopedPointer<CMedIOHeader> ecatMainGuard(ecatMain), niftiMainGuard(niftiMain), philipsMainGuard(philipsMain);

  QByteArray* ecatData = NULL;
  QByteArray* niftiData = NULL;
  CMedIOHeader* ecatSub = NULL;
  CMedIOHeader* niftiSub = NULL;
  QVERIFY(in.readMatrix(ecatData, ecatSub, CMedIOData::ECAT, 1));
  QVERIFY(in.readMatrix(niftiData, niftiSub, CMedIOData::NIFTI, 1));
  QScopedPointer<QByteArray> ecatDataGuard(ecatData), niftiDataGuard(niftiData);
  QScopedPointer<CMedIOHeader> ecatSubGuard(ecatSub), niftiSubGuard(niftiSub);

  // pass 0: main headers of other formats are rejected, 1: fitting main headers
  for(int pass=0; pass < 2; pass++) {
    // ECAT7 file: ECAT7 matrix written unchanged (frame 1), NIfTI matrix converted (frame 2)
    const QString ecatPath = tmpFile(QString("main-header-%1.v").arg(pass));
    {
      CECATFile out(ecatPath, CECATMainHeader::ECAT7_Volume16);
      QVERIFY(createEcat(out, *static_cast<CECAT7MainHeader*>(ecatMain)));
      if(pass == 0) {
        QVERIFY(out.writeMatrix(*ecatData, *ecatSub, 1, philipsMain) == false);
        QVERIFY(out.errorStatus() != CMedIOData::MERR_Ok);
        QVERIFY(out.writeMatrix(*ecatData, *ecatSub, 1, niftiMain) == false);
        QVERIFY(out.writeMatrix(*ecatData, *ecatSub, 1, ecatSub) == false);   // a sub header
        QVERIFY(out.writeMatrix(*ecatData, *ecatSub, CMedIOData::ECAT, 1, philipsMain) == false);
        QVERIFY(out.writeMatrix(*niftiData, *niftiSub, 2, ecatMain) == false);
        QVERIFY(out.writeMatrix(niftiData->constData(), niftiData->size(), *niftiSub, 2, philipsMain) == false);
      } else {
        QVERIFY(out.writeMatrix(*ecatData, *ecatSub, 1, ecatMain));
        QVERIFY(out.errorStatus() == CMedIOData::MERR_Ok);
        QVERIFY(out.writeMatrix(*niftiData, *niftiSub, 2, niftiMain));
      }
      out.close();
    }
    QCOMPARE(ecatMatrix(ecatPath, 1).isEmpty(), pass == 0);
    QCOMPARE(ecatMatrix(ecatPath, 2).isEmpty(), pass == 0);
    if(pass == 1)
      QVERIFY(ecatMatrix(ecatPath, 1) == reference.matrices[0]);

    // NIfTI file: the main header of NIfTI matrices is not used
    const QString niftiPath = tmpFile(QString("main-header-%1.nii").arg(pass));
    {
      CNIFTIFile out(niftiPath, CNIFTIMainHeader::NIFTI1);
      QVERIFY(createNifti(out, niftiMain));
      if(pass == 0) {
        QVERIFY(out.writeMatrix(*niftiData, *niftiSub, 1, ecatMain) == false);
        QVERIFY(out.writeMatrix(*niftiData, *niftiSub, CMedIOData::NIFTI, 1, philipsMain) == false);
        QVERIFY(out.writeMatrix(*ecatData, *ecatSub, 1, niftiMain) == false);
      } else {
        QVERIFY(out.writeMatrix(*niftiData, *niftiSub, 1, niftiMain));
      }
      out.close();
    }
    CNIFTIFile written(niftiPath);
    QVERIFY(written.open(QIODevice::ReadOnly));
    QByteArray* data = NULL;
    const bool read = written.readMatrix(data, 1);
    QScopedPointer<QByteArray> dataGuard(data);
    if(pass == 0)
      QVERIFY(read == false || data->count('\0') == data->size());
    else
      QVERIFY(read && *data == *niftiData);
  }
}

//---------------------------------------------------------------------------------------------

void TestMedIOMatrix::errors()
{
  CECATFile ecat(medioTestDataDir() + "/ecat/pet-static.v");
  QVERIFY(ecat.open(QIODevice::ReadOnly));

  QByteArray* data = NULL;
  CMedIOHeader* header = NULL;
  QVERIFY(ecat.readMatrix(data, header, CMedIOData::Unknown, 1) == false);
  QVERIFY(data == NULL && header == NULL);
  QVERIFY(ecat.errorStatus() != CMedIOData::MERR_Ok);
  QVERIFY(ecat.readMatrix(data, header, CMedIOData::PhilipsBin, 1) == false);
  QVERIFY(ecat.readMatrix(data, header, CMedIOData::ECAT, 99) == false);
  QVERIFY(ecat.readMainHeader(header, CMedIOData::Unknown) == false);
  QVERIFY(header == NULL);

  CMedIOVolume volume;
  QVERIFY(ecat.readVolume(volume, 99) == false);
  QVERIFY(ecat.readVolume(volume, 1));
  QVERIFY(ecat.errorStatus() == CMedIOData::MERR_Ok);

  // empty volume
  CECATFile out2(tmpFile("empty.v"), CECATMainHeader::ECAT7_Volume16);
  QVERIFY(createEcat(out2, CECAT7MainHeader(NULL, CECATMainHeader::ECAT7_Volume16)));
  QVERIFY(out2.writeVolume(CMedIOVolume(), 1) == false);
  out2.close();
}

QTEST_GUILESS_MAIN(TestMedIOMatrix)
#include "testMedIOMatrix.moc"
