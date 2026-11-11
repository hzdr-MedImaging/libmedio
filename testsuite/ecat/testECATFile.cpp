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

// Tests of reading synthetic ECAT7 files with CECATFile.

#include <CECATFile.h>
#include <CECATMainHeader.h>
#include <CECATSubHeader.h>
#include <CECAT7MainHeader.h>
#include <CECAT7SubHeaderImage.h>
#include <CMedIOVolume.h>

#include <QtTest/QtTest>
#include <QTemporaryDir>

// 64 x 64 x 2 int16 voxels (16 KB, 32 blocks) with value = linear index
static const int nx = 64, ny = 64, nz = 2;

static bool writeEcat(const QString& path)
{
  CECATFile file(path, CECATMainHeader::ECAT7_Volume16);
  if(file.open(QIODevice::WriteOnly) == false)
    return false;

  CECAT7MainHeader mainHeader(&file, CECATMainHeader::ECAT7_Volume16);
  mainHeader.setNum_Frames(1);
  mainHeader.setNum_Planes(nz);
  mainHeader.setCalibration_Factor(1.0f);

  CECAT7SubHeaderImage subHeader(&file);
  subHeader.setData_Type(CECATSubHeader::SunShort);
  subHeader.setNum_Dimensions(3);
  subHeader.setX_Dimension(nx);
  subHeader.setY_Dimension(ny);
  subHeader.setZ_Dimension(nz);
  subHeader.setScale_Factor(1.0f);

  QByteArray matrix(nx * ny * nz * 2, '\0');
  for(int i=0; i < nx * ny * nz; i++)
    reinterpret_cast<qint16*>(matrix.data())[i] = static_cast<qint16>(i);

  const bool result = file.writeMainHeader(mainHeader) &&
                      file.writeSubHeader(subHeader, 1) &&
                      file.writeMatrix(matrix, 1);
  file.close();
  return result;
}

class TestECATFile : public QObject
{
  Q_OBJECT

  private:
    QTemporaryDir m_tmp;

  private slots:
    void initTestCase() { QVERIFY(m_tmp.isValid()); }
    void readMatrix();
    void truncatedFile();
};

// the matrix is returned in machine byte order
void TestECATFile::readMatrix()
{
  const QString path = m_tmp.filePath("complete.v");
  QVERIFY(writeEcat(path));

  CECATFile file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QByteArray* data = NULL;
  CECATSubHeader* subHeader = NULL;
  QVERIFY(file.readMatrix(data, subHeader, 1));
  QScopedPointer<QByteArray> dataGuard(data);
  QScopedPointer<CECATSubHeader> subHeaderGuard(subHeader);
  QVERIFY(data->size() >= nx * ny * nz * 2);
  for(int i=0; i < nx * ny * nz; i++)
    QCOMPARE(reinterpret_cast<const qint16*>(data->constData())[i], static_cast<qint16>(i));
  file.close();
}

// a file truncated within the matrix data: readMatrix() reported success
// with partly uninitialized data before
void TestECATFile::truncatedFile()
{
  const QString path = m_tmp.filePath("truncated.v");
  QVERIFY(writeEcat(path));
  {
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadWrite));
    QVERIFY(file.size() > 4096);
    QVERIFY(file.resize(file.size() - 4096 + 100));
    file.close();
  }

  CECATFile file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));

  char* raw = NULL;
  unsigned int length = 0;
  QVERIFY(file.readMatrix(raw, length, 1) == false);
  QVERIFY(raw == NULL);
  QCOMPARE(length, 0u);

  QByteArray* data = NULL;
  CECATSubHeader* subHeader = NULL;
  QVERIFY(file.readMatrix(data, subHeader, 1) == false);
  QVERIFY(data == NULL);
  delete subHeader;

  CMedIOVolume volume;
  QVERIFY(file.readVolume(volume, 1) == false);
  file.close();
}

QTEST_GUILESS_MAIN(TestECATFile)
#include "testECATFile.moc"
