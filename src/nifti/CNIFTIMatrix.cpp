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
#include "CMedIOHeaderMapping.h"
#include "CMedIOHeaderDictionary.h"
#include "CNIFTIFile.h"
#include "CNIFTIMainHeader.h"
#include "CNIFTI1MainHeader.h"
#include "CNIFTI2MainHeader.h"
#include "CNIFTIGeometry.h"
#include "CNIFTIExtensions.h"
#include "CECATFile.h"
#include "CECATSubHeader.h"
#include "CPhilipsFile.h"
#include "CPhilipsSubHeader.h"
#include "bswap.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
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


//---------------------------------------------------------------------------------------------
// frame timing of the volumes written into a NIfTI file
//
// NIfTI headers can only describe equally spaced volumes (toffset, pixdim[4]).
// writeNiftiFrame() therefore stores the start time and duration (s) of every
// volume in the metadata extensions (see CNIFTIExtensions.h): as
// FrameTimesStart and FrameDuration in the JSON extension like CNIFTIConverter
// (read by niftiFrameTiming()) and in the private tags of PMOD in the DICOM
// extension. Other entries of the metadata are kept. The extensions are
// written into the room between the header and vox_offset (single files) or
// at the end of the header file (header/image pairs). A single file whose
// voxel data starts directly behind the header gets room for the timing of
// all volumes when its first volume is written: vox_offset is moved behind
// it. Other extensions than those of the metadata are never overwritten.

// number of volumes with a timing of their own (ECATGeometryFromNIFTI::frames)
template <class H>
int timingVolumes(const H* h)
{
  return (h->dim(0) >= 4 && h->dim(4) > 1) ? static_cast<int>(h->dim(4)) : 1;
}

// write the extension indicator and the metadata extensions filling room bytes
template <class H>
bool writeMetadataExtensions(CNIFTIFile* file, const H* h, const QJsonObject& json, qint64 room)
{
  QByteArray extension("\x01\0\0\0", 4);
  extension.append(NIFTIExtensions::metadataExtensions(json, h->byteOrder() != QSysInfo::ByteOrder, room));

  return extension.size() == 4 + room &&
         file->seek(h->rawDataSize()) && file->write(extension) == extension.size();
}

// store the timing of volume frame (1-based) in the JSON metadata of the file
template <class H>
bool storeFrameTiming(CNIFTIFile* file, H* h, short frame, double start, double duration)
{
  const int volumes = timingVolumes(h);
  if(frame < 1 || frame > volumes)
    return false; // further volumes of dim[5..7] have no timing of their own

  // start of the voxel data of a single file: at least behind the header and
  // the extension indicator (as CNIFTIFile and the header save() take it)
  const qint64 headerSize = h->rawDataSize();
  const qint64 voxOffset = qMax(static_cast<qint64>(h->vox_Offset()), headerSize + 4);

  // the extensions present: only the metadata extensions (a JSON extension
  // and a DICOM extension, see CNIFTIFile::writeMainHeader()) are rewritten
  const qint64 end = file->isPair() ? file->size() : voxOffset;
  bool extensions = false;
  qint64 used = 0;
  bool own = true;
  bool hasJson = false;
  QJsonObject json;
  foreach(const NIFTIExtensions::Extension& e, NIFTIExtensions::read(*file, headerSize, end, h->byteOrder() != QSysInfo::ByteOrder, &extensions, &used)) {
    if(e.code == NIFTIExtensions::DicomCode)
      continue;
    QByteArray text = e.data;
    while(text.endsWith('\0'))
      text.chop(1);
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(text, &error);
    if(e.code == NIFTIExtensions::JsonCode && hasJson == false && error.error == QJsonParseError::NoError && document.isObject()) {
      json = document.object();
      hasJson = true;
    } else {
      own = false;
    }
  }
  if(extensions && (own == false || hasJson == false)) {
    W("the NIfTI file has other header extensions than the metadata, the frame timing is not stored");
    return false;
  }

  // metadata extensions filling the room (e.g. the header attributes written
  // with the main header, see CNIFTIFile::writeMainHeader()) can be made
  // larger like a file without extensions
  const bool filling = extensions && used >= 16 && headerSize + 4 + used == end;

  // timing of all volumes: as stored before, the timing of the header otherwise
  QJsonArray starts = json["FrameTimesStart"].toArray();
  QJsonArray durations = json["FrameDuration"].toArray();
  if(starts.size() != volumes || durations.size() != volumes) {
    double start0 = 0.0, step = 0.0;
    niftiTimingOf(h, start0, step);
    if(h->dim(0) < 4)
      step = 0.0;
    starts = QJsonArray();
    durations = QJsonArray();
    for(int i=0; i < volumes; i++) {
      starts.append(start0 + i * step);
      durations.append(step);
    }
  }
  starts[frame - 1] = start;
  durations[frame - 1] = duration;
  json["FrameTimesStart"] = starts;
  json["FrameDuration"] = durations;
  const qint64 needed = NIFTIExtensions::metadataExtensions(json, h->byteOrder() != QSysInfo::ByteOrder).size();

  // room for the extensions: up to vox_offset (single file) or as before (pair)
  qint64 room = 0;
  if(file->isPair()) {
    room = qMax(static_cast<qint64>(0), file->size() - headerSize - 4);
    if(room == 0 || (filling && room < needed))
      room = qMax(needed, NIFTIExtensions::reservedSize(json, volumes));
  } else {
    room = voxOffset - headerSize - 4;
    if((extensions == false || filling) && room < needed && file->size() <= voxOffset) {
      // no voxel data yet: move the voxel data behind the room for the timing of all volumes
      room = qMax(needed, NIFTIExtensions::reservedSize(json, volumes));
      room += (16 - room % 16) % 16;
      h->setVox_Offset(headerSize + 4 + room);
      h->setMedIOData(file);
      if(file->writeMainHeader(*h) == false) {
        W("unable to make room for the frame timing in the NIfTI file");
        return false;
      }
    }
  }

  if(room < needed || room % 16 != 0) {
    W("no room for the frame timing in the NIfTI file, only the timing of the header is stored");
    return false;
  }
  if(writeMetadataExtensions(file, h, json, room) == false) {
    W("unable to write the frame timing into the NIfTI file");
    return false;
  }
  return true;
}

// start time and duration (s) of the frame of an ECAT7 or Philips image sub
// header (Philips: via its ECAT7 representation)
bool sourceFrameTiming(const CMedIOHeader* subHeader, const CMedIOHeader* mainHeader, double& start, double& duration)
{
  if(subHeader == NULL)
    return false;

  if(subHeader->headerFormat() == CMedIOHeader::ECATSubHeader) {
    if(static_cast<const CECATSubHeader*>(subHeader)->subHeaderType() != CECATSubHeader::ECAT7_Image)
      return false;
    const CECAT7SubHeaderImage* s = static_cast<const CECAT7SubHeaderImage*>(subHeader);
    start = s->frame_Start_Time() / 1000.0;
    duration = s->frame_Duration() / 1000.0;
    return true;
  }

  if(subHeader->headerFormat() == CMedIOHeader::PhilipsSubHeader && mainHeader != NULL &&
     mainHeader->headerFormat() == CMedIOHeader::PhilipsMainHeader) {
    CECAT7SubHeaderImage s;
    if(s.convertFrom(subHeader, mainHeader) == false)
      return false;
    start = s.frame_Start_Time() / 1000.0;
    duration = s.frame_Duration() / 1000.0;
    return true;
  }

  return false;
}

// sub header of a frame of an ECAT or Philips file (Philips: first slice), NULL if unavailable
CMedIOHeader* readSourceSubHeader(CMedIOData* file, short frame)
{
  switch(file->dataFormat()) {
    case CMedIOData::ECAT: {
      CECATSubHeader* sh = NULL;
      if(static_cast<CECATFile*>(file)->readSubHeader(sh, frame))
        return sh;
      delete sh;
    }
    break;

    case CMedIOData::Philips: {
      CPhilipsSubHeader* sh = NULL;
      if(static_cast<CPhilipsFile*>(file)->readSubHeader(sh, frame, -1))
        return sh;
      delete sh;
    }
    break;

    default:
    break;
  }
  return NULL;
}

} // namespace

QJsonObject niftiAttributesOfSource(const CMedIOHeader* mainHeader, const CMedIOHeader* subHeader, int volumes)
{
  ENTER();

  QJsonObject json = mainHeader->attributes();
  foreach(const QString& key, MedIOHeaderMapping::frameKeys())
    json.remove(key);

  // attributes and timing of the frames: of all frames of the file the main
  // header belongs to (if it is open for reading and has as many frames as
  // the volumes), of the given sub header for a single volume
  QList<QJsonObject> frameAttributes;
  QJsonArray starts, durations;
  CMedIOData* file = mainHeader->medIOData();
  if(volumes > 1 && file != NULL && file->isOpen() && file->isReadable()) {
    const QList<short> numbers = frameNumbers(file);
    if(numbers.size() == volumes) {
      foreach(short number, numbers) {
        QScopedPointer<CMedIOHeader> sh(readSourceSubHeader(file, number));
        double start = 0.0, duration = 0.0;
        if(sh.isNull() || sourceFrameTiming(sh.data(), mainHeader, start, duration) == false)
          break;
        frameAttributes.append(sh->attributes());
        starts.append(start);
        durations.append(duration);
      }
    }
  } else if(volumes == 1) {
    double start = 0.0, duration = 0.0;
    if(sourceFrameTiming(subHeader, mainHeader, start, duration)) {
      frameAttributes.append(subHeader->attributes());
      starts.append(start);
      durations.append(duration);
    }
  }

  if(frameAttributes.size() == volumes) {
    QJsonArray frames;
    bool any = false;
    foreach(const QJsonObject& f, frameAttributes) {
      frames.append(f);
      any = any || (f.isEmpty() == false);
    }
    if(any)
      json[QString::fromLatin1(CMedIOHeaderDictionary::FramesKey)] = frames;
    json["FrameTimesStart"] = starts;
    json["FrameDuration"] = durations;
  }

  LEAVE();
  return json;
}

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
  if(h) {
    setNiftiFrameTiming(h, f.volume.frameStart(), f.volume.frameDuration());

    // attributes of the source: main attributes and the frame in "Frames"
    MedIOHeaderMapping::transfer(f.mainAttributes, *h, MedIOHeaderMapping::frameKeys());
    if(f.frameAttributes.isEmpty() == false)
      h->updateAttributes(QJsonObject{{CMedIOHeaderDictionary::FramesKey, QJsonArray{f.frameAttributes}}});
  }
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
  if(same == false && v.convertToPhysicalValues() == false) {
    RETURN(false);
    return false;
  }

  header = niftiHeader(f, v);

  // frame attributes of all frames of the source
  if(header != NULL && source != NULL) {
    QJsonArray frames;
    bool any = false;
    foreach(short number, numbers) {
      QJsonObject mainAttributes, frameAttributes;
      fileAttributes(source, number, mainAttributes, frameAttributes);
      frames.append(frameAttributes);
      any = any || (frameAttributes.isEmpty() == false);
    }
    if(any)
      header->updateAttributes(QJsonObject{{CMedIOHeaderDictionary::FramesKey, frames}});
  }

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

    // store the values with the type of the file (without an intermediate
    // buffer of doubles, which would need 8 bytes per voxel)
    const qint64 size = voxels * CMedIOVolume::bytesPerVoxel(type);
    if(size > MaxDataSize) {
      E("%lld voxels of NIfTI datatype %d (%lld bytes) are too large for a frame", voxels, n.datatype, size);
      RETURN(false);
      return false;
    }

    QByteArray typed(static_cast<int>(size), '\0');
    char* d = typed.data();
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

      switch(type) {
        case CMedIOVolume::UInt8:   reinterpret_cast<quint8*>(d)[i] = static_cast<quint8>(value); break;
        case CMedIOVolume::Int8:    reinterpret_cast<qint8*>(d)[i] = static_cast<qint8>(value); break;
        case CMedIOVolume::Int16:   reinterpret_cast<qint16*>(d)[i] = static_cast<qint16>(value); break;
        case CMedIOVolume::UInt16:  reinterpret_cast<quint16*>(d)[i] = static_cast<quint16>(value); break;
        case CMedIOVolume::Int32:   reinterpret_cast<qint32*>(d)[i] = static_cast<qint32>(value); break;
        case CMedIOVolume::UInt32:  reinterpret_cast<quint32*>(d)[i] = static_cast<quint32>(value); break;
        case CMedIOVolume::Float32: reinterpret_cast<float*>(d)[i] = static_cast<float>(value); break;
        default:                    reinterpret_cast<double*>(d)[i] = value; break;
      }
    }

    if(converted.setData(v.dimension(0), v.dimension(1), v.dimension(2), type, typed) == false) {
      RETURN(false);
      return false;
    }
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

  // timing of the volume (JSON metadata), before the voxel data is written
  // (vox_offset may still be moved for it, see storeFrameTiming())
  if(h->mainHeaderType() == CNIFTIMainHeader::NIFTI1MainHeader)
    storeFrameTiming(file, static_cast<CNIFTI1MainHeader*>(h), frame, v.frameStart(), v.frameDuration());
  else
    storeFrameTiming(file, static_cast<CNIFTI2MainHeader*>(h), frame, v.frameStart(), v.frameDuration());

  bool result = file->writeMatrix(data, frame);

  RETURN(result);
  return result;
}

bool storeNiftiFrameTiming(CNIFTIFile* file, short frame, double start, double duration)
{
  ENTER();

  CNIFTIMainHeader* h = NULL;
  if(file->readMainHeader(h) == false || h == NULL) {
    E("the NIfTI main header has to be written first");
    RETURN(false);
    return false;
  }
  QScopedPointer<CNIFTIMainHeader> headerGuard(h);

  if(std::isfinite(start) == false || std::isfinite(duration) == false || start < 0.0 || duration < 0.0) {
    E("invalid frame timing (start %g s, duration %g s)", start, duration);
    RETURN(false);
    return false;
  }

  bool result = false;
  if(h->mainHeaderType() == CNIFTIMainHeader::NIFTI1MainHeader)
    result = storeFrameTiming(file, static_cast<CNIFTI1MainHeader*>(h), frame, start, duration);
  else
    result = storeFrameTiming(file, static_cast<CNIFTI2MainHeader*>(h), frame, start, duration);

  if(result == false)
    E("the timing of volume %d cannot be stored in the NIfTI file", frame);

  RETURN(result);
  return result;
}

} // namespace MedIOMatrix
