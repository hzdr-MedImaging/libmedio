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

//! @file CHDF5FrameHeader.h
//! @brief contains the definition of the class CHDF5FrameHeader

#ifndef CHDF5FRAMEHEADER_H
#define CHDF5FRAMEHEADER_H

#include <QString>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOHeader>
#include <CMedIOVolume>
#else
#include <CMedIOHeader.h>
#include <CMedIOVolume.h>
#endif

class CHDF5FrameHeaderPrivate;

//! @class CHDF5FrameHeader
//! @brief the description of one volume of an HDF5 image file (see CHDF5File)
//!
//! A volume is the image of one frame, gate and bed position: a row of the
//! datasets of /image/volumes and the hyperslab data[volume()] of /image/data.
//! CHDF5File::readMatrix() gives its voxels in the order of the file (x fastest,
//! then y, then the planes) in the byte order of this machine; they are the
//! physical values in units().
//!
//! The format independent attributes (attributes()) are the frame attributes of
//! the volume: ActualFrameDuration and the values of the further datasets of
//! /image/volumes (DecayFactor, DeadTimeFactor, FrameReferenceTime, TriggerTime,
//! TablePosition), and the keys of the entry of the volume in "Frames" of the JSON
//! object of the file that have no dataset.
class CHDF5FrameHeader : public CMedIOHeader
{
  public:
    //! @brief constructor
    //! @param file the file the header belongs to (load() reads it from there)
    //! @param frame the number of the frame, of its first gate and bed position
    CHDF5FrameHeader(CMedIOData* file = NULL, short frame = -1);
    CHDF5FrameHeader(const CHDF5FrameHeader& src);
    virtual ~CHDF5FrameHeader();

    //! @brief copy of another HDF5 frame header (other headers: nothing is copied)
    CMedIOHeader& operator=(const CMedIOHeader& src);
    CHDF5FrameHeader& operator=(const CHDF5FrameHeader& src);

    // CMedIOHeader interface
    void clear();
    //! @brief read the header of frame() from the CHDF5File it belongs to
    bool load();
    //! @brief HDF5 files cannot be written yet: false
    bool save() const;
    CMedIOHeader::Format headerFormat() const;
    using CMedIOHeader::convertFrom;
    //! @brief copy of an HDF5 frame header (header1); false for other headers
    bool convertFrom(const CMedIOHeader* header1, const CMedIOHeader* header2 = NULL);
    CMedIOHeader* clone() const;

    //! @brief index of the volume in the file (0..n-1)
    int volume() const;
    void setVolume(int volume);

    //! @brief frame (1..n, as stored: the numbers can have gaps), gate (1..n) and bed
    //!        position (0..n-1) of the volume
    short frame() const;
    short gate() const;
    short bed() const;
    void setIndex(short frame, short gate, short bed);

    //! @brief matrix size: columns, rows, planes
    int dimension(int axis) const;
    void setDimensions(int nx, int ny, int nz);

    //! @brief data type of the values: CMedIOVolume::Float32 or Float64
    CMedIOVolume::DataType dataType() const;
    void setDataType(CMedIOVolume::DataType type);

    //! @brief units of the values as DICOM Units, e.g. "BQML" (empty: unknown)
    QString units() const;
    void setUnits(const QString& units);

    //! @brief position of the voxels: affine m (3 x 4) transforming voxel (i, j, k)
    //!        of the file into DICOM patient coordinates (LPS, mm)
    void affine(double m[3][4]) const;
    void setAffine(const double m[3][4]);

    //! @brief start of the acquisition of the volume relative to the start of the
    //!        acquisition of the image and its duration (s)
    double frameStart() const;
    double frameDuration() const;
    void setTiming(double start, double duration);

  private:
    CHDF5FrameHeaderPrivate* m_pData;
};

#endif // CHDF5FRAMEHEADER_H
