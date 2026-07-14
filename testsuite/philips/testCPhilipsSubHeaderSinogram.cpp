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

#include "testCPhilipsSubHeaderSinogram.h"

#include <CPhilipsFile.h>
#include <CPhilipsSubHeaderSinogram.h>

#include <iostream>

void TestCPhilipsSubHeaderSinogram::initTestCase()
{
  m_pInputFile = new CPhilipsFile(m_sFileName);
  //m_pOutputFile = new CPhilipsFile("output.img");

  QCOMPARE(m_pInputFile->open(QIODevice::ReadOnly), true);
  //QCOMPARE(m_pOutputFile->open(QIODevice::WriteOnly), true);
}

void TestCPhilipsSubHeaderSinogram::testLoad()
{
  CPhilipsSubHeader *subHeader = NULL;

  QCOMPARE(m_pInputFile->readSubHeader(subHeader), true);
  QVERIFY(subHeader != NULL);
}

void TestCPhilipsSubHeaderSinogram::cleanupTestCase()
{
  m_pInputFile->close();
  delete m_pInputFile;

  // m_pOutputFile->close();
  // delete m_pOutputFile;
}
