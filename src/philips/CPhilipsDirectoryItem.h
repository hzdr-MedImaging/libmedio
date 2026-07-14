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

#ifndef CPHILIPSDIRECTORYITEM_H
#define CPHILIPSDIRECTORYITEM_H

#ifndef __MEDIO_PRIVATE__
#include <CPhilipsSubHeader>
#else
#include <CPhilipsSubHeader.h>

// some useful defines on the Philips file format
// especially for PhilipsBlock calculation as Philips Files
// are seperated in 512 byte blocks and positions within
// the directorylist are refered in this format starting
// from 1.
#define PHILIPS_BLOCKSIZE        512
#define PHILIPS_POS_MAINHEADER   1    // the MainHeader is always at block 1
#define PHILIPS_POS_MAINDIR      2    // the MainDir is always at block 2
#define PhilipsBlock2FilePos(v)  ((static_cast<qint64>(v)-1)*PHILIPS_BLOCKSIZE)
#define FilePos2PhilipsBlock(v)  (static_cast<qint64>(v)/PHILIPS_BLOCKSIZE+1)
#define PHILIPS_EXT_HEADER_ID    0x7FFFFFFF // matrixID for the extended header
#define PHILIPS_EXT_HEADER_POS   3          // fixed position of extended header
#define PHILIPS_EXT_HEADER_END   18         // fixed end-position of ext. header
#endif

// special macros to convert the MatrixID to their respect
// slice/frame and tilt representation
#define matrixID2Frame(x)  ( (x) & 0x0000FFFF)
#define matrixID2Slice(x)  (((x) & 0x0FFF0000) >> 16)
#define matrixID2Tilt(x)   (((x) & 0xF0000000) >> 28)

#define frame2MatrixID(x)  ( (x) & 0x0000FFFF)
#define slice2MatrixID(x)  (((x) & 0x00000FFF) << 16)
#define tilt2MatrixID(x)   (((x) & 0x0000000F) << 28)

#define convertToMatrixID(s, f, t) (slice2MatrixID(s) | \
                                    frame2MatrixID(f) | \
                                    tilt2MatrixID(t) )
// forward declarations
class CPhilipsDirectoryItemPrivate;
class CPhilipsFile;

class CPhilipsDirectoryItem
{
  public:
    enum CompressionFlag { Uncompressed = 0,
                           Compressed = 1
                         };

    enum ContentFlag { Used = 1,
                       Unused = 0,
                       Removed = -1,
                       Header = -2
                     };

    // constructors
    CPhilipsDirectoryItem(CPhilipsFile* file,
                          quint32 matrixID = 0);
    ~CPhilipsDirectoryItem();

    // copy constructur and default assignment operator
    CPhilipsDirectoryItem(const CPhilipsDirectoryItem& src);    
    CPhilipsDirectoryItem& operator=(const CPhilipsDirectoryItem& src);

    // accessor methods
    quint32 matrixID() const;
    bool isExtendedHeader() const;
    qint64 dataBlock_Start() const;
    qint64 dataBlock_End() const;
    CompressionFlag compressionFlag() const;
    ContentFlag contentFlag() const;
    short frame() const;
    short slice() const;
    short tilt() const;

    // mutator methods
    void setMatrixID(const quint32 matrixID);
    void setDataBlock_Start(const qint64 offset);
    void setDataBlock_End(const qint64 offset);
    void setCompressionFlag(const CompressionFlag flag);
    void setContentFlag(const ContentFlag flag);
    void setFrame(const short f);
    void setSlice(const short s);
    void setTilt(const short t);

    // read i/o methods
    bool readSubHeader(CPhilipsSubHeader*& subHeader);
    bool readMatrix(QByteArray*& data);
    bool readMatrix(char*& data, unsigned int& len);
    bool readMatrix(QByteArray*& data, CPhilipsSubHeader*& subHeader);
    bool readMatrix(char*& data, unsigned int& len, CPhilipsSubHeader*& subHeader);

    // write i/o methods
    bool writeSubHeader(const CPhilipsSubHeader& subHeader);
    bool writeMatrix(const QByteArray& data);
    bool writeMatrix(const char* data, unsigned int len);
    bool writeMatrix(const QByteArray& data, CPhilipsSubHeader::Data_Type type);
    bool writeMatrix(const char* data, unsigned int len, CPhilipsSubHeader::Data_Type type);
    bool writeMatrix(const QByteArray& data, const CPhilipsSubHeader& subHeader);
    bool writeMatrix(const char* data, unsigned int len, const CPhilipsSubHeader& subHeader);

    // internal methods to sync specific data with our headers
    void subHeaderWritten(const CPhilipsSubHeader& subHeader);

  private:
    CPhilipsDirectoryItemPrivate* m_pData;
};

#endif // CPHILIPSDIRECTORYITEM_H
