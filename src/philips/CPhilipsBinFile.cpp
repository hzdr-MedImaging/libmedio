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

      if(m_header.magicNumber == BIN_FILE_MAGIC_NUMBER)
      {
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
  quint32 version = -1;

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
      {
        bswap_matrix<quint32>(reinterpret_cast<const quint32*>(data), numberOfElements()*sizeof(quint32), reinterpret_cast<quint32*>(data));
      }
      break;

      case Float32:
      {
        float* data_flt = reinterpret_cast<float*>(data);

        for(quint32 i = 0; i < numberOfElements(); ++i)
          BSWAP_FLT(data_flt[i]);
      }
      break;

      case Float64:
      {
        double* data_dbl = reinterpret_cast<double*>(data);

        for(quint32 i = 0; i < numberOfElements(); ++i)
          BSWAP_DBL(data_dbl[i]);
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
    length = numberOfElements() * elementSize();
    data = new char[length];
    qint64 bytesRead = 0;

    seek(sizeof(PhilipsBinHeader1));

    bytesRead = read(data, length);

    if((bytesRead != -1) &&
       (bytesRead == static_cast<qint64>(length)))
    {
      swap(data);
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

  RETURN(returnValue);
  return returnValue;
}
