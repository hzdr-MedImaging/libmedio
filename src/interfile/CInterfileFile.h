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
#else
#include <CMedIOData.h>
#endif

class CInterfileMainHeader;
class CInterfileFrameHeader;
class CInterfileFilePrivate;

//! @class CInterfileFile
//! @brief images stored as Interfile (read only)
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
//! - Other variants: the axes of the voxels towards patient left, posterior and
//!   superior (as written by PMOD), centered at 0; the origin keys of the variants
//!   are not used (their meaning has not been verified).
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

    //! @brief open the file; Interfile files can be read only (QIODevice::ReadOnly)
    bool open(QIODevice::OpenModeFlag mode);
    void close();

    CMedIOData::Format dataFormat() const;

    //! @brief the header that was opened (the main header of a Siemens reconstruction)
    QString headerFileName() const;
    //! @brief all files of the image: the headers and the data files
    QStringList allFileNames() const;
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

    // format independent access of CMedIOData (in the representation of other formats)
    using CMedIOData::readMainHeader;
    using CMedIOData::readMatrix;

  private:
    CInterfileFilePrivate* m_pData;
};

#endif // CINTERFILEFILE_H
