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

//! @file CPhilipsMetadata.cpp
//! @brief private: format independent metadata of the Philips headers (see CMedIOMetadataMap.h)
//!
//! Units of the Philips main header: patient height in mm, weight in g,
//! activity in MBq, half life in min, energy window in keV, times as Unix
//! time (UTC), birth date as year, month and day.
//!
//! The physical values of slices scaled to SUV (suvscl) depend on the
//! activity, the injection and acquisition time, the half life and the weight
//! of the main header (CPhilipsSubHeaderImage::ecat_Scale_Factor()): changing
//! them changes the values of the slices written before.

#include "CMedIOMetadataMap.h"
#include "CPhilipsMainHeader.h"
#include "CPhilipsSubHeader.h"

#include <QJsonArray>

#include <cmath>

namespace MedIOMetadata {

namespace {

typedef CPhilipsMainHeader MH;
typedef CPhilipsSubHeader SH;

//---------------------------------------------------------------------------------------------
// conversions

// DICOM patient position of the Philips patient orientation
const struct { CPhilipsMainHeader::Patient_Orientation_hf hf; CPhilipsMainHeader::Patient_Orientation_ps ps; const char* position; } positions[] = {
  { CPhilipsMainHeader::Head_First, CPhilipsMainHeader::Supine, "HFS"  },
  { CPhilipsMainHeader::Head_First, CPhilipsMainHeader::Prone,  "HFP"  },
  { CPhilipsMainHeader::Head_First, CPhilipsMainHeader::Left,   "HFDL" },
  { CPhilipsMainHeader::Head_First, CPhilipsMainHeader::Right,  "HFDR" },
  { CPhilipsMainHeader::Feet_First, CPhilipsMainHeader::Supine, "FFS"  },
  { CPhilipsMainHeader::Feet_First, CPhilipsMainHeader::Prone,  "FFP"  },
  { CPhilipsMainHeader::Feet_First, CPhilipsMainHeader::Left,   "FFDL" },
  { CPhilipsMainHeader::Feet_First, CPhilipsMainHeader::Right,  "FFDR" },
};

QJsonValue patientPosition(const MH& h)
{
  for(unsigned i=0; i < sizeof(positions) / sizeof(positions[0]); i++) {
    if(positions[i].hf == h.orient_hf() && positions[i].ps == h.orient_ps())
      return QJsonValue(QString::fromLatin1(positions[i].position));
  }
  return QJsonValue(QJsonValue::Undefined);
}

bool setPatientPosition(MH& h, const QJsonValue& value)
{
  h.setOrient_hf(CPhilipsMainHeader::Undefined_Orientation_hf);
  h.setOrient_ps(CPhilipsMainHeader::Undefined_Orientation_ps);
  if(value.isNull())
    return true;
  for(unsigned i=0; i < sizeof(positions) / sizeof(positions[0]); i++) {
    if(value.toString() == QLatin1String(positions[i].position)) {
      h.setOrient_hf(positions[i].hf);
      h.setOrient_ps(positions[i].ps);
      return true;
    }
  }
  return false;
}

// DICOM radiopharmaceutical route (text) of the Philips route
const struct { CPhilipsMainHeader::Route_Type route; const char* text; } routes[] = {
  { CPhilipsMainHeader::Intravenous, "Intravenous" },
  { CPhilipsMainHeader::Intra_Arterial, "Intra-arterial" },
  { CPhilipsMainHeader::Oral, "Oral" },
  { CPhilipsMainHeader::By_Inhalation, "By inhalation" },
  { CPhilipsMainHeader::Intramuscular, "Intramuscular" },
  { CPhilipsMainHeader::Subcutaneous, "Subcutaneous" },
  { CPhilipsMainHeader::Intracutaneous, "Intracutaneous" },
  { CPhilipsMainHeader::Intraperitoneal, "Intraperitoneal" },
  { CPhilipsMainHeader::Intramedullary, "Intramedullary" },
  { CPhilipsMainHeader::Intrathecal, "Intrathecal" },
  { CPhilipsMainHeader::Intra_Articular, "Intra-articular" },
  { CPhilipsMainHeader::Intraepithelial, "Intraepithelial" },
  { CPhilipsMainHeader::Topical, "Topical" },
  { CPhilipsMainHeader::Transluminal, "Transluminal" },
  { CPhilipsMainHeader::Intraluminal, "Intraluminal" },
  { CPhilipsMainHeader::Extraluminal, "Extraluminal" },
  { CPhilipsMainHeader::Per_Rectum, "Per rectum" },
  { CPhilipsMainHeader::Vaginal_Route, "Vaginal" },
  { CPhilipsMainHeader::OtherRoute, "Other" },
};

QJsonValue route(const MH& h)
{
  for(unsigned i=0; i < sizeof(routes) / sizeof(routes[0]); i++) {
    if(routes[i].route == h.route())
      return QJsonValue(QString::fromLatin1(routes[i].text));
  }
  return QJsonValue(QJsonValue::Undefined);
}

bool setRoute(MH& h, const QJsonValue& value)
{
  h.setRoute(CPhilipsMainHeader::UndefinedRoute);
  if(value.isNull())
    return true;
  for(unsigned i=0; i < sizeof(routes) / sizeof(routes[0]); i++) {
    if(value.toString().compare(QLatin1String(routes[i].text), Qt::CaseInsensitive) == 0) {
      h.setRoute(routes[i].route);
      return true;
    }
  }
  return false;
}

// integer of a value multiplied with factor (false if it does not fit into T)
template<typename T>
bool scaledInteger(const QJsonValue& value, double factor, T& out)
{
  if(value.isNull()) { out = 0; return true; }
  return fromJson(QJsonValue(static_cast<double>(std::floor(value.toDouble() * factor + 0.5))), out);
}

QJsonValue firstText(const char* a, const char* b)
{
  const QJsonValue t = text(a);
  return t.isUndefined() ? text(b) : t;
}

//---------------------------------------------------------------------------------------------
// main header

const Field mainKeys[] = {
  MEDIO_KEY(MH, "PatientName", firstText(h.Dpat_name(), h.patient_name()),
            h.setDpat_name(latin1(value).constData()); h.setPatient_name(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "PatientID", firstText(h.Dpat_id(), h.ssn()),
            h.setDpat_id(latin1(value).constData()); h.setSsn(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "PatientBirthDate",
            h.patient_Birth_Date_Qt().isValid() ? QJsonValue(h.patient_Birth_Date_Qt().toString(Qt::ISODate)) : QJsonValue(QJsonValue::Undefined),
            const QDate d = date(value);
            h.setBthyr(static_cast<short>(d.isValid() ? d.year() : 0));
            h.setBthmo(static_cast<short>(d.isValid() ? d.month() : 0));
            h.setBthday(static_cast<short>(d.isValid() ? d.day() : 0)); return true;),
  MEDIO_KEY(MH, "PatientSex",
            (h.sex() == 'M' || h.sex() == 'F' || h.sex() == 'O') ? toJson(h.sex()) : QJsonValue(QJsonValue::Undefined),
            const QString s = value.toString();
            h.setSex(0);
            if(value.isNull()) return true;
            if(s != "M" && s != "F" && s != "O") return false;
            h.setSex(s.toLatin1()[0]); return true;),
  MEDIO_KEY(MH, "PatientSize", h.height() > 0 ? number(h.height() / 1000.0) : QJsonValue(QJsonValue::Undefined),
            short v = 0; if(scaledInteger(value, 1000.0, v) == false) return false; h.setHeight(v); return true;),
  MEDIO_KEY(MH, "PatientWeight", h.weight() > 0 ? number(h.weight() / 1000.0) : QJsonValue(QJsonValue::Undefined),
            int v = 0; if(scaledInteger(value, 1000.0, v) == false) return false; h.setWeight(v); return true;),
  MEDIO_KEY(MH, "PatientPosition", patientPosition(h), return setPatientPosition(h, value);),
  MEDIO_KEY(MH, "StudyInstanceUID", text(h.study_uid()), h.setStudy_uid(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "SeriesInstanceUID", text(h.series_uid()), h.setSeries_uid(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "FrameOfReferenceUID", text(h.frame_ref_uid()), h.setFrame_ref_uid(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "StudyID", text(h.study_id()), h.setStudy_id(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "SeriesNumber", h.series_number() > 0 ? toJson(h.series_number()) : QJsonValue(QJsonValue::Undefined),
            int v = 0; if(fromJson(value, v) == false) return false; h.setSeries_number(v); return true;),
  MEDIO_KEY(MH, "AccessionNumber", text(h.accNum()), h.setAccNum(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "AcquisitionDateTime", dateTimeOfUnixTime(h.acq_date_time()),
            qint64 s = 0; if(unixTimeOfDateTime(value, s) == false) return false;
            h.setAcq_date_time(static_cast<time_t>(s)); return true;),
  MEDIO_KEY(MH, "SeriesDescription", text(h.series_desc()), h.setSeries_desc(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "InstitutionName", text(h.scnOrigin()), h.setScnOrigin(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "ReferringPhysicianName", text(h.ref_phys()), h.setRef_phys(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "RequestingPhysician", text(h.req_phys()), h.setReq_phys(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "OperatorsName", strings(text(h.operator_name())),
            h.setOperator_name(latin1(firstString(value)).constData()); return value.toArray().size() <= 1;),
  MEDIO_KEY(MH, "ManufacturerModelName", text(h.Dmanuf_model()), h.setDmanuf_model(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "DeviceSerialNumber", text(h.Dserial_number()), h.setDserial_number(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "SoftwareVersions", strings(text(h.pet_software_version())),
            h.setPet_software_version(latin1(firstString(value)).constData()); return value.toArray().size() <= 1;),
  MEDIO_KEY(MH, "Radiopharmaceutical", text(h.radiopharm_name()), h.setRadiopharm_name(latin1(value).constData()); return true;),
  MEDIO_KEY(MH, "RadiopharmaceuticalRoute", route(h), return setRoute(h, value);),
  MEDIO_KEY(MH, "RadiopharmaceuticalStartDateTime", dateTimeOfUnixTime(h.injection_date_time()),
            qint64 s = 0; if(unixTimeOfDateTime(value, s) == false) return false;
            h.setInjection_date_time(static_cast<time_t>(s)); return true;),
  MEDIO_KEY(MH, "RadionuclideTotalDose", h.activity() > 0.0f ? floatNumber(h.activity(), 1.0e6) : QJsonValue(QJsonValue::Undefined),
            h.setActivity(static_cast<float>(value.toDouble() / 1.0e6)); return true;),
  MEDIO_KEY(MH, "RadionuclideHalfLife", h.halfLife() > 0.0f ? floatNumber(h.halfLife(), 60.0) : QJsonValue(QJsonValue::Undefined),
            h.setHalfLife(static_cast<float>(value.toDouble() / 60.0)); return true;),
  MEDIO_KEY(MH, "RadionuclidePositronFraction",
            h.positron_fraction() > 0.0f ? floatNumber(h.positron_fraction()) : QJsonValue(QJsonValue::Undefined),
            h.setPositron_fraction(static_cast<float>(value.toDouble())); return true;),
  MEDIO_KEY(MH, "RadionuclideCodeSequence",
            code(QString::fromLatin1(h.isotope_code_value()), QString::fromLatin1(h.isotope_coding_scheme_designator()),
                 QString::fromLatin1(h.isotope_coding_scheme_version()), QString::fromLatin1(h.isotope_code_meaning())),
            const QJsonObject c = value.toObject();
            h.setIsotope_code_value(latin1(c["CodeValue"]).constData());
            h.setIsotope_coding_scheme_designator(latin1(c["CodingSchemeDesignator"]).constData());
            h.setIsotope_coding_scheme_version(latin1(c["CodingSchemeVersion"]).constData());
            h.setIsotope_code_meaning(latin1(c["CodeMeaning"]).constData()); return true;),
  MEDIO_KEY(MH, "RadiopharmaceuticalCodeSequence",
            code(QString::fromLatin1(h.radiopharm_code_value()), QString::fromLatin1(h.radiopharm_coding_scheme_designator()),
                 QString::fromLatin1(h.radiopharm_coding_scheme_version()), QString::fromLatin1(h.radiopharm_code_meaning())),
            const QJsonObject c = value.toObject();
            h.setRadiopharm_code_value(latin1(c["CodeValue"]).constData());
            h.setRadiopharm_coding_scheme_designator(latin1(c["CodingSchemeDesignator"]).constData());
            h.setRadiopharm_coding_scheme_version(latin1(c["CodingSchemeVersion"]).constData());
            h.setRadiopharm_code_meaning(latin1(c["CodeMeaning"]).constData()); return true;),
  MEDIO_KEY(MH, "EnergyWindowLowerLimit", h.eglob_low() > 0 ? toJson(h.eglob_low()) : QJsonValue(QJsonValue::Undefined),
            short v = 0; if(scaledInteger(value, 1.0, v) == false) return false;
            h.setEglob_low(v); return value.isNull() || value.toDouble() == v;),
  MEDIO_KEY(MH, "EnergyWindowUpperLimit", h.eglob_up() > 0 ? toJson(h.eglob_up()) : QJsonValue(QJsonValue::Undefined),
            short v = 0; if(scaledInteger(value, 1.0, v) == false) return false;
            h.setEglob_up(v); return value.isNull() || value.toDouble() == v;),
};

// native fields (without the fields describing the image data: file format
// version and type, byte order, sizes, slices and frames, scaling, geometry,
// bed positions and PET/CT alignment)
const Field mainNative[] = {
  MEDIO_NATIVE(MH, "scan_geom", scan_geom, setScan_geom, short),
  MEDIO_NATIVE(MH, "hw_config", hw_config, setHw_config, short),
  MEDIO_NATIVE(MH, "edit_flag", edit_flag, setEdit_flag, short),
  MEDIO_NATIVE(MH, "minTransXtalDiff", minTransXtalDiff, setMinTransXtalDiff, int),
  MEDIO_NATIVE(MH, "tofTstampScale", tofTstampScale, setTofTstampScale, float),
  MEDIO_NATIVE(MH, "dep_daycre", dep_daycre, setDep_daycre, short),
  MEDIO_NATIVE(MH, "dep_mocre", dep_mocre, setDep_mocre, short),
  MEDIO_NATIVE(MH, "dep_yrcre", dep_yrcre, setDep_yrcre, short),
  MEDIO_NATIVE(MH, "dep_hrcre", dep_hrcre, setDep_hrcre, short),
  MEDIO_NATIVE(MH, "dep_mincre", dep_mincre, setDep_mincre, short),
  MEDIO_NATIVE(MH, "dep_seccre", dep_seccre, setDep_seccre, short),
  MEDIO_NATIVE(MH, "duratn", duratn, setDuratn, short),
  MEDIO_NATIVE(MH, "detectorRadius", detectorRadius, setDetectorRadius, float),
  MEDIO_NATIVE(MH, "virtualXtal", virtualXtal, setVirtualXtal, bool),
  MEDIO_NATIVE(MH, "polygonSides", polygonSides, setPolygonSides, short),
  MEDIO_NATIVE(MH, "xtalsPerSide", xtalsPerSide, setXtalsPerSide, short),
  MEDIO_NATIVE(MH, "nXtalRows", nXtalRows, setNXtalRows, short),
  MEDIO_NATIVE(MH, "crystalThickness", crystalThickness, setCrystalThickness, float),
  MEDIO_NATIVE(MH, "xXtalPitch", xXtalPitch, setXXtalPitch, float),
  MEDIO_NATIVE(MH, "zXtalPitch", zXtalPitch, setZXtalPitch, float),
  MEDIO_NATIVE(MH, "axialFov", axialFov, setAxialFov, float),
  MEDIO_NATIVE_ENUM(MH, "rphiType", rphiType, setRphiType, CPhilipsMainHeader::RPhi_Type),
  MEDIO_NATIVE_ENUM(MH, "sliceType", sliceType, setSliceType, CPhilipsMainHeader::Slice_Type),
  MEDIO_NATIVE_ENUM(MH, "delayType", delayType, setDelayType, CPhilipsMainHeader::Delay_Type),
  MEDIO_NATIVE_ENUM(MH, "pattyp", pattyp, setPattyp, CPhilipsMainHeader::Scan_Type),
  MEDIO_NATIVE_ENUM(MH, "scntyp", scntyp, setScntyp, CPhilipsMainHeader::Acquisition_Type),
  MEDIO_NATIVE_ENUM(MH, "dep_isotop", dep_isotop, setDep_isotop, CPhilipsMainHeader::Isotope),
  MEDIO_NATIVE(MH, "dep_injtim", dep_injtim, setDep_injtim, short),
  MEDIO_NATIVE(MH, "polygonVertAt0deg", polygonVertAt0deg, setPolygonVertAt0deg, float),
  MEDIO_NATIVE(MH, "bthday", bthday, setBthday, short),
  MEDIO_NATIVE(MH, "bthmo", bthmo, setBthmo, short),
  MEDIO_NATIVE(MH, "bthyr", bthyr, setBthyr, short),
  MEDIO_NATIVE_TEXT(MH, "ssn", ssn, setSsn),
  MEDIO_NATIVE(MH, "petnum", petnum, setPetnum, short),
  MEDIO_NATIVE(MH, "activity", activity, setActivity, float),
  MEDIO_NATIVE(MH, "weight", weight, setWeight, int),
  MEDIO_NATIVE(MH, "dep_hrinj", dep_hrinj, setDep_hrinj, short),
  MEDIO_NATIVE(MH, "dep_mininj", dep_mininj, setDep_mininj, short),
  MEDIO_NATIVE(MH, "srcRadius", srcRadius, setSrcRadius, float),
  MEDIO_NATIVE(MH, "srcZpos", srcZpos, setSrcZpos, float),
  MEDIO_NATIVE(MH, "halfLife", halfLife, setHalfLife, float),
  MEDIO_NATIVE(MH, "nevent", nevent, setNevent, float),
  MEDIO_NATIVE(MH, "eglob_low", eglob_low, setEglob_low, short),
  MEDIO_NATIVE(MH, "eglob_up", eglob_up, setEglob_up, short),
  MEDIO_NATIVE(MH, "eloc_low", eloc_low, setEloc_low, short),
  MEDIO_NATIVE(MH, "eloc_up", eloc_up, setEloc_up, short),
  MEDIO_NATIVE_ENUM(MH, "orient_hf", orient_hf, setOrient_hf, CPhilipsMainHeader::Patient_Orientation_hf),
  MEDIO_NATIVE_TEXT(MH, "scan_swrel", scan_swrel, setScan_swrel),
  MEDIO_NATIVE(MH, "petct_sepdist", petct_sepdist, setPetct_sepdist, short),
  MEDIO_NATIVE(MH, "petct_landmrk", petct_landmrk, setPetct_landmrk, short),
  MEDIO_NATIVE_ENUM(MH, "orient_ps", orient_ps, setOrient_ps, CPhilipsMainHeader::Patient_Orientation_ps),
  MEDIO_NATIVE(MH, "frontLeadDiameter", frontLeadDiameter, setFrontLeadDiameter, float),
  MEDIO_NATIVE(MH, "backLeadDiameter", backLeadDiameter, setBackLeadDiameter, float),
  MEDIO_NATIVE(MH, "leadSeparation", leadSeparation, setLeadSeparation, float),
  MEDIO_NATIVE(MH, "ndelays", ndelays, setNdelays, float),
  MEDIO_NATIVE_ENUM(MH, "petct_valid", petct_valid, setPetct_valid, CPhilipsMainHeader::PETCT_Valid_Type),
  MEDIO_NATIVE_TEXT(MH, "fctrfil", fctrfil, setFctrfil),
  MEDIO_NATIVE_TEXT(MH, "baselin", baselin, setBaselin),
  MEDIO_NATIVE_TEXT(MH, "dstpkfl", dstpkfl, setDstpkfl),
  MEDIO_NATIVE_TEXT(MH, "dep_aqprotocol_name", dep_aqprotocol_name, setDep_Aqprotocol_name),
  MEDIO_NATIVE_ENUM(MH, "aqprotocol_type", aqprotocol_type, setAqprotocol_type, CPhilipsMainHeader::Acquisition_Protocol_Type),
  MEDIO_NATIVE_TEXT(MH, "patient_name", patient_name, setPatient_name),
  MEDIO_NATIVE_ENUM(MH, "rebin_type", rebin_type, setRebin_type, CPhilipsMainHeader::Rebin_Type),
  MEDIO_NATIVE_TEXT(MH, "scnOrigin", scnOrigin, setScnOrigin),
  MEDIO_NATIVE_TEXT(MH, "accNum", accNum, setAccNum),
  MEDIO_NATIVE(MH, "movementCoinc", movementCoinc, setMovementCoinc, short),
  MEDIO_NATIVE(MH, "movementSing", movementSing, setMovementSing, short),
  MEDIO_NATIVE(MH, "crbTstampPeriod", crbTstampPeriod, setCrbTstampPeriod, short),
  MEDIO_NATIVE_TEXT(MH, "Dpat_name", Dpat_name, setDpat_name),
  MEDIO_NATIVE_TEXT(MH, "Dpat_id", Dpat_id, setDpat_id),
  MEDIO_NATIVE_TEXT(MH, "study_uid", study_uid, setStudy_uid),
  MEDIO_NATIVE_TEXT(MH, "series_uid", series_uid, setSeries_uid),
  MEDIO_NATIVE_TEXT(MH, "view_code", view_code, setView_code),
  MEDIO_NATIVE_TEXT(MH, "sortproto_name", sortproto_name, setSortproto_name),
  MEDIO_NATIVE_ENUM(MH, "route", route, setRoute, CPhilipsMainHeader::Route_Type),
  MEDIO_NATIVE_ENUM(MH, "dep_pharm", dep_pharm, setDep_pharm, CPhilipsMainHeader::Pharm_Type),
  MEDIO_NATIVE_TEXT(MH, "req_phys", req_phys, setReq_phys),
  MEDIO_NATIVE_ENUM(MH, "card_phstate", card_phstate, setCard_phstate, CPhilipsMainHeader::Card_Ph_State),
  MEDIO_NATIVE(MH, "assay_date", assay_date, setAssay_date, int),
  MEDIO_NATIVE(MH, "assay_time", assay_time, setAssay_time, int),
  MEDIO_NATIVE_TEXT(MH, "series_desc", series_desc, setSeries_desc),
  MEDIO_NATIVE(MH, "height", height, setHeight, short),
  MEDIO_NATIVE(MH, "dep_abundance", dep_abundance, setDep_abundance, short),
  MEDIO_NATIVE(MH, "acq_date_time", acq_date_time, setAcq_date_time, time_t),
  MEDIO_NATIVE(MH, "study_date_time", study_date_time, setStudy_date_time, time_t),
  MEDIO_NATIVE(MH, "injection_date_time", injection_date_time, setInjection_date_time, time_t),
  MEDIO_NATIVE(MH, "file_create_date_time", file_create_date_time, setFile_create_date_time, time_t),
  MEDIO_NATIVE_ENUM(MH, "resp_trig_loc", resp_trig_loc, setResp_trig_loc, CPhilipsMainHeader::Respiration_Trigger_Location),
  MEDIO_NATIVE_ENUM(MH, "card_arrhythmia_rej_tech", card_arrhythmia_rej_tech, setCard_arrhythmia_rej_tech, CPhilipsMainHeader::Card_Arrhythmia_Rej_Tech),
  MEDIO_NATIVE(MH, "window_center", window_center, setWindow_center, float),
  MEDIO_NATIVE(MH, "window_width", window_width, setWindow_width, float),
  MEDIO_NATIVE(MH, "positron_fraction", positron_fraction, setPositron_fraction, float),
  MEDIO_NATIVE(MH, "pulmo_trig_of", pulmo_trig_of, setPulmo_trig_of, short),
  MEDIO_NATIVE(MH, "resp_trig_threshold", resp_trig_threshold, setResp_trig_threshold, short),
  MEDIO_NATIVE(MH, "resp_phase_duration", resp_phase_duration, setResp_phase_duration, short),
  MEDIO_NATIVE(MH, "resp_phase_offset", resp_phase_offset, setResp_phase_offset, short),
  MEDIO_NATIVE_ENUM(MH, "window_units", window_units, setWindow_units, CPhilipsMainHeader::Window_Units),
  MEDIO_NATIVE_TEXT(MH, "ref_phys", ref_phys, setRef_phys),
  MEDIO_NATIVE_TEXT(MH, "study_id", study_id, setStudy_id),
  MEDIO_NATIVE(MH, "pt_tbl_vers", pt_tbl_vers, setPt_tbl_vers, short),
  MEDIO_NATIVE(MH, "sex", sex, setSex, char),
  MEDIO_NATIVE(MH, "table_height", table_height, setTable_height, float),
  MEDIO_NATIVE(MH, "card_bt_rej", card_bt_rej, setCard_bt_rej, bool),
  MEDIO_NATIVE_ENUM(MH, "card_fr_type", card_fr_type, setCard_fr_type, CPhilipsMainHeader::Card_Fr_Type),
  MEDIO_NATIVE_TEXT(MH, "Dmanuf_model", Dmanuf_model, setDmanuf_model),
  MEDIO_NATIVE_TEXT(MH, "Dimg_type", Dimg_type, setDimg_type),
  MEDIO_NATIVE(MH, "der_filled", der_filled, setDer_filled, bool),
  MEDIO_NATIVE(MH, "series_number", series_number, setSeries_number, int),
  MEDIO_NATIVE(MH, "dep_study_date", dep_study_date, setDep_study_date, int),
  MEDIO_NATIVE(MH, "dep_study_time", dep_study_time, setDep_study_time, int),
  MEDIO_NATIVE(MH, "dep_acq_time", dep_acq_time, setDep_acq_time, int),
  MEDIO_NATIVE_ENUM(MH, "card_slc_dir", card_slc_dir, setCard_slc_dir, CPhilipsMainHeader::Card_Slc_Dir),
  MEDIO_NATIVE(MH, "card_skip_msec", card_skip_msec, setCard_skip_msec, int),
  MEDIO_NATIVE(MH, "card_skip_counts", card_skip_counts, setCard_skip_counts, int),
  MEDIO_NATIVE(MH, "card_dur_msec", card_dur_msec, setCard_dur_msec, int),
  MEDIO_NATIVE(MH, "card_dur_counts", card_dur_counts, setCard_dur_counts, int),
  MEDIO_NATIVE(MH, "card_beats_tot", card_beats_tot, setCard_beats_tot, int),
  MEDIO_NATIVE(MH, "card_beats_acc", card_beats_acc, setCard_beats_acc, int),
  MEDIO_NATIVE(MH, "card_skip_beats", card_skip_beats, setCard_skip_beats, short),
  MEDIO_NATIVE(MH, "pvc_threshold", pvc_threshold, setPvc_threshold, short),
  MEDIO_NATIVE(MH, "dep_acq_date", dep_acq_date, setDep_acq_date, int),
  MEDIO_NATIVE_TEXT(MH, "radiopharm_name", radiopharm_name, setRadiopharm_name),
  MEDIO_NATIVE_TEXT(MH, "Dserial_number", Dserial_number, setDserial_number),
  MEDIO_NATIVE_TEXT(MH, "attncor_label", attncor_label, setAttncor_label),
  MEDIO_NATIVE_TEXT(MH, "operator_name", operator_name, setOperator_name),
  MEDIO_NATIVE_TEXT(MH, "contr_bolus_agent", contr_bolus_agent, setContr_bolus_agent),
  MEDIO_NATIVE_TEXT(MH, "sop_uid", sop_uid, setSop_uid),
  MEDIO_NATIVE_TEXT(MH, "frame_ref_uid", frame_ref_uid, setFrame_ref_uid),
  MEDIO_NATIVE_TEXT(MH, "pps_file", pps_file, setPps_file),
  MEDIO_NATIVE_TEXT(MH, "worklist_file", worklist_file, setWorklist_file),
  MEDIO_NATIVE_TEXT(MH, "recon_swrel", recon_swrel, setRecon_swrel),
  MEDIO_NATIVE_TEXT(MH, "analy_swrel", analy_swrel, setAnaly_swrel),
  MEDIO_NATIVE_TEXT(MH, "recprotocol_name", recprotocol_name, setRecprotocol_name),
  MEDIO_NATIVE_TEXT(MH, "insinofile", insinofile, setInsinofile),
  MEDIO_NATIVE_ENUM(MH, "fltr_type", fltr_type, setFltr_type, CPhilipsMainHeader::Fltr_Type),
  MEDIO_NATIVE(MH, "smoth", smoth, setSmoth, short),
  MEDIO_NATIVE(MH, "scatcorr_type", scatcorr_type, setScatcorr_type, short),
  MEDIO_NATIVE(MH, "edge_exp", edge_exp, setEdge_exp, short),
  MEDIO_NATIVE(MH, "bckang_avg", bckang_avg, setBckang_avg, short),
  MEDIO_NATIVE(MH, "bck_coeff", bck_coeff, setBck_coeff, float),
  MEDIO_NATIVE(MH, "bck_wid", bck_wid, setBck_wid, short),
  MEDIO_NATIVE_ENUM(MH, "attncor_type", attncor_type, setAttncor_type, CPhilipsMainHeader::AttnCorr_Type),
  MEDIO_NATIVE_ENUM(MH, "attncor_ecc", attncor_ecc, setAttncor_ecc, CPhilipsMainHeader::AttnCorrEcc_Type),
  MEDIO_NATIVE(MH, "attn_coeff", attn_coeff, setAttn_coeff, float),
  MEDIO_NATIVE_TEXT(MH, "regfile", regfile, setRegfile),
  MEDIO_NATIVE_TEXT(MH, "proc_transinofile", proc_transinofile, setProc_transinofile),
  MEDIO_NATIVE(MH, "skull_comp", skull_comp, setSkull_comp, float),
  MEDIO_NATIVE_ENUM(MH, "norm_type", norm_type, setNorm_type, CPhilipsMainHeader::Norm_Type),
  MEDIO_NATIVE(MH, "smp_norm", smp_norm, setSmp_norm, bool),
  MEDIO_NATIVE_TEXT(MH, "axnfile", axnfile, setAxnfile),
  MEDIO_NATIVE_TEXT(MH, "effnormfile", effnormfile, setEffnormfile),
  MEDIO_NATIVE(MH, "gap_comp", gap_comp, setGap_comp, bool),
  MEDIO_NATIVE_ENUM(MH, "algtype_em", algtype_em, setAlgtype_em, CPhilipsMainHeader::Alg_Type),
  MEDIO_NATIVE(MH, "num_iter", num_iter, setNum_iter, short),
  MEDIO_NATIVE(MH, "iter_em", iter_em, setIter_em, short),
  MEDIO_NATIVE(MH, "subset_em", subset_em, setSubset_em, short),
  MEDIO_NATIVE(MH, "nsmooth_em", nsmooth_em, setNsmooth_em, short),
  MEDIO_NATIVE(MH, "nrepeat_em", nrepeat_em, setNrepeat_em, short),
  MEDIO_NATIVE(MH, "bckslc_avg", bckslc_avg, setBckslc_avg, short),
  MEDIO_NATIVE(MH, "dead_corr", dead_corr, setDead_corr, bool),
  MEDIO_NATIVE(MH, "decay_corr", decay_corr, setDecay_corr, bool),
  MEDIO_NATIVE_TEXT(MH, "transinofile", transinofile, setTransinofile),
  MEDIO_NATIVE_TEXT(MH, "blnksinofile", blnksinofile, setBlnksinofile),
  MEDIO_NATIVE(MH, "tran_ray_fwhm", tran_ray_fwhm, setTran_ray_fwhm, float),
  MEDIO_NATIVE(MH, "tran_axl_fwhm", tran_axl_fwhm, setTran_axl_fwhm, float),
  MEDIO_NATIVE(MH, "surv_mask", surv_mask, setSurv_mask, short),
  MEDIO_NATIVE_ENUM(MH, "preflt_type", preflt_type, setPreflt_type, CPhilipsMainHeader::PreFlt_Type),
  // setPostflt_type() takes the type of setPreflt_type() (public API, kept as it is)
  MEDIO_NATIVE_ENUM(MH, "postflt_type", postflt_type, setPostflt_type, CPhilipsMainHeader::PreFlt_Type),
  MEDIO_NATIVE_ENUM(MH, "tr_posttyp", tr_posttyp, setTr_posttyp, CPhilipsMainHeader::PostProc_Type),
  MEDIO_NATIVE_ENUM(MH, "algtype_tr", algtype_tr, setAlgtype_tr, CPhilipsMainHeader::AlgTr_Type),
  MEDIO_NATIVE(MH, "iter_tr", iter_tr, setIter_tr, short),
  MEDIO_NATIVE(MH, "subset_tr", subset_tr, setSubset_tr, short),
  MEDIO_NATIVE(MH, "nsmooth_tr", nsmooth_tr, setNsmooth_tr, short),
  MEDIO_NATIVE(MH, "nrepeat_tr", nrepeat_tr, setNrepeat_tr, short),
  MEDIO_NATIVE(MH, "attn_corr_3d", attn_corr_3d, setAttn_corr_3d, bool),
  MEDIO_NATIVE(MH, "ramla_no_it", ramla_no_it, setRamla_no_it, short),
  MEDIO_NATIVE(MH, "ramla_sysac", ramla_sysac, setRamla_sysac, short),
  MEDIO_NATIVE(MH, "ramla_blrad", ramla_blrad, setRamla_blrad, float),
  MEDIO_NATIVE(MH, "ramla_blalpha", ramla_blalpha, setRamla_blalpha, float),
  MEDIO_NATIVE(MH, "ramla_bcc_rsz", ramla_bcc_rsz, setRamla_bcc_rsz, float),
  MEDIO_NATIVE(MH, "recon_date_time", recon_date_time, setRecon_date_time, time_t),
  MEDIO_NATIVE(MH, "gating_type", gating_type, setGating_type, short),
  MEDIO_NATIVE_TEXT(MH, "ref_attncor_series_uid", ref_attncor_series_uid, setRef_attncor_series_uid),
  MEDIO_NATIVE_TEXT(MH, "ref_gated_qc_image_inst_uid", ref_gated_qc_image_inst_uid, setRef_gated_qc_image_inst_uid),
  MEDIO_NATIVE_TEXT(MH, "ref_raw_data_inst_uid", ref_raw_data_inst_uid, setRef_raw_data_inst_uid),
  MEDIO_NATIVE(MH, "mr_valid", mr_valid, setMr_valid, bool),
  MEDIO_NATIVE_TEXT(MH, "ref_raw_data_series_inst_uid", ref_raw_data_series_inst_uid, setRef_raw_data_series_inst_uid),
  MEDIO_NATIVE_TEXT(MH, "pet_software_version", pet_software_version, setPet_software_version),
  MEDIO_NATIVE(MH, "num_tile_rings", num_tile_rings, setNum_tile_rings, unsigned char),
  MEDIO_NATIVE(MH, "num_detector_modules", num_detector_modules, setNum_detector_modules, unsigned char),
  MEDIO_NATIVE(MH, "num_tiles_per_module_row", num_tiles_per_module_row, setNum_tiles_per_module_row, unsigned char),
  MEDIO_NATIVE(MH, "X_tile_gap", X_tile_gap, setX_tile_gap, float),
  MEDIO_NATIVE(MH, "Z_tile_gap", Z_tile_gap, setZ_tile_gap, float),
  MEDIO_NATIVE(MH, "crystal_type", crystal_type, setCrystal_type, short),
  MEDIO_NATIVE(MH, "psf1", psf1, setPsf1, short),
  MEDIO_NATIVE(MH, "psf1_iter", psf1_iter, setPsf1_iter, short),
  MEDIO_NATIVE(MH, "psf1_regularization", psf1_regularization, setPsf1_regularization, float),
  MEDIO_NATIVE(MH, "psf2", psf2, setPsf2, short),
  MEDIO_NATIVE(MH, "psf2_iter", psf2_iter, setPsf2_iter, short),
  MEDIO_NATIVE(MH, "gaussian_kernel", gaussian_kernel, setGaussian_kernel, float),
  MEDIO_NATIVE_TEXT(MH, "acqprotocol_name", acqprotocol_name, setAcqprotocol_name),
  MEDIO_NATIVE(MH, "coinc_half_window_bins", coinc_half_window_bins, setCoinc_half_window_bins, unsigned short),
  MEDIO_NATIVE_TEXT(MH, "isotope_name", isotope_name, setIsotope_name),
  MEDIO_NATIVE_TEXT(MH, "isotope_code_value", isotope_code_value, setIsotope_code_value),
  MEDIO_NATIVE_TEXT(MH, "isotope_coding_scheme_designator", isotope_coding_scheme_designator, setIsotope_coding_scheme_designator),
  MEDIO_NATIVE_TEXT(MH, "isotope_coding_scheme_version", isotope_coding_scheme_version, setIsotope_coding_scheme_version),
  MEDIO_NATIVE_TEXT(MH, "isotope_code_meaning", isotope_code_meaning, setIsotope_code_meaning),
  MEDIO_NATIVE_TEXT(MH, "radiopharm_code_value", radiopharm_code_value, setRadiopharm_code_value),
  MEDIO_NATIVE_TEXT(MH, "radiopharm_coding_scheme_designator", radiopharm_coding_scheme_designator, setRadiopharm_coding_scheme_designator),
  MEDIO_NATIVE_TEXT(MH, "radiopharm_coding_scheme_version", radiopharm_coding_scheme_version, setRadiopharm_coding_scheme_version),
  MEDIO_NATIVE_TEXT(MH, "radiopharm_code_meaning", radiopharm_code_meaning, setRadiopharm_code_meaning),
};

//---------------------------------------------------------------------------------------------
// sub header (the metadata of a frame are the ones of its first slice)

// frame duration (ms) of the start and end time of a slice
QJsonValue frameDuration(const SH& h)
{
  if(h.start_date_time() == 0 || h.end_date_time() == 0)
    return QJsonValue(QJsonValue::Undefined);
  const qint64 ms = (static_cast<qint64>(h.end_date_time()) - static_cast<qint64>(h.start_date_time())) * 1000 +
                    static_cast<qint64>(h.end_date_time_msec()) - static_cast<qint64>(h.start_date_time_msec());
  return ms > 0 ? QJsonValue(static_cast<double>(ms)) : QJsonValue(QJsonValue::Undefined);
}

const Field subKeys[] = {
  // the frame timing is part of the image data (start and end time of the slices)
  MEDIO_KEY(SH, "ActualFrameDuration", frameDuration(h), return value.isNull();),
  MEDIO_KEY(SH, "DeadTimeFactor", h.deadtimefac() > 0.0f ? floatNumber(h.deadtimefac()) : QJsonValue(QJsonValue::Undefined),
            h.setDeadtimefac(static_cast<float>(value.toDouble())); return true;),
};

// native fields (without the fields describing the image data: data type,
// dimensions, slice and tilt number, pixel spacing, position, frame timing,
// scaling and value range)
const Field subNative[] = {
  MEDIO_NATIVE_TEXT(SH, "atten_corr", atten_corr, setAtten_corr),
  MEDIO_NATIVE(SH, "card_fr_time", card_fr_time, setCard_fr_time, int),
  MEDIO_NATIVE(SH, "card_high_rr", card_high_rr, setCard_high_rr, int),
  MEDIO_NATIVE(SH, "card_low_rr", card_low_rr, setCard_low_rr, int),
  MEDIO_NATIVE(SH, "card_tr_time", card_tr_time, setCard_tr_time, int),
  MEDIO_NATIVE_TEXT(SH, "scatter_corr", scatter_corr, setScatter_corr),
  MEDIO_NATIVE(SH, "deadtime_corr", deadtime_corr, setDeadtime_corr, short),
  MEDIO_NATIVE_ENUM(SH, "randoms_corr", randoms_corr, setRandoms_corr, CPhilipsSubHeader::Randoms_Type),
  MEDIO_NATIVE(SH, "det_norm", det_norm, setDet_norm, short),
  MEDIO_NATIVE(SH, "nu_radsamp_corr", nu_radsamp_corr, setNu_radsamp_corr, short),
  MEDIO_NATIVE(SH, "pat_mot_corr", pat_mot_corr, setPat_mot_corr, short),
  MEDIO_NATIVE(SH, "echo_time", echo_time, setEcho_time, float),
  MEDIO_NATIVE(SH, "exposure_time", exposure_time, setExposure_time, float),
  MEDIO_NATIVE(SH, "cntloss_corr", cntloss_corr, setCntloss_corr, short),
  MEDIO_NATIVE(SH, "xray_current", xray_current, setXray_current, float),
  MEDIO_NATIVE(SH, "kvp", kvp, setKvp, float),
  MEDIO_NATIVE_ENUM(SH, "decay_corr", decay_corr, setDecay_corr, CPhilipsSubHeader::Decay_Type),
  MEDIO_NATIVE(SH, "bgdelrt", bgdelrt, setBgdelrt, float),
  MEDIO_NATIVE(SH, "enddelrt", enddelrt, setEnddelrt, float),
  MEDIO_NATIVE(SH, "bgsngrt", bgsngrt, setBgsngrt, float),
  MEDIO_NATIVE(SH, "bgcoincrt", bgcoincrt, setBgcoincrt, float),
  MEDIO_NATIVE(SH, "endsngrt", endsngrt, setEndsngrt, float),
  MEDIO_NATIVE(SH, "endcoincrt", endcoincrt, setEndcoincrt, float),
  MEDIO_NATIVE(SH, "deadtimefac", deadtimefac, setDeadtimefac, float),
  MEDIO_NATIVE(SH, "deadtime_bgsub", deadtime_bgsub, setDeadtime_bgsub, float),
  MEDIO_NATIVE_TEXT(SH, "sop_uid", sop_uid, setSop_uid),
  MEDIO_NATIVE_TEXT(SH, "recon_method", recon_method, setRecon_method),
  MEDIO_NATIVE_ENUM(SH, "laterality", laterality, setLaterality, CPhilipsSubHeader::Laterality_Type),
  MEDIO_NATIVE_ENUM(SH, "anatomy", anatomy, setAnatomy, CPhilipsSubHeader::Anatomy_Type),
  MEDIO_NATIVE(SH, "card_rr_time", card_rr_time, setCard_rr_time, int),
  MEDIO_NATIVE(SH, "resp_int_time", resp_int_time, setResp_int_time, int),
  MEDIO_NATIVE(SH, "prompt_events_sum", prompt_events_sum, setPrompt_events_sum, double),
  MEDIO_NATIVE(SH, "delay_events_sum", delay_events_sum, setDelay_events_sum, double),
  MEDIO_NATIVE(SH, "scatter_fraction_average", scatter_fraction_average, setScatter_fraction_average, float),
};

const Adapter mainAdapter = { "Philips", mainKeys, sizeof(mainKeys) / sizeof(mainKeys[0]),
                              mainNative, sizeof(mainNative) / sizeof(mainNative[0]) };
const Adapter subAdapter = { "Philips", subKeys, sizeof(subKeys) / sizeof(subKeys[0]),
                             subNative, sizeof(subNative) / sizeof(subNative[0]) };

} // namespace

const Adapter* philipsMainHeaderAdapter()
{
  return &mainAdapter;
}

const Adapter* philipsSubHeaderAdapter()
{
  return &subAdapter;
}

} // namespace MedIOMetadata
