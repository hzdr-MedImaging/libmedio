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

//! @file CMedIODataFactory.h
//! @brief contains definition of the class CMedIODataFactory

#ifndef CMEDIODATAFACTORY_H
#define CMEDIODATAFACTORY_H

#ifndef __MEDIO_PRIVATE__
#include <CMedIOData>
#else
#include <CMedIOData.h>
#endif

#include <QString>

//! @class CMedIODataFactory
//! @brief factory class allowing construction of CMedIOData objects 
//! @bug no bugs reported
//! @warning still in development
//!
//! This class is a factory which allows easy construction of different 
//! CMedIOData objects. All kinds of medical data which are implemented
//! can be instancieted by this class.
class CMedIODataFactory
{
  public:
    //! @brief identifies fileformat and returns the type of it
    static CMedIOData::Format identify(const QString& fileName);

    //! @brief identifies fileformat and constructs CMedIOData object
    //! @param file: filename where medical data is located
    //! @return CMedIOData object if format is known otherwise NULL
    static CMedIOData* create(const QString& fileName);
};

#endif
