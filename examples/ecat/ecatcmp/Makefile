#/* vim:set ts=2 nowrap: ****************************************************
#
# libmedio - Medical Data C++ I/O Library
# Copyright (C) 2004-2005 by Jens Langner <Jens.Langner@light-speed.de>
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
# $Id$
#
#****************************************************************************

TARGET = ecatcmp

# programs
CC		= g++
RM		= rm -f
RMDIR = rm -rf
MKDIR = mkdir

# Directories
PREFIX = .

# Compiler/Linker Flags
WARN    = -W -Wall
CFLAGS  = -I/usr/local/include/medio -I$(QTDIR)/include $(WARN) -c
LDFLAGS = -L../../../lib -L$(QTDIR)/lib
LDLIBS  = -lqt-mt -lmedio

OBJS = ecatcmp.o

#
all: $(TARGET)

#
%.o: %.cpp
	@printf '\033[32mCompiling $<\033[0m\n'
	@$(CC) $(CFLAGS) $< -o $@

#
$(TARGET): $(OBJS)
	@printf '\033[32mLinking \033[1m$@\033[0m\n'
	@$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

#
clean:
	-$(RM) $(TARGET) $(OBJS)
