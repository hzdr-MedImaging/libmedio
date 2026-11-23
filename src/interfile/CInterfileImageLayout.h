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

//! @file CInterfileImageLayout.h
//! @brief private: the image data described by an Interfile header
//!
//! The Interfile variants describe their image data with different keys. This
//! class reads them into one description: the data file, the byte order, and for
//! every frame the number format, the matrix size, the voxel size, the position,
//! the timing, the scaling and the offset of its data in the data file.
//!
//! Supported are
//! - Interfile 3.3 (Todd-Pokropek et al. 1992): static and tomographic studies
//!   ("!matrix size [1]", "!number of slices", "image duration (sec)") and dynamic
//!   studies in frame groups ("!frame group number", "!number of images this frame
//!   group", "pause between frame groups (sec)"), with the keys written by PMOD
//!   ("data rescale slope", "data rescale offset", "units", "origin (mm) [i]"),
//! - the PET keys of the proposal for Interfile PET (Thielemans et al. 2001) as
//!   written by STIR ("number of time frames", "image duration (sec)[f]", "image
//!   relative start time (sec)[f]", "image scaling factor[f]", "data offset in
//!   bytes[f]", "first pixel offset (mm) [i]", "quantification units"),
//! - the spellings of Siemens ("image data byte order", "scale factor (mm/pixel)").
//!
//! The data is given either in a separate file ("name of data file") or in the
//! header file after the header ("data offset in bytes", "data starting block" in
//! blocks of 2048 bytes). The byte order is big endian if not given (Interfile 3.3).

#ifndef CINTERFILEIMAGELAYOUT_H
#define CINTERFILEIMAGELAYOUT_H

#include <QList>
#include <QString>
#include <QVector>

class CInterfileKeys;

//! @brief one frame (volume) of the image data
struct CInterfileFrame
{
  //! @brief type of the stored values
  enum NumberFormat { SignedInteger, UnsignedInteger, Float };

  NumberFormat numberFormat;  //!< type of the stored values
  int bytesPerPixel;          //!< size of a stored value: 1, 2, 4 or 8
  QVector<int> dims;          //!< matrix size: columns, rows, planes
  QVector<double> voxelSize;  //!< size of a voxel in mm (the same order as dims)
  bool hasOrigin;             //!< whether origin is given in the header
  QVector<double> origin;     //!< "first pixel offset (mm)"/"origin (mm)" (mm)
  double startTime;           //!< start of the frame relative to the study (s)
  double duration;            //!< duration of the frame (s), 0 if not given
  double slope;               //!< physical value = stored value * slope + intercept
  double intercept;           //!< see slope
  QString units;              //!< units of the physical values as given, e.g. "Bq/ml"
  qint64 offset;              //!< offset of the data of the frame in the data file (bytes)

  CInterfileFrame();

  //! @brief number of voxels of the frame
  qint64 voxelCount() const;

  //! @brief size of the data of the frame in bytes
  qint64 byteSize() const;
};

//! @brief the image data described by an Interfile header
class CInterfileImageLayout
{
  public:
    CInterfileImageLayout();

    //! @brief read the description from the keys of a header
    //! @param keys the keys of the header
    //! @param headerFileName the file holding the header (relative data file names
    //!        are relative to its directory; the data is in this file if no data
    //!        file is named)
    //! @param error if given, receives the reason of a failure
    //! @return false if the keys do not describe image data this class can read
    //!         (unsupported number format, missing matrix size, ...)
    bool read(const CInterfileKeys& keys, const QString& headerFileName, QString* error = 0);

    QString versionOfKeys;          //!< "version of keys", e.g. "3.3"
    QString typeOfData;             //!< "type of data" in lower case, e.g. "dynamic", "pet"
    QString dataFileName;           //!< the file holding the data
    bool dataInHeaderFile;          //!< the data follows the header in the header file
    bool bigEndian;                 //!< byte order of the stored values
    QList<CInterfileFrame> frames;  //!< the frames in the order of the file

    //! @brief size the data file needs at least (end of the data of the last frame)
    qint64 requiredDataSize() const;
};

#endif // CINTERFILEIMAGELAYOUT_H
