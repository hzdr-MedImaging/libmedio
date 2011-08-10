#include "testCPhilipsFile.h"

#include <CPhilipsFile.h>

#include <iostream>

void TestCPhilipsFile::initTestCase()
{
  m_pInputFile = new CPhilipsFile("../../../philipstestfile.img");
  m_pOutputFile = new CPhilipsFile("output.img");

  QCOMPARE(m_pInputFile->open(QIODevice::ReadOnly), true);
  QCOMPARE(m_pOutputFile->open(QIODevice::WriteOnly), true);
}

void TestCPhilipsFile::testReadMainHeader()
{
  CPhilipsMainHeader* mainHeader = NULL;

  m_pInputFile->readMainHeader(mainHeader);
  
  QVERIFY(mainHeader != NULL);
  QCOMPARE(mainHeader->file_Type(), CPhilipsMainHeader::Image);
  
  delete mainHeader;
}

void TestCPhilipsFile::testGetFileType()
{
  CPhilipsMainHeader::File_Type fileType = CPhilipsMainHeader::Unknown;

  fileType = m_pInputFile->fileType();

  QCOMPARE(fileType, CPhilipsMainHeader::Image);
}

void TestCPhilipsFile::testGetSubHeaderType()
{
  CPhilipsSubHeader::Type subHeaderType = CPhilipsSubHeader::Unknown;

  subHeaderType = m_pInputFile->subHeaderType();

  QCOMPARE(subHeaderType, CPhilipsSubHeader::Image);
}

void TestCPhilipsFile::testCreateEmptyMainHeader()
{
  CPhilipsMainHeader* mainHeader = NULL;

  mainHeader = m_pInputFile->createEmptyMainHeader();

  QVERIFY(mainHeader != NULL);
  QCOMPARE(mainHeader->file_Type(), CPhilipsMainHeader::Unknown);

  delete mainHeader;
}

void TestCPhilipsFile::cleanupTestCase()
{
  m_pInputFile->close();
  delete m_pInputFile;

  m_pOutputFile->close();
  delete m_pOutputFile;
}
