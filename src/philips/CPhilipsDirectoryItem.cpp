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

#include <QCoreApplication>
#include "CPhilipsDirectoryItem.h"
#include "CPhilipsFile.h"

#include "CPhilipsSubHeader.h"
#include "CPhilipsSubHeaderImage.h"
#include "CPhilipsSubHeaderSinogram.h"
#include "CPhilipsSubHeaderSyntegra.h"
#include "CPhilipsSubHeaderListmode.h"

#include <QDataStream>
#include <rtdebug.h>

#include "bswap.h"

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CPhilipsDirectoryItemPrivate
{
  public:
    void cacheSubHeader(const CPhilipsSubHeader& subHeader);
    
    CPhilipsDirectoryItem* dirItem; // pointer to the directory Item
    CPhilipsFile* file;             // pointer to the PhilipsFile
    CPhilipsSubHeader* cachedSubHeader;  // pointer to a cached SubHeader object

    // META information about the directory Item
    short frame;                // information normally
    short slice;                // encoded in
    short tilt;                 // the MatrixID
    qint64 dataBlock_Start;     // also start of SubHeader or extendedMainHeader
    qint64 dataBlock_End;       // end of raw Data area

    enum CPhilipsDirectoryItem::CompressionFlag compressionFlag;
    enum CPhilipsDirectoryItem::ContentFlag contentFlag;
};

CPhilipsDirectoryItem::CPhilipsDirectoryItem(CPhilipsFile* pFile,
                                             quint32 matrixID)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CPhilipsDirectoryItemPrivate();
  m_pData->dirItem = this;
  m_pData->file = pFile;
  m_pData->cachedSubHeader = NULL;
  m_pData->dataBlock_Start = 0;
  m_pData->dataBlock_End = 0;
  m_pData->compressionFlag = Uncompressed;
  m_pData->contentFlag = Unused;

  if(matrixID != 0)
  {
    m_pData->slice = matrixID2Slice(matrixID);
    m_pData->frame = matrixID2Frame(matrixID);
    m_pData->tilt  = matrixID2Tilt(matrixID);

    if(matrixID == PHILIPS_EXT_HEADER_ID)
    {
      D("created new DirItem (%08lx) (extended main header)", matrixID);
      m_pData->contentFlag = Header;
    }
    else
      D("created new DirItem (%d/%d/%d)", m_pData->slice, m_pData->frame, m_pData->tilt);
  }
  else
  {
    m_pData->frame = -1;
    m_pData->slice = -1;
    m_pData->tilt = -1;
  }    
  
  LEAVE();
}

CPhilipsDirectoryItem::~CPhilipsDirectoryItem()
{
  ENTER();

  delete m_pData->cachedSubHeader;
  delete m_pData;

  LEAVE();
}

CPhilipsDirectoryItem::CPhilipsDirectoryItem(const CPhilipsDirectoryItem& src)
{
  ENTER();

  m_pData = new CPhilipsDirectoryItemPrivate();

  // copy the instance data
  *this = src;

  m_pData->dirItem = this;
  m_pData->file = NULL;
  m_pData->cachedSubHeader = NULL;
  
  LEAVE();
}

CPhilipsDirectoryItem& CPhilipsDirectoryItem::operator=(const CPhilipsDirectoryItem& src)
{
 ENTER();

  if(m_pData != src.m_pData)
  {
    m_pData->slice = src.m_pData->slice;
    m_pData->frame = src.m_pData->frame;
    m_pData->tilt = src.m_pData->tilt;
    m_pData->dataBlock_Start = src.m_pData->dataBlock_Start;
    m_pData->dataBlock_End = src.m_pData->dataBlock_End;
    m_pData->compressionFlag = src.m_pData->compressionFlag;
    m_pData->contentFlag = src.m_pData->contentFlag;
    m_pData->dirItem = this;
  }

  LEAVE();
  return *this;
}

quint32 CPhilipsDirectoryItem::matrixID() const
{ 
  return convertToMatrixID(m_pData->slice, 
                           m_pData->frame, 
                           m_pData->tilt);
}

bool CPhilipsDirectoryItem::isExtendedHeader() const
{
  return ((m_pData->contentFlag == Header) &&
          (matrixID() == PHILIPS_EXT_HEADER_ID));
}

qint64 CPhilipsDirectoryItem::dataBlock_Start() const
{ 
  return m_pData->dataBlock_Start;
}

qint64 CPhilipsDirectoryItem::dataBlock_End() const
{ 
  return m_pData->dataBlock_End;
}

CPhilipsDirectoryItem::CompressionFlag CPhilipsDirectoryItem::compressionFlag() const
{ 
  return m_pData->compressionFlag; 
}

CPhilipsDirectoryItem::ContentFlag CPhilipsDirectoryItem::contentFlag() const
{ 
  return m_pData->contentFlag; 
}

short CPhilipsDirectoryItem::frame() const
{ 
  return m_pData->frame;
}

short CPhilipsDirectoryItem::slice() const
{ 
  return m_pData->slice;
}

short CPhilipsDirectoryItem::tilt() const
{ 
  return m_pData->tilt;
}

void CPhilipsDirectoryItem::setMatrixID(const quint32 matrixID)
{
  m_pData->frame = matrixID2Frame(matrixID);
  m_pData->slice = matrixID2Slice(matrixID);
  m_pData->tilt  = matrixID2Tilt(matrixID);
}

void CPhilipsDirectoryItem::setDataBlock_Start(const qint64 offset)
{ 
  m_pData->dataBlock_Start = offset;
}

void CPhilipsDirectoryItem::setDataBlock_End(const qint64 offset)
{ 
  m_pData->dataBlock_End = offset;
}

void CPhilipsDirectoryItem::setCompressionFlag(const CPhilipsDirectoryItem::CompressionFlag flag)
{
  m_pData->compressionFlag = flag;
}

void CPhilipsDirectoryItem::setContentFlag(const CPhilipsDirectoryItem::ContentFlag flag)
{
  m_pData->contentFlag = flag;
}

void CPhilipsDirectoryItem::setFrame(const short frame)
{ 
  m_pData->frame = frame;
}

void CPhilipsDirectoryItem::setSlice(const short slice)
{ 
  m_pData->slice = slice;
}

void CPhilipsDirectoryItem::setTilt(const short tilt)
{ 
  m_pData->tilt = tilt;
}

bool CPhilipsDirectoryItem::readSubHeader(CPhilipsSubHeader*& subHeader)
{
  ENTER();
  bool result = false;

  if(m_pData->file && m_pData->file->isReadable())
  {
    // check if we have a cached sub header ready already so that
    // we can take that one instead of loading the sub header once
    // more from scratch
    if(m_pData->cachedSubHeader)
    {
      // lets prepare the SubHeader depending on the type of the
      // PhilipsFile
      switch(m_pData->file->subHeaderType())
      {
        case CPhilipsSubHeader::Image:
          subHeader = new CPhilipsSubHeaderImage(*static_cast<CPhilipsSubHeaderImage*>(m_pData->cachedSubHeader));
          result = true;
        break;

        case CPhilipsSubHeader::Sinogram:
          subHeader = new CPhilipsSubHeaderSinogram(*static_cast<CPhilipsSubHeaderSinogram*>(m_pData->cachedSubHeader));
          result = true;
        break;          

        case CPhilipsSubHeader::Syntegra:
          subHeader = new CPhilipsSubHeaderSyntegra(*static_cast<CPhilipsSubHeaderSyntegra*>(m_pData->cachedSubHeader));
          result = true;
        break;          

        case CPhilipsSubHeader::Listmode:
          subHeader = new CPhilipsSubHeaderListmode(*static_cast<CPhilipsSubHeaderListmode*>(m_pData->cachedSubHeader));
          result = true;
        break;          

        case CPhilipsSubHeader::Unknown:
          // do nothing
          E("Unknown subheader type found!");
        break;
      }

      // set the mediodata object for the newly created subheader
      subHeader->setMedIOData(m_pData->file);
      subHeader->setDirectoryItem(this);
    }
    else
    {
      // lets prepare the SubHeader depending on the type of the
      // PhilipsFile
      switch(m_pData->file->subHeaderType())
      {
        case CPhilipsSubHeader::Image:
          subHeader = new CPhilipsSubHeaderImage(m_pData->file, this);
        break;

        case CPhilipsSubHeader::Syntegra:
          subHeader = new CPhilipsSubHeaderSyntegra(m_pData->file, this);
        break;

        case CPhilipsSubHeader::Sinogram:
          subHeader = new CPhilipsSubHeaderSinogram(m_pData->file, this);
        break;

        case CPhilipsSubHeader::Listmode:
          subHeader = new CPhilipsSubHeaderListmode(m_pData->file, this);
        break;

        default:
          E("Philips type isn't specified or not supported yet.");
      }

      if(subHeader)
      {
        if(subHeader->load() == false)
        {
          delete subHeader;
          subHeader = NULL;
        }
        else
          result = true;
      }
    }
  }

  RETURN(result);
  return result;
}

bool CPhilipsDirectoryItem::readMatrix(QByteArray*& matrixData)
{
  ENTER();
  bool result = false;
  char* data = NULL;
  unsigned int dataLen = 0;

  // we use our method operating on a raw char pointer
  if(readMatrix(data, dataLen) && dataLen > 0)
  {
    // copy the raw data into a QByteArray
    matrixData = new QByteArray(data, dataLen);
    delete [] data;
    result = true;
  }

  RETURN(result);
  return result;
}

bool CPhilipsDirectoryItem::readMatrix(char*& matrixData, unsigned int& matrixSize)
{
  ENTER();
  bool result = true;

  // clear the ptrs first
  matrixData = NULL;
  matrixSize = 0;

  // check if the file associated with this item is readable or
  // not
  if(m_pData->file && (m_pData->file->isReadable() == false))
  {
    RETURN(false);
    return false;
  }

  // we load the MatrixData out of the Philipsfile manually but only
  // if the content flag  of this item is set to "used"
  if(m_pData->contentFlag == Used)
  {
    // before we can read in the raw data we need to read in
    // the Subheader so that we know the data type of our raw
    // data
    CPhilipsSubHeader* subHeader = NULL;
    if((readSubHeader(subHeader) == false) || subHeader == NULL)
    {
      RETURN(false);
      return false;
    }

    // the next step is to set the file handle to the matrix start.
    // This data is stored directly after the subheader
    if(m_pData->file->seek(m_pData->dataBlock_Start + subHeader->rawDataSize()) == false)
    {
      delete subHeader;

      RETURN(false);
      return false;
    }

    SHOWVALUE(m_pData->file->pos());

    // then we allocate some memory for loading the matrixdata
    // this should be:
    matrixSize = m_pData->dataBlock_End - m_pData->dataBlock_Start - subHeader->rawDataSize() + PHILIPS_BLOCKSIZE;
    matrixData = new char[matrixSize];

    // now we read in the whole data at once
    if(m_pData->file->read(matrixData, matrixSize) == matrixSize)
    {
      // now we find out if this machine requires byte swapping or
      // not depending on the input data type and the sysinfo
      int dataTypeSize = sizeof(char);
      bool byteSwapping = false;

      switch(subHeader->datype())
      {
        case CPhilipsSubHeader::UnknownDataType:
        case CPhilipsSubHeader::ByteData:
          // nothing to do
        break;

        // Signed and Unsigned short is a 16bit big endian value
        case CPhilipsSubHeader::SignedShort:
        case CPhilipsSubHeader::UnsignedShort:
        {
          dataTypeSize = sizeof(quint16);
          byteSwapping = (QSysInfo::ByteOrder != QSysInfo::BigEndian);
        }
        break;
  
        // float is a 32bit big endian value
        case CPhilipsSubHeader::Float:
        {
          dataTypeSize = sizeof(float);
          byteSwapping = (QSysInfo::ByteOrder != QSysInfo::BigEndian);
        }
        break;
          
        default:
        {
          D("Data type not supported");
          result = false;
        }
      }

      // before caring about byte swapping the data we check the
      // compression flag and eventually uncompress the whole matrixData
      // first
      if(m_pData->compressionFlag == Compressed)
      {
        STARTCLOCK("uncompress");

        quint32 uncompressedMatrixSize = subHeader->xdim() * subHeader->ydim() * dataTypeSize;
        QByteArray compressedData = QByteArray::fromRawData(matrixData, matrixSize);

        // the qUncompress() of Qt4 requires us to add a special header
        // with the expected final size of the uncompressed data in bytes
        QByteArray zlibHeader;
        QDataStream stream(&zlibHeader, QIODevice::WriteOnly);
        stream << uncompressedMatrixSize;
        compressedData.prepend(zlibHeader);

        // lets uncompress the data
        QByteArray uncompressedData = qUncompress(compressedData);

        // lets copy the uncompressedData to matrixData
        delete[] matrixData;
        matrixSize = uncompressedData.size();
        matrixData = new char[matrixSize];
        memcpy(matrixData, uncompressedData.constData(), matrixSize);

        STOPCLOCK("uncompress");
      }

      // lets perform the byte swapping if required
      if(byteSwapping == true)
      {
        STARTCLOCK("byteswap");

        switch(dataTypeSize)
        {
          // 32bit data
          case sizeof(quint32):
            bswap_matrix<quint32>(reinterpret_cast<const quint32*>(matrixData), matrixSize, reinterpret_cast<quint32*>(matrixData));
          break;

          // 16bit data
          case sizeof(quint16):
            bswap_matrix<quint16>(reinterpret_cast<const quint16*>(matrixData), matrixSize, reinterpret_cast<quint16*>(matrixData));
          break;

          default:
            E("invalid dataTypeSize: %d", dataTypeSize);
          break;
        }

        STOPCLOCK("byteswap");
      }
      
      result = true;
    }

    // now we delete the subHeader we loaded temporarly
    delete subHeader;

    if(result == false)
    {
      delete matrixData;
      matrixData = NULL;
      matrixSize = 0;
    }
  }
  else
    result = false;

  RETURN(result);
  return result;
}

bool CPhilipsDirectoryItem::readMatrix(QByteArray*& matrixData, CPhilipsSubHeader*& subHeader)
{
  ENTER();
  bool result = false;
  char* data = NULL;
  unsigned int dataLen = 0;

  // we use our method operating on a raw char pointer
  if(readMatrix(data, dataLen, subHeader) && dataLen > 0)
  {
    // copy the raw data into a QByteArray
    matrixData = new QByteArray(data, dataLen);
    delete [] data;
    result = true;
  }

  RETURN(result);
  return result;
}

bool CPhilipsDirectoryItem::readMatrix(char*& data, unsigned int& len, CPhilipsSubHeader*& subHeader)
{
  ENTER();
  bool result = false;

  // read out the subHeader first
  if(readSubHeader(subHeader))
  {
    // we use our method operating on a raw char pointer
    if(readMatrix(data, len) && len > 0)
      result = true;
  }

  RETURN(result);
  return result;
}

bool CPhilipsDirectoryItem::writeSubHeader(const CPhilipsSubHeader& subHeaer)
{
  ENTER();
  bool result = false;

  m_pData->cacheSubHeader(subHeaer);

  if(m_pData->cachedSubHeader != NULL)
     result = m_pData->cachedSubHeader->save();

  RETURN(result);
  return result;
}

void CPhilipsDirectoryItemPrivate::cacheSubHeader(const CPhilipsSubHeader& subHeader)
{
  ENTER();

  if(cachedSubHeader != NULL)
    *cachedSubHeader = *&subHeader;
  else
  {
    switch(file->subHeaderType())
    {
      case CPhilipsSubHeader::Image:
        cachedSubHeader = new CPhilipsSubHeaderImage(*static_cast<const CPhilipsSubHeaderImage*>(&subHeader));
      break;

      case CPhilipsSubHeader::Sinogram:
        cachedSubHeader = new CPhilipsSubHeaderSinogram(*static_cast<const CPhilipsSubHeaderSinogram*>(&subHeader));
      break;

      case CPhilipsSubHeader::Syntegra:
        cachedSubHeader = new CPhilipsSubHeaderSyntegra(*static_cast<const CPhilipsSubHeaderSyntegra*>(&subHeader));
      break;

      case CPhilipsSubHeader::Listmode:
        cachedSubHeader = new CPhilipsSubHeaderListmode(*static_cast<const CPhilipsSubHeaderListmode*>(&subHeader));
      break;

      case CPhilipsSubHeader::Unknown:
        E("unknown subheader type found");
      break;
    }
  }

  if(cachedSubHeader)
  {
    cachedSubHeader->setMedIOData(file);
    cachedSubHeader->setDirectoryItem(dirItem);
  }

  LEAVE();
}

void CPhilipsDirectoryItem::subHeaderWritten(const CPhilipsSubHeader& subHeader)
{
  ENTER();

  if(&subHeader != m_pData->cachedSubHeader)
  {
    m_pData->cacheSubHeader(subHeader);
  }

  LEAVE();
}


bool CPhilipsDirectoryItem::writeMatrix(const QByteArray& matrixData)
{
  ENTER();
  bool result;

  result = writeMatrix(matrixData.data(), matrixData.size());

  RETURN(result);
  return result;
}

bool CPhilipsDirectoryItem::writeMatrix(const QByteArray& matrixData,
                                        CPhilipsSubHeader::Data_Type dataType)
{
  ENTER();
  bool result;

  result = writeMatrix(matrixData.data(), matrixData.size(), dataType);

  RETURN(result);
  return result;
}

bool CPhilipsDirectoryItem::writeMatrix(const char* matrixData, unsigned int matrixSize)
{
  ENTER();
  bool result;

  result = writeMatrix(matrixData, matrixSize, CPhilipsSubHeader::UnknownDataType); 

  RETURN(result);
  return result;
}

bool CPhilipsDirectoryItem::writeMatrix(const QByteArray& matrixData, const CPhilipsSubHeader& subHeader)
{
  ENTER();
  bool result;

  result = writeMatrix(matrixData.data(), matrixData.size(), subHeader);

  RETURN(result);
  return result;
}

bool CPhilipsDirectoryItem::writeMatrix(const char* matrixData, unsigned int matrixSize,
                                        CPhilipsSubHeader::Data_Type dataType)
{
  ENTER();
  bool result = true;

  // check if the file associated with this item is writeable at all
  if(m_pData->file == NULL ||
     m_pData->file->isWritable() == false)
  {
    RETURN(false);
    return false;
  }

  // before we can write out the raw data we need to read in
  // the Subheader so that we know the data type of our raw
  // data but before we actually read it out we check the status
  // of the directory item and eventually generate a new subHeader if
  // there isn't already one.
  CPhilipsSubHeader* subHeader = NULL;

  SHOWVALUE(m_pData->contentFlag);
  SHOWVALUE(m_pData->compressionFlag);
  SHOWVALUE(m_pData->dataBlock_Start);
  SHOWVALUE(m_pData->dataBlock_End);
  SHOWPOINTER(m_pData->cachedSubHeader);

  // try to read the subheader first
  if(dataType == CPhilipsSubHeader::UnknownDataType ||
     m_pData->contentFlag != Unused ||
     m_pData->dataBlock_End != 0)
  {
    readSubHeader(subHeader);
  }

  SHOWPOINTER(subHeader);
  
  if(subHeader == NULL &&
     m_pData->contentFlag == Unused &&
     m_pData->dataBlock_End == 0)
  {
    SHOWVALUE(m_pData->file->subHeaderType());

    switch(m_pData->file->subHeaderType())
    {
      case CPhilipsSubHeader::Image: 
        subHeader = new CPhilipsSubHeaderImage(m_pData->file, this);
      break;

      case CPhilipsSubHeader::Sinogram:  
        subHeader = new CPhilipsSubHeaderSinogram(m_pData->file, this);
      break;

      case CPhilipsSubHeader::Syntegra:  
        subHeader = new CPhilipsSubHeaderSyntegra(m_pData->file, this);
      break;

      case CPhilipsSubHeader::Listmode:  
        subHeader = new CPhilipsSubHeaderListmode(m_pData->file, this);
      break;

      case CPhilipsSubHeader::Unknown:
        // do nothing
      break;
    }
  }

  if(subHeader == NULL)
    result = false;
  else
  {
    if(subHeader->datype() != dataType &&
       dataType != CPhilipsSubHeader::UnknownDataType)
    {
      subHeader->setDatype(dataType);
    }

    result = writeMatrix(matrixData, matrixSize, *subHeader);
  }

  // make sure to delete the new subHeader
  delete subHeader;
  
  RETURN(result);
  return result;
}

bool CPhilipsDirectoryItem::writeMatrix(const char* matrixData, unsigned int matrixSize,
                                        const CPhilipsSubHeader& subHeader)
{
  ENTER();
  bool result = true;
  
  // check if the file associated with this item is writeable at all
  if(m_pData->file == NULL || m_pData->file->isWritable() == false)
  {
    RETURN(false);
    return false;
  }

  // now that we have the proper subHeader here we make sure to write it to
  // out file prior to saving the matrixData
  if(writeSubHeader(subHeader) == false)
  {
    RETURN(false);
    return false;
  }

  // set the PhilipsFile handle to the correct position of
  // the matrix start. This should normally be
  // Matrix_SubHeader_StartPosition+subHeaderSize
  if(m_pData->file->seek(m_pData->dataBlock_Start+subHeader.rawDataSize()) == false)
  {
    RETURN(false);
    return false;
  }

  D("about to write matrix data");
  SHOWVALUE(m_pData->file->pos());

  // now we find out if this machine requires byte swapping or
  // not depending on the input data type and the sysinfo
  int dataTypeSize = sizeof(char);
  bool byteSwapping = false;

  switch(subHeader.datype())
  {
    case CPhilipsSubHeader::UnknownDataType:
    case CPhilipsSubHeader::ByteData:
      // nothing to do
    break;

    // Signed and Unsigned short is a 16bit big endian value
    case CPhilipsSubHeader::SignedShort:
    case CPhilipsSubHeader::UnsignedShort:
    {
      dataTypeSize = sizeof(quint16);
      byteSwapping = (QSysInfo::ByteOrder != QSysInfo::BigEndian);
    }
    break;
  
    // float is a 32bit big endian value
    case CPhilipsSubHeader::Float:
    {
      dataTypeSize = sizeof(float);
      byteSwapping = (QSysInfo::ByteOrder != QSysInfo::BigEndian);
    }
    break;
      
    default:
    {
      D("Data type not supported");
      result = false;
    }
  }

  // if we need byte swapping we do it NOW
  if(byteSwapping == true)
  {
    STARTCLOCK("byteswap");

    // before performing the byteswapping we create a copy of the
    // matrix data
    char* swappedMatrixData = NULL;

    switch(dataTypeSize)
    {
      // 32bit data
      case sizeof(quint32):
        swappedMatrixData = reinterpret_cast<char*>(bswap_matrix<quint32>(reinterpret_cast<const quint32*>(matrixData), matrixSize));
      break;

      // 16bit data
      case sizeof(quint16):
        swappedMatrixData = reinterpret_cast<char*>(bswap_matrix<quint16>(reinterpret_cast<const quint16*>(matrixData), matrixSize));
      break;

      default:
        E("invalid dataTypeSize: %d", dataTypeSize);
      break;
    }

    // lets exchange matrixData with swappedMatrixData
    matrixData = swappedMatrixData;

    STOPCLOCK("byteswap");
  }

#if 1
  // we always compress the data before we write it out to the file
  STARTCLOCK("compress");

  // create an uncompressedData bytearray to apply qCompress()
  QByteArray uncompressedData = QByteArray::fromRawData(matrixData, matrixSize);

  // lets compress the data
  QByteArray compressedData = qCompress(uncompressedData, 1);

  // the qCompress() of Qt4 adds the final size of the uncompressed
  // data to the first 4 bytes of the compressedData array. We need
  // to strip that to write it out.
  compressedData = compressedData.remove(0, 4);

  // set matrixSize to the new compressed size
  matrixSize = compressedData.size();

  // set the directory item as containing compressed data
  m_pData->compressionFlag = CPhilipsDirectoryItem::Compressed;

  STOPCLOCK("compress");
#else

  QByteArray compressedData = QByteArray::fromRawData(matrixData, matrixSize);

#endif
 
  // finally we write out the whole matrixData in one single run.
  if(m_pData->file->write(compressedData) == (qint64)matrixSize)
    result = true;
  else
    E("Error while trying to write compressedData to file");

  if(byteSwapping == true)
    delete[] matrixData;

  if(result)
  {
    // check if the data we have written is divideable through the PHILIPS_BLOCKSIZE
    // or we have to add some more NULL bytes until we have 512 byte aligned data
    // because the Philips standard defines Philips files to be always dividable through
    // 512 byte blocks.
    if((matrixSize % PHILIPS_BLOCKSIZE) != 0)
    {
      unsigned int fillLen = PHILIPS_BLOCKSIZE - (matrixSize % PHILIPS_BLOCKSIZE);
      char fillData[fillLen];

      memset(fillData, 0, fillLen*sizeof(char));
W("before: %ld", m_pData->file->pos());
      if(m_pData->file->write(fillData, fillLen) != (qint64)fillLen)
      {
        result = false;
        E("Error occurred while trying to write %d NULL bytes.", fillLen);
      }
      else
      {
        W("matrixSize(%d) %% PHILIPS_BLOCKSIZE(%d) != 0 - added %d NULL bytes", matrixSize, PHILIPS_BLOCKSIZE, fillLen);
        matrixSize += fillLen;
      }
    }

    if(result)
    {
      // at the end of our operation we calculate the new EndPosition
      m_pData->dataBlock_End = m_pData->dataBlock_Start+subHeader.rawDataSize()+matrixSize-PHILIPS_BLOCKSIZE;
      W("pos: %ld %ld %ld", m_pData->file->pos(), m_pData->dataBlock_End, subHeader.rawDataSize());
      m_pData->contentFlag = CPhilipsDirectoryItem::Used;
    }
  }

  RETURN(result);
  return result;
}
