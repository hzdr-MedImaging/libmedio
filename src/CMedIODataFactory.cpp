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

//! @file CMedIODataFactory.cpp
//! @brief contains the implementation of the class CMedIODataFactory

#include "CMedIODataFactory.h"

#include "CECATFile.h"
#include "CMedIOData.h"
#include "CConcordeFile.h"
#include "CConcordeSinogram.h"
#include "CConcordeImage.h"
#include "CPhilipsFile.h"

#include <rtdebug.h>

CMedIOData::Format CMedIODataFactory::identify(const QString& fileName)
{
  ENTER();

  CMedIOData::Format result;

  if(CECATFile::isOfType(fileName))
    result = CMedIOData::ECAT;
  else if(CPhilipsFile::isOfType(fileName))
    result = CMedIOData::Philips;
  else if(CConcordeFile::isOfType(fileName))
    result = CMedIOData::ConcordeMicropet;
  else
    result = CMedIOData::Unknown;  

  RETURN(result);
  return result;
}

//  Class: CMedIODataFactory
//  method: create
//!
//! This method tries to identify the medical data format of the given file.
//! So it allows an easy handling of files with the medio library.
//! 
//! @param file: filename where medical data is located
//! @return CMedIOData object if format is known otherwise NULL
////////////////////////////////////////////////////////////////////////////////
CMedIOData* CMedIODataFactory::create(const QString& fileName)
{
  ENTER();
  CMedIOData* mData;

  // here we check for our different main data formats 
  if((mData = CECATFile::createFromFile(fileName)))
    D("ECAT format found");
  else if((mData = CConcordeFile::createFromFile(fileName)))
    D("Concorde microPET format found");
  else if((mData = CPhilipsFile::createFromFile(fileName)))
    D("Philips file found");
  else
  {
    E("Can not identify file format");
    mData = NULL;
  }
  
  RETURN(mData);
  return mData;
}
