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

//! @file CMedIOData.cpp
//! @brief contains the implementation of the class CMedIOData

#include "CMedIOData.h"
#include "CMedIOHeader.h"

#include <QString>

#include <rtdebug.h>

//  Class: CMedIOData
//  Constructor: CMedIOData
//!
//! constructs a CMedIOData object
//!
////////////////////////////////////////////////////////////////////////////////
CMedIOData::CMedIOData(const QString& fileName)
  : QFile(fileName)
{
  m_iErrStatus = MERR_Ok;
}

//  Class: CMedIOData
//  Destructor:  CMedIOData
//!
//! destructucts a CMedIOData object.
//! 
////////////////////////////////////////////////////////////////////////////////
CMedIOData::~CMedIOData()
{

}

//! @brief for querying error status information
//! @return integer value for the specific error reason
int CMedIOData::errorStatus(void) const
{ 
  return m_iErrStatus;
}

int CMedIOData::userRtti() const
{
  ENTER();

  RETURN(-1);
  return -1;
}
