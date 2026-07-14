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

//! @file CMedIOHeader.cpp
//! @brief contains the implementation of the class CMedIOHeader

#include "CMedIOHeader.h"
#include "CMedIOData.h"
#include "CECATFile.h"
#include "CPhilipsFile.h"
#include "CConcordeFile.h"
#include "CConcordeMainHeader.h"
#include "CConcordeFrameHeader.h"

#include <rtdebug.h>
#include <iostream>

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CMedIOHeaderPrivate
{
  public:
    CMedIOData* medIOData;
};
 
CMedIOHeader::CMedIOHeader()
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CMedIOHeaderPrivate();
  m_pData->medIOData = NULL;

  LEAVE();
}

CMedIOHeader::CMedIOHeader(const CMedIOHeader& src)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CMedIOHeaderPrivate();
  m_pData->medIOData = src.medIOData();

  LEAVE();
}

CMedIOHeader::CMedIOHeader(CMedIOData* data)
{ 
  ENTER();

  // allocate data from our private instance class
  m_pData = new CMedIOHeaderPrivate();

  // assign the mediodata pointer to our private class
  m_pData->medIOData = data;
 
  LEAVE();
}

//  Class: CMedIOHeader
//  Destructor:  CMedIOHeader
//!
//! destructucts a CMedIOHeader object.
//! 
////////////////////////////////////////////////////////////////////////////////
CMedIOHeader::~CMedIOHeader()
{
  ENTER();

  delete m_pData;

  LEAVE();
}

CMedIOHeader& CMedIOHeader::operator=(const CMedIOHeader& src)
{
  ENTER();

  convertFrom(&src);

  LEAVE();
  return *this;
}

CMedIOData* CMedIOHeader::medIOData(void) const
{
  return m_pData->medIOData;
}

void CMedIOHeader::setMedIOData(CMedIOData* data)
{
  ENTER();

  m_pData->medIOData = data;
  SHOWPOINTER(m_pData->medIOData);
  
  LEAVE();
}

int CMedIOHeader::userRtti() const
{
  ENTER();
 
  RETURN(-1);
  return -1;
}

bool CMedIOHeader::convertFrom(CMedIOData* pData)
{
  ENTER();
  bool result = false;

  // check if the file opened for reading, if not do it
  if(pData->isReadable())
  {
    switch(pData->dataFormat())
    {
      case CMedIOData::ECAT:
      {
        CECATFile* ecatFile = static_cast<CECATFile*>(pData);
        CECATMainHeader* mainHeader;
        CECATSubHeader* subHeader;

        // read the main and sub header
        ecatFile->readMainHeader(mainHeader);
        ecatFile->readSubHeader(subHeader);

        // convert in one operation
        if(isMainHeader())
          convertFrom(mainHeader, subHeader);
        else
          convertFrom(subHeader, mainHeader);

        delete mainHeader;
        delete subHeader;

        result = true;
      }
      break;

      case CMedIOData::Philips:
      {
        CPhilipsFile* philipsFile = static_cast<CPhilipsFile*>(pData);
        CPhilipsMainHeader* mainHeader;
        CPhilipsSubHeader* subHeader;

        // read the main and subheader
        philipsFile->readMainHeader(mainHeader);
        philipsFile->readSubHeader(subHeader);

        // convert in one operation
        if(isMainHeader())
          convertFrom(mainHeader, subHeader);
        else
          convertFrom(subHeader, mainHeader);

        delete mainHeader;
        delete subHeader;

        result = true;
      }
      break;

      case CMedIOData::ConcordeMicropet:
      { 
        CConcordeFile* concordeFile = static_cast<CConcordeFile*>(pData);
        CConcordeMainHeader* mainHeader;
        CConcordeFrameHeader* subHeader;

        // read the main and subheader
        concordeFile->readMainHeader(mainHeader);
        concordeFile->readSubHeader(subHeader, 1);

        // convert in one operation
        if(isMainHeader())
          convertFrom(mainHeader, subHeader);
        else
          convertFrom(subHeader, mainHeader);

        delete mainHeader;
        delete subHeader;

        result = true;
      }
      break;

      case CMedIOData::Unknown:
        result = false;
      break;

      case CMedIOData::PhilipsBin:
        E("medio datatype %d conversion not implemented!", pData->dataFormat());
        result = false;
      break;
    }
  }

  RETURN(result);
  return result;
}

bool CMedIOHeader::isMainHeader(void) const
{
  ENTER();
  bool result = false;

  switch(headerFormat())
  {
    case CMedIOHeader::Unknown:
      // nothing
    break;

    case CMedIOHeader::ConcordeMicroPetMainHeader:
    case CMedIOHeader::ECATMainHeader:
    case CMedIOHeader::PhilipsMainHeader:
      result = true;
    break;
    
    case CMedIOHeader::ConcordeMicroPetFrameHeader:
    case CMedIOHeader::ECATSubHeader:
    case CMedIOHeader::PhilipsSubHeader:
    case CMedIOHeader::PhilipsListviewHeader:
      result = false;
    break;
  }
  
  RETURN(result);
  return result;
}
