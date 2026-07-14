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

#include "CConcordeMainHeaderSinogram.h"
#include "CConcordeMainHeader.h"

#include <rtdebug.h>

CConcordeMainHeaderSinogram::CConcordeMainHeaderSinogram(const QString& fileName)
  : CConcordeMainHeader(fileName)
{
  ENTER();
  LEAVE();
}

CConcordeMainHeaderSinogram::CConcordeMainHeaderSinogram(CConcordeFile* file)
  : CConcordeMainHeader(file)
{
  ENTER();
  LEAVE();
}

unsigned int CConcordeMainHeaderSinogram::frameSize()
{  
  ENTER();
  
  unsigned int framesize = 0;
  unsigned int typesize = 0;
  
  //calculate the frame size: 
  //frame_size = number_of_sinograms*x_dimension*y_dimension*data_type_size
  //number_of_dimensions = sum(sinograms) over all delta_elements

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
  
  for(unsigned int i = 0; i < 11; i++)
    framesize += deltaElements(i);
  
  framesize = framesize*xDimension()*yDimension()*typesize;

  RETURN(framesize);
  return framesize;
}

CMedIOHeader* CConcordeMainHeaderSinogram::clone() const
{
  CConcordeMainHeaderSinogram* pTmp = new CConcordeMainHeaderSinogram(*this);
  return pTmp;
}
