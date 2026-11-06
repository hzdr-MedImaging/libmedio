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

// Writes multi-frame Philips image files slice by slice with the native
// CPhilipsFile API in different frame and slice orders and reads every slice
// back. A file with more than 31 matrices needs several directory lists; they
// must neither overlap the data of matrices written earlier nor be overwritten
// by matrices written later, whatever the order the matrices are written in.

#include <CPhilipsFile.h>
#include <CPhilipsMainHeader.h>
#include <CPhilipsSubHeader.h>
#include <CPhilipsSubHeaderImage.h>

#include <QtTest/QtTest>
#include <QFile>
#include <QMap>
#include <QTemporaryDir>
#include <QVector>

namespace {

const int NX = 32;
const int NY = 32;
const int NSLICES = 39;

// a matrix (frame, slice) in the order it is written
struct MatrixPos
{
  short frame;
  short slice;
};

// deterministic, badly compressible voxel values of slice (frame, slice), so
// that the compressed matrices span several 512 byte blocks of varying length
QVector<float> sliceValues(short frame, short slice)
{
  QVector<float> values(NX * NY);
  quint32 state = 2654435761u * static_cast<quint32>(frame * 4096 + slice + 1);
  for(int i=0; i < values.size(); i++) {
    state = state * 1664525u + 1013904223u;
    values[i] = static_cast<float>(state >> 8) / 1024.0f - 8192.0f;
  }
  return values;
}

// write order: the frames in the given order, the slices of each frame
// ascending or descending
QVector<MatrixPos> frameOrder(const QVector<short>& frames, bool slicesDescending)
{
  QVector<MatrixPos> order;
  foreach(short frame, frames) {
    for(int k=0; k < NSLICES; k++) {
      MatrixPos pos;
      pos.frame = frame;
      pos.slice = static_cast<short>(slicesDescending ? NSLICES - k : k + 1);
      order.append(pos);
    }
  }
  return order;
}

bool writeFile(const QString& fileName, const QVector<MatrixPos>& order, int frames)
{
  CPhilipsFile file(fileName, CPhilipsMainHeader::Image);
  if(file.open(QIODevice::WriteOnly) == false)
    return false;

  QScopedPointer<CPhilipsMainHeader> mainHeader(file.createEmptyMainHeader());
  QScopedPointer<CPhilipsSubHeader> subHeaderPtr(file.createEmptySubHeader());
  bool result = (mainHeader.data() != NULL) && (subHeaderPtr.data() != NULL);

  if(result) {
    mainHeader->setFiltyp(CPhilipsMainHeader::Image);
    mainHeader->setNslice(NSLICES);
    mainHeader->setNframe(frames);

    CPhilipsSubHeaderImage* subHeader = static_cast<CPhilipsSubHeaderImage*>(subHeaderPtr.data());
    subHeader->setXdim(NX);
    subHeader->setYdim(NY);
    subHeader->setDatype(CPhilipsSubHeader::Float);

    for(int i=0; i < order.size() && result; i++) {
      const QVector<float> values = sliceValues(order[i].frame, order[i].slice);
      subHeader->setSlcnum(order[i].slice);
      result = file.writeMatrix(reinterpret_cast<const char*>(values.constData()),
                                static_cast<unsigned int>(values.size() * sizeof(float)),
                                *subHeader, order[i].frame, order[i].slice);
    }

    if(result)
      result = file.writeMainHeader(*mainHeader);
  }

  file.close();
  return result;
}

quint32 bigEndian32(const QByteArray& data, int offset)
{
  const uchar* p = reinterpret_cast<const uchar*>(data.constData()) + offset;
  return (quint32(p[0]) << 24) | (quint32(p[1]) << 16) | (quint32(p[2]) << 8) | quint32(p[3]);
}

// Checks the layout of the file independently of the libmedio reader: follows
// the chain of directory lists (512 byte blocks starting at block 2, a 16 byte
// header with free items, next, previous and number of items, then up to 31
// items of 16 bytes with matrix ID, first and last data block) and verifies
// that no directory list block lies within the data of an item, that the data
// of the items do not overlap and that the directory holds numItems items.
QString checkLayout(const QString& fileName, int numItems)
{
  QFile file(fileName);
  if(file.open(QIODevice::ReadOnly) == false)
    return "cannot open file";
  const QByteArray content = file.readAll();
  const qint64 numBlocks = content.size() / 512;

  // blocks used by directory lists and data (block -> owner)
  QMap<qint64, QString> used;
  int items = 0;
  qint64 block = 2;
  do {
    if(block < 2 || block > numBlocks || used.size() > numBlocks)
      return QString("directory list at invalid block %1").arg(block);
    if(used.contains(block))
      return QString("directory list at block %1 overlaps %2").arg(block).arg(used[block]);
    used.insert(block, "a directory list");

    const QByteArray dirList = content.mid((block - 1) * 512, 512);
    const quint32 count = bigEndian32(dirList, 12);
    if(count > 31)
      return QString("directory list at block %1 holds %2 items").arg(block).arg(count);

    for(quint32 i=0; i < count; i++) {
      const quint32 id = bigEndian32(dirList, 16 + 16 * i);
      qint64 start = bigEndian32(dirList, 16 + 16 * i + 4);
      qint64 end = bigEndian32(dirList, 16 + 16 * i + 8);
      // the extended main header occupies blocks 3-18 (16 blocks)
      if(id == 0x7FFFFFFF) {
        start = 3;
        end = 18;
      }
      const QString owner = QString("matrix %1").arg(id, 8, 16, QChar('0'));
      if(start < 3 || end < start || end > numBlocks)
        return QString("%1 has invalid data blocks %2-%3").arg(owner).arg(start).arg(end);
      for(qint64 b=start; b <= end; b++) {
        if(used.contains(b))
          return QString("block %1 of %2 overlaps %3").arg(b).arg(owner).arg(used[b]);
        used.insert(b, owner);
      }
      items++;
    }

    block = bigEndian32(dirList, 4);
  }
  while(block != 2);

  if(items != numItems)
    return QString("directory holds %1 items instead of %2").arg(items).arg(numItems);

  return QString();
}

} // namespace

class TestPhilipsWrite : public QObject
{
  Q_OBJECT

  private slots:
    void writeOrder_data();
    void writeOrder();
};

void TestPhilipsWrite::writeOrder_data()
{
  QTest::addColumn<QVector<short> >("frames");
  QTest::addColumn<bool>("slicesDescending");

  QTest::newRow("frames ascending")              << (QVector<short>() << 1 << 2 << 3) << false;
  QTest::newRow("frames descending")             << (QVector<short>() << 3 << 2 << 1) << false;
  QTest::newRow("frames mixed 2,3,1")            << (QVector<short>() << 2 << 3 << 1) << false;
  QTest::newRow("frames mixed 1,3,2")            << (QVector<short>() << 1 << 3 << 2) << false;
  QTest::newRow("frames descending, slices too") << (QVector<short>() << 3 << 2 << 1) << true;
  QTest::newRow("frames ascending, slices desc") << (QVector<short>() << 1 << 2 << 3) << true;
  QTest::newRow("five frames mixed 4,1,5,2,3")   << (QVector<short>() << 4 << 1 << 5 << 2 << 3) << false;
}

void TestPhilipsWrite::writeOrder()
{
  QFETCH(QVector<short>, frames);
  QFETCH(bool, slicesDescending);

  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString fileName = dir.filePath("write_order.img");

  QVERIFY(writeFile(fileName, frameOrder(frames, slicesDescending), frames.size()));

  // the extended main header and one item per slice
  const QString layoutError = checkLayout(fileName, 1 + frames.size() * NSLICES);
  QVERIFY2(layoutError.isEmpty(), qPrintable(layoutError));

  CPhilipsFile file(fileName);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QCOMPARE(int(file.numFrames()), frames.size());

  for(short frame=1; frame <= frames.size(); frame++) {
    QCOMPARE(int(file.numSlices(frame)), NSLICES);

    for(short slice=1; slice <= NSLICES; slice++) {
      const QByteArray tag = QString("frame %1 slice %2").arg(frame).arg(slice).toLatin1();

      char* data = NULL;
      unsigned int len = 0;
      CPhilipsSubHeader* subHeader = NULL;
      const bool read = file.readMatrix(data, len, subHeader, frame, slice);
      QScopedArrayPointer<char> dataPtr(data);
      QScopedPointer<CPhilipsSubHeader> subHeaderPtr(subHeader);

      QVERIFY2(read, tag.constData());
      QVERIFY2(subHeader != NULL, tag.constData());
      QVERIFY2(subHeader->xdim() == NX, tag.constData());
      QVERIFY2(subHeader->ydim() == NY, tag.constData());
      QVERIFY2(subHeader->datype() == CPhilipsSubHeader::Float, tag.constData());
      QVERIFY2(subHeader->slcnum() == slice, tag.constData());

      const QVector<float> expected = sliceValues(frame, slice);
      QVERIFY2(len == expected.size() * sizeof(float), tag.constData());
      QVERIFY2(memcmp(data, expected.constData(), len) == 0, tag.constData());
    }
  }

  file.close();
}

QTEST_GUILESS_MAIN(TestPhilipsWrite)
#include "testPhilipsWrite.moc"
