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

#include "CPhilipsSubHeaderSinogram.h"
#include "CPhilipsDirectoryItem.h"
#include "CPhilipsFile.h"

#include <QDataStream>

#include <rtdebug.h>

CPhilipsSubHeaderSinogram::CPhilipsSubHeaderSinogram(CPhilipsFile* philipsFile,
                                                     CPhilipsDirectoryItem* pDirItem)
  : CPhilipsSubHeader(philipsFile, pDirItem)
{
  ENTER();

  LEAVE();
}

CPhilipsSubHeaderSinogram::CPhilipsSubHeaderSinogram(const CPhilipsSubHeaderSinogram& src)
  : CPhilipsSubHeader(src)
{
  ENTER();

  LEAVE();
}

CPhilipsSubHeader::Type CPhilipsSubHeaderSinogram::subHeaderType(void) const
{
  return CPhilipsSubHeader::Sinogram;
}

CMedIOHeader* CPhilipsSubHeaderSinogram::clone() const
{
  ENTER();

  CPhilipsSubHeaderSinogram* pNewHeader = new CPhilipsSubHeaderSinogram(*this);

  RETURN(pNewHeader);
  return pNewHeader;
} 

float CPhilipsSubHeaderSinogram::scnscl() const
{
  return CPhilipsSubHeader::scnscl();
}

short CPhilipsSubHeaderSinogram::scnmin() const
{
  return CPhilipsSubHeader::scnmin();
}

short CPhilipsSubHeaderSinogram::scnmax() const
{
  return CPhilipsSubHeader::scnmax();
}

float CPhilipsSubHeaderSinogram::scnsum() const
{
  return CPhilipsSubHeader::scnsum();
}

void CPhilipsSubHeaderSinogram::setScnscl(const float scnscl)
{
  CPhilipsSubHeader::setScnscl(scnscl);
}

void CPhilipsSubHeaderSinogram::setScnmin(const short scnmin)
{
  CPhilipsSubHeader::setScnmin(scnmin);
}

void CPhilipsSubHeaderSinogram::setScnmax(const short scnmax)
{
  CPhilipsSubHeader::setScnmax(scnmax);
}

void CPhilipsSubHeaderSinogram::setScnsum(const float scnsum)
{
  CPhilipsSubHeader::setScnsum(scnsum);
}
