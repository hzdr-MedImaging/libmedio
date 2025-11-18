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

#ifndef CPHILIPSDIRECTORY_H
#define CPHILIPSDIRECTORY_H

#ifndef __MEDIO_PRIVATE__
#include <CPhilipsDirectoryItem>
#else
#include <CPhilipsDirectoryItem.h>
#endif

// forward declarations
class CPhilipsDirectoryPrivate;
class CPhilipsFile;

class CPhilipsDirectory
{
  public:
    // constructors    
    CPhilipsDirectory(CPhilipsFile* philipsFile);
    ~CPhilipsDirectory();

    // copy constructur and default assignment operator
    CPhilipsDirectory(const CPhilipsDirectory& src);    
    CPhilipsDirectory& operator=(const CPhilipsDirectory& src);  

    // file i/o routines
    bool load();
    bool save() const;

    // load function to support the philips listmode format
    // which has no directory
    bool loadFake();

    // accessor methods
    bool isEmpty() const;
    unsigned int count() const;
    CPhilipsDirectoryItem* operator[](unsigned int num) const;
    CPhilipsDirectoryItem* item(short frame=-1, short slice=-1, short tilt=-1);

    // accessor method to get extended main header item
    CPhilipsDirectoryItem* extendedMainHeaderItem() const;

    // methods to calculate the real amount
    // of frame/slice/tilt numbers carried in the directory.
    short maxFrame(void) const;
    short minFrame(void) const;
    short numFrames(void) const;
    short maxSlice(void) const;
    short minSlice(void) const;
    short numSlices(short frame=-1) const;
    short maxTilt(void) const;
    short minTilt(void) const;
    short numTilts(void) const;

    // read methods
    bool readSubHeader(CPhilipsSubHeader*& subHeader,
                       short frame=-1, short slice=-1, short tilt=-1);
    bool readMatrix(QByteArray*& matrixData,
                    short frame=-1, short slice=-1, short tilt=-1);
    bool readMatrix(char*& matrixData, unsigned int& len,
                    short frame=-1, short slice=-1, short tilt=-1);
    bool readMatrix(QByteArray*& matrixData, CPhilipsSubHeader*& subHeader,
                    short frame=-1, short slice=-1, short tilt=-1);
    bool readMatrix(char*& matrixData, unsigned int& len, CPhilipsSubHeader*& subHeader,
                    short frame=-1, short slice=-1, short tilt=-1);

    // write methods
    bool writeSubHeader(const CPhilipsSubHeader& subHeader,
                        short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const QByteArray& matrix,
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const char* matrix, unsigned int size,
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const QByteArray& matrix, CPhilipsSubHeader::Data_Type type,
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const char* matrix, unsigned int size, CPhilipsSubHeader::Data_Type type,
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const QByteArray& matrix, const CPhilipsSubHeader& subHeader,
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const char* matrix, unsigned int size, const CPhilipsSubHeader& subHeader,
                     short frame=-1, short slice=-1, short tilt=-1);

  private:
    CPhilipsDirectoryPrivate* m_pData;
};

#endif // CPHILIPSDIRECTORY_H
