/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio
 
  Copyright (C) 2004-2026 hzdr.de and contributors
 
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
    http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

//! @file CMedIO.h
//! @brief contains global definition of the class CMedIO

#ifndef CMEDIO_H
#define CMEDIO_H

#include <QString>

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
    static QString copyright();    
    static QString name();

    static unsigned int qtMajorVersion();
    static unsigned int qtMinorVersion();
    static unsigned int qtBuildVersion();
    static QString qtVersion();
};

#endif // CMEDIO_H


