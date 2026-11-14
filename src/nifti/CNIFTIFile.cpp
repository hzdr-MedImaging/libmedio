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
#include "CMedIOHeaderMapping.h"
#include "CMedIOMatrix.h"
#include "CMedIOData.h"
#include "CNIFTI1MainHeader.h"
#include "CNIFTI2MainHeader.h"
#include "CNIFTIExtensions.h"

#include <QFileInfo>
#include <QTemporaryFile>
#include <QDir>

#include <zlib.h>

#include <algorithm>
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

    // methods
    bool syncMainHeader(CNIFTIFile* file) const;
    bool matrixLayout(qint64& offset, qint64& size, int& swapSize) const;
    bool volumeLayout(short frame, qint64& offset, qint64& size, int& swapSize) const;
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
        if(d->cachedMainHeader->load()) {
          // the attributes of the JSON header extension or sidecar file
          MedIOHeaderMapping::readNiftiAttributes(*this, *d->cachedMainHeader);
          result = true;
        }
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
  const bool writing = isOpen() && isWritable();
  const bool compress = d->compressed && d->temporaryFile != NULL && writing;

  // data still buffered is written now: errors (e.g. a full disk) are reported
  // by errorStatus()
  bool result = true;
  if(writing) {
    result = QFile::flush();
    if(d->imageFile && d->imageFile->isOpen())
      result = d->imageFile->flush() && result;
  }

  QFile::close();
  if(d->imageFile)
    d->imageFile->close();

  if(writing && result == false)
    E("Unable to write NIfTI file %s", d->originalFileName.toLocal8Bit().constData());

  if(compress && result) {
    result = compressGzipFile(d->temporaryFile->fileName(), d->headerFileName);

    if(result && d->pair && d->imageTemporaryFile)
      result = compressGzipFile(d->imageTemporaryFile->fileName(), d->imageFileName);

    if(result)
      D("Compressed NIfTI file successfully written.");
    else
      E("Unable to compress NIfTI file %s", d->originalFileName.toLocal8Bit().constData());
  }

  if(writing)
    m_iErrStatus = result ? MERR_Ok : MERR_Unspecified;

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

  // the header saves itself with its attributes as JSON and DICOM header
  // extensions (between the header and vox_offset in single files, at the end
  // of the header file of header/image pairs) and the descrip of PMOD, see
  // CNIFTIExtensions.h, so that a direct save() of the header writes the same
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
      if(swapSize > 1) {
        // the voxel data has to be stored in the byte order of the main
        // header: swapped in chunks (it can be larger than a QByteArray)
        const qint64 chunkSize = (16 * 1024 * 1024 / swapSize) * swapSize;
        QByteArray chunk;
        while(bytesWritten < matrixSize) {
          const qint64 n = qMin(chunkSize, matrixSize - bytesWritten);
          chunk = QByteArray(matrixData + bytesWritten, static_cast<int>(n));
          swapMatrix(chunk.data(), n, swapSize);
          if(file->write(chunk) != n)
            break;
          bytesWritten += n;
        }
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
// FRAME-WISE MATRIX I/O

//------------------------------------------------------------------------------------------------
// method of the private class to determine the file position and size (in
// bytes) of volume frame (1-based) of the voxel matrix
bool CNIFTIFilePrivate::volumeLayout(short frame, qint64& offset, qint64& size, int& swapSize) const {
  ENTER();

  qint64 matrixSize = 0;
  if(matrixLayout(offset, matrixSize, swapSize) == false) {
    RETURN(false);
    return false;
  }

  // number of volumes: dim[4..7] (the dimensions are valid, see matrixLayout())
  qint64 volumes = 1;
  for(int i=4; i < 8; i++) {
    const qint64 dim = (iNIFTIformat == CNIFTIFile::NIFTI1) ? static_cast<const CNIFTI1MainHeader*>(cachedMainHeader)->dim(i)
                                                            : static_cast<const CNIFTI2MainHeader*>(cachedMainHeader)->dim(i);
    const qint64 dim0 = (iNIFTIformat == CNIFTIFile::NIFTI1) ? static_cast<const CNIFTI1MainHeader*>(cachedMainHeader)->dim(0)
                                                             : static_cast<const CNIFTI2MainHeader*>(cachedMainHeader)->dim(0);
    if(i <= dim0)
      volumes *= dim;
  }

  if(frame < 1 || frame > volumes) {
    E("frame %d does not exist (%lld frames)", frame, volumes);
    RETURN(false);
    return false;
  }

  size = matrixSize / volumes;
  offset += (frame - 1) * size;

  RETURN(true);
  return true;
}

//------------------------------------------------------------------------------------------------
bool CNIFTIFile::readMatrix(QByteArray*& matrixData, short frame) {
  ENTER();
  bool result = false;
  qint64 offset = 0;
  qint64 size = 0;
  int swapSize = 0;

  matrixData = NULL;

  QFile* file = m_pData->matrixFile(this);

  if(file && file->isReadable() && m_pData->volumeLayout(frame, offset, size, swapSize)) {
    if(size > std::numeric_limits<int>::max()) {
      E("volume of %lld bytes is too large for a QByteArray", size);
    } else if(file->size() < offset + size) {
      E("file too short: frame %d requires %lld bytes at offset %lld", frame, size, offset);
    } else if(file->seek(offset)) {
      matrixData = new QByteArray(file->read(size));

      if(matrixData->size() == size) {
        // return the voxel data in the byte order of this machine
        swapMatrix(matrixData->data(), size, swapSize);
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
bool CNIFTIFile::writeMatrix(const QByteArray& matrixData, short frame) {
  ENTER();
  bool result = false;
  qint64 offset = 0;
  qint64 size = 0;
  int swapSize = 0;

  // the main header has to be written first as it defines the matrix layout
  QFile* file = m_pData->matrixFile(this);

  if(file && file->isWritable() && m_pData->volumeLayout(frame, offset, size, swapSize)) {
    if(matrixData.size() != size) {
      E("size of the volume (%d bytes) does not match the main header (%lld bytes)", matrixData.size(), size);
    } else if(file->seek(offset)) {
      if(swapSize > 1) {
        // the voxel data has to be stored in the byte order of the main header
        QByteArray data(matrixData);
        swapMatrix(data.data(), size, swapSize);
        result = (file->write(data) == size);
      } else {
        result = (file->write(matrixData) == size);
      }
    }
  }

  RETURN(result);
  return result;
}
//------------------------------------------------------------------------------------------------
bool CNIFTIFile::writeMatrix(const QByteArray& matrixData, short frame, double frameStart, double frameDuration) {
  ENTER();
  bool result = false;
  qint64 offset = 0;
  qint64 size = 0;
  int swapSize = 0;

  // the size is checked before the timing is stored (which may move vox_offset)
  if(isWritable() == false || m_pData->volumeLayout(frame, offset, size, swapSize) == false) {
    E("the NIfTI main header has to be written first");
  } else if(matrixData.size() != size) {
    E("size of the volume (%d bytes) does not match the main header (%lld bytes)", matrixData.size(), size);
  } else if(MedIOMatrix::storeNiftiFrameTiming(this, frame, frameStart, frameDuration)) {
    result = writeMatrix(matrixData, frame);
  }

  RETURN(result);
  return result;
}
