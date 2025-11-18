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

#ifndef CPHILIPSFILE_H
#define CPHILIPSFILE_H

#include <QFile>
#include <QString>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOData>
#include <CPhilipsMainHeader>
#include <CPhilipsSubHeader>
#else
#include <CMedIOData.h>
#include <CPhilipsMainHeader.h>
#include <CPhilipsSubHeader.h>
#endif

// forward declarations
class CPhilipsFilePrivate;
class CPhilipsDirectory;

class CPhilipsFile : public CMedIOData
{
  public:
    // constructors
    CPhilipsFile(const QString& filename,
                 CPhilipsMainHeader::File_Type fileType = CPhilipsMainHeader::Unknown);
    CPhilipsFile(CPhilipsMainHeader::File_Type fileType = CPhilipsMainHeader::Unknown);
    ~CPhilipsFile();

    // static methods for the MedIOData Interface
    static bool isOfType(const QString& filename);
    static CMedIOData* createFromFile(const QString& fileName);

    // runtime type information
    CMedIOData::Format dataFormat() const;

    // the file open/close methods
    bool open(QIODevice::OpenModeFlag mode);
    void close(void);

    CPhilipsMainHeader::File_Type fileType();
    CPhilipsSubHeader::Type subHeaderType();

    // methods to calculate the real amount
    // of frame/plane/gate numbers carried in the directory.
    short maxFrame(void) const;
    short minFrame(void) const;
    short numFrames(void) const;
    short maxSlice(void) const;
    short minSlice(void) const;
    short numSlices(short frame=-1) const;
    short maxTilt(void) const;
    short minTilt(void) const;
    short numTilts(void) const;

    // interface methods to read out specific data from the philips files
    bool readMainHeader(CPhilipsMainHeader*& mainHeader);
    bool readSubHeader(CPhilipsSubHeader*& subHeader, short frame=-1, short slice=-1, short tilt=-1);
    bool readMatrix(QByteArray*& matrixData,
                    short frame=-1, short slice=-1, short tilt=-1);
    bool readMatrix(char*& matrixData, unsigned int& len,
                    short frame=-1, short slice=-1, short tilt=-1);
    bool readMatrix(QByteArray*& matrixData, CPhilipsSubHeader*& subHeader,
                    short frame=-1, short slice=-1, short tilt=-1);
    bool readMatrix(char*& matrixData, unsigned int& len, CPhilipsSubHeader*& subHeader,
                    short frame=-1, short slice=-1, short tilt=-1);

    // methods to write out headers/data to the file.
    bool writeMainHeader(CPhilipsMainHeader& mainHeader);
    bool writeSubHeader(const CPhilipsSubHeader& subHeader,
                        short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const QByteArray& matrixData,
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const char* matrixData, unsigned int size,
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const QByteArray& matrixData, const CPhilipsSubHeader& subHeader,
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const char* matrixData, unsigned int size, const CPhilipsSubHeader& subHeader,
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const QByteArray& matrixData, CPhilipsSubHeader::Data_Type type, 
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const char* matrixData, unsigned int size, CPhilipsSubHeader::Data_Type type,
                     short frame=-1, short slice=-1, short tilt=-1);

    // more advanced methods to access the directory list of a philips file
    CPhilipsDirectory* directory() const;

    // methods to generate some objects just for convienence
    CPhilipsMainHeader* createEmptyMainHeader();
    CPhilipsSubHeader* createEmptySubHeader();

    // internal methods to sync specific data with our headers
    void mainHeaderWritten(const CPhilipsMainHeader& mainHeader);
    bool reWriteMainHeader();

  private:
    CPhilipsFilePrivate* m_pData;
};

#endif // CPHILIPSFILE_H
