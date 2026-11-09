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

//! @file CNIFTIMatrix.cpp
//! @brief NIfTI adapter of the matrix conversion (see CMedIOMatrix.h)
//!
//! NIfTI volumes are reordered by means of the sform, the qform or the
//! default orientation into the ECAT7 voxel order and back, using the same
//! geometry as CNIFTIConverter (see CNIFTIConverter.h and CNIFTIGeometry.h).
//! Matrices in the NIfTI representation of another format use the voxel order
//! of NIfTI files written by libmedio (ECAT7 voxel order with reversed
//! planes) and a NIfTI-1 header describing the file the frame belongs to.

#include "CMedIOMatrix.h"
#include "CNIFTIFile.h"
#include "CNIFTIMainHeader.h"
#include "CNIFTI1MainHeader.h"
#include "CNIFTI2MainHeader.h"
#include "CNIFTIGeometry.h"

#include <QScopedPointer>
#include <QVector>

#include <climits>
#include <cmath>
#include <cstring>

#include <rtdebug.h>

namespace MedIOMatrix {

namespace {

const unsigned niftiTypes = typeBit(CMedIOVolume::UInt8) | typeBit(CMedIOVolume::Int8) |
                            typeBit(CMedIOVolume::Int16) | typeBit(CMedIOVolume::UInt16) |
                            typeBit(CMedIOVolume::Int32) | typeBit(CMedIOVolume::UInt32) |
                            typeBit(CMedIOVolume::Float32) | typeBit(CMedIOVolume::Float64);

// header values of both NIfTI versions
struct NIFTIValues
{
  qint64 dim[8];
  int datatype;
  double slope;   // 1 if scl_slope is 0 (no scaling)
  double inter;
  double affine[3][4]; // LPS, mm
};

template <class H>
NIFTIValues niftiValuesOf(const H& h)
{
  NIFTIValues n;
  for(short i=0; i < 8; i++)
    n.dim[i] = h.dim(i);
  n.datatype = h.dataType();
  n.slope = h.scl_Slope();
  n.inter = h.scl_Inter();
  if(n.slope == 0.0 || std::isfinite(n.slope) == false) {
    n.slope = 1.0;
    n.inter = 0.0;
  }
  if(std::isfinite(n.inter) == false)
    n.inter = 0.0;

  // RAS -> LPS
  niftiAffine(h, n.affine);
  for(int j=0; j < 4; j++) {
    n.affine[0][j] = -n.affine[0][j];
    n.affine[1][j] = -n.affine[1][j];
  }

  return n;
}

NIFTIValues niftiValues(const CNIFTIMainHeader* header)
{
  if(header->mainHeaderType() == CNIFTIMainHeader::NIFTI1MainHeader)
    return niftiValuesOf(*static_cast<const CNIFTI1MainHeader*>(header));

  return niftiValuesOf(*static_cast<const CNIFTI2MainHeader*>(header));
}

CMedIOVolume::DataType volumeTypeOf(int datatype)
{
  switch(datatype) {
    case 2:   return CMedIOVolume::UInt8;
    case 256: return CMedIOVolume::Int8;
    case 4:   return CMedIOVolume::Int16;
    case 512: return CMedIOVolume::UInt16;
    case 8:   return CMedIOVolume::Int32;
    case 768: return CMedIOVolume::UInt32;
    case 16:  return CMedIOVolume::Float32;
    case 64:  return CMedIOVolume::Float64;
  }
  return CMedIOVolume::UnknownDataType;
}

short niftiDatatypeOf(CMedIOVolume::DataType type)
{
  switch(type) {
    case CMedIOVolume::UInt8:   return 2;
    case CMedIOVolume::Int8:    return 256;
    case CMedIOVolume::Int16:   return 4;
    case CMedIOVolume::UInt16:  return 512;
    case CMedIOVolume::Int32:   return 8;
    case CMedIOVolume::UInt32:  return 768;
    case CMedIOVolume::Float32: return 16;
    case CMedIOVolume::Float64: return 64;
    case CMedIOVolume::UnknownDataType: break;
  }
  return 0;
}

// volume of NIfTI voxel data (one 3D volume, machine byte order) described by a header
bool niftiVolume(const CNIFTIMainHeader* header, const QByteArray& data, CMedIOVolume& v)
{
  const NIFTIValues n = niftiValues(header);
  const CMedIOVolume::DataType type = volumeTypeOf(n.datatype);
  if(type == CMedIOVolume::UnknownDataType) {
    E("NIfTI datatype %d is not supported", n.datatype);
    return false;
  }

  int dims[3];
  for(int a=0; a < 3; a++)
    dims[a] = (a + 1 <= n.dim[0]) ? static_cast<int>(n.dim[a + 1]) : 1;

  if(v.setData(dims[0], dims[1], dims[2], type, data) == false)
    return false;

  v.setScaleFactor(n.slope);
  v.setIntercept(n.inter);
  v.setAffine(n.affine);
  return true;
}

// ECAT7 representation of a NIfTI volume (native voxel order) of volume f (0-based)
bool niftiFrame(const CNIFTIMainHeader* header, const CMedIOVolume& volume, const QJsonObject& json, int f, CMedIOFrame& frame)
{
  ECATGeometryFromNIFTI g;
  QString error;
  if(ecatGeometryFromNIFTI(header, g, &error) == false) {
    E("%s", error.toLatin1().constData());
    return false;
  }

  if(frame.mainHeader.convertFrom(header, NULL) == false || frame.subHeader.convertFrom(NULL, header) == false) {
    E("unable to convert the NIfTI header");
    return false;
  }
  applyNIFTIMetadata(frame.mainHeader, json);
  frame.mainHeader.setCalibration_Factor(1.0f);
  frame.mainHeader.setMedIOData(NULL);
  frame.subHeader.setMedIOData(NULL);

  double start = 0.0, duration = 0.0;
  niftiFrameTiming(g, json, f, start, duration);
  frame.subHeader.setFrame_Start_Time(ecatFrameTime(start));
  frame.subHeader.setFrame_Duration(ecatFrameTime(duration));
  if(json.contains("Recon_Zoom"))
    frame.subHeader.setRecon_Zoom(json["Recon_Zoom"].toDouble());

  // voxels in ECAT7 voxel order (oblique axes: closest axes, as the geometry of the headers)
  frame.volume = volume;
  frame.volume.setFrameStart(start);
  frame.volume.setFrameDuration(duration);
  bool oblique = false;
  if(frame.volume.reorient("LPI", &oblique) == false)
    return false;
  if(oblique)
    W("oblique NIfTI orientation, the voxel axes are assigned to the closest patient axes");

  for(int a=0; a < 3; a++) {
    if(frame.volume.dimension(a) != g.dims[a]) {
      E("inconsistent NIfTI geometry");
      return false;
    }
  }

  return true;
}

// NIfTI-1 header of a frame in the NIfTI representation (voxel order of
// libmedio, data type and scaling of v)
CNIFTI1MainHeader* niftiHeader(const CMedIOFrame& f, const CMedIOVolume& v)
{
  QScopedPointer<CNIFTI1MainHeader> h(new CNIFTI1MainHeader());

  CECAT7SubHeaderImage sub(f.subHeader);
  sub.setX_Dimension(v.dimension(0));
  sub.setY_Dimension(v.dimension(1));
  sub.setZ_Dimension(v.dimension(2));
  sub.setScale_Factor(1.0f);
  if(convertNIFTIFromECAT(*h, &f.mainHeader, &sub) == false)
    return NULL;

  const short datatype = niftiDatatypeOf(v.dataType());
  h->setDataType(datatype);
  h->setBit_Pix(CMedIOVolume::bytesPerVoxel(v.dataType()) * 8);
  h->setScl_Slope(v.scaleFactor());
  h->setScl_Inter(v.intercept());
  h->setDescrip("converted by libmedio");
  h->setMedIOData(NULL);

  return h.take();
}

template <class H>
void setNiftiTimingOf(H* h, double start, double duration)
{
  // time unit of the header (s if unknown)
  double unit = 1.0;
  switch(static_cast<int>(h->xyzt_Units()) & 0x38) {
    case 16: unit = 0.001; break; // ms
    case 24: unit = 1e-6;  break; // us
  }
  h->setToffset(start / unit);
  h->setPix_Dim(4, duration / unit);
}

template <class H>
void niftiTimingOf(const H* h, double& start, double& duration)
{
  double unit = 1.0;
  switch(static_cast<int>(h->xyzt_Units()) & 0x38) {
    case 16: unit = 0.001; break; // ms
    case 24: unit = 1e-6;  break; // us
  }
  start = h->toffset() * unit;
  duration = h->pix_Dim(4) * unit;
  if(std::isfinite(start) == false || start < 0.0)
    start = 0.0;
  if(std::isfinite(duration) == false || duration < 0.0)
    duration = 0.0;
}

} // namespace

//! the header of a NIfTI matrix describes the file it belongs to, with the
//! start time and duration of the frame as toffset and pixdim[4]
void setNiftiFrameTiming(CNIFTIMainHeader* header, double start, double duration)
{
  if(header->mainHeaderType() == CNIFTIMainHeader::NIFTI1MainHeader)
    setNiftiTimingOf(static_cast<CNIFTI1MainHeader*>(header), start, duration);
  else
    setNiftiTimingOf(static_cast<CNIFTI2MainHeader*>(header), start, duration);
}

//=============================================================================================

QList<short> niftiFrameNumbers(CNIFTIFile* file)
{
  ENTER();
  QList<short> numbers;

  // volumes: dim[4..dim[0]] (see CNIFTIFile::readMatrix(data, frame))
  CNIFTIMainHeader* h = NULL;
  if(file->readMainHeader(h) && h != NULL) {
    const NIFTIValues n = niftiValues(h);
    qint64 volumes = 1;
    for(int i=4; i < 8; i++) {
      if(i <= n.dim[0])
        volumes *= n.dim[i];
    }
    for(qint64 frame=1; frame <= volumes && frame <= SHRT_MAX; frame++)
      numbers.append(static_cast<short>(frame));
  }
  delete h;

  LEAVE();
  return numbers;
}

bool readNiftiVolume(CNIFTIFile* file, short frame, CMedIOVolume& v, CNIFTIMainHeader** header, QJsonObject* metadata)
{
  ENTER();

  if(frame == -1)
    frame = 1;

  CNIFTIMainHeader* h = NULL;
  if(file->readMainHeader(h) == false || h == NULL) {
    RETURN(false);
    return false;
  }
  QScopedPointer<CNIFTIMainHeader> headerGuard(h);

  QByteArray* data = NULL;
  bool result = file->readMatrix(data, frame) && niftiVolume(h, *data, v);
  delete data;

  if(result) {
    // frame timing from the metadata of CNIFTIConverter or the header
    const QJsonObject json = niftiMetadata(*file, h, file->fileName());
    ECATGeometryFromNIFTI g;
    if(ecatGeometryFromNIFTI(h, g)) {
      double start = 0.0, duration = 0.0;
      niftiFrameTiming(g, json, frame - 1, start, duration);
      v.setFrameStart(start);
      v.setFrameDuration(duration);
    }
    if(metadata)
      *metadata = json;
    if(header)
      *header = headerGuard.take();
  }

  RETURN(result);
  return result;
}

bool readNiftiFrame(CNIFTIFile* file, short frame, CMedIOFrame& f)
{
  ENTER();

  if(frame == -1)
    frame = 1;

  CMedIOVolume v;
  CNIFTIMainHeader* h = NULL;
  QJsonObject json;
  bool result = readNiftiVolume(file, frame, v, &h, &json);
  QScopedPointer<CNIFTIMainHeader> headerGuard(h);

  if(result)
    result = niftiFrame(h, v, json, frame - 1, f);

  RETURN(result);
  return result;
}

bool niftiFrameFromMatrix(const QByteArray& data, const CNIFTIMainHeader& header, CMedIOFrame& f)
{
  ENTER();

  CMedIOVolume v;
  bool result = niftiVolume(&header, data, v) && niftiFrame(&header, v, QJsonObject(), 0, f);

  // timing of the frame (see setNiftiFrameTiming())
  if(result) {
    double start = 0.0, duration = 0.0;
    if(header.mainHeaderType() == CNIFTIMainHeader::NIFTI1MainHeader)
      niftiTimingOf(static_cast<const CNIFTI1MainHeader*>(&header), start, duration);
    else
      niftiTimingOf(static_cast<const CNIFTI2MainHeader*>(&header), start, duration);
    f.subHeader.setFrame_Start_Time(ecatFrameTime(start));
    f.subHeader.setFrame_Duration(ecatFrameTime(duration));
    f.volume.setFrameStart(start);
    f.volume.setFrameDuration(duration);
  }

  RETURN(result);
  return result;
}

bool niftiMatrixFromFrame(const CMedIOFrame& f, QByteArray& data, CMedIOHeader*& header)
{
  ENTER();

  // voxel order of NIfTI files written by libmedio: ECAT7 order with reversed planes
  CMedIOVolume v = f.volume;
  if(v.reorient("LPS") == false || fitDataType(v, niftiTypes, false, true) == false) {
    RETURN(false);
    return false;
  }

  CNIFTI1MainHeader* h = niftiHeader(f, v);
  if(h)
    setNiftiFrameTiming(h, f.volume.frameStart(), f.volume.frameDuration());
  header = h;
  data = v.data();

  bool result = (header != NULL);
  RETURN(result);
  return result;
}

bool niftiMainHeaderFromFrame(const CMedIOFrame& f, CMedIOData* source, CMedIOHeader*& header)
{
  ENTER();

  CMedIOVolume v = f.volume;
  if(fitDataType(v, niftiTypes, false, true) == false) {
    RETURN(false);
    return false;
  }

  // all frames are stored with the same datatype and scaling: float32 if the
  // frames differ in their representation
  // (f is the first frame of source, see CMedIOData::readMainHeader())
  bool same = true;
  const QList<short> numbers = source ? frameNumbers(source) : QList<short>();
  for(int i=1; i < numbers.size() && same; i++) {
    CMedIOFrame other;
    if(readFrame(source, numbers[i], other) == false) {
      same = false;
      break;
    }
    CMedIOVolume w = other.volume;
    same = fitDataType(w, niftiTypes, false, true) && w.dataType() == v.dataType() &&
           w.scaleFactor() == v.scaleFactor() && w.intercept() == v.intercept();
  }
  if(same == false)
    v.convertToPhysicalValues();

  header = niftiHeader(f, v);

  bool result = (header != NULL);
  RETURN(result);
  return result;
}

bool writeNiftiFrame(CNIFTIFile* file, short frame, const CMedIOFrame& f)
{
  ENTER();

  if(frame == -1)
    frame = 1;

  // the main header of the file (written before) defines datatype, scaling and orientation
  CNIFTIMainHeader* h = NULL;
  if(file->readMainHeader(h) == false || h == NULL) {
    E("the NIfTI main header has to be written first");
    RETURN(false);
    return false;
  }
  QScopedPointer<CNIFTIMainHeader> headerGuard(h);

  ECATGeometryFromNIFTI g;
  QString error;
  if(ecatGeometryFromNIFTI(h, g, &error) == false) {
    E("%s", error.toLatin1().constData());
    RETURN(false);
    return false;
  }

  const CMedIOVolume& v = f.volume;
  for(int a=0; a < 3; a++) {
    if(v.dimension(a) != g.dims[a]) {
      E("dimensions of the matrix (%d x %d x %d) do not match the NIfTI main header (%d x %d x %d)",
        v.dimension(0), v.dimension(1), v.dimension(2), g.dims[0], g.dims[1], g.dims[2]);
      RETURN(false);
      return false;
    }
  }

  const NIFTIValues n = niftiValues(h);
  const CMedIOVolume::DataType type = volumeTypeOf(n.datatype);
  if(type == CMedIOVolume::UnknownDataType) {
    E("NIfTI datatype %d is not supported", n.datatype);
    RETURN(false);
    return false;
  }

  // values in the datatype and scaling of the file (ECAT7 voxel order)
  CMedIOVolume converted = v;
  const bool same = (v.dataType() == type && v.hasUniformScaleFactor() && v.scaleFactor() == n.slope && v.intercept() == n.inter);
  if(same == false) {
    const qint64 voxels = v.voxelCount();
    double lo = 0.0, hi = 0.0;
    const bool integer = (type != CMedIOVolume::Float32 && type != CMedIOVolume::Float64);
    if(integer) {
      // value range of the integer type
      switch(type) {
        case CMedIOVolume::UInt8:  lo = 0.0;           hi = 255.0;        break;
        case CMedIOVolume::Int8:   lo = -128.0;        hi = 127.0;        break;
        case CMedIOVolume::Int16:  lo = -32768.0;      hi = 32767.0;      break;
        case CMedIOVolume::UInt16: lo = 0.0;           hi = 65535.0;      break;
        case CMedIOVolume::Int32:  lo = -2147483648.0; hi = 2147483647.0; break;
        default:                   lo = 0.0;           hi = 4294967295.0; break;
      }
    }

    QVector<double> stored(voxels);
    for(qint64 i=0; i < voxels; i++) {
      double value = (v.value(i) - n.inter) / n.slope;
      if(integer) {
        value = std::floor(value + 0.5);
        if(std::isfinite(value) == false || value < lo || value > hi) {
          E("voxel %lld (%g) does not fit into the NIfTI datatype %d with scl_slope %g and scl_inter %g",
            i, v.value(i), n.datatype, n.slope, n.inter);
          RETURN(false);
          return false;
        }
      }
      stored[i] = value;
    }

    // store the values with the type of the file
    QByteArray typed(voxels * CMedIOVolume::bytesPerVoxel(type), '\0');
    for(qint64 i=0; i < voxels; i++) {
      char* d = typed.data();
      switch(type) {
        case CMedIOVolume::UInt8:   reinterpret_cast<quint8*>(d)[i] = static_cast<quint8>(stored[i]); break;
        case CMedIOVolume::Int8:    reinterpret_cast<qint8*>(d)[i] = static_cast<qint8>(stored[i]); break;
        case CMedIOVolume::Int16:   reinterpret_cast<qint16*>(d)[i] = static_cast<qint16>(stored[i]); break;
        case CMedIOVolume::UInt16:  reinterpret_cast<quint16*>(d)[i] = static_cast<quint16>(stored[i]); break;
        case CMedIOVolume::Int32:   reinterpret_cast<qint32*>(d)[i] = static_cast<qint32>(stored[i]); break;
        case CMedIOVolume::UInt32:  reinterpret_cast<quint32*>(d)[i] = static_cast<quint32>(stored[i]); break;
        case CMedIOVolume::Float32: reinterpret_cast<float*>(d)[i] = static_cast<float>(stored[i]); break;
        default:                    reinterpret_cast<double*>(d)[i] = stored[i]; break;
      }
    }
    converted.setData(v.dimension(0), v.dimension(1), v.dimension(2), type, typed);
  }

  // reorder from the ECAT7 voxel order into the voxel order of the file
  const qint64 dims[3] = { n.dim[1], n.dim[2], n.dim[3] };
  const int bytes = CMedIOVolume::bytesPerVoxel(type);
  QByteArray data(converted.data().size(), '\0');
  const char* src = converted.data().constData();
  for(int k=0; k < g.dims[2]; k++)
    for(int j=0; j < g.dims[1]; j++)
      for(int i=0; i < g.dims[0]; i++, src += bytes)
        memcpy(data.data() + niftiVoxelIndex(g, dims, i, j, k) * bytes, src, bytes);

  bool result = file->writeMatrix(data, frame);

  RETURN(result);
  return result;
}

} // namespace MedIOMatrix
