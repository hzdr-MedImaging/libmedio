/* -*- mode: c++; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 * vim:set ts=2 sw=2 expandtab: *********************************************
 *
 * libmedio - C++ I/O Library for loading/saving medical data formats
 * Copyright (C) 2006-2010 by Jens Langner <Jens.Langner@light-speed.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id$
 *
 **************************************************************************/

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
