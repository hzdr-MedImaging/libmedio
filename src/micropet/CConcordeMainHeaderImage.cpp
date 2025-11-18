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

#include "CConcordeMainHeaderImage.h"
#include "CConcordeMainHeader.h"

#include <rtdebug.h>

CConcordeMainHeaderImage::CConcordeMainHeaderImage(const QString& fileName)
  : CConcordeMainHeader(fileName)
{
  ENTER();
  LEAVE();
}

CConcordeMainHeaderImage::CConcordeMainHeaderImage(CConcordeFile* file)
  : CConcordeMainHeader(file)
{
  ENTER();
  LEAVE();
}


//  Class: CConcordeMainHeaderImage
//  Method: frameSize
//!
//! @return framesize of a imagevolume in bytes
////////////////////////////////////////////////////////////////////////////////
unsigned int CConcordeMainHeaderImage::frameSize()
{  
  ENTER();
  
  unsigned int framesize = 0;
  unsigned int typesize = 0;

  //calculate the imageframe size: 
  //framesize = x_dimension*y_dimension*z_dimension*data_type_size
  
  switch(dataType())
  {
    case CConcordeMainHeader::IntelShort:
    case CConcordeMainHeader::SunShort:
    {
      typesize = 2;
    }
    break;

    case CConcordeMainHeader::IntelInt:
    case CConcordeMainHeader::IntelFloat:
    case CConcordeMainHeader::IEEEFloat:
    case CConcordeMainHeader::SunInt:
    {
      typesize = 4;
    }
    break;

    case CConcordeMainHeader::Byte:
    case CConcordeMainHeader::UnknownDataType:
    {
      typesize = 1;
    }
    break;
  }
  
  framesize = xDimension()*yDimension()*zDimension()*typesize;

  RETURN(framesize);
  return framesize;
}

CMedIOHeader* CConcordeMainHeaderImage::clone() const
{
  CConcordeMainHeaderImage* pTmp = new CConcordeMainHeaderImage(*this);
  return pTmp;
}
