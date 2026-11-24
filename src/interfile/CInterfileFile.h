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

//! @file CInterfileFile.h
//! @brief contains the definition of the class CInterfileFile

#ifndef CINTERFILEFILE_H
#define CINTERFILEFILE_H

#include <QByteArray>
#include <QString>
#include <QStringList>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOData>
#include <CMedIOVolume>
#else
#include <CMedIOData.h>
#include <CMedIOVolume.h>
#endif

class CInterfileMainHeader;
class CInterfileFrameHeader;
class CInterfileFilePrivate;

//! @class CInterfileFile
//! @brief images stored as Interfile
//!
//! An Interfile image is described by an ASCII header (see CInterfileKeys) and
//! stored in a data file or in the header file behind the header. Supported are
//! the variants described in docs/interfile.md:
//!
//! - Interfile 3.3 (e.g. PMOD): static, tomographic and dynamic studies (frame
//!   groups), single files and header/data pairs,
//! - the PET proposal and STIR (number of time frames, vectored keys),
//! - Siemens reconstructions: a main header (.mhdr) naming the image headers
//!   (.v.hdr) and data files (.v) of its data sets ("%data set[n]"), or an image
//!   header on its own.
//!
//! The file name given to the constructor can be a header, a Siemens main header
//! or a data file: for a data file, a header next to it that names it is used
//! (<data file>.hdr, <name>.hdr, <name>.hv, <name>.h33). The frames are numbered
//! 1..n: the frames of the data sets of a Siemens main header in the order of the
//! data sets, the frames of a header in the order of the header.
//!
//! Position of the voxels (CInterfileFrameHeader::affine()):
//! - Siemens: from "%image info[n]" (x and y of the first voxel, z of the planes;
//!   plane k of the data file has the z of "%image info[planes - k]"), verified
//!   with the DICOM series of the same reconstruction (patient orientation HFS);
//!   without it from the bed position and the gantry offset.
//! - STIR ("first pixel offset (mm) [i]"): the position of the first voxel in the
//!   coordinates of STIR, whose directions in LPS depend on "patient orientation" and
//!   "patient rotation" (head_in/supine if not given), as STIR converts its images.
//! - Other variants: the axes of the voxels towards patient left, posterior and
//!   superior (as written by PMOD), centered at 0; "origin (mm)" of PMOD is not used
//!   (its meaning has not been verified).
//!
//! New files are written as STIR writes images (see docs/interfile.md): the header
//! <name>.hv (any name; its extension is replaced by ".v" for the data file) and the
//! data file <name>.v, little endian, signed 16 bit integers with a scale factor per
//! frame or float32, the frames one after the other ("image scaling factor[f]", "data
//! offset in bytes[f]", "image duration (sec)[f]", "image relative start time
//! (sec)[f]"), the position as "first pixel offset (mm)" with "patient orientation"
//! and "patient rotation". Create the object with CMedIODataFactory::createForWriting(),
//! open() it with QIODevice::WriteOnly, write the main header (number of frames,
//! attributes) and the frames 1..n; the header is written again after every frame. All
//! frames have the matrix, number format and position of the first one written.
class CInterfileFile : public CMedIOData
{
  public:
    //! @brief constructor
    //! @param fileName a header, a Siemens main header or a data file
    CInterfileFile(const QString& fileName = QString());
    virtual ~CInterfileFile();

    //! @brief true if the file is an Interfile header, Siemens main header or the
    //!        data file of a header next to it that describes image data
    static bool isOfType(const QString& fileName);
    //! @brief a new CInterfileFile (not opened) if isOfType(), otherwise NULL
    static CMedIOData* createFromFile(const QString& fileName);

    //! @brief open the file for reading (QIODevice::ReadOnly) or a new file for writing
    //!        (QIODevice::WriteOnly: the header and the data file are created)
    bool open(QIODevice::OpenModeFlag mode);
    void close();

    CMedIOData::Format dataFormat() const;

    //! @brief the header that was opened (the main header of a Siemens reconstruction)
    QString headerFileName() const;
    //! @brief all files of the image: the headers and the data files (a file that is not
    //!        open for reading: the file and dataFileNameOf() it)
    QStringList allFileNames() const;

    //! @brief the data file of a new file: the name of the header with ".v" instead of
    //!        its extension (STIR: <name>.hv and <name>.v); a name without extension or
    //!        ending with ".v" gets ".v" appended
    static QString dataFileNameOf(const QString& headerFileName);
    //! @brief the reason why the last open() failed (empty if it did not)
    QString openError() const;

    //! @brief number of frames (0 if the file is not open)
    int numFrames() const;

    //! @brief the main header (to be deleted by the caller)
    bool readMainHeader(CInterfileMainHeader*& mainHeader);
    //! @brief the description of a frame (1..numFrames(), -1: frame 1; to be deleted by the caller)
    bool readSubHeader(CInterfileFrameHeader*& subHeader, short frame);
    //! @brief the voxels of a frame in the order of the file and the data type of its
    //!        stored values, in the byte order of this machine (both to be deleted by the caller)
    bool readMatrix(QByteArray*& matrixData, CInterfileFrameHeader*& subHeader, short frame);

    //! @brief write the main header of a new file: the number of frames (numFrames()) and
    //!        the attributes stored in the header (see docs/interfile.md)
    bool writeMainHeader(const CInterfileMainHeader& mainHeader);

    //! @brief write a frame (1..number of frames of the main header, -1: frame 1) of a new
    //!        file: voxels in the order and data type of subHeader (signed 16 bit integers
    //!        or float32, intercept 0), in the byte order of this machine; the voxel axes
    //!        have to be the axes of STIR for one of the patient positions HFS, FFS, HFP or
    //!        FFP (CInterfileFrameHeader::affine(): diagonal with the signs (1, 1, -1),
    //!        (-1, 1, 1), (-1, -1, -1) or (1, -1, 1)), as given by readMatrix() of another
    //!        file in the representation of Interfile
    bool writeMatrix(const QByteArray& matrixData, const CInterfileFrameHeader& subHeader, short frame);

    //! @brief a file open for writing: the number format of the frames (fixed by the first
    //!        frame written, UnknownDataType before) and the orientation code of their voxel
    //!        axes, from the patient position of the main header written (DICOM Patient
    //!        Position HFS "LPI", FFS "RPS", HFP "RAI", FFP "LAS", otherwise HFS)
    CMedIOVolume::DataType writtenDataType() const;
    QString writtenOrientation() const;

    // format independent access of CMedIOData (in the representation of other formats)
    using CMedIOData::readMainHeader;
    using CMedIOData::readMatrix;
    using CMedIOData::writeMainHeader;
    using CMedIOData::writeMatrix;

  private:
    //! @brief write the header of a new file (again)
    bool writeHeader();

    CInterfileFilePrivate* m_pData;
};

#endif // CINTERFILEFILE_H
