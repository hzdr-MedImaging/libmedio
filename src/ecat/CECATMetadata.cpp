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

//! @file CECATMetadata.cpp
//! @brief private: format independent metadata of the ECAT7 headers (see CMedIOMetadataMap.h)
//!
//! Units of the ECAT7 main header: patient height in cm, weight in kg, age in
//! years, isotope half life in s, dosage in Bq (as written by libmedio, e.g.
//! when converting from Philips, and by ROVER), energy thresholds in keV, scan
//! and dose start time as Unix time (UTC), birth date as Unix time of midnight.

#include "CMedIOMetadataMap.h"
#include "CECAT7MainHeader.h"
#include "CECAT7SubHeaderImage.h"

#include <QJsonArray>
#include <QRegExp>

#include <climits>

namespace MedIOMetadata {

namespace {

typedef CECAT7MainHeader MH;
typedef CECAT7SubHeaderImage SH;

//---------------------------------------------------------------------------------------------
// conversions

// DICOM patient position of the ECAT7 patient orientation
const struct { CECAT7MainHeader::Patient_Orientation orientation; const char* position; } positions[] = {
  { CECAT7MainHeader::FF_Prone,  "FFP"  }, { CECAT7MainHeader::HF_Prone,  "HFP"  },
  { CECAT7MainHeader::FF_Supine, "FFS"  }, { CECAT7MainHeader::HF_Supine, "HFS"  },
  { CECAT7MainHeader::FF_Right,  "FFDR" }, { CECAT7MainHeader::HF_Right,  "HFDR" },
  { CECAT7MainHeader::FF_Left,   "FFDL" }, { CECAT7MainHeader::HF_Left,   "HFDL" },
};

QJsonValue patientPosition(const MH& h)
{
  for(unsigned i=0; i < sizeof(positions) / sizeof(positions[0]); i++) {
    if(positions[i].orientation == h.patient_Orientation())
      return QJsonValue(QString::fromLatin1(positions[i].position));
  }
  return QJsonValue(QJsonValue::Undefined);
}

bool setPatientPosition(MH& h, const QJsonValue& value)
{
  h.setPatient_Orientation(CECAT7MainHeader::Orient_Unknown);
  if(value.isNull())
    return true;
  for(unsigned i=0; i < sizeof(positions) / sizeof(positions[0]); i++) {
    if(value.toString() == QLatin1String(positions[i].position)) {
      h.setPatient_Orientation(positions[i].orientation);
      return true;
    }
  }
  return false;
}

// DICOM age string (e.g. "045Y") of an age in years
QJsonValue patientAge(float years)
{
  if(years <= 0.0f)
    return QJsonValue(QJsonValue::Undefined);
  return QJsonValue(QString("%1Y").arg(qRound(years), 3, 10, QChar('0')));
}

bool setPatientAge(MH& h, const QJsonValue& value)
{
  h.setPatient_Age(0.0f);
  if(value.isNull())
    return true;

  // nnnD, nnnW, nnnM or nnnY
  QRegExp pattern("^(\\d{3})([DWMY])$");
  if(pattern.exactMatch(value.toString()) == false)
    return false;
  const double n = pattern.cap(1).toDouble();
  const QString unit = pattern.cap(2);
  const double years = (unit == "Y") ? n : (unit == "M") ? n / 12.0 : (unit == "W") ? n * 7.0 / 365.25 : n / 365.25;
  h.setPatient_Age(static_cast<float>(years));
  return true;
}

// DICOM units of the ECAT7 data units
const struct { const char* ecat; const char* dicom; } units[] = {
  { "Bq/cc", "BQML" }, { "Bq/ml", "BQML" }, { "Bq/mL", "BQML" },
  { "ECAT counts", "CNTS" }, { "counts", "CNTS" },
};

QJsonValue dataUnits(const MH& h)
{
  const QString s = QString::fromLatin1(h.data_Units()).trimmed();
  for(unsigned i=0; i < sizeof(units) / sizeof(units[0]); i++) {
    if(s.compare(QLatin1String(units[i].ecat), Qt::CaseInsensitive) == 0)
      return QJsonValue(QString::fromLatin1(units[i].dicom));
  }

  // DICOM codes written by libmedio
  if(QRegExp("^[A-Z0-9]{1,16}$").exactMatch(s))
    return QJsonValue(s);

  return QJsonValue(QJsonValue::Undefined);
}

bool setDataUnits(MH& h, const QJsonValue& value)
{
  const QString code = value.toString();
  QByteArray text = code.toLatin1();
  if(code == "BQML")
    text = "Bq/ml";
  else if(code == "CNTS")
    text = "ECAT counts";
  h.setData_Units(text.constData());
  return true;
}

//---------------------------------------------------------------------------------------------
// main header

const Field mainKeys[] = {
  MEDIO_KEY(MH, "PatientName", text(h.patient_Name()),
            h.setPatient_Name(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "PatientID", text(h.patient_ID()),
            h.setPatient_ID(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "PatientBirthDate", dateOfUnixTime(static_cast<qint32>(h.patient_Birth_Date())),
            qint64 s = 0; if(unixTimeOfDate(value, s) == false) return false;
            if(s < INT_MIN || s > INT_MAX) return false;
            h.setPatient_Birth_Date(static_cast<time_t>(s)); return true;),
  MEDIO_KEY(MH, "PatientSex",
            h.patient_Sex() == CECAT7MainHeader::Sex_Male ? QJsonValue(QString("M")) :
            h.patient_Sex() == CECAT7MainHeader::Sex_Female ? QJsonValue(QString("F")) : QJsonValue(QJsonValue::Undefined),
            h.setPatient_Sex(CECAT7MainHeader::Sex_Unknown);
            if(value.toString() == "M") h.setPatient_Sex(CECAT7MainHeader::Sex_Male);
            else if(value.toString() == "F") h.setPatient_Sex(CECAT7MainHeader::Sex_Female);
            else return value.isNull();
            return true;),
  MEDIO_KEY(MH, "PatientAge", patientAge(h.patient_Age()), return setPatientAge(h, value);),
  MEDIO_KEY(MH, "PatientSize", h.patient_Height() > 0.0f ? floatNumber(h.patient_Height(), 0.01) : QJsonValue(QJsonValue::Undefined),
            h.setPatient_Height(static_cast<float>(value.toDouble() * 100.0)); return true;),
  MEDIO_KEY(MH, "PatientWeight", h.patient_Weight() > 0.0f ? floatNumber(h.patient_Weight()) : QJsonValue(QJsonValue::Undefined),
            h.setPatient_Weight(static_cast<float>(value.toDouble())); return true;),
  MEDIO_KEY(MH, "PatientPosition", patientPosition(h), return setPatientPosition(h, value);),
  MEDIO_KEY(MH, "StudyDescription", text(h.study_Description()),
            h.setStudy_Description(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "InstitutionName", text(h.facility_Name()),
            h.setFacility_Name(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "ReferringPhysicianName", text(h.physician_Name()),
            h.setPhysician_Name(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "OperatorsName", strings(text(h.operator_Name())),
            h.setOperator_Name(latin1(firstString(value)).constData()); return value.toArray().size() <= 1;),
  MEDIO_KEY(MH, "ManufacturerModelName",
            h.system_Type() != 0 ? QJsonValue(QString::number(h.system_Type())) : QJsonValue(QJsonValue::Undefined),
            h.setSystem_Type(0); if(value.isNull()) return true;
            bool ok = false; const short type = value.toString().toShort(&ok);
            if(ok == false || type <= 0) return false;
            h.setSystem_Type(type); return true;),
  MEDIO_KEY(MH, "DeviceSerialNumber", text(h.serial_Number()),
            h.setSerial_Number(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "AcquisitionDateTime", dateTimeOfUnixTime(h.scan_Start_Time()),
            qint64 s = 0; if(unixTimeOfDateTime(value, s) == false) return false;
            h.setScan_Start_Time(static_cast<time_t>(s)); return true;),
  MEDIO_KEY(MH, "Radiopharmaceutical", text(h.radiopharmaceutical()),
            h.setRadiopharmaceutical(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "RadiopharmaceuticalStartDateTime", dateTimeOfUnixTime(h.dose_Start_Time()),
            qint64 s = 0; if(unixTimeOfDateTime(value, s) == false) return false;
            h.setDose_Start_Time(static_cast<time_t>(s)); return true;),
  MEDIO_KEY(MH, "RadionuclideTotalDose", h.dosage() > 0.0f ? floatNumber(h.dosage()) : QJsonValue(QJsonValue::Undefined),
            h.setDosage(static_cast<float>(value.toDouble())); return true;),
  MEDIO_KEY(MH, "RadionuclideHalfLife", h.isotope_Halflife() > 0.0f ? floatNumber(h.isotope_Halflife()) : QJsonValue(QJsonValue::Undefined),
            h.setIsotope_Halflife(static_cast<float>(value.toDouble())); return true;),
  MEDIO_KEY(MH, "RadionuclidePositronFraction",
            h.branching_Fraction() > 0.0f ? floatNumber(h.branching_Fraction()) : QJsonValue(QJsonValue::Undefined),
            h.setBranching_Fraction(static_cast<float>(value.toDouble())); return true;),
  MEDIO_KEY(MH, "EnergyWindowLowerLimit",
            h.lwr_True_Thres() > 0 ? toJson(h.lwr_True_Thres()) : QJsonValue(QJsonValue::Undefined),
            short v = 0; if(fromJson(value.isDouble() ? QJsonValue(qRound(value.toDouble())) : value, v) == false) return false;
            h.setLwr_True_Thres(v); return value.isNull() || value.toDouble() == v;),
  MEDIO_KEY(MH, "EnergyWindowUpperLimit",
            h.upr_True_Thres() > 0 ? toJson(h.upr_True_Thres()) : QJsonValue(QJsonValue::Undefined),
            short v = 0; if(fromJson(value.isDouble() ? QJsonValue(qRound(value.toDouble())) : value, v) == false) return false;
            h.setUpr_True_Thres(v); return value.isNull() || value.toDouble() == v;),
  MEDIO_KEY(MH, "Units", dataUnits(h), return setDataUnits(h, value);),
};

// native fields (without the fields describing the image data: magic number,
// file type, number of planes, frames, gates and bed positions, bed positions,
// plane separation, calibration factor, compression code)
const Field mainNative[] = {
  MEDIO_NATIVE_TEXT(MH, "Original_File_Name", original_File_Name, setOriginal_File_Name),
  MEDIO_NATIVE(MH, "Sw_Version", sw_Version, setSW_Version, short),
  MEDIO_NATIVE(MH, "System_Type", system_Type, setSystem_Type, short),
  MEDIO_NATIVE_TEXT(MH, "Serial_Number", serial_Number, setSerial_Number),
  MEDIO_NATIVE(MH, "Scan_Start_Time", scan_Start_Time, setScan_Start_Time, long),
  MEDIO_NATIVE_TEXT(MH, "Isotope_Name", isotope_Name, setIsotope_Name),
  MEDIO_NATIVE(MH, "Isotope_Halflife", isotope_Halflife, setIsotope_Halflife, float),
  MEDIO_NATIVE_TEXT(MH, "Radiopharmaceutical", radiopharmaceutical, setRadiopharmaceutical),
  MEDIO_NATIVE(MH, "Gantry_Tilt", gantry_Tilt, setGantry_Tilt, float),
  MEDIO_NATIVE(MH, "Gantry_Rotation", gantry_Rotation, setGantry_Rotation, float),
  MEDIO_NATIVE(MH, "Bed_Elevation", bed_Elevation, setBed_Elevation, float),
  MEDIO_NATIVE(MH, "Intrinsic_Tilt", intrinsic_Tilt, setIntrinsic_Tilt, float),
  MEDIO_NATIVE(MH, "Wobble_Speed", wobble_Speed, setWobble_Speed, short),
  MEDIO_NATIVE_ENUM(MH, "Transm_Source_Type", transm_Source_Type, setTransm_Source_Type, CECAT7MainHeader::Transm_Source_Type),
  MEDIO_NATIVE(MH, "Distance_Scanned", distance_Scanned, setDistance_Scanned, float),
  MEDIO_NATIVE(MH, "Transaxial_FOV", transaxial_FOV, setTransaxial_FOV, float),
  MEDIO_NATIVE_ENUM(MH, "Angular_Compression", angular_Compression, setAngular_Compression, CECAT7MainHeader::Angular_Compression),
  MEDIO_NATIVE_ENUM(MH, "Coin_Samp_Mode", coin_Samp_Mode, setCoin_Samp_Mode, CECAT7MainHeader::Coin_Samp_Mode),
  MEDIO_NATIVE_ENUM(MH, "Axial_Samp_Mode", axial_Samp_Mode, setAxial_Samp_Mode, CECAT7MainHeader::Axial_Samp_Mode),
  MEDIO_NATIVE_ENUM(MH, "Calibration_Units", calibration_Units, setCalibration_Units, CECAT7MainHeader::Calibration_Units),
  MEDIO_NATIVE_ENUM(MH, "Calibration_Units_Label", calibration_Units_Label, setCalibration_Units_Label, CECAT7MainHeader::Calibration_Units_Label),
  MEDIO_NATIVE_TEXT(MH, "Study_Type", study_Type, setStudy_Type),
  MEDIO_NATIVE_TEXT(MH, "Patient_ID", patient_ID, setPatient_ID),
  MEDIO_NATIVE_TEXT(MH, "Patient_Name", patient_Name, setPatient_Name),
  MEDIO_KEY(MH, "Patient_Sex", toJson(static_cast<char>(h.patient_Sex())),
            char c = 0; if(fromJson(value, c) == false) return false;
            h.setPatient_Sex(static_cast<CECAT7MainHeader::Patient_Sex>(c != 0 ? c : 'U')); return true;),
  MEDIO_KEY(MH, "Patient_Dexterity", toJson(static_cast<char>(h.patient_Dexterity())),
            char c = 0; if(fromJson(value, c) == false) return false;
            h.setPatient_Dexterity(static_cast<CECAT7MainHeader::Patient_Dexterity>(c != 0 ? c : 'U')); return true;),
  MEDIO_NATIVE(MH, "Patient_Age", patient_Age, setPatient_Age, float),
  MEDIO_NATIVE(MH, "Patient_Height", patient_Height, setPatient_Height, float),
  MEDIO_NATIVE(MH, "Patient_Weight", patient_Weight, setPatient_Weight, float),
  MEDIO_KEY(MH, "Patient_Birth_Date", toJson(static_cast<qint32>(h.patient_Birth_Date())),
            int v = 0; if(fromJson(value, v) == false) return false;
            h.setPatient_Birth_Date(static_cast<time_t>(v)); return true;),
  MEDIO_NATIVE_TEXT(MH, "Physician_Name", physician_Name, setPhysician_Name),
  MEDIO_NATIVE_TEXT(MH, "Operator_Name", operator_Name, setOperator_Name),
  MEDIO_NATIVE_TEXT(MH, "Study_Description", study_Description, setStudy_Description),
  MEDIO_NATIVE_ENUM(MH, "Acquisition_Type", acquisition_Type, setAcquisition_Type, CECAT7MainHeader::Acquisition_Type),
  MEDIO_NATIVE_ENUM(MH, "Patient_Orientation", patient_Orientation, setPatient_Orientation, CECAT7MainHeader::Patient_Orientation),
  MEDIO_NATIVE_TEXT(MH, "Facility_Name", facility_Name, setFacility_Name),
  MEDIO_NATIVE(MH, "Lwr_Sctr_Thres", lwr_Sctr_Thres, setLwr_Sctr_Thres, short),
  MEDIO_NATIVE(MH, "Lwr_True_Thres", lwr_True_Thres, setLwr_True_Thres, short),
  MEDIO_NATIVE(MH, "Upr_True_Thres", upr_True_Thres, setUpr_True_Thres, short),
  MEDIO_NATIVE_TEXT(MH, "User_Process_Code", user_Process_Code, setUser_Process_Code),
  MEDIO_NATIVE_ENUM(MH, "Acquisition_Mode", acquisition_Mode, setAcquisition_Mode, CECAT7MainHeader::Acquisition_Mode),
  MEDIO_NATIVE(MH, "Bin_Size", bin_Size, setBin_Size, float),
  MEDIO_NATIVE(MH, "Branching_Fraction", branching_Fraction, setBranching_Fraction, float),
  MEDIO_NATIVE(MH, "Dose_Start_Time", dose_Start_Time, setDose_Start_Time, long),
  MEDIO_NATIVE(MH, "Dosage", dosage, setDosage, float),
  MEDIO_NATIVE(MH, "Well_Counter_Corr_Factor", well_Counter_Corr_Factor, setWell_Counter_Corr_Factor, float),
  MEDIO_NATIVE_TEXT(MH, "Data_Units", data_Units, setData_Units),
  MEDIO_NATIVE_ENUM(MH, "Septa_State", septa_State, setSepta_State, CECAT7MainHeader::Septa_State),
};

//---------------------------------------------------------------------------------------------
// image sub header

const Field imageKeys[] = {
  MEDIO_KEY(SH, "ActualFrameDuration", h.frame_Duration() > 0 ? toJson(h.frame_Duration()) : QJsonValue(QJsonValue::Undefined),
            unsigned int v = 0; if(fromJson(value.isDouble() ? QJsonValue(qRound64(value.toDouble())) : value, v) == false) return false;
            h.setFrame_Duration(v); return true;),
  MEDIO_KEY(SH, "DecayFactor", h.decay_Corr_Fctr() > 0.0f ? floatNumber(h.decay_Corr_Fctr()) : QJsonValue(QJsonValue::Undefined),
            h.setDecay_Corr_Fctr(static_cast<float>(value.toDouble())); return true;),
};

// native fields (without the fields describing the image data: data type,
// dimensions, offsets, scale factor, value range, pixel sizes, frame timing,
// transformation matrix)
const Field imageNative[] = {
  MEDIO_NATIVE(SH, "Recon_Zoom", recon_Zoom, setRecon_Zoom, float),
  MEDIO_NATIVE_ENUM(SH, "Filter_Code", filter_Code, setFilter_Code, CECAT7SubHeaderImage::Filter_Code),
  MEDIO_NATIVE(SH, "X_Resolution", x_Resolution, setX_Resolution, float),
  MEDIO_NATIVE(SH, "Y_Resolution", y_Resolution, setY_Resolution, float),
  MEDIO_NATIVE(SH, "Z_Resolution", z_Resolution, setZ_Resolution, float),
  MEDIO_NATIVE(SH, "Num_R_Elements", num_R_Elements, setNum_R_Elements, float),
  MEDIO_NATIVE(SH, "Num_Angles", num_Angles, setNum_Angles, float),
  MEDIO_NATIVE(SH, "Z_Rotation_Angle", z_Rotation_Angle, setZ_Rotation_Angle, float),
  MEDIO_NATIVE(SH, "Decay_Corr_Fctr", decay_Corr_Fctr, setDecay_Corr_Fctr, float),
  MEDIO_NATIVE(SH, "Processing_Code", processing_Code, setProcessing_Code, unsigned int),
  MEDIO_NATIVE(SH, "Gate_Duration", gate_Duration, setGate_Duration, unsigned int),
  MEDIO_NATIVE(SH, "R_Wave_Offset", r_Wave_Offset, setR_Wave_Offset, unsigned int),
  MEDIO_NATIVE(SH, "Num_Accepted_Beats", num_Accepted_Beats, setNum_Accepted_Beats, unsigned int),
  MEDIO_NATIVE(SH, "Filter_Cutoff_Frequency", filter_Cutoff_Frequency, setFilter_Cutoff_Frequency, float),
  MEDIO_NATIVE(SH, "Filter_Resolution", filter_Resolution, setFilter_Resolution, float),
  MEDIO_NATIVE(SH, "Filter_Ramp_Slope", filter_Ramp_Slope, setFilter_Ramp_Slope, float),
  MEDIO_NATIVE(SH, "Filter_Order", filter_Order, setFilter_Order, short),
  MEDIO_NATIVE(SH, "Filter_Scatter_Fraction", filter_Scatter_Fraction, setFilter_Scatter_Fraction, float),
  MEDIO_NATIVE(SH, "Filter_Scatter_Slope", filter_Scatter_Slope, setFilter_Scatter_Slope, float),
  MEDIO_NATIVE_TEXT(SH, "Annotation", annotation, setAnnotation),
  MEDIO_NATIVE(SH, "RFilter_Cutoff", rfilter_Cutoff, setRFilter_Cutoff, float),
  MEDIO_NATIVE(SH, "RFilter_Resolution", rfilter_Resolution, setRFilter_Resolution, float),
  MEDIO_NATIVE_ENUM(SH, "RFilter_Code", rfilter_Code, setRFilter_Code, CECAT7SubHeaderImage::Filter_Code),
  MEDIO_NATIVE(SH, "RFilter_Order", rfilter_Order, setRFilter_Order, short),
  MEDIO_NATIVE(SH, "ZFilter_Cutoff", zfilter_Cutoff, setZFilter_Cutoff, float),
  MEDIO_NATIVE(SH, "ZFilter_Resolution", zfilter_Resolution, setZFilter_Resolution, float),
  MEDIO_NATIVE_ENUM(SH, "ZFilter_Code", zfilter_Code, setZFilter_Code, CECAT7SubHeaderImage::Filter_Code),
  MEDIO_NATIVE(SH, "ZFilter_Order", zfilter_Order, setZFilter_Order, short),
  MEDIO_NATIVE_ENUM(SH, "Scatter_Type", scatter_Type, setScatter_Type, CECAT7SubHeaderImage::Scatter_Type),
  MEDIO_NATIVE_ENUM(SH, "Recon_Type", recon_Type, setRecon_Type, CECAT7SubHeaderImage::Recon_Type),
  MEDIO_NATIVE(SH, "Recon_Views", recon_Views, setRecon_Views, short),
};

const Adapter mainAdapter = { "ECAT7", mainKeys, sizeof(mainKeys) / sizeof(mainKeys[0]),
                              mainNative, sizeof(mainNative) / sizeof(mainNative[0]) };
const Adapter imageAdapter = { "ECAT7", imageKeys, sizeof(imageKeys) / sizeof(imageKeys[0]),
                               imageNative, sizeof(imageNative) / sizeof(imageNative[0]) };

} // namespace

const Adapter* ecat7MainHeaderAdapter()
{
  return &mainAdapter;
}

const Adapter* ecat7ImageSubHeaderAdapter()
{
  return &imageAdapter;
}

} // namespace MedIOMetadata
