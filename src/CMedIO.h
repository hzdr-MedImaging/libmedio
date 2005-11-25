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

//! @file CMedIO.h
//! @author Christian Poetzsch
//! @brief contains global definition of the class CMedIO

#ifndef CMEDIO_H
#define CMEDIO_H

#include <qstring.h>

//! @class CMedIO
//! @brief a global class which provides information of the lib medio
//! @bug no bugs reported
class CMedIO
{
	public:
		static unsigned int majorVersion();
		static unsigned int minorVersion();
		static unsigned int buildVersion();
		static QString version();
		static QString buildDate();

		static unsigned int qtMajorVersion();
		static unsigned int qtMinorVersion();
		static unsigned int qtBuildVersion();
		static QString qtVersion();
};

#endif // CMEDIO_H


