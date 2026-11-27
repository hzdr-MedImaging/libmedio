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
//! and bed position of the file. Frames are written with their physical values
//! (float32, or float64 for float64 data) in the ECAT7 voxel order.

#include "CMedIOMatrix.h"
#include "CHDF5File.h"
#include "CHDF5FrameHeader.h"
#include "CHDF5MainHeader.h"
#include "CMedIOHeaderDictionary.h"
#include "CMedIOHeaderMapping.h"

#include <QJsonArray>
#include <QJsonObject>

#include <rtdebug.h>

namespace MedIOMatrix {

namespace {

bool volumeOfMatrix(const QByteArray& data, const CHDF5FrameHeader& header, CMedIOVolume& v)
{
  if((header.dataType() != CMedIOVolume::Float32 && header.dataType() != CMedIOVolume::Float64) ||
     v.setData(header.dimension(0), header.dimension(1), header.dimension(2), header.dataType(), data) == false)
  {
    E("invalid HDF5 matrix (%d bytes)", data.size());
    return false;
  }

  double m[3][4];
  header.affine(m);
  v.setAffine(m);
  v.setFrameStart(header.frameStart());
  v.setFrameDuration(header.frameDuration());

  return true;
}

// the frame as HDF5 matrix: the physical values in the voxel order of the frame
bool hdf5Matrix(const CMedIOFrame& f, QByteArray& data, CHDF5FrameHeader& header)
{
  CMedIOVolume v = f.volume;
  if(v.isEmpty() || v.convertToPhysicalValues() == false)
    return false;

  double m[3][4];
  v.affine(m);
  header.setDimensions(v.dimension(0), v.dimension(1), v.dimension(2));
  header.setDataType(v.dataType());
  header.setUnits(f.mainAttributes.value("Units").toString());
  header.setAffine(m);
  header.setTiming(v.frameStart(), v.frameDuration());
  MedIOHeaderMapping::transfer(f.frameAttributes, header);
  data = v.data();

  return true;
}

} // namespace

QList<short> hdf5FrameNumbers(CHDF5File* file)
{
  return file->firstGateFrames();
}

bool readHdf5Volume(CHDF5File* file, short frame, CMedIOVolume& v)
{
  ENTER();

  QByteArray* data = NULL;
  CHDF5FrameHeader* header = NULL;
  bool result = file->readMatrix(data, header, frame) && data != NULL && header != NULL &&
                volumeOfMatrix(*data, *header, v);
  delete data;
  delete header;

  RETURN(result);
  return result;
}

bool hdf5FrameFromMatrix(const QByteArray& data, const CHDF5FrameHeader& header, CMedIOFrame& f)
{
  ENTER();

  CMedIOVolume v;
  bool result = volumeOfMatrix(data, header, v) && frameFromVolume(v, f);

  RETURN(result);
  return result;
}

bool hdf5MatrixFromFrame(const CMedIOFrame& f, QByteArray& data, CMedIOHeader*& header)
{
  ENTER();

  CHDF5FrameHeader* frameHeader = new CHDF5FrameHeader();
  bool result = hdf5Matrix(f, data, *frameHeader);
  if(result)
    header = frameHeader;
  else
    delete frameHeader;

  RETURN(result);
  return result;
}

bool hdf5MainHeaderFromFrame(const CMedIOFrame& f, CMedIOData* source, CMedIOHeader*& header)
{
  ENTER();

  CHDF5MainHeader* mainHeader = new CHDF5MainHeader();
  mainHeader->setNumFrames(f.mainHeader.num_Frames() > 0 ? f.mainHeader.num_Frames() : 1);
  mainHeader->setNumVolumes(mainHeader->numFrames());

  // the frame attributes of all frames of the source, for volumes written without attributes
  QJsonObject attributes;
  if(source != NULL && source->isOpen() && source->readAttributes(attributes))
  {
    const QString key = QString::fromLatin1(CMedIOHeaderDictionary::FramesKey);
    if(attributes.value(key).toArray().isEmpty() == false)
      mainHeader->updateAttributes(QJsonObject{{key, attributes.value(key)}});
  }
  header = mainHeader;

  RETURN(true);
  return true;
}

bool writeHdf5Frame(CHDF5File* file, short frame, const CMedIOFrame& f)
{
  ENTER();

  QByteArray data;
  CHDF5FrameHeader header;
  bool result = hdf5Matrix(f, data, header) && file->writeMatrix(data, header, frame == -1 ? 1 : frame);

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
