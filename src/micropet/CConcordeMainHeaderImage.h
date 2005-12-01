/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004 by Jens Langner <Jens.Langner@light-speed.de>

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

//! @file CConcordeMainHeaderImage.h
//! @author Hagen Moelle
//! @date 09/10/2005
//! @brief contains definition of the class CConcordeMainHeaderImage

#ifndef CCONCORDEMAINHEADERIMAGE_H
#define CCONCORDEMAINHEADERIMAGE_H

#include <QString>

#ifndef __MEDIO_PRIVATE__
#include <CConcordeMainHeader>
#else
#include <CConcordeMainHeader.h>
#endif

// forward declarations
class CConcordeFile;
class CMedIOHeader;

class CConcordeMainHeaderImage : public CConcordeMainHeader
{
	public:
		CConcordeMainHeaderImage(QString File) : CConcordeMainHeader(File){};
		CConcordeMainHeaderImage(CConcordeFile* file) : CConcordeMainHeader(file){};
		unsigned int frameSize();
		CMedIOHeader* clone() const;
};

#endif
