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

//! @file CConcordeMainHeaderSinogram.h
//! @brief contains definition of the class CConcordeMainHeaderSinogram

#ifndef CCONCORDEMAINHEADERSINOGRAM_H
#define CCONCORDEMAINHEADERSINOGRAM_H

#include <QString>

#ifndef __MEDIO_PRIVATE__
#include <CConcordeMainHeader>
#else
#include <CConcordeMainHeader.h>
#endif

// forward declarations
class CConcordeFile;
class CMedIOHeader;

class CConcordeMainHeaderSinogram : public CConcordeMainHeader
{
  public:
    CConcordeMainHeaderSinogram(const QString& fileName);
    CConcordeMainHeaderSinogram(CConcordeFile* file = NULL);

    unsigned int frameSize();

    // clone methods
    CMedIOHeader* clone() const;
};

#endif
