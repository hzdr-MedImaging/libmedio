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

// Conversion of an ECAT7 image whose NIfTI voxel data is larger than a
// QByteArray can hold (2 GB) and back: CNIFTIConverter has to write and read
// the NIfTI voxel data frame by frame.
//
// The test needs about 3.3 GB of temporary disk space and 1.5 GB of memory,
// it is registered with the CTest label "large" (ctest -LE large skips it).

#include <CNIFTIConverter.h>
#include <CNIFTIFile.h>
#include <CNIFTIMainHeader.h>
#include <CNIFTI1MainHeader.h>
#include <CECATFile.h>
#include <CECATMainHeader.h>
#include <CECATSubHeader.h>
#include <CECAT7MainHeader.h>
#include <CECAT7SubHeaderImage.h>

#include <QtTest/QtTest>
#include <QTemporaryDir>

#include <cmath>
#include <limits>

// 8 frames of 256 x 256 x 1025 voxels: 16 bit integers in the ECAT7 file
// (1.07 GB), float values in the NIfTI file (2.15 GB)
static const int NX = 256, NY = 256, NZ = 1025, FRAMES = 8;

// stored value of voxel index of frame f (0-based)
static inline qint16 storedValue(qint64 index, int f)
{
  return static_cast<qint16>((index + 1000 * f) % 30000 - 15000);
}

// scale factor of frame f: different per frame, so the converter writes
// float values; stored value x scale is exact in float
static inline double frameScale(int f)
{
  return 0.25 * (f + 1);
}

class TestNIFTILargeImage : public QObject
{
  Q_OBJECT

  private:
    QTemporaryDir m_tmp;

    QString tmpFile(const QString& name) const { return m_tmp.filePath(name); }

  private slots:
    void initTestCase();
    void ecatToNiftiLargerThan2GB();
    void niftiToEcatLargerThan2GB();   // uses the NIfTI file of ecatToNiftiLargerThan2GB()
};

void TestNIFTILargeImage::initTestCase()
{
  QVERIFY(m_tmp.isValid());
}

void TestNIFTILargeImage::ecatToNiftiLargerThan2GB()
{
  const qint64 voxels = static_cast<qint64>(NX) * NY * NZ;
  const qint64 planeVoxels = static_cast<qint64>(NX) * NY;
  QVERIFY(voxels * FRAMES * static_cast<qint64>(sizeof(float)) > std::numeric_limits<int>::max());

  // synthetic ECAT7 file
  const QString ecat = tmpFile("large.v");
  {
    CECATFile file(ecat, CECATMainHeader::ECAT7_Volume16);
    QVERIFY(file.open(QIODevice::WriteOnly));

    CECAT7MainHeader mainHeader(&file, CECATMainHeader::ECAT7_Volume16);
    mainHeader.setNum_Frames(FRAMES);
    mainHeader.setNum_Planes(NZ);
    mainHeader.setCalibration_Factor(1.0f);
    QVERIFY(file.writeMainHeader(mainHeader));

    QByteArray matrix(static_cast<int>(voxels * sizeof(qint16)), '\0');
    for(int f=0; f < FRAMES; f++) {
      CECAT7SubHeaderImage subHeader(&file);
      subHeader.setData_Type(CECATSubHeader::SunShort);
      subHeader.setNum_Dimensions(3);
      subHeader.setX_Dimension(NX);
      subHeader.setY_Dimension(NY);
      subHeader.setZ_Dimension(NZ);
      subHeader.setX_Pixel_Size(0.2f);
      subHeader.setY_Pixel_Size(0.2f);
      subHeader.setZ_Pixel_Size(0.2f);
      subHeader.setScale_Factor(static_cast<float>(frameScale(f)));
      subHeader.setFrame_Duration(60000u);

      qint16* values = reinterpret_cast<qint16*>(matrix.data());
      for(qint64 i=0; i < voxels; i++)
        values[i] = storedValue(i, f);

      QVERIFY(file.writeSubHeader(subHeader, f + 1));
      QVERIFY(file.writeMatrix(matrix, f + 1));
    }

    file.close();
  }

  // conversion: the voxel data of all frames is larger than 2 GB
  const QString nifti = tmpFile("large.nii");
  QString error;
  QVERIFY2(CNIFTIConverter::ecatToNifti(ecat, nifti, CNIFTIConverter::Options(), &error), qPrintable(error));
  QVERIFY(QFile::remove(ecat));

  CNIFTIFile file(nifti);
  QVERIFY(file.open(QIODevice::ReadOnly));

  CNIFTIMainHeader* headerPtr = NULL;
  QVERIFY(file.readMainHeader(headerPtr) && headerPtr != NULL);
  QScopedPointer<CNIFTIMainHeader> header(headerPtr);
  QCOMPARE(static_cast<int>(header->mainHeaderType()), static_cast<int>(CNIFTIMainHeader::NIFTI1MainHeader));

  const CNIFTI1MainHeader* h = static_cast<const CNIFTI1MainHeader*>(headerPtr);
  QCOMPARE(static_cast<int>(h->dim(0)), 4);
  QCOMPARE(static_cast<int>(h->dim(1)), NX);
  QCOMPARE(static_cast<int>(h->dim(2)), NY);
  QCOMPARE(static_cast<int>(h->dim(3)), NZ);
  QCOMPARE(static_cast<int>(h->dim(4)), FRAMES);
  QCOMPARE(static_cast<int>(h->dataType()), 16);  // DT_FLOAT32
  QCOMPARE(static_cast<double>(h->cal_Min()), -15000.0 * frameScale(FRAMES - 1));
  QCOMPARE(static_cast<double>(h->cal_Max()), 14999.0 * frameScale(FRAMES - 1));
  QCOMPARE(QFileInfo(nifti).size(), static_cast<qint64>(h->vox_Offset()) + voxels * FRAMES * static_cast<qint64>(sizeof(float)));

  // NIfTI voxel (i, j, nz-1-k) is ECAT7 voxel (i, j, k)
  for(int f=0; f < FRAMES; f++) {
    QByteArray* matrixPtr = NULL;
    QVERIFY(file.readMatrix(matrixPtr, static_cast<short>(f + 1)) && matrixPtr != NULL);
    QScopedPointer<QByteArray> matrix(matrixPtr);
    QCOMPARE(static_cast<qint64>(matrix->size()), voxels * static_cast<qint64>(sizeof(float)));

    const float* values = reinterpret_cast<const float*>(matrix->constData());
    for(int k=0; k < NZ; k++) {
      const qint64 ecatPlane = k * planeVoxels;
      const qint64 niftiPlane = (NZ - 1 - k) * planeVoxels;

      for(qint64 i=0; i < planeVoxels; i++) {
        const float expected = static_cast<float>(storedValue(ecatPlane + i, f) * frameScale(f));
        if(values[niftiPlane + i] != expected)
          QFAIL(qPrintable(QString("frame %1, plane %2, voxel %3: %4 instead of %5").arg(f + 1).arg(k).arg(i)
                                                                                     .arg(values[niftiPlane + i]).arg(expected)));
      }
    }
  }

  file.close();
}

void TestNIFTILargeImage::niftiToEcatLargerThan2GB()
{
  const QString nifti = tmpFile("large.nii");
  QVERIFY2(QFile::exists(nifti), "the NIfTI file of ecatToNiftiLargerThan2GB() is missing");

  const qint64 voxels = static_cast<qint64>(NX) * NY * NZ;
  const qint64 planeVoxels = static_cast<qint64>(NX) * NY;

  // conversion: the voxel data of the NIfTI file is larger than 2 GB
  const QString ecat = tmpFile("back.v");
  QString error;
  QVERIFY2(CNIFTIConverter::niftiToEcat(nifti, ecat, &error), qPrintable(error));
  QVERIFY(QFile::remove(nifti));

  CECATFile file(ecat);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QCOMPARE(file.frameNumbers().size(), FRAMES);

  // 16 bit integers with a scale factor per frame: the largest absolute value
  // of frame f is 15000 x frameScale(f), stored as 32767
  for(int f=0; f < FRAMES; f++) {
    QByteArray* matrixPtr = NULL;
    CECATSubHeader* subHeaderPtr = NULL;
    QVERIFY(file.readMatrix(matrixPtr, subHeaderPtr, static_cast<short>(f + 1)) && matrixPtr != NULL && subHeaderPtr != NULL);
    QScopedPointer<QByteArray> matrix(matrixPtr);
    QScopedPointer<CECATSubHeader> subHeaderGuard(subHeaderPtr);
    QCOMPARE(static_cast<int>(subHeaderPtr->subHeaderType()), static_cast<int>(CECATSubHeader::ECAT7_Image));

    const CECAT7SubHeaderImage* s = static_cast<const CECAT7SubHeaderImage*>(subHeaderPtr);
    QCOMPARE(static_cast<int>(s->x_Dimension()), NX);
    QCOMPARE(static_cast<int>(s->y_Dimension()), NY);
    QCOMPARE(static_cast<int>(s->z_Dimension()), NZ);
    QCOMPARE(static_cast<int>(s->data_Type()), static_cast<int>(CECATSubHeader::SunShort));
    QVERIFY(static_cast<qint64>(matrix->size()) >= voxels * static_cast<qint64>(sizeof(qint16)));

    const double scale = s->scale_Factor();
    const double expectedScale = 15000.0 * frameScale(f) / 32767.0;
    QVERIFY2(std::fabs(scale - expectedScale) <= 1e-6 * expectedScale, qPrintable(QString("frame %1: scale factor %2").arg(f + 1).arg(scale)));

    // the NIfTI planes are reversed again: ECAT7 voxel order of the original;
    // rounding to 16 bit integers (half a step, plus the float precision of
    // the stored scale factor)
    const qint16* values = reinterpret_cast<const qint16*>(matrix->constData());
    for(qint64 i=0; i < voxels; i++) {
      const double expected = storedValue(i, f) * frameScale(f);
      if(std::fabs(values[i] * scale - expected) > 0.501 * scale)
        QFAIL(qPrintable(QString("frame %1, voxel %2 (plane %3): %4 instead of %5").arg(f + 1).arg(i).arg(i / planeVoxels)
                                                                                    .arg(values[i] * scale).arg(expected)));
    }
  }

  file.close();
}

QTEST_GUILESS_MAIN(TestNIFTILargeImage)
#include "testNIFTILargeImage.moc"
