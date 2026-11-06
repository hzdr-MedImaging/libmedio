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
#include "CNIFTIGeometry.h"
#include "CECATSubHeader.h"
#include "CECAT7SubHeaderImage.h"

#include <QFileInfo>
#include <QJsonDocument>
#include <QTemporaryFile>
#include <QDir>

#include <zlib.h>

#include <algorithm>
#include <cmath>
#include <cstring>
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
    // True if the input/output file uses gzip compression (.nii.gz, .hdr.gz/.img.gz)
    bool compressed;
    QString originalFileName; // Store the original file name for reference
    QTemporaryFile* temporaryFile; // Temporary file for handling compressed files

    // header/image file pairs (.hdr/.img): the header (and its extensions) is
    // accessed via the QFile of CNIFTIFile, the voxel data via imageFile
    bool pair;
    QString headerFileName;
    QString imageFileName;
    QFile* imageFile;
    QTemporaryFile* imageTemporaryFile;

    // layout of one 3D volume of the voxel matrix (see volumeLayout())
    struct Volume {
      qint64 offset;        // file position of the volume
      qint64 size;          // size of the volume in bytes
      qint64 dims[3];       // dim[1..3] of the main header
      int bytesPerVoxel;
      int swapSize;         // size of the numbers to swap (0: no swapping)
      ECATGeometryFromNIFTI geometry; // ECATOrientation only
    };

    // methods
    bool syncMainHeader(CNIFTIFile* file) const;
    bool matrixLayout(qint64& offset, qint64& size, int& swapSize) const;
    bool volumeLayout(short& frame, CMedIOData::Orientation orientation, Volume& volume) const;
    bool readVolume(CNIFTIFile* file, short& frame, CMedIOData::Orientation orientation,
                    QByteArray& data, Volume& volume) const;
    bool writeVolume(CNIFTIFile* file, short frame, CMedIOData::Orientation orientation,
                     const char* data, qint64 size) const;
    QJsonObject metadata(CNIFTIFile* file) const;
    QFile* matrixFile(CNIFTIFile* file) const { return pair ? imageFile : file; }
    void release(void);
};

// free the resources of an opened file
void CNIFTIFilePrivate::release(void)
{
  delete cachedMainHeader;
  cachedMainHeader = NULL;

  if(imageFile) {
    imageFile->close();
    delete imageFile;
    imageFile = NULL;
  }

  delete temporaryFile;
  temporaryFile = NULL;
  delete imageTemporaryFile;
  imageTemporaryFile = NULL;
}


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
// byte order handling of the voxel data

// size in bytes of the individual numbers stored in a voxel of the given NIfTI datatype
static int voxelNumberSize(int datatype, int bitpix)
{
  switch(datatype) {
    case 32:   return 4;  // DT_COMPLEX64: 2 x float32
    case 1792: return 8;  // DT_COMPLEX128: 2 x float64
    case 2048: return 16; // DT_COMPLEX256: 2 x float128
    case 128:             // DT_RGB24: 3 x uint8
    case 2304: return 1;  // DT_RGBA32: 4 x uint8
  }

  return bitpix / 8;
}

// reverse the byte order of all numbers of size numberSize in the given data
static void swapMatrix(char* data, qint64 size, int numberSize)
{
  if(numberSize < 2)
    return;

  for(qint64 i=0; i + numberSize <= size; i += numberSize)
    std::reverse(data + i, data + i + numberSize);
}

//=============================================================================================
// identification of NIfTI headers

// size of the largest NIfTI header (NIfTI-2)
#define NIFTI_MAX_HEADER_SIZE 540

// Identify the NIfTI version of a header by means of its sizeof_hdr field
// and the magic signature. pair is set to true for the header of a
// header/image file pair ("ni1"/"ni2") and to false for a single file
// ("n+1"/"n+2"). Analyze 7.5 headers (without magic) are not supported.
static CNIFTIFile::NIFTIFormat identifyHeader(const QByteArray& header, bool& pair)
{
  pair = false;

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

    if(memcmp(header.constData()+344, "ni1\0", 4) == 0) {
      pair = true;
      return CNIFTIFile::NIFTI1;
    }
  }
  else if((sizeofHdrLE == 540 || sizeofHdrBE == 540) && header.size() >= 540) {
    // NIfTI-2: magic at byte offset 4
    if(memcmp(header.constData()+4, "n+2\0", 4) == 0)
      return CNIFTIFile::NIFTI2;

    if(memcmp(header.constData()+4, "ni2\0", 4) == 0) {
      pair = true;
      return CNIFTIFile::NIFTI2;
    }
  }

  return CNIFTIFile::Undefined;
}

// Determine the names of the header and image file of a header/image file
// pair from the name of one of them (.hdr/.img, optionally gzip compressed
// as .hdr.gz/.img.gz). Returns false for the names of single files.
static bool pairFileNames(const QString& fileName, QString& headerName, QString& imageName, bool& compressed)
{
  QString stem = fileName;
  QString gz;

  if(stem.endsWith(".gz", Qt::CaseInsensitive)) {
    gz = stem.right(3);
    stem.chop(3);
  }

  const QString extension = stem.right(4);
  if(extension.compare(".hdr", Qt::CaseInsensitive) != 0 &&
     extension.compare(".img", Qt::CaseInsensitive) != 0) {
    return false;
  }
  stem.chop(4);

  // use the letter case of the given extension, but accept existing
  // files using the other one (e.g. "data.hdr" and "data.IMG")
  const bool upper = (extension == extension.toUpper());
  headerName = stem + (upper ? ".HDR" : ".hdr") + gz;
  imageName = stem + (upper ? ".IMG" : ".img") + gz;

  if(QFile::exists(headerName) == false && QFile::exists(stem + (upper ? ".hdr" : ".HDR") + gz))
    headerName = stem + (upper ? ".hdr" : ".HDR") + gz;
  if(QFile::exists(imageName) == false && QFile::exists(stem + (upper ? ".img" : ".IMG") + gz))
    imageName = stem + (upper ? ".img" : ".IMG") + gz;

  // the given name is used as it is
  if(extension.compare(".hdr", Qt::CaseInsensitive) == 0)
    headerName = fileName;
  else
    imageName = fileName;

  compressed = (gz.isEmpty() == false);
  return true;
}

// decompress a gzip compressed file into a new temporary file
static QTemporaryFile* decompressToTemporaryFile(const QString& fileName)
{
  QTemporaryFile* temporaryFile = new QTemporaryFile(QDir::tempPath() + "/libmedio-nifti-XXXXXX");
  temporaryFile->setAutoRemove(true);

  // the file is reopened by its user
  if(temporaryFile->open() == false) {
    W("unable to create a temporary file for %s", fileName.toLocal8Bit().constData());
    delete temporaryFile;
    return NULL;
  }
  temporaryFile->close();

  if(decompressGzipFile(fileName, temporaryFile->fileName()) == false) {
    W("unable to decompress %s", fileName.toLocal8Bit().constData());
    delete temporaryFile;
    return NULL;
  }

  return temporaryFile;
}

// create an empty temporary file which is compressed into fileName when
// closing the NIfTI file; fileName is created right away so that an
// unwritable destination is detected when opening the file
static QTemporaryFile* createTemporaryFile(const QString& fileName)
{
  QFile destination(fileName);
  if(destination.open(QIODevice::WriteOnly | QIODevice::Truncate) == false) {
    W("unable to create %s", fileName.toLocal8Bit().constData());
    return NULL;
  }
  destination.close();

  QTemporaryFile* temporaryFile = new QTemporaryFile(QDir::tempPath() + "/libmedio-nifti-XXXXXX");
  temporaryFile->setAutoRemove(true);

  if(temporaryFile->open() == false) {
    W("unable to create a temporary file for %s", fileName.toLocal8Bit().constData());
    delete temporaryFile;
    QFile::remove(fileName);
    return NULL;
  }
  temporaryFile->close();

  return temporaryFile;
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
    
    m_pData->originalFileName = filename; // Store the original file name for reference
    m_pData->temporaryFile = NULL; // Initialize the temporary file pointer to NULL
    m_pData->imageFile = NULL;
    m_pData->imageTemporaryFile = NULL;

    // header/image file pair (.hdr/.img) or single file (.nii), both
    // optionally gzip compressed (.hdr.gz/.img.gz, .nii.gz)
    m_pData->pair = pairFileNames(filename, m_pData->headerFileName, m_pData->imageFileName, m_pData->compressed);
    if(m_pData->pair == false) {
      m_pData->compressed = QFileInfo(filename).fileName().endsWith(".nii.gz", Qt::CaseInsensitive);
      m_pData->headerFileName = filename;
      m_pData->imageFileName = filename;
    }

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

    // write out (and compress) data of a file which is still open
    if(isOpen() || m_pData->imageFile)
      close();

    m_pData->release();
    delete m_pData;

    LEAVE();
}

//=============================================================================================
// isOfType --> Static method to check if a file is of the correct type 
bool CNIFTIFile::isOfType(const QString& filename) {
  ENTER();

  // only read the header bytes so that a compressed file
  // does not have to be decompressed completely
  QString headerName, imageName;
  bool compressed = false;
  bool pairName = pairFileNames(filename, headerName, imageName, compressed);
  if(pairName == false) {
    headerName = filename;
    compressed = QFileInfo(filename).fileName().endsWith(".nii.gz", Qt::CaseInsensitive);
  }

  // a header/image pair also requires the image file
  bool pair = false;
  bool result = (identifyHeader(readHeaderBytes(headerName, compressed), pair) != CNIFTIFile::Undefined) &&
                pair == pairName &&
                (pair == false || QFile::exists(imageName));

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
  CNIFTIFilePrivate* d = m_pData;

  // close a previously opened file
  if(isOpen() || d->imageFile)
    close();
  d->release();

  const bool exists = QFile::exists(d->headerFileName) &&
                      (d->pair == false || QFile::exists(d->imageFileName));
  QString imagePath = d->imageFileName;
  bool writing = false;

  // reading an existing file (ReadOnly or ReadWrite mode)
  if((mode & QIODevice::ReadOnly) == QIODevice::ReadOnly && exists) {

    QString headerPath = d->headerFileName;
    bool files = true;

    // compressed files are decompressed into temporary files
    if(d->compressed) {
      if((d->temporaryFile = decompressToTemporaryFile(d->headerFileName)) != NULL)
        headerPath = d->temporaryFile->fileName();
      else
        files = false;

      if(files && d->pair) {
        if((d->imageTemporaryFile = decompressToTemporaryFile(d->imageFileName)) != NULL)
          imagePath = d->imageTemporaryFile->fileName();
        else
          files = false;
      }
    }

    QFile::setFileName(headerPath);

    if(files && QFile::open(QIODevice::ReadOnly)) {

      // identify the NIfTI version by means of sizeof_hdr and the magic signature
      bool pair = false;
      CNIFTIFile::NIFTIFormat format = identifyHeader(QFile::read(NIFTI_MAX_HEADER_SIZE), pair);

      if(format != CNIFTIFile::Undefined && pair != d->pair) {
        W("%s: the NIfTI magic does not match the file name (%s expected)",
          d->originalFileName.toLocal8Bit().constData(), d->pair ? "header/image pair" : "single file");
        format = CNIFTIFile::Undefined;
      }

      if(format == CNIFTIFile::NIFTI1) {
        D("Found NIfTI-1 file");
        d->iNIFTIformat = CNIFTIFile::NIFTI1;
        d->cachedMainHeader = new CNIFTI1MainHeader(this);
      }
      else if(format == CNIFTIFile::NIFTI2) {
        D("Found NIfTI-2 file");
        d->iNIFTIformat = CNIFTIFile::NIFTI2;
        d->cachedMainHeader = new CNIFTI2MainHeader(this);
      }
      else {
        W("Magic number error: it's not a valid NIfTI file.");
      }

      // load the rest of the header data from the file
      if(d->cachedMainHeader != NULL) {
        if(d->cachedMainHeader->load())
          result = true;
        else
          W("Error while loading NIfTI header");
      }

      QFile::close();
    }
  }
  // create a new file (WriteOnly)
  else if((mode & QIODevice::WriteOnly) == QIODevice::WriteOnly) {

    // create an empty header ready to be filled
    if(d->iNIFTIformat == CNIFTIFile::NIFTI1)
      d->cachedMainHeader = new CNIFTI1MainHeader(this);
    else if(d->iNIFTIformat == CNIFTIFile::NIFTI2)
      d->cachedMainHeader = new CNIFTI2MainHeader(this);
    else
      E("Format NIFTI unknown for writing");

    if(d->cachedMainHeader != NULL) {
      result = true;
      writing = true;

      if(d->compressed) {
        // the data is written uncompressed into temporary files which are
        // compressed into the actual files when closing the file
        if((d->temporaryFile = createTemporaryFile(d->headerFileName)) != NULL)
          QFile::setFileName(d->temporaryFile->fileName());
        else
          result = false;

        if(result && d->pair) {
          if((d->imageTemporaryFile = createTemporaryFile(d->imageFileName)) != NULL)
            imagePath = d->imageTemporaryFile->fileName();
          else
            result = false;
        }
      } else {
        QFile::setFileName(d->headerFileName);
        QFile::remove(d->headerFileName);
        if(d->pair)
          QFile::remove(d->imageFileName);
      }
    }
  }

  // reopen the file(s) with the mode requested by the user
  if(result) {
    mode = static_cast<QIODevice::OpenModeFlag>(mode & ~(QIODevice::Append|QIODevice::Truncate|QIODevice::Text));

    result = QFile::open(mode|QIODevice::ReadOnly);

    if(result && d->pair) {
      d->imageFile = new QFile(imagePath);
      result = d->imageFile->open(mode|QIODevice::ReadOnly);
    }

    if(result == false)
      QFile::close();
  }

  if(result == false) {
    d->release();
    QFile::setFileName(d->originalFileName);

    // do not leave (partially) created files behind
    if(writing) {
      QFile::remove(d->headerFileName);
      if(d->pair)
        QFile::remove(d->imageFileName);
    }
  }

  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
// 2. close --> method to close the file

void CNIFTIFile::close(void) {

  ENTER();
  CNIFTIFilePrivate* d = m_pData;

  // compress the temporary files of a compressed file opened for writing
  const bool compress = d->compressed && d->temporaryFile != NULL && isWritable();

  QFile::close();
  if(d->imageFile)
    d->imageFile->close();

  if(compress) {
    bool result = compressGzipFile(d->temporaryFile->fileName(), d->headerFileName);

    if(result && d->pair && d->imageTemporaryFile)
      result = compressGzipFile(d->imageTemporaryFile->fileName(), d->imageFileName);

    if(result)
      D("Compressed NIfTI file successfully written.");
    else
      E("Unable to compress NIfTI file %s", d->originalFileName.toLocal8Bit().constData());
  }

  d->release();

  // restore the file name originally supplied to CNIFTIFile
  QFile::setFileName(d->originalFileName);

  LEAVE();
}
//------------------------------------------------------------------------------------------------
// header/image file pair (.hdr/.img)
bool CNIFTIFile::isPair(void) const {
  return m_pData->pair;
}
//------------------------------------------------------------------------------------------------
// name of the file holding the header (and its extensions)
QString CNIFTIFile::headerFileName(void) const {
  return m_pData->headerFileName;
}
//------------------------------------------------------------------------------------------------
// name of the file holding the voxel data
QString CNIFTIFile::imageFileName(void) const {
  return m_pData->imageFileName;
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
bool CNIFTIFilePrivate::matrixLayout(qint64& offset, qint64& size, int& swapSize) const {
  ENTER();

  if(cachedMainHeader == NULL) {
    RETURN(false);
    return false;
  }

  qint64 dims[8];
  int bitpix = 0;
  int datatype = 0;
  QSysInfo::Endian byteOrder = QSysInfo::LittleEndian;

  if(iNIFTIformat == CNIFTIFile::NIFTI1) {
    const CNIFTI1MainHeader* header = static_cast<const CNIFTI1MainHeader*>(cachedMainHeader);
    for(int i=0; i < 8; i++)
      dims[i] = header->dim(i);
    bitpix = header->bit_Pix();
    datatype = header->dataType();
    byteOrder = header->byteOrder();
    offset = static_cast<qint64>(header->vox_Offset());
  } else if(iNIFTIformat == CNIFTIFile::NIFTI2) {
    const CNIFTI2MainHeader* header = static_cast<const CNIFTI2MainHeader*>(cachedMainHeader);
    for(int i=0; i < 8; i++)
      dims[i] = header->dim(i);
    bitpix = header->bit_Pix();
    datatype = header->dataType();
    byteOrder = header->byteOrder();
    offset = header->vox_Offset();
  } else {
    RETURN(false);
    return false;
  }

  // in a single file (.nii) the voxel data follows the header and the
  // 4 byte extension indicator at the earliest, in the image file of a
  // header/image pair it usually starts at offset 0
  if(pair == false && offset < cachedMainHeader->rawDataSize() + 4)
    offset = cachedMainHeader->rawDataSize() + 4;
  else if(offset < 0)
    offset = 0;

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

  // the voxel data is stored in the byte order of the main header
  swapSize = (byteOrder != QSysInfo::ByteOrder) ? voxelNumberSize(datatype, bitpix) : 0;

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
  int swapSize = 0;

  matrixData = NULL;

  QFile* file = m_pData->matrixFile(this);

  if(file && file->isReadable() && m_pData->matrixLayout(offset, matrixSize, swapSize)) {
    if(matrixSize > std::numeric_limits<int>::max()) {
      E("voxel matrix of %lld bytes is too large for a QByteArray", matrixSize);
    } else if(file->size() < offset + matrixSize) {
      E("file too short: voxel matrix requires %lld bytes at offset %lld", matrixSize, offset);
    } else if(file->seek(offset)) {
      matrixData = new QByteArray(file->read(matrixSize));

      if(matrixData->size() == matrixSize) {
        // return the voxel data in the byte order of this machine
        swapMatrix(matrixData->data(), matrixSize, swapSize);
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
  int swapSize = 0;

  matrixData = NULL;
  len = 0;

  QFile* file = m_pData->matrixFile(this);

  if(file && file->isReadable() && m_pData->matrixLayout(offset, matrixSize, swapSize)) {
    if(matrixSize > std::numeric_limits<unsigned int>::max()) {
      E("voxel matrix of %lld bytes is too large", matrixSize);
    } else if(file->size() < offset + matrixSize) {
      E("file too short: voxel matrix requires %lld bytes at offset %lld", matrixSize, offset);
    } else if(file->seek(offset)) {
      matrixData = new char[matrixSize];

      if(file->read(matrixData, matrixSize) == matrixSize) {
        // return the voxel data in the byte order of this machine
        swapMatrix(matrixData, matrixSize, swapSize);
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
  int swapSize = 0;

  // the main header has to be written first as it defines the matrix layout
  QFile* file = m_pData->matrixFile(this);

  if(file && file->isWritable() && m_pData->matrixLayout(offset, matrixSize, swapSize)) {
    if(static_cast<qint64>(size) != matrixSize) {
      E("size of the voxel matrix (%u bytes) does not match the main header (%lld bytes)", size, matrixSize);
    } else if(file->seek(offset)) {
      qint64 bytesWritten = 0;

      // vox_offset is used so that header extensions are not overwritten
      if(swapSize > 0) {
        // the voxel data has to be stored in the byte order of the main header
        QByteArray swappedData(matrixData, size);
        swapMatrix(swappedData.data(), matrixSize, swapSize);
        bytesWritten = file->write(swappedData);
      } else {
        bytesWritten = file->write(matrixData, size);
      }

      if(bytesWritten == matrixSize)
        result = file->resize(offset + matrixSize);
    }
  }

  RETURN(result);
  return result;
}

//=============================================================================================
// FRAME-WISE MATRIX I/O (native or ECAT7 voxel order, ECAT7 representation)

namespace {

// datatype and scaling of a NIfTI main header (scl_slope = 0: no scaling)
struct NIFTIScaling {
  int datatype;
  double slope;
  double inter;
};

template <class H>
NIFTIScaling niftiScalingOf(const H* h)
{
  NIFTIScaling s;
  s.datatype = h->dataType();
  s.slope = h->scl_Slope();
  s.inter = h->scl_Inter();

  if(s.slope == 0.0 || std::isfinite(s.slope) == false) {
    s.slope = 1.0;
    s.inter = 0.0;
  }
  if(std::isfinite(s.inter) == false)
    s.inter = 0.0;

  return s;
}

NIFTIScaling niftiScaling(const CNIFTIMainHeader* header)
{
  if(header->mainHeaderType() == CNIFTIMainHeader::NIFTI1MainHeader)
    return niftiScalingOf(static_cast<const CNIFTI1MainHeader*>(header));

  return niftiScalingOf(static_cast<const CNIFTI2MainHeader*>(header));
}

// size in bytes of the NIfTI datatypes with a single number per voxel (0: other datatypes)
int niftiNumberSize(int datatype)
{
  switch(datatype) {
    case 2: case 256:           return 1; // uint8, int8
    case 4: case 512:           return 2; // int16, uint16
    case 8: case 16: case 768:  return 4; // int32, float32, uint32
    case 64:                    return 8; // float64
  }
  return 0;
}

// stored value of voxel i of NIfTI data in machine byte order
double niftiNumber(const char* data, qint64 i, int datatype)
{
  switch(datatype) {
    case 2:   return reinterpret_cast<const quint8*>(data)[i];
    case 256: return reinterpret_cast<const qint8*>(data)[i];
    case 4:   return reinterpret_cast<const qint16*>(data)[i];
    case 512: return reinterpret_cast<const quint16*>(data)[i];
    case 8:   return reinterpret_cast<const qint32*>(data)[i];
    case 768: return reinterpret_cast<const quint32*>(data)[i];
    case 16:  return reinterpret_cast<const float*>(data)[i];
    default:  return reinterpret_cast<const double*>(data)[i];
  }
}

// value range of the NIfTI integer datatypes
bool niftiIntegerRange(int datatype, double& minValue, double& maxValue)
{
  switch(datatype) {
    case 2:   minValue = 0.0;         maxValue = 255.0;        return true;
    case 256: minValue = -128.0;      maxValue = 127.0;        return true;
    case 4:   minValue = -32768.0;    maxValue = 32767.0;      return true;
    case 512: minValue = 0.0;         maxValue = 65535.0;      return true;
    case 8:   minValue = -2147483648.0; maxValue = 2147483647.0; return true;
    case 768: minValue = 0.0;         maxValue = 4294967295.0; return true;
  }
  return false;
}

// set voxel i of NIfTI data (machine byte order) to value
void setNiftiNumber(char* data, qint64 i, int datatype, double value)
{
  switch(datatype) {
    case 2:   reinterpret_cast<quint8*>(data)[i] = static_cast<quint8>(value); break;
    case 256: reinterpret_cast<qint8*>(data)[i] = static_cast<qint8>(value); break;
    case 4:   reinterpret_cast<qint16*>(data)[i] = static_cast<qint16>(value); break;
    case 512: reinterpret_cast<quint16*>(data)[i] = static_cast<quint16>(value); break;
    case 8:   reinterpret_cast<qint32*>(data)[i] = static_cast<qint32>(value); break;
    case 768: reinterpret_cast<quint32*>(data)[i] = static_cast<quint32>(value); break;
    case 16:  reinterpret_cast<float*>(data)[i] = static_cast<float>(value); break;
    default:  reinterpret_cast<double*>(data)[i] = value; break;
  }
}

// bytes per voxel of the ECAT7 data types (0: unsupported)
int ecatNumberSize(CECATSubHeader::Data_Type type)
{
  switch(type) {
    case CECATSubHeader::ByteData:  return 1;
    case CECATSubHeader::VAX_Ix2:
    case CECATSubHeader::SunShort:  return 2;
    case CECATSubHeader::VAX_Ix4:
    case CECATSubHeader::SunLong:
    case CECATSubHeader::VAX_Rx4:
    case CECATSubHeader::IEEEFloat: return 4;
    default:                        return 0;
  }
}

// NIfTI datatype holding the values of an ECAT7 data type unchanged
int niftiDatatypeOf(CECATSubHeader::Data_Type type)
{
  switch(type) {
    case CECATSubHeader::ByteData:  return 2;
    case CECATSubHeader::VAX_Ix2:
    case CECATSubHeader::SunShort:  return 4;
    case CECATSubHeader::VAX_Ix4:
    case CECATSubHeader::SunLong:   return 8;
    case CECATSubHeader::VAX_Rx4:
    case CECATSubHeader::IEEEFloat: return 16;
    default:                        return 0;
  }
}

// value of voxel i of ECAT7 data in machine byte order
double ecatNumber(const char* data, qint64 i, CECATSubHeader::Data_Type type)
{
  switch(type) {
    case CECATSubHeader::ByteData:  return reinterpret_cast<const quint8*>(data)[i];
    case CECATSubHeader::VAX_Ix4:
    case CECATSubHeader::SunLong:   return reinterpret_cast<const qint32*>(data)[i];
    case CECATSubHeader::VAX_Rx4:
    case CECATSubHeader::IEEEFloat: return reinterpret_cast<const float*>(data)[i];
    default:                        return reinterpret_cast<const qint16*>(data)[i];
  }
}

// convert NIfTI voxel data (machine byte order) into an ECAT7 data type
// (see CNIFTIFile.h); physical value = result x scale
bool ecatDataFromNifti(const QByteArray& src, const NIFTIScaling& nifti, QByteArray& dst,
                       CECATSubHeader::Data_Type& type, double& scale)
{
  const int size = niftiNumberSize(nifti.datatype);
  if(size == 0) {
    E("NIfTI datatype %d cannot be converted into an ECAT7 data type", nifti.datatype);
    return false;
  }

  const qint64 voxels = src.size() / size;
  const char* data = src.constData();
  bool physical = (nifti.inter != 0.0);

  // uint32 values are kept as 32 bit integers if they fit
  if(physical == false && nifti.datatype == 768) {
    const quint32* v = reinterpret_cast<const quint32*>(data);
    for(qint64 i=0; i < voxels && physical == false; i++)
      physical = (v[i] > static_cast<quint32>(std::numeric_limits<qint32>::max()));
  }

  if(physical || nifti.datatype == 64) {
    // physical values as float32
    dst = QByteArray(voxels * 4, '\0');
    float* v = reinterpret_cast<float*>(dst.data());
    for(qint64 i=0; i < voxels; i++)
      v[i] = static_cast<float>(niftiNumber(data, i, nifti.datatype) * nifti.slope + nifti.inter);

    type = CECATSubHeader::IEEEFloat;
    scale = 1.0;
    return true;
  }

  scale = nifti.slope;

  switch(nifti.datatype) {
    case 2:   type = CECATSubHeader::ByteData;  dst = src; break;
    case 4:   type = CECATSubHeader::SunShort;  dst = src; break;
    case 8:   type = CECATSubHeader::SunLong;   dst = src; break;
    case 16:  type = CECATSubHeader::IEEEFloat; dst = src; break;

    case 256: { // int8 -> int16
      type = CECATSubHeader::SunShort;
      dst = QByteArray(voxels * 2, '\0');
      qint16* v = reinterpret_cast<qint16*>(dst.data());
      for(qint64 i=0; i < voxels; i++)
        v[i] = reinterpret_cast<const qint8*>(data)[i];
    }
    break;

    default: { // uint16, uint32 -> int32
      type = CECATSubHeader::SunLong;
      dst = QByteArray(voxels * 4, '\0');
      qint32* v = reinterpret_cast<qint32*>(dst.data());
      for(qint64 i=0; i < voxels; i++)
        v[i] = static_cast<qint32>(niftiNumber(data, i, nifti.datatype));
    }
    break;
  }

  return true;
}

// convert ECAT7 voxel data (machine byte order) with the given factor to
// physical values (scale factor x calibration factor) into the datatype and scaling of a NIfTI header (see CNIFTIFile.h)
bool niftiDataFromEcat(const char* src, qint64 voxels, CECATSubHeader::Data_Type type, double scale,
                       const NIFTIScaling& nifti, QByteArray& dst)
{
  const int size = niftiNumberSize(nifti.datatype);
  if(size == 0) {
    E("ECAT7 data cannot be converted into the NIfTI datatype %d", nifti.datatype);
    return false;
  }

  // same datatype and scaling: copy the stored values
  if(niftiDatatypeOf(type) == nifti.datatype && scale == nifti.slope && nifti.inter == 0.0) {
    dst = QByteArray(src, voxels * size);
    return true;
  }

  double minValue = 0.0, maxValue = 0.0;
  const bool integer = niftiIntegerRange(nifti.datatype, minValue, maxValue);

  dst = QByteArray(voxels * size, '\0');
  for(qint64 i=0; i < voxels; i++) {
    double value = (ecatNumber(src, i, type) * scale - nifti.inter) / nifti.slope;

    if(integer) {
      value = std::floor(value + 0.5);
      if(std::isfinite(value) == false || value < minValue || value > maxValue) {
        E("ECAT7 voxel %lld (%g) does not fit into the NIfTI datatype %d with scl_slope %g and scl_inter %g",
          i, ecatNumber(src, i, type) * scale, nifti.datatype, nifti.slope, nifti.inter);
        dst.clear();
        return false;
      }
    }

    setNiftiNumber(dst.data(), i, nifti.datatype, value);
  }

  return true;
}

} // namespace

//------------------------------------------------------------------------------------------------
// method of the private class to determine the layout of volume frame (1-based,
// -1: first volume); for ECATOrientation also the ECAT7 geometry
bool CNIFTIFilePrivate::volumeLayout(short& frame, CMedIOData::Orientation orientation, Volume& volume) const {
  ENTER();

  qint64 offset = 0;
  qint64 size = 0;
  if(matrixLayout(offset, size, volume.swapSize) == false) {
    RETURN(false);
    return false;
  }

  qint64 dims[8];
  for(int i=0; i < 8; i++) {
    dims[i] = (iNIFTIformat == CNIFTIFile::NIFTI1) ? static_cast<const CNIFTI1MainHeader*>(cachedMainHeader)->dim(i)
                                                   : static_cast<const CNIFTI2MainHeader*>(cachedMainHeader)->dim(i);
  }

  // dimensions of a volume and number of volumes (dims are valid, see matrixLayout())
  qint64 voxels = 1;
  qint64 volumes = 1;
  for(int i=1; i <= 3; i++) {
    volume.dims[i-1] = (i <= dims[0]) ? dims[i] : 1;
    voxels *= volume.dims[i-1];
  }
  for(int i=4; i <= dims[0]; i++)
    volumes *= dims[i];

  if(orientation == CMedIOData::ECATOrientation) {
    QString error;
    if(ecatGeometryFromNIFTI(cachedMainHeader, volume.geometry, &error) == false) {
      E("%s", error.toLatin1().constData());
      RETURN(false);
      return false;
    }
    for(int i=5; i <= dims[0]; i++) {
      if(dims[i] > 1) {
        E("NIfTI data with more than 4 dimensions cannot be accessed in ECAT7 orientation");
        RETURN(false);
        return false;
      }
    }
    if(volume.geometry.oblique)
      W("%s: oblique NIfTI orientation, the voxel axes are assigned to the closest patient axes",
        originalFileName.toLocal8Bit().constData());
  } else if(orientation != CMedIOData::NativeOrientation) {
    E("unsupported orientation %d", static_cast<int>(orientation));
    RETURN(false);
    return false;
  }

  if(frame == -1)
    frame = 1;
  if(frame < 1 || frame > volumes) {
    E("frame %d does not exist (%lld frames)", frame, volumes);
    RETURN(false);
    return false;
  }

  volume.bytesPerVoxel = static_cast<int>(size / (voxels * volumes));
  volume.size = voxels * volume.bytesPerVoxel;
  volume.offset = offset + (frame - 1) * volume.size;

  RETURN(true);
  return true;
}

//------------------------------------------------------------------------------------------------
// method of the private class to read one volume in machine byte order and the given orientation
bool CNIFTIFilePrivate::readVolume(CNIFTIFile* file, short& frame, CMedIOData::Orientation orientation,
                                   QByteArray& data, Volume& volume) const {
  ENTER();
  bool result = false;
  QFile* f = matrixFile(file);

  if(f && f->isReadable() && volumeLayout(frame, orientation, volume)) {
    if(volume.size > std::numeric_limits<int>::max()) {
      E("volume of %lld bytes is too large for a QByteArray", volume.size);
    } else if(f->size() < volume.offset + volume.size) {
      E("file too short: frame %d requires %lld bytes at offset %lld", frame, volume.size, volume.offset);
    } else if(f->seek(volume.offset)) {
      QByteArray raw = f->read(volume.size);

      if(raw.size() == volume.size) {
        swapMatrix(raw.data(), volume.size, volume.swapSize);

        if(orientation == CMedIOData::ECATOrientation) {
          // reorder into the ECAT7 voxel order
          const ECATGeometryFromNIFTI& g = volume.geometry;
          const int n = volume.bytesPerVoxel;
          data = QByteArray(volume.size, '\0');
          char* dst = data.data();

          for(int k=0; k < g.dims[2]; k++)
            for(int j=0; j < g.dims[1]; j++)
              for(int i=0; i < g.dims[0]; i++, dst += n)
                memcpy(dst, raw.constData() + niftiVoxelIndex(g, volume.dims, i, j, k) * n, n);
        } else {
          data = raw;
        }

        result = true;
      }
    }
  }

  RETURN(result);
  return result;
}

//------------------------------------------------------------------------------------------------
// method of the private class to write one volume given in machine byte order and the given orientation
bool CNIFTIFilePrivate::writeVolume(CNIFTIFile* file, short frame, CMedIOData::Orientation orientation,
                                    const char* data, qint64 size) const {
  ENTER();
  bool result = false;
  Volume volume;

  // the main header has to be written first as it defines the matrix layout
  QFile* f = matrixFile(file);

  if(f && f->isWritable() && volumeLayout(frame, orientation, volume)) {
    if(size != volume.size) {
      E("size of the volume (%lld bytes) does not match the main header (%lld bytes)", size, volume.size);
    } else {
      QByteArray raw;

      if(orientation == CMedIOData::ECATOrientation) {
        // reorder from the ECAT7 voxel order into the voxel order of the file
        const ECATGeometryFromNIFTI& g = volume.geometry;
        const int n = volume.bytesPerVoxel;
        raw = QByteArray(volume.size, '\0');
        const char* src = data;

        for(int k=0; k < g.dims[2]; k++)
          for(int j=0; j < g.dims[1]; j++)
            for(int i=0; i < g.dims[0]; i++, src += n)
              memcpy(raw.data() + niftiVoxelIndex(g, volume.dims, i, j, k) * n, src, n);
      } else {
        raw = QByteArray(data, size);
      }

      // the voxel data has to be stored in the byte order of the main header
      swapMatrix(raw.data(), volume.size, volume.swapSize);

      if(f->seek(volume.offset))
        result = (f->write(raw) == volume.size);
    }
  }

  RETURN(result);
  return result;
}

//------------------------------------------------------------------------------------------------
// method of the private class to read the metadata of CNIFTIConverter: JSON
// header extension or JSON sidecar file (see CNIFTIConverter.h)
QJsonObject CNIFTIFilePrivate::metadata(CNIFTIFile* file) const {
  ENTER();
  QJsonObject json;

  if(iNIFTIformat == CNIFTIFile::NIFTI1)
    json = static_cast<const CNIFTI1MainHeader*>(cachedMainHeader)->readHeaderExtension(*file);
  else if(iNIFTIformat == CNIFTIFile::NIFTI2)
    json = static_cast<const CNIFTI2MainHeader*>(cachedMainHeader)->readHeaderExtension(*file);

  if(json.isEmpty()) {
    QFile sidecar(niftiSidecarFileName(originalFileName));
    if(sidecar.open(QIODevice::ReadOnly))
      json = QJsonDocument::fromJson(sidecar.readAll()).object();
  }

  LEAVE();
  return json;
}

//------------------------------------------------------------------------------------------------
bool CNIFTIFile::readMatrix(QByteArray*& matrixData, CMedIOData::Orientation orientation, short frame) {
  ENTER();
  CNIFTIFilePrivate::Volume volume;
  QByteArray data;

  matrixData = NULL;
  bool result = m_pData->readVolume(this, frame, orientation, data, volume);
  if(result)
    matrixData = new QByteArray(data);

  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
bool CNIFTIFile::readMatrix(char*& matrixData, unsigned int& len, CMedIOData::Orientation orientation, short frame) {
  ENTER();
  CNIFTIFilePrivate::Volume volume;
  QByteArray data;

  matrixData = NULL;
  len = 0;
  bool result = m_pData->readVolume(this, frame, orientation, data, volume);
  if(result) {
    matrixData = new char[data.size()];
    memcpy(matrixData, data.constData(), data.size());
    len = static_cast<unsigned int>(data.size());
  }

  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
bool CNIFTIFile::writeMatrix(const QByteArray& matrixData, CMedIOData::Orientation orientation, short frame) {
  ENTER();
  bool result = m_pData->writeVolume(this, frame, orientation, matrixData.constData(), matrixData.size());
  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
bool CNIFTIFile::writeMatrix(const char* matrixData, unsigned int size, CMedIOData::Orientation orientation, short frame) {
  ENTER();
  bool result = (matrixData != NULL) && m_pData->writeVolume(this, frame, orientation, matrixData, size);
  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
bool CNIFTIFile::readMatrix(QByteArray*& matrixData, CECATSubHeader*& subHeader, short frame) {
  ENTER();
  CNIFTIFilePrivate* d = m_pData;
  CNIFTIFilePrivate::Volume volume;
  QByteArray data;

  matrixData = NULL;
  subHeader = NULL;

  bool result = d->readVolume(this, frame, CMedIOData::ECATOrientation, data, volume);

  // ECAT7 data type
  QByteArray ecatData;
  CECATSubHeader::Data_Type type = CECATSubHeader::UnknownDataType;
  double scale = 1.0;
  if(result)
    result = ecatDataFromNifti(data, niftiScaling(d->cachedMainHeader), ecatData, type, scale);

  // ECAT7 image sub header of this volume
  CECAT7SubHeaderImage* s = NULL;
  if(result) {
    s = new CECAT7SubHeaderImage();
    result = s->convertFrom(NULL, d->cachedMainHeader);
  }

  if(result) {
    s->setData_Type(type);
    s->setScale_Factor(scale);

    // value range of the stored values (16 bit sub header fields)
    if(type == CECATSubHeader::ByteData || type == CECATSubHeader::SunShort) {
      const qint64 voxels = ecatData.size() / ecatNumberSize(type);
      double minValue = 0.0, maxValue = 0.0;
      for(qint64 i=0; i < voxels; i++) {
        const double value = ecatNumber(ecatData.constData(), i, type);
        if(i == 0 || value < minValue)
          minValue = value;
        if(i == 0 || value > maxValue)
          maxValue = value;
      }
      s->setImage_Min(static_cast<short>(minValue));
      s->setImage_Max(static_cast<short>(maxValue));
    }

    // frame timing and reconstruction zoom from the metadata, the header otherwise (s -> ms)
    const QJsonObject json = d->metadata(this);
    double start = 0.0, duration = 0.0;
    niftiFrameTiming(volume.geometry, json, frame - 1, start, duration);
    s->setFrame_Start_Time(ecatFrameTime(start));
    s->setFrame_Duration(ecatFrameTime(duration));

    if(json.contains("Recon_Zoom"))
      s->setRecon_Zoom(json["Recon_Zoom"].toDouble());

    matrixData = new QByteArray(ecatData);
    subHeader = s;
  } else {
    delete s;
  }

  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
bool CNIFTIFile::readMatrix(char*& matrixData, unsigned int& len, CECATSubHeader*& subHeader, short frame) {
  ENTER();
  QByteArray* data = NULL;

  matrixData = NULL;
  len = 0;
  bool result = readMatrix(data, subHeader, frame);
  if(result) {
    matrixData = new char[data->size()];
    memcpy(matrixData, data->constData(), data->size());
    len = static_cast<unsigned int>(data->size());
  }
  delete data;

  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
bool CNIFTIFile::writeMatrix(const QByteArray& matrixData, const CECATSubHeader& subHeader, short frame,
                             const CECATMainHeader* mainHeader) {
  ENTER();
  bool result = writeMatrix(matrixData.constData(), matrixData.size(), subHeader, frame, mainHeader);
  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
bool CNIFTIFile::writeMatrix(const char* matrixData, unsigned int size, const CECATSubHeader& subHeader, short frame,
                             const CECATMainHeader* mainHeader) {
  ENTER();
  CNIFTIFilePrivate* d = m_pData;
  CNIFTIFilePrivate::Volume volume;
  bool result = false;

  if(matrixData == NULL || subHeader.subHeaderType() != CECATSubHeader::ECAT7_Image) {
    E("only ECAT7 image data can be written to NIfTI files");
  } else if(d->cachedMainHeader && d->volumeLayout(frame, CMedIOData::ECATOrientation, volume)) {
    const CECAT7SubHeaderImage& s = static_cast<const CECAT7SubHeaderImage&>(subHeader);
    const ECATGeometryFromNIFTI& g = volume.geometry;
    const CECATSubHeader::Data_Type type = s.data_Type();
    const qint64 voxels = static_cast<qint64>(g.dims[0]) * g.dims[1] * g.dims[2];

    if(s.x_Dimension() != g.dims[0] || s.y_Dimension() != g.dims[1] || s.z_Dimension() != g.dims[2]) {
      E("dimensions of the ECAT7 sub header (%d x %d x %d) do not match the NIfTI main header (%d x %d x %d)",
        s.x_Dimension(), s.y_Dimension(), s.z_Dimension(), g.dims[0], g.dims[1], g.dims[2]);
    } else if(ecatNumberSize(type) == 0) {
      E("unsupported ECAT7 data type %d", static_cast<int>(type));
    } else if(static_cast<qint64>(size) < voxels * ecatNumberSize(type) ||
              static_cast<qint64>(size) > (voxels * ecatNumberSize(type) + 511) / 512 * 512) {
      // CECATFile::readMatrix() returns the matrix padded to ECAT7 blocks of 512 bytes
      E("size of the ECAT7 matrix (%u bytes) does not match its sub header (%lld bytes)", size, voxels * ecatNumberSize(type));
    } else {
      QByteArray data;
      double scale = s.scale_Factor();
      if(scale == 0.0 || std::isfinite(scale) == false)
        scale = 1.0;
      scale *= ecatCalibrationFactor(mainHeader);

      result = niftiDataFromEcat(matrixData, voxels, type, scale, niftiScaling(d->cachedMainHeader), data) &&
               d->writeVolume(this, frame, CMedIOData::ECATOrientation, data.constData(), data.size());
    }
  }

  RETURN(result);
  return result;
}
