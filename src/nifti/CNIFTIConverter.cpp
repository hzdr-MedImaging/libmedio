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

template <class H>
bool writeNIFTIExtension(H* h, CNIFTIFile& file, const QJsonObject& json)
{
  return h->writeHeaderExtension(file, json);
}

//=============================================================================================
// ECAT7 metadata <-> JSON

QJsonObject ecatMetadata(const CECAT7MainHeader* mainHeader, const QList<CECAT7SubHeaderImage*>& subHeaders)
{
  QJsonObject json;

  json["Original_File_Name"] = QString(mainHeader->original_File_Name());
  json["Study_Description"] = QString(mainHeader->study_Description());
  json["Physician_Name"] = QString(mainHeader->physician_Name());
  json["Operator_Name"] = QString(mainHeader->operator_Name());
  json["Facility_Name"] = QString(mainHeader->facility_Name());
  json["Study_Type"] = QString(mainHeader->study_Type());
  json["Patient_Name"] = QString(mainHeader->patient_Name());
  json["Patient_ID"] = QString(mainHeader->patient_ID());
  json["Patient_Sex"] = mainHeader->patient_Sex();
  json["Patient_Age"] = static_cast<double>(mainHeader->patient_Age());
  json["Patient_Weight"] = static_cast<double>(mainHeader->patient_Weight());
  json["Patient_Height"] = static_cast<double>(mainHeader->patient_Height());
  json["Patient_Birth_Date"] = static_cast<double>(mainHeader->patient_Birth_Date());
  json["Patient_Orientation"] = mainHeader->patient_Orientation();
  json["Isotope_Name"] = QString(mainHeader->isotope_Name());
  json["Isotope_Halflife"] = static_cast<double>(mainHeader->isotope_Halflife());
  json["Radiopharmaceutical"] = QString(mainHeader->radiopharmaceutical());
  json["Dosage"] = static_cast<double>(mainHeader->dosage());
  json["Data_Units"] = QString(mainHeader->data_Units());
  json["Calibration_Factor"] = static_cast<double>(mainHeader->calibration_Factor());
  json["Scan_Start_Time"] = static_cast<double>(mainHeader->scan_Start_Time());
  json["Dose_Start_Time"] = static_cast<double>(mainHeader->dose_Start_Time());
  json["Calibration_Units"] = static_cast<int>(mainHeader->calibration_Units());
  json["Calibration_Units_Label"] = static_cast<int>(mainHeader->calibration_Units_Label());
  json["Acquisition_Type"] = static_cast<int>(mainHeader->acquisition_Type());
  json["Lwr_True_Thres"] = static_cast<int>(mainHeader->lwr_True_Thres());
  json["Upr_True_Thres"] = static_cast<int>(mainHeader->upr_True_Thres());
  json["Lwr_Sctr_Thres"] = static_cast<int>(mainHeader->lwr_Sctr_Thres());
  json["Init_Bed_Position"] = static_cast<double>(mainHeader->init_Bed_Position());
  json["Distance_Scanned"] = static_cast<double>(mainHeader->distance_Scanned());
  json["Transaxial_FOV"] = static_cast<double>(mainHeader->transaxial_FOV());
  json["Bin_Size"] = static_cast<double>(mainHeader->bin_Size());

  if(subHeaders.isEmpty() == false)
    json["Recon_Zoom"] = static_cast<double>(subHeaders.first()->recon_Zoom());

  // frame timing in s (BIDS PET naming)
  QJsonArray start, duration;
  foreach(const CECAT7SubHeaderImage* subHeader, subHeaders) {
    start.append(subHeader->frame_Start_Time() / 1000.0);
    duration.append(subHeader->frame_Duration() / 1000.0);
  }
  json["FrameTimesStart"] = start;
  json["FrameDuration"] = duration;

  json["ConversionSoftware"] = QString("libmedio %1").arg(CMedIO::version());

  return json;
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

  // read all frames
  const int frames = (image.mainHeader->num_Frames() > 1) ? image.mainHeader->num_Frames() : 1;
  const double calibration = ecatCalibrationFactor(image.mainHeader);

  for(int f=1; f <= frames; f++) {
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

    double scale = s->scale_Factor();
    if(scale == 0.0 || std::isfinite(scale) == false)
      scale = 1.0;
    image.planeScales.append(QVector<double>(qMax(0, static_cast<int>(s->z_Dimension())), scale * calibration));
  }

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

  const int frames = file->numFrames();
  const int minFrame = file->minFrame();
  const int minSlice = file->minSlice();
  const int maxSlice = file->maxSlice();
  if(frames < 1)
    return fail(error, "no frames found in " + fileName);

  int nx = 0, ny = 0, nz = 0;
  int dataType = CPhilipsSubHeader::UnknownDataType;
  QScopedPointer<CPhilipsSubHeader> firstSubHeader;
  bool unscaled = false;

  for(int f=0; f < frames; f++) {
    const int frame = minFrame + f;
    const int slices = file->numSlices(frame);

    if(f == 0)
      nz = slices;
    else if(slices != nz)
      return fail(error, "the frames of the Philips file differ in their number of slices");

    // slice numbers of this frame (see CPhilipsFile::readMatrix())
    const int increment = (nz > 1) ? (maxSlice - minSlice) / (nz - 1) : 1;
    if(nz < 1 || increment < 1)
      return fail(error, "invalid slice numbering in " + fileName);

    QByteArray frameData;
    QVector<double> scales;
    QScopedPointer<CPhilipsSubHeader> frameSubHeader;

    for(int k=0; k < nz; k++) {
      const int slice = minSlice + k * increment;
      QByteArray* data = NULL;
      CPhilipsSubHeader* subHeader = NULL;

      if(file->readMatrix(data, subHeader, frame, slice) == false || data == NULL || subHeader == NULL) {
        delete data;
        delete subHeader;
        return fail(error, QString("unable to read slice %1 of frame %2 of %3").arg(slice).arg(frame).arg(fileName));
      }
      QScopedPointer<QByteArray> dataGuard(data);
      QScopedPointer<CPhilipsSubHeader> subHeaderGuard(subHeader);

      if(subHeader->subHeaderType() != CPhilipsSubHeader::Image && subHeader->subHeaderType() != CPhilipsSubHeader::Syntegra)
        return fail(error, "only Philips image data can be converted to NIfTI");
      const CPhilipsSubHeaderImage* s = static_cast<const CPhilipsSubHeaderImage*>(subHeader);

      if(f == 0 && k == 0) {
        nx = s->xdim();
        ny = s->ydim();
        dataType = s->datype();
      }

      if(s->xdim() != nx || s->ydim() != ny || nx < 1 || ny < 1)
        return fail(error, "the slices of the Philips file differ in their dimensions");
      if(s->datype() != dataType)
        return fail(error, "the slices of the Philips file differ in their data type");

      // stored values as ECAT7 data type: unsigned 16 bit values are kept as 32 bit integers
      const qint64 voxels = static_cast<qint64>(nx) * ny;
      int bytes = 0;
      switch(dataType) {
        case CPhilipsSubHeader::ByteData:      bytes = 1; break;
        case CPhilipsSubHeader::SignedShort:
        case CPhilipsSubHeader::UnsignedShort: bytes = 2; break;
        case CPhilipsSubHeader::Float:         bytes = 4; break;
        default:
          return fail(error, QString("unsupported Philips data type %1").arg(dataType));
      }
      if(data->size() < voxels * bytes)
        return fail(error, QString("slice %1 of frame %2 is too small").arg(slice).arg(frame));

      if(dataType == CPhilipsSubHeader::UnsignedShort) {
        const quint16* src = reinterpret_cast<const quint16*>(data->constData());
        QByteArray converted(voxels * 4, '\0');
        qint32* dst = reinterpret_cast<qint32*>(converted.data());
        for(qint64 i=0; i < voxels; i++)
          dst[i] = src[i];
        frameData.append(converted);
      } else {
        frameData.append(data->constData(), voxels * bytes);
      }

      bool ok = false;
      double scale = s->ecat_Scale_Factor(ok, mainHeader);
      if(ok == false || scale <= 0.0 || std::isfinite(scale) == false) {
        unscaled = true;
        scale = 1.0;
      }
      scales.append(scale);

      if(k == 0)
        frameSubHeader.reset(subHeaderGuard.take());
    }

    // ECAT7 image sub header of this frame from the sub header of its first slice
    CECAT7SubHeaderImage* ecatSubHeader = new CECAT7SubHeaderImage(&image.ecatFile);
    image.subHeaders.append(ecatSubHeader);
    if(ecatSubHeader->convertFrom(frameSubHeader.data(), mainHeader) == false)
      return fail(error, "unable to convert the Philips sub header of " + fileName);

    ecatSubHeader->setZ_Dimension(nz);
    switch(dataType) {
      case CPhilipsSubHeader::ByteData:      ecatSubHeader->setData_Type(CECATSubHeader::ByteData); break;
      case CPhilipsSubHeader::SignedShort:   ecatSubHeader->setData_Type(CECATSubHeader::SunShort); break;
      case CPhilipsSubHeader::UnsignedShort: ecatSubHeader->setData_Type(CECATSubHeader::SunLong); break;
      default:                               ecatSubHeader->setData_Type(CECATSubHeader::IEEEFloat); break;
    }
    ecatSubHeader->setScale_Factor(scales.first());

    image.matrices.append(frameData);
    image.planeScales.append(scales);

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
    const QJsonObject json = ecatMetadata(image.mainHeader, image.subHeaders);

    if(options.niftiVersion == 2) {
      CNIFTI2MainHeader* h = static_cast<CNIFTI2MainHeader*>(header.data());
      setNIFTIData(h, storedValues == false, slope, minValue, maxValue, duration);
      if(options.jsonExtension && niftiFile.isPair() == false)
        h->setVox_Offset(540 + 4 + h->headerExtensionSize(json));
    } else {
      CNIFTI1MainHeader* h = static_cast<CNIFTI1MainHeader*>(header.data());
      setNIFTIData(h, storedValues == false, slope, minValue, maxValue, duration);
      if(options.jsonExtension && niftiFile.isPair() == false)
        h->setVox_Offset(348 + 4 + h->headerExtensionSize(json));
    }

    result = niftiFile.writeMainHeader(*header);

    if(result && options.jsonExtension) {
      result = (options.niftiVersion == 2) ? writeNIFTIExtension(static_cast<CNIFTI2MainHeader*>(header.data()), niftiFile, json)
                                           : writeNIFTIExtension(static_cast<CNIFTI1MainHeader*>(header.data()), niftiFile, json);
    }

    if(result)
      result = niftiFile.writeMatrix(data);

    if(result && options.jsonSidecar) {
      QFile sidecar(niftiSidecarFileName(niftiFileName));
      result = sidecar.open(QIODevice::WriteOnly | QIODevice::Truncate) &&
               sidecar.write(QJsonDocument(json).toJson(QJsonDocument::Indented)) > 0;
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

  applyNIFTIMetadata(*image.mainHeader, json);

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

    subHeader->setData_Type(CECATSubHeader::SunShort);
    subHeader->setScale_Factor(scale);
    subHeader->setImage_Min(minStored);
    subHeader->setImage_Max(maxStored);

    // frame timing from the metadata, the NIfTI header otherwise (s -> ms)
    double start = 0.0, duration = 0.0;
    niftiFrameTiming(geometry, json, f, start, duration);
    subHeader->setFrame_Start_Time(ecatFrameTime(start));
    subHeader->setFrame_Duration(ecatFrameTime(duration));

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
