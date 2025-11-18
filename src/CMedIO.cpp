/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio
 
  Copyright (C) 2004-2025 hzdr.de and contributors
 
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

#include "CMedIO.h"
#include "config.h"

unsigned int CMedIO::majorVersion()
{
  return QString(PROJECT_VERSION).section('.', 0, 0).toUInt();
}

unsigned int CMedIO::minorVersion()
{
  return QString(PROJECT_VERSION).section('.', 1, 1).toUInt();
}

unsigned int CMedIO::buildVersion()
{
  return QString(PROJECT_VERSION).section('.', 2, 2).toUInt();
}

QString CMedIO::version()
{
  return QString(PROJECT_VERSION);
}

QString CMedIO::name()
{
  return QString(PROJECT_LONGNAME);
}

QString CMedIO::buildDate()
{
  return QString(__DATE__) + " " + QString(__TIME__);
}

QString CMedIO::copyright()
{
  return QString("Copyright (C) 2004-2025 hzdr.de and contributors (Apache-2.0)");
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

