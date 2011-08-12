#include "testCPhilipsSubHeaderSinogram.h"

#include <CPhilipsFile.h>
#include <CPhilipsSubHeaderSinogram.h>

#include <iostream>

void TestCPhilipsSubHeaderSinogram::initTestCase()
{
  m_pInputFile = new CPhilipsFile("../../../philipstestsinogram.scn");
  //m_pOutputFile = new CPhilipsFile("output.img");

  QCOMPARE(m_pInputFile->open(QIODevice::ReadOnly), true);
  //QCOMPARE(m_pOutputFile->open(QIODevice::WriteOnly), true);
}

void TestCPhilipsSubHeaderSinogram::testLoad()
{
  CPhilipsSubHeader *subHeader = NULL;

  QCOMPARE(m_pInputFile->readSubHeader(subHeader, 2, 1, 1), true);
  QVERIFY(subHeader != NULL);
}

void TestCPhilipsSubHeaderSinogram::cleanupTestCase()
{
  m_pInputFile->close();
  delete m_pInputFile;

  // m_pOutputFile->close();
  // delete m_pOutputFile;
}
