/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio
 
  Copyright (C) 2004-2025 hzdr.de and contributors
 
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

#include "testCPhilipsFile.h"

#include <CPhilipsFile.h>
#include <CPhilipsSubHeaderImage.h>
#include <CPhilipsSubHeaderSyntegra.h>
#include <CPhilipsSubHeaderSinogram.h>
#include <CPhilipsSubHeaderListmode.h>

#include <iostream>

void TestCPhilipsFile::initTestCase()
{
  m_pInputFile = new CPhilipsFile(m_sFileName);
  // m_pOutputFile = new CPhilipsFile("output.img");

  QCOMPARE(m_pInputFile->open(QIODevice::ReadOnly), true);
  // QCOMPARE(m_pOutputFile->open(QIODevice::WriteOnly), true);
}

void TestCPhilipsFile::testReadMainHeader()
{
  CPhilipsMainHeader* mainHeader = NULL;

  m_pInputFile->readMainHeader(mainHeader);
  
  CPhilipsFile o("new.img");
  if(o.open(QIODevice::WriteOnly))
  {
    CPhilipsMainHeader* newHeader = o.createEmptyMainHeader();
    if(!newHeader)
    {
      std::cout << "can't create new main header in destination file" << std::endl;
    }
    else
    {
      *newHeader = *mainHeader;
      if(o.writeMainHeader(*newHeader) == false)
        std::cout << "Can't write main header" << std::endl;
    }
    o.close();
  }
  else
     std::cout << "Can't open file" << std::endl;

  QVERIFY(mainHeader != NULL);
  QVERIFY(mainHeader->filtyp() != CPhilipsMainHeader::Unknown);
  
  delete mainHeader;
}

void TestCPhilipsFile::testGetFileType()
{
  CPhilipsMainHeader::File_Type fileType = CPhilipsMainHeader::Unknown;
  fileType = m_pInputFile->fileType();

  QVERIFY(fileType != CPhilipsMainHeader::Unknown);
}

void TestCPhilipsFile::testGetSubHeaderType()
{
  CPhilipsSubHeader::Type subHeaderType = CPhilipsSubHeader::Unknown;
  subHeaderType = m_pInputFile->subHeaderType();

  QVERIFY(subHeaderType != CPhilipsSubHeader::Unknown);
}

void TestCPhilipsFile::testReadMatrixDataQByteArray()
{
  QByteArray* matrixData = NULL;
  CPhilipsSubHeader* subHeader = NULL;
  bool ok = m_pInputFile->readMatrix(matrixData, subHeader);
 
  if(m_pInputFile->subHeaderType() == CPhilipsSubHeader::Image)
  {
    CPhilipsSubHeaderImage* sHead = static_cast<CPhilipsSubHeaderImage*>(subHeader);
    std::cout << "scale_factor: " << sHead->imgscl();
  }
  else if(m_pInputFile->subHeaderType() == CPhilipsSubHeader::Syntegra)
  {
    CPhilipsSubHeaderSyntegra* sHead = static_cast<CPhilipsSubHeaderSyntegra*>(subHeader);
    std::cout << "scale_factor: " << sHead->imgscl();
  }
  else if(m_pInputFile->subHeaderType() == CPhilipsSubHeader::Sinogram)
  {
    CPhilipsSubHeaderSinogram* sHead = static_cast<CPhilipsSubHeaderSinogram*>(subHeader);
    std::cout << "scale_factor: " << sHead->scnscl();
  }
  else if(m_pInputFile->subHeaderType() == CPhilipsSubHeader::Listmode)
  {
    CPhilipsSubHeaderListmode* sHead = static_cast<CPhilipsSubHeaderListmode*>(subHeader);
    std::cout << "actual_bedpos: " << sHead->actual_bedpos();
  }

  QVERIFY(ok != false);
  QVERIFY(matrixData != NULL);
  QVERIFY(subHeader != NULL);

  delete matrixData;
  delete subHeader;
}

void TestCPhilipsFile::testReadMatrixDataChar()
{
  char* matrixData = NULL;
  unsigned int matrixSize;
  
  bool ok =m_pInputFile->readMatrix(matrixData, matrixSize, 1000, 1);
  std::cout << "matrixSize: " << matrixSize << std::endl;
  
  // QFile o("output.bin");
  // if(o.open(QIODevice::WriteOnly))
  // {

  //   QByteArray bufArray(8192, 0); // write in 8KB chunks
  //   quint16* ptr = (quint16*)matrixData;
  //   for(unsigned int written=0; written < matrixSize;)
  //   {
  //     unsigned int toWrite = matrixSize-written >= 8192 ? 8192 : matrixSize-written;

  //     // check if the curRead value is divide able through our data type 
  //     // ASSERT(toWrite % sizeof(quint16) == 0);

  //     // now that we have our chunk we use a bufferStream to stream
  //     // in the values to it for making sure our data is correctly
  //     // converted regarding to little/big endianess
  //     QDataStream bufStream(&bufArray, QIODevice::WriteOnly);
  //     for(unsigned int i=0; i < toWrite; i+=sizeof(quint16))
  //     {
  //       bufStream << *ptr;
  //       ++ptr;
  //     }

  //     // write out the data from our buffer to the file
  //     if(o.write(bufArray.data(), toWrite) != (qint64)toWrite)
  //     {
  //       break;
  //     }
        
  //     // increase our read counter
  //     written += toWrite;
  //   }


  // }
  // else
  //   std::cout << "Can't open file" << std::endl;

  QVERIFY(ok != false);
  QVERIFY(matrixData != NULL);
  if(matrixData)
    delete [] matrixData;
}

void TestCPhilipsFile::testReadMatrixDataQByteArrayAndSubHeader()
{
  QByteArray* matrixData = NULL;
  CPhilipsSubHeader* subHeader = NULL;
  bool ok = m_pInputFile->readMatrix(matrixData, subHeader, 1000, 1);
  
  QVERIFY(ok != false);
  QVERIFY(matrixData != NULL);
  QVERIFY(subHeader != NULL);
  if(matrixData)
    delete matrixData;
  if(subHeader)
    delete subHeader;
}

void TestCPhilipsFile::testReadMatrixDataCharAndSubHeader()
{
  char* matrixData = NULL;
  unsigned int matrixSize;
  CPhilipsSubHeader* subHeader;
  
  bool ok =m_pInputFile->readMatrix(matrixData, matrixSize, subHeader, 1000, 1);

  QVERIFY(ok != false);
  QVERIFY(matrixData != NULL);
  QVERIFY(subHeader != NULL);
  if(matrixData)
    delete [] matrixData;
}

void TestCPhilipsFile::testCreateEmptyMainHeader()
{
  CPhilipsMainHeader* mainHeader = NULL;

  mainHeader = m_pInputFile->createEmptyMainHeader();

  QVERIFY(mainHeader != NULL);
  QCOMPARE(mainHeader->filtyp(), CPhilipsMainHeader::Unknown);

  if(mainHeader)
    delete mainHeader;
}

void TestCPhilipsFile::cleanupTestCase()
{
  m_pInputFile->close();
  delete m_pInputFile;

  // m_pOutputFile->close();
  // delete m_pOutputFile;
}
