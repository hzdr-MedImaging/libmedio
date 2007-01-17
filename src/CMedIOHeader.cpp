/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004-2007 by Jens Langner <Jens.Langner@light-speed.de>

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

//! @file CMedIOHeader.cpp
//! @brief contains the implementation of the class CMedIOHeader
//! @author Hagen Moelle

#include "CMedIOHeader.h"
#include "CMedIOData.h"

#include <rtdebug.h>

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
