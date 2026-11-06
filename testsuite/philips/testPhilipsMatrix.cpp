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

// Tests the frame-wise access to Philips image and Syntegra files in the
// ECAT7 representation (CPhilipsFile::readMatrix(data, CECATSubHeader*&, frame)):
//
// - the files of the test datasets (testsuite/data/philips) against the
//   expected values of the Philips files and of the ECAT7 file of the same
//   dataset (nibabel), whose stored values are identical: dimensions, voxel
//   sizes, data type, scale factor, frame timing and value statistics. The
//   voxels are additionally compared one by one with the ECAT7 file.
// - synthetic Philips files written into a temporary directory with known
//   values: data types, slices with different scale factors (physical
//   float32 values), frame numbering and invalid frames.

#include <CPhilipsFile.h>
#include <CPhilipsMainHeader.h>
#include <CPhilipsSubHeader.h>
#include <CPhilipsSubHeaderImage.h>
#include <CECATFile.h>
#include <CECATSubHeader.h>
#include <CECAT7SubHeaderImage.h>

#include "MedIOTestData.h"

#include <QtTest/QtTest>
#include <QJsonArray>
#include <QTemporaryDir>

#include <cmath>
#include <cstring>

namespace {

// a synthetic slice: stored values and scale factor
struct SyntheticSlice
{
  QVector<double> values;  // stored values
  float scale;             // imgscl (0: no valid scale factor)
};

// a synthetic frame
struct SyntheticFrame
{
  QVector<SyntheticSlice> slices;
  int start;               // start time relative to the acquisition (s)
  int duration;            // duration (s)
};

const int NX = 4;
const int NY = 3;
const time_t ACQ_TIME = 1700000000; // 2023-11-14 22:13:20 UTC

// stored value of voxel i of slice k in frame f (all data types can hold it)
double syntheticValue(int f, int k, int i, CPhilipsSubHeader::Data_Type type)
{
  switch(type) {
    case CPhilipsSubHeader::ByteData:      return (17 * f + 5 * k + 3 * i) % 256;
    case CPhilipsSubHeader::SignedShort:   return -1200 + 997 * f + 131 * k + 71 * i;
    case CPhilipsSubHeader::UnsignedShort: return 40000 + 1009 * f + 211 * k + 97 * i; // > 32767
    default:                               return -2.5 + 10.25 * f + 1.5 * k + 0.125 * i;
  }
}

// slice data in the machine byte order for CPhilipsFile::writeMatrix()
QByteArray sliceData(const QVector<double>& values, CPhilipsSubHeader::Data_Type type)
{
  QByteArray data;
  for(int i=0; i < values.size(); i++) {
    switch(type) {
      case CPhilipsSubHeader::ByteData: {
        const quint8 v = static_cast<quint8>(values[i]);
        data.append(reinterpret_cast<const char*>(&v), sizeof(v));
      } break;
      case CPhilipsSubHeader::SignedShort: {
        const qint16 v = static_cast<qint16>(values[i]);
        data.append(reinterpret_cast<const char*>(&v), sizeof(v));
      } break;
      case CPhilipsSubHeader::UnsignedShort: {
        const quint16 v = static_cast<quint16>(values[i]);
        data.append(reinterpret_cast<const char*>(&v), sizeof(v));
      } break;
      default: {
        const float v = static_cast<float>(values[i]);
        data.append(reinterpret_cast<const char*>(&v), sizeof(v));
      } break;
    }
  }
  return data;
}

// write a synthetic Philips image file with the frames firstFrame, firstFrame+1, ...
bool writeSyntheticFile(const QString& fileName, CPhilipsSubHeader::Data_Type type,
                        const QVector<SyntheticFrame>& frames, int firstFrame)
{
  CPhilipsFile file(fileName, CPhilipsMainHeader::Image);
  if(file.open(QIODevice::WriteOnly) == false)
    return false;

  QScopedPointer<CPhilipsMainHeader> mainHeader(file.createEmptyMainHeader());
  QScopedPointer<CPhilipsSubHeader> subHeaderPtr(file.createEmptySubHeader());
  bool result = (mainHeader.data() != NULL) && (subHeaderPtr.data() != NULL);

  if(result) {
    const int slices = frames.first().slices.size();
    mainHeader->setFiltyp(CPhilipsMainHeader::Image);
    mainHeader->setNslice(slices);
    mainHeader->setNframe(frames.size());
    mainHeader->setSlcthk(1);
    mainHeader->setDslice_thick(2.5f);
    mainHeader->setAcq_date_time(ACQ_TIME);

    CPhilipsSubHeaderImage* subHeader = static_cast<CPhilipsSubHeaderImage*>(subHeaderPtr.data());
    subHeader->setXdim(NX);
    subHeader->setYdim(NY);
    subHeader->setDatype(type);
    subHeader->setPix_spacing_x(2.0f);
    subHeader->setPix_spacing_y(3.0f);

    for(int f=0; f < frames.size() && result; f++) {
      subHeader->setStart_date_time(ACQ_TIME + frames[f].start);
      subHeader->setEnd_date_time(ACQ_TIME + frames[f].start + frames[f].duration);

      for(int k=0; k < frames[f].slices.size() && result; k++) {
        const SyntheticSlice& slice = frames[f].slices[k];
        subHeader->setSlcnum(k);
        subHeader->setImgscl(slice.scale);
        const QByteArray data = sliceData(slice.values, type);
        result = file.writeMatrix(data.constData(), data.size(), *subHeader, firstFrame + f, k);
      }
    }

    if(result)
      result = file.writeMainHeader(*mainHeader);
  }

  file.close();
  return result;
}

// synthetic frames with the stored values of syntheticValue() and the scale factors of scales[f][k]
QVector<SyntheticFrame> syntheticFrames(CPhilipsSubHeader::Data_Type type, const QVector<QVector<float> >& scales)
{
  QVector<SyntheticFrame> frames;
  for(int f=0; f < scales.size(); f++) {
    SyntheticFrame frame;
    frame.start = 60 * f;
    frame.duration = 30 + 30 * f;
    for(int k=0; k < scales[f].size(); k++) {
      SyntheticSlice slice;
      slice.scale = scales[f][k];
      for(int i=0; i < NX * NY; i++)
        slice.values.append(syntheticValue(f, k, i, type));
      frame.slices.append(slice);
    }
    frames.append(frame);
  }
  return frames;
}

// value of voxel i of ECAT7 data in machine byte order
double ecatValue(const char* data, qint64 i, int type)
{
  switch(type) {
    case CECATSubHeader::ByteData:  return reinterpret_cast<const quint8*>(data)[i];
    case CECATSubHeader::SunShort:  return reinterpret_cast<const qint16*>(data)[i];
    case CECATSubHeader::SunLong:   return reinterpret_cast<const qint32*>(data)[i];
    case CECATSubHeader::IEEEFloat: return reinterpret_cast<const float*>(data)[i];
  }
  return std::numeric_limits<double>::quiet_NaN();
}

// ECAT7 image sub header returned by CPhilipsFile::readMatrix() (NULL for other sub headers)
const CECAT7SubHeaderImage* image(const CECATSubHeader* subHeader)
{
  if(subHeader == NULL || subHeader->subHeaderType() != CECATSubHeader::ECAT7_Image)
    return NULL;
  return static_cast<const CECAT7SubHeaderImage*>(subHeader);
}

int ecatBytes(int type)
{
  switch(type) {
    case CECATSubHeader::ByteData:  return 1;
    case CECATSubHeader::SunShort:  return 2;
    case CECATSubHeader::SunLong:
    case CECATSubHeader::IEEEFloat: return 4;
  }
  return 0;
}

} // namespace

Q_DECLARE_METATYPE(CPhilipsSubHeader::Data_Type)

class TestPhilipsMatrix : public QObject
{
  Q_OBJECT

  private:
    MedIOTestData m_data;

  private slots:
    void initTestCase();

    void datasetFrames_data();
    void datasetFrames();

    void dataTypes_data();
    void dataTypes();

    void differentSliceScales();
    void invalidScaleFactor();
    void frameNumbering();
    void invalidAccess();
};

void TestPhilipsMatrix::initTestCase()
{
  m_data = medioTestData();
}

//---------------------------------------------------------------------------------------------
// files of the test datasets

void TestPhilipsMatrix::datasetFrames_data()
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

void TestPhilipsMatrix::datasetFrames()
{
  QFETCH(int, index);
  if(index < 0)
    QSKIP("no Philips files in the test data");

  const MedIOTestFile& file = m_data.files[index];
  const QJsonObject expected = file.expected;
  const QJsonArray dims = expected.value("dims").toArray();
  const QJsonArray pixelSize = expected.value("pixel_size_mm").toArray();
  const QJsonArray volumes = expected.value("volumes").toArray();
  const int frames = expected.value("frames").toInt();
  QCOMPARE(expected.value("data_type").toString(), QString("int16"));

  // ECAT7 file of the same dataset: expected values and stored voxels
  QString ecatPath;
  QJsonArray ecatMatrices;
  double ecatCalibration = 1.0;
  foreach(const MedIOTestFile& other, m_data.files) {
    if(other.format == "ecat" && other.dataset == file.dataset) {
      ecatPath = other.path;
      ecatMatrices = other.expected.value("matrices").toArray();
      ecatCalibration = other.expected.value("main_header").toObject().value("calibration_factor").toDouble(1.0);
    }
  }
  if(ecatPath.isEmpty() == false)
    QCOMPARE(ecatMatrices.size(), frames);

  CPhilipsFile philipsFile(file.path);
  QVERIFY(philipsFile.open(QIODevice::ReadOnly));
  QCOMPARE(int(philipsFile.numFrames()), frames);

  CECATFile ecatFile(ecatPath);
  if(ecatPath.isEmpty() == false)
    QVERIFY(ecatFile.open(QIODevice::ReadOnly));

  const qint64 voxels = static_cast<qint64>(dims[0].toInt()) * dims[1].toInt() * dims[2].toInt();

  for(int frame=1; frame <= frames; frame++) {
    QByteArray* data = NULL;
    CECATSubHeader* subHeader = NULL;
    QVERIFY2(philipsFile.readMatrix(data, subHeader, frame), qPrintable(QString("frame %1").arg(frame)));
    QScopedPointer<QByteArray> dataGuard(data);
    QScopedPointer<CECATSubHeader> subHeaderGuard(subHeader);
    QVERIFY(image(subHeader) != NULL);
    QVERIFY(data != NULL && subHeader != NULL);

    QCOMPARE(int(subHeader->subHeaderType()), int(CECATSubHeader::ECAT7_Image));
    const CECAT7SubHeaderImage* s = static_cast<const CECAT7SubHeaderImage*>(subHeader);

    // stored int16 values with one scale factor per frame in these files
    QCOMPARE(int(s->data_Type()), int(CECATSubHeader::SunShort));
    QCOMPARE(int(s->x_Dimension()), dims[0].toInt());
    QCOMPARE(int(s->y_Dimension()), dims[1].toInt());
    QCOMPARE(int(s->z_Dimension()), dims[2].toInt());
    QVERIFY(std::fabs(s->x_Pixel_Size() * 10.0 - pixelSize[0].toDouble()) < 1e-4);
    QVERIFY(std::fabs(s->y_Pixel_Size() * 10.0 - pixelSize[1].toDouble()) < 1e-4);
    QVERIFY(std::fabs(s->z_Pixel_Size() * 10.0 - pixelSize[2].toDouble()) < 1e-4);
    QCOMPARE(qint64(data->size()), voxels * 2);

    // value statistics of the stored values
    const qint16* values = reinterpret_cast<const qint16*>(data->constData());
    double minValue = values[0], maxValue = values[0], sum = 0.0;
    for(qint64 i=0; i < voxels; i++) {
      minValue = qMin(minValue, double(values[i]));
      maxValue = qMax(maxValue, double(values[i]));
      sum += values[i];
    }
    const QJsonObject e = volumes[frame - 1].toObject();
    QCOMPARE(minValue, e.value("raw_min").toDouble());
    QCOMPARE(maxValue, e.value("raw_max").toDouble());
    QCOMPARE(sum, e.value("raw_sum").toDouble());
    QCOMPARE(double(s->image_Min()), e.value("raw_min").toDouble());
    QCOMPARE(double(s->image_Max()), e.value("raw_max").toDouble());

    if(ecatPath.isEmpty())
      continue;

    // scale factor (physical value / stored value) and frame timing of the ECAT7 file
    const QJsonObject m = ecatMatrices[frame - 1].toObject();
    const double ecatScale = m.value("scale_factor").toDouble() * ecatCalibration;
    QVERIFY2(std::fabs(s->scale_Factor() - ecatScale) <= 1e-5 * ecatScale,
             qPrintable(QString("frame %1: scale factor %2 instead of %3").arg(frame).arg(s->scale_Factor(), 0, 'g', 8).arg(ecatScale, 0, 'g', 8)));
    QCOMPARE(qint64(s->frame_Start_Time()), qint64(m.value("frame_start_time_ms").toInt()));
    QCOMPARE(qint64(s->frame_Duration()), qint64(m.value("frame_duration_ms").toInt()));

    // the same stored values in the same voxel order as the ECAT7 file
    QByteArray* ecatData = NULL;
    CECATSubHeader* ecatSubHeader = NULL;
    QVERIFY(ecatFile.readMatrix(ecatData, ecatSubHeader, frame));
    QScopedPointer<QByteArray> ecatDataGuard(ecatData);
    QScopedPointer<CECATSubHeader> ecatSubHeaderGuard(ecatSubHeader);
    QCOMPARE(int(ecatSubHeader->data_Type()), int(CECATSubHeader::SunShort));
    QVERIFY(qint64(ecatData->size()) >= voxels * 2);
    QVERIFY(memcmp(ecatData->constData(), data->constData(), voxels * 2) == 0);

    // char* variant
    char* raw = NULL;
    unsigned int len = 0;
    CECATSubHeader* rawSubHeader = NULL;
    QVERIFY(philipsFile.readMatrix(raw, len, rawSubHeader, frame));
    QScopedArrayPointer<char> rawGuard(raw);
    QScopedPointer<CECATSubHeader> rawSubHeaderGuard(rawSubHeader);
    QVERIFY(image(rawSubHeader) != NULL);
    QCOMPARE(qint64(len), voxels * 2);
    QVERIFY(memcmp(raw, data->constData(), len) == 0);
    QCOMPARE(int(rawSubHeader->data_Type()), int(CECATSubHeader::SunShort));
    QCOMPARE(image(rawSubHeader)->scale_Factor(), s->scale_Factor());
  }

  // -1 = first frame
  QByteArray* first = NULL;
  QByteArray* fallback = NULL;
  CECATSubHeader* firstSubHeader = NULL;
  CECATSubHeader* fallbackSubHeader = NULL;
  QVERIFY(philipsFile.readMatrix(first, firstSubHeader, 1));
  QVERIFY(philipsFile.readMatrix(fallback, fallbackSubHeader));
  QScopedPointer<QByteArray> g1(first), g2(fallback);
  QScopedPointer<CECATSubHeader> g3(firstSubHeader), g4(fallbackSubHeader);
  QVERIFY(image(firstSubHeader) != NULL && image(fallbackSubHeader) != NULL);
  QVERIFY(*first == *fallback);
  QCOMPARE(image(fallbackSubHeader)->scale_Factor(), image(firstSubHeader)->scale_Factor());
  QCOMPARE(image(fallbackSubHeader)->frame_Start_Time(), image(firstSubHeader)->frame_Start_Time());

  philipsFile.close();
}

//---------------------------------------------------------------------------------------------
// synthetic files

void TestPhilipsMatrix::dataTypes_data()
{
  QTest::addColumn<CPhilipsSubHeader::Data_Type>("philipsType");
  QTest::addColumn<int>("ecatType");

  QTest::newRow("byte")           << CPhilipsSubHeader::ByteData      << int(CECATSubHeader::ByteData);
  QTest::newRow("signed short")   << CPhilipsSubHeader::SignedShort   << int(CECATSubHeader::SunShort);
  QTest::newRow("unsigned short") << CPhilipsSubHeader::UnsignedShort << int(CECATSubHeader::SunLong);
  QTest::newRow("float")          << CPhilipsSubHeader::Float         << int(CECATSubHeader::IEEEFloat);
}

void TestPhilipsMatrix::dataTypes()
{
  QFETCH(CPhilipsSubHeader::Data_Type, philipsType);
  QFETCH(int, ecatType);

  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString fileName = dir.filePath("synthetic.img");

  // two frames with three slices, one scale factor per frame
  QVector<QVector<float> > scales;
  scales << (QVector<float>() << 0.5f << 0.5f << 0.5f)
         << (QVector<float>() << 2.25f << 2.25f << 2.25f);
  const QVector<SyntheticFrame> frames = syntheticFrames(philipsType, scales);
  QVERIFY(writeSyntheticFile(fileName, philipsType, frames, 1));

  CPhilipsFile file(fileName);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QCOMPARE(int(file.numFrames()), 2);

  for(int f=0; f < frames.size(); f++) {
    QByteArray* data = NULL;
    CECATSubHeader* subHeader = NULL;
    QVERIFY(file.readMatrix(data, subHeader, f + 1));
    QScopedPointer<QByteArray> dataGuard(data);
    QScopedPointer<CECATSubHeader> subHeaderGuard(subHeader);
    QVERIFY(image(subHeader) != NULL);

    const CECAT7SubHeaderImage* s = static_cast<const CECAT7SubHeaderImage*>(subHeader);
    QCOMPARE(int(s->subHeaderType()), int(CECATSubHeader::ECAT7_Image));
    QCOMPARE(int(s->data_Type()), ecatType);
    QCOMPARE(int(s->x_Dimension()), NX);
    QCOMPARE(int(s->y_Dimension()), NY);
    QCOMPARE(int(s->z_Dimension()), 3);
    QCOMPARE(s->x_Pixel_Size(), 0.2f);
    QCOMPARE(s->y_Pixel_Size(), 0.3f);
    QCOMPARE(s->z_Pixel_Size(), 0.25f);
    QCOMPARE(double(s->scale_Factor()), double(scales[f][0]));
    QCOMPARE(qint64(s->frame_Start_Time()), qint64(frames[f].start) * 1000);
    QCOMPARE(qint64(s->frame_Duration()), qint64(frames[f].duration) * 1000);
    QCOMPARE(data->size(), 3 * NX * NY * ecatBytes(ecatType));

    // stored values, slices in the order of their slice numbers
    double minValue = 0.0, maxValue = 0.0;
    for(int k=0; k < 3; k++) {
      for(int i=0; i < NX * NY; i++) {
        const double value = ecatValue(data->constData(), k * NX * NY + i, ecatType);
        const double expectedValue = frames[f].slices[k].values[i];
        QCOMPARE(value, double(static_cast<float>(expectedValue)));
        minValue = (k == 0 && i == 0) ? expectedValue : qMin(minValue, expectedValue);
        maxValue = (k == 0 && i == 0) ? expectedValue : qMax(maxValue, expectedValue);
      }
    }

    // value range only for the 16 bit types of the ECAT7 sub header
    if(ecatType == CECATSubHeader::ByteData || ecatType == CECATSubHeader::SunShort) {
      QCOMPARE(double(s->image_Min()), minValue);
      QCOMPARE(double(s->image_Max()), maxValue);
    } else {
      QCOMPARE(int(s->image_Min()), 0);
      QCOMPARE(int(s->image_Max()), 0);
    }
  }

  file.close();
}

void TestPhilipsMatrix::differentSliceScales()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  const CPhilipsSubHeader::Data_Type types[] = { CPhilipsSubHeader::SignedShort, CPhilipsSubHeader::UnsignedShort,
                                                 CPhilipsSubHeader::ByteData, CPhilipsSubHeader::Float };

  for(unsigned int t=0; t < sizeof(types) / sizeof(types[0]); t++) {
    const QString fileName = dir.filePath(QString("scales%1.img").arg(t));

    // frame 1: different scale factors per slice, frame 2: one scale factor
    QVector<QVector<float> > scales;
    scales << (QVector<float>() << 0.5f << 1.75f << 0.03125f)
           << (QVector<float>() << 4.0f << 4.0f << 4.0f);
    const QVector<SyntheticFrame> frames = syntheticFrames(types[t], scales);
    QVERIFY(writeSyntheticFile(fileName, types[t], frames, 1));

    CPhilipsFile file(fileName);
    QVERIFY(file.open(QIODevice::ReadOnly));

    // frame 1: float32 physical values with scale factor 1
    QByteArray* data = NULL;
    CECATSubHeader* subHeader = NULL;
    QVERIFY(file.readMatrix(data, subHeader, 1));
    QScopedPointer<QByteArray> dataGuard(data);
    QScopedPointer<CECATSubHeader> subHeaderGuard(subHeader);
    QVERIFY(image(subHeader) != NULL);

    QCOMPARE(int(subHeader->data_Type()), int(CECATSubHeader::IEEEFloat));
    QCOMPARE(image(subHeader)->scale_Factor(), 1.0f);
    QCOMPARE(int(image(subHeader)->image_Min()), 0);
    QCOMPARE(int(image(subHeader)->image_Max()), 0);
    QCOMPARE(data->size(), 3 * NX * NY * 4);

    const float* values = reinterpret_cast<const float*>(data->constData());
    for(int k=0; k < 3; k++) {
      for(int i=0; i < NX * NY; i++) {
        const float stored = static_cast<float>(frames[0].slices[k].values[i]);
        const float expectedValue = static_cast<float>(double(stored) * double(scales[0][k]));
        QCOMPARE(values[k * NX * NY + i], expectedValue);
      }
    }

    // frame 2: stored values with the common scale factor
    QByteArray* data2 = NULL;
    CECATSubHeader* subHeader2 = NULL;
    QVERIFY(file.readMatrix(data2, subHeader2, 2));
    QScopedPointer<QByteArray> data2Guard(data2);
    QScopedPointer<CECATSubHeader> subHeader2Guard(subHeader2);
    QVERIFY(image(subHeader2) != NULL);
    QVERIFY(subHeader2->data_Type() != CECATSubHeader::IEEEFloat || types[t] == CPhilipsSubHeader::Float);
    QCOMPARE(image(subHeader2)->scale_Factor(), 4.0f);
    QCOMPARE(ecatValue(data2->constData(), NX * NY + 1, subHeader2->data_Type()),
             double(static_cast<float>(frames[1].slices[1].values[1])));

    file.close();
  }
}

void TestPhilipsMatrix::invalidScaleFactor()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString fileName = dir.filePath("unscaled.img");

  // frame 1: no valid scale factor at all (stored values with scale factor 1),
  // frame 2: one slice without a valid scale factor (scale factor 1 for it)
  QVector<QVector<float> > scales;
  scales << (QVector<float>() << 0.0f << 0.0f)
         << (QVector<float>() << 3.0f << 0.0f);
  const QVector<SyntheticFrame> frames = syntheticFrames(CPhilipsSubHeader::SignedShort, scales);
  QVERIFY(writeSyntheticFile(fileName, CPhilipsSubHeader::SignedShort, frames, 1));

  CPhilipsFile file(fileName);
  QVERIFY(file.open(QIODevice::ReadOnly));

  QByteArray* data = NULL;
  CECATSubHeader* subHeader = NULL;
  QVERIFY(file.readMatrix(data, subHeader, 1));
  QScopedPointer<QByteArray> dataGuard(data);
  QScopedPointer<CECATSubHeader> subHeaderGuard(subHeader);
  QVERIFY(image(subHeader) != NULL);
  QCOMPARE(int(subHeader->data_Type()), int(CECATSubHeader::SunShort));
  QCOMPARE(image(subHeader)->scale_Factor(), 1.0f);
  QCOMPARE(double(reinterpret_cast<const qint16*>(data->constData())[NX * NY + 2]), frames[0].slices[1].values[2]);

  QByteArray* data2 = NULL;
  CECATSubHeader* subHeader2 = NULL;
  QVERIFY(file.readMatrix(data2, subHeader2, 2));
  QScopedPointer<QByteArray> data2Guard(data2);
  QScopedPointer<CECATSubHeader> subHeader2Guard(subHeader2);
  QVERIFY(image(subHeader2) != NULL);
  QCOMPARE(int(subHeader2->data_Type()), int(CECATSubHeader::IEEEFloat));
  QCOMPARE(image(subHeader2)->scale_Factor(), 1.0f);
  const float* values = reinterpret_cast<const float*>(data2->constData());
  QCOMPARE(double(values[2]), frames[1].slices[0].values[2] * 3.0);
  QCOMPARE(double(values[NX * NY + 2]), frames[1].slices[1].values[2]);

  file.close();
}

void TestPhilipsMatrix::frameNumbering()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString fileName = dir.filePath("frames.img");

  // file starting with frame 3: the frame numbers are those of the file
  // (as for the other CPhilipsFile methods), -1 selects the first frame
  QVector<QVector<float> > scales;
  scales << (QVector<float>() << 1.0f << 1.0f)
         << (QVector<float>() << 2.0f << 2.0f)
         << (QVector<float>() << 3.0f << 3.0f);
  const QVector<SyntheticFrame> frames = syntheticFrames(CPhilipsSubHeader::SignedShort, scales);
  QVERIFY(writeSyntheticFile(fileName, CPhilipsSubHeader::SignedShort, frames, 3));

  CPhilipsFile file(fileName);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QCOMPARE(int(file.minFrame()), 3);
  QCOMPARE(int(file.maxFrame()), 5);

  for(int f=0; f < 3; f++) {
    QByteArray* data = NULL;
    CECATSubHeader* subHeader = NULL;
    QVERIFY(file.readMatrix(data, subHeader, 3 + f));
    QScopedPointer<QByteArray> dataGuard(data);
    QScopedPointer<CECATSubHeader> subHeaderGuard(subHeader);
    QVERIFY(image(subHeader) != NULL);
    QCOMPARE(double(image(subHeader)->scale_Factor()), double(scales[f][0]));
    QCOMPARE(qint64(image(subHeader)->frame_Start_Time()), qint64(frames[f].start) * 1000);
    QCOMPARE(double(reinterpret_cast<const qint16*>(data->constData())[5]), frames[f].slices[0].values[5]);
  }

  // -1: first frame
  QByteArray* data = NULL;
  CECATSubHeader* subHeader = NULL;
  QVERIFY(file.readMatrix(data, subHeader));
  QScopedPointer<QByteArray> dataGuard(data);
  QScopedPointer<CECATSubHeader> subHeaderGuard(subHeader);
  QVERIFY(image(subHeader) != NULL);
  QCOMPARE(image(subHeader)->scale_Factor(), 1.0f);
  QCOMPARE(double(reinterpret_cast<const qint16*>(data->constData())[5]), frames[0].slices[0].values[5]);

  // frames 1 and 2 do not exist
  for(short frame=1; frame <= 2; frame++) {
    QByteArray* missing = reinterpret_cast<QByteArray*>(1);
    CECATSubHeader* missingSubHeader = reinterpret_cast<CECATSubHeader*>(1);
    QVERIFY(file.readMatrix(missing, missingSubHeader, frame) == false);
    QVERIFY(missing == NULL && missingSubHeader == NULL);
  }

  file.close();
}

void TestPhilipsMatrix::invalidAccess()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString fileName = dir.filePath("invalid.img");

  QVector<QVector<float> > scales;
  scales << (QVector<float>() << 1.0f << 1.0f);
  QVERIFY(writeSyntheticFile(fileName, CPhilipsSubHeader::SignedShort,
                             syntheticFrames(CPhilipsSubHeader::SignedShort, scales), 1));

  CPhilipsFile file(fileName);

  // not opened
  QByteArray* data = reinterpret_cast<QByteArray*>(1);
  CECATSubHeader* subHeader = reinterpret_cast<CECATSubHeader*>(1);
  QVERIFY(file.readMatrix(data, subHeader, 1) == false);
  QVERIFY(data == NULL && subHeader == NULL);

  QVERIFY(file.open(QIODevice::ReadOnly));

  // frames that do not exist
  const short invalidFrames[] = { 0, 2, -2 };
  for(unsigned int i=0; i < sizeof(invalidFrames) / sizeof(invalidFrames[0]); i++) {
    data = reinterpret_cast<QByteArray*>(1);
    subHeader = reinterpret_cast<CECATSubHeader*>(1);
    QVERIFY(file.readMatrix(data, subHeader, invalidFrames[i]) == false);
    QVERIFY(data == NULL && subHeader == NULL);

    char* raw = reinterpret_cast<char*>(1);
    unsigned int len = 1;
    subHeader = reinterpret_cast<CECATSubHeader*>(1);
    QVERIFY(file.readMatrix(raw, len, subHeader, invalidFrames[i]) == false);
    QVERIFY(raw == NULL && len == 0 && subHeader == NULL);
  }

  file.close();
}

QTEST_GUILESS_MAIN(TestPhilipsMatrix)
#include "testPhilipsMatrix.moc"
