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

// NIfTI JSON sidecar file name (<name>.json next to <name>.nii/.nii.gz)
QString sidecarFileName(const QString& niftiFileName)
{
  QFileInfo info(niftiFileName);
  QString base = info.fileName();

  if(base.endsWith(".nii.gz", Qt::CaseInsensitive))
    base.chop(7);
  else if(base.endsWith(".nii", Qt::CaseInsensitive))
    base.chop(4);

  return info.absoluteDir().filePath(base + ".json");
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
void setNIFTIData(H* h, bool floatData, double minValue, double maxValue, double duration)
{
  if(floatData) {
    h->setDataType(NIFTI_DT_FLOAT32);
    h->setBit_Pix(32);
    h->setScl_Slope(1.0);
    h->setScl_Inter(0.0);
  }

  h->setCal_Min(minValue);
  h->setCal_Max(maxValue);
  h->setPix_Dim(4, duration);
  h->setDescrip("converted from ECAT7 by libmedio");
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

void applyEcatMetadata(CECAT7MainHeader& h, const QJsonObject& json)
{
#define JSON_STRING(key, setter) if(json.contains(key)) h.setter(json[key].toString().toLatin1().constData())
#define JSON_NUMBER(key, setter, type) if(json.contains(key)) h.setter(static_cast<type>(json[key].toDouble()))

  JSON_STRING("Original_File_Name", setOriginal_File_Name);
  JSON_STRING("Study_Description", setStudy_Description);
  JSON_STRING("Physician_Name", setPhysician_Name);
  JSON_STRING("Operator_Name", setOperator_Name);
  JSON_STRING("Facility_Name", setFacility_Name);
  JSON_STRING("Study_Type", setStudy_Type);
  JSON_STRING("Patient_Name", setPatient_Name);
  JSON_STRING("Patient_ID", setPatient_ID);
  JSON_NUMBER("Patient_Sex", setPatient_Sex, CECAT7MainHeader::Patient_Sex);
  JSON_NUMBER("Patient_Age", setPatient_Age, float);
  JSON_NUMBER("Patient_Weight", setPatient_Weight, float);
  JSON_NUMBER("Patient_Height", setPatient_Height, float);
  JSON_NUMBER("Patient_Birth_Date", setPatient_Birth_Date, int);
  JSON_NUMBER("Patient_Orientation", setPatient_Orientation, CECAT7MainHeader::Patient_Orientation);
  JSON_STRING("Isotope_Name", setIsotope_Name);
  JSON_NUMBER("Isotope_Halflife", setIsotope_Halflife, float);
  JSON_STRING("Radiopharmaceutical", setRadiopharmaceutical);
  JSON_NUMBER("Dosage", setDosage, float);
  JSON_STRING("Data_Units", setData_Units);
  JSON_NUMBER("Scan_Start_Time", setScan_Start_Time, int);
  JSON_NUMBER("Dose_Start_Time", setDose_Start_Time, int);
  JSON_NUMBER("Calibration_Units", setCalibration_Units, CECAT7MainHeader::Calibration_Units);
  JSON_NUMBER("Calibration_Units_Label", setCalibration_Units_Label, CECAT7MainHeader::Calibration_Units_Label);
  JSON_NUMBER("Acquisition_Type", setAcquisition_Type, CECAT7MainHeader::Acquisition_Type);
  JSON_NUMBER("Lwr_True_Thres", setLwr_True_Thres, short);
  JSON_NUMBER("Upr_True_Thres", setUpr_True_Thres, short);
  JSON_NUMBER("Lwr_Sctr_Thres", setLwr_Sctr_Thres, short);
  JSON_NUMBER("Distance_Scanned", setDistance_Scanned, float);
  JSON_NUMBER("Transaxial_FOV", setTransaxial_FOV, float);
  JSON_NUMBER("Bin_Size", setBin_Size, float);

#undef JSON_STRING
#undef JSON_NUMBER
}

} // namespace

//=============================================================================================
// ECAT7 -> NIfTI

bool CNIFTIConverter::ecatToNifti(const QString& ecatFileName, const QString& niftiFileName,
                                  const Options& options, QString* error)
{
  ENTER();

  CECATFile ecatFile(ecatFileName);
  if(ecatFile.open(QIODevice::ReadOnly) == false)
    return fail(error, "unable to open " + ecatFileName + " as ECAT file");

  CECATMainHeader* mainHeaderPtr = NULL;
  if(ecatFile.readMainHeader(mainHeaderPtr) == false || mainHeaderPtr == NULL)
    return fail(error, "unable to read the ECAT main header of " + ecatFileName);
  QScopedPointer<CECATMainHeader> mainHeaderGuard(mainHeaderPtr);

  if(mainHeaderPtr->mainHeaderType() != CECATMainHeader::ECAT7MainHeader)
    return fail(error, "only ECAT7 files can be converted to NIfTI");
  const CECAT7MainHeader* mainHeader = static_cast<const CECAT7MainHeader*>(mainHeaderPtr);

  // read all frames
  const int frames = (mainHeader->num_Frames() > 1) ? mainHeader->num_Frames() : 1;
  QList<CECAT7SubHeaderImage*> subHeaders;
  QList<QByteArray> matrices;

  struct Cleanup {
    QList<CECAT7SubHeaderImage*>& list;
    ~Cleanup() { qDeleteAll(list); }
  } cleanup = { subHeaders };

  for(int f=1; f <= frames; f++) {
    CECATSubHeader* subHeader = NULL;
    QByteArray* matrix = NULL;

    if(ecatFile.readMatrix(matrix, subHeader, f) == false || matrix == NULL || subHeader == NULL) {
      delete matrix;
      delete subHeader;
      return fail(error, QString("unable to read frame %1 of %2").arg(f).arg(ecatFileName));
    }

    matrices.append(*matrix);
    delete matrix;

    if(subHeader->subHeaderType() != CECATSubHeader::ECAT7_Image) {
      delete subHeader;
      return fail(error, "only ECAT7 image data can be converted to NIfTI");
    }
    subHeaders.append(static_cast<CECAT7SubHeaderImage*>(subHeader));
  }
  ecatFile.close();

  const CECAT7SubHeaderImage* first = subHeaders.first();
  const int nx = first->x_Dimension();
  const int ny = first->y_Dimension();
  const int nz = first->z_Dimension();
  const qint64 voxels = static_cast<qint64>(nx) * ny * nz;
  const qint64 planeVoxels = static_cast<qint64>(nx) * ny;
  const double calibration = ecatCalibrationFactor(mainHeader);

  // keep the stored values if all frames share data type and scale factor
  bool storedValues = (options.floatData == false);
  bool sameDuration = true;

  for(int f=0; f < frames; f++) {
    const CECAT7SubHeaderImage* s = subHeaders[f];

    if(s->x_Dimension() != nx || s->y_Dimension() != ny || s->z_Dimension() != nz)
      return fail(error, "the frames of the ECAT7 file differ in their dimensions");

    if(ecatBytesPerVoxel(s->data_Type()) == 0)
      return fail(error, QString("unsupported ECAT7 data type %1").arg(s->data_Type()));

    if(matrices[f].size() < voxels * ecatBytesPerVoxel(s->data_Type()))
      return fail(error, QString("matrix of frame %1 is too small").arg(f + 1));

    if(s->data_Type() != first->data_Type() || s->scale_Factor() != first->scale_Factor())
      storedValues = false;

    if(s->frame_Duration() != first->frame_Duration())
      sameDuration = false;
  }

  const int bytesPerVoxel = storedValues ? ecatBytesPerVoxel(first->data_Type()) : 4;
  QByteArray data(voxels * frames * bytesPerVoxel, '\0');
  double minValue = std::numeric_limits<double>::max();
  double maxValue = -std::numeric_limits<double>::max();

  for(int f=0; f < frames; f++) {
    const CECAT7SubHeaderImage* s = subHeaders[f];
    double scale = s->scale_Factor();
    if(scale == 0.0 || std::isfinite(scale) == false)
      scale = 1.0;
    scale *= calibration;

    const char* src = matrices[f].constData();
    char* dst = data.data() + f * voxels * bytesPerVoxel;

    // NIfTI stores the planes in reversed order (see CNIFTIGeometry.h)
    for(int k=0; k < nz; k++) {
      const qint64 srcPlane = k * planeVoxels;
      const qint64 dstPlane = (nz - 1 - k) * planeVoxels;

      if(storedValues) {
        // ECAT matrices are returned in machine byte order
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
  bool result = (header.data() != NULL) && header->convertFrom(mainHeaderPtr, first);

  if(result) {
    const double duration = sameDuration ? first->frame_Duration() / 1000.0 : 0.0;
    const QJsonObject json = ecatMetadata(mainHeader, subHeaders);

    if(options.niftiVersion == 2) {
      CNIFTI2MainHeader* h = static_cast<CNIFTI2MainHeader*>(header.data());
      setNIFTIData(h, storedValues == false, minValue, maxValue, duration);
      if(options.jsonExtension)
        h->setVox_Offset(540 + 4 + h->headerExtensionSize(json));
    } else {
      CNIFTI1MainHeader* h = static_cast<CNIFTI1MainHeader*>(header.data());
      setNIFTIData(h, storedValues == false, minValue, maxValue, duration);
      if(options.jsonExtension)
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
      QFile sidecar(sidecarFileName(niftiFileName));
      result = sidecar.open(QIODevice::WriteOnly | QIODevice::Truncate) &&
               sidecar.write(QJsonDocument(json).toJson(QJsonDocument::Indented)) > 0;
    }
  }

  niftiFile.close();

  if(result == false) {
    QFile::remove(niftiFileName);
    return fail(error, "unable to write " + niftiFileName);
  }

  RETURN(true);
  return true;
}

//=============================================================================================
// NIfTI -> ECAT7

bool CNIFTIConverter::niftiToEcat(const QString& niftiFileName, const QString& ecatFileName, QString* error)
{
  ENTER();

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
      return fail(error, "NIfTI data with more than 4 dimensions cannot be converted to ECAT7");
  }

  const int bytesPerVoxel = niftiBytesPerVoxel(info.datatype);
  if(bytesPerVoxel == 0)
    return fail(error, QString("unsupported NIfTI datatype %1").arg(info.datatype));

  QByteArray* matrixPtr = NULL;
  if(niftiFile.readMatrix(matrixPtr) == false || matrixPtr == NULL)
    return fail(error, "unable to read the voxel data of " + niftiFileName);
  QScopedPointer<QByteArray> matrix(matrixPtr);

  // metadata: JSON header extension or JSON sidecar file
  QJsonObject json = nifti1 ? static_cast<CNIFTI1MainHeader*>(headerPtr)->readHeaderExtension(niftiFile)
                            : static_cast<CNIFTI2MainHeader*>(headerPtr)->readHeaderExtension(niftiFile);
  niftiFile.close();

  if(json.isEmpty()) {
    QFile sidecar(sidecarFileName(niftiFileName));
    if(sidecar.open(QIODevice::ReadOnly))
      json = QJsonDocument::fromJson(sidecar.readAll()).object();
  }

  const int frames = geometry.frames;
  const int nx = geometry.dims[0], ny = geometry.dims[1], nz = geometry.dims[2];
  const qint64 voxels = static_cast<qint64>(nx) * ny * nz;
  const qint64 dimNifti[3] = { info.dim[1], info.dim[2], info.dim[3] };

  // keep the stored values if they are 8/16 bit integers without intercept
  const bool storedValues = (info.datatype == 2 || info.datatype == 256 || info.datatype == 4) && info.inter == 0.0;

  // frame timing (s)
  QJsonArray startTimes = json["FrameTimesStart"].toArray();
  QJsonArray durations = json["FrameDuration"].toArray();

  CECATFile ecatFile(ecatFileName, CECATMainHeader::ECAT7_Volume16);
  if(ecatFile.open(QIODevice::WriteOnly) == false)
    return fail(error, "unable to open " + ecatFileName + " for writing");

  CECAT7MainHeader mainHeader(&ecatFile, CECATMainHeader::ECAT7_Volume16);
  bool result = mainHeader.convertFrom(headerPtr, NULL);

  if(result) {
    applyEcatMetadata(mainHeader, json);

    // the values written already contain the calibration factor
    mainHeader.setCalibration_Factor(1.0f);
    result = ecatFile.writeMainHeader(mainHeader);
  }

  for(int f=0; f < frames && result; f++) {
    // reorder the voxels of this frame into the ECAT7 voxel order
    QVector<double> values(voxels);
    double maxAbs = 0.0;

    for(int k=0; k < nz; k++) {
      for(int j=0; j < ny; j++) {
        for(int i=0; i < nx; i++) {
          const int e[3] = { i, j, k };
          qint64 v[3];
          for(int a=0; a < 3; a++)
            v[geometry.axis[a]] = geometry.flip[a] ? geometry.dims[a] - 1 - e[a] : e[a];

          const qint64 index = v[0] + dimNifti[0] * (v[1] + dimNifti[1] * (v[2] + dimNifti[2] * f));
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

    QScopedPointer<CECATSubHeader> subHeaderPtr(ecatFile.createEmptySubHeader());
    CECAT7SubHeaderImage* subHeader = static_cast<CECAT7SubHeaderImage*>(subHeaderPtr.data());
    result = (subHeader != NULL) && subHeader->convertFrom(NULL, headerPtr);

    if(result) {
      subHeader->setData_Type(CECATSubHeader::SunShort);
      subHeader->setScale_Factor(scale);
      subHeader->setImage_Min(minStored);
      subHeader->setImage_Max(maxStored);

      // frame timing from the metadata, the NIfTI header otherwise (s -> ms)
      double start = geometry.frameStart + f * geometry.frameDuration;
      double duration = geometry.frameDuration;
      if(startTimes.size() == frames && durations.size() == frames) {
        start = startTimes[f].toDouble();
        duration = durations[f].toDouble();
      } else if(frames == 1 && json.contains("Frame_Duration")) {
        // metadata written by earlier versions (ms)
        start = json["Frame_Start_Time"].toDouble() / 1000.0;
        duration = json["Frame_Duration"].toDouble() / 1000.0;
      }
      subHeader->setFrame_Start_Time(start > 0.0 ? static_cast<unsigned int>(start * 1000.0 + 0.5) : 0);
      subHeader->setFrame_Duration(duration > 0.0 ? static_cast<unsigned int>(duration * 1000.0 + 0.5) : 0);

      if(json.contains("Recon_Zoom"))
        subHeader->setRecon_Zoom(json["Recon_Zoom"].toDouble());

      result = ecatFile.writeSubHeader(*subHeader, f + 1) &&
               ecatFile.writeMatrix(data, f + 1);
    }
  }

  ecatFile.close();

  if(result == false) {
    QFile::remove(ecatFileName);
    return fail(error, "unable to write " + ecatFileName);
  }

  RETURN(true);
  return true;
}
