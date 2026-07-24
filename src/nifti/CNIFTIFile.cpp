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

#include "CNIFTIFile.h"
#include "CMedIOData.h"
#include "CNIFTI1MainHeader.h"
#include "CNIFTI2MainHeader.h"

#include <QDataStream> // NIfTI are pure binary files
#include <QFileInfo>

#include <rtdebug.h>

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers 

//=============================================================================================
// CNIFTIFilePrivate class is a private class that is used to store the private data of the CNIFTIFile class
// private class to add private variables to the CNIFTIFile class without exposing them in the public header file 

class CNIFTIFilePrivate {
  public:
    CNIFTIFile::NIFTIFormat iNIFTIformat;
    CNIFTIMainHeader::Type  iMainHeaderType;
    CNIFTIMainHeader*  cachedMainHeader; // for speed reasons we cache the loaded main header

    // methods
    bool syncMainHeader(CNIFTIFile* file) const;
};

//=============================================================================================
// Constructors and Destructors for the CNIFTIFile class
CNIFTIFile::CNIFTIFile(const QString& filename, CNIFTIMainHeader::Type fileType): CMedIOData(filename) {
    
    ENTER();

    // allocate data from our private instance class m_pData is a pointer to the private data of the CNIFTIFile class
    m_pData = new CNIFTIFilePrivate();
    m_pData->iNIFTIformat = CNIFTIFile::Undefined;
    m_pData->iMainHeaderType = fileType;
    m_pData->cachedMainHeader = NULL;
    
    setFileType(fileType);

    LEAVE();
}
//------------------------------------------------------------------------------------------------
CNIFTIFile::~CNIFTIFile() {
    ENTER();

    delete m_pData->cachedMainHeader;
    delete m_pData;

    LEAVE();
}

//=============================================================================================
// isOfType --> Static method to check if a file is of the correct type 
bool CNIFTIFile::isOfType(const QString& filename) {
  ENTER();

  bool result = false;

  // try to open the file (in read mode) and identify it as a NIfTI file
  CNIFTIFile file(filename);

  if(file.open(QIODevice::ReadOnly)) { 
    if(file.format() != CNIFTIFile::Undefined)
      result = true;

    file.close();
  }

  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
// createFromFile --> Static method to create an instance of the CNIFTIFile class from a file
CMedIOData* CNIFTIFile::createFromFile(const QString& fileName)
{
  ENTER();

  CMedIOData* mData = NULL;

  if(isOfType(fileName))
    mData = new CNIFTIFile(fileName);

  RETURN(mData);
  return mData;
}

//=============================================================================================
// dataFormat --> method to return the format of the data
CMedIOData::Format CNIFTIFile::dataFormat() const
{ 
  return CMedIOData::NIFTI;
}

//------------------------------------------------------------------------------------------------
// the file (PET scan files) open/close methods:

// 1. open --> method to open the file
bool CNIFTIFile::open(QIODevice::OpenModeFlag mode) {
  ENTER();
  bool result = false;
  
  // delete the cached main header if it exists
  delete m_pData->cachedMainHeader;
  m_pData->cachedMainHeader = NULL;

  // reading an existing file (ReadOnly or ReadWrite mode) and the file exists
  if((((mode & QIODevice::ReadWrite) == QIODevice::ReadWrite) || 
      ((mode & QIODevice::ReadOnly) == QIODevice::ReadOnly))  && 
     exists()) {
    if(QFile::open(QIODevice::ReadOnly)) {
      
      QDataStream stream(this);
      stream.setByteOrder(QDataStream::LittleEndian); // NIfTI is Little Endian 

      // read the first 4 bytes to determine the size of the header (NIfTI-1: 32-bit -> 348 bytes, NIfTI-2: 64-bit -> 540 bytes)
      qint32 sizeof_hdr; 
      stream >> sizeof_hdr;

      //*********************************************************** 
      // DEBUG to check the size of the header
      std::cout << "DEBUG: sizeof_hdr = " << sizeof_hdr << " : " << std::endl;
      //*********************************************************** 

      // Verify if it is a NIfTI-1 (348 byte)
      if(sizeof_hdr == 348 || sizeof_hdr == 1543569408) {
        D("Found NIfTI-1 file");
        std::cout << "NIFTI1" << std::endl;
        m_pData->iNIFTIformat = CNIFTIFile::NIFTI1;
        
        // Create the header object CNIFTIMainHeader and tell it to load the rest of the data:
        // Once the file is open and verified as a NIfTI file, we create a new CNIFTIMainHeader object 
        // and call its load() method to read the rest of the header data from the file. 
        // If the load is successful, we set result to true; otherwise, we delete the header object and set result to false.
        m_pData->cachedMainHeader = new CNIFTI1MainHeader(this);
        if(m_pData->cachedMainHeader->load()) {
          result = true;
        } else {
          std::cout << "blah" << std::endl;
          W("Error while loading NIfTI-1 header");
        }
      }
      // Verify if it is a NIfTI-2 (540 byte)
      else if(sizeof_hdr == 540 || sizeof_hdr == 469893120) {
        D("Found NIfTI-2 file");
        m_pData->iNIFTIformat = CNIFTIFile::NIFTI2;
        
        m_pData->cachedMainHeader = new CNIFTI2MainHeader(this);
        if(m_pData->cachedMainHeader->load()) {
          result = true;
        } else {
          W("Error while loading NIfTI-2 header");
        }
      }
      else {
         W("Magic number error: it's not a valid NIfTI file.");
      }
    
      QFile::close();
    }
  }

// Create a new file (WriteOnly)
  else if((mode & QIODevice::WriteOnly) == QIODevice::WriteOnly)
  {
    if(m_pData->iNIFTIformat != CNIFTIFile::Undefined) {
       // Create an empty header ready to be filled
       if (m_pData->iNIFTIformat == CNIFTIFile::NIFTI1) {
           m_pData->cachedMainHeader = new CNIFTI1MainHeader(this);
       } else if (m_pData->iNIFTIformat == CNIFTIFile::NIFTI2) {
           m_pData->cachedMainHeader = new CNIFTI2MainHeader(this);
       }
       result = true;
    } else {
      E("Format NIFTI unknown for writing");
    }

    if(result)
      QFile::remove(fileName());
  }

  // Reopen final file with the correct permissions requested by the user
  if(result) {
    mode = static_cast<QIODevice::OpenModeFlag>(mode & ~(QIODevice::Append|QIODevice::Truncate|QIODevice::Text)); // Mask bit to bit, to remove the flags we don't need
    
    if((result = QFile::open(mode|QIODevice::ReadOnly)) == false)
      QFile::close();
  }
    
  if(result == false) {
    delete m_pData->cachedMainHeader;
    m_pData->cachedMainHeader = NULL;
  }

  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
// 2. close --> method to close the file
void CNIFTIFile::close(void) {
    
  // close the opened file and clean everything up
  QFile::close();

  if(m_pData->cachedMainHeader)
  {
    delete m_pData->cachedMainHeader;
    m_pData->cachedMainHeader = NULL;
  }      
}
//------------------------------------------------------------------------------------------------
// Getter for the format of the file
CNIFTIFile::NIFTIFormat CNIFTIFile::format(void) const {
  return m_pData->iNIFTIformat;
}
//------------------------------------------------------------------------------------------------
// Getter for the type of the file
CNIFTIMainHeader::Type CNIFTIFile::fileType(void) const {
  return m_pData->iMainHeaderType;
}
//------------------------------------------------------------------------------------------------
// Setter for the type of the file
bool CNIFTIFile::setFileType(CNIFTIMainHeader::Type fileType) {
  m_pData->iMainHeaderType = fileType;
  return true;
}

//=============================================================================================
// Member Function to read out and write METADATA from the NIfTI files --> I/O
// To interprete the medical data, we need to read the main header and the voxel data (matrix) from the NIfTI file
bool CNIFTIFile::readMainHeader(CNIFTIMainHeader*& MainHeader) {
  ENTER();
  bool result = false;
  
  if(isReadable() && m_pData->cachedMainHeader) { // first we check if the file is readable and if there is already a cached main header in memory
    
    // Copy of the header saved in cache
    MainHeader = static_cast<CNIFTIMainHeader*>(m_pData->cachedMainHeader->clone());
    MainHeader->setMedIOData(this); //link the new header to the current file
    result = true;
  }
  
  if(result == false)
    MainHeader = NULL;

  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
bool CNIFTIFile::writeMainHeader(CNIFTIMainHeader& MainHeader) {
  ENTER();
  // save the header to the file
  bool result = MainHeader.save();
  RETURN(result);
  return result;
}

//------------------------------------------------------------------------------------------------
CNIFTIMainHeader* CNIFTIFile::createEmptyHeader(void) {
  ENTER();
  CNIFTIMainHeader* pEmptyHeader = NULL;
  
  if (m_pData->iNIFTIformat == CNIFTIFile::NIFTI1) {
      pEmptyHeader = new CNIFTI1MainHeader(this);
  } else if (m_pData->iNIFTIformat == CNIFTIFile::NIFTI2) {
      pEmptyHeader = new CNIFTI2MainHeader(this);
  }
  
  RETURN(pEmptyHeader);
  return pEmptyHeader;
}
//------------------------------------------------------------------------------------------------
void CNIFTIFile::mainHeaderWritten(const CNIFTIMainHeader& MainHeader) {
  ENTER();

if(m_pData->cachedMainHeader) {
    *m_pData->cachedMainHeader = MainHeader;
  } else {
    m_pData->cachedMainHeader = static_cast<CNIFTIMainHeader*>(MainHeader.clone());
  }
  
  m_pData->cachedMainHeader->setMedIOData(this);
  LEAVE();
}

//------------------------------------------------------------------------------------------------
bool CNIFTIFile::reWriteMainHeader(void) {
  ENTER();
  bool result = false;
  if(m_pData->cachedMainHeader) {
    result = m_pData->cachedMainHeader->save();
  }
  RETURN(result);
  return result;
}

//------------------------------------------------------------------------------------------------
// method of the private class to sync data with our headers
bool CNIFTIFilePrivate::syncMainHeader(CNIFTIFile* /*file*/) const {
  ENTER();


  bool result = true;
  RETURN(result);
  return result;
}




//=============================================================================================
// MATRIX I/O METHODS (Monolithic format)

bool CNIFTIFile::readMatrix(QByteArray*& matrixData) {
  ENTER();
  bool result = false;

  if(isReadable() && m_pData->cachedMainHeader) {
    // 1. offset skipping the header (348 o 540 byte)
    int offset = m_pData->cachedMainHeader->rawDataSize(); 
    
    // 2. Shift the file pointer beyond the header (shift the file pointer to the beginning of the voxel data)
    seek(offset);

    // 3. Read the entire voxel block
    matrixData = new QByteArray(readAll());
    
    if(!matrixData->isEmpty()) {
      result = true;
    } else {
      delete matrixData;
      matrixData = NULL;
    }
  }

  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
bool CNIFTIFile::readMatrix(char*& matrixData, unsigned int& len) {
  ENTER();
  bool result = false;

  if(isReadable() && m_pData->cachedMainHeader) {
    int offset = m_pData->cachedMainHeader->rawDataSize();
    seek(offset);
    
    len = size() - offset; // Number of bytes remaining in the file
    
    matrixData = new char[len];
    qint64 bytesRead = read(matrixData, len);
    
    if(bytesRead == len) {
      result = true;
    } else {
      delete[] matrixData;
      matrixData = NULL;
    }
  }

  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
bool CNIFTIFile::writeMatrix(const QByteArray& matrixData) {
  ENTER();
  bool result = false;

  if(isWritable() && m_pData->cachedMainHeader) {
    int offset = m_pData->cachedMainHeader->rawDataSize();
    seek(offset);
    
    qint64 bytesWritten = write(matrixData);
    if(bytesWritten == matrixData.size()) {
      result = true;
    }
  }

  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
bool CNIFTIFile::writeMatrix(const char* matrixData, unsigned int size) {
  ENTER();
  bool result = false;

  if(isWritable() && m_pData->cachedMainHeader) {
    int offset = m_pData->cachedMainHeader->rawDataSize();
    seek(offset);
    
    qint64 bytesWritten = write(matrixData, size);
    if(bytesWritten == size) {
      result = true;
    }
  }

  RETURN(result);
  return result;
}

