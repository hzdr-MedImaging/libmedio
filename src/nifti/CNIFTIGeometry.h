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

#ifndef CNIFTIGEOMETRY_H
#define CNIFTIGEOMETRY_H

// Private helpers (not installed) converting the geometry, data type and
// scaling between ECAT7 image headers and NIfTI-1/NIfTI-2 main headers. They
// are shared by the header classes of both formats and by CNIFTIConverter, so
// that both conversion directions are the inverse of each other.
//
// Geometry conventions:
//
// ECAT7 stores the planes from superior (plane 1) to inferior. The patient
// coordinates (DICOM LPS, mm) of ECAT voxel (i, j, k), 0-based, are
//
//   x = x_offset + (i - (nx-1)/2) * x_pixel_size
//   y = y_offset + (j - (ny-1)/2) * y_pixel_size
//   z = init_bed_position + z_offset - k * z_pixel_size
//
// i.e. the image center is located at (x_offset, y_offset). NIfTI files
// written by libmedio store the same voxels with reversed slice order
// (NIfTI voxel (i, j, nz-1-k)) and the corresponding RAS affine in both
// qform and sform (qform_code = sform_code = 1).
//
// NIfTI files without qform and sform (qform_code = sform_code = 0) are
// assumed to use the voxel order of these files, as written by common PET
// tools (e.g. PMOD, ROVER): first axis towards patient left, second axis
// towards posterior, third axis towards superior.

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <cmath>

#include "CECATMainHeader.h"
#include "CECATSubHeader.h"
#include "CECAT7MainHeader.h"
#include "CECAT7SubHeaderImage.h"
#include "CPhilipsMainHeader.h"
#include "CPhilipsSubHeader.h"
#include "CNIFTIMainHeader.h"
#include "CNIFTI1MainHeader.h"
#include "CNIFTI2MainHeader.h"

// NIfTI datatype codes and units
#define NIFTI_DT_UINT8     2
#define NIFTI_DT_INT16     4
#define NIFTI_DT_INT32     8
#define NIFTI_DT_FLOAT32  16
#define NIFTI_XFORM_SCANNER_ANAT 1
#define NIFTI_UNITS_MM_SEC 10

//=============================================================================================
// ECAT7 -> NIfTI

// calibration factor of an ECAT7 main header (1 if not set)
static inline double ecatCalibrationFactor(const CECATMainHeader* mainHeader)
{
  const CECAT7MainHeader* header = dynamic_cast<const CECAT7MainHeader*>(mainHeader);
  double factor = header ? header->calibration_Factor() : 1.0;

  return (factor > 0.0 && std::isfinite(factor)) ? factor : 1.0;
}

// Fill a NIfTI-1/NIfTI-2 header from an ECAT7 main header and the image sub
// header of its first frame: dimensions (incl. the number of frames), voxel
// sizes, geometry (qform/sform), units, timing, data type and scaling
// (scale factor x calibration factor). Returns false if subHeader is no ECAT7
// image sub header.
template <class H>
static bool convertNIFTIFromECAT(H& h, const CECATMainHeader* mainHeader, const CECATSubHeader* subHeader, QString* error = NULL)
{
  if(subHeader == NULL || subHeader->subHeaderType() != CECATSubHeader::ECAT7_Image) {
    if(error)
      *error = "only ECAT7 image data can be converted to NIfTI";
    return false;
  }

  const CECAT7SubHeaderImage* sub = static_cast<const CECAT7SubHeaderImage*>(subHeader);
  const CECAT7MainHeader* main = dynamic_cast<const CECAT7MainHeader*>(mainHeader);

  const int nx = sub->x_Dimension();
  const int ny = sub->y_Dimension();
  const int nz = sub->z_Dimension();
  const int frames = (main && main->num_Frames() > 1) ? main->num_Frames() : 1;

  // ECAT7 stores lengths in cm
  const double px = sub->x_Pixel_Size() * 10.0;
  const double py = sub->y_Pixel_Size() * 10.0;
  const double pz = sub->z_Pixel_Size() * 10.0;
  const double xOffset = sub->x_Offset() * 10.0;
  const double yOffset = sub->y_Offset() * 10.0;
  const double bedPosition = (main ? main->init_Bed_Position() * 10.0 : 0.0) + sub->z_Offset() * 10.0;

  // dimensions
  h.setDim(0, frames > 1 ? 4 : 3);
  h.setDim(1, nx);
  h.setDim(2, ny);
  h.setDim(3, nz);
  h.setDim(4, frames);
  for(short i=5; i < 8; i++)
    h.setDim(i, 1);

  h.setPix_Dim(0, 1.0); // qfac
  h.setPix_Dim(1, px);
  h.setPix_Dim(2, py);
  h.setPix_Dim(3, pz);
  h.setPix_Dim(4, sub->frame_Duration() / 1000.0);
  for(short i=5; i < 8; i++)
    h.setPix_Dim(i, 1.0);

  h.setXyzt_Units(NIFTI_UNITS_MM_SEC);
  h.setToffset(sub->frame_Start_Time() / 1000.0);

  // RAS affine of NIfTI voxel (i, j, nz-1-k), see above
  const double tx = (nx - 1) / 2.0 * px - xOffset;
  const double ty = (ny - 1) / 2.0 * py - yOffset;
  const double tz = bedPosition - (nz - 1) * pz;

  h.setQform_Code(NIFTI_XFORM_SCANNER_ANAT);
  h.setQuatern_B(0.0); // rotation by 180 degrees about z: diag(-1, -1, 1)
  h.setQuatern_C(0.0);
  h.setQuatern_D(1.0);
  h.setQoffset_X(tx);
  h.setQoffset_Y(ty);
  h.setQoffset_Z(tz);

  h.setSform_Code(NIFTI_XFORM_SCANNER_ANAT);
  for(short i=0; i < 4; i++) {
    h.setSrow_X(i, 0.0);
    h.setSrow_Y(i, 0.0);
    h.setSrow_Z(i, 0.0);
  }
  h.setSrow_X(0, -px); h.setSrow_X(3, tx);
  h.setSrow_Y(1, -py); h.setSrow_Y(3, ty);
  h.setSrow_Z(2,  pz); h.setSrow_Z(3, tz);

  // data type and scaling of the stored values
  switch(sub->data_Type()) {
    case CECATSubHeader::ByteData:
      h.setDataType(NIFTI_DT_UINT8);
      h.setBit_Pix(8);
      break;

    case CECATSubHeader::VAX_Ix4:
    case CECATSubHeader::SunLong:
      h.setDataType(NIFTI_DT_INT32);
      h.setBit_Pix(32);
      break;

    case CECATSubHeader::VAX_Rx4:
    case CECATSubHeader::IEEEFloat:
      h.setDataType(NIFTI_DT_FLOAT32);
      h.setBit_Pix(32);
      break;

    default:
      h.setDataType(NIFTI_DT_INT16);
      h.setBit_Pix(16);
      break;
  }

  double scale = sub->scale_Factor();
  if(scale == 0.0 || std::isfinite(scale) == false)
    scale = 1.0;

  h.setScl_Slope(scale * ecatCalibrationFactor(mainHeader));
  h.setScl_Inter(0.0);

  return true;
}

//=============================================================================================
// Philips -> NIfTI: via the ECAT7 representation of the Philips headers
// (existing Philips -> ECAT7 conversions), see CNIFTIConverter.h

template <class H>
static bool convertNIFTIFromPhilips(H& h, const CPhilipsMainHeader* mainHeader, const CPhilipsSubHeader* subHeader, QString* error = NULL)
{
  if(subHeader == NULL ||
     (subHeader->subHeaderType() != CPhilipsSubHeader::Image && subHeader->subHeaderType() != CPhilipsSubHeader::Syntegra)) {
    if(error)
      *error = "only Philips image data can be converted to NIfTI";
    return false;
  }

  CECAT7MainHeader ecatMainHeader;
  CECAT7SubHeaderImage ecatSubHeader;
  if(ecatMainHeader.convertFrom(mainHeader, subHeader) == false ||
     ecatSubHeader.convertFrom(subHeader, mainHeader) == false) {
    if(error)
      *error = "unable to convert the Philips headers";
    return false;
  }

  // the scale factor of the sub header is in physical units already
  ecatMainHeader.setCalibration_Factor(1.0f);
  return convertNIFTIFromECAT(h, &ecatMainHeader, &ecatSubHeader, error);
}

//=============================================================================================
// NIfTI -> ECAT7

// geometry of the ECAT7 image corresponding to a NIfTI header
struct ECATGeometryFromNIFTI
{
  int dims[3];          // ECAT7 x, y, z dimension
  double pixelSize[3];  // ECAT7 voxel size (mm)
  double xOffset;       // ECAT7 x/y offset (mm)
  double yOffset;
  double bedPosition;   // ECAT7 bed position of plane 1 (mm)
  int axis[3];          // NIfTI voxel axis (0..2) providing ECAT7 axis 0..2
  bool flip[3];         // ECAT7 axis runs opposite to the NIfTI voxel axis
  int frames;           // number of 3D volumes
  double frameStart;    // start time of the first volume (s)
  double frameDuration; // time between the volumes (s), 0 if unknown
  bool oblique;         // the NIfTI axes are not aligned with the patient axes
};

// factor converting the NIfTI spatial unit to mm (unknown: mm)
template <class H>
static double niftiSpaceScale(const H& h)
{
  switch(static_cast<int>(h.xyzt_Units()) & 0x07) {
    case 1: return 1000.0; // NIFTI_UNITS_METER
    case 3: return 0.001;  // NIFTI_UNITS_MICRON
  }
  return 1.0;
}

// factor converting the NIfTI time unit to s (unknown: s)
template <class H>
static double niftiTimeScale(const H& h)
{
  switch(static_cast<int>(h.xyzt_Units()) & 0x38) {
    case 16: return 0.001; // NIFTI_UNITS_MSEC
    case 24: return 1e-6;  // NIFTI_UNITS_USEC
  }
  return 1.0;
}

// RAS affine of a NIfTI header: sform, qform or the default described above
template <class H>
static void niftiAffine(const H& h, double m[3][4])
{
  const double px = std::fabs(static_cast<double>(h.pix_Dim(1)));
  const double py = std::fabs(static_cast<double>(h.pix_Dim(2)));
  const double pz = std::fabs(static_cast<double>(h.pix_Dim(3)));

  if(h.sform_Code() > 0) {
    for(short i=0; i < 4; i++) {
      m[0][i] = h.srow_X(i);
      m[1][i] = h.srow_Y(i);
      m[2][i] = h.srow_Z(i);
    }
  } else if(h.qform_Code() > 0) {
    // quaternion representation, see nifti1.h
    double b = h.quatern_B(), c = h.quatern_C(), d = h.quatern_D();
    double a = 1.0 - (b*b + c*c + d*d);
    a = (a < 1e-7) ? 0.0 : std::sqrt(a);
    if(a == 0.0) {
      // normalize (b, c, d) for a 180 degree rotation
      double n = std::sqrt(b*b + c*c + d*d);
      b /= n; c /= n; d /= n;
    }
    const double qfac = (h.pix_Dim(0) < 0) ? -1.0 : 1.0;
    const double r[3][3] = { { a*a+b*b-c*c-d*d, 2*(b*c-a*d),     2*(b*d+a*c)     },
                             { 2*(b*c+a*d),     a*a+c*c-b*b-d*d, 2*(c*d-a*b)     },
                             { 2*(b*d-a*c),     2*(c*d+a*b),     a*a+d*d-c*c-b*b } };
    const double s[3] = { px, py, qfac * pz };
    for(int i=0; i < 3; i++)
      for(int j=0; j < 3; j++)
        m[i][j] = r[i][j] * s[j];
    m[0][3] = h.qoffset_X();
    m[1][3] = h.qoffset_Y();
    m[2][3] = h.qoffset_Z();
  } else {
    const double def[3][4] = { { -px, 0.0, 0.0, 0.0 },
                               { 0.0, -py, 0.0, 0.0 },
                               { 0.0, 0.0,  pz, 0.0 } };
    for(int i=0; i < 3; i++)
      for(int j=0; j < 4; j++)
        m[i][j] = def[i][j];
  }

  const double unit = niftiSpaceScale(h);
  for(int i=0; i < 3; i++)
    for(int j=0; j < 4; j++)
      m[i][j] *= unit;
}

// Determine the ECAT7 geometry corresponding to a NIfTI header. The NIfTI voxel
// axes are assigned to the patient axis they are (mostly) aligned with.
template <class H>
static bool ecatGeometryFromNIFTIHeader(const H& h, ECATGeometryFromNIFTI& g, QString* error = NULL)
{
  if(h.dim(0) < 3 || h.dim(0) > 4) {
    if(error)
      *error = QString("only 3D and 4D NIfTI data can be converted to ECAT7 (dim[0] = %1)").arg(h.dim(0));
    return false;
  }

  double m[3][4];
  niftiAffine(h, m);

  // RAS -> LPS
  for(int j=0; j < 4; j++) {
    m[0][j] = -m[0][j];
    m[1][j] = -m[1][j];
  }

  bool used[3] = { false, false, false };
  g.oblique = false;

  for(int a=0; a < 3; a++) {
    double norm = std::sqrt(m[0][a]*m[0][a] + m[1][a]*m[1][a] + m[2][a]*m[2][a]);
    int w = 0;
    for(int i=1; i < 3; i++) {
      if(std::fabs(m[i][a]) > std::fabs(m[w][a]))
        w = i;
    }

    if(norm == 0.0 || used[w]) {
      if(error)
        *error = "the NIfTI voxel axes cannot be assigned to the patient axes";
      return false;
    }

    if(std::fabs(m[w][a]) < norm * (1.0 - 1e-6))
      g.oblique = true;

    used[w] = true;
    g.axis[w] = a;
    g.dims[w] = static_cast<int>(h.dim(a + 1));
    g.pixelSize[w] = norm;
    // ECAT7: x towards left, y towards posterior, z (plane number) towards inferior
    g.flip[w] = (w < 2) ? (m[w][a] < 0) : (m[w][a] > 0);
  }

  // patient position of ECAT7 voxel (0, 0, 0)
  double v[3];
  for(int e=0; e < 3; e++)
    v[g.axis[e]] = g.flip[e] ? g.dims[e] - 1 : 0;

  double p[3];
  for(int i=0; i < 3; i++)
    p[i] = m[i][0] * v[0] + m[i][1] * v[1] + m[i][2] * v[2] + m[i][3];

  // positions rounded to 0.1 um to remove the rounding noise of the float affine
  g.xOffset = std::floor((p[0] + (g.dims[0] - 1) / 2.0 * g.pixelSize[0]) * 1e4 + 0.5) / 1e4;
  g.yOffset = std::floor((p[1] + (g.dims[1] - 1) / 2.0 * g.pixelSize[1]) * 1e4 + 0.5) / 1e4;
  g.bedPosition = std::floor(p[2] * 1e4 + 0.5) / 1e4;
  g.frames = (h.dim(0) >= 4 && h.dim(4) > 1) ? static_cast<int>(h.dim(4)) : 1;

  // pixdim[4] is only meaningful for 4D data
  const double time = niftiTimeScale(h);
  const double toffset = h.toffset() * time;
  const double duration = (h.dim(0) >= 4) ? h.pix_Dim(4) * time : 0.0;
  g.frameStart = (std::isfinite(toffset) && toffset > 0.0) ? toffset : 0.0;
  g.frameDuration = (std::isfinite(duration) && duration > 0.0) ? duration : 0.0;

  return true;
}

// dispatch for both NIfTI header versions
static inline bool ecatGeometryFromNIFTI(const CNIFTIMainHeader* header, ECATGeometryFromNIFTI& g, QString* error = NULL)
{
  if(header->mainHeaderType() == CNIFTIMainHeader::NIFTI1MainHeader)
    return ecatGeometryFromNIFTIHeader(*static_cast<const CNIFTI1MainHeader*>(header), g, error);

  return ecatGeometryFromNIFTIHeader(*static_cast<const CNIFTI2MainHeader*>(header), g, error);
}

// Index (within one 3D volume) of the NIfTI voxel holding ECAT7 voxel (i, j, k);
// niftiDims are dim[1..3] of the NIfTI header.
static inline qint64 niftiVoxelIndex(const ECATGeometryFromNIFTI& g, const qint64 niftiDims[3], int i, int j, int k)
{
  const int e[3] = { i, j, k };
  qint64 v[3];
  for(int a=0; a < 3; a++)
    v[g.axis[a]] = g.flip[a] ? g.dims[a] - 1 - e[a] : e[a];

  return v[0] + niftiDims[0] * (v[1] + niftiDims[1] * v[2]);
}

//=============================================================================================
// metadata of NIfTI files written by CNIFTIConverter (JSON header extension
// or JSON sidecar file, see CNIFTIConverter.h)

// NIfTI JSON sidecar file name (<name>.json next to <name>.nii/.nii.gz/.hdr/.img)
static inline QString niftiSidecarFileName(const QString& niftiFileName)
{
  QFileInfo info(niftiFileName);
  QString base = info.fileName();

  if(base.endsWith(".gz", Qt::CaseInsensitive))
    base.chop(3);
  if(base.endsWith(".nii", Qt::CaseInsensitive) ||
     base.endsWith(".hdr", Qt::CaseInsensitive) ||
     base.endsWith(".img", Qt::CaseInsensitive))
    base.chop(4);

  return info.absoluteDir().filePath(base + ".json");
}

// metadata of an opened NIfTI file: JSON header extension or JSON sidecar file
static inline QJsonObject niftiMetadata(CNIFTIFile& file, const CNIFTIMainHeader* header, const QString& fileName)
{
  QJsonObject json = (header->mainHeaderType() == CNIFTIMainHeader::NIFTI1MainHeader)
                     ? static_cast<const CNIFTI1MainHeader*>(header)->readHeaderExtension(file)
                     : static_cast<const CNIFTI2MainHeader*>(header)->readHeaderExtension(file);

  if(json.isEmpty()) {
    QFile sidecar(niftiSidecarFileName(fileName));
    if(sidecar.open(QIODevice::ReadOnly))
      json = QJsonDocument::fromJson(sidecar.readAll()).object();
  }

  return json;
}

// start time and duration (s) of volume f (0-based) of a NIfTI file: from
// the metadata if it holds the timing of all volumes, from the header otherwise
static inline void niftiFrameTiming(const ECATGeometryFromNIFTI& g, const QJsonObject& json, int f,
                                    double& start, double& duration)
{
  const QJsonArray startTimes = json["FrameTimesStart"].toArray();
  const QJsonArray durations = json["FrameDuration"].toArray();

  start = g.frameStart + f * g.frameDuration;
  duration = g.frameDuration;

  if(startTimes.size() == g.frames && durations.size() == g.frames) {
    start = startTimes[f].toDouble();
    duration = durations[f].toDouble();
  } else if(g.frames == 1 && json.contains("Frame_Duration")) {
    // metadata written by earlier versions (ms)
    start = json["Frame_Start_Time"].toDouble() / 1000.0;
    duration = json["Frame_Duration"].toDouble() / 1000.0;
  }
}

// ECAT7 frame start time or duration (ms) of a time in s
static inline unsigned int ecatFrameTime(double seconds)
{
  return seconds > 0.0 ? static_cast<unsigned int>(seconds * 1000.0 + 0.5) : 0;
}

// restore the ECAT7 main header values stored in the metadata by CNIFTIConverter
static inline void applyNIFTIMetadata(CECAT7MainHeader& h, const QJsonObject& json)
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

#endif // CNIFTIGEOMETRY_H
