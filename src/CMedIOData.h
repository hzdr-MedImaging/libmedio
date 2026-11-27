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
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

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
      NIFTI,             //!< NIfTI format
      Interfile,         //!< Interfile (see CInterfileFile)
      HDF5               //!< HDF5 (see CHDF5File)
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
    //!        CConcordeFrameHeader (microPET files only), Interfile:
    //!        CInterfileFrameHeader (voxels in the order of the file; other formats: in the
    //!        axes of STIR for the patient position, see CInterfileFile)
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
    //! number of frames). NIfTI volumes, microPET and Interfile frames are numbered 1..n.
    //! The number of frames in the main headers returned by readMainHeader()
    //! for another format is the size of this list.
    QList<short> frameNumbers();

    //! @brief frame as format independent volume in the voxel order of the file
    bool readVolume(CMedIOVolume& volume, short frame=-1);

    //! @brief write a volume as frame of the file (main header written before)
    bool writeVolume(const CMedIOVolume& volume, short frame=-1);

    //! @brief main header for a new file, made from a volume instead of a source file
    //! @param volume a frame of the image to be written: dimensions, voxel size,
    //!        position, orientation and timing; for NIfTI also data type and scaling
    //! @param format ECAT, NIFTI, Philips (Philips: an image file, change the file
    //!        type with setFiltyp() for a Syntegra file) or Interfile
    //! @param numFrames number of frames the file will hold (NIfTI: volumes of dim[4])
    //! @param mainHeader the new header (to be deleted by the caller); it is not linked
    //!        to a file, call setMedIOData() before saving it
    //! @return false (and mainHeader NULL) for an empty volume, a format that cannot be
    //!         written or a number of frames outside 1..32767
    //!
    //! Write the header into the new file with the writeMainHeader() of its class, then
    //! the frames with writeVolume(). The header gets the defaults of the format for
    //! everything a volume does not describe (patient, isotope, ...); set them with
    //! updateAttributes() before writing it. A NIfTI file stores all frames with the
    //! data type and scaling of this volume: the other frames are converted into them
    //! and writeVolume() fails if their values do not fit, so pass a volume with
    //! physical values (CMedIOVolume::convertToPhysicalValues()) if the frames differ.
    //! Philips files store no position within the planes: the planes are centered.
    //! Interfile files store all frames with the number format of the first frame written
    //! (signed 16 bit integers with a scale factor per frame, or float32).
    static bool mainHeaderFromVolume(const CMedIOVolume& volume, CMedIOData::Format format,
                                     int numFrames, CMedIOHeader*& mainHeader);

    //---------------------------------------------------------------------------------------
    // Writing new files of any writable format (ECAT7, NIfTI, Philips, Interfile, HDF5): create the object
    // with CMedIODataFactory::createForWriting(), check fileNames() for existing files,
    // open() it for writing, write the main header with writeMainHeader() and the frames
    // with writeVolume(), and removeFiles() if writing fails.

    //! @brief write the main header of a new file in the format of the file
    //! @param mainHeader a main header of the format of this file, e.g. made with
    //!        readMainHeader(mainHeader, format) of a source file or mainHeaderFromVolume();
    //!        it is linked to this file while it is written. Philips: the file type passed
    //!        to the constructor of the file (CPhilipsFile::requestedFileType(), e.g.
    //!        Syntegra) is set in the header.
    //! @return false (nothing written) if the header is no main header of the format of
    //!         this file, or if it cannot be written
    //!
    //! Same as the writeMainHeader() of the format classes, which take the header class of
    //! their format.
    bool writeMainHeader(CMedIOHeader& mainHeader);

    //! @brief names of all files the data is stored in, as given to the constructor: the
    //!        file itself; for NIfTI header/image pairs the header and the image file (as
    //!        CNIFTIFile names them, also if an existing image file uses the other letter
    //!        case, see CNIFTIFile::imageFileName()); for microPET files also the header
    //!        file (".hdr" appended); for Interfile files the headers and data files
    //!        (CInterfileFile::allFileNames(); a new file: the header and its data file)
    //!
    //! A new file replaces all of them: check that none exists before it is created.
    QStringList fileNames() const;

    //! @brief remove all files of fileNames(), e.g. a new file that could not be written
    //!        completely; the file has to be closed
    //! @return true if none of the files exists afterwards
    bool removeFiles();

    //! @brief format independent header attributes of the file (see
    //!        CMedIOHeader::attributes()): the attributes of the main header and in "Frames"
    //!        the attributes of every frame, as held by the NIfTI representation of the main
    //!        header; the section "NIfTI" is only included for NIfTI files
    //! @return false if the file is not open or its main header cannot be read
    bool readAttributes(QJsonObject& attributes);

    //! @brief number of frames a file with this main header has to get: the volumes of a
    //!        NIfTI main header (dim[4], 1 for 3D data), the frames of an Interfile main
    //!        header; a NIfTI or Interfile file without all of them is incomplete. 0 for the
    //!        main headers of the other formats, whose files take any number of frames.
    static int requiredFrames(const CMedIOHeader& mainHeader);

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
