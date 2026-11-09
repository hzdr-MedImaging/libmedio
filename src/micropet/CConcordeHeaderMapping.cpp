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

//! @file CConcordeHeaderMapping.cpp
//! @brief private: format independent header attributes of the microPET headers (see CMedIOHeaderMapping.h)
//!
//! The microPET header gives the units of the dose (mCi, MBq), the subject
//! weight (g, oz, kg, lb) and length (mm, cm, in); values with unknown units
//! are only part of the native section. The isotope half life is given in s,
//! the energy window in keV, the frame duration in s. The scan and injection
//! time are local times without time zone (e.g. "Mon Jan 1 12:00:00 2001")
//! and are given as ISO 8601 date and time without time zone.

#include "CMedIOHeaderMapping.h"
#include "CConcordeMainHeader.h"
#include "CConcordeFrameHeader.h"

#include <QDateTime>
#include <QJsonArray>

namespace MedIOHeaderMapping {

namespace {

typedef CConcordeMainHeader MH;
typedef CConcordeFrameHeader FH;

//---------------------------------------------------------------------------------------------
// conversions

// factor of the dose units to Bq
double doseFactor(CConcordeMainHeader::DoseUnits units)
{
  switch(units) {
    case CConcordeMainHeader::mCi: return 3.7e7;
    case CConcordeMainHeader::MBq: return 1.0e6;
    default: break;
  }
  return 0.0;
}

// factor of the weight units to kg
double weightFactor(CConcordeMainHeader::SubjectWeightUnits units)
{
  switch(units) {
    case CConcordeMainHeader::Grams:     return 0.001;
    case CConcordeMainHeader::Ounces:    return 0.028349523125;
    case CConcordeMainHeader::Kilograms: return 1.0;
    case CConcordeMainHeader::Pounds:    return 0.45359237;
    default: break;
  }
  return 0.0;
}

// factor of the length units to m
double lengthFactor(CConcordeMainHeader::SubjectLengthUnits units)
{
  switch(units) {
    case CConcordeMainHeader::Millimeters: return 0.001;
    case CConcordeMainHeader::Centimeters: return 0.01;
    case CConcordeMainHeader::Inches:      return 0.0254;
    default: break;
  }
  return 0.0;
}

// value in the units of the header, converted with the factor to the DICOM
// unit (Undefined if not set or the units are unknown)
QJsonValue converted(float value, double factor)
{
  return (value > 0.0f && factor > 0.0) ? floatNumber(value, factor) : QJsonValue(QJsonValue::Undefined);
}

// DICOM patient position of the microPET subject orientation
const struct { CConcordeMainHeader::SubjectOrientation orientation; const char* position; } positions[] = {
  { CConcordeMainHeader::FeetFirstProne,  "FFP"  }, { CConcordeMainHeader::HeadFirstProne,  "HFP"  },
  { CConcordeMainHeader::FeetFirstSupine, "FFS"  }, { CConcordeMainHeader::HeadFirstSupine, "HFS"  },
  { CConcordeMainHeader::FeetFirstRight,  "FFDR" }, { CConcordeMainHeader::HeadFirstRight,  "HFDR" },
  { CConcordeMainHeader::FeetFirstLeft,   "FFDL" }, { CConcordeMainHeader::HeadFirstLeft,   "HFDL" },
};

QJsonValue patientPosition(const MH& h)
{
  for(unsigned i=0; i < sizeof(positions) / sizeof(positions[0]); i++) {
    if(positions[i].orientation == h.subjectOrientation())
      return QJsonValue(QString::fromLatin1(positions[i].position));
  }
  return QJsonValue(QJsonValue::Undefined);
}

bool setPatientPosition(MH& h, const QJsonValue& value)
{
  h.setSubjectOrientation(CConcordeMainHeader::UnknownSubjectOrientation);
  if(value.isNull())
    return true;
  for(unsigned i=0; i < sizeof(positions) / sizeof(positions[0]); i++) {
    if(value.toString() == QLatin1String(positions[i].position)) {
      h.setSubjectOrientation(positions[i].orientation);
      return true;
    }
  }
  return false;
}

// local date and time of a microPET time as ISO 8601 without time zone
QJsonValue localDateTime(const QDateTime& time, long unixTime)
{
  if(unixTime == 0 || time.isValid() == false)
    return QJsonValue(QJsonValue::Undefined);
  return QJsonValue(time.toLocalTime().toString("yyyy-MM-ddTHH:mm:ss"));
}

// Unix time of an ISO 8601 date and time: without time zone as local time
// (as the microPET header stores it), else the given time
bool unixTimeOfLocalDateTime(const QJsonValue& value, long& seconds)
{
  seconds = 0;
  if(value.isNull())
    return true;
  QDateTime dt = QDateTime::fromString(value.toString(), Qt::ISODate);
  if(dt.isValid() == false)
    return false;
  seconds = static_cast<long>(dt.toMSecsSinceEpoch() / 1000);
  return true;
}

//---------------------------------------------------------------------------------------------
// main header

const Field mainKeys[] = {
  MEDIO_KEY(MH, "PatientID", text(h.subjectIdentifier()), h.setSubjectIdentifier(value.toString()); return true;),
  MEDIO_KEY(MH, "PatientSize", converted(h.subjectLength(), lengthFactor(h.subjectLengthUnits())),
            if(value.isNull()) { h.setSubjectLength(0.0f); return true; }
            if(lengthFactor(h.subjectLengthUnits()) <= 0.0) h.setSubjectLength_units(CConcordeMainHeader::Centimeters);
            h.setSubjectLength(static_cast<float>(value.toDouble() / lengthFactor(h.subjectLengthUnits()))); return true;),
  MEDIO_KEY(MH, "PatientWeight", converted(h.subjectWeight(), weightFactor(h.subjectWeightUnits())),
            if(value.isNull()) { h.setSubjectWeight(0.0f); return true; }
            if(weightFactor(h.subjectWeightUnits()) <= 0.0) h.setSubjectWeight_units(CConcordeMainHeader::Grams);
            h.setSubjectWeight(static_cast<float>(value.toDouble() / weightFactor(h.subjectWeightUnits()))); return true;),
  MEDIO_KEY(MH, "PatientPosition", patientPosition(h), return setPatientPosition(h, value);),
  MEDIO_KEY(MH, "StudyID", text(h.studyIdentifier()), h.setStudyIdentifier(value.toString()); return true;),
  // read only: the header has no setter of the study
  MEDIO_KEY(MH, "StudyDescription", text(h.study()), return false;),
  MEDIO_KEY(MH, "InstitutionName", text(h.institution()), h.setInstitution(value.toString()); return true;),
  MEDIO_KEY(MH, "OperatorsName", strings(text(h.operatorName())),
            h.setOperatorName(firstString(value).toString()); return value.toArray().size() <= 1;),
  MEDIO_KEY(MH, "ManufacturerModelName", h.model() > 0 ? QJsonValue(QString::number(h.model())) : QJsonValue(QJsonValue::Undefined),
            h.setModel(0); if(value.isNull()) return true;
            bool ok = false; const int model = value.toString().toInt(&ok);
            if(ok == false || model <= 0) return false;
            h.setModel(model); return true;),
  MEDIO_KEY(MH, "AcquisitionDateTime", localDateTime(h.scanTimeQt(), h.scanTime()),
            long s = 0; if(unixTimeOfLocalDateTime(value, s) == false) return false; h.setScanTime(s); return true;),
  MEDIO_KEY(MH, "Radiopharmaceutical", text(h.injectedCompound()), h.setInjectedCompound(value.toString()); return true;),
  MEDIO_KEY(MH, "RadiopharmaceuticalStartDateTime", localDateTime(h.injectionTimeQt(), h.injectionTime()),
            long s = 0; if(unixTimeOfLocalDateTime(value, s) == false) return false; h.setInjectionTime(s); return true;),
  MEDIO_KEY(MH, "RadionuclideTotalDose", converted(h.dose(), doseFactor(h.doseUnits())),
            if(value.isNull()) { h.setDose(0.0f); return true; }
            if(doseFactor(h.doseUnits()) <= 0.0) h.setDoseUnits(CConcordeMainHeader::MBq);
            h.setDose(static_cast<float>(value.toDouble() / doseFactor(h.doseUnits()))); return true;),
  MEDIO_KEY(MH, "RadionuclideHalfLife", h.isotopeHalfTime() > 0.0f ? floatNumber(h.isotopeHalfTime()) : QJsonValue(QJsonValue::Undefined),
            h.setIsotopeHalftime(static_cast<float>(value.toDouble())); return true;),
  MEDIO_KEY(MH, "RadionuclidePositronFraction",
            h.isotopeBranchingFraction() > 0.0f ? floatNumber(h.isotopeBranchingFraction()) : QJsonValue(QJsonValue::Undefined),
            h.setIsotopeBranchingFraction(static_cast<float>(value.toDouble())); return true;),
  MEDIO_KEY(MH, "EnergyWindowLowerLimit", h.lld() > 0.0f ? floatNumber(h.lld()) : QJsonValue(QJsonValue::Undefined),
            h.setLld(static_cast<float>(value.toDouble())); return true;),
  MEDIO_KEY(MH, "EnergyWindowUpperLimit", h.uld() > 0.0f ? floatNumber(h.uld()) : QJsonValue(QJsonValue::Undefined),
            h.setUld(static_cast<float>(value.toDouble())); return true;),
  MEDIO_KEY(MH, "Units",
            h.calibrationUnits() == CConcordeMainHeader::BqPerCC ? QJsonValue(QString("BQML")) : QJsonValue(QJsonValue::Undefined),
            if(value.isNull()) { h.setCalibrationUnits(CConcordeMainHeader::UnknownCalibrationUnits); return true; }
            if(value.toString() != "BQML") return false;
            h.setCalibrationUnits(CConcordeMainHeader::BqPerCC); return true;),
};

//! native text field of a QString getter and setter
#define MEDIO_NATIVE_QSTRING(H, name, getter, setter) \
  MEDIO_KEY(H, name, toJson(h.getter()), QString v; if(fromJson(value, v) == false) return false; h.setter(v); return true;)

// native fields with the names of the microPET header (without the fields
// describing the image data: file type, number of frames, data type and
// order, dimensions, pixel and plane size, bin size, span, ring difference,
// calibration factor)
const Field mainNative[] = {
  MEDIO_NATIVE(MH, "model", model, setModel, int),
  MEDIO_NATIVE_QSTRING(MH, "institution", institution, setInstitution),
  MEDIO_KEY(MH, "study", toJson(h.study()), return false;),   // read only (no setter)
  MEDIO_NATIVE_QSTRING(MH, "file_name", fileName, setFileName),
  MEDIO_NATIVE_ENUM(MH, "acquisition_mode", acquisitionMode, setAcquisitionMode, int),
  MEDIO_NATIVE_ENUM(MH, "bed_motion", bedMotion, setBedMotion, int),
  MEDIO_NATIVE_QSTRING(MH, "isotope", isotope, setIsotope),
  MEDIO_NATIVE(MH, "isotope_half_life", isotopeHalfTime, setIsotopeHalftime, float),
  MEDIO_NATIVE(MH, "isotope_branching_fraction", isotopeBranchingFraction, setIsotopeBranchingFraction, float),
  MEDIO_NATIVE(MH, "transaxial_crystals_per_block", transaxialCrystalsPerBlock, setTransaxialCrystalsPerBlock, int),
  MEDIO_NATIVE(MH, "axial_crystals_per_block", axialCrystalsPerBlock, setAxialCrystalsPerBlock, int),
  MEDIO_NATIVE(MH, "intrinsic_crystal_offset", intrinsicCrystalOffset, setIntrinsicCrystalOffset, int),
  MEDIO_NATIVE(MH, "axial_blocks", axialBlocks, setAxialBlocks, int),
  MEDIO_NATIVE(MH, "axial_crystal_pitch", axialCrystalPitch, setAxialCrystalPitch, float),
  MEDIO_NATIVE(MH, "radius", radius, setRadius, float),
  MEDIO_NATIVE(MH, "radial_fov", radialFov, setRadialFov, float),
  MEDIO_NATIVE(MH, "src_radius", srcRadius, setSrcRadius, float),
  MEDIO_NATIVE(MH, "src_cm_per_rev", srcCmPerRev, setSrcCmPerRev, float),
  MEDIO_NATIVE_ENUM(MH, "tx_src_type", txSrcType, setTxSrcType, int),
  MEDIO_NATIVE(MH, "lld", lld, setLld, float),
  MEDIO_NATIVE(MH, "uld", uld, setUld, float),
  MEDIO_NATIVE_ENUM(MH, "deadtime_correction_applied", deadtimeCorrectionApplied, setDeadtimeCorrectionApplied, int),
  MEDIO_NATIVE_ENUM(MH, "decay_correction_applied", decayCorrectionApplied, setDecayCorrectionApplied, int),
  MEDIO_NATIVE_ENUM(MH, "normalization_applied", normalizationApplied, setNormalizationApplied, int),
  MEDIO_NATIVE_ENUM(MH, "attenuation_applied", attenuationApplied, setAttenuationApplied, int),
  MEDIO_NATIVE_ENUM(MH, "scatter_correction", scatterCorrection, setScatterCorrection, int),
  MEDIO_NATIVE_ENUM(MH, "arc_correction", arcCorrectionApplied, setArcCorrectionApplied, int),
  MEDIO_NATIVE_ENUM(MH, "calibration_units", calibrationUnits, setCalibrationUnits, int),
  MEDIO_NATIVE(MH, "calibration_branching_fraction", calibrationBranchingFraction, setCalibrationBranchingFraction, float),
  MEDIO_NATIVE(MH, "number_of_singles_rates", numberOfSinglesRates, setNumberOfSinglesRates, int),
  MEDIO_NATIVE_QSTRING(MH, "investigator", investigatorName, setInvestigatorName),
  MEDIO_NATIVE_QSTRING(MH, "operator", operatorName, setOperatorName),
  MEDIO_NATIVE_QSTRING(MH, "study_identifier", studyIdentifier, setStudyIdentifier),
  MEDIO_NATIVE(MH, "scan_time", scanTime, setScanTime, long),
  MEDIO_NATIVE_QSTRING(MH, "injected_compound", injectedCompound, setInjectedCompound),
  MEDIO_NATIVE_ENUM(MH, "dose_units", doseUnits, setDoseUnits, int),
  MEDIO_NATIVE(MH, "dose", dose, setDose, float),
  MEDIO_NATIVE(MH, "injection_time", injectionTime, setInjectionTime, long),
  MEDIO_NATIVE(MH, "injection_decay_correction", injectionDecayCorrection, setInjectionDecayCorrection, float),
  MEDIO_NATIVE_QSTRING(MH, "subject_identifier", subjectIdentifier, setSubjectIdentifier),
  MEDIO_NATIVE_QSTRING(MH, "subject_genus", subjectGenus, setSubjectGenus),
  MEDIO_NATIVE_ENUM(MH, "subject_orientation", subjectOrientation, setSubjectOrientation, int),
  MEDIO_NATIVE_ENUM(MH, "subject_length_units", subjectLengthUnits, setSubjectLength_units, int),
  MEDIO_NATIVE(MH, "subject_length", subjectLength, setSubjectLength, float),
  MEDIO_NATIVE_ENUM(MH, "subject_weight_units", subjectWeightUnits, setSubjectWeight_units, int),
  MEDIO_NATIVE(MH, "subject_weight", subjectWeight, setSubjectWeight, float),
  MEDIO_NATIVE_QSTRING(MH, "subject_phenotype", subjectPhenotype, setSubjectPhenotype),
  MEDIO_NATIVE_QSTRING(MH, "study_model", studyModel, setStudyModel),
  MEDIO_NATIVE_QSTRING(MH, "anesthesia", anesthesia, setAnesthesia),
  MEDIO_NATIVE_QSTRING(MH, "analgesia", analgesia, setAnalgesia),
  MEDIO_NATIVE_QSTRING(MH, "other_drugs", otherDrugs, setOtherDrugs),
  MEDIO_NATIVE_QSTRING(MH, "food_access", foodAccess, setFoodAccess),
  MEDIO_NATIVE_QSTRING(MH, "water_access", waterAccess, setWaterAccess),
};

//---------------------------------------------------------------------------------------------
// frame header

const Field frameKeys[] = {
  MEDIO_KEY(FH, "ActualFrameDuration", h.frameDuration() > 0.0f ? floatNumber(h.frameDuration(), 1000.0) : QJsonValue(QJsonValue::Undefined),
            h.setFrameDuration(static_cast<float>(value.toDouble() / 1000.0)); return true;),
  MEDIO_KEY(FH, "DecayFactor", h.decayCorrection() > 0.0f ? floatNumber(h.decayCorrection()) : QJsonValue(QJsonValue::Undefined),
            h.setDecayCorrection(static_cast<float>(value.toDouble())); return true;),
  MEDIO_KEY(FH, "DeadTimeFactor", h.deadTimeCorrection() > 0.0f ? floatNumber(h.deadTimeCorrection()) : QJsonValue(QJsonValue::Undefined),
            h.setDeadtimeCorrection(static_cast<float>(value.toDouble())); return true;),
};

// native fields with the names of the microPET header (without the fields
// describing the image data: frame, gate and bed number, bed offsets, data
// file pointer, frame timing, scale factor and value range)
const Field frameNative[] = {
  MEDIO_NATIVE(FH, "event_type", eventType, setEventType, int),
  MEDIO_NATIVE(FH, "deadtime_correction", deadTimeCorrection, setDeadtimeCorrection, float),
  MEDIO_NATIVE(FH, "decay_correction", decayCorrection, setDecayCorrection, float),
  MEDIO_NATIVE_QSTRING(FH, "prompts", prompts, setPrompts),
  MEDIO_NATIVE_QSTRING(FH, "delays", delays, setDelays),
  MEDIO_NATIVE_QSTRING(FH, "trues", trues, setTrues),
  MEDIO_NATIVE(FH, "prompts_rate", promptsRate, setPromptsRate, int),
  MEDIO_NATIVE(FH, "delays_rate", delaysRate, setDelaysRate, int),
};

#undef MEDIO_NATIVE_QSTRING

const Adapter mainAdapter = { "microPET", mainKeys, sizeof(mainKeys) / sizeof(mainKeys[0]),
                              mainNative, sizeof(mainNative) / sizeof(mainNative[0]) };
const Adapter frameAdapter = { "microPET", frameKeys, sizeof(frameKeys) / sizeof(frameKeys[0]),
                               frameNative, sizeof(frameNative) / sizeof(frameNative[0]) };

} // namespace

const Adapter* concordeMainHeaderAdapter()
{
  return &mainAdapter;
}

const Adapter* concordeFrameHeaderAdapter()
{
  return &frameAdapter;
}

} // namespace MedIOHeaderMapping
