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
QT = core
DEPENDPATH += .
INCLUDEPATH += . ../../src/ ../../src/ecat ../../src/ecat/ecat6 ../../src/ecat/ecat7 ../../src/micropet ../../src/helpers
INCLUDEPATH += /usr/local/petlib/include/rtdebug /usr/local/include/rtdebug
LIBS *= ../../lib/libmedio.a
LIBS *= -L/usr/local/petlib/lib -lrtdebug
DEFINES *= __MEDIO_PRIVATE__
DEFINES *= DEBUG
COMPILE_LEVEL = debug

# Input
SOURCES += readwritematrix.cpp
