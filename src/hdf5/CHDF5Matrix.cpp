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

//! @file CHDF5Matrix.cpp
//! @brief HDF5 adapter of the format independent matrix access (see CMedIOMatrix.h)
//!
//! A frame is read as CMedIOVolume in the voxel order of the file with the position
//! of its voxels (CHDF5FrameHeader::affine()) and converted into the ECAT7
//! representation with frameFromVolume(). The frames are those of the first gate
//! and bed position of the file. HDF5 files cannot be written yet.

#include "CMedIOMatrix.h"
#include "CHDF5File.h"
#include "CHDF5FrameHeader.h"

#include <rtdebug.h>

namespace MedIOMatrix {

QList<short> hdf5FrameNumbers(CHDF5File* file)
{
  return file->firstGateFrames();
}

bool readHdf5Volume(CHDF5File* file, short frame, CMedIOVolume& v)
{
  ENTER();

  QByteArray* data = NULL;
  CHDF5FrameHeader* header = NULL;
  bool result = file->readMatrix(data, header, frame) && data != NULL && header != NULL;
  if(result)
  {
    result = v.setData(header->dimension(0), header->dimension(1), header->dimension(2), header->dataType(), *data);
    double m[3][4];
    header->affine(m);
    v.setAffine(m);
    v.setFrameStart(header->frameStart());
    v.setFrameDuration(header->frameDuration());
  }
  delete data;
  delete header;

  RETURN(result);
  return result;
}

bool readHdf5Frame(CHDF5File* file, short frame, CMedIOFrame& f)
{
  ENTER();

  CMedIOVolume v;
  bool result = readHdf5Volume(file, frame, v) && frameFromVolume(v, f);

  RETURN(result);
  return result;
}

} // namespace MedIOMatrix
