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

//! @file CMedIOMatrix.cpp
//! @brief conversion of matrix data between the file formats (see CMedIOMatrix.h)

#include "CMedIOMatrix.h"
#include "CMedIOHeaderMapping.h"
#include "CMedIOHeaderDictionary.h"
#include "CMedIOHeader.h"
#include "CECATFile.h"
#include "CECATSubHeader.h"
#include "CNIFTIFile.h"
#include "CNIFTIMainHeader.h"
#include "CPhilipsFile.h"
#include "CPhilipsSubHeader.h"
#include "CPhilipsMainHeader.h"
#include "CConcordeFile.h"
#include "CConcordeMainHeader.h"
#include "CConcordeFrameHeader.h"

#include <QJsonArray>
#include <QVector>

#include <cmath>
#include <limits>

#include <rtdebug.h>

namespace MedIOMatrix {

//=============================================================================================
// data types

namespace {

// value range of the integer types
bool integerRange(CMedIOVolume::DataType type, double& lo, double& hi)
{
  switch(type) {
    case CMedIOVolume::UInt8:  lo = 0.0;           hi = 255.0;          return true;
    case CMedIOVolume::Int8:   lo = -128.0;        hi = 127.0;          return true;
    case CMedIOVolume::Int16:  lo = -32768.0;      hi = 32767.0;        return true;
    case CMedIOVolume::UInt16: lo = 0.0;           hi = 65535.0;        return true;
    case CMedIOVolume::Int32:  lo = -2147483648.0; hi = 2147483647.0;   return true;
    case CMedIOVolume::UInt32: lo = 0.0;           hi = 4294967295.0;   return true;
    default: break;
  }
  return false;
}

// integer types from the smallest to the largest
const CMedIOVolume::DataType integerTypes[] = { CMedIOVolume::UInt8, CMedIOVolume::Int8,
                                                CMedIOVolume::Int16, CMedIOVolume::UInt16,
                                                CMedIOVolume::Int32, CMedIOVolume::UInt32 };

void setStored(char* data, qint64 i, CMedIOVolume::DataType type, double value)
{
  switch(type) {
    case CMedIOVolume::UInt8:   reinterpret_cast<quint8*>(data)[i] = static_cast<quint8>(value); break;
    case CMedIOVolume::Int8:    reinterpret_cast<qint8*>(data)[i] = static_cast<qint8>(value); break;
    case CMedIOVolume::Int16:   reinterpret_cast<qint16*>(data)[i] = static_cast<qint16>(value); break;
    case CMedIOVolume::UInt16:  reinterpret_cast<quint16*>(data)[i] = static_cast<quint16>(value); break;
    case CMedIOVolume::Int32:   reinterpret_cast<qint32*>(data)[i] = static_cast<qint32>(value); break;
    case CMedIOVolume::UInt32:  reinterpret_cast<quint32*>(data)[i] = static_cast<quint32>(value); break;
    case CMedIOVolume::Float32: reinterpret_cast<float*>(data)[i] = static_cast<float>(value); break;
    case CMedIOVolume::Float64: reinterpret_cast<double*>(data)[i] = value; break;
    case CMedIOVolume::UnknownDataType: break;
  }
}

} // namespace

bool convertStoredValues(CMedIOVolume& v, CMedIOVolume::DataType type)
{
  ENTER();

  if(v.dataType() == type) {
    RETURN(true);
    return true;
  }

  const qint64 voxels = v.voxelCount();
  QByteArray data(voxels * CMedIOVolume::bytesPerVoxel(type), '\0');
  for(qint64 i=0; i < voxels; i++)
    setStored(data.data(), i, type, v.storedValue(i));

  // keep the scaling
  QVector<double> scales(v.dimension(2));
  for(int k=0; k < scales.size(); k++)
    scales[k] = v.scaleFactor(k);
  const double intercept = v.intercept();

  bool result = v.setData(v.dimension(0), v.dimension(1), v.dimension(2), type, data);
  for(int k=0; k < scales.size(); k++)
    v.setScaleFactor(k, scales[k]);
  v.setIntercept(intercept);

  RETURN(result);
  return result;
}

bool fitDataType(CMedIOVolume& v, unsigned allowed, bool planeScales, bool intercept)
{
  ENTER();
  const CMedIOVolume::DataType type = v.dataType();

  bool physical = (intercept == false && v.intercept() != 0.0) ||
                  (planeScales == false && v.hasUniformScaleFactor() == false);

  if(physical == false) {
    if((allowed & typeBit(type)) != 0) {
      RETURN(true);
      return true;
    }

    double lo = 0.0, hi = 0.0;
    if(integerRange(type, lo, hi)) {
      // smallest allowed integer type holding all values of the type ...
      for(unsigned t=0; t < sizeof(integerTypes) / sizeof(integerTypes[0]); t++) {
        double l = 0.0, h = 0.0;
        if((allowed & typeBit(integerTypes[t])) && integerRange(integerTypes[t], l, h) && l <= lo && h >= hi) {
          bool result = convertStoredValues(v, integerTypes[t]);
          RETURN(result);
          return result;
        }
      }

      // ... or all values of the volume
      const qint64 voxels = v.voxelCount();
      lo = std::numeric_limits<double>::max();
      hi = -std::numeric_limits<double>::max();
      for(qint64 i=0; i < voxels; i++) {
        lo = qMin(lo, v.storedValue(i));
        hi = qMax(hi, v.storedValue(i));
      }
      for(unsigned t=0; t < sizeof(integerTypes) / sizeof(integerTypes[0]); t++) {
        double l = 0.0, h = 0.0;
        if((allowed & typeBit(integerTypes[t])) && integerRange(integerTypes[t], l, h) && l <= lo && h >= hi) {
          bool result = convertStoredValues(v, integerTypes[t]);
          RETURN(result);
          return result;
        }
      }
    }
  }

  // physical values as float32 (float64 if the target supports it)
  v.convertToPhysicalValues();
  if(v.dataType() == CMedIOVolume::Float64 && (allowed & typeBit(CMedIOVolume::Float64)) == 0)
    convertStoredValues(v, CMedIOVolume::Float32);

  bool result = (allowed & typeBit(v.dataType())) != 0;
  if(result == false)
    E("the target format does not support the data type %d", static_cast<int>(v.dataType()));

  RETURN(result);
  return result;
}

CMedIOVolume::DataType volumeType(CECATSubHeader::Data_Type type)
{
  switch(type) {
    case CECATSubHeader::ByteData:  return CMedIOVolume::UInt8;
    case CECATSubHeader::VAX_Ix2:
    case CECATSubHeader::SunShort:  return CMedIOVolume::Int16;
    case CECATSubHeader::VAX_Ix4:
    case CECATSubHeader::SunLong:   return CMedIOVolume::Int32;
    case CECATSubHeader::VAX_Rx4:
    case CECATSubHeader::IEEEFloat: return CMedIOVolume::Float32;
    default: break;
  }
  return CMedIOVolume::UnknownDataType;
}

CECATSubHeader::Data_Type ecatType(CMedIOVolume::DataType type)
{
  switch(type) {
    case CMedIOVolume::UInt8:   return CECATSubHeader::ByteData;
    case CMedIOVolume::Int16:   return CECATSubHeader::SunShort;
    case CMedIOVolume::Int32:   return CECATSubHeader::SunLong;
    case CMedIOVolume::Float32: return CECATSubHeader::IEEEFloat;
    default: break;
  }
  return CECATSubHeader::UnknownDataType;
}

//=============================================================================================
// geometry

double ecatCalibration(const CECATMainHeader* mainHeader)
{
  const CECAT7MainHeader* header = dynamic_cast<const CECAT7MainHeader*>(mainHeader);
  const double factor = header ? header->calibration_Factor() : 1.0;

  return (factor > 0.0 && std::isfinite(factor)) ? factor : 1.0;
}

double ecatScale(double scaleFactor)
{
  return (scaleFactor != 0.0 && std::isfinite(scaleFactor)) ? scaleFactor : 1.0;
}

void ecatAffine(const CECAT7MainHeader& mainHeader, const CECAT7SubHeaderImage& subHeader, double m[3][4])
{
  // ECAT7 stores lengths in cm
  const double px = subHeader.x_Pixel_Size() * 10.0;
  const double py = subHeader.y_Pixel_Size() * 10.0;
  const double pz = subHeader.z_Pixel_Size() * 10.0;

  for(int i=0; i < 3; i++)
    for(int j=0; j < 4; j++)
      m[i][j] = 0.0;

  m[0][0] = px;
  m[1][1] = py;
  m[2][2] = -pz;
  m[0][3] = subHeader.x_Offset() * 10.0 - (subHeader.x_Dimension() - 1) / 2.0 * px;
  m[1][3] = subHeader.y_Offset() * 10.0 - (subHeader.y_Dimension() - 1) / 2.0 * py;
  m[2][3] = (mainHeader.init_Bed_Position() + subHeader.z_Offset()) * 10.0;
}

void setVolumeGeometry(CMedIOFrame& f)
{
  double m[3][4];
  ecatAffine(f.mainHeader, f.subHeader, m);
  f.volume.setAffine(m);
  f.volume.setFrameStart(f.subHeader.frame_Start_Time() / 1000.0);
  f.volume.setFrameDuration(f.subHeader.frame_Duration() / 1000.0);
}

bool frameFromVolume(const CMedIOVolume& volume, CMedIOFrame& f)
{
  ENTER();

  f.volume = volume;
  if(volume.isEmpty() || f.volume.reorient("LPI") == false) {
    E("invalid volume");
    RETURN(false);
    return false;
  }

  const CMedIOVolume& v = f.volume;
  const int nx = v.dimension(0), ny = v.dimension(1), nz = v.dimension(2);
  double p[3];
  v.position(0, 0, 0, p);

  // ECAT7 geometry (cm) of the voxel axes (oblique axes: closest axes)
  f.subHeader.setNum_Dimensions(3);
  f.subHeader.setX_Dimension(nx);
  f.subHeader.setY_Dimension(ny);
  f.subHeader.setZ_Dimension(nz);
  f.subHeader.setX_Pixel_Size(v.voxelSize(0) / 10.0);
  f.subHeader.setY_Pixel_Size(v.voxelSize(1) / 10.0);
  f.subHeader.setZ_Pixel_Size(v.voxelSize(2) / 10.0);
  f.subHeader.setX_Offset((p[0] + (nx - 1) / 2.0 * v.voxelSize(0)) / 10.0);
  f.subHeader.setY_Offset((p[1] + (ny - 1) / 2.0 * v.voxelSize(1)) / 10.0);
  f.subHeader.setZ_Offset(0.0f);
  f.subHeader.setRecon_Zoom(1.0f);
  f.subHeader.setFrame_Start_Time(v.frameStart() > 0.0 ? static_cast<unsigned int>(v.frameStart() * 1000.0 + 0.5) : 0);
  f.subHeader.setFrame_Duration(v.frameDuration() > 0.0 ? static_cast<unsigned int>(v.frameDuration() * 1000.0 + 0.5) : 0);

  f.mainHeader.setInit_Bed_Position(p[2] / 10.0);
  f.mainHeader.setNum_Planes(nz);
  f.mainHeader.setNum_Frames(1);
  f.mainHeader.setCalibration_Factor(1.0f);

  RETURN(true);
  return true;
}

CMedIOData::Format headerDataFormat(const CMedIOHeader& header)
{
  switch(header.headerFormat()) {
    case CMedIOHeader::ECATMainHeader:
    case CMedIOHeader::ECATSubHeader:               return CMedIOData::ECAT;
    case CMedIOHeader::NIFTIMainHeader:             return CMedIOData::NIFTI;
    case CMedIOHeader::PhilipsMainHeader:
    case CMedIOHeader::PhilipsSubHeader:            return CMedIOData::Philips;
    case CMedIOHeader::ConcordeMicroPetMainHeader:
    case CMedIOHeader::ConcordeMicroPetFrameHeader: return CMedIOData::ConcordeMicropet;
    default: break;
  }
  return CMedIOData::Unknown;
}

bool mainHeaderFitsMatrix(const CMedIOHeader& subHeader, const CMedIOHeader* mainHeader)
{
  if(mainHeader == NULL)
    return true;

  CMedIOHeader::Format expected = CMedIOHeader::Unknown;
  switch(headerDataFormat(subHeader)) {
    case CMedIOData::ECAT:             expected = CMedIOHeader::ECATMainHeader; break;
    case CMedIOData::NIFTI:            expected = CMedIOHeader::NIFTIMainHeader; break;
    case CMedIOData::Philips:          expected = CMedIOHeader::PhilipsMainHeader; break;
    case CMedIOData::ConcordeMicropet: expected = CMedIOHeader::ConcordeMicroPetMainHeader; break;
    default: break;
  }

  return expected != CMedIOHeader::Unknown && mainHeader->headerFormat() == expected;
}

QList<short> frameNumbers(CMedIOData* file)
{
  QList<short> numbers;
  if(file == NULL || file->isOpen() == false)
    return numbers;

  switch(file->dataFormat()) {
    case CMedIOData::ECAT:             numbers = ecatFrameNumbers(static_cast<CECATFile*>(file)); break;
    case CMedIOData::NIFTI:            numbers = niftiFrameNumbers(static_cast<CNIFTIFile*>(file)); break;
    case CMedIOData::Philips:          numbers = philipsFrameNumbers(static_cast<CPhilipsFile*>(file)); break;
    case CMedIOData::ConcordeMicropet: numbers = concordeFrameNumbers(static_cast<CConcordeFile*>(file)); break;
    default: break;
  }
  return numbers;
}

void fileAttributes(CMedIOData* file, short frame, QJsonObject& mainAttributes, QJsonObject& frameAttributes)
{
  ENTER();
  mainAttributes = QJsonObject();
  frameAttributes = QJsonObject();

  const QList<short> numbers = frameNumbers(file);
  if(frame == -1 && numbers.isEmpty() == false)
    frame = numbers.first();

  CMedIOHeader* mh = NULL;
  if(readNativeMainHeader(file, mh) && mh != NULL)
    mainAttributes = mh->attributes();
  delete mh;

  switch(file->dataFormat()) {
    case CMedIOData::ECAT: {
      CECATSubHeader* sh = NULL;
      if(static_cast<CECATFile*>(file)->readSubHeader(sh, frame) && sh != NULL)
        frameAttributes = sh->attributes();
      delete sh;
    }
    break;

    case CMedIOData::Philips: {
      CPhilipsSubHeader* sh = NULL;
      if(static_cast<CPhilipsFile*>(file)->readSubHeader(sh, frame, -1) && sh != NULL)
        frameAttributes = sh->attributes();
      delete sh;
    }
    break;

    case CMedIOData::ConcordeMicropet: {
      CConcordeFrameHeader* fh = NULL;
      if(static_cast<CConcordeFile*>(file)->readSubHeader(fh, frame) && fh != NULL)
        frameAttributes = fh->attributes();
      delete fh;
    }
    break;

    case CMedIOData::NIFTI: {
      // the frame attributes of all frames are stored with the main header
      const QJsonArray frames = mainAttributes.take(QString::fromLatin1(CMedIOHeaderDictionary::FramesKey)).toArray();
      const int index = numbers.indexOf(frame);
      if(index >= 0 && index < frames.size())
        frameAttributes = frames[index].toObject();
    }
    break;

    default:
    break;
  }

  LEAVE();
}

//=============================================================================================
// dispatch to the format adapters

namespace {

bool unsupported(CMedIOData::Format format, const char* what)
{
  Q_UNUSED(format);
  Q_UNUSED(what);
  E("%s is not supported for data format %d", what, static_cast<int>(format));
  return false;
}

} // namespace

bool readVolume(CMedIOData* file, short frame, CMedIOVolume& volume)
{
  ENTER();
  bool result = false;

  switch(file->dataFormat()) {
    case CMedIOData::NIFTI:
      result = readNiftiVolume(static_cast<CNIFTIFile*>(file), frame, volume);
    break;

    default: {
      // formats storing their images in the ECAT7 voxel order
      CMedIOFrame f;
      result = readFrame(file, frame, f);
      if(result)
        volume = f.volume;
    }
    break;
  }

  RETURN(result);
  return result;
}

bool readFrame(CMedIOData* file, short frame, CMedIOFrame& f)
{
  ENTER();
  bool result = false;

  switch(file->dataFormat()) {
    case CMedIOData::ECAT:
      result = readEcatFrame(static_cast<CECATFile*>(file), frame, f);
    break;

    case CMedIOData::NIFTI:
      result = readNiftiFrame(static_cast<CNIFTIFile*>(file), frame, f);
    break;

    case CMedIOData::Philips:
      result = readPhilipsFrame(static_cast<CPhilipsFile*>(file), frame, f);
    break;

    case CMedIOData::ConcordeMicropet:
      result = readConcordeFrame(static_cast<CConcordeFile*>(file), frame, f);
    break;

    default:
      result = unsupported(file->dataFormat(), "reading matrices in another format");
    break;
  }

  // the representations of the main header give the number of frames, not
  // the highest frame number (ECAT7, Philips)
  if(result)
    f.mainHeader.setNum_Frames(static_cast<short>(frameNumbers(file).size()));

  // attributes of the frame for the targets
  if(result)
    fileAttributes(file, frame, f.mainAttributes, f.frameAttributes);

  RETURN(result);
  return result;
}

bool frameFromMatrix(const QByteArray& data, const CMedIOHeader& header, const CMedIOHeader* mainHeader, CMedIOFrame& f)
{
  ENTER();
  bool result = false;

  switch(headerDataFormat(header)) {
    case CMedIOData::ECAT:
      result = (header.headerFormat() == CMedIOHeader::ECATSubHeader) &&
               ecatFrameFromMatrix(data, static_cast<const CECATSubHeader&>(header), mainHeader, f);
    break;

    case CMedIOData::NIFTI:
      result = niftiFrameFromMatrix(data, static_cast<const CNIFTIMainHeader&>(header), f);
    break;

    case CMedIOData::Philips:
      result = (header.headerFormat() == CMedIOHeader::PhilipsSubHeader) &&
               philipsFrameFromMatrix(data, static_cast<const CPhilipsSubHeader&>(header), mainHeader, f);
    break;

    case CMedIOData::ConcordeMicropet:
      result = (header.headerFormat() == CMedIOHeader::ConcordeMicroPetFrameHeader) &&
               concordeFrameFromMatrix(data, static_cast<const CConcordeFrameHeader&>(header), mainHeader, f);
    break;

    default:
      result = unsupported(headerDataFormat(header), "writing matrices of this format into another format");
    break;
  }

  // attributes of the matrix for the targets (the NIfTI representation of a
  // frame holds the main attributes and the frame in "Frames")
  if(result) {
    if(headerDataFormat(header) == CMedIOData::NIFTI) {
      f.mainAttributes = header.attributes();
      const QJsonArray frames = f.mainAttributes.take(QString::fromLatin1(CMedIOHeaderDictionary::FramesKey)).toArray();
      f.frameAttributes = frames.isEmpty() ? QJsonObject() : frames.first().toObject();
    } else {
      f.frameAttributes = header.attributes();
      f.mainAttributes = mainHeader ? mainHeader->attributes() : QJsonObject();
    }
  }

  RETURN(result);
  return result;
}

bool matrixFromFrame(const CMedIOFrame& f, CMedIOData::Format format, QByteArray& data, CMedIOHeader*& header)
{
  ENTER();
  bool result = false;
  header = NULL;

  switch(format) {
    case CMedIOData::ECAT:
      result = ecatMatrixFromFrame(f, data, header);
    break;

    case CMedIOData::NIFTI:
      result = niftiMatrixFromFrame(f, data, header);
    break;

    case CMedIOData::Philips:
      result = philipsMatrixFromFrame(f, data, header);
    break;

    default:
      result = unsupported(format, "converting matrices into this format");
    break;
  }

  RETURN(result);
  return result;
}

bool mainHeaderFromFrame(const CMedIOFrame& f, CMedIOData::Format format, CMedIOData* source, CMedIOHeader*& header)
{
  ENTER();
  bool result = false;
  header = NULL;

  switch(format) {
    case CMedIOData::ECAT:
      result = ecatMainHeaderFromFrame(f, header);
    break;

    case CMedIOData::NIFTI:
      result = niftiMainHeaderFromFrame(f, source, header);
    break;

    case CMedIOData::Philips:
      result = philipsMainHeaderFromFrame(f, header);
    break;

    default:
      result = unsupported(format, "converting main headers into this format");
    break;
  }

  // attributes of the source (the frame attributes belong to the frames)
  if(result && header != NULL)
    MedIOHeaderMapping::transfer(f.mainAttributes, *header, MedIOHeaderMapping::frameKeys());

  RETURN(result);
  return result;
}

bool writeFrame(CMedIOData* file, short frame, const CMedIOFrame& f)
{
  ENTER();
  bool result = false;

  switch(file->dataFormat()) {
    case CMedIOData::ECAT:
      result = writeEcatFrame(static_cast<CECATFile*>(file), frame, f);
    break;

    case CMedIOData::NIFTI:
      result = writeNiftiFrame(static_cast<CNIFTIFile*>(file), frame, f);
    break;

    case CMedIOData::Philips:
      result = writePhilipsFrame(static_cast<CPhilipsFile*>(file), frame, f);
    break;

    default:
      result = unsupported(file->dataFormat(), "writing matrices of another format");
    break;
  }

  RETURN(result);
  return result;
}

bool readNativeMatrix(CMedIOData* file, short frame, QByteArray& data, CMedIOHeader*& header)
{
  ENTER();
  bool result = false;
  header = NULL;

  switch(file->dataFormat()) {
    case CMedIOData::ECAT: {
      QByteArray* matrix = NULL;
      CECATSubHeader* subHeader = NULL;
      result = static_cast<CECATFile*>(file)->readMatrix(matrix, subHeader, frame);
      if(result) {
        data = *matrix;
        header = subHeader;
      }
      delete matrix;
    }
    break;

    case CMedIOData::ConcordeMicropet: {
      CConcordeFile* concorde = static_cast<CConcordeFile*>(file);
      char* matrix = NULL;
      unsigned int length = 0;
      CConcordeFrameHeader* frameHeader = NULL;
      result = concorde->readMatrix(matrix, length, frameHeader, frame == -1 ? 1 : frame);
      if(result) {
        data = QByteArray(matrix, length);
        header = frameHeader;
      }
      delete[] matrix;
    }
    break;

    case CMedIOData::NIFTI: {
      // one volume in the voxel order and datatype of the file and the header
      // of the file with the timing of the frame
      CMedIOVolume volume;
      CNIFTIMainHeader* mainHeader = NULL;
      result = readNiftiVolume(static_cast<CNIFTIFile*>(file), frame, volume, &mainHeader);
      if(result) {
        setNiftiFrameTiming(mainHeader, volume.frameStart(), volume.frameDuration());
        data = volume.data();
        header = mainHeader;
      }
    }
    break;

    default:
      result = unsupported(file->dataFormat(), "reading matrices");
    break;
  }

  RETURN(result);
  return result;
}

bool readNativeMainHeader(CMedIOData* file, CMedIOHeader*& header)
{
  ENTER();
  bool result = false;
  header = NULL;

  switch(file->dataFormat()) {
    case CMedIOData::ECAT: {
      CECATMainHeader* mainHeader = NULL;
      result = static_cast<CECATFile*>(file)->readMainHeader(mainHeader);
      header = mainHeader;
    }
    break;

    case CMedIOData::NIFTI: {
      CNIFTIMainHeader* mainHeader = NULL;
      result = static_cast<CNIFTIFile*>(file)->readMainHeader(mainHeader);
      header = mainHeader;
    }
    break;

    case CMedIOData::Philips: {
      CPhilipsMainHeader* mainHeader = NULL;
      result = static_cast<CPhilipsFile*>(file)->readMainHeader(mainHeader);
      header = mainHeader;
    }
    break;

    case CMedIOData::ConcordeMicropet: {
      CConcordeMainHeader* mainHeader = NULL;
      result = static_cast<CConcordeFile*>(file)->readMainHeader(mainHeader);
      header = mainHeader;
    }
    break;

    default:
      result = unsupported(file->dataFormat(), "reading main headers");
    break;
  }

  RETURN(result);
  return result;
}

bool writeNativeMatrix(CMedIOData* file, short frame, const QByteArray& data, const CMedIOHeader& header)
{
  ENTER();
  bool result = false;

  switch(file->dataFormat()) {
    case CMedIOData::ECAT:
      result = static_cast<CECATFile*>(file)->writeMatrix(data, static_cast<const CECATSubHeader&>(header),
                                                          ecatWriteFrame(frame));
    break;

    default:
      result = unsupported(file->dataFormat(), "writing matrices");
    break;
  }

  RETURN(result);
  return result;
}

} // namespace MedIOMatrix
