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
//! A frame is read as CMedIOVolume in the voxel order of the file with the position of
//! its voxels (CInterfileFrameHeader::affine()) and converted into the ECAT7
//! representation with frameFromVolume(). Frames are written as STIR writes images:
//! the voxels reordered into the axes of STIR for the patient position (HFS: "LPI"),
//! signed 16 bit integers with a scale factor or float32 (the type of the first frame
//! of a file).

#include "CMedIOMatrix.h"
#include "CInterfileFile.h"
#include "CInterfileFrameHeader.h"
#include "CInterfileMainHeader.h"
#include "CMedIOHeaderMapping.h"

#include <QScopedPointer>

#include <cmath>

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

// the orientation code of the voxel axes of STIR for the patient position (DICOM)
QString stirOrientation(const QJsonObject& attributes)
{
  const QString position = attributes.value("PatientPosition").toString();
  if(position == "FFS")
    return "RPS";
  if(position == "HFP")
    return "RAI";
  if(position == "FFP")
    return "LAS";
  return "LPI";
}

// the frame as Interfile matrix: voxels in the given orientation, data type of the allowed
// types (signed 16 bit integers with a scale factor or float32)
bool interfileMatrix(const CMedIOFrame& f, const QString& orientation, unsigned allowed, QByteArray& data,
                     CInterfileFrameHeader& header)
{
  CMedIOVolume v = f.volume;
  bool oblique = false;
  if(v.reorient(orientation, &oblique) == false || fitDataType(v, allowed, false, false) == false)
    return false;
  if(oblique)
    W("oblique voxel axes are stored as the closest axes of STIR");

  // the closest axes of STIR: an affine without rotation, the position of the first voxel kept
  double m[3][4];
  v.affine(m);
  double axes[3][4];
  for(int i = 0; i < 3; i++)
  {
    for(int j = 0; j < 3; j++)
      axes[i][j] = 0.0;
    axes[i][3] = m[i][3];
  }
  for(int j = 0; j < 3; j++)
  {
    int row = 0;
    for(int i = 1; i < 3; i++)
    {
      if(std::fabs(m[i][j]) > std::fabs(m[row][j]))
        row = i;
    }
    axes[row][j] = (m[row][j] < 0.0 ? -1.0 : 1.0) * v.voxelSize(j);
  }

  header.setDimensions(v.dimension(0), v.dimension(1), v.dimension(2));
  header.setDataType(v.dataType());
  header.setScaling(v.scaleFactor(), 0.0);
  header.setUnits(f.mainAttributes.value("Units").toString());
  header.setAffine(axes);
  header.setTiming(v.frameStart(), v.frameDuration());
  MedIOHeaderMapping::transfer(f.frameAttributes, header);
  data = v.data();

  return true;
}

const unsigned interfileTypes = typeBit(CMedIOVolume::Int16) | typeBit(CMedIOVolume::Float32);

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

bool interfileMatrixFromFrame(const CMedIOFrame& f, QByteArray& data, CMedIOHeader*& header)
{
  ENTER();

  CInterfileFrameHeader* frameHeader = new CInterfileFrameHeader();
  bool result = interfileMatrix(f, stirOrientation(f.mainAttributes), interfileTypes, data, *frameHeader);
  if(result)
    header = frameHeader;
  else
    delete frameHeader;

  RETURN(result);
  return result;
}

bool interfileMainHeaderFromFrame(const CMedIOFrame& f, CMedIOHeader*& header)
{
  ENTER();

  CInterfileMainHeader* mainHeader = new CInterfileMainHeader();
  mainHeader->setNumFrames(f.mainHeader.num_Frames() > 0 ? f.mainHeader.num_Frames() : 1);
  header = mainHeader;

  RETURN(true);
  return true;
}

bool writeInterfileFrame(CInterfileFile* file, short frame, const CMedIOFrame& f)
{
  ENTER();

  // the frames of a file have the number format of the first one
  unsigned allowed = interfileTypes;
  if(file->writtenDataType() != CMedIOVolume::UnknownDataType)
    allowed = typeBit(file->writtenDataType());

  QByteArray data;
  CInterfileFrameHeader header;
  bool result = interfileMatrix(f, file->writtenOrientation(), allowed, data, header) &&
                file->writeMatrix(data, header, frame);

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
