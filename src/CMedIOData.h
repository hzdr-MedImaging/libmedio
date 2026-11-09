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
#include <QList>
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
    //!        required), microPET (required); unused for NIfTI and for ECAT7
    //!        matrices written into an ECAT7 file (written unchanged). The
    //!        method fails without writing if it is not a main header of the
    //!        format of the matrix, also where it is not used.
    //!
    //! The main header of this file has to be written before. The values are
    //! converted into the representation of this file (NIfTI: datatype,
    //! scaling and orientation of its main header; the method fails without
    //! writing if a value does not fit into the datatype). Philips: all slices
    //! of a frame with one scale factor; a frame written before is
    //! overwritten if the new data has the same slices.
    //!
    //! NIfTI: the start time and duration of every volume written are stored
    //! as JSON header extension (FrameTimesStart, FrameDuration, as written
    //! by CNIFTIConverter); other JSON metadata of the file is kept. The
    //! extension is written between the header and vox_offset (single files)
    //! or at the end of the header file (header/image pairs). If the voxel
    //! data of a single file starts directly behind the header and no voxel
    //! data has been written yet, vox_offset is moved to make room for the
    //! timing of all volumes; the main header must not be written again
    //! with the former vox_offset afterwards. Without room, or if the file has
    //! other header extensions, only the voxel data is written (with a warning).
    bool writeMatrix(const QByteArray& matrixData, const CMedIOHeader& subHeader,
                     short frame=-1, const CMedIOHeader* mainHeader=NULL);
    bool writeMatrix(const char* matrixData, unsigned int size, const CMedIOHeader& subHeader,
                     short frame=-1, const CMedIOHeader* mainHeader=NULL);

    //! @brief write a frame given in the representation of the format @p format
    //!
    //! Same as writeMatrix() without a format, but fails without writing if
    //! @p subHeader does not describe a matrix of @p format. Counterpart of
    //! readMatrix(matrixData, subHeader, format, frame).
    bool writeMatrix(const QByteArray& matrixData, const CMedIOHeader& subHeader,
                     CMedIOData::Format format, short frame=-1, const CMedIOHeader* mainHeader=NULL);
    bool writeMatrix(const char* matrixData, unsigned int size, const CMedIOHeader& subHeader,
                     CMedIOData::Format format, short frame=-1, const CMedIOHeader* mainHeader=NULL);

    //! @brief numbers of the frames of the file in ascending order, as passed
    //!        to readMatrix() and readVolume() (empty if there are none or the
    //!        file is not open)
    //!
    //! ECAT7 and Philips files address their frames by the frame numbers
    //! stored in the file, which do not have to start with 1 and can have
    //! gaps (numFrames() of these classes is the highest frame number, not the
    //! number of frames). NIfTI volumes and microPET frames are numbered 1..n.
    //! The number of frames in the main headers returned by readMainHeader()
    //! for another format is the size of this list.
    QList<short> frameNumbers();

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
