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

//! @file CInterfileMainHeader.cpp
//! @brief contains the implementation of the class CInterfileMainHeader

#include "CInterfileMainHeader.h"
#include "CInterfileFile.h"

#include <rtdebug.h>

class CInterfileMainHeaderPrivate
{
  public:
    CInterfileMainHeaderPrivate() : numFrames(0) { }

    CInterfileKeys keys;
    QString headerFileName;
    int numFrames;
};

CInterfileMainHeader::CInterfileMainHeader(CMedIOData* file)
  : CMedIOHeader(file), m_pData(new CInterfileMainHeaderPrivate())
{
}

CInterfileMainHeader::CInterfileMainHeader(const CInterfileMainHeader& src)
  : CMedIOHeader(src), m_pData(new CInterfileMainHeaderPrivate(*src.m_pData))
{
}

CInterfileMainHeader::~CInterfileMainHeader()
{
  delete m_pData;
}

CMedIOHeader& CInterfileMainHeader::operator=(const CMedIOHeader& src)
{
  if(&src != this && src.headerFormat() == CMedIOHeader::InterfileMainHeader)
    *this = static_cast<const CInterfileMainHeader&>(src);

  return *this;
}

CInterfileMainHeader& CInterfileMainHeader::operator=(const CInterfileMainHeader& src)
{
  if(&src != this)
  {
    copyAttributes(src);
    *m_pData = *src.m_pData;
  }

  return *this;
}

void CInterfileMainHeader::clear()
{
  *m_pData = CInterfileMainHeaderPrivate();
}

bool CInterfileMainHeader::load()
{
  ENTER();
  bool result = false;

  CInterfileFile* file = dynamic_cast<CInterfileFile*>(medIOData());
  CInterfileMainHeader* header = NULL;
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

bool CInterfileMainHeader::save() const
{
  ENTER();

  CInterfileFile* file = dynamic_cast<CInterfileFile*>(medIOData());
  const bool result = (file != NULL) && file->writeMainHeader(*this);

  RETURN(result);
  return result;
}

CMedIOHeader::Format CInterfileMainHeader::headerFormat() const
{
  return CMedIOHeader::InterfileMainHeader;
}

bool CInterfileMainHeader::convertFrom(const CMedIOHeader* header1, const CMedIOHeader* header2)
{
  Q_UNUSED(header2);

  if(header1 == NULL || header1->headerFormat() != CMedIOHeader::InterfileMainHeader)
    return false;

  *this = *static_cast<const CInterfileMainHeader*>(header1);
  return true;
}

CMedIOHeader* CInterfileMainHeader::clone() const
{
  return new CInterfileMainHeader(*this);
}

const CInterfileKeys& CInterfileMainHeader::keys() const
{
  return m_pData->keys;
}

void CInterfileMainHeader::setKeys(const CInterfileKeys& keys)
{
  m_pData->keys = keys;
}

QString CInterfileMainHeader::headerFileName() const
{
  return m_pData->headerFileName;
}

void CInterfileMainHeader::setHeaderFileName(const QString& fileName)
{
  m_pData->headerFileName = fileName;
}

int CInterfileMainHeader::numFrames() const
{
  return m_pData->numFrames;
}

void CInterfileMainHeader::setNumFrames(int frames)
{
  m_pData->numFrames = frames;
}
