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

#include <rtdebug.h>
#include "CPhilipsBinFile.h"

#include "bswap.h"

#define BIN_FILE_MAGIC_NUMBER 0x7c1e40d4

CPhilipsBinFile::CPhilipsBinFile(QString filename)
  : CMedIOData(filename)
{
  ENTER();
  LEAVE();
}

bool CPhilipsBinFile::open(QIODevice::OpenModeFlag mode)
{
  ENTER();
  bool returnValue = false;

  if(QFile::open(mode))
  {
    if(read(reinterpret_cast<char*>(&m_header), sizeof(PhilipsBinHeader1)))
    {
      if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
      {
        BSWAP_32(m_header.magicNumber);
        BSWAP_32(m_header.headerVersion);
        BSWAP_32(m_header.headerWords);
        BSWAP_32(m_header.dataType);
        BSWAP_32(m_header.zDim);
        BSWAP_32(m_header.yDim);
        BSWAP_32(m_header.xDim);
      }

      D("magicNumber..: %08x", m_header.magicNumber);
      D("headerVersion: %d", m_header.headerVersion);
      D("headerWords..: %d", m_header.headerWords);
      D("dataType.....: %d", m_header.dataType);
      D("zDim.........: %d", m_header.zDim);
      D("yDim.........: %d", m_header.yDim);
      D("xDim.........: %d", m_header.xDim);

      if(m_header.magicNumber == BIN_FILE_MAGIC_NUMBER)
      {
        // if we find a dimension to be 0 we assume 1
        // to fix potentially broken files
        if(m_header.zDim < 1) m_header.zDim = 1;
        if(m_header.yDim < 1) m_header.yDim = 1;
        if(m_header.xDim < 1) m_header.xDim = 1;

        if(m_header.headerVersion == 1)
          returnValue = true;
        else
          E("only header version 1 supported yet");
      }
      else
      {
        E("this is not a bin file");
        close();
      }
    }
    else
      close();
  }

  RETURN(returnValue);
  return returnValue;
}

quint32 CPhilipsBinFile::readVersion()
{
  ENTER();
  quint32 version = 0;

  if(isOpen() &&
     isReadable())
  {
    quint64 oldPosition = pos();

    if(seek(sizeof(quint32)))
    {
      read(reinterpret_cast<char*>(&version), sizeof(quint32));

      if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
        BSWAP_32(version);
    }

    seek(oldPosition);
  }

  RETURN(version);
  return version;
}

void CPhilipsBinFile::close()
{
  ENTER();
  QFile::close();
  LEAVE();
}

unsigned int CPhilipsBinFile::elementSize() const
{
  ENTER();
  unsigned int size = 0;

  switch(m_header.dataType)
  {
    case UnsignedInt8:
    case SignedInt8:
    {
      size = 1;
    }
    break;

    case UnsignedInt16:
    case SignedInt16:
    {
      size = 2;
    }
    break;

    case UnsignedInt32:
    case SignedInt32:
    case Float32:
    {
      size = 4;
    }
    break;

    case Float64:
    {
      size = 8;
    }
    break;

    default:
      // nothing
    break;
  }

  RETURN(size);
  return size;
}

unsigned int CPhilipsBinFile::numberOfElements() const
{
  ENTER();
  RETURN(m_header.zDim * m_header.yDim * m_header.xDim);
  return m_header.zDim * m_header.yDim * m_header.xDim;
}

void CPhilipsBinFile::swap(char*& data)
{
  ENTER();

  if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
  {
    switch(m_header.dataType)
    {
      case UnsignedInt16:
      case SignedInt16:
      {
        bswap_matrix<quint16>(reinterpret_cast<const quint16*>(data), numberOfElements()*sizeof(quint16), reinterpret_cast<quint16*>(data));
      }
      break;

      case UnsignedInt32:
      case SignedInt32:
      case Float32:
      {
        bswap_matrix<quint32>(reinterpret_cast<const quint32*>(data), numberOfElements()*sizeof(quint32), reinterpret_cast<quint32*>(data));
      }
      break;

      case Float64:
      {
        bswap_matrix<quint64>(reinterpret_cast<const quint64*>(data), numberOfElements()*sizeof(quint64), reinterpret_cast<quint64*>(data));
      }
      break;
    }
  }

  LEAVE();
}

bool CPhilipsBinFile::readMatrix(char*& data, unsigned int& length)
{
  ENTER();
  bool returnValue = false;

  if(isOpen() &&
     isReadable())
  {
    quint32 dataSize = numberOfElements() * elementSize();
    if(dataSize > 0)
    {
      data = new char[dataSize];
      qint64 bytesRead = 0;

      seek(m_header.headerWords * sizeof(quint32));
      bytesRead = read(data, dataSize);
      if((bytesRead != -1) &&
         (bytesRead == static_cast<qint64>(dataSize)))
      {
        swap(data);

        length = dataSize;
        returnValue = true;
      }
      else
      {
        length = 0;
        delete [] data;
        data = NULL;
        E("reading failed");
      }
    }
    else
      E("data size <= 0");
  }

  RETURN(returnValue);
  return returnValue;
}
