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

//! @file CInterfileFrameHeader.cpp
//! @brief contains the implementation of the class CInterfileFrameHeader

#include "CInterfileFrameHeader.h"
#include "CInterfileFile.h"

#include <rtdebug.h>

class CInterfileFrameHeaderPrivate
{
  public:
    CInterfileFrameHeaderPrivate()
      : frame(1), dataType(CMedIOVolume::UnknownDataType), slope(1.0), intercept(0.0), start(0.0), duration(0.0)
    {
      for(int i = 0; i < 3; i++)
      {
        dims[i] = 0;
        for(int j = 0; j < 4; j++)
          affine[i][j] = (i == j) ? 1.0 : 0.0;
      }
    }

    short frame;
    CInterfileKeys keys;
    QString headerFileName;
    QString dataFileName;
    int dims[3];
    CMedIOVolume::DataType dataType;
    double slope;
    double intercept;
    QString units;
    double affine[3][4];
    double start;
    double duration;
};

CInterfileFrameHeader::CInterfileFrameHeader(CMedIOData* file, short frame)
  : CMedIOHeader(file), m_pData(new CInterfileFrameHeaderPrivate())
{
  m_pData->frame = frame;
}

CInterfileFrameHeader::CInterfileFrameHeader(const CInterfileFrameHeader& src)
  : CMedIOHeader(src), m_pData(new CInterfileFrameHeaderPrivate(*src.m_pData))
{
}

CInterfileFrameHeader::~CInterfileFrameHeader()
{
  delete m_pData;
}

CMedIOHeader& CInterfileFrameHeader::operator=(const CMedIOHeader& src)
{
  if(&src != this && src.headerFormat() == CMedIOHeader::InterfileFrameHeader)
    *this = static_cast<const CInterfileFrameHeader&>(src);

  return *this;
}

CInterfileFrameHeader& CInterfileFrameHeader::operator=(const CInterfileFrameHeader& src)
{
  if(&src != this)
  {
    copyAttributes(src);
    *m_pData = *src.m_pData;
  }

  return *this;
}

void CInterfileFrameHeader::clear()
{
  const short frame = m_pData->frame;
  *m_pData = CInterfileFrameHeaderPrivate();
  m_pData->frame = frame;
}

bool CInterfileFrameHeader::load()
{
  ENTER();
  bool result = false;

  CInterfileFile* file = dynamic_cast<CInterfileFile*>(medIOData());
  CInterfileFrameHeader* header = NULL;
  if(file != NULL && file->readSubHeader(header, m_pData->frame) && header != NULL)
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

bool CInterfileFrameHeader::save() const
{
  E("Interfile frames are written with CInterfileFile::writeMatrix()");
  return false;
}

CMedIOHeader::Format CInterfileFrameHeader::headerFormat() const
{
  return CMedIOHeader::InterfileFrameHeader;
}

bool CInterfileFrameHeader::convertFrom(const CMedIOHeader* header1, const CMedIOHeader* header2)
{
  Q_UNUSED(header2);

  if(header1 == NULL || header1->headerFormat() != CMedIOHeader::InterfileFrameHeader)
    return false;

  *this = *static_cast<const CInterfileFrameHeader*>(header1);
  return true;
}

CMedIOHeader* CInterfileFrameHeader::clone() const
{
  return new CInterfileFrameHeader(*this);
}

short CInterfileFrameHeader::frame() const { return m_pData->frame; }
void CInterfileFrameHeader::setFrame(short frame) { m_pData->frame = frame; }

const CInterfileKeys& CInterfileFrameHeader::keys() const { return m_pData->keys; }
void CInterfileFrameHeader::setKeys(const CInterfileKeys& keys) { m_pData->keys = keys; }

QString CInterfileFrameHeader::headerFileName() const { return m_pData->headerFileName; }
void CInterfileFrameHeader::setHeaderFileName(const QString& fileName) { m_pData->headerFileName = fileName; }
QString CInterfileFrameHeader::dataFileName() const { return m_pData->dataFileName; }
void CInterfileFrameHeader::setDataFileName(const QString& fileName) { m_pData->dataFileName = fileName; }

int CInterfileFrameHeader::dimension(int axis) const
{
  return (axis >= 0 && axis < 3) ? m_pData->dims[axis] : 0;
}

void CInterfileFrameHeader::setDimensions(int nx, int ny, int nz)
{
  m_pData->dims[0] = nx;
  m_pData->dims[1] = ny;
  m_pData->dims[2] = nz;
}

CMedIOVolume::DataType CInterfileFrameHeader::dataType() const { return m_pData->dataType; }
void CInterfileFrameHeader::setDataType(CMedIOVolume::DataType type) { m_pData->dataType = type; }

double CInterfileFrameHeader::slope() const { return m_pData->slope; }
double CInterfileFrameHeader::intercept() const { return m_pData->intercept; }

void CInterfileFrameHeader::setScaling(double slope, double intercept)
{
  m_pData->slope = slope;
  m_pData->intercept = intercept;
}

QString CInterfileFrameHeader::units() const { return m_pData->units; }
void CInterfileFrameHeader::setUnits(const QString& units) { m_pData->units = units; }

void CInterfileFrameHeader::affine(double m[3][4]) const
{
  for(int i = 0; i < 3; i++)
    for(int j = 0; j < 4; j++)
      m[i][j] = m_pData->affine[i][j];
}

void CInterfileFrameHeader::setAffine(const double m[3][4])
{
  for(int i = 0; i < 3; i++)
    for(int j = 0; j < 4; j++)
      m_pData->affine[i][j] = m[i][j];
}

double CInterfileFrameHeader::frameStart() const { return m_pData->start; }
double CInterfileFrameHeader::frameDuration() const { return m_pData->duration; }

void CInterfileFrameHeader::setTiming(double start, double duration)
{
  m_pData->start = start;
  m_pData->duration = duration;
}
