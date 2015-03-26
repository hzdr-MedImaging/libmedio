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

//! @file CConcordeFile.cpp
//! @brief contains the implementation of the class CConcordeFile
//! @author Hagen Moelle

#include <QDataStream>

#include "CConcordeFile.h"
#include "CConcordeSinogram.h"
#include "CConcordeImage.h"
#include "CConcordeMainHeader.h"
#include "CConcordeMainHeaderSinogram.h"
#include "CConcordeMainHeaderImage.h"
#include "CConcordeFrameHeader.h"
#include "CMedIOData.h"
#include "CIntVector.h"
#include "CKeyParser.h"

#include <rtdebug.h>

#include "bswap.h"

class CConcordeFilePrivate
{
  public:
    CIntVector frameOrder;
    CConcordeFilePrivate(const QString& fileName);
};

CConcordeFilePrivate::CConcordeFilePrivate(const QString& fileName)
{
  ENTER();
  //at first get frame order from header file
  CKeyParser frameOrderParser;
  frameOrderParser.addSeparator(" ");
  frameOrderParser.addComment("#");
  frameOrderParser.addKey("frame", &frameOrder, CIntVector::wrapper_parseKeyValue);  
  if(!frameOrderParser.parse(fileName+".hdr"))
    E("Error on parsing frame order from headerfile!");
  LEAVE();
}

//  Class: CConcordeFile
//  Constructor: CConcordeFile
//!
//! constructs a CConcordeFile object
//!
////////////////////////////////////////////////////////////////////////////////
CConcordeFile::CConcordeFile(const QString& fileName)
  : CMedIOData(fileName),
    m_pCachedMainHeader(NULL)
{
  ENTER();
  m_pData = new CConcordeFilePrivate(fileName);
  LEAVE();
}

//  Class: CConcordeFile
//  Destructor:  CConcordeFile
//!
//! destructucts a CConcordeFile object.
//! 
////////////////////////////////////////////////////////////////////////////////
CConcordeFile::~CConcordeFile()
{
  ENTER();

  delete m_pCachedMainHeader;
  
  LEAVE();
}

CConcordeFile::CConcordeFile(const CConcordeFile& src)
  : CMedIOData(src.fileName())
{
  ENTER();

  // just set the cachedMainHeader pointer to NULL
  m_pData = new CConcordeFilePrivate(src.fileName());
  m_pCachedMainHeader = NULL;

  LEAVE();
}
  
CConcordeFile& CConcordeFile::operator=(const CConcordeFile&)
{
  ENTER();

  // do nothing

  LEAVE();
  return *this;
}

CMedIOData::Format CConcordeFile::dataFormat() const 
{ 
  return CMedIOData::ConcordeMicropet;
}

//  Class: CConcordeFile
//  Method: close
//!
//! This method closes the file
//!
//! @return true if successful otherwise false
////////////////////////////////////////////////////////////////////////////////
void CConcordeFile::close()
{
  ENTER();
  m_pCachedMainHeader->save();
  delete m_pCachedMainHeader;
  m_pCachedMainHeader = NULL;
  QFile::close();
  LEAVE();
  return;
}

bool CConcordeFile::readMainHeader(CConcordeMainHeader*& mainHeader)
{
  ENTER();
  bool result = false;
  if(!isOpen())
  {
    E("Can not read mainheader if file is closed");
    result = false;
    mainHeader = NULL;
  }
  else
  {
    D("Trying to read main header"); 
    //delete mainHeader;
    switch(m_pCachedMainHeader->fileType())
    {
      case CConcordeMainHeader::Sinogram:
      case CConcordeMainHeader::Normalization:
      case CConcordeMainHeader::Attenuation:
      {
        D("Setting new sinogram");
        mainHeader = new CConcordeMainHeaderSinogram(this);//(fileName() + ".hdr");
        result = true;
      }
      break;
      case CConcordeMainHeader::Image:
      case CConcordeMainHeader::MuMap:
      {
        //#warning "filename is wrong but not used"
        D("Setting new image");
        mainHeader = new CConcordeMainHeaderImage(this);//(fileName() + ".hdr");
        result = true;
      }
      break;
      default:
      {
        E("File type is not yet supported");
        result = false;
      }
      break;
    }
    if(result)
        *mainHeader = *m_pCachedMainHeader;
  }
  RETURN(result);
  return result;
}

bool CConcordeFile::readSubHeader(CConcordeFrameHeader*& subHeader, int frame)
{
  ENTER();
  bool result = false;
  if(!isOpen())
  {
    W("Can not read mainheader if file is closed");
    result = false;
    subHeader = NULL;
  }
  else
  {
    //TODO: check if frame is in between 1 and number of frames in study
    W("TODO: check if frame is in between 1 and number of frames in study");
    //subHeader = new CConcordeFrameHeader(this, frame-1);
    subHeader = new CConcordeFrameHeader(this, m_pData->frameOrder[frame-1]);
    if(subHeader)
      result = true;
  }
  RETURN(result);
  return result;
}

//  Class: CConcordeFile
//  Method: isoftype
//!
//! This method tries to identify the format of the given file.
//! 
//! @return type of file or unknown if unknown or error
////////////////////////////////////////////////////////////////////////////////
bool CConcordeFile::isOfType(const QString& file)
{
  ENTER();
  bool result = false;

  // try to get Headerinfo on Sinogramfile
  CConcordeMainHeaderSinogram head(file+".hdr");
  switch(head.fileType())
  {
    case CConcordeMainHeader::Sinogram:
    case CConcordeMainHeader::Normalization:
    case CConcordeMainHeader::Attenuation:
    case CConcordeMainHeader::Image:
    case CConcordeMainHeader::MuMap:
      result = true;
    break;
  }

  RETURN(result);
  return result;
}

CMedIOData* CConcordeFile::createFromFile(const QString& fileName)
{
  ENTER();
  CMedIOData* mData = NULL;  
  CConcordeFile::FileType ftype = CConcordeFile::Unknown;

  // try to get Headerinfo on Sinogramfile
  CConcordeMainHeaderSinogram head(fileName+".hdr");
  switch(head.fileType())
  {
    case CConcordeMainHeader::Sinogram:
    case CConcordeMainHeader::Normalization:
    case CConcordeMainHeader::Attenuation:
      ftype = CConcordeFile::ConcordeMicropet_Sinogram;
    break;

    case CConcordeMainHeader::Image:
    case CConcordeMainHeader::MuMap:
      ftype = CConcordeFile::ConcordeMicropet_Image;
    break;

    case CConcordeMainHeader::Unknown:
      ftype = CConcordeFile::Unknown;
    break;
  }

  switch(ftype)
  {
    case CConcordeFile::ConcordeMicropet_Sinogram:
    {
      D("Creating concorde microPET sinogram");
      mData = new CConcordeSinogram(fileName);
    }
    break;

    case CConcordeFile::ConcordeMicropet_Image:
    {
      D("Creating concorde microPET image");
      mData = new CConcordeImage(fileName);
    }
    break;

    case CConcordeFile::Unknown:
      W("ConcordeFile Format could not be identified.");
    break;
  }

  if(mData == NULL) 
    D("An error occured when creating image");

  RETURN(mData);
  return mData;
}

bool CConcordeFile::readMatrix(QByteArray*& matrixData, short frame)
{
    ENTER();
    bool result = false;
    matrixData = NULL;
    if(isOpen())
    {
        char* pTmp = NULL;
        unsigned int iFrameSize = 0;
        if(readMatrix(pTmp, iFrameSize, frame))
        {
            matrixData = new QByteArray;
            matrixData->setRawData(pTmp, iFrameSize);
            result = true;
        }
    }
    RETURN(result);
    return result;
}

bool CConcordeFile::readMatrix(QByteArray*& matrixData, CConcordeFrameHeader*& subHeader, short frame)
{
    ENTER();
    bool result = false;
    matrixData = NULL;
    if(isOpen())
        if(readSubHeader(subHeader, frame))
        {
            char* pTmp = NULL;
            unsigned int iFrameSize = 0;
            if(readMatrix(pTmp, iFrameSize, frame))
            {
                matrixData = new QByteArray;
                matrixData->setRawData(pTmp, iFrameSize);
                result = true;
            }
        }
    RETURN(result);
    return result;
}

bool CConcordeFile::readMatrix(char*& matrixData, unsigned int& length, CConcordeFrameHeader*& subHeader, short frame)
{
    ENTER();
    bool result = false;
    if(isOpen())
        if(readSubHeader(subHeader, frame))
            if(readMatrix(matrixData, length, frame))
                result = true;
    RETURN(result);
    return result;
}

//  Class: CConcordeFile
//  Method: readMatrix
//!
//! This method loads the data of a sinogram from file to memory and constructs
//! a QByteArray which holds the data. The user is responsible for destruction
//! of the object.
//!
//! @return data of specific frame in sinogram or NULL on error
////////////////////////////////////////////////////////////////////////////////
bool CConcordeFile::readMatrix(char*& matrixData, unsigned int& matrixSize, short frame)
{
  ENTER();
  bool result = true;

  // clear the ptrs first
  matrixData = NULL;
  matrixSize = 0;

  // check if file is readable
  if(isReadable() == false)
  {
    RETURN(false);
    return false;
  }

  CConcordeMainHeader* head = m_pCachedMainHeader;
  if(head == NULL)
  {
    RETURN(false);
    return false;
  }
  
  // check if desired frame exists
  if(frame > head->totalFrames())
  {
    RETURN(false);
    return false;
  }
  
  // set readpointer to appropriate frame
  if(seek(static_cast<qint64>(frame-1)*m_pCachedMainHeader->frameSize()) == false)
  {
    RETURN(false);
    return false;
  }

  // then we allocate some memory for loading the matrixdata
  // this should be:
  matrixSize = m_pCachedMainHeader->frameSize();
  matrixData = new char[matrixSize];

  // now we read in the whole data at once
  if(QFile::read(matrixData, matrixSize) == matrixSize)
  {
    // now we find out if this machine requires byte swapping or
    // not depending on the input data type and the sysinfo
    int dataTypeSize = sizeof(char);
    bool byteSwapping = false;

    switch(head->dataType())
    {
      case CConcordeMainHeader::UnknownDataType:
      case CConcordeMainHeader::Byte:
        // nothing to do
      break;

      case CConcordeMainHeader::IntelShort:
      {
        dataTypeSize = sizeof(quint16);
        byteSwapping = (QSysInfo::ByteOrder != QSysInfo::LittleEndian);
      }
      break;
    
      case CConcordeMainHeader::IntelInt:
      {
        dataTypeSize = sizeof(quint32);
        byteSwapping = (QSysInfo::ByteOrder != QSysInfo::LittleEndian);
      }
      break;

      case CConcordeMainHeader::IntelFloat:
      {
        dataTypeSize = sizeof(float);
        byteSwapping = (QSysInfo::ByteOrder != QSysInfo::LittleEndian);
      }
      break;

      case CConcordeMainHeader::IEEEFloat:
      {
        dataTypeSize = sizeof(float);
        byteSwapping = (QSysInfo::ByteOrder != QSysInfo::BigEndian);
      }
      break;

      case CConcordeMainHeader::SunShort:
      {
        dataTypeSize = sizeof(quint16);
        byteSwapping = (QSysInfo::ByteOrder != QSysInfo::BigEndian);
      }
      break;
  
      case CConcordeMainHeader::SunInt:
      {
        dataTypeSize = sizeof(quint32);
        byteSwapping = (QSysInfo::ByteOrder != QSysInfo::BigEndian);
      }
      break;
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
  else
    E("Error when seeking frame: %d at filepos: %lld", frame, ((qint64)(frame-1))*m_pCachedMainHeader->frameSize());

  if(result == false)
  {
    delete matrixData;
    matrixData = NULL;
    matrixSize = 0;
  }

  RETURN(result);
  return result;
}
