/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004-2005 by Jens Langner <Jens.Langner@light-speed.de>

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

#include "CECATSubHeader.h"
#include "CECATDirectoryItem.h"

#include <rtdebug.h>

CECATSubHeader::CECATSubHeader(CMedIOData* ecatFile,
															 CECATDirectoryItem* dItem)
	: CMedIOHeader(ecatFile),
		m_pDirItem(dItem)
{
	ENTER();

	LEAVE();
}

CECATSubHeader::CECATSubHeader(const CECATSubHeader& src __attribute__((unused)))
	: CMedIOHeader(src)
{
	ENTER();

	// do nothing

	LEAVE();
}

CECATSubHeader& CECATSubHeader::operator=(const CECATSubHeader& src)
{
	ENTER();

	convertFrom(&src);
	
	LEAVE();
	return *this;
}

CMedIOHeader::Format CECATSubHeader::headerFormat() const
{ 
	return CMedIOHeader::ECATSubHeader;
}

void CECATSubHeader::setDirectoryItem(CECATDirectoryItem* dItem)
{ 
	m_pDirItem = dItem;
}
