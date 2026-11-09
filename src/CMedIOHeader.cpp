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
#include "CMedIOMetadata.h"
#include "CMedIOMetadataMap.h"
#include "CMedIOData.h"
#include "CECATFile.h"
#include "CPhilipsFile.h"
#include "CConcordeFile.h"
#include "CConcordeMainHeader.h"
#include "CConcordeFrameHeader.h"
#include "CNIFTIFile.h"
#include "CNIFTIMainHeader.h"

#include <rtdebug.h>
#include <iostream>

#include <QScopedPointer>

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CMedIOHeaderPrivate
{
  public:
    CMedIOData* medIOData;
    QJsonObject metadata;   // metadata the format cannot store (see metadata())
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
  m_pData->metadata = src.m_pData->metadata;

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
  copyMetadata(src);
  ENTER();

  convertFrom(&src);

  LEAVE();
  return *this;
}

void CMedIOHeader::copyMetadata(const CMedIOHeader& src)
{
  if(m_pData != src.m_pData)
    m_pData->metadata = src.m_pData->metadata;
}

QJsonObject CMedIOHeader::metadata() const
{
  ENTER();
  const QJsonObject json = MedIOMetadata::metadata(*this, m_pData->metadata);
  LEAVE();
  return json;
}

bool CMedIOHeader::updateMetadata(const QJsonObject& changes)
{
  ENTER();

  // apply the changes to a copy first: nothing is changed if one of them is invalid
  QScopedPointer<CMedIOHeader> copy(clone());
  bool result = (copy.isNull() == false) &&
                MedIOMetadata::update(*copy, copy->m_pData->metadata, changes) &&
                MedIOMetadata::update(*this, m_pData->metadata, changes);

  RETURN(result);
  return result;
}

QJsonValue CMedIOHeader::metadataValue(const QString& keywordOrTag) const
{
  return metadata().value(CMedIOMetadata::normalizedKey(keywordOrTag));
}

QJsonValue CMedIOHeader::metadataValue(quint32 tag) const
{
  return metadataValue(CMedIOMetadata::tagString(tag));
}

bool CMedIOHeader::setMetadataValue(const QString& keywordOrTag, const QJsonValue& value)
{
  QJsonObject change;
  change[CMedIOMetadata::normalizedKey(keywordOrTag)] = value;
  return updateMetadata(change);
}

bool CMedIOHeader::setMetadataValue(quint32 tag, const QJsonValue& value)
{
  return setMetadataValue(CMedIOMetadata::tagString(tag), value);
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

      case CMedIOData::NIFTI:
      {
        CNIFTIFile* niftiFile = static_cast<CNIFTIFile*>(pData);
        CNIFTIMainHeader* mainHeader = NULL;

        // NIfTI only has a main header which carries all information,
        // so it serves as main and sub header at the same time
        if(niftiFile->readMainHeader(mainHeader))
          result = convertFrom(mainHeader, mainHeader);

        delete mainHeader;
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
    case CMedIOHeader::NIFTIMainHeader:
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
