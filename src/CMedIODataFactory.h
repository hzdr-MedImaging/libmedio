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

//! @file CMedIODataFactory.h
//! @author Hagen Moelle
//! @brief contains definition of the class CMedIODataFactory

#ifndef CMEDIODATAFACTORY_H
#define CMEDIODATAFACTORY_H

#ifndef __MEDIO_PRIVATE__
#include <CMedIOData>
#else
#include <CMedIOData.h>
#endif

#include <qstring.h>

//! @class CMedIODataFactory
//! @brief factory class allowing construction of CMedIOData objects 
//! @bug no bugs reported
//! @warning still in development
//!
//! This class is a factory which allows easy construction of different 
//! CMedIOData objects. All kinds of medical data which are implemented
//! can be instancieted by this class.
class CMedIODataFactory
{
	public:
		//! @brief identifies fileformat and returns the type of it
		static CMedIOData::Format identify(const QString& fileName);

		//! @brief identifies fileformat and constructs CMedIOData object
		//! @param file: filename where medical data is located
		//! @return CMedIOData object if format is known otherwise NULL
		static CMedIOData* create(const QString& fileName);
};

#endif
