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

// Tests of the frame-wise matrix access of CNIFTIFile in the native and in
// the ECAT7 voxel order and of the ECAT7 representation of NIfTI volumes
// (readMatrix()/writeMatrix() with CMedIOData::Orientation or CECATSubHeader).
//
// The expected values are independent of libmedio:
// - nibabel reference files (data/generate_fixtures.py) with a known value
//   pattern and affine,
// - the ECAT7 and NIfTI files of the same datasets in testsuite/data, which
//   were written by different tools (ROVER, PMOD, ecat2nifti),
// - synthetic files whose ECAT7 voxel order is computed in the test from
//   their affine and the documented ECAT7 geometry (see CNIFTIConverter.h):
//
//   ECAT7 voxel (i, j, k) is located at (LPS, mm)
//     x = x_offset + (i - (nx-1)/2) * px
//     y = y_offset + (j - (ny-1)/2) * py
//     z = bed position + z_offset - k * pz

#include <CNIFTIFile.h>
#include <CNIFTIMainHeader.h>
#include <CNIFTI1MainHeader.h>
#include <CNIFTI2MainHeader.h>
#include <CECATFile.h>
#include <CECATMainHeader.h>
#include <CECATSubHeader.h>
#include <CECAT7MainHeader.h>
#include <CECAT7SubHeaderImage.h>
#include <CECAT7SubHeaderScan.h>
#include <CMedIOData.h>

#include "MedIOTestData.h"

#include <QtTest/QtTest>
#include <QTemporaryDir>

#include <cmath>
#include <cstring>
#include <limits>

#ifndef NIFTI_TEST_DATA_DIR
#error "NIFTI_TEST_DATA_DIR has to be defined"
#endif

//=============================================================================================
// helpers

// value of voxel i of NIfTI data in machine byte order
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

// value of voxel i of ECAT7 data in machine byte order
static double ecatValue(const char* data, qint64 i, int type)
{
  switch(type) {
    case CECATSubHeader::ByteData:  return reinterpret_cast<const quint8*>(data)[i];
    case CECATSubHeader::SunLong:   return reinterpret_cast<const qint32*>(data)[i];
    case CECATSubHeader::IEEEFloat: return reinterpret_cast<const float*>(data)[i];
  }
  return reinterpret_cast<const qint16*>(data)[i];
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

// voxel value of linear index i in the nibabel reference files (see data/generate_fixtures.py)
static double patternValue(qint64 i, int datatype)
{
  switch(datatype) {
    case 16:
    case 64:  return i * 0.25 - 2.0;
    case 2:   return (i * 7) % 50;
    default:  return (i * 7) % 50 - 10;
  }
}

static bool approx(double a, double b, double relative, double absolute = 0.0)
{
  return std::fabs(a - b) <= qMax(absolute, relative * qMax(std::fabs(a), std::fabs(b)));
}

// geometry of the ECAT7 representation: sub header of a volume and bed
// position of the ECAT7 main header converted from the NIfTI main header
struct EcatGeometry
{
  int nx, ny, nz;
  double px, py, pz, xOffset, yOffset, zPosition; // mm

  bool set(const CECATSubHeader* sh, const CNIFTIMainHeader* niftiHeader)
  {
    if(sh == NULL || sh->subHeaderType() != CECATSubHeader::ECAT7_Image)
      return false;
    const CECAT7SubHeaderImage* s = static_cast<const CECAT7SubHeaderImage*>(sh);

    CECAT7MainHeader mainHeader;
    if(mainHeader.convertFrom(niftiHeader, NULL) == false)
      return false;

    nx = s->x_Dimension(); ny = s->y_Dimension(); nz = s->z_Dimension();
    px = s->x_Pixel_Size() * 10.0; py = s->y_Pixel_Size() * 10.0; pz = s->z_Pixel_Size() * 10.0;
    xOffset = s->x_Offset() * 10.0; yOffset = s->y_Offset() * 10.0;
    zPosition = (mainHeader.init_Bed_Position() + s->z_Offset()) * 10.0;
    return true;
  }

  // LPS position (mm) of ECAT7 voxel (i, j, k)
  void position(int i, int j, int k, double p[3]) const
  {
    p[0] = xOffset + (i - (nx - 1) / 2.0) * px;
    p[1] = yOffset + (j - (ny - 1) / 2.0) * py;
    p[2] = zPosition - k * pz;
  }
};

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

static int bitsOf(int datatype)
{
  return datatype == 32 ? 64 : niftiSize(datatype) * 8;
}

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
  h->setBit_Pix(bitsOf(s.datatype));
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

// complete voxel data of a NIfTI file (machine byte order)
static QByteArray readAll(const QString& path)
{
  CNIFTIFile file(path);
  QByteArray* data = NULL;
  if(file.open(QIODevice::ReadOnly) == false || file.readMatrix(data) == false || data == NULL)
    return QByteArray();
  QByteArray result(*data);
  delete data;
  return result;
}

// ECAT7 image file read with CECATFile
struct EcatFile
{
  CECAT7MainHeader* mainHeader;
  QList<CECAT7SubHeaderImage*> subHeaders;
  QList<QByteArray> matrices;

  EcatFile() : mainHeader(NULL) { }
  ~EcatFile() { delete mainHeader; qDeleteAll(subHeaders); }

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
      matrices.append(*data);
      delete data;
    }
    return true;
  }
};

//=============================================================================================

class TestNIFTIMatrix : public QObject
{
  Q_OBJECT

  private:
    QTemporaryDir m_tmp;
    QString tmpFile(const QString& name) const { return m_tmp.filePath(name); }

  private slots:
    void initTestCase();

    void referenceFiles_data();
    void referenceFiles();

    void orientations_data();
    void orientations();

    void dataTypes_data();
    void dataTypes();

    void datasets_data();
    void datasets();

    void niftiToEcatFile();

    void writeFromEcat_data();
    void writeFromEcat();
    void writeRequantized();

    void errors();
};

void TestNIFTIMatrix::initTestCase()
{
  QVERIFY(m_tmp.isValid());
}

//---------------------------------------------------------------------------------------------
// nibabel reference files: affine diag(-2, 2.5, 3) + (10, -20, -30) for all
// files, i.e. NIfTI voxel (i, j, k) is ECAT7 voxel (i, ny-1-j, nz-1-k)
// (ECAT7 y towards posterior, planes from superior to inferior)

void TestNIFTIMatrix::referenceFiles_data()
{
  QTest::addColumn<QString>("file");
  QTest::addColumn<int>("frames");

  QTest::newRow("n1_le_int16.nii")          << "n1_le_int16.nii" << 1;
  QTest::newRow("n1_be_float32_4d.nii")     << "n1_be_float32_4d.nii" << 2;
  QTest::newRow("n1_le_uint8.nii.gz")       << "n1_le_uint8.nii.gz" << 1;
  QTest::newRow("n1_le_int16_ext.nii")      << "n1_le_int16_ext.nii" << 1;
  QTest::newRow("n2_le_float64.nii")        << "n2_le_float64.nii" << 1;
  QTest::newRow("n2_be_int32_ext.nii")      << "n2_be_int32_ext.nii" << 1;
  QTest::newRow("n2_le_int16_4d.nii.gz")    << "n2_le_int16_4d.nii.gz" << 2;
  QTest::newRow("p1_le_int16_ext.hdr")      << "p1_le_int16_ext.hdr" << 1;
  QTest::newRow("p2_be_float32_4d.hdr")     << "p2_be_float32_4d.hdr" << 2;
  QTest::newRow("p1_le_uint8.hdr.gz")       << "p1_le_uint8.hdr.gz" << 1;
  QTest::newRow("p1_le_int16_offset.img")   << "p1_le_int16_offset.img" << 1;
}

void TestNIFTIMatrix::referenceFiles()
{
  QFETCH(QString, file);
  QFETCH(int, frames);

  const int nx = 4, ny = 3, nz = 2;
  const qint64 voxels = nx * ny * nz;
  const double affine[3][4] = { { -2, 0, 0, 10 }, { 0, 2.5, 0, -20 }, { 0, 0, 3, -30 } };

  CNIFTIFile nifti(QString(NIFTI_TEST_DATA_DIR) + "/" + file);
  QVERIFY(nifti.open(QIODevice::ReadOnly));
  CNIFTIMainHeader* header = NULL;
  QVERIFY(nifti.readMainHeader(header));
  QScopedPointer<CNIFTIMainHeader> headerGuard(header);
  const int datatype = (header->mainHeaderType() == CNIFTIMainHeader::NIFTI1MainHeader)
                       ? static_cast<CNIFTI1MainHeader*>(header)->dataType()
                       : static_cast<CNIFTI2MainHeader*>(header)->dataType();
  const int size = niftiSize(datatype);
  QVERIFY(size > 0);

  for(int f=1; f <= frames; f++) {
    const qint64 first = (f - 1) * voxels;

    // native voxel order
    QByteArray* native = NULL;
    QVERIFY(nifti.readMatrix(native, CMedIOData::NativeOrientation, f));
    QScopedPointer<QByteArray> nativeGuard(native);
    QCOMPARE(native->size(), static_cast<int>(voxels * size));
    for(qint64 n=0; n < voxels; n++)
      QCOMPARE(niftiValue(native->constData(), n, datatype), patternValue(first + n, datatype));

    // ECAT7 voxel order, NIfTI datatype
    QByteArray* ordered = NULL;
    QVERIFY(nifti.readMatrix(ordered, CMedIOData::ECATOrientation, f));
    QScopedPointer<QByteArray> orderedGuard(ordered);
    QCOMPARE(ordered->size(), static_cast<int>(voxels * size));

    // ECAT7 representation: scl_inter = 1 -> physical values as float32
    QByteArray* data = NULL;
    CECATSubHeader* sh = NULL;
    QVERIFY(nifti.readMatrix(data, sh, f));
    QScopedPointer<QByteArray> dataGuard(data);
    QScopedPointer<CECATSubHeader> shGuard(sh);
    const CECAT7SubHeaderImage* s = static_cast<const CECAT7SubHeaderImage*>(sh);
    QCOMPARE(static_cast<int>(sh->subHeaderType()), static_cast<int>(CECATSubHeader::ECAT7_Image));
    QCOMPARE(static_cast<int>(s->data_Type()), static_cast<int>(CECATSubHeader::IEEEFloat));
    QCOMPARE(static_cast<double>(s->scale_Factor()), 1.0);
    QCOMPARE(data->size(), static_cast<int>(voxels * 4));

    EcatGeometry g;
    QVERIFY(g.set(sh, header));
    QCOMPARE(g.nx, nx); QCOMPARE(g.ny, ny); QCOMPARE(g.nz, nz);

    for(int k=0; k < nz; k++)
      for(int j=0; j < ny; j++)
        for(int i=0; i < nx; i++) {
          const qint64 e = i + nx * (j + ny * k);
          const double v[3] = { static_cast<double>(i), static_cast<double>(ny - 1 - j), static_cast<double>(nz - 1 - k) };
          const qint64 n = i + nx * ((ny - 1 - j) + ny * (nz - 1 - k));

          QCOMPARE(niftiValue(ordered->constData(), e, datatype), patternValue(first + n, datatype));
          QVERIFY(approx(ecatValue(data->constData(), e, CECATSubHeader::IEEEFloat),
                        static_cast<double>(static_cast<float>(patternValue(first + n, datatype) * 0.5 + 1.0)), 1e-6));

          // same position in both representations
          double pe[3], pn[3];
          g.position(i, j, k, pe);
          niftiPosition(affine, v, pn);
          for(int r=0; r < 3; r++)
            QVERIFY2(approx(pe[r], pn[r], 0.0, 1e-3), qPrintable(QString("voxel (%1,%2,%3) axis %4: %5 != %6").arg(i).arg(j).arg(k).arg(r).arg(pe[r]).arg(pn[r])));
        }

    // the char* variants return the same data
    char* raw = NULL;
    unsigned int len = 0;
    QVERIFY(nifti.readMatrix(raw, len, CMedIOData::ECATOrientation, f));
    QCOMPARE(QByteArray(raw, len), *ordered);
    delete[] raw;

    CECATSubHeader* sh2 = NULL;
    QVERIFY(nifti.readMatrix(raw, len, sh2, f));
    QCOMPARE(QByteArray(raw, len), *data);
    delete[] raw;
    delete sh2;
  }

  // -1 is the first frame, other frames do not exist
  QByteArray* first = NULL;
  QByteArray* defaultFrame = NULL;
  QVERIFY(nifti.readMatrix(first, CMedIOData::ECATOrientation, 1));
  QVERIFY(nifti.readMatrix(defaultFrame, CMedIOData::ECATOrientation));
  QCOMPARE(*defaultFrame, *first);
  delete first;
  delete defaultFrame;

  QByteArray* none = NULL;
  QVERIFY(nifti.readMatrix(none, CMedIOData::ECATOrientation, frames + 1) == false);
  QVERIFY(none == NULL);
  QVERIFY(nifti.readMatrix(none, CMedIOData::NativeOrientation, 0) == false);
  CECATSubHeader* noHeader = NULL;
  QVERIFY(nifti.readMatrix(none, noHeader, frames + 1) == false);
  QVERIFY(none == NULL && noHeader == NULL);
}

//---------------------------------------------------------------------------------------------
// synthetic files with different orientations (2 frames, values 1 + linear
// index): reading in the ECAT7 voxel order, the frame timing and writing in
// the ECAT7 voxel order (inverse of reading)

void TestNIFTIMatrix::orientations_data()
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

void TestNIFTIMatrix::orientations()
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
  CNIFTIMainHeader* header = NULL;
  QVERIFY(nifti.readMainHeader(header));
  QScopedPointer<CNIFTIMainHeader> headerGuard(header);

  QList<QByteArray> ordered;
  QList<QByteArray> ecatData;
  QList<CECAT7SubHeaderImage*> subHeaders;

  for(int f=1; f <= s.frames; f++) {
    QByteArray* data = NULL;
    CECATSubHeader* sh = NULL;
    QVERIFY(nifti.readMatrix(data, sh, f));
    QScopedPointer<QByteArray> dataGuard(data);
    const CECAT7SubHeaderImage* sub = static_cast<const CECAT7SubHeaderImage*>(sh);
    subHeaders.append(static_cast<CECAT7SubHeaderImage*>(sh));
    ecatData.append(*data);

    QCOMPARE(static_cast<int>(sub->data_Type()), static_cast<int>(CECATSubHeader::SunShort));
    QCOMPARE(static_cast<double>(sub->scale_Factor()), 1.0);
    QCOMPARE(sub->frame_Start_Time(), static_cast<unsigned int>(qRound((1000.0 + (f - 1) * 500.0) * timeScale * 1000.0)));
    QCOMPARE(sub->frame_Duration(), static_cast<unsigned int>(qRound(500.0 * timeScale * 1000.0)));

    QByteArray* orderedData = NULL;
    QVERIFY(nifti.readMatrix(orderedData, CMedIOData::ECATOrientation, f));
    ordered.append(*orderedData);
    QCOMPARE(*orderedData, *data); // int16 without scaling: same data
    delete orderedData;

    EcatGeometry g;
    QVERIFY(g.set(sh, header));
    QCOMPARE(g.nx * g.ny * g.nz, static_cast<int>(voxels));

    // every ECAT7 voxel holds the value of the NIfTI voxel at the same position
    for(int k=0; k < g.nz; k++)
      for(int j=0; j < g.ny; j++)
        for(int i=0; i < g.nx; i++) {
          double p[3], v[3];
          g.position(i, j, k, p);
          niftiVoxel(a, p, v);
          for(int r=0; r < 3; r++) {
            const int n[3] = { s.nx, s.ny, s.nz };
            QVERIFY2(std::fabs(v[r] - qRound(v[r])) < 1e-3 && qRound(v[r]) >= 0 && qRound(v[r]) < n[r],
                     qPrintable(QString("ECAT voxel (%1,%2,%3) maps to NIfTI (%4,%5,%6)").arg(i).arg(j).arg(k).arg(v[0]).arg(v[1]).arg(v[2])));
          }
          const double expected = 1 + qRound(v[0]) + s.nx * (qRound(v[1]) + s.ny * (qRound(v[2]) + s.nz * (f - 1)));
          QCOMPARE(ecatValue(data->constData(), i + g.nx * (j + g.ny * k), CECATSubHeader::SunShort), expected);
        }
  }

  // writing in ECAT7 voxel order is the inverse of reading (frames in any order)
  {
    CNIFTIFile out(tmpFile(name + ".ordered.nii"), version == 2 ? CNIFTIMainHeader::NIFTI2 : CNIFTIMainHeader::NIFTI1);
    QVERIFY(createNifti(out, s));
    for(int f=s.frames; f >= 1; f--)
      QVERIFY(out.writeMatrix(ordered[f-1], CMedIOData::ECATOrientation, f));
    out.close();
    QCOMPARE(readAll(tmpFile(name + ".ordered.nii")), values);
  }

  // and so is writing the ECAT7 representation (char* variant, gzip compressed)
  {
    CNIFTIFile out(tmpFile(name + ".ecat.nii.gz"), version == 2 ? CNIFTIMainHeader::NIFTI2 : CNIFTIMainHeader::NIFTI1);
    QVERIFY(createNifti(out, s));
    for(int f=s.frames; f >= 1; f--)
      QVERIFY(out.writeMatrix(ecatData[f-1].constData(), ecatData[f-1].size(), *subHeaders[f-1], f));
    out.close();
    QCOMPARE(readAll(tmpFile(name + ".ecat.nii.gz")), values);
  }

  qDeleteAll(subHeaders);
}

//---------------------------------------------------------------------------------------------
// NIfTI datatypes and scaling -> ECAT7 data type and scale factor

void TestNIFTIMatrix::dataTypes_data()
{
  QTest::addColumn<int>("datatype");
  QTest::addColumn<double>("slope");
  QTest::addColumn<double>("inter");
  QTest::addColumn<double>("lo");      // stored values from lo to hi
  QTest::addColumn<double>("hi");
  QTest::addColumn<int>("ecatType");
  QTest::addColumn<double>("scale");   // 0: physical values (scale factor 1)

  QTest::newRow("uint8")             << 2   << 2.0 << 0.0 << 0.0 << 255.0 << static_cast<int>(CECATSubHeader::ByteData) << 2.0;
  QTest::newRow("int8")              << 256 << 2.0 << 0.0 << -128.0 << 127.0 << static_cast<int>(CECATSubHeader::SunShort) << 2.0;
  QTest::newRow("int16")             << 4   << 0.25 << 0.0 << -32768.0 << 32767.0 << static_cast<int>(CECATSubHeader::SunShort) << 0.25;
  QTest::newRow("uint16")            << 512 << 0.5 << 0.0 << 0.0 << 65535.0 << static_cast<int>(CECATSubHeader::SunLong) << 0.5;
  QTest::newRow("int32")             << 8   << 3.0 << 0.0 << -2147483648.0 << 2147483647.0 << static_cast<int>(CECATSubHeader::SunLong) << 3.0;
  QTest::newRow("uint32 int32 range") << 768 << 1.5 << 0.0 << 0.0 << 2147483647.0 << static_cast<int>(CECATSubHeader::SunLong) << 1.5;
  QTest::newRow("uint32 large")      << 768 << 1.5 << 0.0 << 0.0 << 4294967295.0 << static_cast<int>(CECATSubHeader::IEEEFloat) << 0.0;
  QTest::newRow("float32")           << 16  << 2.0 << 0.0 << -2.5 << 1e6 << static_cast<int>(CECATSubHeader::IEEEFloat) << 2.0;
  QTest::newRow("float64")           << 64  << 2.0 << 0.0 << -1e-3 << 1e10 << static_cast<int>(CECATSubHeader::IEEEFloat) << 0.0;
  QTest::newRow("int16 scl_inter")   << 4   << 0.5 << 5.0 << -100.0 << 100.0 << static_cast<int>(CECATSubHeader::IEEEFloat) << 0.0;
  QTest::newRow("uint8 scl_inter")   << 2   << 1.0 << -1.0 << 0.0 << 255.0 << static_cast<int>(CECATSubHeader::IEEEFloat) << 0.0;
  QTest::newRow("scl_slope 0")       << 4   << 0.0 << 7.0 << -100.0 << 100.0 << static_cast<int>(CECATSubHeader::SunShort) << 1.0;
}

void TestNIFTIMatrix::dataTypes()
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

  // stored values from lo to hi; the affine of libmedio: NIfTI voxel (i, j, nz-1-k) is ECAT7 voxel (i, j, k)
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
  QByteArray* data = NULL;
  CECATSubHeader* sh = NULL;
  QVERIFY(nifti.readMatrix(data, sh, 1));
  QScopedPointer<QByteArray> dataGuard(data);
  QScopedPointer<CECATSubHeader> shGuard(sh);
  const CECAT7SubHeaderImage* sub = static_cast<const CECAT7SubHeaderImage*>(sh);

  QCOMPARE(static_cast<int>(sub->data_Type()), ecatType);
  QCOMPARE(data->size(), static_cast<int>(voxels * ecatSize(ecatType)));
  const double expectedScale = (scale == 0.0) ? 1.0 : scale;
  QCOMPARE(static_cast<double>(sub->scale_Factor()), expectedScale);

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
// (PMOD, ecat2nifti) from the ECAT7 files, their ECAT7 representation has to
// match the ECAT7 file

void TestNIFTIMatrix::datasets_data()
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

void TestNIFTIMatrix::datasets()
{
  QFETCH(QString, ecat);
  QFETCH(QString, nifti);
  QFETCH(int, ecatType);
  QFETCH(bool, calibrated);
  QFETCH(bool, requantized);

  EcatFile reference;
  QVERIFY(reference.read(medioTestDataDir() + "/" + ecat));
  const double calibration = calibrated ? reference.mainHeader->calibration_Factor() : 1.0;

  CNIFTIFile file(medioTestDataDir() + "/" + nifti);
  QVERIFY(file.open(QIODevice::ReadOnly));

  double niftiScale = 0.0;
  for(int f=1; f <= reference.subHeaders.size(); f++) {
    const CECAT7SubHeaderImage* e = reference.subHeaders[f-1];
    const QByteArray& ecatData = reference.matrices[f-1];

    QByteArray* data = NULL;
    CECATSubHeader* sh = NULL;
    QVERIFY(file.readMatrix(data, sh, f));
    QScopedPointer<QByteArray> dataGuard(data);
    QScopedPointer<CECATSubHeader> shGuard(sh);
    const CECAT7SubHeaderImage* s = static_cast<const CECAT7SubHeaderImage*>(sh);

    QCOMPARE(static_cast<int>(s->data_Type()), ecatType);
    QCOMPARE(s->x_Dimension(), e->x_Dimension());
    QCOMPARE(s->y_Dimension(), e->y_Dimension());
    QCOMPARE(s->z_Dimension(), e->z_Dimension());
    QVERIFY(approx(s->x_Pixel_Size(), e->x_Pixel_Size(), 1e-5));
    QVERIFY(approx(s->y_Pixel_Size(), e->y_Pixel_Size(), 1e-5));
    QVERIFY(approx(s->z_Pixel_Size(), e->z_Pixel_Size(), 1e-5));

    if(f == 1)
      niftiScale = s->scale_Factor();
    else
      QCOMPARE(static_cast<double>(s->scale_Factor()), niftiScale);

    const qint64 voxels = static_cast<qint64>(e->x_Dimension()) * e->y_Dimension() * e->z_Dimension();
    QCOMPARE(static_cast<qint64>(data->size()), voxels * ecatSize(ecatType));

    if(ecatType == CECATSubHeader::SunShort && (requantized == false || f == 1)) {
      // same stored values and scale factor
      QCOMPARE(static_cast<double>(s->scale_Factor()), static_cast<double>(e->scale_Factor()));
      // (CECATFile::readMatrix() pads the matrix to ECAT7 blocks of 512 bytes)
      QVERIFY(*data == ecatData.left(data->size()));
    } else {
      // same physical values (one quantization step of requantized frames)
      const double tolerance = requantized ? niftiScale : 0.0;
      for(qint64 n=0; n < voxels; n++) {
        const double expected = ecatValue(ecatData.constData(), n, e->data_Type()) * e->scale_Factor() * calibration;
        const double value = ecatValue(data->constData(), n, ecatType) * s->scale_Factor();
        QVERIFY2(approx(value, expected, 1e-6, tolerance), qPrintable(QString("frame %1 voxel %2: %3 != %4").arg(f).arg(n).arg(value).arg(expected)));
      }
    }
  }
}

//---------------------------------------------------------------------------------------------
// the ECAT7 representation can be written with CECATFile: NIfTI file of
// PMOD -> ECAT7 file with the stored values of the original ECAT7 file

void TestNIFTIMatrix::niftiToEcatFile()
{
  CNIFTIFile nifti(medioTestDataDir() + "/nifti/pet-static.pair/pet-static.hdr");
  QVERIFY(nifti.open(QIODevice::ReadOnly));
  CNIFTIMainHeader* header = NULL;
  QVERIFY(nifti.readMainHeader(header));
  QScopedPointer<CNIFTIMainHeader> headerGuard(header);

  QByteArray* data = NULL;
  CECATSubHeader* sh = NULL;
  QVERIFY(nifti.readMatrix(data, sh, 1));
  QScopedPointer<QByteArray> dataGuard(data);
  QScopedPointer<CECATSubHeader> shGuard(sh);
  nifti.close();

  const QString path = tmpFile("pet-static-from-nifti.v");
  {
    CECATFile ecat(path, CECATMainHeader::ECAT7_Volume16);
    QVERIFY(ecat.open(QIODevice::WriteOnly));
    CECAT7MainHeader mainHeader(&ecat, CECATMainHeader::ECAT7_Volume16);
    QVERIFY(mainHeader.convertFrom(header, NULL));
    QVERIFY(ecat.writeMainHeader(mainHeader));
    QVERIFY(ecat.writeSubHeader(*sh, 1));
    QVERIFY(ecat.writeMatrix(*data, 1));
    ecat.close();
  }

  EcatFile written, reference;
  QVERIFY(written.read(path));
  QVERIFY(reference.read(medioTestDataDir() + "/ecat/pet-static.v"));
  QCOMPARE(written.subHeaders.size(), 1);
  QCOMPARE(static_cast<int>(written.subHeaders[0]->data_Type()), static_cast<int>(CECATSubHeader::SunShort));
  QCOMPARE(written.subHeaders[0]->scale_Factor(), reference.subHeaders[0]->scale_Factor());
  QCOMPARE(written.matrices[0].size(), reference.matrices[0].size());
  QVERIFY(written.matrices[0] == reference.matrices[0]);
}

//---------------------------------------------------------------------------------------------
// ECAT7 representation -> NIfTI file. The NIfTI header is converted from the
// ECAT7 headers, i.e. ECAT7 voxel (i, j, k) is NIfTI voxel (i, j, nz-1-k)
// (see CNIFTIConverter.h); the positions are checked as well.

void TestNIFTIMatrix::writeFromEcat_data()
{
  QTest::addColumn<QString>("ecat");
  QTest::addColumn<QString>("nifti");
  QTest::addColumn<int>("datatype");  // NIfTI datatype (int16: scl_slope of convertFrom())

  QTest::newRow("pet-static int16")     << "ecat/pet-static.v" << "static.nii" << 4;
  QTest::newRow("pet-static float32")   << "ecat/pet-static.v" << "static.hdr" << 16;
  QTest::newRow("pet-dynamic float32")  << "ecat/pet-dynamic.v" << "dynamic.nii.gz" << 16;
  QTest::newRow("pet-dynamic float64")  << "ecat/pet-dynamic.v" << "dynamic-nifti2.nii" << 64;
}

void TestNIFTIMatrix::writeFromEcat()
{
  QFETCH(QString, ecat);
  QFETCH(QString, nifti);
  QFETCH(int, datatype);

  EcatFile reference;
  QVERIFY(reference.read(medioTestDataDir() + "/" + ecat));
  const int frames = reference.subHeaders.size();
  const CECAT7SubHeaderImage* first = reference.subHeaders.first();
  const int nx = first->x_Dimension(), ny = first->y_Dimension(), nz = first->z_Dimension();
  const qint64 voxels = static_cast<qint64>(nx) * ny * nz;

  const QString path = tmpFile(nifti);
  const bool nifti2 = (datatype == 64);
  {
    CNIFTIFile file(path, nifti2 ? CNIFTIMainHeader::NIFTI2 : CNIFTIMainHeader::NIFTI1);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QScopedPointer<CNIFTIMainHeader> header(file.createEmptyHeader());
    QVERIFY(header->convertFrom(reference.mainHeader, first));
    if(datatype != 4) {
      if(nifti2) {
        static_cast<CNIFTI2MainHeader*>(header.data())->setDataType(datatype);
        static_cast<CNIFTI2MainHeader*>(header.data())->setBit_Pix(niftiSize(datatype) * 8);
        static_cast<CNIFTI2MainHeader*>(header.data())->setScl_Slope(1.0);
      } else {
        static_cast<CNIFTI1MainHeader*>(header.data())->setDataType(datatype);
        static_cast<CNIFTI1MainHeader*>(header.data())->setBit_Pix(niftiSize(datatype) * 8);
        static_cast<CNIFTI1MainHeader*>(header.data())->setScl_Slope(1.0);
      }
    }
    QVERIFY(file.writeMainHeader(*header));

    // frames in reversed order
    for(int f=frames; f >= 1; f--)
      QVERIFY(file.writeMatrix(reference.matrices[f-1], *reference.subHeaders[f-1], f, reference.mainHeader));
    file.close();
  }

  CNIFTIFile file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));
  CNIFTIMainHeader* h = NULL;
  QVERIFY(file.readMainHeader(h));
  QScopedPointer<CNIFTIMainHeader> hGuard(h);
  QByteArray* all = NULL;
  QVERIFY(file.readMatrix(all));
  QScopedPointer<QByteArray> allGuard(all);
  QCOMPARE(static_cast<qint64>(all->size()), voxels * frames * niftiSize(datatype));

  double slope = 1.0, srow[3][4];
  if(nifti2) {
    const CNIFTI2MainHeader* n = static_cast<const CNIFTI2MainHeader*>(h);
    slope = n->scl_Slope();
    for(short i=0; i < 4; i++) { srow[0][i] = n->srow_X(i); srow[1][i] = n->srow_Y(i); srow[2][i] = n->srow_Z(i); }
  } else {
    const CNIFTI1MainHeader* n = static_cast<const CNIFTI1MainHeader*>(h);
    slope = n->scl_Slope();
    for(short i=0; i < 4; i++) { srow[0][i] = n->srow_X(i); srow[1][i] = n->srow_Y(i); srow[2][i] = n->srow_Z(i); }
  }

  const double calibration = reference.mainHeader->calibration_Factor();

  // ECAT7 geometry following the documented convention (main header bed position)
  const double bed = reference.mainHeader->init_Bed_Position() * 10.0;

  for(int f=0; f < frames; f++) {
    const CECAT7SubHeaderImage* e = reference.subHeaders[f];
    for(int k=0; k < nz; k++)
      for(int j=0; j < ny; j++)
        for(int i=0; i < nx; i++) {
          const qint64 n = i + static_cast<qint64>(nx) * (j + static_cast<qint64>(ny) * (nz - 1 - k));
          const double stored = ecatValue(reference.matrices[f].constData(), i + static_cast<qint64>(nx) * (j + static_cast<qint64>(ny) * k), e->data_Type());
          const double value = niftiValue(all->constData(), n + voxels * f, datatype);

          if(datatype == 4)
            QCOMPARE(value, stored); // stored values and scale factor kept (calibration factor 1)
          else
            QVERIFY(approx(value * slope, stored * e->scale_Factor() * calibration, 1e-6)); // physical values

          if(f == 0 && (i == 0 || i == nx - 1) && (j == 0 || j == ny - 1)) {
            const double v[3] = { static_cast<double>(i), static_cast<double>(j), static_cast<double>(nz - 1 - k) };
            double pn[3];
            niftiPosition(srow, v, pn);
            const double pe[3] = { e->x_Offset() * 10.0 + (i - (nx - 1) / 2.0) * e->x_Pixel_Size() * 10.0,
                                   e->y_Offset() * 10.0 + (j - (ny - 1) / 2.0) * e->y_Pixel_Size() * 10.0,
                                   bed + e->z_Offset() * 10.0 - k * e->z_Pixel_Size() * 10.0 };
            for(int r=0; r < 3; r++)
              QVERIFY(approx(pn[r], pe[r], 0.0, 1e-3));
          }
        }
  }
}

// ECAT7 frames with different scale factors and calibration factor 1000
// written into an int16 NIfTI file with the scaling of convertFrom() (scale
// factor of frame 1 x calibration factor): frame 1 keeps its stored values,
// the other frames are requantized; values exceeding int16 are rejected
void TestNIFTIMatrix::writeRequantized()
{
  EcatFile reference;
  QVERIFY(reference.read(medioTestDataDir() + "/ecat/pet-dynamic.v"));
  QVERIFY(reference.subHeaders.size() == 3);
  const CECAT7SubHeaderImage* first = reference.subHeaders.first();
  const int nx = first->x_Dimension(), ny = first->y_Dimension(), nz = first->z_Dimension();
  const qint64 voxels = static_cast<qint64>(nx) * ny * nz;
  const double calibration = reference.mainHeader->calibration_Factor();
  QCOMPARE(calibration, 1000.0);

  for(int pass=0; pass < 2; pass++) {
    const QString path = tmpFile(QString("requantized-%1.nii").arg(pass));
    CNIFTIFile file(path, CNIFTIMainHeader::NIFTI1);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QScopedPointer<CNIFTIMainHeader> header(file.createEmptyHeader());
    QVERIFY(header->convertFrom(reference.mainHeader, first));
    CNIFTI1MainHeader* h = static_cast<CNIFTI1MainHeader*>(header.data());
    QVERIFY(approx(h->scl_Slope(), first->scale_Factor() * calibration, 1e-6));
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
            QVERIFY(std::fabs(value * slope - stored * reference.subHeaders[f]->scale_Factor() * calibration) <= slope * (0.5 + 1e-6));
        }
  }
}

//---------------------------------------------------------------------------------------------

void TestNIFTIMatrix::errors()
{
  Synthetic s;
  const qint64 voxels = s.voxels();
  QByteArray values(voxels * 2, '\0');

  // the main header has to be written first
  {
    CNIFTIFile file(tmpFile("noheader.nii"), CNIFTIMainHeader::NIFTI1);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.writeMatrix(values, CMedIOData::ECATOrientation, 1) == false);
    file.close();
  }

  CNIFTIFile file(tmpFile("errors.nii"), CNIFTIMainHeader::NIFTI1);
  QVERIFY(createNifti(file, s));

  // wrong size and frame
  QVERIFY(file.writeMatrix(values.left(values.size() - 2), CMedIOData::ECATOrientation, 1) == false);
  QVERIFY(file.writeMatrix(values, CMedIOData::NativeOrientation, 2) == false);
  QVERIFY(file.writeMatrix(values, CMedIOData::ECATOrientation, 0) == false);
  QVERIFY(file.writeMatrix(values, static_cast<CMedIOData::Orientation>(42), 1) == false);
  QVERIFY(file.writeMatrix(NULL, 0, CMedIOData::ECATOrientation, 1) == false);
  QVERIFY(file.writeMatrix(values, CMedIOData::ECATOrientation, 1));

  // ECAT7 sub header not matching the main header
  CECAT7SubHeaderImage sub;
  sub.setData_Type(CECATSubHeader::SunShort);
  sub.setScale_Factor(1.0f);
  sub.setX_Dimension(s.nx); sub.setY_Dimension(s.ny); sub.setZ_Dimension(s.nz);
  QVERIFY(file.writeMatrix(values, sub, 1));
  sub.setZ_Dimension(s.nz + 1);
  QVERIFY(file.writeMatrix(values, sub, 1) == false);
  sub.setZ_Dimension(s.nz);
  QVERIFY(file.writeMatrix(values.left(values.size() - 2), sub, 1) == false);
  QVERIFY(file.writeMatrix(values + QByteArray(512 - values.size(), '\0'), sub, 1)); // padded to 512 bytes
  QVERIFY(file.writeMatrix(values + QByteArray(512 - values.size() + 2, '\0'), sub, 1) == false);
  sub.setData_Type(CECATSubHeader::IEEEFloat);
  QVERIFY(file.writeMatrix(values, sub, 1) == false); // size of float data
  QByteArray floats(voxels * 4, '\0');
  reinterpret_cast<float*>(floats.data())[3] = 40000.0f;
  QVERIFY(file.writeMatrix(floats, sub, 1) == false); // exceeds int16
  reinterpret_cast<float*>(floats.data())[3] = std::numeric_limits<float>::quiet_NaN();
  QVERIFY(file.writeMatrix(floats, sub, 1) == false); // NaN in int16
  reinterpret_cast<float*>(floats.data())[3] = 2.6f;
  QVERIFY(file.writeMatrix(floats, sub, 1));          // rounded to 3
  sub.setData_Type(CECATSubHeader::UnknownDataType);
  QVERIFY(file.writeMatrix(values, sub, 1) == false);
  CECAT7SubHeaderScan scan;
  QVERIFY(file.writeMatrix(values, scan, 1) == false);
  file.close();

  // the rejected writes did not change the data
  const QByteArray written = readAll(tmpFile("errors.nii"));
  QCOMPARE(written.size(), values.size());
  for(qint64 n=0; n < voxels; n++) {
    // ECAT7 voxel 3 = (3, 0, 0) is NIfTI voxel (3, 0, nz-1)
    const double expected = (n == 3 + s.nx * s.ny * (s.nz - 1)) ? 3.0 : 0.0;
    QCOMPARE(niftiValue(written.constData(), n, 4), expected);
  }

  // complex data: voxel order only, no ECAT7 representation
  {
    Synthetic c;
    c.datatype = 32; // DT_COMPLEX64
    QByteArray complex(voxels * 8, '\0');
    for(qint64 n=0; n < voxels; n++)
      reinterpret_cast<float*>(complex.data())[2 * n] = n;
    QVERIFY(writeNifti(tmpFile("complex.nii"), c, complex));

    CNIFTIFile in(tmpFile("complex.nii"));
    QVERIFY(in.open(QIODevice::ReadOnly));
    QByteArray* data = NULL;
    QVERIFY(in.readMatrix(data, CMedIOData::ECATOrientation, 1));
    QCOMPARE(data->size(), static_cast<int>(voxels * 8));
    // ECAT7 voxel (0, 0, 0) is NIfTI voxel (0, 0, nz-1)
    QCOMPARE(static_cast<double>(reinterpret_cast<const float*>(data->constData())[0]), static_cast<double>(s.nx * s.ny * (s.nz - 1)));
    delete data;

    CECATSubHeader* sh = NULL;
    QVERIFY(in.readMatrix(data, sh, 1) == false);
    QVERIFY(data == NULL && sh == NULL);
  }

  // 5D data: native voxel order only
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
    QVERIFY(in.readMatrix(data, CMedIOData::NativeOrientation, 6));
    QCOMPARE(*data, data5.right(voxels * 2));
    delete data;
    QVERIFY(in.readMatrix(data, CMedIOData::NativeOrientation, 7) == false);
    QVERIFY(in.readMatrix(data, CMedIOData::ECATOrientation, 1) == false);
  }

  // truncated file
  {
    QVERIFY(writeNifti(tmpFile("truncated.nii"), s, values));
    QFile f(tmpFile("truncated.nii"));
    QVERIFY(f.resize(f.size() - 1));

    CNIFTIFile in(tmpFile("truncated.nii"));
    QVERIFY(in.open(QIODevice::ReadOnly));
    QByteArray* data = NULL;
    QVERIFY(in.readMatrix(data, CMedIOData::ECATOrientation, 1) == false);
    QVERIFY(data == NULL);
  }
}

QTEST_GUILESS_MAIN(TestNIFTIMatrix)
#include "testNIFTIMatrix.moc"
