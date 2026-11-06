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

#include "MedIOTestData.h"

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QJsonArray>

#include <cmath>

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
    void writeReorientedVolume();
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
