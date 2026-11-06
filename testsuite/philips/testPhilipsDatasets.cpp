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

// Validates the Philips image and Syntegra files of the test datasets
// (testsuite/data/philips) against their expected values <file>.json (see
// testsuite/data/README.md): file type, frames, dimensions, voxel size and
// the stored voxel values per frame. If the dataset also holds an ECAT7 file,
// the scale factors and the frame timing are compared with the expected
// values of that file (the Philips and ECAT7 files of the datasets hold the
// same stored values).

#include <CPhilipsFile.h>
#include <CPhilipsMainHeader.h>
#include <CPhilipsSubHeader.h>
#include <CPhilipsSubHeaderImage.h>
#include <CMedIODataFactory.h>

#include "MedIOTestData.h"

#include <QtTest/QtTest>
#include <QJsonArray>

#include <cmath>

class TestPhilipsDatasets : public QObject
{
  Q_OBJECT

  private:
    MedIOTestData m_data;

  private slots:
    void initTestCase();

    void philipsFiles_data();
    void philipsFiles();
};

void TestPhilipsDatasets::initTestCase()
{
  m_data = medioTestData();
  foreach(const QString& pending, m_data.pending)
    qInfo("pending test data file: %s", qPrintable(pending));
}

void TestPhilipsDatasets::philipsFiles_data()
{
  QTest::addColumn<int>("index");

  bool any = false;
  for(int i=0; i < m_data.files.size(); i++) {
    if(m_data.files[i].format == "philips") {
      QTest::newRow(qPrintable(m_data.files[i].relativePath)) << i;
      any = true;
    }
  }

  if(any == false)
    QTest::newRow("no Philips test data") << -1;
}

void TestPhilipsDatasets::philipsFiles()
{
  QFETCH(int, index);
  if(index < 0)
    QSKIP("no Philips files in the test data");

  const MedIOTestFile& file = m_data.files[index];
  const QJsonObject expected = file.expected;
  const QString format = expected.value("format").toString();

  QVERIFY(CPhilipsFile::isOfType(file.path));
  QCOMPARE(CMedIODataFactory::identify(file.path), CMedIOData::Philips);

  CPhilipsFile philipsFile(file.path);
  QVERIFY(philipsFile.open(QIODevice::ReadOnly));

  CPhilipsMainHeader* mainHeader = NULL;
  QVERIFY(philipsFile.readMainHeader(mainHeader));
  QScopedPointer<CPhilipsMainHeader> guard(mainHeader);

  // file type
  if(format == "philips-image")
    QCOMPARE(int(mainHeader->filtyp()), int(CPhilipsMainHeader::Image));
  else if(format == "philips-syntegra")
    QCOMPARE(int(mainHeader->filtyp()), int(CPhilipsMainHeader::Syntegra));
  else
    QFAIL(qPrintable("unknown format " + format));

  // frames and dimensions
  const QJsonArray dims = expected.value("dims").toArray();
  const QJsonArray pixelSize = expected.value("pixel_size_mm").toArray();
  const QJsonArray volumes = expected.value("volumes").toArray();
  const int frames = expected.value("frames").toInt();
  QCOMPARE(dims.size(), 3);
  QCOMPARE(int(philipsFile.numFrames()), frames);
  QCOMPARE(volumes.size(), frames);
  QVERIFY(std::fabs(mainHeader->Dslice_thick() - pixelSize[2].toDouble()) < 1e-4);

  // expected values of the ECAT7 file of the same dataset
  QJsonArray ecatMatrices;
  double ecatCalibration = 1.0;
  foreach(const MedIOTestFile& other, m_data.files) {
    if(other.format == "ecat" && other.dataset == file.dataset) {
      ecatMatrices = other.expected.value("matrices").toArray();
      ecatCalibration = other.expected.value("main_header").toObject().value("calibration_factor").toDouble(1.0);
    }
  }

  const int slices = dims[2].toInt();
  const int minSlice = philipsFile.minSlice();
  const int increment = (slices > 1) ? (philipsFile.maxSlice() - minSlice) / (slices - 1) : 1;

  for(int f=0; f < frames; f++) {
    const int frame = philipsFile.minFrame() + f;
    QCOMPARE(int(philipsFile.numSlices(frame)), slices);

    double minValue = 0.0, maxValue = 0.0, sum = 0.0;
    qint64 voxels = 0;
    QSet<float> scales;

    for(int k=0; k < slices; k++) {
      const int slice = minSlice + k * increment;
      QByteArray* data = NULL;
      CPhilipsSubHeader* subHeader = NULL;
      QVERIFY2(philipsFile.readMatrix(data, subHeader, frame, slice), qPrintable(QString("frame %1 slice %2").arg(frame).arg(slice)));
      QScopedPointer<QByteArray> dataGuard(data);
      QScopedPointer<CPhilipsSubHeader> subHeaderGuard(subHeader);

      QVERIFY(subHeader->subHeaderType() == CPhilipsSubHeader::Image || subHeader->subHeaderType() == CPhilipsSubHeader::Syntegra);
      const CPhilipsSubHeaderImage* s = static_cast<const CPhilipsSubHeaderImage*>(subHeader);

      QCOMPARE(int(s->xdim()), dims[0].toInt());
      QCOMPARE(int(s->ydim()), dims[1].toInt());
      QVERIFY(std::fabs(s->pix_spacing_x() - pixelSize[0].toDouble()) < 1e-4);
      QVERIFY(std::fabs(s->pix_spacing_y() - pixelSize[1].toDouble()) < 1e-4);
      QCOMPARE(expected.value("data_type").toString(), QString("int16"));
      QCOMPARE(int(s->datype()), int(CPhilipsSubHeader::SignedShort));

      const qint64 n = static_cast<qint64>(s->xdim()) * s->ydim();
      QVERIFY(data->size() >= n * 2);
      const qint16* values = reinterpret_cast<const qint16*>(data->constData());
      for(qint64 i=0; i < n; i++) {
        minValue = (voxels == 0) ? values[i] : qMin(minValue, double(values[i]));
        maxValue = (voxels == 0) ? values[i] : qMax(maxValue, double(values[i]));
        sum += values[i];
        voxels++;
      }

      // scale factor and frame timing (seconds and milliseconds)
      bool ok = false;
      const double scale = s->ecat_Scale_Factor(ok, mainHeader);
      QVERIFY(ok);
      scales.insert(float(scale));

      if(ecatMatrices.size() == frames) {
        const QJsonObject m = ecatMatrices[f].toObject();
        const double ecatScale = m.value("scale_factor").toDouble() * ecatCalibration;
        QVERIFY2(std::fabs(scale - ecatScale) <= 1e-5 * ecatScale,
                 qPrintable(QString("frame %1 slice %2: scale factor %3 instead of %4").arg(frame).arg(slice).arg(scale, 0, 'g', 8).arg(ecatScale, 0, 'g', 8)));

        const qint64 start = (static_cast<qint64>(s->start_date_time()) - mainHeader->acq_date_time()) * 1000 + s->start_date_time_msec();
        const qint64 duration = (static_cast<qint64>(s->end_date_time()) - s->start_date_time()) * 1000 +
                                s->end_date_time_msec() - s->start_date_time_msec();
        QCOMPARE(start, qint64(m.value("frame_start_time_ms").toInt()));
        QCOMPARE(duration, qint64(m.value("frame_duration_ms").toInt()));
      }
    }

    QCOMPARE(scales.size(), 1); // one scale factor per frame in these files

    const QJsonObject e = volumes[f].toObject();
    QCOMPARE(voxels, qint64(dims[0].toInt()) * dims[1].toInt() * slices);
    QCOMPARE(minValue, e.value("raw_min").toDouble());
    QCOMPARE(maxValue, e.value("raw_max").toDouble());
    QCOMPARE(sum, e.value("raw_sum").toDouble());
  }

  philipsFile.close();
}

QTEST_GUILESS_MAIN(TestPhilipsDatasets)
#include "testPhilipsDatasets.moc"
