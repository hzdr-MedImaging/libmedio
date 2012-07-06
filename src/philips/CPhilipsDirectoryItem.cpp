/* -*- mode: c++; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 * vim:set ts=2 sw=2 expandtab: *********************************************
 *
 * libmedio - C++ I/O Library for loading/saving medical data formats
 * Copyright (C) 2006-2010 by Jens Langner <Jens.Langner@light-speed.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id$
 *
 **************************************************************************/
#include <QCoreApplication>
#include "CPhilipsDirectoryItem.h"
#include "CPhilipsFile.h"

#include "CPhilipsSubHeader.h"
#include "CPhilipsSubHeaderImage.h"
#include "CPhilipsSubHeaderSinogram.h"

#include <QDataStream>
#include <rtdebug.h>
#include <iostream>
// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CPhilipsDirectoryItemPrivate
{
  public:
    void cacheExtendedMainHeader(const CPhilipsExtendedMainHeader& extendedMainHeader);
    void cacheSubHeader(const CPhilipsSubHeader& subHeader);
    
    CPhilipsDirectoryItem* dirItem; // pointer to the directory Item
    CPhilipsFile* file;             // pointer to the PhilipsFile
    CPhilipsSubHeader* cachedSubHeader;  // pointer to a cached SubHeader object
    CPhilipsExtendedMainHeader* cachedExtendedMainHeader;  // pointer to a cached ExtendedMainHeader object

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

    if(matrixID == PHILIPS_EXTENDED_HEADER)
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
  delete m_pData->cachedExtendedMainHeader;
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
  m_pData->cachedExtendedMainHeader = NULL;
  
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
          (matrixID() == PHILIPS_EXTENDED_HEADER));
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

void CPhilipsDirectoryItem::setDataBlock_Start(const qint64 offset)
{ 
  m_pData->dataBlock_Start = offset;
}

void CPhilipsDirectoryItem::setDataBlock_End(const qint64 offset)
{ 
  m_pData->dataBlock_End = offset;
}

bool CPhilipsDirectoryItem::readExtendedMainHeader(CPhilipsExtendedMainHeader*& extendedMainHeader)
{
  ENTER();
  bool result = false;

  if(m_pData->file && m_pData->file->isReadable())
  {
    // check if we have a cached extendedMainHeader ready already so that
    // we can take that one instead of loading the extendedMainHeader once
    // more from scratch
    if(m_pData->cachedExtendedMainHeader)
    {
      extendedMainHeader = new CPhilipsExtendedMainHeader(*m_pData->cachedExtendedMainHeader);
      result = true;

      // set the mediodata object for the newly created extendedMainHeader
      extendedMainHeader->setMedIOData(m_pData->file);
      extendedMainHeader->setDirectoryItem(this);
    }
    else
    {
      extendedMainHeader = new CPhilipsExtendedMainHeader(m_pData->file, this);

      if(extendedMainHeader)
      {
        if(extendedMainHeader->load() == false)
        {
          delete extendedMainHeader;
          extendedMainHeader = NULL;
        }
        else
          result = true;
      }
    }
  }

  RETURN(result);
  return result;
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

        default:
          E("philips type isn't specified or not supported yet.");
        
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

        case CPhilipsSubHeader::Sinogram:
          subHeader = new CPhilipsSubHeaderSinogram(m_pData->file, this);
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

  // check if the file associated with this item is readable or
  // not
  if(m_pData->file && (m_pData->file->isReadable() == false))
  {
    RETURN(false);
    return false;
  }

/*
  // as we can't handle compressed data abort
  if(m_pData->compressionFlag == Compressed)
  {
    W("matrix data is compressed. abort reading");
    RETURN(false);
    return false;
    }
*/

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
      RETURN(false);
      return false;
    }

    SHOWVALUE(m_pData->file->pos());

    if(m_pData->compressionFlag == Compressed)
    {
      W("matrix data is compressed.");
      QByteArray uncompressedData;

      unsigned int compressedLength = m_pData->dataBlock_End - m_pData->dataBlock_Start - subHeader->rawDataSize() + PHILIPS_BLOCKSIZE;
      unsigned int uncompressedLength = 144*144*2;

      char tmp[compressedLength];
      qint64 bytesRead =  m_pData->file->read(tmp, compressedLength);

      QByteArray compressedData(tmp, bytesRead);
      char* n = reinterpret_cast<char*>(&uncompressedLength);

      QByteArray a;
      a.resize(4+bytesRead);

      QDataStream stream(&a, QIODevice::WriteOnly);
      stream.setVersion(QDataStream::Qt_4_5);
      stream << uncompressedLength;

      a.append(compressedData);
      //compressedData.prepend(QByteArray::number(uncompressedLength));
      uncompressedData = qUncompress(a);
      matrixSize = uncompressedData.size();
      memcpy(matrixData, uncompressedData.data(), matrixSize);


      QFile file("compressedStuff");
      
      file.open(QIODevice::WriteOnly);
      file.write(compressedData);
      file.close();
      qApp->exit(0);
    }
    else
    {
      // then we allocate some memory for loading the matrixdata
      // because dataBlock_End points to the last data block of the
      // record, we need to add the lenght of one data block to get
      // the size of the matrix
      matrixSize = m_pData->dataBlock_End - m_pData->dataBlock_Start - subHeader->rawDataSize() + PHILIPS_BLOCKSIZE;
      matrixData = new char[matrixSize];

      // then we process the matrix data that is associated with
      // this directoryitem. According to the LUMINARY TOOLKIT MANUAL
      // all data is stored using big endian byte ordering. As the QDataStream
      // is per default big endian, we don't have to set it to another byte order
      // and just use the QDataStream operators to ensure correct byte swapping
      switch(subHeader->datype())
      {
#if 0
        case CPhilipsSubHeader::ByteData:
        {
          if(m_pData->file->read(matrixData, matrixSize) > 0)
            result = true;
        }
        break;
#endif
        case CPhilipsSubHeader::SignedShort:
          //case CPhilipsSubHeader::UnsignedShort:
        {
          QByteArray bufArray(8192, 0); // read 8KB chunks
          quint16* ptr = (quint16*)matrixData;
          for(unsigned int read=0; read < matrixSize;)
          {
            unsigned int toRead = matrixSize-read >= 8192 ? 8192 : matrixSize-read;
            unsigned int curRead = m_pData->file->read(bufArray.data(), toRead);
            if(curRead != toRead)
            {
              result = false;
              break;
            }

            // check if the curRead value is divide able through our data type 
            ASSERT(curRead % sizeof(quint16) == 0);

            // now that we have our chunk we use a bufferStream to stream
            // out the values from it for making sure our data is correctly
            // converted regarding to little/big endianess
            QDataStream bufStream(bufArray);
            for(unsigned int i=0; i < curRead; i+=sizeof(quint16))
            {
              bufStream >> *ptr;
              ++ptr;
            }

            // increase our read counter
            read += curRead;
          }
        }
        break;

#if 0
        case CPhilipsSubHeader::Float:
        {
          QByteArray bufArray(8192, 0); // read 8KB chunks
          float* ptr = (float*)matrixData;
          for(unsigned int read=0; read < matrixSize;)
          {
            unsigned int toRead = matrixSize-read >= 8192 ? 8192 : matrixSize-read;
            unsigned int curRead = m_pData->file->read(bufArray.data(), toRead);
            if(curRead != toRead)
            {
              result = false;
              break;
            }

            // check if the curRead value is divide able through our data type 
            ASSERT(curRead % sizeof(float) == 0);

            // now that we have our chunk we use a bufferStream to stream
            // out the values from it for making sure our data is correctly
            // converted regarding to little/big endianess
            QDataStream bufStream(bufArray);

            // we have to set the QDataStream version to the Qt4.5 version
            // because with Qt4.6 the floating point precision changed and
            // otherwise causes our streaming to fail
            bufStream.setVersion(QDataStream::Qt_4_5);

            for(unsigned int i=0; i < curRead; i+=sizeof(float))
            {
              bufStream >> *ptr;
              ++ptr;
            }

            // increase our read counter
            read += curRead;
          }
        }
        break;
#endif

        default:
        {
          D("Data type is not supported yet.");
          result = false;
        }
      }
    }
    // now we delete the subHeader we loaded temporarly
    delete subHeader;
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

bool CPhilipsDirectoryItem::writeExtendedMainHeader(const CPhilipsExtendedMainHeader& extendedMainHeader)
{
  ENTER();
  bool result = false;
 
  m_pData->cacheExtendedMainHeader(extendedMainHeader);

  if(m_pData->cachedExtendedMainHeader != NULL)
    result = m_pData->cachedExtendedMainHeader->save();

  // now we calculate the new EndPosition
  if(result)
  {
    m_pData->dataBlock_End = m_pData->dataBlock_Start + extendedMainHeader.rawDataSize() - PHILIPS_BLOCKSIZE;
    m_pData->contentFlag = Header;
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

void CPhilipsDirectoryItemPrivate::cacheExtendedMainHeader(const CPhilipsExtendedMainHeader& extendedMainHeader)
{
  ENTER();

  if(cachedExtendedMainHeader)
    *cachedExtendedMainHeader = extendedMainHeader;
  else
    cachedExtendedMainHeader = new CPhilipsExtendedMainHeader(extendedMainHeader);

  if(cachedExtendedMainHeader)
  {
    cachedExtendedMainHeader->setMedIOData(file);
    cachedExtendedMainHeader->setDirectoryItem(dirItem);
  }

  LEAVE();
}

void CPhilipsDirectoryItemPrivate::cacheSubHeader(const CPhilipsSubHeader& subHeader)
{
  ENTER();

  switch(file->subHeaderType())
  {
    case CPhilipsSubHeader::Image:
    {
      if(cachedSubHeader)
        *static_cast<CPhilipsSubHeaderImage*>(cachedSubHeader) = *static_cast<const CPhilipsSubHeaderImage*>(&subHeader);
      else
        cachedSubHeader = new CPhilipsSubHeaderImage(*static_cast<const CPhilipsSubHeaderImage*>(&subHeader));
    }
    break;

    case CPhilipsSubHeader::Sinogram:
    {
      if(cachedSubHeader)
        *static_cast<CPhilipsSubHeaderSinogram*>(cachedSubHeader) = *static_cast<const CPhilipsSubHeaderSinogram*>(&subHeader);
      else
        cachedSubHeader = new CPhilipsSubHeaderSinogram(*static_cast<const CPhilipsSubHeaderSinogram*>(&subHeader));
    }
    break;

    default:
      E("Philips type isn't specified or not supportet yet.");
  }

  if(cachedSubHeader)
  {
    cachedSubHeader->setMedIOData(file);
    cachedSubHeader->setDirectoryItem(dirItem);
  }

  LEAVE();
}

void CPhilipsDirectoryItem::extendedMainHeaderWritten(const CPhilipsExtendedMainHeader& extendedMainHeader)
{
  ENTER();

  // if the extendedMainHeaderWritten is the same as our cached extendedMainHeader we don't
  // have to copy it again and can break out immediately
  if(&extendedMainHeader != m_pData->cachedExtendedMainHeader)
  {
    // otherwise we make sure the written extendedMainHeader is cached
    m_pData->cacheExtendedMainHeader(extendedMainHeader);
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

      default:
        E("Philips type isn't specified or not supported yet.");
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

  SHOWVALUE(m_pData->file->pos());

  // then we process the matrix data that is associated with
  // this directoryitem.
  // here we have to care about the correct endianess, so that
  // we convert BIG->LITTLE or vise versa depending on the
  // system we are running.
  switch(subHeader.datype())
  {
    case CPhilipsSubHeader::UnknownDataType:
    {
      E("No or an unknown data type was set for the matrix data of dirItem %08lx",
        convertToMatrixID(m_pData->slice, m_pData->frame, m_pData->tilt));
      
      result = false;
    }
    break;

    // write out Byte data. (1 byte)
    case CPhilipsSubHeader::ByteData:
    {
      if(m_pData->file->write(matrixData, matrixSize) == (qint64)matrixSize)
        result = true;
    }
    break;

    case CPhilipsSubHeader::SignedShort:
    {
      // the endianess conversion takes quite some time, so
      // what we do is to us a temporarly buffer to which we read
      // some data from our fileStream and read out to our QByteArray
      QByteArray bufArray(8192, 0); // write in 8KB chunks
      quint16* ptr = (quint16*)matrixData;
      for(unsigned int written=0; written < matrixSize;)
      {
        unsigned int toWrite = matrixSize-written >= 8192 ? 8192 : matrixSize-written;

        // check if the curRead value is divide able through our data type 
        ASSERT(toWrite % sizeof(quint16) == 0);

        // now that we have our chunk we use a bufferStream to stream
        // in the values to it for making sure our data is correctly
        // converted regarding to little/big endianess
        QDataStream bufStream(&bufArray, QIODevice::WriteOnly);
        for(unsigned int i=0; i < toWrite; i+=sizeof(quint16))
        {
          bufStream << *ptr;
          ++ptr;
        }

        // write out the data from our buffer to the file
        if(m_pData->file->write(bufArray.data(), toWrite) != (qint64)toWrite)
        {
          result = false;
          break;
        }
        
        // increase our read counter
        written += toWrite;
      }

    }
    break;

    default:
    {
      E("Data type not supported yet.");
      result = false;
    }
  }    

  if(result)
  {
    // check if the data we have written is divideable through the PHILIPS_BLOCKSIZE
    // or we have to add some more NULL bytes until we have 512 byte aligned data
    // because the Philips standard defines Philips files to be always dividable through
    // 512 byte blocks.
    if((matrixSize % PHILIPS_BLOCKSIZE) > 0)
    {
      unsigned int fillLen = PHILIPS_BLOCKSIZE - (matrixSize % PHILIPS_BLOCKSIZE);
      char fillData[fillLen];

      memset(fillData, 0, fillLen*sizeof(char));

      if(m_pData->file->write(fillData, fillLen) != (qint64)fillLen)
      {
        result = false;
        E("Error occurred while trying to write %d NULL bytes.", fillLen);
      }
      else
      {
        matrixSize += fillLen;
        W("matrixsize %% Philips_BLOCKSIZE != 0. added %d NULL bytes", fillLen);
      }
    }

    if(result)
    {
      // at the end of our operation we calculate the new EndPosition
      m_pData->dataBlock_End = m_pData->dataBlock_Start+subHeader.rawDataSize()+matrixSize-PHILIPS_BLOCKSIZE;
      m_pData->contentFlag = CPhilipsDirectoryItem::Used;
    }
  }

  RETURN(result);
  return result;
}


QDataStream& operator<<(QDataStream& stream, const CPhilipsDirectoryItem& dItem)
{
  ENTER();

  // first convert and write out the matrixID
  quint32 matrixID = convertToMatrixID(dItem.m_pData->slice,
                                       dItem.m_pData->frame,
                                       dItem.m_pData->tilt);
  stream << matrixID;
  
  // then we read out the rest
  quint32 dataBlock_Start = FilePos2PhilipsBlock(dItem.m_pData->dataBlock_Start);
  stream << dataBlock_Start;

  quint32 dataBlock_End = FilePos2PhilipsBlock(dItem.m_pData->dataBlock_End);
  stream << dataBlock_End;
  
  qint16 compressionFlag = static_cast<qint16>(dItem.m_pData->compressionFlag);
  stream << compressionFlag;

  qint16 contentFlag = static_cast<qint16>(dItem.m_pData->contentFlag);
  stream << contentFlag;

  D("DItem.Matrix_ID       : %08x (%d/%d/%d)", matrixID,
                                                      dItem.m_pData->slice,
                                                      dItem.m_pData->frame,
                                                      dItem.m_pData->tilt);

  D("DItem.DataBlock_Start : %lld (%lld)", FilePos2PhilipsBlock(dItem.m_pData->dataBlock_Start), dItem.m_pData->dataBlock_Start);
  D("DItem.DataBlock_End   : %lld (%lld)", FilePos2PhilipsBlock(dItem.m_pData->dataBlock_End), dItem.m_pData->dataBlock_End);
  D("DItem.compressionFlag : %d", dItem.m_pData->compressionFlag);
  D("DItem.contentFlag     : %d", dItem.m_pData->contentFlag);
  
  LEAVE();
  return stream;
}

QDataStream& operator>>(QDataStream& stream, CPhilipsDirectoryItem& dItem)
{
  ENTER();

  // first read and convert the matrixID
  quint32 matrixID;
  stream >> matrixID;
  dItem.m_pData->frame = matrixID2Frame(matrixID);
  dItem.m_pData->slice = matrixID2Slice(matrixID);
  dItem.m_pData->tilt  = matrixID2Tilt(matrixID);

  // then we read out the rest
  quint32 dataBlock_Start;
  stream >> dataBlock_Start;
  dItem.m_pData->dataBlock_Start = PhilipsBlock2FilePos(dataBlock_Start);

  quint32 dataBlock_End;
  stream >> dataBlock_End;
  dItem.m_pData->dataBlock_End = PhilipsBlock2FilePos(dataBlock_End);

  qint16 compressionFlag;
  stream >> compressionFlag;
  dItem.m_pData->compressionFlag = static_cast<CPhilipsDirectoryItem::CompressionFlag>(compressionFlag);

  qint16 contentFlag;
  stream >> contentFlag;
  dItem.m_pData->contentFlag = static_cast<CPhilipsDirectoryItem::ContentFlag>(contentFlag);

  // output some debug information.
#if defined(DEBUG)
  if(dItem.isExtendedHeader())
  {
    D("DItem.Matrix_ID       : %08x (extended header)", matrixID);
    D("DItem.DataBlock_Start : %lld (%lld)", dItem.m_pData->dataBlock_Start, FilePos2PhilipsBlock(dItem.m_pData->dataBlock_Start));
    D("DItem.DataBlock_End   : %lld (%lld)", dItem.m_pData->dataBlock_End, FilePos2PhilipsBlock(dItem.m_pData->dataBlock_End));
  }
  else
  {
    D("DItem.Matrix_ID       : %08x (%d/%d/%d)", matrixID,
                                                  dItem.m_pData->slice,
                                                  dItem.m_pData->frame,
                                                  dItem.m_pData->tilt);

    D("DItem.DataBlock_Start : %d (%ld)", dItem.m_pData->dataBlock_Start, FilePos2PhilipsBlock(dItem.m_pData->dataBlock_Start));
    D("DItem.DataBlock_End   : %d (%ld)", dItem.m_pData->dataBlock_End, FilePos2PhilipsBlock(dItem.m_pData->dataBlock_End));
    D("DItem.compressionFlag : %04x", dItem.m_pData->compressionFlag);
    D("DItem.contentFlag     : %04x", dItem.m_pData->contentFlag);
  }
#endif
  LEAVE();
  return stream;
}
