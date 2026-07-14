#
# libmedio - C++ I/O Library for loading/saving medical data formats
#            https://github.com/hzdr-MedImaging/libmedio
#
# Copyright (C) 2004-2026 hzdr.de and contributors
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
TARGET = 
DEPENDPATH += .
INCLUDEPATH += . ../../src/ ../../src/philips ../../src/helpers
INCLUDEPATH += /usr/local/petlib/include/rtdebug
LIBS *= ../../lib/libmedio.a
LIBS *= -L/usr/local/petlib/lib -lrtdebug
DEFINES *= __MEDIO_PRIVATE__

QT += testlib
QT -= gui

# directory specification
unix {
  OBJECTS_DIR = .obj
  MOC_DIR     = .moc
  UI_DIR      = .ui
}
win32 {
  OBJECTS_DIR = _obj
  MOC_DIR     = _moc
  UI_DIR      = _ui
}
# Input
HEADERS += testCPhilipsFile.h testCPhilipsSubHeaderImage.h testCPhilipsSubHeaderSinogram.h
SOURCES += testCPhilipsFile.cpp testPhilipsFormat.cpp testCPhilipsSubHeaderImage.cpp testCPhilipsSubHeaderSinogram.cpp

# override the compile output
QMAKE_CXX  = @echo \'  CC   $<\'; $$QMAKE_CXX
QMAKE_LINK = @echo \'  LD   $@\'; $$QMAKE_LINK
QMAKE_MOC  = @echo \'  MOC  $@\'; $$QMAKE_MOC
