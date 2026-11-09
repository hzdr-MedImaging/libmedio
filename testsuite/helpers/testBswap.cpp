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

// Tests bswap_matrix() (src/include/bswap.h), which swaps the byte order of
// the values of an array, for 8, 16, 32 and 64 bit values: into a new array,
// into another array and in place, also for arrays that are only aligned to
// the size of their values (e.g. a 16 bit array at an address that is not a
// multiple of 4). The expected bytes are the bytes of every value in reverse
// order. Where the compiler supports it, the test is built with the
// alignment sanitizer, so that misaligned accesses abort it.

#include <QtGlobal>
#include <QByteArray>
#include <QtTest/QtTest>

#include "bswap.h"

#include <cstring>

namespace {

// bytes 0x01, 0x02, ... of count values of size bytes
QByteArray pattern(int size, int count)
{
  QByteArray bytes(size * count, '\0');
  for(int i=0; i < bytes.size(); i++)
    bytes[i] = static_cast<char>(i + 1);
  return bytes;
}

// the bytes of every value in reverse order
QByteArray reversed(const QByteArray& bytes, int size)
{
  QByteArray result(bytes.size(), '\0');
  for(int v=0; v < bytes.size() / size; v++)
    for(int b=0; b < size; b++)
      result[v * size + b] = bytes[v * size + size - 1 - b];
  return result;
}

// an array at offset bytes behind a 16 byte aligned address
struct Buffer
{
  explicit Buffer(const QByteArray& bytes, int offset)
    : storage(new quint64[(bytes.size() + offset) / sizeof(quint64) + 2]),
      data(reinterpret_cast<char*>(storage) + offset)
  {
    memcpy(data, bytes.constData(), bytes.size());
  }
  ~Buffer() { delete[] storage; }

  quint64* storage;  // aligned to 8 bytes at least
  char* data;
};

template <typename T>
void checkSwap(int count, int offset)
{
  const int size = static_cast<int>(sizeof(T));
  const QByteArray bytes = pattern(size, count);
  const QByteArray expected = reversed(bytes, size);
  const quint32 matrixSize = static_cast<quint32>(bytes.size());

  // into a new array
  {
    Buffer source(bytes, offset);
    T* result = bswap_matrix<T>(reinterpret_cast<const T*>(source.data), matrixSize);
    QVERIFY(result != NULL);
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(result), bytes.size()), expected);
    QCOMPARE(QByteArray(source.data, bytes.size()), bytes); // the source is unchanged
    delete[] reinterpret_cast<char*>(result);
  }

  // into another array
  {
    Buffer source(bytes, offset);
    Buffer target(QByteArray(bytes.size(), '\0'), offset);
    T* result = bswap_matrix<T>(reinterpret_cast<const T*>(source.data), matrixSize, reinterpret_cast<T*>(target.data));
    QVERIFY(result == reinterpret_cast<T*>(target.data));
    QCOMPARE(QByteArray(target.data, bytes.size()), expected);
  }

  // in place
  {
    Buffer source(bytes, offset);
    T* data = reinterpret_cast<T*>(source.data);
    QVERIFY(bswap_matrix<T>(data, matrixSize, data) == data);
    QCOMPARE(QByteArray(source.data, bytes.size()), expected);

    // swapping twice restores the values
    bswap_matrix<T>(data, matrixSize, data);
    QCOMPARE(QByteArray(source.data, bytes.size()), bytes);
  }
}

} // namespace

class TestBswap : public QObject
{
  Q_OBJECT

  private slots:
    void swap_data();
    void swap();
    void values();
};

void TestBswap::swap_data()
{
  QTest::addColumn<int>("size");
  QTest::addColumn<int>("count");
  QTest::addColumn<int>("offset");

  // even and odd numbers of values (the former implementation swapped pairs),
  // arrays aligned to twice the size of their values and only to their size
  const int sizes[] = { 1, 2, 4, 8 };
  const int counts[] = { 1, 2, 7, 64 };
  for(unsigned int s=0; s < sizeof(sizes) / sizeof(sizes[0]); s++)
    for(unsigned int c=0; c < sizeof(counts) / sizeof(counts[0]); c++)
      for(int aligned=0; aligned < 2; aligned++) {
        const int offset = aligned ? 0 : sizes[s];
        QTest::newRow(qPrintable(QString("%1 bit, %2 values, offset %3").arg(sizes[s] * 8).arg(counts[c]).arg(offset)))
          << sizes[s] << counts[c] << offset;
      }
}

void TestBswap::swap()
{
  QFETCH(int, size);
  QFETCH(int, count);
  QFETCH(int, offset);

  switch(size) {
    case 1: checkSwap<quint8>(count, offset); break;
    case 2: checkSwap<quint16>(count, offset); break;
    case 4: checkSwap<quint32>(count, offset); break;
    case 8: checkSwap<quint64>(count, offset); break;
  }
}

// values of the types used by the readers and writers
void TestBswap::values()
{
  const qint16 shorts[] = { 0x0102, -2, 32767 };
  qint16 s[3];
  bswap_matrix<qint16>(shorts, sizeof(shorts), s);
  QCOMPARE(static_cast<quint16>(s[0]), static_cast<quint16>(0x0201));
  QCOMPARE(static_cast<quint16>(s[1]), static_cast<quint16>(0xfeff));
  QCOMPARE(static_cast<quint16>(s[2]), static_cast<quint16>(0xff7f));

  // a float in big endian byte order: 1.5f is 0x3fc00000
  const quint32 bigEndian = 0x0000c03f;
  float f = 0.0f;
  bswap_matrix<float>(reinterpret_cast<const float*>(&bigEndian), sizeof(f), &f);
  quint32 bits = 0;
  memcpy(&bits, &f, sizeof(bits));
  QCOMPARE(bits, static_cast<quint32>(0x3fc00000));

  const quint64 big = Q_UINT64_C(0x0102030405060708);
  quint64 swapped = 0;
  bswap_matrix<quint64>(&big, sizeof(big), &swapped);
  QCOMPARE(swapped, Q_UINT64_C(0x0807060504030201));
}

QTEST_GUILESS_MAIN(TestBswap)
#include "testBswap.moc"
