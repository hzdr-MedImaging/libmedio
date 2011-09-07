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

#include "CPhilipsFile.h"
#include "CPhilipsMainHeader.h"
#include "CPhilipsDirectory.h"

#include <QDataStream>
#include <rtdebug.h>

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CPhilipsFilePrivate
{
  public:
    bool isPhilipsFile(CMedIOData* file);

    CPhilipsMainHeader::File_Type fileType;
    CPhilipsDirectory* directory;
    CPhilipsMainHeader* cachedMainHeader; // for speed reasons we cache the loaded main header
};

bool CPhilipsFilePrivate::isPhilipsFile(CMedIOData* file)
{
  bool isValidPhilipsFile = false;

  if(file->isReadable())
  {
    QDataStream stream(file);
    
    // we check the first 6 bytes of the file
    // the bytes should be 0x01 0x00 0x00 0x00 0x00 0x16
    char firstBytes[6];
    if(stream.readRawData(firstBytes, 6) == 6)
    {
      if((firstBytes[0] == 0x01) &&
         (firstBytes[1] == 0x00) &&
         (firstBytes[2] == 0x00) &&
         (firstBytes[3] == 0x00) &&
         (firstBytes[4] == 0x00) &&
         (firstBytes[5] == 0x16))
      {
        isValidPhilipsFile = true;
      }
      else
        W("this is no philips file");
    }
  }

  RETURN(isValidPhilipsFile);
  return isValidPhilipsFile;
}

CPhilipsFile::CPhilipsFile(const QString& filename, CPhilipsMainHeader::File_Type fileType)
  : CMedIOData(filename)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CPhilipsFilePrivate();
  m_pData->fileType = fileType;
  m_pData->cachedMainHeader = NULL;
  m_pData->cachedMainHeader = NULL;

  LEAVE();
}

CPhilipsFile::~CPhilipsFile()
{
  ENTER();

  delete m_pData->directory;
  delete m_pData->cachedMainHeader;
  delete m_pData;

  LEAVE();
}

int CPhilipsFile::rtti() const
{
  return CMedIOData::Philips;
}

short CPhilipsFile::maxFrame(void) const
{
  return m_pData->directory->maxFrame();
}

short CPhilipsFile::minFrame(void) const
{
  return m_pData->directory->minFrame();
}

short CPhilipsFile::numFrames(void) const
{
  return m_pData->directory->numFrames();
}

short CPhilipsFile::maxSlice(void) const
{
  return m_pData->directory->maxSlice();
}

short CPhilipsFile::minSlice(void) const
{
  return m_pData->directory->minSlice();
}

short CPhilipsFile::numSlices(void) const
{
  return m_pData->directory->numSlices();
}

short CPhilipsFile::numTilts(void) const
{
  return m_pData->directory->numTilts();
}

bool CPhilipsFile::open(QIODevice::OpenModeFlag mode)
{
  ENTER();

  bool result = false;

  // first we check if we already have some valid MainHeader/MainDirectory
  // combination and if so we need to delete it first.
  delete m_pData->directory;
  m_pData->directory = NULL;
  delete m_pData->cachedMainHeader;
  m_pData->cachedMainHeader = NULL;

  // depending on the specified open mode we either read some META data from
  // an existing Philips file or we create an empty one
  if((((mode & QIODevice::ReadWrite) == QIODevice::ReadWrite) || // ReadWrite mode requested 
      ((mode & QIODevice::ReadOnly) == QIODevice::ReadOnly))  && // or ReadOnly mode requested
     exists()) // and file exists
  {
    // we open the file and read in the
    // main header and directory list of the philips file
    if(QFile::open(QIODevice::ReadOnly))
    {
      if(m_pData->isPhilipsFile(this))
      {
        m_pData->cachedMainHeader = new CPhilipsMainHeader(this);

        // now that we have created our proper MainHeader we try
        // to load the header information and then load the
        // Philips DirectoryList out of the PhilipsFile.
        if(m_pData->cachedMainHeader->load())
        {
          m_pData->directory = new CPhilipsDirectory(this);
          if(m_pData->directory->load())
          {
            // only if the directory loading also suceeded we
            // finally loaded the philips file
            result = true;
          }
        }
      }

      // close the file
      QFile::close();
    }
  }
  else if((mode & QIODevice::WriteOnly) == QIODevice::WriteOnly) // mode contains WriteOnly flag
  {
    // the file doesn't exist and therefore we do not have any
    // main header or directory list. so lets create some empty ones
    m_pData->directory = new CPhilipsDirectory(this);
    m_pData->cachedMainHeader = new CPhilipsMainHeader(this, m_pData->fileType);

    // make sure the file is removed upon opening it
    // we don't do a plain QFile::remove() here because otherwise
    // QFile will close and free our data.
    QFile::remove(fileName());
    result = true;
  }
  
  // only if we succeeded with our mainheader/dirlisting loading
  // we can assume everything worked out fine and reopen the file
  // with the user settings.
  if(result)
  {
    // to make the open operation a bit safer we mask out the IO operation
    // bits we don't need
    mode = static_cast<QIODevice::OpenModeFlag>(mode & ~(QIODevice::Append|QIODevice::Truncate|QIODevice::Text));

    // make sure the file is always openend in read mode also
    // because we need to read in data as we write as well 
    if((result = QFile::open(mode|QIODevice::ReadOnly)) == false)
      QFile::close();
  }

  if(result == false)
  {
    // otherwise we go and delete our main directory again
    if(m_pData->cachedMainHeader)
    {
      delete m_pData->cachedMainHeader;
      m_pData->cachedMainHeader = NULL;
    }    

    if(m_pData->cachedMainHeader)
    {
      delete m_pData->cachedMainHeader;
      m_pData->cachedMainHeader = NULL;
    }
  }

  SHOWPOINTER(m_pData->directory);
  SHOWPOINTER(m_pData->cachedMainHeader);

  RETURN(result);
  return result;
}

void CPhilipsFile::close()
{
  ENTER();

  // close the opened file and clean everything up
  QFile::close();

  if(m_pData->directory)
  {
    delete m_pData->directory;
    m_pData->directory = NULL;
  }

  if(m_pData->cachedMainHeader)
  {
    delete m_pData->cachedMainHeader;
    m_pData->cachedMainHeader = NULL;
  }      
  LEAVE();
}

CPhilipsMainHeader::File_Type CPhilipsFile::fileType()
{
  ENTER();
  CPhilipsMainHeader::File_Type type = CPhilipsMainHeader::Unknown;

  if(isOpen())
  {
    bool cachedMainHeaderUsed = true;
    CPhilipsMainHeader* mainHeader = m_pData->cachedMainHeader;
    SHOWPOINTER(mainHeader);
    if(mainHeader == NULL)
    {
      cachedMainHeaderUsed = false;
      if(readMainHeader(mainHeader) == false)
      {
        delete mainHeader;

        RETURN(CPhilipsMainHeader::Unknown);
        return CPhilipsMainHeader::Unknown;
      }
    }

    type = mainHeader->filtyp();

    if(cachedMainHeaderUsed == false)
      delete mainHeader;
  }

  RETURN(type);
  return type;
}

CPhilipsSubHeader::Type CPhilipsFile::subHeaderType()
{
  ENTER();
  CPhilipsSubHeader::Type type = CPhilipsSubHeader::Unknown;

  // we use the file type from the main header
  switch(fileType())
  {
    case CPhilipsMainHeader::Image:
      type = CPhilipsSubHeader::Image;
    break;

    case CPhilipsMainHeader::Sinogram:
      type = CPhilipsSubHeader::Sinogram;
    break;

    default:
    {
      W("Philips fileType couldn't be identified.");
    }
  }

  RETURN(type);
  return type;
}

bool CPhilipsFile::readMainHeader(CPhilipsMainHeader*& mainHeader)
{
  ENTER();

  bool result = false;

  if(isReadable())
  {
    // check if we have a cached main header somewhere around so
    // we can take that one instead of loading the main header from
    // scratch again
    if(m_pData->cachedMainHeader)
    {
      mainHeader = new CPhilipsMainHeader(*m_pData->cachedMainHeader);

      result = true;

      // we set the MedIOData object of the newly created main
      // header now
      mainHeader->setMedIOData(this);
    }
    else
    {
      mainHeader = new CPhilipsMainHeader(this, m_pData->fileType);
      if(mainHeader->load())
        result = true;
      else
        delete mainHeader;
    }
  }

  if(result == false)
    mainHeader = NULL;
  else
  {
    // if the loading was successfull we have to make sure to use that
    // new instance of the main header as the new cached one.
    if(m_pData->cachedMainHeader == NULL)
    {
      m_pData->cachedMainHeader = new CPhilipsMainHeader(*mainHeader);

      // set the CPhilipsFile object as the MedIOData object
      // of the cached header
      m_pData->cachedMainHeader->setMedIOData(this);
    }
  }

  RETURN(result);
  return result;
}

bool CPhilipsFile::readSubHeader(CPhilipsSubHeader*& subHeader, short slice,
                                 short frame, short tilt)
{
  ENTER();
  bool result = false;
  D("IN philips file readSubHeader");
  ASSERT(m_pData->directory);

  // before we are going to read the SubHeader from the philips file we
  // have to check wheter the file is correctly open in Read mode.
  if(isReadable() && m_pData->directory)
    result = m_pData->directory->readSubHeader(subHeader, slice, frame, tilt);
  
  RETURN(result);
  return result;
}

bool CPhilipsFile::readMatrix(QByteArray*& matrixData,
                              short slice, short frame, short tilt)
{
  ENTER();
  bool result = false;
  ASSERT(m_pData->directory);
  
  // we have to perform a search within the whole DirectoryList
  // to get the MatrixData but we just need to query the
  // Main directory here as it will forward the query to it's sub
  // directories
  if(isReadable() && m_pData->directory)
    result = m_pData->directory->readMatrix(matrixData, slice, frame, tilt);

  RETURN(result);
  return result;
}

bool CPhilipsFile::readMatrix(char*& matrixData, unsigned int& len,
                              short slice, short frame, short tilt)
{
  ENTER();
  bool result = false;

  ASSERT(m_pData->directory);

  // we have to perform a search within the whole DirectoryList
  // to get the MatrixData but we just need to query the
  // Main directory here as it will forward the query to it's sub
  // directories
  if(isReadable() && m_pData->directory)
    result = m_pData->directory->readMatrix(matrixData, len, slice, frame, tilt);

  RETURN(result);
  return result;
}

bool CPhilipsFile::readMatrix(QByteArray*& matrixData, CPhilipsSubHeader*& subHeader,
                              short slice, short frame, short tilt)
{
  ENTER();
  bool result = false;
  ASSERT(m_pData->directory);
  
  // we have to perform a search within the whole DirectoryList
  // to get the MatrixData but we just need to query the
  // Main directory here as it will forward the query to it's sub
  // directories
  if(isReadable() && m_pData->directory)
    result = m_pData->directory->readMatrix(matrixData, subHeader, slice, frame, tilt);

  RETURN(result);
  return result;
}

bool CPhilipsFile::readMatrix(char*& matrixData, unsigned int& len, CPhilipsSubHeader*& subHeader,
                              short slice, short frame, short tilt)
{
  ENTER();
  bool result = false;

  ASSERT(m_pData->directory);

  // we have to perform a search within the whole DirectoryList
  // to get the MatrixData but we just need to query the
  // Main directory here as it will forward the query to it's sub
  // directories
  if(isReadable() && m_pData->directory)
    result = m_pData->directory->readMatrix(matrixData, len, subHeader, slice, frame, tilt);

  RETURN(result);
  return result;
}

bool CPhilipsFile::writeMainHeader(CPhilipsMainHeader& mainHeader)
{
  ENTER();
  bool result;

  // we forward the write request to the main Header's own save
  // routines.
  result = mainHeader.save();
  
  RETURN(result);
  return result;
}

CPhilipsMainHeader* CPhilipsFile::createEmptyMainHeader()
{
  ENTER();
  CPhilipsMainHeader* pEmptyMainHaeder = NULL;
  if(isOpen())
  {
    pEmptyMainHaeder = new CPhilipsMainHeader(this);
  }

  RETURN(pEmptyMainHaeder);
  return pEmptyMainHaeder;
}

void CPhilipsFile::mainHeaderWritten(const CPhilipsMainHeader& mainHeader)
{
  ENTER();

  // now that a new main Header has been written we have to
  // update our cached copy accordingly.
  if(m_pData->cachedMainHeader)
    *m_pData->cachedMainHeader = *(&mainHeader);
  else
    m_pData->cachedMainHeader = new CPhilipsMainHeader(mainHeader);

  // in addition to that we have to place the correct slices/frames/tilts
  // stuff in the cached header to be totally correct.
  if(m_pData->cachedMainHeader)
  {
    // Please note that we do not specify any PLANES here as in Philips the
    // planes are normally integrated in one matrix file. So we have to
    // allow the user to specify the planes himself.
    m_pData->cachedMainHeader->setNframe(numFrames());
    m_pData->cachedMainHeader->setNslice(numSlices());
    m_pData->cachedMainHeader->setNtilt(numTilts());
    
    // set the CPhilipsFile object as the MedIOData object
    // of the cached header
    m_pData->cachedMainHeader->setMedIOData(this);
  }

  LEAVE();
}
