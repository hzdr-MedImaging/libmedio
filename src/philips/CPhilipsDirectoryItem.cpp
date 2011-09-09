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
    // void cacheSubHeader(const CPhilipsSubHeader& subHeader);
    
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
    m_pData->frame = matrixID2Frame(matrixID);
    m_pData->slice = matrixID2Slice(matrixID);
    m_pData->tilt  = matrixID2Tilt(matrixID);
    D("created new DirItem (%d/%d/%d)", m_pData->frame, m_pData->slice, m_pData->tilt);
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
      case CPhilipsSubHeader::ByteData:
      {
        if(m_pData->file->read(matrixData, matrixSize) > 0)
          result = true;
      }
      break;

      case CPhilipsSubHeader::SignedShort:
      case CPhilipsSubHeader::UnsignedShort:
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
    D("DItem.DataBlock_Start : %d (%ld)", dItem.m_pData->dataBlock_Start, FilePos2PhilipsBlock(dItem.m_pData->dataBlock_Start));
    D("DItem.DataBlock_End   : %d (%ld)", dItem.m_pData->dataBlock_End, FilePos2PhilipsBlock(dItem.m_pData->dataBlock_End));
  }
  else
  {
    D("DItem.Matrix_ID       : %08x (%d/%d/%d)", matrixID,
                                                  dItem.m_pData->frame,
                                                  dItem.m_pData->slice,
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
