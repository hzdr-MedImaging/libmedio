/* vim:set ts=2 sw=2 expandtab: ********************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2006-2010 by Jens Langner <Jens.Langner@light-speed.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

//! @file CMedIODataFactory.cpp
//! @brief contains the implementation of the class CMedIODataFactory
//! @author Hagen Moelle

#include "CMedIODataFactory.h"

#include "CECATFile.h"
#include "CMedIOData.h"
#include "CConcordeFile.h"
#include "CConcordeSinogram.h"
#include "CConcordeImage.h"

#include <rtdebug.h>

CMedIOData::Format CMedIODataFactory::identify(const QString& fileName)
{
  ENTER();

  CMedIOData::Format result;

//  if(CConcordeFile::isOfType(fileName))
//    result = CMedIOData::ConcordeMicropet;
  if(CECATFile::isOfType(fileName))
    result = CMedIOData::ECAT;
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
  else
  {
    E("Can not identify file format");
    mData = NULL;
  }
          
  RETURN(mData);
  return mData;
}
