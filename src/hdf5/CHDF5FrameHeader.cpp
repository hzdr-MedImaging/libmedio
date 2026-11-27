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

//! @file CHDF5FrameHeader.cpp
//! @brief contains the implementation of the class CHDF5FrameHeader

#include "CHDF5FrameHeader.h"
#include "CHDF5File.h"

#include <rtdebug.h>

class CHDF5FrameHeaderPrivate
{
  public:
    CHDF5FrameHeaderPrivate()
      : volume(0), frame(-1), gate(1), bed(0), dataType(CMedIOVolume::UnknownDataType), start(0.0), duration(0.0)
    {
      for(int i = 0; i < 3; i++)
      {
        dimensions[i] = 0;
        for(int j = 0; j < 4; j++)
          affine[i][j] = (i == j) ? 1.0 : 0.0;
      }
    }

    int volume;
    short frame;
    short gate;
    short bed;
    int dimensions[3];
    CMedIOVolume::DataType dataType;
    QString units;
    double affine[3][4];
    double start;
    double duration;
};

CHDF5FrameHeader::CHDF5FrameHeader(CMedIOData* file, short frame)
  : CMedIOHeader(file), m_pData(new CHDF5FrameHeaderPrivate())
{
  m_pData->frame = frame;
}

CHDF5FrameHeader::CHDF5FrameHeader(const CHDF5FrameHeader& src)
  : CMedIOHeader(src), m_pData(new CHDF5FrameHeaderPrivate(*src.m_pData))
{
}

CHDF5FrameHeader::~CHDF5FrameHeader()
{
  delete m_pData;
}

CMedIOHeader& CHDF5FrameHeader::operator=(const CMedIOHeader& src)
{
  if(&src != this && src.headerFormat() == CMedIOHeader::HDF5FrameHeader)
    *this = static_cast<const CHDF5FrameHeader&>(src);
  return *this;
}

CHDF5FrameHeader& CHDF5FrameHeader::operator=(const CHDF5FrameHeader& src)
{
  if(&src != this)
  {
    copyAttributes(src);
    *m_pData = *src.m_pData;
  }
  return *this;
}

void CHDF5FrameHeader::clear()
{
  *m_pData = CHDF5FrameHeaderPrivate();
}

bool CHDF5FrameHeader::load()
{
  ENTER();
  bool result = false;

  CHDF5File* file = dynamic_cast<CHDF5File*>(medIOData());
  CHDF5FrameHeader* header = NULL;
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

bool CHDF5FrameHeader::save() const
{
  return false;
}

CMedIOHeader::Format CHDF5FrameHeader::headerFormat() const
{
  return CMedIOHeader::HDF5FrameHeader;
}

bool CHDF5FrameHeader::convertFrom(const CMedIOHeader* header1, const CMedIOHeader* header2)
{
  Q_UNUSED(header2);
  if(header1 == NULL || header1->headerFormat() != CMedIOHeader::HDF5FrameHeader)
    return false;

  *this = *static_cast<const CHDF5FrameHeader*>(header1);
  return true;
}

CMedIOHeader* CHDF5FrameHeader::clone() const
{
  return new CHDF5FrameHeader(*this);
}

int CHDF5FrameHeader::volume() const
{
  return m_pData->volume;
}

void CHDF5FrameHeader::setVolume(int volume)
{
  m_pData->volume = volume;
}

short CHDF5FrameHeader::frame() const
{
  return m_pData->frame;
}

short CHDF5FrameHeader::gate() const
{
  return m_pData->gate;
}

short CHDF5FrameHeader::bed() const
{
  return m_pData->bed;
}

void CHDF5FrameHeader::setIndex(short frame, short gate, short bed)
{
  m_pData->frame = frame;
  m_pData->gate = gate;
  m_pData->bed = bed;
}

int CHDF5FrameHeader::dimension(int axis) const
{
  return (axis >= 0 && axis < 3) ? m_pData->dimensions[axis] : 0;
}

void CHDF5FrameHeader::setDimensions(int nx, int ny, int nz)
{
  m_pData->dimensions[0] = nx;
  m_pData->dimensions[1] = ny;
  m_pData->dimensions[2] = nz;
}

CMedIOVolume::DataType CHDF5FrameHeader::dataType() const
{
  return m_pData->dataType;
}

void CHDF5FrameHeader::setDataType(CMedIOVolume::DataType type)
{
  m_pData->dataType = type;
}

QString CHDF5FrameHeader::units() const
{
  return m_pData->units;
}

void CHDF5FrameHeader::setUnits(const QString& units)
{
  m_pData->units = units;
}

void CHDF5FrameHeader::affine(double m[3][4]) const
{
  for(int i = 0; i < 3; i++)
  {
    for(int j = 0; j < 4; j++)
      m[i][j] = m_pData->affine[i][j];
  }
}

void CHDF5FrameHeader::setAffine(const double m[3][4])
{
  for(int i = 0; i < 3; i++)
  {
    for(int j = 0; j < 4; j++)
      m_pData->affine[i][j] = m[i][j];
  }
}

double CHDF5FrameHeader::frameStart() const
{
  return m_pData->start;
}

double CHDF5FrameHeader::frameDuration() const
{
  return m_pData->duration;
}

void CHDF5FrameHeader::setTiming(double start, double duration)
{
  m_pData->start = start;
  m_pData->duration = duration;
}
