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

#include "CMedIO.h"
#include "config.h"

unsigned int CMedIO::majorVersion()
{
	return QString(PACKAGE_VERSION).section('.', 0, 0).toUInt();
}

unsigned int CMedIO::minorVersion()
{
	return QString(PACKAGE_VERSION).section('.', 1, 1).toUInt();
}

unsigned int CMedIO::buildVersion()
{
	return QString(PACKAGE_VERSION).section('.', 2, 2).toUInt();
}

QString CMedIO::version()
{
	return QString(PACKAGE_VERSION);
}

QString CMedIO::buildDate()
{
	return QString(__DATE__) + " " + QString(__TIME__);
}

QString CMedIO::copyright()
{
	return QString("Copyright (C) 2004-2007 by Jens Langner <Jens.Langner@light-speed.de>");
}

unsigned int CMedIO::qtMajorVersion()
{
	return (QT_VERSION & 0xff0000)>>16;
}

unsigned int CMedIO::qtMinorVersion()
{
	return (QT_VERSION & 0x00ff00)>>8;
}

unsigned int CMedIO::qtBuildVersion()
{
	return QT_VERSION & 0x0000ff;
}

QString CMedIO::qtVersion()
{
	return QString::number(qtMajorVersion()) + "." + QString::number(qtMinorVersion()) + "." + QString::number(qtBuildVersion());
}

