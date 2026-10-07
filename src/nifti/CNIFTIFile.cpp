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

#include <QFileInfo>
#include <QTemporaryFile>
#include <QDir>

#include <zlib.h>

#include <limits>

#include <rtdebug.h>

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers 

//=============================================================================================
// CNIFTIFilePrivate class is a private class that is used to store the private data of the CNIFTIFile class.
// Private class to add private variables to the CNIFTIFile class without exposing them in the public header file 

class CNIFTIFilePrivate {
  public:
    CNIFTIFile::NIFTIFormat iNIFTIformat;
    CNIFTIMainHeader::Type  iMainHeaderType;
    CNIFTIMainHeader*  cachedMainHeader; // for speed reasons we cache the loaded main header

    // gzip support
    // True if the input/output file uses gzip compression (.nii.gz)
    bool compressed;
    QString originalFileName; // Store the original file name for reference
    QTemporaryFile* temporaryFile; // Temporary file for handling compressed files

    // methods
    bool syncMainHeader(CNIFTIFile* file) const;
    bool matrixLayout(qint64& offset, qint64& size) const;
};


//=============================================================================================
// Function to decompress a gzip-compressed NIfTI file (.nii.gz)

static bool decompressGzipFile(const QString& sourceFilename,
                               const QString& destinationFilename)
{
    QByteArray encodedFilename = QFile::encodeName(sourceFilename);

    gzFile input = gzopen(encodedFilename.constData(), "rb");

    if (!input) {
        return false;
    }

    QFile output(destinationFilename);

    if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        gzclose(input);
        return false;
    }

    char buffer[64 * 1024];

    int bytesRead = 0;
    bool result = true;

    while ((bytesRead = gzread(input, buffer, sizeof(buffer))) > 0) {

        if (output.write(buffer, bytesRead) != bytesRead) {
            result = false;
            break;
        }
    }

    if (bytesRead < 0) {
        result = false;
    }

    output.close();

    if (gzclose(input) != Z_OK) {
        result = false;
    }

    return result;
}

// Compress a regular file into gzip format

static bool compressGzipFile(const QString& sourceFilename,
                             const QString& destinationFilename)
{
    QFile input(sourceFilename);

    if(!input.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray destinationName = QFile::encodeName(destinationFilename);

    gzFile output = gzopen(destinationName.constData(), "wb");

    if(output == NULL) {
        input.close();
        return false;
    }

    char buffer[64 * 1024];
    bool result = true;

    while(true) {

        qint64 bytesRead = input.read(buffer, sizeof(buffer));

        if(bytesRead < 0) {
            result = false;
            break;
        }

        if(bytesRead == 0) {
            break;
        }

        int bytesWritten =
            gzwrite(output,
                    buffer,
                    static_cast<unsigned int>(bytesRead));

        if(bytesWritten != bytesRead) {
            result = false;
            break;
        }
    }

    input.close();

    if(gzclose(output) != Z_OK) {
        result = false;
    }

    if(!result) {
        QFile::remove(destinationFilename);
    }

    return result;
}
//=============================================================================================
// identification of NIfTI headers

// size of the largest NIfTI header (NIfTI-2)
#define NIFTI_MAX_HEADER_SIZE 540

// Identify the NIfTI version of a header by means of its sizeof_hdr field
// and the magic signature. Only single file (.nii) NIfTI data is supported.
static CNIFTIFile::NIFTIFormat identifyHeader(const QByteArray& header)
{
  if(header.size() < 348)
    return CNIFTIFile::Undefined;

  // sizeof_hdr is either stored in little or big endian byte order
  const uchar* p = reinterpret_cast<const uchar*>(header.constData());
  quint32 sizeofHdrLE = p[0] | (p[1] << 8) | (p[2] << 16) | (static_cast<quint32>(p[3]) << 24);
  quint32 sizeofHdrBE = p[3] | (p[2] << 8) | (p[1] << 16) | (static_cast<quint32>(p[0]) << 24);

  if(sizeofHdrLE == 348 || sizeofHdrBE == 348) {
    // NIfTI-1: magic at byte offset 344
    if(memcmp(header.constData()+344, "n+1\0", 4) == 0)
      return CNIFTIFile::NIFTI1;

    if(memcmp(header.constData()+344, "ni1\0", 4) == 0)
      W("NIfTI-1 header/image file pairs (.hdr/.img) are not supported");
  }
  else if((sizeofHdrLE == 540 || sizeofHdrBE == 540) && header.size() >= 540) {
    // NIfTI-2: magic at byte offset 4
    if(memcmp(header.constData()+4, "n+2\0", 4) == 0)
      return CNIFTIFile::NIFTI2;

    if(memcmp(header.constData()+4, "ni2\0", 4) == 0)
      W("NIfTI-2 header/image file pairs (.hdr/.img) are not supported");
  }

  return CNIFTIFile::Undefined;
}

// read the first bytes of a (possibly gzip compressed) file which are
// required to identify a NIfTI header
static QByteArray readHeaderBytes(const QString& fileName, bool compressed)
{
  QByteArray header;

  if(compressed) {
    gzFile input = gzopen(QFile::encodeName(fileName).constData(), "rb");

    if(input) {
      header.resize(NIFTI_MAX_HEADER_SIZE);

      int bytesRead = gzread(input, header.data(), NIFTI_MAX_HEADER_SIZE);
      header.resize(bytesRead > 0 ? bytesRead : 0);

      gzclose(input);
    }
  } else {
    QFile input(fileName);

    if(input.open(QIODevice::ReadOnly)) {
      header = input.read(NIFTI_MAX_HEADER_SIZE);
      input.close();
    }
  }

  return header;
}

//=============================================================================================
// Constructors and Destructors for the CNIFTIFile class
CNIFTIFile::CNIFTIFile(const QString& filename, CNIFTIMainHeader::Type fileType): CMedIOData(filename) {
    
    ENTER();

    // allocate data from our private instance class m_pData is a pointer to the private data of the CNIFTIFile class
    m_pData = new CNIFTIFilePrivate();
    m_pData->iNIFTIformat = CNIFTIFile::Undefined;
    m_pData->iMainHeaderType = fileType;
    m_pData->cachedMainHeader = NULL;
    
    // Check whether the NIfTI file is gzip-compressed (.nii.gz)     
    m_pData->compressed = QFileInfo(filename).fileName().endsWith(".nii.gz", Qt::CaseInsensitive);

    m_pData->originalFileName = filename; // Store the original file name for reference
    m_pData->temporaryFile = NULL; // Initialize the temporary file pointer to NULL

    setFileType(fileType);

    // to specify the NIfTI format version (NIFTI1 or NIFTI2) based on the file extension
    if (fileType == CNIFTIMainHeader::NIFTI1) {
      m_pData->iNIFTIformat = CNIFTIFile::NIFTI1;
    } else if (fileType == CNIFTIMainHeader::NIFTI2) {
      m_pData->iNIFTIformat = CNIFTIFile::NIFTI2; 
    }

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

  // only read the header bytes so that a compressed file
  // does not have to be decompressed completely
  bool compressed = QFileInfo(filename).fileName().endsWith(".nii.gz", Qt::CaseInsensitive);
  bool result = (identifyHeader(readHeaderBytes(filename, compressed)) != CNIFTIFile::Undefined);

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

    // ------------------------------------------------------------
    // Handle gzip-compressed NIfTI files (.nii.gz)
    // ------------------------------------------------------------
    if(m_pData->compressed) {

        m_pData->temporaryFile =
            new QTemporaryFile(
                QDir::tempPath() + "/libmedio-nifti-XXXXXX.nii"
            );

        m_pData->temporaryFile->setAutoRemove(true);

        if(!m_pData->temporaryFile->open()) {

            W("Unable to create temporary file for compressed NIfTI.");

            delete m_pData->temporaryFile;
            m_pData->temporaryFile = NULL;

            RETURN(false);
            return false;
        }

        QString temporaryFilename =
            m_pData->temporaryFile->fileName();

        // The file will be reopened by CNIFTIFile
        m_pData->temporaryFile->close();

        if(!decompressGzipFile(m_pData->originalFileName,
                               temporaryFilename)) {

            W("Unable to decompress NIfTI gzip file.");

            delete m_pData->temporaryFile;
            m_pData->temporaryFile = NULL;

            RETURN(false);
            return false;
        }

        // CNIFTIFile will now operate on the decompressed temporary .nii
        QFile::setFileName(temporaryFilename);

        D("Compressed NIfTI file successfully decompressed.");
    }

    if(QFile::open(QIODevice::ReadOnly)) {
      
      // identify the NIfTI version by means of sizeof_hdr and the magic signature
      CNIFTIFile::NIFTIFormat format = identifyHeader(QFile::read(NIFTI_MAX_HEADER_SIZE));

      if(format == CNIFTIFile::NIFTI1) {
        D("Found NIfTI-1 file");
        m_pData->iNIFTIformat = CNIFTIFile::NIFTI1;
        m_pData->cachedMainHeader = new CNIFTI1MainHeader(this);
      }
      else if(format == CNIFTIFile::NIFTI2) {
        D("Found NIfTI-2 file");
        m_pData->iNIFTIformat = CNIFTIFile::NIFTI2;
        m_pData->cachedMainHeader = new CNIFTI2MainHeader(this);
      }
      else {
        W("Magic number error: it's not a valid NIfTI file.");
      }

      // load the rest of the header data from the file
      if(m_pData->cachedMainHeader != NULL) {
        if(m_pData->cachedMainHeader->load())
          result = true;
        else
          W("Error while loading NIfTI header");
      }

      QFile::close();
    }
  }

// Create a new file (WriteOnly)
else if((mode & QIODevice::WriteOnly) == QIODevice::WriteOnly)
{
    if(m_pData->iNIFTIformat != CNIFTIFile::Undefined) {

        // Create an empty header ready to be filled
        if(m_pData->iNIFTIformat == CNIFTIFile::NIFTI1) {
            m_pData->cachedMainHeader = new CNIFTI1MainHeader(this);
        }
        else if(m_pData->iNIFTIformat == CNIFTIFile::NIFTI2) {
            m_pData->cachedMainHeader = new CNIFTI2MainHeader(this);
        }

        result = true;

    } else {

        E("Format NIFTI unknown for writing");
    }


    if(result) {

        // ------------------------------------------------------------
        // Compressed NIfTI output (.nii.gz)
        // ------------------------------------------------------------
        if(m_pData->compressed) {

            // The actual NIfTI file is first written uncompressed
            // into a temporary .nii file.
            m_pData->temporaryFile =
                new QTemporaryFile(
                    QDir::tempPath() + "/libmedio-nifti-XXXXXX.nii"
                );

            m_pData->temporaryFile->setAutoRemove(true);

            if(!m_pData->temporaryFile->open()) {

                W("Unable to create temporary file for compressed NIfTI output.");

                delete m_pData->temporaryFile;
                m_pData->temporaryFile = NULL;

                delete m_pData->cachedMainHeader;
                m_pData->cachedMainHeader = NULL;

                RETURN(false);
                return false;
            }

            QString temporaryFilename = m_pData->temporaryFile->fileName();

            // CNIFTIFile will reopen the temporary file itself
            m_pData->temporaryFile->close();

            // From this point CNIFTIFile writes to the temporary
            // uncompressed NIfTI file instead of directly to .nii.gz
            QFile::setFileName(temporaryFilename);

            D("Using temporary uncompressed NIfTI file for gzip output.");

        } else {

            // Normal uncompressed .nii output
            QFile::remove(fileName());
        }
    }
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

    if(m_pData->temporaryFile) {

        QFile::setFileName(m_pData->originalFileName);

        delete m_pData->temporaryFile;
        m_pData->temporaryFile = NULL;
    }
}
    

  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
// 2. close --> method to close the file

void CNIFTIFile::close(void) {

    ENTER();

    // Remember whether the temporary NIfTI file was opened for writing.
    bool compressOnClose =
        m_pData->compressed &&
        m_pData->temporaryFile != NULL &&
        isWritable();

    QString temporaryFilename;

    if(m_pData->temporaryFile != NULL) {
        temporaryFilename = QFile::fileName();
    }

    // Close the uncompressed file first.
    QFile::close();

    // If this was a compressed output file, gzip the temporary .nii.
    if(compressOnClose) {

        if(compressGzipFile(temporaryFilename,
                            m_pData->originalFileName)) {

            D("Compressed NIfTI file successfully written.");

        } else {

            E("Unable to compress NIfTI file.");
        }
    }

    if(m_pData->cachedMainHeader)
    {
        delete m_pData->cachedMainHeader;
        m_pData->cachedMainHeader = NULL;
    }

    if(m_pData->temporaryFile) {
        // Restore the filename originally supplied to CNIFTIFile.
        QFile::setFileName(m_pData->originalFileName);

        delete m_pData->temporaryFile;
        m_pData->temporaryFile = NULL;
    }

    LEAVE();
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

  if (m_pData->cachedMainHeader == &MainHeader) {
    m_pData->cachedMainHeader->setMedIOData(this); //link the cached header to the current file
    LEAVE();
    return;
  }

  delete m_pData->cachedMainHeader;

  m_pData->cachedMainHeader = static_cast<CNIFTIMainHeader*>(MainHeader.clone());

  if(m_pData->cachedMainHeader != NULL) {
    m_pData->cachedMainHeader->setMedIOData(this); //link the cached header to the current file
  }

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




//------------------------------------------------------------------------------------------------
// method of the private class to determine the file position and the size (in bytes)
// of the voxel matrix as described by the cached main header
bool CNIFTIFilePrivate::matrixLayout(qint64& offset, qint64& size) const {
  ENTER();

  if(cachedMainHeader == NULL) {
    RETURN(false);
    return false;
  }

  qint64 dims[8];
  int bitpix = 0;

  if(iNIFTIformat == CNIFTIFile::NIFTI1) {
    const CNIFTI1MainHeader* header = static_cast<const CNIFTI1MainHeader*>(cachedMainHeader);
    for(int i=0; i < 8; i++)
      dims[i] = header->dim(i);
    bitpix = header->bit_Pix();
    offset = static_cast<qint64>(header->vox_Offset());
  } else if(iNIFTIformat == CNIFTIFile::NIFTI2) {
    const CNIFTI2MainHeader* header = static_cast<const CNIFTI2MainHeader*>(cachedMainHeader);
    for(int i=0; i < 8; i++)
      dims[i] = header->dim(i);
    bitpix = header->bit_Pix();
    offset = header->vox_Offset();
  } else {
    RETURN(false);
    return false;
  }

  // in a single file (.nii) the voxel data follows the header and the
  // 4 byte extension indicator at the earliest
  if(offset < cachedMainHeader->rawDataSize() + 4)
    offset = cachedMainHeader->rawDataSize() + 4;

  if(dims[0] < 1 || dims[0] > 7 || bitpix <= 0 || (bitpix % 8) != 0) {
    E("invalid NIfTI matrix description (dim[0]=%lld, bitpix=%d)", dims[0], bitpix);
    RETURN(false);
    return false;
  }

  size = bitpix / 8;
  for(int i=1; i <= dims[0]; i++) {
    if(dims[i] < 1 || size > std::numeric_limits<qint64>::max() / dims[i]) {
      E("invalid NIfTI matrix dimension dim[%d]=%lld", i, dims[i]);
      RETURN(false);
      return false;
    }

    size *= dims[i];
  }

  RETURN(true);
  return true;
}

//=============================================================================================
// MATRIX I/O METHODS (Monolithic format)

bool CNIFTIFile::readMatrix(QByteArray*& matrixData) {
  ENTER();
  bool result = false;
  qint64 offset = 0;
  qint64 matrixSize = 0;

  matrixData = NULL;

  if(isReadable() && m_pData->matrixLayout(offset, matrixSize)) {
    if(matrixSize > std::numeric_limits<int>::max()) {
      E("voxel matrix of %lld bytes is too large for a QByteArray", matrixSize);
    } else if(size() < offset + matrixSize) {
      E("file too short: voxel matrix requires %lld bytes at offset %lld", matrixSize, offset);
    } else if(seek(offset)) {
      matrixData = new QByteArray(read(matrixSize));

      if(matrixData->size() == matrixSize) {
        result = true;
      } else {
        delete matrixData;
        matrixData = NULL;
      }
    }
  }

  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
bool CNIFTIFile::readMatrix(char*& matrixData, unsigned int& len) {
  ENTER();
  bool result = false;
  qint64 offset = 0;
  qint64 matrixSize = 0;

  matrixData = NULL;
  len = 0;

  if(isReadable() && m_pData->matrixLayout(offset, matrixSize)) {
    if(matrixSize > std::numeric_limits<unsigned int>::max()) {
      E("voxel matrix of %lld bytes is too large", matrixSize);
    } else if(size() < offset + matrixSize) {
      E("file too short: voxel matrix requires %lld bytes at offset %lld", matrixSize, offset);
    } else if(seek(offset)) {
      matrixData = new char[matrixSize];

      if(read(matrixData, matrixSize) == matrixSize) {
        len = static_cast<unsigned int>(matrixSize);
        result = true;
      } else {
        delete[] matrixData;
        matrixData = NULL;
      }
    }
  }

  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
bool CNIFTIFile::writeMatrix(const QByteArray& matrixData) {
  ENTER();

  bool result = writeMatrix(matrixData.constData(), matrixData.size());

  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
bool CNIFTIFile::writeMatrix(const char* matrixData, unsigned int size) {
  ENTER();
  bool result = false;
  qint64 offset = 0;
  qint64 matrixSize = 0;

  // the main header has to be written first as it defines the matrix layout
  if(isWritable() && m_pData->matrixLayout(offset, matrixSize)) {
    if(static_cast<qint64>(size) != matrixSize) {
      E("size of the voxel matrix (%u bytes) does not match the main header (%lld bytes)", size, matrixSize);
    } else if(seek(offset)) {
      // vox_offset is used so that header extensions are not overwritten
      if(write(matrixData, size) == matrixSize)
        result = resize(offset + matrixSize);
    }
  }

  RETURN(result);
  return result;
}
