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

//! @file CInterfileMatrix.cpp
//! @brief Interfile adapter of the matrix conversion (see CMedIOMatrix.h)
//!
//! Interfile files can be read only. A frame is read as CMedIOVolume in the voxel
//! order of the file with the position of its voxels (CInterfileFrameHeader::affine())
//! and converted into the ECAT7 representation with frameFromVolume().

#include "CMedIOMatrix.h"
#include "CInterfileFile.h"
#include "CInterfileFrameHeader.h"

#include <QScopedPointer>

#include <rtdebug.h>

namespace MedIOMatrix {

namespace {

bool volumeOfMatrix(const QByteArray& data, const CInterfileFrameHeader& header, CMedIOVolume& v)
{
  if(header.dataType() == CMedIOVolume::UnknownDataType ||
     v.setData(header.dimension(0), header.dimension(1), header.dimension(2), header.dataType(), data) == false)
  {
    E("invalid Interfile matrix (%d bytes)", data.size());
    return false;
  }

  double m[3][4];
  header.affine(m);
  v.setAffine(m);
  v.setScaleFactor(header.slope());
  v.setIntercept(header.intercept());
  v.setFrameStart(header.frameStart());
  v.setFrameDuration(header.frameDuration());

  return true;
}

} // namespace

QList<short> interfileFrameNumbers(CInterfileFile* file)
{
  QList<short> numbers;
  for(int frame = 1; frame <= file->numFrames(); frame++)
    numbers.append(static_cast<short>(frame));
  return numbers;
}

bool readInterfileVolume(CInterfileFile* file, short frame, CMedIOVolume& v)
{
  ENTER();

  QByteArray* data = NULL;
  CInterfileFrameHeader* header = NULL;
  bool result = file->readMatrix(data, header, frame) && data != NULL && header != NULL &&
                volumeOfMatrix(*data, *header, v);
  delete data;
  delete header;

  RETURN(result);
  return result;
}

bool readInterfileFrame(CInterfileFile* file, short frame, CMedIOFrame& f)
{
  ENTER();

  CMedIOVolume v;
  bool result = readInterfileVolume(file, frame, v) && frameFromVolume(v, f);

  RETURN(result);
  return result;
}

bool interfileFrameFromMatrix(const QByteArray& data, const CInterfileFrameHeader& header, CMedIOFrame& f)
{
  ENTER();

  CMedIOVolume v;
  bool result = volumeOfMatrix(data, header, v) && frameFromVolume(v, f);

  RETURN(result);
  return result;
}

} // namespace MedIOMatrix
