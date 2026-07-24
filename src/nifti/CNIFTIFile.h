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

#ifndef CNIFTIFILE_H
#define CNIFTIFILE_H

// Qt libraries 
#include <QFile>
#include <QString>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOData>
#include <CNIFTIMainHeader>

#else
#include <CMedIOData.h>
#include <CNIFTIMainHeader.h>

#endif


// forward declarations
class CNIFTIFilePrivate;

class CNIFTIFile : public CMedIOData { // CNIFTIFile class inherits from CMedIOData class, which is a base class for medical data formats

    public:
        enum NIFTIFormat  { Undefined=0, NIFTI1, NIFTI2 }; // enum to define the NIfTI file format version

//=============================================================================================
    //constructors
        CNIFTIFile(const QString& filename,
                  CNIFTIMainHeader::Type fileType = CNIFTIMainHeader::Unknown);
        CNIFTIFile(CNIFTIMainHeader::Type fileType = CNIFTIMainHeader::Unknown);
        
    //destructor
        ~CNIFTIFile();

//=============================================================================================
    // static methods for the MedIOData interface
         // static methods can be used without creating an instance of the class, 
         // they are used to check if a file is of the correct type and to create an instance of the class from a file 
        static bool isOfType(const QString& fileName); // to check if a file is of the correct type
        static CMedIOData* createFromFile(const QString& fileName); // to create an instance of the class from a file

//=============================================================================================

    // runtime type information
        CMedIOData::Format dataFormat() const; // dataFormat() (pure virtual function defined in the CMedIOData class) that returns the format of the data 
    // the file (PET scan files) open/close methods
        bool open(QIODevice::OpenModeFlag mode);
        void close(void);
    // Getter for the format of the file
        CNIFTIFile::NIFTIFormat format(void) const;
    // Getter for the type of the file
        CNIFTIMainHeader::Type fileType(void) const;
    // Setter for the type of the file
        bool setFileType(CNIFTIMainHeader::Type fileType);
    
//=============================================================================================

    // NIfTI does not have subheaders, so these methods are not applicable --> NIfTI only has a main header


//=============================================================================================

    // NIfTI does not have frames, planes, or gates, so these methods are not applicable

 
//=============================================================================================

    // Member Function to read out and write specific METADATA from the NIfTI files --> I/O

    // readHeader() reads the main header of the NIfTI file and returns a pointer to a CNIFTIMainHeader object
        bool readMainHeader(CNIFTIMainHeader*& MainHeader);

    // voxel data reading methods, which read the voxel data from the NIfTI file and return a pointer to a QByteArray 
    // or char array containing the voxel data
        bool readMatrix(QByteArray*& matrixData);
        bool readMatrix(char*& matrixData, unsigned int& len);

    // methods to write out headers/data to the file.
        bool writeMainHeader(CNIFTIMainHeader& MainHeader);

        bool writeMatrix(const QByteArray& matrixData);
        bool writeMatrix(const char* matrixData, unsigned int size);

//=============================================================================================

    // methods to generate some objects just for convienence
        CNIFTIMainHeader* createEmptyHeader(void);

        // internal methods to sync specific data with our headers
        void mainHeaderWritten(const CNIFTIMainHeader& MainHeader);
        bool reWriteMainHeader(void);
 
//=============================================================================================

    private:
        CNIFTIFilePrivate*  m_pData; // m_pData is a pointer to the private class data of the CNIFTIFile class (pointer to implementation) 
                                     // used to hide the implementation details of the class from the user 

};

#endif // CNIFTIFILE_H