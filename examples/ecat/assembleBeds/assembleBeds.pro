#/* vim:set ts=2 nowrap: ****************************************************
#
# libmedio - Medical Data C++ I/O Library
# Copyright (C) 2004 by Jens Langner <Jens.Langner@light-speed.de>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# $Id: ecatcmp.pro 317 2006-03-17 17:51:06Z langner $
#
#****************************************************************************

TEMPLATE = app
QT = core
DEPENDPATH += .
INCLUDEPATH += . /usr/local/petlib/include/medio
INCLUDEPATH += /usr/local/petlib/include/rtdebug
LIBS *= /usr/local/petlib/lib/libmedio.so.2
LIBS *= /usr/local/petlib/lib/librtdebug.so

DEFINES *= DEBUG

# Input
HEADERS += CECAT7Assemble.h \
           CDataArray.h \
           CMultiDimArray.h
SOURCES += CECAT7Assemble.cpp \
           assembleBeds.cpp
