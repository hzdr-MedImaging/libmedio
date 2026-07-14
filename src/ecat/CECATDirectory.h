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

#ifndef CECATDIRECTORY_H
#define CECATDIRECTORY_H

#ifndef __MEDIO_PRIVATE__
#include <CECATDirectoryItem>
#else
#include <CECATDirectoryItem.h>
#endif

// forward declarations
class CECATDirectoryPrivate;
class CECATFile;

class CECATDirectory
{
  public:
    // constructors    
    CECATDirectory(CECATFile* ecatFile);
    ~CECATDirectory();

    // copy constructur and default assignment operator
    CECATDirectory(const CECATDirectory& src);    
    CECATDirectory& operator=(const CECATDirectory& src);  

    // file i/o routines
    bool load(void);
    bool save(void) const;

    // accessor methods
    bool isEmpty() const;
    unsigned int count() const;
    CECATDirectoryItem* operator[](unsigned int num) const;
    CECATDirectoryItem* item(short frame=-1, short plane=-1,
                             short gate=-1, short bed=-1, short data=-1);

    // methods to calculate the real amount
    // of frame/plane/gate numbers carried in the directory.
    short maxFrame(void) const;
    short minFrame(void) const;
    short numFrames(void) const;
    short maxPlane(void) const;
    short minPlane(void) const;
    short numPlanes(void) const;
    short maxGate(void) const;
    short minGate(void) const;
    short numGates(void) const;
    short maxBedPos(void) const;
    short minBedPos(void) const;
    short numBedPos(void) const;
    short maxData(void) const;
    short minData(void) const;
    short numData(void) const;

    // read methods
    bool readSubHeader(CECATSubHeader*& subHeader,
                       short frame=-1, short plane=-1, short gate=-1, short bed=-1, short data=-1);
    bool readMatrix(QByteArray*& matrixData,
                    short frame=-1, short plane=-1, short gate=-1, short bed=-1, short data=-1);
    bool readMatrix(char*& matrixData, unsigned int& len,
                    short frame=-1, short plane=-1, short gate=-1, short bed=-1, short data=-1);
    bool readMatrix(QByteArray*& matrixData, CECATSubHeader*& subHeader,
                    short frame=-1, short plane=-1, short gate=-1, short bed=-1, short data=-1);
    bool readMatrix(char*& matrixData, unsigned int& len, CECATSubHeader*& subHeader,
                    short frame=-1, short plane=-1, short gate=-1, short bed=-1, short data=-1);  
  
    // write methods
    bool writeSubHeader(const CECATSubHeader& subHeader,
                        short frame=-1, short plane=-1, short gate=-1, short bed=-1, short data=-1);
    bool writeMatrix(const QByteArray& matrix,
                     short frame=-1, short plane=-1, short gate=-1, short bed=-1, short data=-1);
    bool writeMatrix(const char* matrix, unsigned int size,
                     short frame=-1, short plane=-1, short gate=-1, short bed=-1, short data=-1);
    bool writeMatrix(const QByteArray& matrix, CECATSubHeader::Data_Type type,
                     short frame=-1, short plane=-1, short gate=-1, short bed=-1, short data=-1);
    bool writeMatrix(const char* matrix, unsigned int size, CECATSubHeader::Data_Type type,
                     short frame=-1, short plane=-1, short gate=-1, short bed=-1, short data=-1);    
    bool writeMatrix(const QByteArray& matrix, const CECATSubHeader& subHeader,
                     short frame=-1, short plane=-1, short gate=-1, short bed=-1, short data=-1);
    bool writeMatrix(const char* matrix, unsigned int size, const CECATSubHeader& subHeader,
                     short frame=-1, short plane=-1, short gate=-1, short bed=-1, short data=-1);

  private:
    CECATDirectoryPrivate* m_pData;
};

#endif // CECATDIRECTORY_H
