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
    void fullTextFields();
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

// ECAT7 text fields filled up to their size have no terminating zero (e.g.
// a study description of 32 characters written by the scanner software):
// they are written and read completely, the getters return them zero
// terminated without the following fields
void TestECATFile::fullTextFields()
{
  const QByteArray description("None|260603_NEMAPhantom_Cu61-LM-");   // 32 characters
  const QByteArray name("Patient^Name^With^32^Characters!");
  QCOMPARE(description.size(), 32);
  QCOMPARE(name.size(), 32);

  const QString path = m_tmp.filePath("text.v");
  {
    CECATFile file(path, CECATMainHeader::ECAT7_Volume16);
    QVERIFY(file.open(QIODevice::WriteOnly));
    CECAT7MainHeader mainHeader(&file, CECATMainHeader::ECAT7_Volume16);
    mainHeader.setNum_Frames(1);
    mainHeader.setNum_Planes(nz);
    mainHeader.setStudy_Description(description.constData());
    mainHeader.setPatient_Name((name + "longer than the field").constData());   // cut to 32 characters
    mainHeader.setPhysician_Name("Physician");
    mainHeader.setAcquisition_Type(CECAT7MainHeader::DynamicEmission);   // non-zero field behind the description
    QCOMPARE(QByteArray(mainHeader.study_Description()), description);
    QCOMPARE(QByteArray(mainHeader.patient_Name()), name);

    CECAT7SubHeaderImage subHeader(&file);
    subHeader.setData_Type(CECATSubHeader::SunShort);
    subHeader.setNum_Dimensions(3);
    subHeader.setX_Dimension(nx);
    subHeader.setY_Dimension(ny);
    subHeader.setZ_Dimension(nz);
    subHeader.setScale_Factor(1.0f);
    QVERIFY(file.writeMainHeader(mainHeader));
    QVERIFY(file.writeSubHeader(subHeader, 1));
    QVERIFY(file.writeMatrix(QByteArray(nx * ny * nz * 2, '\0'), 1));
    file.close();
  }

  // the fields in the file: Patient_Name at byte 182, Study_Description at
  // byte 296 (ECAT7 main header), both 32 bytes
  QFile raw(path);
  QVERIFY(raw.open(QIODevice::ReadOnly));
  const QByteArray header = raw.read(512);
  QCOMPARE(header.mid(182, 32), name);
  QCOMPARE(header.mid(296, 32), description);
  raw.close();

  CECATFile file(path);
  QVERIFY(file.open(QIODevice::ReadOnly));
  CECATMainHeader* h = NULL;
  QVERIFY(file.readMainHeader(h) && h != NULL);
  QScopedPointer<CECATMainHeader> guard(h);
  const CECAT7MainHeader* mainHeader = static_cast<const CECAT7MainHeader*>(h);
  const char* studyDescription = mainHeader->study_Description();
  const char* patientName = mainHeader->patient_Name();
  QCOMPARE(QByteArray(studyDescription), description);   // both pointers valid together
  QCOMPARE(QByteArray(patientName), name);
  QCOMPARE(QByteArray(mainHeader->physician_Name()), QByteArray("Physician"));
  QCOMPARE(h->attributes().value("StudyDescription").toString(), QString::fromLatin1(description));
  file.close();
}

QTEST_GUILESS_MAIN(TestECATFile)
#include "testECATFile.moc"
