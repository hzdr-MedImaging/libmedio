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

#ifndef CECATDIRECTORYITEM_H
#define CECATDIRECTORYITEM_H

#ifndef __MEDIO_PRIVATE__
#include <CECATSubHeader>
#else
#include <CECATSubHeader.h>
// some useful defines on the ECAT file format
// especially for ECATBlock calculation as ECAT Files
// are seperated in 512 byte blocks and positions within
// the directorylist are refered in this format starting
// from 1.
#define ECAT_BLOCKSIZE        512
#define ECAT_POS_MAINHEADER   1    // the MainHeader is always at block 1
#define ECAT_POS_MAINDIR      2    // the MainDir is always at block 2
#define ECATBlock2FilePos(v)  ((static_cast<qint64>(v)-1)*ECAT_BLOCKSIZE)
#define FilePos2ECATBlock(v)  (static_cast<qint64>(v)/ECAT_BLOCKSIZE+1)

#endif

// special macros to convert the MatrixID to their respect
// frame/plane/gate/bed and data representation
#define matrixID2Frame(x) ((x) & 0x1FF)
#define matrixID2Plane(x) ((((x)>>16) & 0xFF) + ((((x)>>9) & 0x3)<<8))
#define matrixID2Gate(x)  (((x)>>24) & 0x3F)
#define matrixID2Bed(x)   (((x)>>12) & 0xF)
#define matrixID2Data(x)  ((((x)>>9) & 0x4) | (((x)>>30) & 0x3))
#define frame2MatrixID(x) ((x) & 0x1FF)
#define plane2MatrixID(x) ((((x) & 0xFF)<<16) | ((((x) & 0x300)>>8)<<9))
#define gate2MatrixID(x)  (((x) & 0x3F)<<24)
#define bed2MatrixID(x)   (((x) & 0xF)<<12)
#define data2MatrixID(x)  ((((x) & 0x3)<<30) | (((x) & 0x4)<<9))

#define convertToMatrixID(f, p, g, b, d) ( frame2MatrixID(f) | \
                                           plane2MatrixID(p) | \
                                           gate2MatrixID(g)  | \
                                           bed2MatrixID(b)   | \
                                           data2MatrixID(d) )
// forward declarations
class CECATDirectoryItemPrivate;
class CECATFile;

class CECATDirectoryItem
{
  public:
    enum AccessStatus { Deleted=-1, 
                        NotYetWritten=0, 
                        Finished=1
                      };

    // constructors
    CECATDirectoryItem(CECATFile* file,
                       quint32 matrixID = 0);
    ~CECATDirectoryItem();

    // copy constructur and default assignment operator
    CECATDirectoryItem(const CECATDirectoryItem& src);    
    CECATDirectoryItem& operator=(const CECATDirectoryItem& src);      
  
    // accessor methods
    quint32 matrixID(void) const;
    qint64 dataBlock_Start(void) const;
    qint64 dataBlock_End(void) const;
    AccessStatus dataBlock_Status(void) const;
    short frame(void) const;
    short plane(void) const;
    short gate(void) const;
    short bed(void) const;
    short data(void) const;

    // mutator methods
    void setMatrixID(const quint32 matrixID);
    void setDataBlock_Start(const qint64 offset);
    void setDataBlock_End(const qint64 offset);
    void setDataBlock_Status(const AccessStatus status);
    void setFrame(const short f);
    void setPlane(const short p);
    void setGate(const short g);
    void setBed(const short b);
    void setData(const short d);
      
    // read i/o methods
    bool readSubHeader(CECATSubHeader*& subHeader);
    bool readMatrix(QByteArray*& data);
    bool readMatrix(char*& data, unsigned int& len);
    bool readMatrix(QByteArray*& data, CECATSubHeader*& subHeader);
    bool readMatrix(char*& data, unsigned int& len, CECATSubHeader*& subHeader);

    // write i/o methods
    bool writeSubHeader(const CECATSubHeader& subHeader);
    bool writeMatrix(const QByteArray& data);
    bool writeMatrix(const char* data, unsigned int len);
    bool writeMatrix(const QByteArray& data, CECATSubHeader::Data_Type type);
    bool writeMatrix(const char* data, unsigned int len, CECATSubHeader::Data_Type type);
    bool writeMatrix(const QByteArray& data, const CECATSubHeader& subHeader);
    bool writeMatrix(const char* data, unsigned int len, const CECATSubHeader& subHeader);

    // internal methods to sync specific data with our headers
    void subHeaderWritten(const CECATSubHeader& subHeader);

  private:
    CECATDirectoryItemPrivate* m_pData;
};

#endif // CECATDIRECTORYITEM_H
