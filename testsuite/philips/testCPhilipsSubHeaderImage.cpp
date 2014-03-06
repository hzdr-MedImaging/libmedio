#include "testCPhilipsSubHeaderImage.h"

#include <CPhilipsFile.h>
#include <CPhilipsSubHeaderImage.h>

#include <iostream>

void TestCPhilipsSubHeaderImage::initTestCase()
{
  m_pInputFile = new CPhilipsFile(m_sFileName);
  //m_pOutputFile = new CPhilipsFile("output.img");

  QCOMPARE(m_pInputFile->open(QIODevice::ReadOnly), true);
  //QCOMPARE(m_pOutputFile->open(QIODevice::WriteOnly), true);
}

void TestCPhilipsSubHeaderImage::testLoad()
{
  CPhilipsSubHeader *subHeader = NULL;

  QCOMPARE(m_pInputFile->readSubHeader(subHeader, 960), true);
  QVERIFY(subHeader != NULL);
}

void TestCPhilipsSubHeaderImage::cleanupTestCase()
{
  m_pInputFile->close();
  delete m_pInputFile;

  // m_pOutputFile->close();
  // delete m_pOutputFile;
}
