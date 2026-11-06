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

#ifndef CPHILIPSVOLUME_H
#define CPHILIPSVOLUME_H

// Private helper (not installed) assembling one frame of a Philips image or
// Syntegra file from its slices in the ECAT7 representation. It is shared by
// CPhilipsFile::readMatrix(..., CECATSubHeader*&, frame) and
// CNIFTIConverter, so that both use the same rules:
//
// - The slices of a frame are numbered from minSlice() to maxSlice() with
//   the increment (maxSlice() - minSlice()) / (numSlices(frame) - 1), as in
//   CPhilipsFile::readMatrix() with slice -1.
// - The voxel order within a slice already is the ECAT7 voxel order, the
//   slices are appended in the order of their slice numbers.
// - The stored values are kept in the corresponding ECAT7 data type: byte
//   as ByteData, signed short as SunShort, float as IEEEFloat and unsigned
//   short as SunLong (32 bit integer, as ECAT7 has no unsigned 16 bit type).
// - Every slice has its own scale factor
//   (CPhilipsSubHeaderImage::ecat_Scale_Factor()); slices without a valid
//   scale factor use 1.

#include <QByteArray>
#include <QScopedPointer>
#include <QString>
#include <QVector>

#include "CECATSubHeader.h"
#include "CPhilipsSubHeader.h"

// forward declarations
class CECAT7SubHeaderImage;
class CPhilipsFile;
class CPhilipsMainHeader;

class CPhilipsVolume
{
  public:
    CPhilipsVolume();

    // read all slices of the frame (Philips frame number, -1 = minFrame())
    // of a Philips image or Syntegra file opened for reading. On failure
    // the reason is returned in error.
    bool read(CPhilipsFile& file, const CPhilipsMainHeader* mainHeader, short frame, QString* error=NULL);

    // true if all slices have the same scale factor
    bool commonScale() const;

    // convert the stored values into float32 physical values (stored value x
    // scale factor of the slice), all scale factors become 1
    void convertToPhysical();

    // ECAT7 image sub header of this frame: converted from the sub header of
    // the first slice with CECAT7SubHeaderImage::convertFrom(), with the
    // number of slices, the ECAT7 data type and the scale factor of the
    // first slice
    bool convertSubHeader(CECAT7SubHeaderImage& subHeader, const CPhilipsMainHeader* mainHeader) const;

    // the frame in the ECAT7 representation
    QByteArray m_Data;                         // nx * ny * nz values in machine byte order
    CECATSubHeader::Data_Type m_DataType;      // ECAT7 data type of m_Data
    int m_PhilipsDataType;                     // CPhilipsSubHeader::Data_Type of the slices
    int m_Nx, m_Ny, m_Nz;                      // dimensions
    QVector<double> m_SliceScales;             // scale factor of every slice
    bool m_Unscaled;                           // some slices have no valid scale factor
    QScopedPointer<CPhilipsSubHeader> m_pFirstSubHeader; // sub header of the first slice

  private:
    Q_DISABLE_COPY(CPhilipsVolume)
};

#endif // CPHILIPSVOLUME_H
