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

//! @file CMedIOHeader.cpp
//! @brief contains the implementation of the class CMedIOHeader

#include "CMedIOHeader.h"
#include "CMedIOData.h"
#include "CECATFile.h"
#include "CPhilipsFile.h"

#include <rtdebug.h>
#include <iostream>

CMedIOHeader::CMedIOHeader(CMedIOData* data)
  : m_pMedIOData(data)
{ 
  ENTER();
  
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
  LEAVE();
}

CMedIOHeader::CMedIOHeader(const CMedIOHeader&)
  : m_pMedIOData(NULL)
{
  ENTER();

  // do nothing

  LEAVE();
}

CMedIOHeader& CMedIOHeader::operator=(const CMedIOHeader& src)
{
  ENTER();

  convertFrom(&src);

  LEAVE();
  return *this;
}

void CMedIOHeader::setMedIOData(CMedIOData* data)
{
  ENTER();

  m_pMedIOData = data;
  
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

        // convert the main header
        CECATMainHeader* mainHeader;
        if(ecatFile->readMainHeader(mainHeader))
          convertFrom(mainHeader);

        // convert the sub header
        CECATSubHeader* subHeader;
        if(ecatFile->readSubHeader(subHeader, 1))
          convertFrom(subHeader);

        delete mainHeader;
        delete subHeader;

        result = true;
      }
      break;

      case CMedIOData::Philips:
      {
        CPhilipsFile* philipsFile = static_cast<CPhilipsFile*>(pData);

        // convert the main header
        CPhilipsMainHeader* mainHeader;
        if(philipsFile->readMainHeader(mainHeader))
          convertFrom(mainHeader);

        // convert the sub header
        CPhilipsSubHeader* subHeader;
        if(philipsFile->readSubHeader(subHeader))
          convertFrom(subHeader);

        delete mainHeader;
        delete subHeader;

        result = true;
      }
      break;
    }
  }

  RETURN(result);
  return result;
}
