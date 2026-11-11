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

//! @file CConcordeMatrix.cpp
//! @brief microPET (Siemens Concorde) adapter of the matrix conversion (see CMedIOMatrix.h)
//!
//! microPET image files can be read only. Their frames are converted as
//! done by mp2ecat: the voxels are taken in the order of the file as ECAT7
//! voxel order (the subject orientation of the header is not used), the
//! headers are converted with the existing Concorde -> ECAT7 header
//! conversions, and the physical value is
//!   stored value x scale_factor / isotope_branching_fraction x calibration_factor
//! (the microPET scale factor does not include the branching fraction).

#include "CMedIOMatrix.h"
#include "CConcordeFile.h"
#include "CConcordeMainHeader.h"
#include "CConcordeFrameHeader.h"
#include "MedIOLimits.h"

#include <QScopedPointer>

#include <climits>
#include <cmath>

#include <rtdebug.h>

namespace MedIOMatrix {

namespace {

CMedIOVolume::DataType volumeTypeOf(CConcordeMainHeader::DataType type)
{
  switch(type) {
    case CConcordeMainHeader::Byte:       return CMedIOVolume::UInt8;
    case CConcordeMainHeader::IntelShort:
    case CConcordeMainHeader::SunShort:   return CMedIOVolume::Int16;
    case CConcordeMainHeader::IntelInt:
    case CConcordeMainHeader::SunInt:     return CMedIOVolume::Int32;
    case CConcordeMainHeader::IntelFloat:
    case CConcordeMainHeader::IEEEFloat:  return CMedIOVolume::Float32;
    default: break;
  }
  return CMedIOVolume::UnknownDataType;
}

double positive(double value)
{
  return (value > 0.0 && std::isfinite(value)) ? value : 1.0;
}

// ECAT7 representation of a microPET frame (data in machine byte order)
bool concordeFrame(const QByteArray& data, const CConcordeFrameHeader& frameHeader, const CConcordeMainHeader& mainHeader,
                   CMedIOFrame& f)
{
  if(mainHeader.fileType() != CConcordeMainHeader::Image && mainHeader.fileType() != CConcordeMainHeader::MuMap) {
    E("only microPET image files can be converted");
    return false;
  }

  const CMedIOVolume::DataType type = volumeTypeOf(mainHeader.dataType());
  const int nx = mainHeader.xDimension(), ny = mainHeader.yDimension(), nz = mainHeader.zDimension();
  const qint64 size = static_cast<qint64>(nx) * ny * nz * CMedIOVolume::bytesPerVoxel(type);

  if(type == CMedIOVolume::UnknownDataType || nx < 1 || ny < 1 || nz < 1 || data.size() < size) {
    E("unsupported microPET image (data type %d, %d x %d x %d voxels, %d bytes)",
      static_cast<int>(mainHeader.dataType()), nx, ny, nz, data.size());
    return false;
  }

  if(f.subHeader.convertFrom(&frameHeader, &mainHeader) == false ||
     f.mainHeader.convertFrom(&mainHeader, &frameHeader) == false) {
    E("unable to convert the microPET headers");
    return false;
  }
  f.mainHeader.setCalibration_Factor(1.0f); // included in the scale factor
  f.mainHeader.setMedIOData(NULL);
  f.subHeader.setMedIOData(NULL);

  if(f.volume.setData(nx, ny, nz, type, data.left(size)) == false)
    return false;

  f.volume.setScaleFactor(ecatScale(frameHeader.scaleFactor()) / positive(mainHeader.isotopeBranchingFraction()) *
                          positive(mainHeader.calibrationFactor()));
  setVolumeGeometry(f);

  return true;
}

} // namespace

//=============================================================================================

QList<short> concordeFrameNumbers(CConcordeFile* file)
{
  ENTER();
  QList<short> numbers;

  // frame k is the k-th frame section of the header (see readConcordeFrame())
  CConcordeMainHeader* mh = NULL;
  if(file->readMainHeader(mh) && mh != NULL) {
    for(int frame=1; frame <= mh->totalFrames() && frame <= SHRT_MAX; frame++)
      numbers.append(static_cast<short>(frame));
  }
  delete mh;

  LEAVE();
  return numbers;
}

bool readConcordeFrame(CConcordeFile* file, short frame, CMedIOFrame& f)
{
  ENTER();

  if(frame == -1)
    frame = 1;

  CConcordeMainHeader* mh = NULL;
  if(file->readMainHeader(mh) == false || mh == NULL) {
    delete mh;
    RETURN(false);
    return false;
  }
  QScopedPointer<CConcordeMainHeader> mainHeader(mh);

  if(frame < 1 || frame > mh->totalFrames()) {
    E("frame %d does not exist (%d frames)", frame, mh->totalFrames());
    RETURN(false);
    return false;
  }

  char* data = NULL;
  unsigned int length = 0;
  CConcordeFrameHeader* fh = NULL;
  bool result = file->readMatrix(data, length, fh, frame) && data != NULL && fh != NULL;
  QScopedPointer<CConcordeFrameHeader> frameHeader(fh);
  if(result && MedIOLimits::fitsByteArray(length) == false) {
    E("frame %d (%u bytes) is too large for a QByteArray", frame, length);
    result = false;
  }
  const QByteArray matrix(data, result ? static_cast<int>(length) : 0);
  delete[] data;

  if(result)
    result = concordeFrame(matrix, *fh, *mh, f);

  RETURN(result);
  return result;
}

bool concordeFrameFromMatrix(const QByteArray& data, const CConcordeFrameHeader& frameHeader, const CMedIOHeader* mainHeader,
                             CMedIOFrame& f)
{
  ENTER();

  const CConcordeMainHeader* mh = dynamic_cast<const CConcordeMainHeader*>(mainHeader);
  if(mh == NULL) {
    E("microPET matrices need the microPET main header");
    RETURN(false);
    return false;
  }

  bool result = concordeFrame(data, frameHeader, *mh, f);

  RETURN(result);
  return result;
}

} // namespace MedIOMatrix
