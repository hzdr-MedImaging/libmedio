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

// Tests of reading Philips .bin files with CPhilipsBinFile. The tests write
// synthetic files: a header of seven big endian 32 bit words (magic number,
// header version 1, header size in words, data type, z, y and x dimension)
// followed by the big endian data.

#include <CPhilipsBinFile.h>

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QtEndian>

static const quint32 BinMagicNumber = 0x7c1e40d4;

// write a .bin file of int16 elements (data type 3) with the given header
// dimensions and data (given in machine byte order)
static bool writeBin(const QString& path, quint32 x, quint32 y, quint32 z, const QVector<qint16>& data)
{
  QFile file(path);
  if(file.open(QIODevice::WriteOnly) == false)
    return false;

  const quint32 header[7] = { BinMagicNumber, 1, 7, 3, z, y, x };
  for(int i=0; i < 7; i++) {
    const quint32 word = qToBigEndian(header[i]);
    if(file.write(reinterpret_cast<const char*>(&word), 4) != 4)
      return false;
  }
  foreach(qint16 value, data) {
    const qint16 v = qToBigEndian(value);
    if(file.write(reinterpret_cast<const char*>(&v), 2) != 2)
      return false;
  }
  return true;
}

class TestPhilipsBinFile : public QObject
{
  Q_OBJECT

  private:
    QTemporaryDir m_tmp;

  private slots:
    void initTestCase() { QVERIFY(m_tmp.isValid()); }
    void readMatrix();
    void sizeOverflow();
};

// the elements are returned in machine byte order
void TestPhilipsBinFile::readMatrix()
{
  QVector<qint16> values;
  for(int i=0; i < 4 * 3 * 2; i++)
    values.append(static_cast<qint16>(i * 300 - 3000));
  const QString path = m_tmp.filePath("small.bin");
  QVERIFY(writeBin(path, 4, 3, 2, values));

  CPhilipsBinFile file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QCOMPARE(file.header().xDim, 4u);
  QCOMPARE(file.header().yDim, 3u);
  QCOMPARE(file.header().zDim, 2u);

  char* data = NULL;
  unsigned int length = 0;
  QVERIFY(file.readMatrix(data, length));
  QCOMPARE(length, 48u);
  for(int i=0; i < values.size(); i++)
    QCOMPARE(reinterpret_cast<const qint16*>(data)[i], values[i]);
  delete[] data;
  file.close();
}

// dimensions whose product overflows 32 bit: 65537 x 65536 int16 elements
// (8 GB) computed with quint32 gave 128 KB, which were returned as data.
// The file is rejected now.
void TestPhilipsBinFile::sizeOverflow()
{
  const QString path = m_tmp.filePath("overflow.bin");
  QVERIFY(writeBin(path, 65537, 65536, 1, QVector<qint16>(65536, 1)));

  CPhilipsBinFile file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));

  char* data = NULL;
  unsigned int length = 0;
  QVERIFY(file.readMatrix(data, length) == false);
  QVERIFY(data == NULL);
  QCOMPARE(length, 0u);
  file.close();
}

QTEST_GUILESS_MAIN(TestPhilipsBinFile)
#include "testPhilipsBinFile.moc"
