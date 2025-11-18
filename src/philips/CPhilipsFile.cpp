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

#include "CPhilipsFile.h"
#include "CPhilipsMainHeader.h"
#include "CPhilipsDirectory.h"
#include "CPhilipsSubHeaderImage.h"
#include "CPhilipsSubHeaderSinogram.h"
#include "CPhilipsSubHeaderSyntegra.h"
#include "CPhilipsSubHeaderListmode.h"
#include "CPhilipsListviewHeader.h"

#include <QDataStream>
#include <rtdebug.h>

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CPhilipsFilePrivate
{
  public:
    bool isPhilipsFile(CMedIOData* file) const;
    bool isPhilipsListModeFile(CPhilipsFile *file) const;
    bool syncMainHeader(CPhilipsFile *file) const;

    CPhilipsMainHeader::File_Type fileType;
    CPhilipsDirectory* directory;
    CPhilipsMainHeader* cachedMainHeader; // for speed reasons we cache the loaded main header
};

bool CPhilipsFilePrivate::isPhilipsFile(CMedIOData* file) const
{
  ENTER();
  bool isValidPhilipsFile = false;

  if(file != NULL &&
     file->isReadable())
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

bool CPhilipsFilePrivate::isPhilipsListModeFile(CPhilipsFile *file) const
{
  ENTER();
  bool result = false;

  if(file != NULL &&
     file->isReadable())
  {
    CPhilipsListviewHeader lvHeader(file);

    if(lvHeader.load())
    {
      QString magicNr = lvHeader.magicNumber();

      result = ((magicNr == "Xtal") ||
                (magicNr == "LOR4") ||
                (magicNr == "LOR8"));
    }
  }

  RETURN(result);
  return result;
}

bool CPhilipsFilePrivate::syncMainHeader(CPhilipsFile* file) const
{
  ENTER();
  bool result = false;

  CPhilipsMainHeader* mainHeader = NULL;
  if(file->readMainHeader(mainHeader))
  {
    if(mainHeader->minfrm() != file->minFrame() ||
       mainHeader->maxfrm() != file->maxFrame() ||
       mainHeader->minslc() != file->minSlice() ||
       mainHeader->maxslc() != file->maxSlice() ||
       mainHeader->ntilt()  != file->numTilts())
    {
      result = file->writeMainHeader(*mainHeader);
    }
    else
      result = true;

    delete mainHeader;
  }

  RETURN(result);
  return result;
}

CPhilipsFile::CPhilipsFile(const QString& filename, CPhilipsMainHeader::File_Type fileType)
  : CMedIOData(filename)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CPhilipsFilePrivate();
  m_pData->fileType = fileType;
  m_pData->directory = NULL;
  m_pData->cachedMainHeader = NULL;

  LEAVE();
}

CPhilipsFile::CPhilipsFile(CPhilipsMainHeader::File_Type fileType)
  : CMedIOData(QString())
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CPhilipsFilePrivate();
  m_pData->fileType = fileType;
  m_pData->directory = NULL;
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

bool CPhilipsFile::isOfType(const QString& filename)
{
  ENTER();
  bool result = false;

  CPhilipsFile file(filename);
  if(file.open(QIODevice::ReadOnly))
  {
    result = true;
    file.close();
  }

  RETURN(result);
  return result;
}

CMedIOData* CPhilipsFile::createFromFile(const QString& fileName)
{
  ENTER();

  CMedIOData* mData = NULL;

  if(isOfType(fileName))
    mData = new CPhilipsFile(fileName);

  RETURN(mData);
  return mData;
}

CMedIOData::Format CPhilipsFile::dataFormat() const
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

short CPhilipsFile::numSlices(short frame) const
{
  return m_pData->directory->numSlices(frame);
}

short CPhilipsFile::maxTilt(void) const
{
  return m_pData->directory->maxTilt();
}

short CPhilipsFile::minTilt(void) const
{
  return m_pData->directory->minTilt();
}

short CPhilipsFile::numTilts(void) const
{
  return m_pData->directory->numTilts();
}

bool CPhilipsFile::open(QIODevice::OpenModeFlag mode)
{
  ENTER();

  bool result = false;

  if(isOpen())
  {
    RETURN(false);
    return false;
  }

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
        D("Found philips format header.");

        // we first create the item directory and load it accordingly.
        m_pData->directory = new CPhilipsDirectory(this);
        if(m_pData->directory->load())
        {
          m_pData->cachedMainHeader = new CPhilipsMainHeader(this);

          // now that we have created our proper MainHeader we try
          // to load the header information
          if(m_pData->cachedMainHeader->load())
            result = true;
          else
            W("main header loading failed");
        }
        else
          W("directory loading failed");
      }
      else if(m_pData->isPhilipsListModeFile(this))
      {
        D("Found philips format header (listmode file)");

        // The listmode format doesn't contain a directory structure.
        // In order be able to read the main/sub header from such a file
        // we have to create a fake directory.
        m_pData->directory = new CPhilipsDirectory(this);
        if(m_pData->directory->loadFake())
        {
          D("Fake directory created");
          m_pData->cachedMainHeader = new CPhilipsMainHeader(this,
                                                             CPhilipsMainHeader::Listmode,
                                                             512); // offset to the main header

          if(m_pData->cachedMainHeader->load())
            result = true;
          else
            W("main header loading failed");
        }
        else
          W("creation of the fake directory failed");
      }
      else
        D("No philips format header found.");

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

    case CPhilipsMainHeader::Syntegra:
      type = CPhilipsSubHeader::Syntegra;
    break;

    case CPhilipsMainHeader::Listmode:
      type = CPhilipsSubHeader::Listmode;
    break;

    case CPhilipsMainHeader::VID:
    case CPhilipsMainHeader::OtherFile:
    case CPhilipsMainHeader::Rawmode:
      E("unsupported philips main header type");
    break;

    case CPhilipsMainHeader::Unknown:
      E("unknown philips main header type");
    break;
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

bool CPhilipsFile::readSubHeader(CPhilipsSubHeader*& subHeader,
                                 short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;
  ASSERT(m_pData->directory);

  subHeader = NULL;

  // before we are going to read the SubHeader from the philips file we
  // have to check wheter the file is correctly open in Read mode.
  if(isReadable() && m_pData->directory)
    result = m_pData->directory->readSubHeader(subHeader, frame, slice, tilt);
  
  RETURN(result);
  return result;
}

bool CPhilipsFile::readMatrix(QByteArray*& matrixData,
                              short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;
  ASSERT(m_pData->directory);
  
  matrixData = NULL;

  // we have to perform a search within the whole DirectoryList
  // to get the MatrixData but we just need to query the
  // Main directory here as it will forward the query to it's sub
  // directories
  if(isReadable() && m_pData->directory)
  {
    // if slice == -1 then we sum all slices of a specified frame
    if(slice == -1)
    {
      short minsl = minSlice();
      short maxsl = maxSlice();
      short incr = (maxsl-minsl)/(numSlices(frame)-1);
      for(short curSlice=minsl; curSlice <= maxsl; curSlice += incr)
      {
        QByteArray* sliceData = NULL;
        if(m_pData->directory->readMatrix(sliceData, frame, curSlice, tilt))
        {
          if(matrixData != NULL)
          {
            matrixData->append(*sliceData);
            delete sliceData;
          }
          else
            matrixData = sliceData;

          result = true;
        }
        else
        {
          delete matrixData;
          matrixData = NULL;
          result = false;
          break;
        }
      }
    }
    else
      result = m_pData->directory->readMatrix(matrixData, frame, slice, tilt);
  }

  RETURN(result);
  return result;
}

bool CPhilipsFile::readMatrix(char*& matrixData, unsigned int& len,
                              short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;
  ASSERT(m_pData->directory);

  matrixData = NULL;
  len = 0;

  // we have to perform a search within the whole DirectoryList
  // to get the MatrixData but we just need to query the
  // Main directory here as it will forward the query to it's sub
  // directories
  if(isReadable() && m_pData->directory)
  {
    // if slice == -1 then we sum all slices of a specified frame
    if(slice == -1)
    {
      // we do that by called the QByteArray version of that function
      // instead
      QByteArray* frameData = NULL;
      if(readMatrix(frameData, frame, slice, tilt) == true)
      {
        // now we copy frameData to a new char array
        len = frameData->size();
        matrixData = new char[len];
        memcpy(matrixData, frameData->constData(), len);

        delete frameData;
        result = true;
      }
    }
    else
      result = m_pData->directory->readMatrix(matrixData, len, frame, slice, tilt);
  }

  RETURN(result);
  return result;
}

bool CPhilipsFile::readMatrix(QByteArray*& matrixData, CPhilipsSubHeader*& subHeader,
                              short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;
  ASSERT(m_pData->directory);
  
  matrixData = NULL;
  subHeader = NULL;

  // we have to perform a search within the whole DirectoryList
  // to get the MatrixData but we just need to query the
  // Main directory here as it will forward the query to it's sub
  // directories
  if(isReadable() && m_pData->directory)
  {
    // if slice == -1 then we sum all slices of a specified frame
    if(slice == -1)
    {
      short minsl = minSlice();
      short maxsl = maxSlice();
      short incr = (maxsl-minsl)/(numSlices(frame)-1);

      // let us first get the subheader which is the same for all slices
      // (hopefully)
      if(m_pData->directory->readSubHeader(subHeader, frame, minsl, tilt))
      {
        for(short curSlice=minsl; curSlice <= maxsl; curSlice += incr)
        {
          QByteArray* sliceData = NULL;
          if(m_pData->directory->readMatrix(sliceData, frame, curSlice, tilt))
          {
            if(matrixData != NULL)
            {
              matrixData->append(*sliceData);
              delete sliceData;
            }
            else
              matrixData = sliceData;

            result = true;
          }
          else
          {
            delete matrixData;
            matrixData = NULL;
            result = false;
            break;
          }
        }
      }
    }
    else
      result = m_pData->directory->readMatrix(matrixData, subHeader, frame, slice, tilt);
  }

  RETURN(result);
  return result;
}

bool CPhilipsFile::readMatrix(char*& matrixData, unsigned int& len, CPhilipsSubHeader*& subHeader,
                              short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;
  ASSERT(m_pData->directory);

  matrixData = NULL;
  len = 0;

  // we have to perform a search within the whole DirectoryList
  // to get the MatrixData but we just need to query the
  // Main directory here as it will forward the query to it's sub
  // directories
  if(isReadable() && m_pData->directory)
  {
    // if slice == -1 then we sum all slices of a specified frame
    if(slice == -1)
    {
      // we do that by called the QByteArray version of that function
      // instead
      QByteArray* frameData = NULL;
      if(readMatrix(frameData, subHeader, frame, slice, tilt) == true)
      {
        // now we copy frameData to a new char array
        len = frameData->size();
        matrixData = new char[len];
        memcpy(matrixData, frameData->constData(), len);

        delete frameData;
        result = true;
      }
    }
    else
      result = m_pData->directory->readMatrix(matrixData, len, subHeader, frame, slice, tilt);
  }

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

bool CPhilipsFile::writeSubHeader(const CPhilipsSubHeader& subHeader,
                                  short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;

  ASSERT(m_pData->directory);

  // forward the write request to the MainDirectory which is going to manage
  // everything else for us
  if(m_pData->directory &&
     m_pData->directory->writeSubHeader(subHeader, frame, slice, tilt))
  {
    // make sure the slice/frame/tilt parameters in the mainheader
    // are in sync
    result = m_pData->syncMainHeader(this);
  }

  RETURN(result);
  return result;
}

bool CPhilipsFile::writeMatrix(const QByteArray& matrixData, 
                               short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;

  ASSERT(m_pData->directory);
  
  // forward the write request to the MainDirectory which is going to manage
  // everything else for us
  if(m_pData->directory &&
     m_pData->directory->writeMatrix(matrixData, frame, slice, tilt))
  {
    // make sure the frames/planes/gates/bedpos parameters in the mainheader
    // are in sync
    result = m_pData->syncMainHeader(this);
  }

  RETURN(result);
  return result;
}

bool CPhilipsFile::writeMatrix(const char* matrixData, unsigned int size,
                               short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;

  ASSERT(m_pData->directory);
  
  // forward the write request to the MainDirectory which is going to manage
  // everything else for us
  if(m_pData->directory && 
     m_pData->directory->writeMatrix(matrixData, size, frame, slice, tilt))
  {
    // make sure the frames/planes/gates/bedpos parameters in the mainheader
    // are in sync
    result = m_pData->syncMainHeader(this);
  }

  RETURN(result);
  return result;
}

bool CPhilipsFile::writeMatrix(const QByteArray& matrixData, const CPhilipsSubHeader& subHeader,
                               short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;

  ASSERT(m_pData->directory);
  
  // forward the write request to the MainDirectory which is going to manage
  // everything else for us
  if(m_pData->directory &&
     m_pData->directory->writeMatrix(matrixData, subHeader, frame, slice, tilt))
  {
    // make sure the frames/planes/gates/bedpos parameters in the mainheader
    // are in sync
    result = m_pData->syncMainHeader(this);
  }

  RETURN(result);
  return result;
}

bool CPhilipsFile::writeMatrix(const char* matrixData, unsigned int size, const CPhilipsSubHeader& subHeader,
                               short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;

  ASSERT(m_pData->directory);
  
  // forward the write request to the MainDirectory which is going to manage
  // everything else for us
  if(m_pData->directory && 
     m_pData->directory->writeMatrix(matrixData, size, subHeader, frame, slice, tilt))
  {
    // make sure the frames/planes/gates/bedpos parameters in the mainheader
    // are in sync
    result = m_pData->syncMainHeader(this);
  }

  RETURN(result);
  return result;
}

bool CPhilipsFile::writeMatrix(const QByteArray& matrixData, CPhilipsSubHeader::Data_Type type, 
                               short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;

  ASSERT(m_pData->directory);
  
  // forward the write request to the MainDirectory which is going to manage
  // everything else for us
  if(m_pData->directory &&
     m_pData->directory->writeMatrix(matrixData, type, frame, slice, tilt))
  {
    // make sure the frames/planes/gates/bedpos parameters in the mainheader
    // are in sync
    result = m_pData->syncMainHeader(this);
  }

  RETURN(result);
  return result;
}

bool CPhilipsFile::writeMatrix(const char* matrixData, unsigned int size, CPhilipsSubHeader::Data_Type type,
                               short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;

  ASSERT(m_pData->directory);
  
  // forward the write request to the MainDirectory which is going to manage
  // everything else for us
  if(m_pData->directory && 
     m_pData->directory->writeMatrix(matrixData, size, type, frame, slice, tilt))
  {
    // make sure the frames/planes/gates/bedpos parameters in the mainheader
    // are in sync
    result = m_pData->syncMainHeader(this);
  }

  RETURN(result);
  return result;
}

CPhilipsDirectory* CPhilipsFile::directory() const
{
  return m_pData->directory;
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

CPhilipsSubHeader* CPhilipsFile::createEmptySubHeader()
{
  ENTER();
  CPhilipsSubHeader* pEmptySubHeader = NULL;
  if(isOpen())
  {
    switch(subHeaderType())
    {
      case CPhilipsSubHeader::Image:
        pEmptySubHeader = new CPhilipsSubHeaderImage(this);
      break;

      case CPhilipsSubHeader::Sinogram:
        pEmptySubHeader = new CPhilipsSubHeaderSinogram(this);
      break;

      case CPhilipsSubHeader::Syntegra:
        pEmptySubHeader = new CPhilipsSubHeaderSyntegra(this);
      break;

      case CPhilipsSubHeader::Listmode:
        pEmptySubHeader = new CPhilipsSubHeaderListmode(this);
      break;

      case CPhilipsSubHeader::Unknown:
        E("unknown subheader type");
      break;
    }
  }

  RETURN(pEmptySubHeader);
  return pEmptySubHeader;
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
    m_pData->cachedMainHeader->setNframe(numFrames());
    m_pData->cachedMainHeader->setMinfrm(minFrame());
    m_pData->cachedMainHeader->setMaxfrm(maxFrame());
    m_pData->cachedMainHeader->setNslice(numSlices());
    m_pData->cachedMainHeader->setMinslc(minSlice());
    m_pData->cachedMainHeader->setMaxslc(maxSlice());
    m_pData->cachedMainHeader->setNtilt(numTilts());
    
    // set the CPhilipsFile object as the MedIOData object
    // of the cached header
    m_pData->cachedMainHeader->setMedIOData(this);
  }

  LEAVE();
}

bool CPhilipsFile::reWriteMainHeader(void)
{
  ENTER();
  bool result = false;

  if(m_pData->cachedMainHeader)
    result = m_pData->cachedMainHeader->save();
  else
  {
    CPhilipsMainHeader* mHeader;

    if(readMainHeader(mHeader))
    {
      result = mHeader->save();

      delete mHeader;
    }
  }

  RETURN(result);
  return result;
}
