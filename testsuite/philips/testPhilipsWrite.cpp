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

// Tests of writing Philips image files with CPhilipsFile. The directory of
// a Philips file holds 31 items per directory list; further lists are
// stored between the data blocks, and the slices are stored compressed.
// Files with more than 31 slices written in any frame or slice order, and
// files whose slices are overwritten with data of another compressed size,
// have to be read back unchanged, and no directory list or slice may
// overlap the data of another slice (checked on block level).

#include <CPhilipsFile.h>
#include <CPhilipsMainHeader.h>
#include <CPhilipsSubHeader.h>
#include <CPhilipsSubHeaderImage.h>

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QtEndian>

#include <algorithm>

static const int nx = 31, ny = 31, slices = 39;

// stored value of voxel i of a slice
static qint16 value(int frame, int slice, int i)
{
  return static_cast<qint16>((i * 7 + slice * 13 + frame * 101) % 30000);
}

// directory lists and items of a Philips file (big endian, blocks of 512
// bytes counted from 1, the first directory list in block 2)
struct Block { quint32 start, end; QString what; };

static QList<Block> directoryBlocks(const QString& fileName, int* items)
{
  QList<Block> blocks;
  QFile file(fileName);
  if(file.open(QIODevice::ReadOnly) == false)
    return blocks;
  const QByteArray data = file.readAll();
  const uchar* d = reinterpret_cast<const uchar*>(data.constData());

  *items = 0;
  quint32 list = 2;
  QList<quint32> seen;
  while(list > 1 && seen.contains(list) == false && static_cast<qint64>(list) * 512 <= data.size()) {
    seen.append(list);
    const uchar* head = d + (list - 1) * 512;
    blocks.append(Block{ list, list, "directory list" });
    const quint32 next = qFromBigEndian<quint32>(head + 4);
    const quint32 follow = qFromBigEndian<quint32>(head + 12);
    for(quint32 i=0; i < follow && i < 31; i++) {
      const uchar* item = head + 16 + 16 * i;
      blocks.append(Block{ qFromBigEndian<quint32>(item + 4), qFromBigEndian<quint32>(item + 8),
                           QString("item %1").arg(qFromBigEndian<quint32>(item), 8, 16, QChar('0')) });
      (*items)++;
    }
    list = next;
  }
  return blocks;
}


// slice with values that compress well (variant 0) or hardly at all (variant 1)
static QByteArray slice(int frame, int slice, int variant)
{
  QByteArray plane(nx * ny * 2, '\0');
  quint32 seed = 1 + frame * 1000 + slice;
  for(int i=0; i < nx * ny; i++) {
    seed = seed * 1103515245u + 12345u;
    reinterpret_cast<qint16*>(plane.data())[i] = (variant == 0) ? static_cast<qint16>(frame * 100 + slice)
                                                                : static_cast<qint16>(seed >> 16);
  }
  return plane;
}

// write the slices of the given frames with the given variant (slices in descending order if reversed)
static bool writeSlices(CPhilipsFile& file, CPhilipsSubHeaderImage* subHeader, const QList<int>& frames, int variant, bool reversed)
{
  foreach(int frame, frames) {
    for(int s=0; s < slices; s++) {
      const int k = reversed ? slices - 1 - s : s;
      const QByteArray plane = slice(frame, k, variant);
      subHeader->setSlcnum(k);
      if(file.writeMatrix(plane.constData(), plane.size(), *subHeader, frame, k) == false)
        return false;
    }
  }
  return true;
}

// directory blocks of a file do not overlap
static QString overlap(const QString& path, int* items)
{
  QList<Block> blocks = directoryBlocks(path, items);
  std::sort(blocks.begin(), blocks.end(), [](const Block& a, const Block& b) { return a.start < b.start; });
  for(int i=1; i < blocks.size(); i++)
    if(blocks[i].start <= blocks[i-1].end)
      return QString("%1 (blocks %2-%3) overlaps %4 (blocks %5-%6)").arg(blocks[i].what).arg(blocks[i].start).arg(blocks[i].end)
             .arg(blocks[i-1].what).arg(blocks[i-1].start).arg(blocks[i-1].end);
  return QString();
}

class TestPhilipsWrite : public QObject
{
  Q_OBJECT

  private:
    QTemporaryDir m_tmp;

  private slots:
    void initTestCase() { QVERIFY(m_tmp.isValid()); }
    void writeOrder_data();
    void writeOrder();
    void overwrite();
    void readFrame();
};

void TestPhilipsWrite::writeOrder_data()
{
  QTest::addColumn<QList<int> >("frames");
  QTest::addColumn<bool>("reversedSlices");

  QTest::newRow("ascending")          << (QList<int>() << 1 << 2 << 3) << false;
  QTest::newRow("descending")         << (QList<int>() << 3 << 2 << 1) << false;
  QTest::newRow("mixed")              << (QList<int>() << 2 << 3 << 1) << false;
  QTest::newRow("mixed, 5 frames")    << (QList<int>() << 5 << 1 << 4 << 2 << 3) << false;
  QTest::newRow("descending slices")  << (QList<int>() << 1 << 2) << true;
}

void TestPhilipsWrite::writeOrder()
{
  QFETCH(QList<int>, frames);
  QFETCH(bool, reversedSlices);

  const QString path = m_tmp.filePath(QString(QTest::currentDataTag()).replace(QRegExp("[^a-z0-9]"), "_") + ".img");
  {
    CPhilipsFile file(path, CPhilipsMainHeader::Image);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QScopedPointer<CPhilipsMainHeader> mainHeader(file.createEmptyMainHeader());
    mainHeader->setFiltyp(CPhilipsMainHeader::Image);
    QVERIFY(file.writeMainHeader(*mainHeader));

    QScopedPointer<CPhilipsSubHeader> subHeaderPtr(file.createEmptySubHeader());
    QVERIFY(subHeaderPtr.data() != NULL);
    CPhilipsSubHeaderImage* subHeader = static_cast<CPhilipsSubHeaderImage*>(subHeaderPtr.data());
    subHeader->setDatype(CPhilipsSubHeader::SignedShort);
    subHeader->setXdim(nx);
    subHeader->setYdim(ny);
    subHeader->setImgscl(1.0f);

    QByteArray plane(nx * ny * 2, '\0');
    foreach(int frame, frames) {
      for(int s=0; s < slices; s++) {
        const int slice = reversedSlices ? slices - 1 - s : s;
        for(int i=0; i < nx * ny; i++)
          reinterpret_cast<qint16*>(plane.data())[i] = value(frame, slice, i);
        subHeader->setSlcnum(slice);
        QVERIFY(file.writeMatrix(plane.constData(), plane.size(), *subHeader, frame, slice));
      }
    }
    file.close();
  }

  // every slice is read back unchanged
  CPhilipsFile file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QCOMPARE(static_cast<int>(file.numFrames()), frames.size());
  foreach(int frame, frames) {
    QCOMPARE(static_cast<int>(file.numSlices(frame)), slices);
    for(int slice=0; slice < slices; slice++) {
      QByteArray* data = NULL;
      CPhilipsSubHeader* subHeader = NULL;
      QVERIFY2(file.readMatrix(data, subHeader, frame, slice), qPrintable(QString("frame %1 slice %2").arg(frame).arg(slice)));
      QScopedPointer<QByteArray> dataGuard(data);
      QScopedPointer<CPhilipsSubHeader> subHeaderGuard(subHeader);
      const CPhilipsSubHeaderImage* s = static_cast<const CPhilipsSubHeaderImage*>(subHeader);
      QVERIFY2(s->xdim() == nx && s->ydim() == ny && s->slcnum() == slice,
               qPrintable(QString("sub header of frame %1 slice %2: %3 x %4, slice %5").arg(frame).arg(slice).arg(s->xdim()).arg(s->ydim()).arg(s->slcnum())));
      QVERIFY(data->size() >= nx * ny * 2);
      for(int i=0; i < nx * ny; i++)
        QCOMPARE(reinterpret_cast<const qint16*>(data->constData())[i], value(frame, slice, i));
    }
  }
  file.close();

  // no directory list overlaps the blocks of a slice
  int items = 0;
  QList<Block> blocks = directoryBlocks(path, &items);
  QCOMPARE(items, frames.size() * slices + 1); // + extended main header
  std::sort(blocks.begin(), blocks.end(), [](const Block& a, const Block& b) { return a.start < b.start; });
  for(int i=1; i < blocks.size(); i++)
    QVERIFY2(blocks[i].start > blocks[i-1].end,
             qPrintable(QString("%1 (blocks %2-%3) overlaps %4 (blocks %5-%6)").arg(blocks[i].what).arg(blocks[i].start).arg(blocks[i].end)
                        .arg(blocks[i-1].what).arg(blocks[i-1].start).arg(blocks[i-1].end)));
}

// slices overwritten with data of a larger and of a smaller compressed size
void TestPhilipsWrite::overwrite()
{
  const QString path = m_tmp.filePath("overwrite.img");
  QMap<QPair<int, int>, int> variants;  // (frame, slice) -> variant written last
  {
    CPhilipsFile file(path, CPhilipsMainHeader::Image);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QScopedPointer<CPhilipsMainHeader> mainHeader(file.createEmptyMainHeader());
    mainHeader->setFiltyp(CPhilipsMainHeader::Image);
    QVERIFY(file.writeMainHeader(*mainHeader));
    QScopedPointer<CPhilipsSubHeader> subHeaderPtr(file.createEmptySubHeader());
    CPhilipsSubHeaderImage* subHeader = static_cast<CPhilipsSubHeaderImage*>(subHeaderPtr.data());
    subHeader->setDatype(CPhilipsSubHeader::SignedShort);
    subHeader->setXdim(nx);
    subHeader->setYdim(ny);
    subHeader->setImgscl(1.0f);

    // frames 1-3 compress well, then frame 2 (descending slices) and frame 1
    // hardly at all, then single slices of frame 2 well again
    QVERIFY(writeSlices(file, subHeader, QList<int>() << 1 << 2 << 3, 0, false));
    QVERIFY(writeSlices(file, subHeader, QList<int>() << 2, 1, true));
    QVERIFY(writeSlices(file, subHeader, QList<int>() << 1, 1, false));
    for(int f=1; f <= 3; f++)
      for(int k=0; k < slices; k++)
        variants[qMakePair(f, k)] = (f == 3) ? 0 : 1;

    foreach(int k, QList<int>() << 0 << 17 << 38) {
      const QByteArray plane = slice(2, k, 0);
      subHeader->setSlcnum(k);
      QVERIFY(file.writeMatrix(plane.constData(), plane.size(), *subHeader, 2, k));
      variants[qMakePair(2, k)] = 0;
    }
    file.close();
  }

  CPhilipsFile file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QCOMPARE(static_cast<int>(file.numFrames()), 3);
  for(int f=1; f <= 3; f++)
    for(int k=0; k < slices; k++) {
      QByteArray* data = NULL;
      CPhilipsSubHeader* subHeader = NULL;
      QVERIFY2(file.readMatrix(data, subHeader, f, k), qPrintable(QString("frame %1 slice %2").arg(f).arg(k)));
      QScopedPointer<QByteArray> dataGuard(data);
      QScopedPointer<CPhilipsSubHeader> subHeaderGuard(subHeader);
      QCOMPARE(static_cast<const CPhilipsSubHeaderImage*>(subHeader)->xdim(), static_cast<short>(nx));
      QCOMPARE(static_cast<const CPhilipsSubHeaderImage*>(subHeader)->slcnum(), static_cast<short>(k));
      QVERIFY2(data->left(nx * ny * 2) == slice(f, k, variants[qMakePair(f, k)]),
               qPrintable(QString("data of frame %1 slice %2").arg(f).arg(k)));
    }
  file.close();

  int items = 0;
  const QString error = overlap(path, &items);
  QVERIFY2(error.isEmpty(), qPrintable(error));
  QCOMPARE(items, 3 * slices + 1);
}

// readMatrix() with slice -1 returns all slices of a frame, also for
// frames with a single slice (division by zero before)
void TestPhilipsWrite::readFrame()
{
  const QString path = m_tmp.filePath("frames.img");
  const int sliceCount[2] = { 1, 3 };
  {
    CPhilipsFile file(path, CPhilipsMainHeader::Image);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QScopedPointer<CPhilipsMainHeader> mainHeader(file.createEmptyMainHeader());
    mainHeader->setFiltyp(CPhilipsMainHeader::Image);
    QVERIFY(file.writeMainHeader(*mainHeader));
    QScopedPointer<CPhilipsSubHeader> subHeaderPtr(file.createEmptySubHeader());
    CPhilipsSubHeaderImage* subHeader = static_cast<CPhilipsSubHeaderImage*>(subHeaderPtr.data());
    subHeader->setDatype(CPhilipsSubHeader::SignedShort);
    subHeader->setXdim(nx);
    subHeader->setYdim(ny);
    subHeader->setImgscl(1.0f);
    for(int f=0; f < 2; f++)
      for(int k=0; k < sliceCount[f]; k++) {
        const QByteArray plane = slice(f + 1, k, 1);
        subHeader->setSlcnum(k);
        QVERIFY(file.writeMatrix(plane.constData(), plane.size(), *subHeader, f + 1, k));
      }
    file.close();
  }

  CPhilipsFile file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));
  for(int f=0; f < 2; f++) {
    // a single slice is read as frame; the slice numbering of the file is 0..2
    if(f == 0)
      QCOMPARE(static_cast<int>(file.numSlices(1)), 1);

    QByteArray expected;
    for(int k=0; k < sliceCount[f]; k++)
      expected.append(slice(f + 1, k, 1));

    QByteArray* data = NULL;
    CPhilipsSubHeader* subHeader = NULL;
    {
      QVERIFY(file.readMatrix(data, f + 1));
      QVERIFY(*data == expected);
      delete data;

      QVERIFY(file.readMatrix(data, subHeader, f + 1));
      QVERIFY(*data == expected);
      QCOMPARE(static_cast<int>(subHeader->slcnum()), 0);
      delete data;
      delete subHeader;

      char* raw = NULL;
      unsigned int length = 0;
      QVERIFY(file.readMatrix(raw, length, f + 1));
      QVERIFY(QByteArray(raw, length) == expected);
      delete[] raw;
      QVERIFY(file.readMatrix(raw, length, subHeader, f + 1));
      QVERIFY(QByteArray(raw, length) == expected);
      delete[] raw;
      delete subHeader;
    }
  }

  // frames that do not exist
  QByteArray* data = NULL;
  CPhilipsSubHeader* subHeader = NULL;
  QVERIFY(file.readMatrix(data, subHeader, 5) == false);
  QVERIFY(data == NULL && subHeader == NULL);
}

QTEST_GUILESS_MAIN(TestPhilipsWrite)
#include "testPhilipsWrite.moc"
