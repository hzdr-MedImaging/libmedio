#
# libmedio - C++ I/O Library for loading/saving medical data formats
#            https://github.com/hzdr-MedImaging/libmedio
# 
# Copyright (C) 2004-2025 hzdr.de and contributors
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

TEMPLATE = app
VERSION  = 0.1.2

# Config
CONFIG += release thread

WITH_DEBUG_MESSAGES {
CONFIG -= release
CONFIG += debug
#DEFINES += DEBUG
QMAKE_CXXFLAGS_DEBUG += -g -O0
}

# Some dirs
OBJECTS_DIR	=.obj
MOC_DIR	        =.moc
UI_DIR          =.ui

TARGET = image2ecat
DESTDIR = ./

QMAKE_QMAKE = qmake

# Flags for release and debug
CXXFLAGS = -fmessage-length=0 -g -O2 -Wno-non-virtual-dtor -DHAVE_CONFIG_H
CFLAGS = $$CFLAGS -Wno-non-virtual-dtor -DHAVE_CONFIG_H


# find out the system
HOST_OS=Linux
contains(HOST_OS, SunOS) {
LIBS +=  -lz -lm -lthread
}
else{
contains(HOST_OS, Linux) {
#LIBS +=  -lz -lm -lwrap -lrt -lpthread -lnsl
LIBS     += -lz -lwrap -lm -lrt -lpthread -lnsl -lssl -lcrypto
}
else{
contains(HOST_OS, MINGW) {			
LIBS +=  -lm -lwsock32
}
}
}


# medio
CXXFLAGS   += -I/usr/local/petlib/include
CCFLAGS    += -I/usr/local/petlib/include
CXXFLAGS   += -I/usr/local/petlib/include/medio
CCFLAGS    += -I/usr/local/petlib/include/medio
LIBS += /usr/local/petlib/lib/libmedio.a

# coreg
CXXFLAGS   += -I/usr/local/petlib/include/coreg
CCFLAGS    += -I/usr/local/petlib/include/coreg
LIBS += /usr/local/petlib/lib/libcoreg.a


QMAKE_CXXFLAGS_RELEASE  += $$CXXFLAGS
                           QMAKE_CFLAGS_RELEASE    += $$CFLAGS
                           QMAKE_LFLAGS_RELEASE    += $$LFLAGS

                           QMAKE_CXXFLAGS_DEBUG    += $$CXXFLAGS
                           QMAKE_CFLAGS_DEBUG      += $$CFLAGS
                           QMAKE_LFLAGS_DEBUG 	+= $$LFLAGS


## Input
HEADERS += COptions.h

SOURCES += main.cpp \
           COptions.cpp

# Test if ansi color for terminal output was requested
ansi_color {
# Suppress echo of compiler execution and print current progressed filename colorized
QMAKE_CC		= @printf '\033[32mCompiling $< ...\033[0m\n'                     ;$$QMAKE_CXX
QMAKE_CXX		= @printf '\033[32mCompiling $< ...\033[0m\n'                     ;$$QMAKE_CXX
QMAKE_LINK	= @printf '\033[32mLinking \033[1m$@\033[0m\033[32m ...\033[0m\n'         ;$$QMAKE_LINK
}

unix {
# -p option should be -P, so that "make install" as root creates files owned by root
# and links are preserved.
QMAKE_COPY_FILE = $(COPY) -P
                  
                  # install executable
target.path = /usr/local/bin

# "make install" configuration options
INSTALLS += target
}

# Application icon for mac os x
macx {
}

win32 {
}


