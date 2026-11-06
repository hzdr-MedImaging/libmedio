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

//! @file CPhilipsMatrix.cpp
//! @brief Philips adapter of the matrix conversion (see CMedIOMatrix.h)
//!
//! Philips image and Syntegra files store every slice of a frame as matrix
//! with its own sub header and scale factor (see
//! CPhilipsSubHeaderImage::ecat_Scale_Factor()). Their voxel order is the one
//! of ECAT7 (slices as planes). A Philips matrix in the representation used
//! by CMedIOData::readMatrix()/writeMatrix() holds all slices of a frame with
//! one common scale factor and is described by the sub header of its first
//! slice; slices with different scale factors are converted into physical
//! values (float32). The headers are converted with the existing Philips <->
//! ECAT7 header conversions; the position along z is given by the bed
//! position, the image positions img_pos_x/y of the slices are not used.

#include "CMedIOMatrix.h"
#include "CPhilipsFile.h"
#include "CPhilipsDirectory.h"
#include "CPhilipsMainHeader.h"
#include "CPhilipsSubHeader.h"
#include "CPhilipsSubHeaderImage.h"

#include <QFileInfo>
#include <QScopedPointer>
#include <QVector>

#include <cmath>

#include <rtdebug.h>

namespace MedIOMatrix {

namespace {

const unsigned philipsTypes = typeBit(CMedIOVolume::UInt8) | typeBit(CMedIOVolume::Int16) |
                              typeBit(CMedIOVolume::UInt16) | typeBit(CMedIOVolume::Float32);

CMedIOVolume::DataType volumeTypeOf(int type)
{
  switch(type) {
    case CPhilipsSubHeader::ByteData:      return CMedIOVolume::UInt8;
    case CPhilipsSubHeader::SignedShort:   return CMedIOVolume::Int16;
    case CPhilipsSubHeader::UnsignedShort: return CMedIOVolume::UInt16;
    case CPhilipsSubHeader::Float:         return CMedIOVolume::Float32;
  }
  return CMedIOVolume::UnknownDataType;
}

CPhilipsSubHeader::Data_Type philipsTypeOf(CMedIOVolume::DataType type)
{
  switch(type) {
    case CMedIOVolume::UInt8:   return CPhilipsSubHeader::ByteData;
    case CMedIOVolume::Int16:   return CPhilipsSubHeader::SignedShort;
    case CMedIOVolume::UInt16:  return CPhilipsSubHeader::UnsignedShort;
    case CMedIOVolume::Float32: return CPhilipsSubHeader::Float;
    default: break;
  }
  return CPhilipsSubHeader::UnknownDataType;
}

bool isImageSubHeader(const CPhilipsSubHeader* subHeader)
{
  return subHeader != NULL && (subHeader->subHeaderType() == CPhilipsSubHeader::Image ||
                               subHeader->subHeaderType() == CPhilipsSubHeader::Syntegra);
}

// ECAT7 headers of a Philips frame (nz slices)
bool philipsHeaders(const CPhilipsMainHeader* mainHeader, const CPhilipsSubHeader* firstSlice, int nz, int frames,
                    const QString& fileName, CMedIOFrame& f)
{
  if(f.subHeader.convertFrom(firstSlice, mainHeader) == false ||
     f.mainHeader.convertFrom(mainHeader, firstSlice) == false) {
    E("unable to convert the Philips headers");
    return false;
  }

  f.subHeader.setZ_Dimension(nz);
  f.subHeader.setMedIOData(NULL);
  f.mainHeader.setNum_Frames(frames);
  f.mainHeader.setNum_Planes(nz);
  f.mainHeader.setCalibration_Factor(1.0f); // the scale factors give physical values
  f.mainHeader.setMedIOData(NULL);

  // the ECAT7 field holds 31 characters only: use the file name without its path
  if(fileName.isEmpty() == false)
    f.mainHeader.setOriginal_File_Name(QFileInfo(fileName).fileName().left(31).toLatin1().constData());

  return true;
}

// ECAT7 image sub header used to convert a frame of data type/scale v into a Philips sub header
CECAT7SubHeaderImage ecatSubHeaderFor(const CMedIOFrame& f, const CMedIOVolume& v)
{
  CECAT7SubHeaderImage s(f.subHeader);
  s.setX_Dimension(v.dimension(0));
  s.setY_Dimension(v.dimension(1));
  s.setZ_Dimension(v.dimension(2));
  s.setData_Type(v.dataType() == CMedIOVolume::UInt8 ? CECATSubHeader::ByteData :
                 v.dataType() == CMedIOVolume::Float32 ? CECATSubHeader::IEEEFloat : CECATSubHeader::SunShort);
  s.setScale_Factor(v.scaleFactor());
  return s;
}

// Philips sub header of a frame with data type and scale factor of v; the
// scale factor is checked with the main header the slices are read with
bool philipsSubHeader(const CMedIOFrame& f, const CMedIOVolume& v, const CPhilipsMainHeader* mainHeader,
                      CPhilipsSubHeaderImage* subHeader)
{
  const CECAT7SubHeaderImage s = ecatSubHeaderFor(f, v);
  if(subHeader->convertFrom(&s, &f.mainHeader) == false) {
    E("unable to convert the ECAT7 sub header into a Philips sub header");
    return false;
  }
  subHeader->setDatype(philipsTypeOf(v.dataType()));
  subHeader->setXdim(v.dimension(0));
  subHeader->setYdim(v.dimension(1));

  // the scale factor (suvscl for calibrated data) has to reproduce the physical values
  bool ok = false;
  const double scale = subHeader->ecat_Scale_Factor(ok, mainHeader);
  if(ok == false || std::fabs(scale - v.scaleFactor()) > 1e-6 * std::fabs(v.scaleFactor())) {
    subHeader->setSuvscl(0.0f);
    subHeader->setImgscl(v.scaleFactor());
  }

  return true;
}

} // namespace

//=============================================================================================

short philipsFrameNumber(CPhilipsFile* file, int index)
{
  return static_cast<short>(file->minFrame() + index);
}

bool readPhilipsFrameData(CPhilipsFile* file, const CPhilipsMainHeader* mainHeader, short frame,
                          CMedIOVolume& v, CPhilipsSubHeader*& firstSlice, bool* unscaled)
{
  ENTER();

  firstSlice = NULL;
  if(unscaled)
    *unscaled = false;

  const int minSlice = file->minSlice();
  const int maxSlice = file->maxSlice();
  const int nz = file->numSlices(frame);

  // slice numbers of this frame (see CPhilipsFile::readMatrix())
  const int increment = (nz > 1) ? (maxSlice - minSlice) / (nz - 1) : 1;
  if(nz < 1 || increment < 1) {
    E("invalid slice numbering of frame %d", frame);
    RETURN(false);
    return false;
  }

  int nx = 0, ny = 0;
  int dataType = CPhilipsSubHeader::UnknownDataType;
  QByteArray frameData;
  QVector<double> scales;
  QScopedPointer<CPhilipsSubHeader> first;

  for(int k=0; k < nz; k++) {
    const int slice = minSlice + k * increment;
    QByteArray* data = NULL;
    CPhilipsSubHeader* subHeader = NULL;

    if(file->readMatrix(data, subHeader, frame, slice) == false || data == NULL || subHeader == NULL) {
      delete data;
      delete subHeader;
      E("unable to read slice %d of frame %d", slice, frame);
      RETURN(false);
      return false;
    }
    QScopedPointer<QByteArray> dataGuard(data);
    QScopedPointer<CPhilipsSubHeader> subHeaderGuard(subHeader);

    if(isImageSubHeader(subHeader) == false) {
      E("only Philips image data can be converted");
      RETURN(false);
      return false;
    }
    const CPhilipsSubHeaderImage* s = static_cast<const CPhilipsSubHeaderImage*>(subHeader);

    if(k == 0) {
      nx = s->xdim();
      ny = s->ydim();
      dataType = s->datype();
    }

    if(s->xdim() != nx || s->ydim() != ny || nx < 1 || ny < 1) {
      E("the slices of the Philips file differ in their dimensions");
      RETURN(false);
      return false;
    }
    if(s->datype() != dataType || volumeTypeOf(dataType) == CMedIOVolume::UnknownDataType) {
      E("unsupported or differing Philips data types");
      RETURN(false);
      return false;
    }

    const qint64 bytes = static_cast<qint64>(nx) * ny * CMedIOVolume::bytesPerVoxel(volumeTypeOf(dataType));
    if(data->size() < bytes) {
      E("slice %d of frame %d is too small", slice, frame);
      RETURN(false);
      return false;
    }
    frameData.append(data->constData(), bytes);

    bool ok = false;
    double scale = s->ecat_Scale_Factor(ok, mainHeader);
    if(ok == false || scale <= 0.0 || std::isfinite(scale) == false) {
      if(unscaled)
        *unscaled = true;
      scale = 1.0;
    }
    scales.append(scale);

    if(k == 0)
      first.reset(subHeaderGuard.take());
  }

  bool result = v.setData(nx, ny, nz, volumeTypeOf(dataType), frameData);
  for(int k=0; k < nz && result; k++)
    v.setScaleFactor(k, scales[k]);

  if(result)
    firstSlice = first.take();

  RETURN(result);
  return result;
}

bool readPhilipsFrame(CPhilipsFile* file, short frame, CMedIOFrame& f)
{
  ENTER();

  if(frame == -1)
    frame = file->minFrame();

  CPhilipsMainHeader* mh = NULL;
  if(file->readMainHeader(mh) == false || mh == NULL) {
    delete mh;
    RETURN(false);
    return false;
  }
  QScopedPointer<CPhilipsMainHeader> mainHeader(mh);

  if(mh->filtyp() != CPhilipsMainHeader::Image && mh->filtyp() != CPhilipsMainHeader::Syntegra) {
    E("only Philips image and Syntegra files can be converted");
    RETURN(false);
    return false;
  }

  CPhilipsSubHeader* firstSlice = NULL;
  bool unscaled = false;
  bool result = readPhilipsFrameData(file, mh, frame, f.volume, firstSlice, &unscaled);
  QScopedPointer<CPhilipsSubHeader> firstSliceGuard(firstSlice);

  if(unscaled)
    W("frame %d: no valid scale factor for some slices, their stored values are used", frame);

  if(result)
    result = philipsHeaders(mh, firstSlice, f.volume.dimension(2), file->numFrames(), file->fileName(), f);

  if(result)
    setVolumeGeometry(f);

  RETURN(result);
  return result;
}

bool philipsFrameFromMatrix(const QByteArray& data, const CPhilipsSubHeader& subHeader, const CMedIOHeader* mainHeader, CMedIOFrame& f)
{
  ENTER();

  const CPhilipsMainHeader* mh = dynamic_cast<const CPhilipsMainHeader*>(mainHeader);
  if(isImageSubHeader(&subHeader) == false || mh == NULL) {
    E("Philips image matrices need a Philips image sub header and the Philips main header");
    RETURN(false);
    return false;
  }

  const CPhilipsSubHeaderImage& s = static_cast<const CPhilipsSubHeaderImage&>(subHeader);
  const CMedIOVolume::DataType type = volumeTypeOf(s.datype());
  const qint64 sliceBytes = static_cast<qint64>(s.xdim()) * s.ydim() * CMedIOVolume::bytesPerVoxel(type);

  // all slices of the frame
  if(sliceBytes <= 0 || data.size() < sliceBytes || data.size() % sliceBytes != 0) {
    E("size of the Philips matrix (%d bytes) does not match its sub header (%lld bytes per slice)", data.size(), sliceBytes);
    RETURN(false);
    return false;
  }
  const int nz = static_cast<int>(data.size() / sliceBytes);

  bool ok = false;
  double scale = s.ecat_Scale_Factor(ok, mh);
  if(ok == false || scale <= 0.0 || std::isfinite(scale) == false) {
    W("no valid scale factor, the stored values are used");
    scale = 1.0;
  }

  bool result = f.volume.setData(s.xdim(), s.ydim(), nz, type, data) &&
                philipsHeaders(mh, &s, nz, mh->maxfrm() - mh->minfrm() + 1, QString(), f);
  if(result) {
    f.volume.setScaleFactor(scale);
    setVolumeGeometry(f);
  }

  RETURN(result);
  return result;
}

bool philipsMatrixFromFrame(const CMedIOFrame& f, QByteArray& data, CMedIOHeader*& header)
{
  ENTER();

  CMedIOVolume v = f.volume;
  if(fitDataType(v, philipsTypes, false, false) == false) {
    RETURN(false);
    return false;
  }

  // the main header of the Philips representation (scale factors)
  CPhilipsMainHeader mainHeader(NULL, CPhilipsMainHeader::Image);
  const CECAT7SubHeaderImage s = ecatSubHeaderFor(f, v);
  mainHeader.convertFrom(&f.mainHeader, &s);

  QScopedPointer<CPhilipsSubHeaderImage> subHeader(new CPhilipsSubHeaderImage());
  bool result = philipsSubHeader(f, v, &mainHeader, subHeader.data());

  if(result) {
    subHeader->setSlcnum(0);

    // value range of the stored values (16 bit fields)
    if(v.dataType() != CMedIOVolume::Float32) {
      double lo = 0.0, hi = 0.0, sum = 0.0;
      for(qint64 i=0; i < v.voxelCount(); i++) {
        const double value = v.storedValue(i);
        lo = (i == 0) ? value : qMin(lo, value);
        hi = (i == 0) ? value : qMax(hi, value);
        sum += value;
      }
      subHeader->setImgmin(static_cast<short>(qBound(-32768.0, lo, 32767.0)));
      subHeader->setImgmax(static_cast<short>(qBound(-32768.0, hi, 32767.0)));
      subHeader->setImgsum(sum);
    }

    subHeader->setMedIOData(NULL);
    data = v.data();
    header = subHeader.take();
  }

  RETURN(result);
  return result;
}

bool philipsMainHeaderFromFrame(const CMedIOFrame& f, CMedIOHeader*& header)
{
  ENTER();

  CMedIOVolume v = f.volume;
  fitDataType(v, philipsTypes, false, false);

  QScopedPointer<CPhilipsMainHeader> mainHeader(new CPhilipsMainHeader(NULL, CPhilipsMainHeader::Image));
  const CECAT7SubHeaderImage s = ecatSubHeaderFor(f, v);
  bool result = mainHeader->convertFrom(&f.mainHeader, &s);

  if(result) {
    mainHeader->setFiltyp(CPhilipsMainHeader::Image);
    mainHeader->setMedIOData(NULL);
    header = mainHeader.take();
  }

  RETURN(result);
  return result;
}

bool writePhilipsFrame(CPhilipsFile* file, short frame, const CMedIOFrame& f)
{
  ENTER();

  if(frame == -1)
    frame = 1;

  // the main header of the file (written before): file type, slice numbering and scale factors
  CPhilipsMainHeader* mh = NULL;
  if(file->readMainHeader(mh) == false || mh == NULL) {
    delete mh;
    E("the Philips main header has to be written first");
    RETURN(false);
    return false;
  }
  QScopedPointer<CPhilipsMainHeader> mainHeader(mh);

  const CPhilipsMainHeader::File_Type type = mh->filtyp();
  if(type != CPhilipsMainHeader::Image && type != CPhilipsMainHeader::Syntegra) {
    E("only Philips image and Syntegra files can be written");
    RETURN(false);
    return false;
  }

  CMedIOVolume v = f.volume;
  if(fitDataType(v, philipsTypes, false, false) == false) {
    RETURN(false);
    return false;
  }

  QScopedPointer<CPhilipsSubHeader> subHeaderPtr(file->createEmptySubHeader());
  if(isImageSubHeader(subHeaderPtr.data()) == false) {
    RETURN(false);
    return false;
  }
  CPhilipsSubHeaderImage* subHeader = static_cast<CPhilipsSubHeaderImage*>(subHeaderPtr.data());
  bool result = philipsSubHeader(f, v, mh, subHeader);

  // every plane as slice: image files number their slices from 0, Syntegra files from 1
  const int firstSlice = (type == CPhilipsMainHeader::Syntegra) ? 1 : 0;
  const int sliceStep = qMax(static_cast<short>(1), mh->slcthk());

  // a frame written before can be overwritten if the new data has the same
  // slices; other slices of the old frame would remain otherwise
  if(file->numFrames() > 0 && file->numSlices(frame) > 0) {
    bool same = (file->numSlices(frame) == v.dimension(2));
    for(int k=0; k < v.dimension(2) && same; k++)
      same = (file->directory()->item(frame, firstSlice + k * sliceStep) != NULL);
    if(same == false) {
      E("frame %d of the Philips file holds other slices (%d) than the new data (%d)", frame, file->numSlices(frame), v.dimension(2));
      RETURN(false);
      return false;
    }
  }
  const qint64 planeVoxels = static_cast<qint64>(v.dimension(0)) * v.dimension(1);
  const int bytes = CMedIOVolume::bytesPerVoxel(v.dataType());

  for(int k=0; k < v.dimension(2) && result; k++) {
    if(v.dataType() != CMedIOVolume::Float32) {
      double lo = 0.0, hi = 0.0, sum = 0.0;
      for(qint64 i=0; i < planeVoxels; i++) {
        const double value = v.storedValue(k * planeVoxels + i);
        lo = (i == 0) ? value : qMin(lo, value);
        hi = (i == 0) ? value : qMax(hi, value);
        sum += value;
      }
      subHeader->setImgmin(static_cast<short>(qBound(-32768.0, lo, 32767.0)));
      subHeader->setImgmax(static_cast<short>(qBound(-32768.0, hi, 32767.0)));
      subHeader->setImgsum(sum);
    }

    const int slice = firstSlice + k * sliceStep;
    subHeader->setSlcnum(slice);
    result = file->writeMatrix(v.data().constData() + k * planeVoxels * bytes, static_cast<unsigned int>(planeVoxels * bytes),
                               *subHeader, frame, slice);
  }

  RETURN(result);
  return result;
}

} // namespace MedIOMatrix
