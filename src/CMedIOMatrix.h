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

//! @file CMedIOMatrix.h
//! @brief private helpers (not installed) converting matrix data between the file formats

#ifndef CMEDIOMATRIX_H
#define CMEDIOMATRIX_H

// Conversion of image matrices between the file formats (see
// CMedIOData::readMatrix()/writeMatrix()).
//
// Every frame is converted via its ECAT7 representation (CMedIOFrame): the
// ECAT7 main header and image sub header, obtained with the existing header
// conversions (convertFrom()), and a CMedIOVolume holding the voxel data in
// the ECAT7 voxel order ("LPI": x towards patient left, y towards posterior,
// planes from superior to inferior) with the data type of the source file
// and the factors converting the stored values into physical values
// (calibration factor included). The adapter of every format converts
// between this representation and its own matrices:
//
//   readFrame()        file frame              -> CMedIOFrame
//   frameFromMatrix()  matrix + header(s)      -> CMedIOFrame
//   matrixFromFrame()  CMedIOFrame             -> matrix + header
//   mainHeaderFromFrame() CMedIOFrame          -> main header
//   writeFrame()       CMedIOFrame             -> frame of a file (main header written before)
//
// Data types (fitDataType()): the stored values are kept if the target
// format can represent them exactly (integers are widened if needed),
// otherwise the physical values are stored as float32.

#include <QByteArray>
#include <QString>

#include "CMedIOData.h"
#include "CMedIOVolume.h"
#include "CECATMainHeader.h"
#include "CECAT7MainHeader.h"
#include "CECAT7SubHeaderImage.h"

class CMedIOHeader;
class CECATFile;
class CECATSubHeader;

//! ECAT7 representation of one frame
struct CMedIOFrame
{
  CMedIOFrame() : mainHeader(NULL, CECATMainHeader::ECAT7_Volume16) { }

  CECAT7MainHeader mainHeader;     //!< calibration factor 1 (included in the volume scale factors)
  CECAT7SubHeaderImage subHeader;  //!< geometry, timing and metadata; data type and scale factor are set for the targets
  CMedIOVolume volume;             //!< ECAT7 voxel order, geometry and timing of the headers
};

namespace MedIOMatrix {

//---------------------------------------------------------------------------------------------
// common helpers

//! bit of a CMedIOVolume data type in a mask of allowed data types
inline unsigned typeBit(CMedIOVolume::DataType type) { return 1u << static_cast<int>(type); }

//! convert the data type of v into one of the allowed types (see above);
//! planeScales/intercept: the target supports scale factors per plane/an intercept
bool fitDataType(CMedIOVolume& v, unsigned allowed, bool planeScales, bool intercept);

//! change the type of the stored values (the values have to fit into the type)
bool convertStoredValues(CMedIOVolume& v, CMedIOVolume::DataType type);

//! calibration factor of an ECAT7 main header (1 if not set)
double ecatCalibration(const CECATMainHeader* mainHeader);

//! scale factor of an ECAT7 sub header (1 if not set)
double ecatScale(double scaleFactor);

//! LPS affine (mm) of ECAT7 voxel (i, j, k):
//!   x = x_offset + (i - (nx-1)/2) * px, y = y_offset + (j - (ny-1)/2) * py,
//!   z = init_bed_position + z_offset - k * pz
void ecatAffine(const CECAT7MainHeader& mainHeader, const CECAT7SubHeaderImage& subHeader, double m[3][4]);

//! set the geometry and timing of the frame's volume from its headers
void setVolumeGeometry(CMedIOFrame& frame);

//! ECAT7 representation of a volume without headers (CMedIOData::writeVolume())
bool frameFromVolume(const CMedIOVolume& volume, CMedIOFrame& frame);

//! CMedIOVolume data type of an ECAT7 data type and vice versa
CMedIOVolume::DataType volumeType(CECATSubHeader::Data_Type type);
CECATSubHeader::Data_Type ecatType(CMedIOVolume::DataType type);

//! file format whose matrices are described by a header
CMedIOData::Format headerDataFormat(const CMedIOHeader& header);

//---------------------------------------------------------------------------------------------
// dispatch to the format adapters

//! frame of a file in its own voxel order (readVolume()) and as CMedIOFrame
bool readVolume(CMedIOData* file, short frame, CMedIOVolume& volume);
bool readFrame(CMedIOData* file, short frame, CMedIOFrame& f);

bool frameFromMatrix(const QByteArray& data, const CMedIOHeader& header, const CMedIOHeader* mainHeader, CMedIOFrame& f);
bool matrixFromFrame(const CMedIOFrame& f, CMedIOData::Format format, QByteArray& data, CMedIOHeader*& header);
bool mainHeaderFromFrame(const CMedIOFrame& f, CMedIOData::Format format, CMedIOData* source, CMedIOHeader*& header);
bool writeFrame(CMedIOData* file, short frame, const CMedIOFrame& f);

//! matrices and main header in the format of the file itself
bool readNativeMatrix(CMedIOData* file, short frame, QByteArray& data, CMedIOHeader*& header);
bool readNativeMainHeader(CMedIOData* file, CMedIOHeader*& header);
bool writeNativeMatrix(CMedIOData* file, short frame, const QByteArray& data, const CMedIOHeader& header);

//---------------------------------------------------------------------------------------------
// ECAT7 adapter (src/ecat/CECATMatrix.cpp)

bool readEcatFrame(CECATFile* file, short frame, CMedIOFrame& f);
bool ecatFrameFromMatrix(const QByteArray& data, const CECATSubHeader& subHeader, const CMedIOHeader* mainHeader, CMedIOFrame& f);
bool ecatMatrixFromFrame(const CMedIOFrame& f, QByteArray& data, CMedIOHeader*& header);
bool ecatMainHeaderFromFrame(const CMedIOFrame& f, CMedIOHeader*& header);
bool writeEcatFrame(CECATFile* file, short frame, const CMedIOFrame& f);

} // namespace MedIOMatrix

#endif // CMEDIOMATRIX_H
