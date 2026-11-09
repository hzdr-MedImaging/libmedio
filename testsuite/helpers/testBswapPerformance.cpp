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

// Benchmark of bswap_matrix() (src/include/bswap.h): the time to swap the
// byte order of arrays of 8, 16, 32 and 64 bit values of 16 KiB (first level
// cache), 1 MiB (second level cache) and 32 MiB (main memory), into a new
// array (incl. its allocation, as the ECAT7 and Philips readers do), into
// another array and in place, for arrays aligned to 64 bytes and arrays only
// aligned to the size of their values. memcpy() of the same arrays is
// measured as reference for the memory bandwidth. The results are reported
// by QBENCHMARK (time per call); every row also checks the swapped bytes.
//
// It is built without sanitizers (unlike testBswap) and with the
// optimization of the build type, so only Release builds give meaningful
// times. Run it with e.g. "testBswapPerformance -minimumtotal 200" for more
// stable values, or "-csv" for a table.

#include <QtGlobal>
#include <QByteArray>
#include <QtTest/QtTest>

#include "bswap.h"

#include <cstring>

namespace {

// make the compiler assume that the memory behind p is used, so that it
// cannot drop the work measured (e.g. a swapped array that is deleted)
static inline void escape(const void* p)
{
#if defined(__GNUC__)
  asm volatile("" : : "g"(p) : "memory");
#else
  static const void* volatile sink = NULL;
  sink = p;
#endif
}

// an array of bytes at offset bytes behind a 64 byte aligned address
struct Buffer
{
  Buffer(qint64 size, int offset)
    : storage(new char[size + offset + 64]),
      data(storage + (64 - reinterpret_cast<quintptr>(storage) % 64) % 64 + offset)
  {
    for(qint64 i=0; i < size; i++)
      data[i] = static_cast<char>(i * 7 + 1);
  }
  ~Buffer() { delete[] storage; }

  char* storage;
  char* data;
};

enum Mode { NewArray, OtherArray, InPlace, Memcpy };

template <typename T>
void run(qint64 size, int offset, Mode mode)
{
  Buffer source(size, offset);
  Buffer target(size, offset);
  const QByteArray original(source.data, static_cast<int>(size));
  const quint32 matrixSize = static_cast<quint32>(size);
  const T* src = reinterpret_cast<const T*>(source.data);
  T* dst = reinterpret_cast<T*>(target.data);
  T* inPlace = reinterpret_cast<T*>(source.data);

  switch(mode) {
    case NewArray:
      QBENCHMARK {
        T* result = bswap_matrix<T>(src, matrixSize);
        escape(result);
        delete[] reinterpret_cast<char*>(result);
      }
    break;

    case OtherArray:
      QBENCHMARK {
        bswap_matrix<T>(src, matrixSize, dst);
        escape(dst);
      }
    break;

    case InPlace: {
      // an even number of swaps restores the source
      int swaps = 0;
      QBENCHMARK {
        bswap_matrix<T>(inPlace, matrixSize, inPlace);
        escape(inPlace);
        swaps++;
      }
      if(swaps % 2 != 0)
        bswap_matrix<T>(inPlace, matrixSize, inPlace);
    } break;

    case Memcpy:
      QBENCHMARK {
        memcpy(target.data, source.data, size);
        escape(target.data);
      }
    break;
  }

  // the result has to be right as well
  QCOMPARE(QByteArray(source.data, static_cast<int>(size)), original);
  if(mode == OtherArray || mode == Memcpy) {
    QByteArray expected(original);
    if(mode == OtherArray) {
      for(qint64 v=0; v < size / static_cast<qint64>(sizeof(T)); v++)
        for(int b=0; b < static_cast<int>(sizeof(T)); b++)
          expected[static_cast<int>(v * sizeof(T) + b)] = original[static_cast<int>(v * sizeof(T) + sizeof(T) - 1 - b)];
    }
    QCOMPARE(QByteArray(target.data, static_cast<int>(size)), expected);
  }
}

} // namespace

Q_DECLARE_METATYPE(Mode)

class TestBswapPerformance : public QObject
{
  Q_OBJECT

  private slots:
    void swap_data();
    void swap();
};

void TestBswapPerformance::swap_data()
{
  QTest::addColumn<int>("bits");
  QTest::addColumn<qint64>("size");
  QTest::addColumn<Mode>("mode");
  QTest::addColumn<int>("offset");

  const qint64 sizes[] = { 16 * 1024, 1024 * 1024, 32 * 1024 * 1024 };
  const char* sizeNames[] = { "16 KiB", "1 MiB", "32 MiB" };
  const int bits[] = { 8, 16, 32, 64 };

  for(int s=0; s < 3; s++) {
    QTest::newRow(qPrintable(QString("memcpy, %1").arg(sizeNames[s]))) << 8 << sizes[s] << Memcpy << 0;
    for(int b=0; b < 4; b++) {
      const QString name = QString("%1 bit, %2").arg(bits[b]).arg(sizeNames[s]);
      QTest::newRow(qPrintable(name + ", new array"))   << bits[b] << sizes[s] << NewArray << 0;
      QTest::newRow(qPrintable(name + ", other array")) << bits[b] << sizes[s] << OtherArray << 0;
      QTest::newRow(qPrintable(name + ", in place"))    << bits[b] << sizes[s] << InPlace << 0;
      if(bits[b] < 64) {
        // aligned to the size of the values only (e.g. a 16 bit array at an odd multiple of 2)
        QTest::newRow(qPrintable(name + ", in place, offset " + QString::number(bits[b] / 8))) << bits[b] << sizes[s] << InPlace << bits[b] / 8;
      }
    }
  }
}

void TestBswapPerformance::swap()
{
  QFETCH(int, bits);
  QFETCH(qint64, size);
  QFETCH(Mode, mode);
  QFETCH(int, offset);

  switch(bits) {
    case 8:  run<quint8>(size, offset, mode); break;
    case 16: run<quint16>(size, offset, mode); break;
    case 32: run<quint32>(size, offset, mode); break;
    case 64: run<quint64>(size, offset, mode); break;
  }
}

QTEST_GUILESS_MAIN(TestBswapPerformance)
#include "testBswapPerformance.moc"
