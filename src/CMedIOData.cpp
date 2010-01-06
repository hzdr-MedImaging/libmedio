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

//! @file CMedIOData.cpp
//! @brief contains the implementation of the class CMedIOData
//! @author Hagen Moelle

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
  //m_pHeader = NULL;
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
  //if(m_pHeader)
  //  delete m_pHeader;
}

//! @brief for querying error status information
//! @return integer value for the specific error reason
int CMedIOData::errorStatus(void) const
{ 
  return m_iErrStatus;
}
