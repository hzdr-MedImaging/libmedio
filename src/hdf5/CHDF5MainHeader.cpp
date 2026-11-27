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

//! @file CHDF5MainHeader.cpp
//! @brief contains the implementation of the class CHDF5MainHeader

#include "CHDF5MainHeader.h"
#include "CHDF5File.h"

#include <rtdebug.h>

class CHDF5MainHeaderPrivate
{
  public:
    CHDF5MainHeaderPrivate() : numVolumes(0), numFrames(0) { }

    QString formatVersion;
    int numVolumes;
    int numFrames;
};

CHDF5MainHeader::CHDF5MainHeader(CMedIOData* file)
  : CMedIOHeader(file), m_pData(new CHDF5MainHeaderPrivate())
{
}

CHDF5MainHeader::CHDF5MainHeader(const CHDF5MainHeader& src)
  : CMedIOHeader(src), m_pData(new CHDF5MainHeaderPrivate(*src.m_pData))
{
}

CHDF5MainHeader::~CHDF5MainHeader()
{
  delete m_pData;
}

CMedIOHeader& CHDF5MainHeader::operator=(const CMedIOHeader& src)
{
  if(&src != this && src.headerFormat() == CMedIOHeader::HDF5MainHeader)
    *this = static_cast<const CHDF5MainHeader&>(src);
  return *this;
}

CHDF5MainHeader& CHDF5MainHeader::operator=(const CHDF5MainHeader& src)
{
  if(&src != this)
  {
    copyAttributes(src);
    *m_pData = *src.m_pData;
  }
  return *this;
}

void CHDF5MainHeader::clear()
{
  *m_pData = CHDF5MainHeaderPrivate();
}

bool CHDF5MainHeader::load()
{
  ENTER();
  bool result = false;

  CHDF5File* file = dynamic_cast<CHDF5File*>(medIOData());
  CHDF5MainHeader* header = NULL;
  if(file != NULL && file->readMainHeader(header) && header != NULL)
  {
    CMedIOData* const linked = medIOData();
    *this = *header;
    setMedIOData(linked);
    result = true;
  }
  delete header;

  RETURN(result);
  return result;
}

bool CHDF5MainHeader::save() const
{
  return false;
}

CMedIOHeader::Format CHDF5MainHeader::headerFormat() const
{
  return CMedIOHeader::HDF5MainHeader;
}

bool CHDF5MainHeader::convertFrom(const CMedIOHeader* header1, const CMedIOHeader* header2)
{
  Q_UNUSED(header2);
  if(header1 == NULL || header1->headerFormat() != CMedIOHeader::HDF5MainHeader)
    return false;

  *this = *static_cast<const CHDF5MainHeader*>(header1);
  return true;
}

CMedIOHeader* CHDF5MainHeader::clone() const
{
  return new CHDF5MainHeader(*this);
}

QString CHDF5MainHeader::formatVersion() const
{
  return m_pData->formatVersion;
}

void CHDF5MainHeader::setFormatVersion(const QString& version)
{
  m_pData->formatVersion = version;
}

int CHDF5MainHeader::numVolumes() const
{
  return m_pData->numVolumes;
}

void CHDF5MainHeader::setNumVolumes(int volumes)
{
  m_pData->numVolumes = volumes;
}

int CHDF5MainHeader::numFrames() const
{
  return m_pData->numFrames;
}

void CHDF5MainHeader::setNumFrames(int frames)
{
  m_pData->numFrames = frames;
}
