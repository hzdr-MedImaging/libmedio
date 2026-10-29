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

// Validates the ECAT7 reading of libmedio against the datasets in
// testsuite/data/ecat7 (see testsuite/data/README.md). The expected values
// of each dataset are generated with an independent implementation
// (testsuite/data/tools/make_expected.py), only the keys present in the
// "expected" block are checked.

#include <QtTest/QtTest>
#include <QJsonArray>
#include <QScopedPointer>

#include <cmath>

#include "CECATFile.h"
#include "CECATMainHeader.h"
#include "CECATSubHeader.h"
#include "CECAT7MainHeader.h"
#include "CECAT7SubHeaderImage.h"

#include "MedIOTestData.h"

Q_DECLARE_METATYPE(MedIOTestDataset)

// compare a number with an expected JSON value using a relative tolerance
#define COMPARE_NUMBER(actual, expectedValue, tolerance)                                     \
  do {                                                                                       \
    double a_ = (actual);                                                                    \
    double e_ = (expectedValue).toDouble();                                                  \
    QVERIFY2(std::fabs(a_ - e_) <= (tolerance) * qMax(1.0, std::fabs(e_)),                   \
             qPrintable(QString("%1: actual %2, expected %3").arg(#actual)                   \
                                                             .arg(a_, 0, 'g', 17)            \
                                                             .arg(e_, 0, 'g', 17)));         \
  } while(0)

// relative tolerance for values stored as float32
#define FLOAT_TOLERANCE 1e-6

class TestECAT7Datasets : public QObject
{
  Q_OBJECT

  private slots:
    void testDatasetLayout();

    void testMainHeader_data();
    void testMainHeader();

    void testMatrix_data();
    void testMatrix();
};

//---------------------------------------------------------------------------------------------
// all dataset directories have to be complete
void TestECAT7Datasets::testDatasetLayout()
{
  QStringList errors;
  medioTestDatasets("ecat7", &errors);

  QVERIFY2(errors.isEmpty(), qPrintable(errors.join("\n")));
}

//---------------------------------------------------------------------------------------------
void TestECAT7Datasets::testMainHeader_data()
{
  QTest::addColumn<MedIOTestDataset>("dataset");

  QList<MedIOTestDataset> datasets = medioTestDatasets("ecat7");

  foreach(const MedIOTestDataset& dataset, datasets)
    QTest::newRow(dataset.name.toLatin1().constData()) << dataset;

  if(datasets.isEmpty())
    QTest::newRow("no datasets") << MedIOTestDataset();
}

void TestECAT7Datasets::testMainHeader()
{
  QFETCH(MedIOTestDataset, dataset);

  if(dataset.name.isEmpty())
    QSKIP(qPrintable("no ECAT7 datasets found in " + medioTestDataDir()));

  QJsonObject expected = dataset.json.value("expected").toObject().value("main_header").toObject();

  CECATFile file(dataset.dataFile());
  QVERIFY(file.open(QIODevice::ReadOnly));

  CECATMainHeader* mainHeader = NULL;
  QVERIFY(file.readMainHeader(mainHeader));
  QScopedPointer<CECATMainHeader> mainHeaderGuard(mainHeader);
  QCOMPARE(mainHeader->mainHeaderType(), CECATMainHeader::ECAT7MainHeader);

  CECAT7MainHeader* header = static_cast<CECAT7MainHeader*>(mainHeader);

  if(expected.contains("file_type"))
    QCOMPARE(static_cast<int>(header->file_Type()), expected.value("file_type").toInt());
  if(expected.contains("num_frames"))
    QCOMPARE(static_cast<int>(header->num_Frames()), expected.value("num_frames").toInt());
  if(expected.contains("num_planes"))
    QCOMPARE(static_cast<int>(header->num_Planes()), expected.value("num_planes").toInt());
  if(expected.contains("calibration_factor"))
    COMPARE_NUMBER(header->calibration_Factor(), expected.value("calibration_factor"), FLOAT_TOLERANCE);
  if(expected.contains("data_units"))
    QCOMPARE(QString::fromLatin1(header->data_Units()).trimmed(), expected.value("data_units").toString());

  file.close();
}

//---------------------------------------------------------------------------------------------
void TestECAT7Datasets::testMatrix_data()
{
  QTest::addColumn<MedIOTestDataset>("dataset");
  QTest::addColumn<QJsonObject>("matrix");

  QList<MedIOTestDataset> datasets = medioTestDatasets("ecat7");

  foreach(const MedIOTestDataset& dataset, datasets)
  {
    foreach(const QJsonValue& value, dataset.json.value("expected").toObject().value("matrices").toArray())
    {
      QJsonObject matrix = value.toObject();
      QString name = QString("%1 f%2 p%3 g%4 b%5 d%6").arg(dataset.name)
                                                      .arg(matrix.value("frame").toInt())
                                                      .arg(matrix.value("plane").toInt())
                                                      .arg(matrix.value("gate").toInt())
                                                      .arg(matrix.value("bed").toInt())
                                                      .arg(matrix.value("data").toInt());
      QTest::newRow(name.toLatin1().constData()) << dataset << matrix;
    }
  }

  if(datasets.isEmpty())
    QTest::newRow("no datasets") << MedIOTestDataset() << QJsonObject();
}

void TestECAT7Datasets::testMatrix()
{
  QFETCH(MedIOTestDataset, dataset);
  QFETCH(QJsonObject, matrix);

  if(dataset.name.isEmpty())
    QSKIP(qPrintable("no ECAT7 datasets found in " + medioTestDataDir()));

  short frame = matrix.value("frame").toInt();
  short plane = matrix.value("plane").toInt();
  short gate  = matrix.value("gate").toInt();
  short bed   = matrix.value("bed").toInt();
  short data  = matrix.value("data").toInt();

  CECATFile file(dataset.dataFile());
  QVERIFY(file.open(QIODevice::ReadOnly));

  // sub header
  CECATSubHeader* subHeader = NULL;
  QVERIFY(file.readSubHeader(subHeader, frame, plane, gate, bed, data));
  QScopedPointer<CECATSubHeader> subHeaderGuard(subHeader);
  QCOMPARE(subHeader->subHeaderType(), CECATSubHeader::ECAT7_Image);

  CECAT7SubHeaderImage* header = static_cast<CECAT7SubHeaderImage*>(subHeader);

  if(matrix.contains("data_type"))
    QCOMPARE(static_cast<int>(header->data_Type()), matrix.value("data_type").toInt());

  if(matrix.contains("dims")) {
    QJsonArray dims = matrix.value("dims").toArray();
    QCOMPARE(static_cast<int>(header->x_Dimension()), dims.at(0).toInt());
    QCOMPARE(static_cast<int>(header->y_Dimension()), dims.at(1).toInt());
    QCOMPARE(static_cast<int>(header->z_Dimension()), dims.at(2).toInt());
  }

  if(matrix.contains("pixel_size_cm")) {
    QJsonArray size = matrix.value("pixel_size_cm").toArray();
    COMPARE_NUMBER(header->x_Pixel_Size(), size.at(0), FLOAT_TOLERANCE);
    COMPARE_NUMBER(header->y_Pixel_Size(), size.at(1), FLOAT_TOLERANCE);
    COMPARE_NUMBER(header->z_Pixel_Size(), size.at(2), FLOAT_TOLERANCE);
  }

  if(matrix.contains("scale_factor"))
    COMPARE_NUMBER(header->scale_Factor(), matrix.value("scale_factor"), FLOAT_TOLERANCE);
  if(matrix.contains("frame_start_time_ms"))
    QCOMPARE(static_cast<qint64>(header->frame_Start_Time()), static_cast<qint64>(matrix.value("frame_start_time_ms").toDouble()));
  if(matrix.contains("frame_duration_ms"))
    QCOMPARE(static_cast<qint64>(header->frame_Duration()), static_cast<qint64>(matrix.value("frame_duration_ms").toDouble()));

  // voxel values as stored (machine byte order, unscaled)
  QByteArray* matrixData = NULL;
  QVERIFY(file.readMatrix(matrixData, frame, plane, gate, bed, data));
  QScopedPointer<QByteArray> matrixGuard(matrixData);

  qint64 voxels = static_cast<qint64>(header->x_Dimension()) * header->y_Dimension() * header->z_Dimension();
  double minValue = 0.0, maxValue = 0.0, sum = 0.0;
  int bytesPerVoxel = 0;

  switch(header->data_Type()) {
    case CECATSubHeader::ByteData:  bytesPerVoxel = 1; break;
    case CECATSubHeader::VAX_Ix2:
    case CECATSubHeader::SunShort:  bytesPerVoxel = 2; break;
    case CECATSubHeader::VAX_Ix4:
    case CECATSubHeader::SunLong:
    case CECATSubHeader::VAX_Rx4:
    case CECATSubHeader::IEEEFloat: bytesPerVoxel = 4; break;
    default:
      QFAIL(qPrintable(QString("unsupported data type %1").arg(header->data_Type())));
  }

  // the matrix may contain padding up to the next 512 byte block
  QVERIFY(matrixData->size() >= voxels * bytesPerVoxel);

  for(qint64 i=0; i < voxels; i++) {
    double value = 0.0;
    const char* p = matrixData->constData() + i * bytesPerVoxel;

    switch(header->data_Type()) {
      case CECATSubHeader::ByteData:  value = *reinterpret_cast<const quint8*>(p); break;
      case CECATSubHeader::VAX_Ix2:
      case CECATSubHeader::SunShort:  value = *reinterpret_cast<const qint16*>(p); break;
      case CECATSubHeader::VAX_Ix4:
      case CECATSubHeader::SunLong:   value = *reinterpret_cast<const qint32*>(p); break;
      default:                        value = *reinterpret_cast<const float*>(p); break;
    }

    if(i == 0 || value < minValue) minValue = value;
    if(i == 0 || value > maxValue) maxValue = value;
    sum += value;
  }

  // integer voxel values have to match exactly
  bool floatData = (header->data_Type() == CECATSubHeader::VAX_Rx4 ||
                    header->data_Type() == CECATSubHeader::IEEEFloat);
  double tolerance = floatData ? FLOAT_TOLERANCE : 0.0;

  if(matrix.contains("raw_min"))
    COMPARE_NUMBER(minValue, matrix.value("raw_min"), tolerance);
  if(matrix.contains("raw_max"))
    COMPARE_NUMBER(maxValue, matrix.value("raw_max"), tolerance);
  if(matrix.contains("raw_sum"))
    COMPARE_NUMBER(sum, matrix.value("raw_sum"), tolerance);

  file.close();
}

QTEST_GUILESS_MAIN(TestECAT7Datasets)
#include "testECAT7Datasets.moc"
