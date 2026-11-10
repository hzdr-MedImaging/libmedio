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
//
// Also tests the macros BSWAP_16/32/64/FLT/DBL(x), which swap a single value
// in place (also a member of a packed structure, as used for the Philips
// headers), and bswap_flt()/bswap_dbl().

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

// the bytes of a value
template <typename T>
QByteArray bytesOf(const T& value)
{
  return QByteArray(reinterpret_cast<const char*>(&value), sizeof(value));
}

#pragma pack(1)
struct Packed
{
  char c;      // the following members are not aligned
  qint16 s;
  qint32 i;
  quint64 l;
  float f;
  double d;
};
#pragma pack()

} // namespace

class TestBswap : public QObject
{
  Q_OBJECT

  private slots:
    void swap_data();
    void swap();
    void values();
    void macros();
    void floats();
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

// BSWAP_X(x) swaps x in place and has the swapped value
void TestBswap::macros()
{
  qint16 s = 0x0102;
  QCOMPARE(BSWAP_16(s), static_cast<qint16>(0x0201));
  QCOMPARE(s, static_cast<qint16>(0x0201));

  quint16 us = 0xfeff;
  BSWAP_16(us);
  QCOMPARE(us, static_cast<quint16>(0xfffe));

  qint32 i = 0x01020304;
  QCOMPARE(BSWAP_32(i), static_cast<qint32>(0x04030201));
  QCOMPARE(i, static_cast<qint32>(0x04030201));

  quint64 l = Q_UINT64_C(0x0102030405060708);
  QCOMPARE(BSWAP_64(l), Q_UINT64_C(0x0807060504030201));
  QCOMPARE(l, Q_UINT64_C(0x0807060504030201));

  // 1.5f is 0x3fc00000, 1.5 is 0x3ff8000000000000
  float f = 1.5f;
  const QByteArray fBytes = bytesOf(f);
  BSWAP_FLT(f);
  QCOMPARE(bytesOf(f), reversed(fBytes, sizeof(f)));
  quint32 fBits = 0;
  memcpy(&fBits, &f, sizeof(fBits));
  QCOMPARE(fBits, static_cast<quint32>(0x0000c03f));
  QCOMPARE(BSWAP_FLT(f), 1.5f);
  QCOMPARE(f, 1.5f);

  double d = 1.5;
  const QByteArray dBytes = bytesOf(d);
  BSWAP_DBL(d);
  QCOMPARE(bytesOf(d), reversed(dBytes, sizeof(d)));
  quint64 dBits = 0;
  memcpy(&dBits, &d, sizeof(dBits));
  QCOMPARE(dBits, Q_UINT64_C(0x000000000000f83f));
  QCOMPARE(BSWAP_DBL(d), 1.5);
  QCOMPARE(d, 1.5);

  // members of a packed structure
  Packed p;
  memset(&p, 0, sizeof(p));
  p.s = 0x0102;
  p.i = 0x01020304;
  p.l = Q_UINT64_C(0x0102030405060708);
  p.f = -2.25f;
  p.d = -2.25;
  const QByteArray before = bytesOf(p);
  BSWAP_16(p.s);
  BSWAP_32(p.i);
  BSWAP_64(p.l);
  BSWAP_FLT(p.f);
  BSWAP_DBL(p.d);
  const QByteArray after = bytesOf(p);
  QCOMPARE(after.mid(0, 1), before.mid(0, 1));
  QCOMPARE(after.mid(1, 2), reversed(before.mid(1, 2), 2));
  QCOMPARE(after.mid(3, 4), reversed(before.mid(3, 4), 4));
  QCOMPARE(after.mid(7, 8), reversed(before.mid(7, 8), 8));
  QCOMPARE(after.mid(15, 4), reversed(before.mid(15, 4), 4));
  QCOMPARE(after.mid(19, 8), reversed(before.mid(19, 8), 8));
  BSWAP_FLT(p.f);
  BSWAP_DBL(p.d);
  const float pf = p.f; // QCOMPARE() would bind a reference to the unaligned member
  const double pd = p.d;
  QCOMPARE(pf, -2.25f);
  QCOMPARE(pd, -2.25);
}

// bswap_flt() and bswap_dbl() return the swapped value and keep the argument
void TestBswap::floats()
{
  // 0x3fc00000 (1.5f) swapped is 0x0000c03f, and back
  const quint32 fIn = 0x3fc00000;
  float f;
  memcpy(&f, &fIn, sizeof(f));
  const float fSwapped = bswap_flt(f);
  quint32 fOut = 0;
  memcpy(&fOut, &fSwapped, sizeof(fOut));
  QCOMPARE(fOut, static_cast<quint32>(0x0000c03f));
  QCOMPARE(f, 1.5f);
  QCOMPARE(bswap_flt(fSwapped), 1.5f);

  // 0x400921fb54442d18 (pi) swapped is 0x182d4454fb210940
  const quint64 dIn = Q_UINT64_C(0x400921fb54442d18);
  double d;
  memcpy(&d, &dIn, sizeof(d));
  const double dSwapped = bswap_dbl(d);
  quint64 dOut = 0;
  memcpy(&dOut, &dSwapped, sizeof(dOut));
  QCOMPARE(dOut, Q_UINT64_C(0x182d4454fb210940));
  QCOMPARE(bswap_dbl(dSwapped), d);
}

QTEST_GUILESS_MAIN(TestBswap)
#include "testBswap.moc"
