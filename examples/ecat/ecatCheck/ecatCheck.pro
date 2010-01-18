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
# $Id: ecatcmp.pro 467 2008-04-17 08:22:25Z langner $
#
#****************************************************************************

TEMPLATE = app
QT = core
DEPENDPATH += .
INCLUDEPATH += . ../../../src/include /usr/local/petlib/include/medio
LIBS *= ../../../lib/libmedio.a

mac {
  CONFIG += x86 ppc
  QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
}

# Input
SOURCES += ecatCheck.cpp
