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

// Tests of reading microPET (Siemens Concorde) image files with
// CConcordeFile. There is no microPET test data in the repository, so the
// tests write synthetic files: an ASCII header <name>.hdr with the main
// header and one section per frame ("frame <n>" ... "end_of_header"), and
// the data file <name> with 4 x 3 x 2 int16 voxels (Intel byte order) per
// frame. The voxels of the frame with frame number n have the values
// 1 + i + 100 * n.

#include <CConcordeFile.h>
#include <CConcordeImage.h>
#include <CConcordeMainHeader.h>
#include <CConcordeFrameHeader.h>
#include <CMedIODataFactory.h>

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QTextStream>
#include <QtEndian>

static const int voxels = 24;

// frame sections of the header: frame number and position of its data (in frames)
struct Frame { int number; int position; };

static bool writeConcorde(const QString& path, const QList<Frame>& frames, int dataFrames)
{
  QFile header(path + ".hdr");
  if(header.open(QIODevice::WriteOnly | QIODevice::Text) == false)
    return false;

  QTextStream h(&header);
  h << "# synthetic microPET header (libmedio test)\n"
    << "model 2000\n"
    << "file_type 5\n"
    << "acquisition_mode 3\n"
    << "total_frames " << frames.size() << "\n"
    << "isotope_branching_fraction 1\n"
    << "data_type 2\n"
    << "number_of_dimensions 3\n"
    << "x_dimension 4\n"
    << "y_dimension 3\n"
    << "z_dimension 2\n"
    << "pixel_size 0.2\n"
    << "axial_crystal_pitch 0.6\n"
    << "calibration_factor 1\n"
    << "end_of_header\n";
  foreach(const Frame& f, frames) {
    h << "frame " << f.number << "\n"
      << "event_type 1\n"
      << "data_file_pointer 0 " << f.position * voxels * 2 << "\n"
      << "frame_start " << f.number * 60 << "\n"
      << "frame_duration 60\n"
      << "scale_factor " << 1.0 + f.number << "\n"
      << "end_of_header\n";
  }
  header.close();

  // data of the frames at their positions
  QFile data(path);
  if(data.open(QIODevice::WriteOnly) == false)
    return false;
  for(int p=0; p < dataFrames; p++) {
    int number = -1;
    foreach(const Frame& f, frames) {
      if(f.position == p)
        number = f.number;
    }
    for(int i=0; i < voxels; i++) {
      const qint16 v = qToLittleEndian(static_cast<qint16>(number < 0 ? -1 : 1 + i + 100 * number));
      data.write(reinterpret_cast<const char*>(&v), 2);
    }
  }
  return true;
}

// the matrix holds the voxels of the frame with the given number
static bool isFrame(const char* data, unsigned int length, int number)
{
  if(length != voxels * 2)
    return false;
  for(int i=0; i < voxels; i++) {
    if(reinterpret_cast<const qint16*>(data)[i] != 1 + i + 100 * number)
      return false;
  }
  return true;
}

class TestConcordeFile : public QObject
{
  Q_OBJECT

  private:
    QTemporaryDir m_tmp;

  private slots:
    void initTestCase() { QVERIFY(m_tmp.isValid()); }
    void readMatrix();
    void truncatedData();
    void copiedHeaders();
};

// all variants of readMatrix() return the frames of the file
void TestConcordeFile::readMatrix()
{
  const QString path = m_tmp.filePath("image");
  QVERIFY(writeConcorde(path, QList<Frame>() << Frame{ 0, 0 } << Frame{ 1, 1 }, 2));

  CConcordeImage file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));

  for(int f=1; f <= 2; f++) {
    QByteArray* data = NULL;
    QVERIFY(file.readMatrix(data, f));
    QVERIFY(isFrame(data->constData(), data->size(), f - 1));
    // the QByteArray owns its data
    data->fill('\0');
    delete data;

    CConcordeFrameHeader* header = NULL;
    QVERIFY(file.readMatrix(data, header, f));
    QVERIFY(isFrame(data->constData(), data->size(), f - 1));
    QCOMPARE(header->frame(), f - 1);
    delete data;
    delete header;

    char* raw = NULL;
    unsigned int length = 0;
    QVERIFY(file.readMatrix(raw, length, f));
    QVERIFY(isFrame(raw, length, f - 1));
    delete[] raw;

    QVERIFY(file.readMatrix(raw, length, header, f));
    QVERIFY(isFrame(raw, length, f - 1));
    delete[] raw;
    delete header;
  }
  file.close();
}

// a data file without all frames: reading a missing frame fails
void TestConcordeFile::truncatedData()
{
  const QString path = m_tmp.filePath("truncated");
  QVERIFY(writeConcorde(path, QList<Frame>() << Frame{ 0, 0 } << Frame{ 1, 1 }, 1));

  CConcordeImage file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));

  char* raw = NULL;
  unsigned int length = 0;
  QVERIFY(file.readMatrix(raw, length, 1));
  delete[] raw;

  QVERIFY(file.readMatrix(raw, length, 2) == false);
  QVERIFY(raw == NULL && length == 0);

  QByteArray* data = NULL;
  CConcordeFrameHeader* header = NULL;
  QVERIFY(file.readMatrix(data, header, 2) == false);
  QVERIFY(data == NULL && header == NULL);
  QVERIFY(file.readMatrix(raw, length, header, 2) == false);
  QVERIFY(raw == NULL && header == NULL);
  file.close();
}

// copies of the headers load into their own values (also after the original is deleted)
void TestConcordeFile::copiedHeaders()
{
  const QString path = m_tmp.filePath("copies");
  QVERIFY(writeConcorde(path, QList<Frame>() << Frame{ 0, 0 } << Frame{ 1, 1 }, 2));

  CConcordeImage file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));

  CConcordeFrameHeader* header = NULL;
  QVERIFY(file.readSubHeader(header, 2));
  CConcordeFrameHeader copy(*header);
  delete header;
  copy.setScaleFactor(0.0f);
  QVERIFY(copy.load());
  QCOMPARE(static_cast<double>(copy.scaleFactor()), 2.0);
  QCOMPARE(copy.frame(), 1);

  CConcordeMainHeader* mainHeader = NULL;
  QVERIFY(file.readMainHeader(mainHeader));
  QScopedPointer<CConcordeMainHeader> mainCopy(static_cast<CConcordeMainHeader*>(mainHeader->clone()));
  delete mainHeader;
  QCOMPARE(mainCopy->totalFrames(), 2);
  QCOMPARE(mainCopy->xDimension(), 4);
  file.close();
}

QTEST_GUILESS_MAIN(TestConcordeFile)
#include "testConcordeFile.moc"
