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
INCLUDEPATH += . ../../../src/include /usr/local/petlib/include/medio
LIBS *= /usr/local/petlib/lib/libmedio.a
#LIBS *= ../../../build-l64/lib/libmedio.a /usr/local/petlib/lib/librtdebug.so

mac {
  CONFIG += x86 ppc
  QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
}

# Input
SOURCES += ecatcmp.cpp
