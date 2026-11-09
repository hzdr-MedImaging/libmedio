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

#include "CNIFTIConverter.h"
#include "CMedIOHeaderDictionary.h"
#include "CMedIOHeaderMapping.h"
#include "CNIFTIFile.h"
#include "CNIFTIMainHeader.h"
#include "CNIFTI1MainHeader.h"
#include "CNIFTI2MainHeader.h"
#include "CNIFTIGeometry.h"
#include "CECATFile.h"
#include "CECATMainHeader.h"
#include "CECATSubHeader.h"
#include "CECAT7MainHeader.h"
#include "CECAT7SubHeaderImage.h"
#include "CMedIO.h"
#include "CMedIODataFactory.h"
#include "CMedIOMatrix.h"
#include "CMedIOVolume.h"
#include "CPhilipsFile.h"
#include "CPhilipsMainHeader.h"
#include "CPhilipsSubHeader.h"
#include "CPhilipsSubHeaderImage.h"

#include <QByteArray>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopedPointer>
#include <QVector>

#include <cmath>
#include <cstring>
#include <limits>

#include <rtdebug.h>

namespace {

//=============================================================================================
// helpers

bool fail(QString* error, const QString& message)
{
  if(error)
    *error = message;
  E("%s", message.toLatin1().constData());
  return false;
}

// value of voxel i of an ECAT7 matrix in machine byte order
double ecatValue(const char* data, qint64 i, CECATSubHeader::Data_Type type)
{
  switch(type) {
    case CECATSubHeader::ByteData:  return reinterpret_cast<const quint8*>(data)[i];
    case CECATSubHeader::VAX_Ix4:
    case CECATSubHeader::SunLong:   return reinterpret_cast<const qint32*>(data)[i];
    case CECATSubHeader::VAX_Rx4:
    case CECATSubHeader::IEEEFloat: return reinterpret_cast<const float*>(data)[i];
    default:                        return reinterpret_cast<const qint16*>(data)[i];
  }
}

int ecatBytesPerVoxel(CECATSubHeader::Data_Type type)
{
  switch(type) {
    case CECATSubHeader::ByteData:  return 1;
    case CECATSubHeader::VAX_Ix2:
    case CECATSubHeader::SunShort:  return 2;
    case CECATSubHeader::VAX_Ix4:
    case CECATSubHeader::SunLong:
    case CECATSubHeader::VAX_Rx4:
    case CECATSubHeader::IEEEFloat: return 4;
    default:                        return 0;
  }
}

// bytes per voxel of the NIfTI datatypes the converter reads (0: unsupported)
int niftiBytesPerVoxel(int datatype)
{
  switch(datatype) {
    case 2:   // DT_UINT8
    case 256: // DT_INT8
      return 1;
    case 4:   // DT_INT16
    case 512: // DT_UINT16
      return 2;
    case 8:   // DT_INT32
    case 16:  // DT_FLOAT32
    case 768: // DT_UINT32
      return 4;
    case 64:  // DT_FLOAT64
      return 8;
  }
  return 0;
}

// stored value of voxel i of a NIfTI matrix in machine byte order
double niftiValue(const char* data, qint64 i, int datatype)
{
  switch(datatype) {
    case 2:   return reinterpret_cast<const quint8*>(data)[i];
    case 256: return reinterpret_cast<const qint8*>(data)[i];
    case 4:   return reinterpret_cast<const qint16*>(data)[i];
    case 512: return reinterpret_cast<const quint16*>(data)[i];
    case 8:   return reinterpret_cast<const qint32*>(data)[i];
    case 768: return reinterpret_cast<const quint32*>(data)[i];
    case 16:  return reinterpret_cast<const float*>(data)[i];
    default:  return reinterpret_cast<const double*>(data)[i];
  }
}

// header values used by the converter for both NIfTI versions
struct NIFTIInfo
{
  qint64 dim[8];
  int datatype;
  double slope;
  double inter;
};

template <class H>
NIFTIInfo niftiInfo(const H* h)
{
  NIFTIInfo info;
  for(short i=0; i < 8; i++)
    info.dim[i] = h->dim(i);
  info.datatype = h->dataType();
  info.slope = h->scl_Slope();
  info.inter = h->scl_Inter();

  // scl_slope = 0 means no scaling
  if(info.slope == 0.0 || std::isfinite(info.slope) == false) {
    info.slope = 1.0;
    info.inter = 0.0;
  }
  if(std::isfinite(info.inter) == false)
    info.inter = 0.0;

  return info;
}

// adjust the NIfTI header written by ecatToNifti()
template <class H>
void setNIFTIData(H* h, bool floatData, double slope, double minValue, double maxValue, double duration)
{
  if(floatData) {
    h->setDataType(NIFTI_DT_FLOAT32);
    h->setBit_Pix(32);
    h->setScl_Slope(1.0);
  } else {
    h->setScl_Slope(slope);
  }
  h->setScl_Inter(0.0);

  h->setCal_Min(minValue);
  h->setCal_Max(maxValue);
  h->setPix_Dim(4, duration);
  h->setDescrip("converted by libmedio");
}

//=============================================================================================
// Image data in the ECAT7 representation, which is the common intermediate
// format of all conversions: the ECAT7 main header, one image sub header and
// the stored voxel values (ECAT7 voxel order, machine byte order) per frame
// and the factor converting the stored values of every plane into physical
// values (incl. the calibration factor).

struct EcatImage
{
  EcatImage() : ecatFile(CECATMainHeader::ECAT7_Volume16), mainHeader(NULL) { }
  ~EcatImage() { delete mainHeader; qDeleteAll(subHeaders); }

  QScopedPointer<CMedIOData> source;        // file the headers were read from
  CECATFile ecatFile;                       // owner of headers created here (never opened)
  CECAT7MainHeader* mainHeader;
  QList<CECAT7SubHeaderImage*> subHeaders;  // one per frame
  QList<QByteArray> matrices;               // stored values per frame
  QList<QVector<double> > planeScales;      // physical value = stored value x planeScales[frame][plane]

  // format independent header attributes of the source (CMedIOHeader::attributes()),
  // given to the headers of the target
  QJsonObject mainAttributes;                 // main header
  QList<QJsonObject> frameAttributes;         // per frame (sub header, first slice, NIfTI "Frames" entry)
};

//---------------------------------------------------------------------------------------------
// ECAT7 file -> EcatImage

bool loadEcat(const QString& fileName, EcatImage& image, QString* error)
{
  CECATFile* ecatFile = new CECATFile(fileName);
  image.source.reset(ecatFile);

  if(ecatFile->open(QIODevice::ReadOnly) == false)
    return fail(error, "unable to open " + fileName + " as ECAT file");

  CECATMainHeader* mainHeader = NULL;
  if(ecatFile->readMainHeader(mainHeader) == false || mainHeader == NULL)
    return fail(error, "unable to read the ECAT main header of " + fileName);

  if(mainHeader->mainHeaderType() != CECATMainHeader::ECAT7MainHeader) {
    delete mainHeader;
    return fail(error, "only ECAT7 files can be converted to NIfTI");
  }
  image.mainHeader = static_cast<CECAT7MainHeader*>(mainHeader);

  // read all frames, by the frame numbers of the file (they do not have to
  // start with 1, num_Frames is not used)
  const QList<short> frames = ecatFile->frameNumbers();
  if(frames.isEmpty())
    return fail(error, "no frames found in " + fileName);
  const double calibration = ecatCalibrationFactor(image.mainHeader);

  foreach(short f, frames) {
    CECATSubHeader* subHeader = NULL;
    QByteArray* matrix = NULL;

    if(ecatFile->readMatrix(matrix, subHeader, f) == false || matrix == NULL || subHeader == NULL) {
      delete matrix;
      delete subHeader;
      return fail(error, QString("unable to read frame %1 of %2").arg(f).arg(fileName));
    }

    image.matrices.append(*matrix);
    delete matrix;

    if(subHeader->subHeaderType() != CECATSubHeader::ECAT7_Image) {
      delete subHeader;
      return fail(error, "only ECAT7 image data can be converted to NIfTI");
    }

    CECAT7SubHeaderImage* s = static_cast<CECAT7SubHeaderImage*>(subHeader);
    image.subHeaders.append(s);
    image.frameAttributes.append(s->attributes());

    double scale = s->scale_Factor();
    if(scale == 0.0 || std::isfinite(scale) == false)
      scale = 1.0;
    image.planeScales.append(QVector<double>(qMax(0, static_cast<int>(s->z_Dimension())), scale * calibration));
  }

  image.mainAttributes = image.mainHeader->attributes();

  ecatFile->close();
  return true;
}

//---------------------------------------------------------------------------------------------
// Philips image/Syntegra file -> EcatImage
//
// The headers are converted with the existing Philips -> ECAT7 header
// conversions. Every slice of a Philips file has its own sub header and
// scale factor (CPhilipsSubHeaderImage::ecat_Scale_Factor()). The image
// positions of the slices (img_pos_x/y) are not used, the position along
// z is given by the bed position (see CNIFTIConverter.h).

bool loadPhilips(const QString& fileName, EcatImage& image, QString* error)
{
  CPhilipsFile* file = new CPhilipsFile(fileName);
  image.source.reset(file);

  if(file->open(QIODevice::ReadOnly) == false)
    return fail(error, "unable to open " + fileName + " as Philips file");

  CPhilipsMainHeader* mainHeader = NULL;
  if(file->readMainHeader(mainHeader) == false || mainHeader == NULL)
    return fail(error, "unable to read the Philips main header of " + fileName);
  QScopedPointer<CPhilipsMainHeader> mainHeaderGuard(mainHeader);

  if(mainHeader->filtyp() != CPhilipsMainHeader::Image && mainHeader->filtyp() != CPhilipsMainHeader::Syntegra)
    return fail(error, "only Philips image and Syntegra files can be converted to NIfTI");

  // the frame numbers of the file (they do not have to start with 1)
  const QList<short> frameNumbers = file->frameNumbers();
  const int frames = frameNumbers.size();
  if(frames < 1)
    return fail(error, "no frames found in " + fileName);

  int nx = 0, ny = 0, nz = 0;
  int dataType = CMedIOVolume::UnknownDataType;
  QScopedPointer<CPhilipsSubHeader> firstSubHeader;
  bool unscaled = false;

  for(int f=0; f < frames; f++) {
    const short frame = frameNumbers[f];

    // all slices of the frame (see CPhilipsMatrix.cpp)
    CMedIOVolume volume;
    CPhilipsSubHeader* first = NULL;
    bool frameUnscaled = false;
    if(MedIOMatrix::readPhilipsFrameData(file, mainHeader, frame, volume, first, &frameUnscaled) == false)
      return fail(error, QString("unable to read frame %1 of %2").arg(frame).arg(fileName));
    QScopedPointer<CPhilipsSubHeader> frameSubHeader(first);
    unscaled = unscaled || frameUnscaled;

    if(f == 0) {
      nx = volume.dimension(0);
      ny = volume.dimension(1);
      nz = volume.dimension(2);
      dataType = volume.dataType();
    } else if(volume.dimension(2) != nz) {
      return fail(error, "the frames of the Philips file differ in their number of slices");
    } else if(volume.dimension(0) != nx || volume.dimension(1) != ny) {
      return fail(error, "the slices of the Philips file differ in their dimensions");
    } else if(volume.dataType() != dataType) {
      return fail(error, "the slices of the Philips file differ in their data type");
    }

    // stored values as ECAT7 data type: unsigned 16 bit values are kept as 32 bit integers
    if(volume.dataType() == CMedIOVolume::UInt16)
      MedIOMatrix::convertStoredValues(volume, CMedIOVolume::Int32);

    QVector<double> scales(nz);
    for(int k=0; k < nz; k++)
      scales[k] = volume.scaleFactor(k);

    // ECAT7 image sub header of this frame from the sub header of its first slice
    CECAT7SubHeaderImage* ecatSubHeader = new CECAT7SubHeaderImage(&image.ecatFile);
    image.subHeaders.append(ecatSubHeader);
    if(ecatSubHeader->convertFrom(frameSubHeader.data(), mainHeader) == false)
      return fail(error, "unable to convert the Philips sub header of " + fileName);

    ecatSubHeader->setZ_Dimension(nz);
    ecatSubHeader->setData_Type(MedIOMatrix::ecatType(volume.dataType()));
    ecatSubHeader->setScale_Factor(scales.first());

    image.matrices.append(volume.data());
    image.planeScales.append(scales);
    image.frameAttributes.append(frameSubHeader->attributes());

    if(f == 0)
      firstSubHeader.reset(frameSubHeader.take());
  }

  if(unscaled)
    W("%s: no valid scale factor for some slices, their stored values are used", fileName.toLocal8Bit().constData());

  // ECAT7 main header (the values are converted into physical values already)
  image.mainHeader = new CECAT7MainHeader(&image.ecatFile, CECATMainHeader::ECAT7_Volume16);
  if(image.mainHeader->convertFrom(mainHeader, firstSubHeader.data()) == false)
    return fail(error, "unable to convert the Philips main header of " + fileName);

  image.mainHeader->setNum_Frames(frames);
  image.mainHeader->setNum_Planes(nz);
  image.mainHeader->setCalibration_Factor(1.0f);

  // the ECAT7 field holds 31 characters only: use the file name without its path
  image.mainHeader->setOriginal_File_Name(QFileInfo(fileName).fileName().left(31).toLatin1().constData());

  image.mainAttributes = mainHeader->attributes();

  file->close();
  return true;
}

//---------------------------------------------------------------------------------------------
// EcatImage -> NIfTI file

bool saveNifti(const EcatImage& image, const QString& niftiFileName, const CNIFTIConverter::Options& options, QString* error)
{
  const int frames = image.subHeaders.size();
  if(frames < 1 || image.matrices.size() != frames || image.planeScales.size() != frames)
    return fail(error, "no image data to convert");

  const CECAT7SubHeaderImage* first = image.subHeaders.first();
  const int nx = first->x_Dimension();
  const int ny = first->y_Dimension();
  const int nz = first->z_Dimension();
  const qint64 voxels = static_cast<qint64>(nx) * ny * nz;
  const qint64 planeVoxels = static_cast<qint64>(nx) * ny;

  if(nx < 1 || ny < 1 || nz < 1)
    return fail(error, "invalid image dimensions");

  // keep the stored values if all planes of all frames share data type and scale factor
  bool storedValues = (options.floatData == false);
  bool sameDuration = true;
  const double slope = image.planeScales.first().isEmpty() ? 1.0 : image.planeScales.first().first();

  for(int f=0; f < frames; f++) {
    const CECAT7SubHeaderImage* s = image.subHeaders[f];

    if(s->x_Dimension() != nx || s->y_Dimension() != ny || s->z_Dimension() != nz || image.planeScales[f].size() != nz)
      return fail(error, "the frames differ in their dimensions");

    if(ecatBytesPerVoxel(s->data_Type()) == 0)
      return fail(error, QString("unsupported ECAT7 data type %1").arg(s->data_Type()));

    if(image.matrices[f].size() < voxels * ecatBytesPerVoxel(s->data_Type()))
      return fail(error, QString("matrix of frame %1 is too small").arg(f + 1));

    if(s->data_Type() != first->data_Type())
      storedValues = false;
    foreach(double scale, image.planeScales[f]) {
      if(scale != slope)
        storedValues = false;
    }

    if(s->frame_Duration() != first->frame_Duration())
      sameDuration = false;
  }

  const int bytesPerVoxel = storedValues ? ecatBytesPerVoxel(first->data_Type()) : 4;
  QByteArray data(voxels * frames * bytesPerVoxel, '\0');
  double minValue = std::numeric_limits<double>::max();
  double maxValue = -std::numeric_limits<double>::max();

  for(int f=0; f < frames; f++) {
    const CECAT7SubHeaderImage* s = image.subHeaders[f];
    const char* src = image.matrices[f].constData();
    char* dst = data.data() + f * voxels * bytesPerVoxel;

    // NIfTI stores the planes in reversed order (see CNIFTIGeometry.h)
    for(int k=0; k < nz; k++) {
      const qint64 srcPlane = k * planeVoxels;
      const qint64 dstPlane = (nz - 1 - k) * planeVoxels;
      const double scale = image.planeScales[f][k];

      if(storedValues) {
        // the matrices are in machine byte order
        memcpy(dst + dstPlane * bytesPerVoxel, src + srcPlane * bytesPerVoxel, planeVoxels * bytesPerVoxel);
      }

      for(qint64 i=0; i < planeVoxels; i++) {
        const double value = ecatValue(src, srcPlane + i, s->data_Type()) * scale;

        if(storedValues == false)
          reinterpret_cast<float*>(dst)[dstPlane + i] = static_cast<float>(value);

        minValue = qMin(minValue, value);
        maxValue = qMax(maxValue, value);
      }
    }
  }

  // write the NIfTI file
  CNIFTIFile niftiFile(niftiFileName, options.niftiVersion == 2 ? CNIFTIMainHeader::NIFTI2 : CNIFTIMainHeader::NIFTI1);
  if(niftiFile.open(QIODevice::WriteOnly) == false)
    return fail(error, "unable to open " + niftiFileName + " for writing");

  QScopedPointer<CNIFTIMainHeader> header(niftiFile.createEmptyHeader());
  bool result = (header.data() != NULL) && header->convertFrom(image.mainHeader, first);

  if(result) {
    const double duration = sameDuration ? first->frame_Duration() / 1000.0 : 0.0;

    if(options.niftiVersion == 2)
      setNIFTIData(static_cast<CNIFTI2MainHeader*>(header.data()), storedValues == false, slope, minValue, maxValue, duration);
    else
      setNIFTIData(static_cast<CNIFTI1MainHeader*>(header.data()), storedValues == false, slope, minValue, maxValue, duration);

    // attributes of the source, the frame attributes of all frames and the frame
    // timing in s (BIDS PET naming)
    MedIOHeaderMapping::transfer(image.mainAttributes, *header, MedIOHeaderMapping::frameKeys());
    QJsonObject json;
    QJsonArray frameAttributes, start, durations;
    bool anyFrameAttributes = false;
    for(int f=0; f < frames; f++) {
      const QJsonObject m = (f < image.frameAttributes.size()) ? image.frameAttributes[f] : QJsonObject();
      frameAttributes.append(m);
      anyFrameAttributes = anyFrameAttributes || (m.isEmpty() == false);
      start.append(image.subHeaders[f]->frame_Start_Time() / 1000.0);
      durations.append(image.subHeaders[f]->frame_Duration() / 1000.0);
    }
    if(anyFrameAttributes)
      json[CMedIOHeaderDictionary::FramesKey] = frameAttributes;
    json["FrameTimesStart"] = start;
    json["FrameDuration"] = durations;
    json["ConversionSoftware"] = QString("libmedio %1").arg(CMedIO::version());
    result = header->updateAttributes(json);

    // the attributes as stored in the file (see CNIFTIFile::writeMainHeader())
    const QJsonObject stored = MedIOHeaderMapping::niftiJson(header->attributes());

    // without header extension: remove the attributes from the header
    if(result && options.jsonExtension == false) {
      QJsonObject removed;
      foreach(const QString& key, header->attributes().keys()) {
        if(key != "NIfTI")
          removed[key] = QJsonValue(QJsonValue::Null);
      }
      result = header->updateAttributes(removed);
    }

    // the main header with the attributes as JSON header extension
    if(result)
      result = niftiFile.writeMainHeader(*header);

    if(result)
      result = niftiFile.writeMatrix(data);

    if(result && options.jsonSidecar) {
      QFile sidecar(niftiSidecarFileName(niftiFileName));
      result = sidecar.open(QIODevice::WriteOnly | QIODevice::Truncate) &&
               sidecar.write(QJsonDocument(stored).toJson(QJsonDocument::Indented)) > 0;
    }
  }

  niftiFile.close();

  if(result == false) {
    // single file or both files of a header/image pair
    QFile::remove(niftiFile.headerFileName());
    QFile::remove(niftiFile.imageFileName());
    return fail(error, "unable to write " + niftiFileName);
  }

  return true;
}

//---------------------------------------------------------------------------------------------
// NIfTI file -> EcatImage (16 bit integers with a scale factor per frame)

bool loadNifti(const QString& niftiFileName, EcatImage& image, QString* error)
{
  CNIFTIFile niftiFile(niftiFileName);
  if(niftiFile.open(QIODevice::ReadOnly) == false)
    return fail(error, "unable to open " + niftiFileName + " as NIfTI file");

  CNIFTIMainHeader* headerPtr = NULL;
  if(niftiFile.readMainHeader(headerPtr) == false || headerPtr == NULL)
    return fail(error, "unable to read the NIfTI header of " + niftiFileName);
  QScopedPointer<CNIFTIMainHeader> header(headerPtr);

  const bool nifti1 = (header->mainHeaderType() == CNIFTIMainHeader::NIFTI1MainHeader);
  const NIFTIInfo info = nifti1 ? niftiInfo(static_cast<const CNIFTI1MainHeader*>(headerPtr))
                                : niftiInfo(static_cast<const CNIFTI2MainHeader*>(headerPtr));

  ECATGeometryFromNIFTI geometry;
  QString geometryError;
  if(ecatGeometryFromNIFTI(headerPtr, geometry, &geometryError) == false)
    return fail(error, geometryError);
  if(geometry.oblique)
    W("%s: oblique NIfTI orientation, the voxel axes are assigned to the closest patient axes", niftiFileName.toLatin1().constData());

  for(int i=5; i <= info.dim[0]; i++) {
    if(info.dim[i] > 1)
      return fail(error, "NIfTI data with more than 4 dimensions cannot be converted");
  }

  const int bytesPerVoxel = niftiBytesPerVoxel(info.datatype);
  if(bytesPerVoxel == 0)
    return fail(error, QString("unsupported NIfTI datatype %1").arg(info.datatype));

  QByteArray* matrixPtr = NULL;
  if(niftiFile.readMatrix(matrixPtr) == false || matrixPtr == NULL)
    return fail(error, "unable to read the voxel data of " + niftiFileName);
  QScopedPointer<QByteArray> matrix(matrixPtr);

  // metadata: JSON header extension or JSON sidecar file
  const QJsonObject json = niftiMetadata(niftiFile, headerPtr, niftiFileName);
  niftiFile.close();

  const int frames = geometry.frames;
  const int nx = geometry.dims[0], ny = geometry.dims[1], nz = geometry.dims[2];
  const qint64 voxels = static_cast<qint64>(nx) * ny * nz;
  const qint64 dimNifti[3] = { info.dim[1], info.dim[2], info.dim[3] };

  // keep the stored values if they are 8/16 bit integers without intercept
  const bool storedValues = (info.datatype == 2 || info.datatype == 256 || info.datatype == 4) && info.inter == 0.0;

  image.mainHeader = new CECAT7MainHeader(&image.ecatFile, CECATMainHeader::ECAT7_Volume16);
  if(image.mainHeader->convertFrom(headerPtr, NULL) == false)
    return fail(error, "unable to convert the NIfTI header of " + niftiFileName);

  // attributes of the NIfTI file (incl. those of CNIFTIConverter of earlier
  // versions, see CMedIOHeaderMapping.h)
  image.mainAttributes = header->attributes();
  const QJsonArray frameAttributes = image.mainAttributes.take(QString::fromLatin1(CMedIOHeaderDictionary::FramesKey)).toArray();
  MedIOHeaderMapping::transfer(image.mainAttributes, *image.mainHeader, MedIOHeaderMapping::frameKeys());

  // the values already contain the calibration factor
  image.mainHeader->setCalibration_Factor(1.0f);

  for(int f=0; f < frames; f++) {
    // reorder the voxels of this frame into the ECAT7 voxel order
    QVector<double> values(voxels);
    double maxAbs = 0.0;

    for(int k=0; k < nz; k++) {
      for(int j=0; j < ny; j++) {
        for(int i=0; i < nx; i++) {
          const qint64 index = niftiVoxelIndex(geometry, dimNifti, i, j, k) + voxels * f;
          double value = niftiValue(matrix->constData(), index, info.datatype);
          if(storedValues == false)
            value = value * info.slope + info.inter;

          values[i + nx * (j + static_cast<qint64>(ny) * k)] = value;
          maxAbs = qMax(maxAbs, std::fabs(value));
        }
      }
    }

    // 16 bit integers with a scale factor per frame
    double scale = info.slope;
    if(storedValues == false)
      scale = (maxAbs > 0.0) ? maxAbs / 32767.0 : 1.0;

    QByteArray data(voxels * sizeof(qint16), '\0');
    qint16* dst = reinterpret_cast<qint16*>(data.data());
    qint16 minStored = std::numeric_limits<qint16>::max();
    qint16 maxStored = std::numeric_limits<qint16>::min();

    for(qint64 i=0; i < voxels; i++) {
      const double stored = storedValues ? values[i] : std::floor(values[i] / scale + 0.5);
      dst[i] = static_cast<qint16>(qBound(-32768.0, stored, 32767.0));
      minStored = qMin(minStored, dst[i]);
      maxStored = qMax(maxStored, dst[i]);
    }

    CECAT7SubHeaderImage* subHeader = new CECAT7SubHeaderImage(&image.ecatFile);
    image.subHeaders.append(subHeader);
    if(subHeader->convertFrom(NULL, headerPtr) == false)
      return fail(error, "unable to convert the NIfTI header of " + niftiFileName);

    // attributes of the frame
    image.frameAttributes.append(f < frameAttributes.size() ? frameAttributes[f].toObject() : QJsonObject());
    MedIOHeaderMapping::transfer(image.frameAttributes.last(), *subHeader);

    subHeader->setData_Type(CECATSubHeader::SunShort);
    subHeader->setScale_Factor(scale);
    subHeader->setImage_Min(minStored);
    subHeader->setImage_Max(maxStored);

    // frame timing from the metadata, the NIfTI header otherwise (s -> ms)
    double start = 0.0, duration = 0.0;
    niftiFrameTiming(geometry, json, f, start, duration);
    subHeader->setFrame_Start_Time(ecatFrameTime(start));
    subHeader->setFrame_Duration(ecatFrameTime(duration));

    // reconstruction zoom written by earlier versions
    if(json.contains("Recon_Zoom"))
      subHeader->setRecon_Zoom(json["Recon_Zoom"].toDouble());

    image.matrices.append(data);
    image.planeScales.append(QVector<double>(nz, scale));
  }

  return true;
}

//---------------------------------------------------------------------------------------------
// EcatImage (16 bit integers) -> ECAT7 file

bool saveEcat(EcatImage& image, const QString& ecatFileName, QString* error)
{
  CECATFile ecatFile(ecatFileName, CECATMainHeader::ECAT7_Volume16);
  if(ecatFile.open(QIODevice::WriteOnly) == false)
    return fail(error, "unable to open " + ecatFileName + " for writing");

  // the main header is saved to the file it is linked to
  image.mainHeader->setMedIOData(&ecatFile);
  bool result = ecatFile.writeMainHeader(*image.mainHeader);

  for(int f=0; f < image.subHeaders.size() && result; f++) {
    result = ecatFile.writeSubHeader(*image.subHeaders[f], f + 1) &&
             ecatFile.writeMatrix(image.matrices[f], f + 1);
  }

  ecatFile.close();
  image.mainHeader->setMedIOData(&image.ecatFile);

  if(result == false) {
    QFile::remove(ecatFileName);
    return fail(error, "unable to write " + ecatFileName);
  }

  return true;
}

//---------------------------------------------------------------------------------------------
// EcatImage (16 bit integers) -> Philips image/Syntegra file
//
// The headers are converted with the existing ECAT7 -> Philips header
// conversions (scale factor as suvscl for calibrated data, as imgscl
// otherwise); every plane is written as slice with its own sub header. Image
// files number their slices from 0, Syntegra files from 1 (like ROVER).

bool savePhilips(EcatImage& image, const QString& philipsFileName, CPhilipsMainHeader::File_Type type, QString* error)
{
  CPhilipsFile file(philipsFileName, type);
  if(file.open(QIODevice::WriteOnly) == false)
    return fail(error, "unable to open " + philipsFileName + " for writing");

  QScopedPointer<CPhilipsMainHeader> mainHeader(file.createEmptyMainHeader());
  bool result = (mainHeader.data() != NULL) &&
                mainHeader->convertFrom(image.mainHeader, image.subHeaders.first());

  // attributes of the source (Philips values that ECAT7 cannot store, too)
  if(result)
    MedIOHeaderMapping::transfer(image.mainAttributes, *mainHeader, MedIOHeaderMapping::frameKeys());

  if(result) {
    mainHeader->setFiltyp(type);

    const int firstSlice = (type == CPhilipsMainHeader::Syntegra) ? 1 : 0;
    const int sliceStep = qMax(static_cast<short>(1), mainHeader->slcthk());

    for(int f=0; f < image.subHeaders.size() && result; f++) {
      const CECAT7SubHeaderImage* ecatSubHeader = image.subHeaders[f];
      const int nx = ecatSubHeader->x_Dimension();
      const int ny = ecatSubHeader->y_Dimension();
      const int nz = ecatSubHeader->z_Dimension();
      const qint64 planeVoxels = static_cast<qint64>(nx) * ny;

      if(ecatSubHeader->data_Type() != CECATSubHeader::SunShort ||
         image.matrices[f].size() < planeVoxels * nz * 2) {
        result = false;
        break;
      }

      QScopedPointer<CPhilipsSubHeader> subHeaderPtr(file.createEmptySubHeader());
      CPhilipsSubHeaderImage* subHeader = static_cast<CPhilipsSubHeaderImage*>(subHeaderPtr.data());
      result = (subHeader != NULL) && subHeader->convertFrom(ecatSubHeader, image.mainHeader);
      if(result && f < image.frameAttributes.size())
        MedIOHeaderMapping::transfer(image.frameAttributes[f], *subHeader);

      for(int k=0; k < nz && result; k++) {
        const qint16* plane = reinterpret_cast<const qint16*>(image.matrices[f].constData()) + k * planeVoxels;
        qint16 minValue = std::numeric_limits<qint16>::max();
        qint16 maxValue = std::numeric_limits<qint16>::min();
        double sum = 0.0;
        for(qint64 i=0; i < planeVoxels; i++) {
          minValue = qMin(minValue, plane[i]);
          maxValue = qMax(maxValue, plane[i]);
          sum += plane[i];
        }

        subHeader->setImgmin(minValue);
        subHeader->setImgmax(maxValue);
        subHeader->setImgsum(sum);

        const int slice = firstSlice + k * sliceStep;
        subHeader->setSlcnum(slice);
        result = file.writeMatrix(reinterpret_cast<const char*>(plane), planeVoxels * 2, *subHeader, f + 1, slice);
      }
    }

    if(result)
      result = file.writeMainHeader(*mainHeader);
  }

  file.close();

  if(result == false) {
    QFile::remove(philipsFileName);
    return fail(error, "unable to write " + philipsFileName);
  }

  return true;
}

// Philips file type by the file name (.syn: Syntegra, image otherwise)
CPhilipsMainHeader::File_Type philipsFileType(const QString& fileName)
{
  return fileName.endsWith(".syn", Qt::CaseInsensitive) ? CPhilipsMainHeader::Syntegra : CPhilipsMainHeader::Image;
}

} // namespace

//=============================================================================================
// public interface

bool CNIFTIConverter::ecatToNifti(const QString& ecatFileName, const QString& niftiFileName,
                                  const Options& options, QString* error)
{
  ENTER();
  EcatImage image;
  bool result = loadEcat(ecatFileName, image, error) &&
                saveNifti(image, niftiFileName, options, error);
  RETURN(result);
  return result;
}

bool CNIFTIConverter::philipsToNifti(const QString& philipsFileName, const QString& niftiFileName,
                                     const Options& options, QString* error)
{
  ENTER();
  EcatImage image;
  bool result = loadPhilips(philipsFileName, image, error) &&
                saveNifti(image, niftiFileName, options, error);
  RETURN(result);
  return result;
}

bool CNIFTIConverter::toNifti(const QString& fileName, const QString& niftiFileName,
                              const Options& options, QString* error)
{
  ENTER();
  bool result = false;

  switch(CMedIODataFactory::identify(fileName)) {
    case CMedIOData::ECAT:
      result = ecatToNifti(fileName, niftiFileName, options, error);
    break;

    case CMedIOData::Philips:
      result = philipsToNifti(fileName, niftiFileName, options, error);
    break;

    default:
      result = fail(error, fileName + " is neither an ECAT7 nor a Philips image file");
    break;
  }

  RETURN(result);
  return result;
}

bool CNIFTIConverter::niftiToEcat(const QString& niftiFileName, const QString& ecatFileName, QString* error)
{
  ENTER();
  EcatImage image;
  bool result = loadNifti(niftiFileName, image, error) &&
                saveEcat(image, ecatFileName, error);
  RETURN(result);
  return result;
}

bool CNIFTIConverter::niftiToPhilips(const QString& niftiFileName, const QString& philipsFileName, QString* error)
{
  ENTER();
  EcatImage image;
  bool result = loadNifti(niftiFileName, image, error) &&
                savePhilips(image, philipsFileName, philipsFileType(philipsFileName), error);
  RETURN(result);
  return result;
}
