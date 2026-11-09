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

//! @file CECATMatrix.cpp
//! @brief ECAT7 adapter of the matrix conversion (see CMedIOMatrix.h)

#include "CMedIOMatrix.h"
#include "CMedIOHeaderMapping.h"
#include "CECATFile.h"
#include "CECATDirectory.h"
#include "CECATMainHeader.h"
#include "CECATSubHeader.h"
#include "CECAT7MainHeader.h"
#include "CECAT7SubHeaderImage.h"

#include <QScopedPointer>

#include <rtdebug.h>

namespace MedIOMatrix {

namespace {

const unsigned ecatTypes = typeBit(CMedIOVolume::UInt8) | typeBit(CMedIOVolume::Int16) |
                           typeBit(CMedIOVolume::Int32) | typeBit(CMedIOVolume::Float32);

// ECAT7 representation of an ECAT7 image matrix (data in machine byte order,
// possibly padded to blocks of 512 bytes) with the main header providing the
// bed position and the calibration factor
bool ecatFrame(const QByteArray& data, const CECAT7SubHeaderImage& subHeader, const CECAT7MainHeader& mainHeader,
               CMedIOFrame& f)
{
  const CMedIOVolume::DataType type = volumeType(subHeader.data_Type());
  const int nx = subHeader.x_Dimension(), ny = subHeader.y_Dimension(), nz = subHeader.z_Dimension();
  const qint64 size = static_cast<qint64>(nx) * ny * nz * CMedIOVolume::bytesPerVoxel(type);

  if(type == CMedIOVolume::UnknownDataType || nx < 1 || ny < 1 || nz < 1) {
    E("unsupported ECAT7 image matrix (data type %d, %d x %d x %d voxels)", subHeader.data_Type(), nx, ny, nz);
    return false;
  }

  // CECATFile::readMatrix() returns the matrix padded to blocks of 512 bytes
  if(data.size() < size || data.size() > (size + 511) / 512 * 512) {
    E("size of the ECAT7 matrix (%d bytes) does not match its sub header (%lld bytes)", data.size(), size);
    return false;
  }

  if(f.volume.setData(nx, ny, nz, type, data.left(size)) == false)
    return false;
  f.volume.setScaleFactor(ecatScale(subHeader.scale_Factor()) * ecatCalibration(&mainHeader));

  f.mainHeader = mainHeader;
  f.mainHeader.setMedIOData(NULL);
  f.mainHeader.setCalibration_Factor(1.0f);
  f.subHeader = subHeader;
  f.subHeader.setMedIOData(NULL);
  setVolumeGeometry(f);

  return true;
}

} // namespace

QList<short> ecatFrameNumbers(CECATFile* file)
{
  ENTER();
  QList<short> numbers;

  // readEcatFrame() reads the matrix of the frame with the default (smallest)
  // plane, gate, bed and data number of the file
  CECATDirectory* directory = file->directory();
  if(directory != NULL && directory->isEmpty() == false) {
    const short last = directory->maxFrame();
    for(short frame=directory->minFrame(); frame <= last; frame++) {
      if(directory->item(frame) != NULL)
        numbers.append(frame);
    }
  }

  LEAVE();
  return numbers;
}

bool readEcatFrame(CECATFile* file, short frame, CMedIOFrame& f)
{
  ENTER();

  CECATMainHeader* mh = NULL;
  if(file->readMainHeader(mh) == false || mh == NULL || mh->mainHeaderType() != CECATMainHeader::ECAT7MainHeader) {
    delete mh;
    E("only ECAT7 image files are supported");
    RETURN(false);
    return false;
  }
  QScopedPointer<CECATMainHeader> mainHeader(mh);

  QByteArray* data = NULL;
  CECATSubHeader* sh = NULL;
  bool result = file->readMatrix(data, sh, frame) && data != NULL && sh != NULL;
  QScopedPointer<QByteArray> dataGuard(data);
  QScopedPointer<CECATSubHeader> subHeader(sh);

  if(result && sh->subHeaderType() != CECATSubHeader::ECAT7_Image) {
    E("only ECAT7 image matrices can be converted");
    result = false;
  }

  if(result)
    result = ecatFrame(*data, *static_cast<CECAT7SubHeaderImage*>(sh), *static_cast<CECAT7MainHeader*>(mh), f);

  RETURN(result);
  return result;
}

bool ecatFrameFromMatrix(const QByteArray& data, const CECATSubHeader& subHeader, const CMedIOHeader* mainHeader, CMedIOFrame& f)
{
  ENTER();

  if(subHeader.subHeaderType() != CECATSubHeader::ECAT7_Image) {
    E("only ECAT7 image matrices can be converted");
    RETURN(false);
    return false;
  }

  // the main header provides the bed position and the calibration factor
  CECAT7MainHeader defaultMainHeader(NULL, CECATMainHeader::ECAT7_Volume16);
  const CECAT7MainHeader* mh = &defaultMainHeader;
  if(mainHeader != NULL) {
    const CECATMainHeader* ecatMainHeader = dynamic_cast<const CECATMainHeader*>(mainHeader);
    if(ecatMainHeader == NULL || ecatMainHeader->mainHeaderType() != CECATMainHeader::ECAT7MainHeader) {
      E("the main header of an ECAT7 matrix has to be an ECAT7 main header");
      RETURN(false);
      return false;
    }
    mh = static_cast<const CECAT7MainHeader*>(ecatMainHeader);
  }

  bool result = ecatFrame(data, static_cast<const CECAT7SubHeaderImage&>(subHeader), *mh, f);

  RETURN(result);
  return result;
}

bool ecatMatrixFromFrame(const CMedIOFrame& f, QByteArray& data, CMedIOHeader*& header)
{
  ENTER();

  CMedIOVolume v = f.volume;
  if(fitDataType(v, ecatTypes, false, false) == false) {
    RETURN(false);
    return false;
  }

  CECAT7SubHeaderImage* s = new CECAT7SubHeaderImage(f.subHeader);
  s->setMedIOData(NULL);
  s->setNum_Dimensions(3);
  s->setX_Dimension(v.dimension(0));
  s->setY_Dimension(v.dimension(1));
  s->setZ_Dimension(v.dimension(2));
  s->setData_Type(ecatType(v.dataType()));
  s->setScale_Factor(v.scaleFactor(0));

  // value range of the stored values (16 bit fields)
  if(v.dataType() == CMedIOVolume::UInt8 || v.dataType() == CMedIOVolume::Int16) {
    double lo = 0.0, hi = 0.0;
    for(qint64 i=0; i < v.voxelCount(); i++) {
      const double value = v.storedValue(i);
      if(i == 0 || value < lo)
        lo = value;
      if(i == 0 || value > hi)
        hi = value;
    }
    s->setImage_Min(static_cast<short>(lo));
    s->setImage_Max(static_cast<short>(hi));
  }

  // attributes of the source frame
  MedIOHeaderMapping::transfer(f.frameAttributes, *s);

  data = v.data();
  header = s;

  RETURN(true);
  return true;
}

bool ecatMainHeaderFromFrame(const CMedIOFrame& f, CMedIOHeader*& header)
{
  ENTER();

  CECAT7MainHeader* mh = new CECAT7MainHeader(f.mainHeader);
  mh->setMedIOData(NULL);
  mh->setNum_Planes(f.volume.dimension(2));
  header = mh;

  RETURN(true);
  return true;
}

bool writeEcatFrame(CECATFile* file, short frame, const CMedIOFrame& f)
{
  ENTER();

  frame = ecatWriteFrame(frame);

  // the main header of the file (written before) provides the bed position and the calibration factor
  CECATMainHeader* mh = NULL;
  if(file->readMainHeader(mh) == false || mh == NULL || mh->mainHeaderType() != CECATMainHeader::ECAT7MainHeader) {
    delete mh;
    E("the ECAT7 main header has to be written first");
    RETURN(false);
    return false;
  }
  QScopedPointer<CECATMainHeader> mainHeader(mh);
  const double calibration = ecatCalibration(mh);
  const double bedPosition = static_cast<CECAT7MainHeader*>(mh)->init_Bed_Position() * 10.0;

  QByteArray data;
  CMedIOHeader* header = NULL;
  if(ecatMatrixFromFrame(f, data, header) == false) {
    RETURN(false);
    return false;
  }
  QScopedPointer<CMedIOHeader> headerGuard(header);
  CECAT7SubHeaderImage* s = static_cast<CECAT7SubHeaderImage*>(header);

  // scale factor without the calibration factor of the file, position relative to its bed position
  s->setScale_Factor(s->scale_Factor() / calibration);
  s->setZ_Offset((f.volume.affine(2, 3) - bedPosition) / 10.0);

  bool result = file->writeMatrix(data, *s, frame);

  RETURN(result);
  return result;
}

} // namespace MedIOMatrix
