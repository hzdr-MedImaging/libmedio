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
class CECATMainHeader;
class CECATSubHeader;

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
    // header/image file pair (.hdr/.img, .hdr.gz/.img.gz) instead of a single
    // file (.nii, .nii.gz); determined by the file name passed to the constructor
        bool isPair(void) const;
    // file holding the main header and its extensions (.hdr/.nii) and file holding
    // the voxel data (.img/.nii); both are the same file for single files
        QString headerFileName(void) const;
        QString imageFileName(void) const;
    
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

    // Frame-wise voxel data access in the voxel order of the file
    // (NativeOrientation) or in the ECAT7 voxel order (ECATOrientation), so
    // that applications working with ECAT7 data can use NIfTI files without
    // handling their orientation themselves.
    //
    // - frame is the 3D volume (1 = first volume, -1 = default = first volume);
    //   one call reads or writes one volume of nx * ny * nz voxels.
    // - ECATOrientation: voxel (i, j, k) is the voxel of an ECAT7 image as
    //   written by CNIFTIConverter::niftiToEcat(): i towards patient left, j
    //   towards posterior, plane k from superior to inferior. The NIfTI voxels
    //   are reordered by means of the sform, the qform or the default
    //   orientation (see CNIFTIConverter.h), whatever orientation the file
    //   has. Oblique data is assigned to the closest axes. 3D/4D data only.
    // - The voxel data is in the byte order of this machine.
    // - writeMatrix(): the main header has to be written first, as it defines
    //   the layout and orientation of the voxel data. Volumes can be written in
    //   any order.

    // voxel data of one volume in the NIfTI datatype of the file
        bool readMatrix(QByteArray*& matrixData, CMedIOData::Orientation orientation, short frame=-1);
        bool readMatrix(char*& matrixData, unsigned int& len, CMedIOData::Orientation orientation, short frame=-1);

        bool writeMatrix(const QByteArray& matrixData, CMedIOData::Orientation orientation, short frame=-1);
        bool writeMatrix(const char* matrixData, unsigned int size, CMedIOData::Orientation orientation, short frame=-1);

    // ECAT7 representation of one volume: voxel data in ECAT7 voxel order and
    // a CECAT7SubHeaderImage (to be deleted by the caller) describing it like
    // the image sub header of an ECAT7 file: dimensions, voxel sizes and x/y
    // offset (cm), data type, scale factor and frame timing (from the
    // metadata of CNIFTIConverter or the header, see CNIFTIConverter.h).
    // Physical value = stored value x scale factor (calibration factor 1).
    // The ECAT7 main header (bed position, number of frames, ...) is
    // obtained with CECAT7MainHeader::convertFrom(niftiMainHeader).
    //
    // Data types: uint8, int16, int32 and float32 are returned as ByteData,
    // SunShort, SunLong and IEEEFloat with scl_slope as scale factor, int8
    // as SunShort, uint16 (and uint32 if all values fit) as SunLong. float64,
    // larger uint32 values and data with scl_inter != 0 are returned as
    // IEEEFloat with the physical values (scale factor 1).
        bool readMatrix(QByteArray*& matrixData, CECATSubHeader*& subHeader, short frame=-1);
        bool readMatrix(char*& matrixData, unsigned int& len, CECATSubHeader*& subHeader, short frame=-1);

    // write one volume given in the ECAT7 representation: voxel data in ECAT7
    // voxel order with the data type, scale factor and dimensions of the
    // ECAT7 image sub header (the size of the data may include the padding
    // to ECAT7 blocks of 512 bytes). Physical value = stored value x scale factor x
    // calibration factor of mainHeader (1 without mainHeader), as for
    // CNIFTIMainHeader::convertFrom(ecatMainHeader, ecatSubHeader), which
    // can be used to create the NIfTI main header. The values are converted into the datatype and scaling (scl_slope,
    // scl_inter) of the main header written before; they are copied
    // unchanged if datatype and scaling correspond to the sub header. Values
    // are rounded for integer datatypes; the method fails without writing
    // anything if a value does not fit into the datatype (e.g. frames with
    // different scale factors and an int16 header: use float32 then).
        bool writeMatrix(const QByteArray& matrixData, const CECATSubHeader& subHeader, short frame=-1,
                         const CECATMainHeader* mainHeader=NULL);
        bool writeMatrix(const char* matrixData, unsigned int size, const CECATSubHeader& subHeader, short frame=-1,
                         const CECATMainHeader* mainHeader=NULL);

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