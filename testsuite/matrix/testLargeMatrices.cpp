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

// Single matrices of 2 to 4 GB, which the methods with a length of type
// unsigned int can describe, but a QByteArray cannot hold:
//
// - an ECAT7 matrix of 2.2 GB (a sparse file whose directory entry is
//   enlarged) is rejected by readMatrix(QByteArray*&, ...); it was returned
//   as empty QByteArray before (QByteArray(data, size) with a negative size
//   treats the data as nul-terminated string)
// - a NIfTI voxel matrix of 2.2 GB in the other byte order is written from
//   a char buffer; the byte swapping copied it into a QByteArray of a
//   negative size before (heap overflow)
//
// The test needs about 2.3 GB of memory and 2.2 GB of temporary disk space;
// it is registered with the CTest label "large" (ctest -LE large skips it).

#include <CECATFile.h>
#include <CECATMainHeader.h>
#include <CECATSubHeader.h>
#include <CECAT7MainHeader.h>
#include <CECAT7SubHeaderImage.h>
#include <CNIFTIFile.h>
#include <CNIFTIMainHeader.h>
#include <CNIFTI1MainHeader.h>

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QtEndian>

#include <limits>

class TestLargeMatrices : public QObject
{
  Q_OBJECT

  private:
    QTemporaryDir m_tmp;

    QString tmpFile(const QString& name) const { return m_tmp.filePath(name); }

  private slots:
    void initTestCase() { QVERIFY(m_tmp.isValid()); }
    void ecatMatrixLargerThan2GB();
    void niftiSwappedMatrixLargerThan2GB();
};

void TestLargeMatrices::ecatMatrixLargerThan2GB()
{
  // ECAT7 file with one small int16 frame
  const QString path = tmpFile("large.v");
  {
    CECATFile file(path, CECATMainHeader::ECAT7_Volume16);
    QVERIFY(file.open(QIODevice::WriteOnly));
    CECAT7MainHeader mainHeader(&file, CECATMainHeader::ECAT7_Volume16);
    mainHeader.setNum_Frames(1);
    mainHeader.setNum_Planes(2);
    QVERIFY(file.writeMainHeader(mainHeader));

    CECAT7SubHeaderImage subHeader(&file);
    subHeader.setData_Type(CECATSubHeader::SunShort);
    subHeader.setNum_Dimensions(3);
    subHeader.setX_Dimension(4);
    subHeader.setY_Dimension(3);
    subHeader.setZ_Dimension(2);
    subHeader.setScale_Factor(1.0f);
    QVERIFY(file.writeSubHeader(subHeader, 1));
    QVERIFY(file.writeMatrix(QByteArray(4 * 3 * 2 * 2, '\1'), 1));
    file.close();
  }

  // enlarge the matrix of the first directory entry (block 2 of the file:
  // 4 header words, then matrix id, first block, last block and status per
  // entry, big endian) to 2.2 GB and the file accordingly (sparse)
  const qint64 matrixSize = 4296000LL * 512;   // 2,199,552,000 bytes
  QVERIFY(matrixSize > std::numeric_limits<int>::max());
  {
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadWrite));
    QVERIFY(file.seek(512 + 16 + 4));
    quint32 words[2];
    QCOMPARE(file.read(reinterpret_cast<char*>(words), 8), qint64(8));
    const quint32 first = qFromBigEndian(words[0]);
    QVERIFY(qFromBigEndian(words[1]) > first);

    // matrix size = (last - first) blocks (the sub header is one block)
    const quint32 last = qToBigEndian(static_cast<quint32>(first + matrixSize / 512));
    QVERIFY(file.seek(512 + 16 + 8));
    QCOMPARE(file.write(reinterpret_cast<const char*>(&last), 4), qint64(4));
    QVERIFY(file.resize((first + matrixSize / 512) * 512));
    file.close();
  }

  CECATFile file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));

  // up to 4 GB with a length of type unsigned int
  char* raw = NULL;
  unsigned int length = 0;
  QVERIFY(file.readMatrix(raw, length, 1));
  QCOMPARE(static_cast<qint64>(length), matrixSize);
  delete[] raw;

  // but not as QByteArray
  QByteArray* matrix = NULL;
  CECATSubHeader* subHeader = NULL;
  QVERIFY(file.readMatrix(matrix, subHeader, 1) == false);
  QVERIFY(matrix == NULL);
  QVERIFY(subHeader == NULL);
  file.close();

  QVERIFY(QFile::remove(path));
}

void TestLargeMatrices::niftiSwappedMatrixLargerThan2GB()
{
  // 2 volumes of 1024 x 1024 x 525 int16 voxels (2.2 GB) in big endian order
  const int nx = 1024, ny = 1024, nz = 525, frames = 2;
  const qint64 voxels = static_cast<qint64>(nx) * ny * nz;
  const qint64 size = voxels * frames * 2;
  QVERIFY(size > std::numeric_limits<int>::max());

  const QString path = tmpFile("swapped.nii");
  {
    CNIFTIFile file(path, CNIFTIMainHeader::NIFTI1);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QScopedPointer<CNIFTIMainHeader> header(file.createEmptyHeader());
    CNIFTI1MainHeader* h = static_cast<CNIFTI1MainHeader*>(header.data());
    const short dims[8] = { 4, nx, ny, nz, frames, 1, 1, 1 };
    for(short i=0; i < 8; i++) {
      h->setDim(i, dims[i]);
      h->setPix_Dim(i, 1.0);
    }
    h->setDataType(4);   // DT_INT16
    h->setBit_Pix(16);
    h->setScl_Slope(1.0);
    h->setByteOrder(QSysInfo::ByteOrder == QSysInfo::BigEndian ? QSysInfo::LittleEndian : QSysInfo::BigEndian);
    QVERIFY(file.writeMainHeader(*header));

    char* data = new char[size];
    qint16* values = reinterpret_cast<qint16*>(data);
    for(qint64 i=0; i < voxels * frames; i++)
      values[i] = static_cast<qint16>(i % 30011 - 15000);
    const bool written = file.writeMatrix(data, static_cast<unsigned int>(size));
    delete[] data;
    QVERIFY(written);
    file.close();
  }

  // read back frame by frame in machine byte order
  CNIFTIFile file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));
  for(int f=0; f < frames; f++) {
    QByteArray* matrixPtr = NULL;
    QVERIFY(file.readMatrix(matrixPtr, static_cast<short>(f + 1)) && matrixPtr != NULL);
    QScopedPointer<QByteArray> matrix(matrixPtr);
    QCOMPARE(static_cast<qint64>(matrix->size()), voxels * 2);

    const qint16* values = reinterpret_cast<const qint16*>(matrix->constData());
    for(qint64 i=0; i < voxels; i++) {
      const qint16 expected = static_cast<qint16>((i + f * voxels) % 30011 - 15000);
      if(values[i] != expected)
        QFAIL(qPrintable(QString("frame %1, voxel %2: %3 instead of %4").arg(f + 1).arg(i).arg(values[i]).arg(expected)));
    }
  }
  file.close();

  QVERIFY(QFile::remove(path));
}

QTEST_GUILESS_MAIN(TestLargeMatrices)
#include "testLargeMatrices.moc"
