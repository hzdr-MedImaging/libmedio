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

//! @file CHDF5File.h
//! @brief contains the definition of the class CHDF5File

#ifndef CHDF5FILE_H
#define CHDF5FILE_H

#include <QByteArray>
#include <QList>
#include <QString>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOData>
#include <CMedIOVolume>
#else
#include <CMedIOData.h>
#include <CMedIOVolume.h>
#endif

class CHDF5MainHeader;
class CHDF5FrameHeader;
class CHDF5FilePrivate;

//! @class CHDF5File
//! @brief PET images stored in files of the Hierarchical Data Format version 5 (HDF5)
//!
//! HDF5 is a container without a meaning of its own. libmedio reads images of
//! the layout "MEDIO-IMAGE" described in docs/hdf5.md:
//!
//! - /image/data: all volumes of the image as one dataset [volume][z][y][x] with
//!   the physical values (float32 or float64),
//! - /image/affine (and /image/volumes/affine): the position of the voxels,
//! - /image/volumes: one row per volume with its frame, gate and bed position,
//!   the frame timing and further values of the frame,
//! - /header: the attributes of the study.
//!
//! A volume is the image of one frame, gate and bed position. The format
//! independent interface of CMedIOData (frameNumbers(), readVolume(),
//! readMatrix(), readAttributes()) gives the frames of the first gate and bed
//! position; numVolumes() and the methods taking the index of a volume give all
//! of them. The voxels are returned in the order of the file with their position
//! (CHDF5FrameHeader::affine()).
//!
//! A new file is written like the files of the other formats: open() it with
//! QIODevice::WriteOnly, write the main header (its attributes) and then the
//! volumes with writeMatrix(), or use the format independent methods of
//! CMedIOData (writeMainHeader(), writeVolume(), writeMatrix()). All volumes have
//! the dimensions and data type of the first one; a volume written for a frame,
//! gate and bed position that was written before replaces it. The file is
//! written under a temporary name in its directory and gets its name when it is
//! closed: until then, and if writing fails, there is no file of that name (an
//! existing one is left as it is). finish() closes the file and tells whether it
//! was written.
//!
//! HDF5 support is an optional feature of the build (CMake option
//! MEDIO_WITH_HDF5), as it needs the HDF5 library. The class and the format
//! CMedIOData::HDF5 exist in every build, so that the interface of libmedio does
//! not depend on it: hasSupport() tells whether this build has HDF5 support,
//! and HDF5 files are identified in every build. Without HDF5 support open()
//! fails. HDF5 files of other layouts are identified, but cannot be opened;
//! openError() gives the reason.
//!
//! The HDF5 library is used by one thread at a time (all objects of this class
//! share one lock).
class CHDF5File : public CMedIOData
{
  public:
    CHDF5File(const QString& fileName = QString());
    virtual ~CHDF5File();

    //! @brief true if the file is an HDF5 file: it has the HDF5 signature at its
    //!        start or, after a user block, at offset 512, 1024, 2048, ...
    //!        (the HDF5 library is not needed for this)
    static bool isOfType(const QString& fileName);
    //! @brief a new CHDF5File (not opened) if isOfType(), otherwise NULL
    static CMedIOData* createFromFile(const QString& fileName);

    //! @brief true if this build of libmedio has HDF5 support
    static bool hasSupport();
    //! @brief version of the HDF5 library used, e.g. "1.10.10" (empty without HDF5 support)
    static QString libraryVersion();

    //! @brief open an image file of the layout MEDIO-IMAGE for reading
    //!        (QIODevice::ReadOnly) or a new file for writing (QIODevice::WriteOnly)
    //! @return false if the file cannot be opened, openError() gives the reason
    bool open(QIODevice::OpenModeFlag mode);
    //! @brief close the file; a new file is completed as by finish()
    void close();
    //! @brief close the file. A new file gets its name now, if its main header and at
    //!        least one volume were written without an error.
    //! @return false if a new file could not be written (there is no new file then,
    //!         openError() gives the reason); true for a file opened for reading
    bool finish();

    CMedIOData::Format dataFormat() const;

    //! @brief the reason why the last open() or finish() failed (empty if it did not)
    QString openError() const;

    //! @brief number of volumes: all frames, gates and bed positions (0 if the file is not open)
    int numVolumes() const;
    //! @brief the frame numbers of the first gate and bed position, in the order of the
    //!        file (as CMedIOData::frameNumbers() gives them)
    QList<short> firstGateFrames() const;
    //! @brief index of the volume of a frame of the first gate and bed position
    //!        (frame -1: the first one), -1 if there is none
    int volumeOfFrame(short frame = -1) const;

    //! @brief the main header of the file (to be deleted by the caller)
    bool readMainHeader(CHDF5MainHeader*& mainHeader);

    //! @brief the header of a frame of the first gate and bed position
    //!        (to be deleted by the caller); frame -1: the first one
    bool readSubHeader(CHDF5FrameHeader*& frameHeader, short frame = -1);
    //! @brief the header of a volume given by its index (0..numVolumes()-1)
    bool readVolumeHeader(CHDF5FrameHeader*& frameHeader, int volume);

    //! @brief the voxels of a frame of the first gate and bed position and its header
    //!        (to be deleted by the caller): nx * ny * nz values of
    //!        CHDF5FrameHeader::dataType() in the byte order of this machine, x running fastest
    bool readMatrix(QByteArray*& data, CHDF5FrameHeader*& frameHeader, short frame = -1);
    //! @brief the voxels and the header of a volume given by its index
    bool readVolumeMatrix(QByteArray*& data, CHDF5FrameHeader*& frameHeader, int volume);

    //! @brief the attributes of a new file: has to be written once, before the volumes.
    //!        "Frames" of the attributes (the frame attributes of all frames, as
    //!        CMedIOData::readAttributes() gives them) are the attributes of the volumes
    //!        written with a frame header without attributes, in the order they are written.
    bool writeMainHeader(const CHDF5MainHeader& mainHeader);
    //! @brief write a volume of a new file
    //! @param data nx * ny * nz physical values of frameHeader.dataType() (Float32 or
    //!        Float64) in the byte order of this machine, x running fastest
    //! @param frameHeader dimensions, data type, affine, frame timing, gate, bed position
    //!        and frame attributes of the volume
    //! @param frame number of the frame; -1: frameHeader.frame(), or the number of
    //!        volumes written so far + 1 if that is not set
    bool writeMatrix(const QByteArray& data, const CHDF5FrameHeader& frameHeader, short frame = -1);

    // the format independent methods of CMedIOData with the same names
    using CMedIOData::readMainHeader;
    using CMedIOData::readMatrix;
    using CMedIOData::writeMainHeader;
    using CMedIOData::writeMatrix;

  private:
    CHDF5File(const CHDF5File&);
    CHDF5File& operator=(const CHDF5File&);

    CHDF5FilePrivate* m_pData;
};

#endif // CHDF5FILE_H
