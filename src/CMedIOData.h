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

//! @file CMedIOData.h
//! @brief contains definition of the class CMedIOData

#ifndef CMEDIODATA_H
#define CMEDIODATA_H

#include <QFile>
#include <QString>

// forward declarations
class CMedIOHeader;
class CMedIOVolume;

//! @class CMedIOData
//! @brief an abstract class which provides an interface for medical data
//! @bug no bugs reported
//! @warning still in development
//!
//! This class is an interface for easier handling of different medical data formats.
//! All data formats which are included in Medical IO Library should use the class
//! as a baseclass.
class CMedIOData : public QFile {
  public:
    //! @enum enumeration of supported medical data formats
    enum Format {  
      Unknown = 0,      //!< unknown data which is not supported
      ConcordeMicropet,  //!< data produced by a concorde micropet scanner  
      ECAT,              //!< ECAT6/7 format
      Philips,           //!< data produced by a philips PET/MR scanner
      PhilipsBin,
      NIFTI              //!< NIfTI format
    };

    //! @enum enumeration carrying special IO error codes which
    //!       can be queried with the errorStatus() method.
    enum errStatus { MERR_Ok = 0,         //!< No Error
                     MERR_Unspecified = 1 //!< Some unspecified error occurred
                   };

    //! @brief constructor
    //! @param file: complete path to file holding medical data
    CMedIOData(const QString& filename);

    //! @brief destructor
    virtual ~CMedIOData();
      
    //! @brief opens the file for a certain operation
    //! @return true if loading is succesful otherwise false
    virtual bool open(QIODevice::OpenModeFlag mode) = 0;
    
    //! @brief saves the data  
    //! @return true if saving is succesful otherwise false
    virtual void close(void) = 0;
    
    //! @brief returns the data format of the mediodata
    virtual CMedIOData::Format dataFormat() const = 0;

    //! @brief run time typeinformation  
    //! @return specific class type at runtime
    int userRtti() const;
 
    //! @brief for querying error status information
    //! @return integer value for the specific error reason
    int errorStatus(void) const;

    //---------------------------------------------------------------------------------------
    // Format independent access to the image matrices: the matrices of every
    // supported image format can be read in the representation of another
    // format, and matrices given in the representation of any format can be
    // written into a file of another format. The conversion uses the ECAT7
    // representation of a frame as common format (see CMedIOMatrix.h):
    //
    // - Voxel order and geometry: the voxels are reordered into the voxel
    //   order of the target format, every voxel keeps its position (ECAT7,
    //   Philips, microPET: x towards patient left, y towards posterior,
    //   planes from superior to inferior; NIfTI: the same with reversed
    //   planes, or the orientation of the NIfTI file written to).
    // - Data types: the stored values are kept if the target format can
    //   represent them exactly (integers are widened if needed), otherwise
    //   the physical values are stored as float32. Physical value = stored
    //   value x scale factor (x calibration factor).
    // - One call reads or writes one frame (3D volume); frame -1 selects the
    //   default frame of the format (the first one).
    // - Only image data is supported (no sinograms, normalizations, ...).
    //   microPET files can be read only.
    //
    // If the requested format is the format of the file itself, the
    // matrices and headers of the file are returned unchanged (except for
    // Philips files, whose slices are combined into one frame). Without a
    // format, readMainHeader() and readMatrix() use the format of the file.

    //! @brief main header of the file in the representation of another format
    //! @param mainHeader the new header (to be deleted by the caller); it is not
    //!        linked to a file, call setMedIOData() before saving it
    bool readMainHeader(CMedIOHeader*& mainHeader, CMedIOData::Format format);

    //! @brief main header of the file in the format of the file itself
    //!        (same as readMainHeader(mainHeader, dataFormat()))
    bool readMainHeader(CMedIOHeader*& mainHeader);

    //! @brief frame of the file in the representation of another format
    //! @param matrixData voxel data in the voxel order and data type of the
    //!        format, in the byte order of this machine (to be deleted by the caller)
    //! @param subHeader header describing the matrix (to be deleted by the caller):
    //!        ECAT: CECAT7SubHeaderImage, NIfTI: CNIFTIMainHeader of a file
    //!        holding the frames, Philips: CPhilipsSubHeaderImage of the first
    //!        slice (all slices of the frame in matrixData), microPET:
    //!        CConcordeFrameHeader (microPET files only)
    bool readMatrix(QByteArray*& matrixData, CMedIOHeader*& subHeader,
                    CMedIOData::Format format, short frame=-1);
    bool readMatrix(char*& matrixData, unsigned int& len, CMedIOHeader*& subHeader,
                    CMedIOData::Format format, short frame=-1);

    //! @brief frame of the file in the format of the file itself (same as
    //!        readMatrix(matrixData, subHeader, dataFormat(), frame))
    bool readMatrix(QByteArray*& matrixData, CMedIOHeader*& subHeader, short frame=-1);
    bool readMatrix(char*& matrixData, unsigned int& len, CMedIOHeader*& subHeader, short frame=-1);

    //! @brief write a frame given in the representation of any format
    //! @param matrixData voxel data as returned by readMatrix() of that
    //!        format (ECAT7 matrices may be padded to blocks of 512 bytes)
    //! @param subHeader header describing the matrix (see readMatrix()); its
    //!        type determines the format of the matrix
    //! @param mainHeader main header of the matrix format: ECAT7 (bed
    //!        position, calibration factor; optional), Philips (scale factors;
    //!        required), microPET (required); unused for NIfTI
    //!
    //! The main header of this file has to be written before. The values are
    //! converted into the representation of this file (NIfTI: datatype,
    //! scaling and orientation of its main header; the method fails without
    //! writing if a value does not fit into the datatype). Philips: all slices
    //! of a frame with one scale factor; a frame written before is
    //! overwritten if the new data has the same slices.
    bool writeMatrix(const QByteArray& matrixData, const CMedIOHeader& subHeader,
                     short frame=-1, const CMedIOHeader* mainHeader=NULL);
    bool writeMatrix(const char* matrixData, unsigned int size, const CMedIOHeader& subHeader,
                     short frame=-1, const CMedIOHeader* mainHeader=NULL);

    //! @brief frame as format independent volume in the voxel order of the file
    bool readVolume(CMedIOVolume& volume, short frame=-1);

    //! @brief write a volume as frame of the file (main header written before)
    bool writeVolume(const CMedIOVolume& volume, short frame=-1);

  protected :
    int            m_iErrStatus;  //!< error status variable

  private:
    // The format classes open their files with open(QIODevice::OpenModeFlag),
    // which hides the open() overloads of QFile. They are declared here
    // (inaccessible) so that this is explicit and calls with other
    // arguments do not bypass the format specific open().
    using QFile::open;
};

#endif
