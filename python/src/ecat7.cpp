#include "pmedio.h"

#include <CMedIOHeader>
#include <CECATFile>
#include <CECATDirectory>
#include <CECATMainHeader>
#include <CECATSubHeader>
#include <CECAT7MainHeader>
#include <CECAT7SubHeaderAttenCorr>
#include <CECAT7SubHeaderImage>
#include <CECAT7SubHeaderNorm>
#include <CECAT7SubHeaderNorm3D>
#include <CECAT7SubHeaderPolarMap>
#include <CECAT7SubHeaderScan>
#include <CECAT7SubHeaderScan3D>

namespace py = pybind11;

// pybind init function for CECAT classes
void init_ecat7(py::module_ &m)
{
  // CECAT7MainHeader
  py::class_<CECAT7MainHeader, CECATMainHeader> ecat7MainHeader(m, "ECAT7MainHeader");
  ecat7MainHeader.def(py::init<CECATFile*, CECATMainHeader::Type>(),
                     py::arg("ecatFile") = nullptr,
                     py::arg("fileType") = CECATMainHeader::Unknown)
                 .def_property("magic_Number", &CECAT7MainHeader::magic_Number, &CECAT7MainHeader::setMagic_Number)
                 .def_property("original_File_Name", &CECAT7MainHeader::original_File_Name, &CECAT7MainHeader::setOriginal_File_Name)
                 .def_property("sw_Version", &CECAT7MainHeader::sw_Version, &CECAT7MainHeader::setSW_Version)
                 .def_property("system_Type", &CECAT7MainHeader::system_Type, &CECAT7MainHeader::setSystem_Type)
                 .def_property("file_Type", &CECAT7MainHeader::file_Type, &CECAT7MainHeader::setFile_Type)
                 .def_property("serial_Number", &CECAT7MainHeader::serial_Number, &CECAT7MainHeader::setSerial_Number)
                 .def_property("scan_Start_Time", &CECAT7MainHeader::scan_Start_Time, &CECAT7MainHeader::setScan_Start_Time)
                 .def_property("isotope_Name", &CECAT7MainHeader::isotope_Name, &CECAT7MainHeader::setIsotope_Name)
                 .def_property("isotope_Halflife", &CECAT7MainHeader::isotope_Halflife, &CECAT7MainHeader::setIsotope_Halflife)
                 .def_property("radiopharmaceutical", &CECAT7MainHeader::radiopharmaceutical, &CECAT7MainHeader::setRadiopharmaceutical)
                 .def_property("gantry_Tilt", &CECAT7MainHeader::gantry_Tilt, &CECAT7MainHeader::setGantry_Tilt)
                 .def_property("gantry_Rotation", &CECAT7MainHeader::gantry_Rotation, &CECAT7MainHeader::setGantry_Rotation)
                 .def_property("bed_Elevation", &CECAT7MainHeader::bed_Elevation, &CECAT7MainHeader::setBed_Elevation)
                 .def_property("intrinsic_Tilt", &CECAT7MainHeader::intrinsic_Tilt, &CECAT7MainHeader::setIntrinsic_Tilt)
                 .def_property("wobble_Speed", &CECAT7MainHeader::wobble_Speed, &CECAT7MainHeader::setWobble_Speed)
                 .def_property("transm_Source_Type", &CECAT7MainHeader::transm_Source_Type, &CECAT7MainHeader::setTransm_Source_Type)
                 .def_property("distance_Scanned", &CECAT7MainHeader::distance_Scanned, &CECAT7MainHeader::setDistance_Scanned)
                 .def_property("transaxial_FOV", &CECAT7MainHeader::transaxial_FOV, &CECAT7MainHeader::setTransaxial_FOV)
                 .def_property("angular_Compression", &CECAT7MainHeader::angular_Compression, &CECAT7MainHeader::setAngular_Compression)
                 .def_property("coin_Samp_Mode", &CECAT7MainHeader::coin_Samp_Mode, &CECAT7MainHeader::setCoin_Samp_Mode)
                 .def_property("axial_Samp_Mode", &CECAT7MainHeader::axial_Samp_Mode, &CECAT7MainHeader::setAxial_Samp_Mode)
                 .def_property("calibration_Factor", &CECAT7MainHeader::calibration_Factor, &CECAT7MainHeader::setCalibration_Factor)
                 .def_property("calibration_Units", &CECAT7MainHeader::calibration_Units, &CECAT7MainHeader::setCalibration_Units)
                 .def_property("calibration_Units_Label", &CECAT7MainHeader::calibration_Units_Label, &CECAT7MainHeader::setCalibration_Units_Label)
                 .def_property("compression_Code", &CECAT7MainHeader::compression_Code, &CECAT7MainHeader::setCompression_Code)
                 .def_property("study_Type", &CECAT7MainHeader::study_Type, &CECAT7MainHeader::setStudy_Type)
                 .def_property("patient_ID", &CECAT7MainHeader::patient_ID, &CECAT7MainHeader::setPatient_ID)
                 .def_property("patient_Name", &CECAT7MainHeader::patient_Name, &CECAT7MainHeader::setPatient_Name)
                 .def_property("patient_Sex", &CECAT7MainHeader::patient_Sex, &CECAT7MainHeader::setPatient_Sex)
                 .def_property("patient_Dexterity", &CECAT7MainHeader::patient_Dexterity, &CECAT7MainHeader::setPatient_Dexterity)
                 .def_property("patient_Age", &CECAT7MainHeader::patient_Age, &CECAT7MainHeader::setPatient_Age)
                 .def_property("patient_Height", &CECAT7MainHeader::patient_Height, &CECAT7MainHeader::setPatient_Height)
                 .def_property("patient_Weight", &CECAT7MainHeader::patient_Weight, &CECAT7MainHeader::setPatient_Weight)
                 .def_property("patient_Birth_Date", &CECAT7MainHeader::patient_Birth_Date, &CECAT7MainHeader::setPatient_Birth_Date)
                 .def_property("physician_Name", &CECAT7MainHeader::physician_Name, &CECAT7MainHeader::setPhysician_Name)
                 .def_property("operator_Name", &CECAT7MainHeader::operator_Name, &CECAT7MainHeader::setOperator_Name)
                 .def_property("study_Description", &CECAT7MainHeader::study_Description, &CECAT7MainHeader::setStudy_Description)
                 .def_property("acquisition_Type", &CECAT7MainHeader::acquisition_Type, &CECAT7MainHeader::setAcquisition_Type)
                 .def_property("patient_Orientation", &CECAT7MainHeader::patient_Orientation, &CECAT7MainHeader::setPatient_Orientation)
                 .def_property("facility_Name", &CECAT7MainHeader::facility_Name, &CECAT7MainHeader::setFacility_Name)
                 .def_property("num_Planes", &CECAT7MainHeader::num_Planes, &CECAT7MainHeader::setNum_Planes)
                 .def_property("num_Frames", &CECAT7MainHeader::num_Frames, &CECAT7MainHeader::setNum_Frames)
                 .def_property("num_Gates", &CECAT7MainHeader::num_Gates, &CECAT7MainHeader::setNum_Gates)
                 .def_property("num_Bed_Pos", &CECAT7MainHeader::num_Bed_Pos, &CECAT7MainHeader::setNum_Bed_Pos)
                 .def_property("init_Bed_Position", &CECAT7MainHeader::init_Bed_Position, &CECAT7MainHeader::setInit_Bed_Position)
                 .def("bed_Offset", &CECAT7MainHeader::bed_Offset)
                 .def("bed_Offset", &CECAT7MainHeader::setBed_Offset)
                 .def_property("plane_Separation", &CECAT7MainHeader::plane_Separation, &CECAT7MainHeader::setPlane_Separation)
                 .def_property("lwr_Sctr_Thres", &CECAT7MainHeader::lwr_Sctr_Thres, &CECAT7MainHeader::setLwr_Sctr_Thres)
                 .def_property("lwr_True_Thres", &CECAT7MainHeader::lwr_True_Thres, &CECAT7MainHeader::setLwr_True_Thres)
                 .def_property("upr_True_Thres", &CECAT7MainHeader::upr_True_Thres, &CECAT7MainHeader::setUpr_True_Thres)
                 .def_property("user_Process_Code", &CECAT7MainHeader::user_Process_Code, &CECAT7MainHeader::setUser_Process_Code)
                 .def_property("acquisition_Mode", &CECAT7MainHeader::acquisition_Mode, &CECAT7MainHeader::setAcquisition_Mode)
                 .def_property("bin_Size", &CECAT7MainHeader::bin_Size, &CECAT7MainHeader::setBin_Size)
                 .def_property("branching_Fraction", &CECAT7MainHeader::branching_Fraction, &CECAT7MainHeader::setBranching_Fraction)
                 .def_property("dose_Start_Time", &CECAT7MainHeader::dose_Start_Time, &CECAT7MainHeader::setDose_Start_Time)
                 .def_property("dosage", &CECAT7MainHeader::dosage, &CECAT7MainHeader::setDosage)
                 .def_property("well_Counter_Corr_Factor", &CECAT7MainHeader::well_Counter_Corr_Factor, &CECAT7MainHeader::setWell_Counter_Corr_Factor)
                 .def_property("data_Units", &CECAT7MainHeader::data_Units, &CECAT7MainHeader::setData_Units)
                 .def_property("septa_State", &CECAT7MainHeader::septa_State, &CECAT7MainHeader::setSepta_State)
                 .def("cti_Reserved", &CECAT7MainHeader::cti_Reserved)
                 .def("cti_Reserved", &CECAT7MainHeader::setCTI_Reserved);
 
  py::enum_<CECAT7MainHeader::File_Type>(ecat7MainHeader, "File_Type")
    .value("Unknown", CECAT7MainHeader::Unknown)
    .value("Sinogram", CECAT7MainHeader::Sinogram)
    .value("Image16", CECAT7MainHeader::Image16)
    .value("AttenuationCorr", CECAT7MainHeader::AttenuationCorr)
    .value("Normalization", CECAT7MainHeader::Normalization)
    .value("PolarMap", CECAT7MainHeader::PolarMap)
    .value("Volume8", CECAT7MainHeader::Volume8)
    .value("Volume16", CECAT7MainHeader::Volume16)
    .value("Projection8", CECAT7MainHeader::Projection8)
    .value("Projection16", CECAT7MainHeader::Projection16)
    .value("Image8", CECAT7MainHeader::Image8)
    .value("Sinogram3D_16", CECAT7MainHeader::Sinogram3D_16)
    .value("Sinogram3D_8", CECAT7MainHeader::Sinogram3D_8)
    .value("Normalization_3D", CECAT7MainHeader::Normalization_3D)
    .value("Sinogram3D_Float", CECAT7MainHeader::Sinogram3D_Float)
    .export_values();

  py::enum_<CECAT7MainHeader::Transm_Source_Type>(ecat7MainHeader, "Transm_Source_Type")
    .value("SRC_NONE", CECAT7MainHeader::SRC_NONE)
    .value("SRC_RRING", CECAT7MainHeader::SRC_RRING)
    .value("SRC_RING", CECAT7MainHeader::SRC_RING)
    .value("SRC_ROD", CECAT7MainHeader::SRC_ROD)
    .value("SRC_RROD", CECAT7MainHeader::SRC_RROD)
    .export_values();

  py::enum_<CECAT7MainHeader::Angular_Compression>(ecat7MainHeader, "Angular_Compression")
    .value("No_Mash", CECAT7MainHeader::No_Mash)
    .value("Mash2", CECAT7MainHeader::Mash2)
    .value("Mash4", CECAT7MainHeader::Mash4)
    .export_values();

  py::enum_<CECAT7MainHeader::Coin_Samp_Mode>(ecat7MainHeader, "Coin_Samp_Mode")
    .value("NetTrues", CECAT7MainHeader::NetTrues)
    .value("PromptsAndDelayed", CECAT7MainHeader::PromptsAndDelayed)
    .value("PromptsDelayedMultiples", CECAT7MainHeader::PromptsDelayedMultiples)
    .export_values();

  py::enum_<CECAT7MainHeader::Axial_Samp_Mode>(ecat7MainHeader, "Axial_Samp_Mode")
    .value("Normal", CECAT7MainHeader::Normal)
    .value("X2", CECAT7MainHeader::X2)
    .value("X3", CECAT7MainHeader::X3)
    .export_values();

  py::enum_<CECAT7MainHeader::Calibration_Units>(ecat7MainHeader, "Calibration_Units")
    .value("Uncalibrated", CECAT7MainHeader::Uncalibrated)
    .value("Calibrated", CECAT7MainHeader::Calibrated)
    .value("CalibrationUnits_Processed", CECAT7MainHeader::CalibrationUnits_Processed)
    .export_values();

  py::enum_<CECAT7MainHeader::Calibration_Units_Label>(ecat7MainHeader, "Calibration_Units_Label")
    .value("Blood_Flow", CECAT7MainHeader::Blood_Flow)
    .value("LMRGLU", CECAT7MainHeader::LMRGLU)
    .value("Label_Processed", CECAT7MainHeader::Label_Processed)
    .export_values();

  py::enum_<CECAT7MainHeader::Compression_Code>(ecat7MainHeader, "Compression_Code")
    .value("Comp_None", CECAT7MainHeader::Comp_None)
    .export_values();

  py::enum_<CECAT7MainHeader::Patient_Sex>(ecat7MainHeader, "Patient_Sex")
    .value("Sex_Male", CECAT7MainHeader::Sex_Male)
    .value("Sex_Female", CECAT7MainHeader::Sex_Female)
    .value("Sex_Unknown", CECAT7MainHeader::Sex_Unknown)
    .export_values();

  py::enum_<CECAT7MainHeader::Patient_Dexterity>(ecat7MainHeader, "Patient_Dexterity")
    .value("Dext_RT", CECAT7MainHeader::Dext_RT)
    .value("Dext_LF", CECAT7MainHeader::Dext_LF)
    .value("Dext_Unknown", CECAT7MainHeader::Dext_Unknown)
    .export_values();

  py::enum_<CECAT7MainHeader::Patient_Orientation>(ecat7MainHeader, "Patient_Orientation")
    .value("FF_Prone", CECAT7MainHeader::FF_Prone)
    .value("HF_Prone", CECAT7MainHeader::HF_Prone)
    .value("FF_Supine", CECAT7MainHeader::FF_Supine)
    .value("HF_Supine", CECAT7MainHeader::HF_Supine)
    .value("FF_Right", CECAT7MainHeader::FF_Right)
    .value("HF_Right", CECAT7MainHeader::HF_Right)
    .value("FF_Left", CECAT7MainHeader::FF_Left)
    .value("HF_Left", CECAT7MainHeader::HF_Left)
    .value("Orient_Unknown", CECAT7MainHeader::Orient_Unknown)
    .export_values();

  py::enum_<CECAT7MainHeader::Acquisition_Type>(ecat7MainHeader, "Acquisition_Type")
    .value("Undefined", CECAT7MainHeader::Undefined)
    .value("Blank", CECAT7MainHeader::Blank)
    .value("Transmission", CECAT7MainHeader::Transmission)
    .value("StaticEmission", CECAT7MainHeader::StaticEmission)
    .value("DynamicEmission", CECAT7MainHeader::DynamicEmission)
    .value("GatedEmission", CECAT7MainHeader::GatedEmission)
    .value("TransmissionRectilinear", CECAT7MainHeader::TransmissionRectilinear)
    .value("EmissionRectilinear", CECAT7MainHeader::EmissionRectilinear)
    .export_values();

  py::enum_<CECAT7MainHeader::Acquisition_Mode>(ecat7MainHeader, "Acquisition_Mode")
    .value("AcqNormal", CECAT7MainHeader::AcqNormal)
    .value("Windowed", CECAT7MainHeader::Windowed)
    .value("WindowedAndNonWindowed", CECAT7MainHeader::WindowedAndNonWindowed)
    .value("DualEnergy", CECAT7MainHeader::DualEnergy)
    .value("UpperEnergy", CECAT7MainHeader::UpperEnergy)
    .value("EmissionTransmission", CECAT7MainHeader::EmissionTransmission)
    .export_values();

  py::enum_<CECAT7MainHeader::Septa_State>(ecat7MainHeader, "Septa_State")
    .value("Extended", CECAT7MainHeader::Extended)
    .value("Retracted", CECAT7MainHeader::Retracted)
    .export_values();

  // CECAT7SubHeaderAttenCorr
  py::class_<CECAT7SubHeaderAttenCorr, CECATSubHeader> ecat7SubHeaderAttenCorr(m, "ECAT7SubHeaderAttenCorr");
  ecat7SubHeaderAttenCorr.def(py::init<CECATFile*, CECATDirectoryItem*>(),
                     py::arg("ecatFile") = nullptr,
                     py::arg("dirItem") = nullptr)
                 .def_property("data_Type", &CECAT7SubHeaderAttenCorr::data_Type, &CECAT7SubHeaderAttenCorr::setData_Type)
                 .def_property("num_Dimensions", &CECAT7SubHeaderAttenCorr::num_Dimensions, &CECAT7SubHeaderAttenCorr::setNum_Dimensions)
                 .def_property("attenuation_Type", &CECAT7SubHeaderAttenCorr::attenuation_Type, &CECAT7SubHeaderAttenCorr::setAttenuation_Type)
                 .def_property("num_R_Elements", &CECAT7SubHeaderAttenCorr::num_R_Elements, &CECAT7SubHeaderAttenCorr::setNum_R_Elements)
                 .def_property("num_Angles", &CECAT7SubHeaderAttenCorr::num_Angles, &CECAT7SubHeaderAttenCorr::setNum_Angles)
                 .def_property("num_Z_Elements", &CECAT7SubHeaderAttenCorr::num_Z_Elements, &CECAT7SubHeaderAttenCorr::setNum_Z_Elements)
                 .def_property("ring_Difference", &CECAT7SubHeaderAttenCorr::ring_Difference, &CECAT7SubHeaderAttenCorr::setRing_Difference)
                 .def_property("x_Resolution", &CECAT7SubHeaderAttenCorr::x_Resolution, &CECAT7SubHeaderAttenCorr::setX_Resolution)
                 .def_property("y_Resolution", &CECAT7SubHeaderAttenCorr::y_Resolution, &CECAT7SubHeaderAttenCorr::setY_Resolution)
                 .def_property("z_Resolution", &CECAT7SubHeaderAttenCorr::z_Resolution, &CECAT7SubHeaderAttenCorr::setZ_Resolution)
                 .def_property("w_Resolution", &CECAT7SubHeaderAttenCorr::w_Resolution, &CECAT7SubHeaderAttenCorr::setW_Resolution)
                 .def_property("scale_Factor", &CECAT7SubHeaderAttenCorr::scale_Factor, &CECAT7SubHeaderAttenCorr::setScale_Factor)
                 .def_property("x_Offset", &CECAT7SubHeaderAttenCorr::x_Offset, &CECAT7SubHeaderAttenCorr::setX_Offset)
                 .def_property("y_Offset", &CECAT7SubHeaderAttenCorr::y_Offset, &CECAT7SubHeaderAttenCorr::setY_Offset)
                 .def_property("x_Radius", &CECAT7SubHeaderAttenCorr::x_Radius, &CECAT7SubHeaderAttenCorr::setX_Radius)
                 .def_property("y_Radius", &CECAT7SubHeaderAttenCorr::y_Radius, &CECAT7SubHeaderAttenCorr::setY_Radius)
                 .def_property("tilt_Angle", &CECAT7SubHeaderAttenCorr::tilt_Angle, &CECAT7SubHeaderAttenCorr::setTilt_Angle)
                 .def_property("attenuation_Coeff", &CECAT7SubHeaderAttenCorr::attenuation_Coeff, &CECAT7SubHeaderAttenCorr::setAttenuation_Coeff)
                 .def_property("attenuation_Min", &CECAT7SubHeaderAttenCorr::attenuation_Min, &CECAT7SubHeaderAttenCorr::setAttenuation_Min)
                 .def_property("attenuation_Max", &CECAT7SubHeaderAttenCorr::attenuation_Max, &CECAT7SubHeaderAttenCorr::setAttenuation_Max)
                 .def_property("skull_Thickness", &CECAT7SubHeaderAttenCorr::skull_Thickness, &CECAT7SubHeaderAttenCorr::setSkull_Thickness)
                 .def_property("num_Additional_Atten_Coeff", &CECAT7SubHeaderAttenCorr::num_Additional_Atten_Coeff, &CECAT7SubHeaderAttenCorr::setNum_Additional_Atten_Coeff)
                 .def("additional_Atten_Coeff", &CECAT7SubHeaderAttenCorr::additional_Atten_Coeff)
                 .def("additional_Atten_Coeff", &CECAT7SubHeaderAttenCorr::setAdditional_Atten_Coeff)
                 .def_property("edge_Finding_Threshold", &CECAT7SubHeaderAttenCorr::edge_Finding_Threshold, &CECAT7SubHeaderAttenCorr::setEdge_Finding_Threshold)
                 .def_property("storage_Order", &CECAT7SubHeaderAttenCorr::storage_Order, &CECAT7SubHeaderAttenCorr::setStorage_Order)
                 .def_property("span", &CECAT7SubHeaderAttenCorr::span, &CECAT7SubHeaderAttenCorr::setSpan)
                 .def("z_Elements", &CECAT7SubHeaderAttenCorr::z_Elements)
                 .def("z_Elements", &CECAT7SubHeaderAttenCorr::setZ_Elements)
                 .def("unused", &CECAT7SubHeaderAttenCorr::unused)
                 .def("unused", &CECAT7SubHeaderAttenCorr::setUnused)
                 .def("cti_Reserved", &CECAT7SubHeaderAttenCorr::cti_Reserved)
                 .def("cti_Reserved", &CECAT7SubHeaderAttenCorr::setCTI_Reserved);

  py::enum_<CECAT7SubHeaderAttenCorr::Attenuation_Type>(ecat7SubHeaderAttenCorr, "Attenuation_Type")
    .value("Atten_None", CECAT7SubHeaderAttenCorr::Atten_None)
    .value("Atten_Meas", CECAT7SubHeaderAttenCorr::Atten_Meas)
    .value("Atten_Calc", CECAT7SubHeaderAttenCorr::Atten_Calc)
    .export_values();

  // CECAT7SubHeaderImage
  py::class_<CECAT7SubHeaderImage, CECATSubHeader> ecat7SubHeaderImage(m, "ECAT7SubHeaderImage");
  ecat7SubHeaderImage.def(py::init<CECATFile*, CECATDirectoryItem*>(),
                     py::arg("ecatFile") = nullptr,
                     py::arg("dirItem") = nullptr)
                 .def_property("data_Type", &CECAT7SubHeaderImage::data_Type, &CECAT7SubHeaderImage::setData_Type)
                 .def_property("num_Dimensions", &CECAT7SubHeaderImage::num_Dimensions, &CECAT7SubHeaderImage::setNum_Dimensions)
                 .def_property("x_Dimension", &CECAT7SubHeaderImage::x_Dimension, &CECAT7SubHeaderImage::setX_Dimension)
                 .def_property("y_Dimension", &CECAT7SubHeaderImage::y_Dimension, &CECAT7SubHeaderImage::setY_Dimension)
                 .def_property("z_Dimension", &CECAT7SubHeaderImage::z_Dimension, &CECAT7SubHeaderImage::setZ_Dimension)
                 .def_property("x_Offset", &CECAT7SubHeaderImage::x_Offset, &CECAT7SubHeaderImage::setX_Offset)
                 .def_property("y_Offset", &CECAT7SubHeaderImage::y_Offset, &CECAT7SubHeaderImage::setY_Offset)
                 .def_property("z_Offset", &CECAT7SubHeaderImage::z_Offset, &CECAT7SubHeaderImage::setZ_Offset)
                 .def_property("recon_Zoom", &CECAT7SubHeaderImage::recon_Zoom, &CECAT7SubHeaderImage::setRecon_Zoom)
                 .def_property("scale_Factor", &CECAT7SubHeaderImage::scale_Factor, &CECAT7SubHeaderImage::setScale_Factor)
                 .def_property("image_Min", &CECAT7SubHeaderImage::image_Min, &CECAT7SubHeaderImage::setImage_Min)
                 .def_property("image_Max", &CECAT7SubHeaderImage::image_Max, &CECAT7SubHeaderImage::setImage_Max)
                 .def_property("x_Pixel_Size", &CECAT7SubHeaderImage::x_Pixel_Size, &CECAT7SubHeaderImage::setX_Pixel_Size)
                 .def_property("y_Pixel_Size", &CECAT7SubHeaderImage::y_Pixel_Size, &CECAT7SubHeaderImage::setY_Pixel_Size)
                 .def_property("z_Pixel_Size", &CECAT7SubHeaderImage::z_Pixel_Size, &CECAT7SubHeaderImage::setZ_Pixel_Size)
                 .def_property("frame_Duration", &CECAT7SubHeaderImage::frame_Duration, &CECAT7SubHeaderImage::setFrame_Duration)
                 .def_property("frame_Start_Time", &CECAT7SubHeaderImage::frame_Start_Time, &CECAT7SubHeaderImage::setFrame_Start_Time)
                 .def_property("filter_Code", &CECAT7SubHeaderImage::filter_Code, &CECAT7SubHeaderImage::setFilter_Code)
                 .def_property("x_Resolution", &CECAT7SubHeaderImage::x_Resolution, &CECAT7SubHeaderImage::setX_Resolution)
                 .def_property("y_Resolution", &CECAT7SubHeaderImage::y_Resolution, &CECAT7SubHeaderImage::setY_Resolution)
                 .def_property("z_Resolution", &CECAT7SubHeaderImage::z_Resolution, &CECAT7SubHeaderImage::setZ_Resolution)
                 .def_property("num_R_Elements", &CECAT7SubHeaderImage::num_R_Elements, &CECAT7SubHeaderImage::setNum_R_Elements)
                 .def_property("num_Angles", &CECAT7SubHeaderImage::num_Angles, &CECAT7SubHeaderImage::setNum_Angles)
                 .def_property("z_Rotation_Angle", &CECAT7SubHeaderImage::z_Rotation_Angle, &CECAT7SubHeaderImage::setZ_Rotation_Angle)
                 .def_property("decay_Corr_Fctr", &CECAT7SubHeaderImage::decay_Corr_Fctr, &CECAT7SubHeaderImage::setDecay_Corr_Fctr)
                 .def_property("processing_Code", &CECAT7SubHeaderImage::processing_Code, &CECAT7SubHeaderImage::setProcessing_Code)
                 .def_property("gate_Duration", &CECAT7SubHeaderImage::gate_Duration, &CECAT7SubHeaderImage::setGate_Duration)
                 .def_property("r_Wave_Offset", &CECAT7SubHeaderImage::r_Wave_Offset, &CECAT7SubHeaderImage::setR_Wave_Offset)
                 .def_property("num_Accepted_Beats", &CECAT7SubHeaderImage::num_Accepted_Beats, &CECAT7SubHeaderImage::setNum_Accepted_Beats)
                 .def_property("filter_Cutoff_Frequency", &CECAT7SubHeaderImage::filter_Cutoff_Frequency, &CECAT7SubHeaderImage::setFilter_Cutoff_Frequency)
                 .def_property("filter_Resolution", &CECAT7SubHeaderImage::filter_Resolution, &CECAT7SubHeaderImage::setFilter_Resolution)
                 .def_property("filter_Ramp_Slope", &CECAT7SubHeaderImage::filter_Ramp_Slope, &CECAT7SubHeaderImage::setFilter_Ramp_Slope)
                 .def_property("filter_Order", &CECAT7SubHeaderImage::filter_Order, &CECAT7SubHeaderImage::setFilter_Order)
                 .def_property("filter_Scatter_Fraction", &CECAT7SubHeaderImage::filter_Scatter_Fraction, &CECAT7SubHeaderImage::setFilter_Scatter_Fraction)
                 .def_property("filter_Scatter_Slope", &CECAT7SubHeaderImage::filter_Scatter_Slope, &CECAT7SubHeaderImage::setFilter_Scatter_Slope)
                 .def_property("annotation", &CECAT7SubHeaderImage::annotation, &CECAT7SubHeaderImage::setAnnotation)
                 .def_property("mt_1_1", &CECAT7SubHeaderImage::mt_1_1, &CECAT7SubHeaderImage::setMT_1_1)
                 .def_property("mt_1_2", &CECAT7SubHeaderImage::mt_1_2, &CECAT7SubHeaderImage::setMT_1_2)
                 .def_property("mt_1_3", &CECAT7SubHeaderImage::mt_1_3, &CECAT7SubHeaderImage::setMT_1_3)
                 .def_property("mt_2_1", &CECAT7SubHeaderImage::mt_2_1, &CECAT7SubHeaderImage::setMT_2_1)
                 .def_property("mt_2_2", &CECAT7SubHeaderImage::mt_2_2, &CECAT7SubHeaderImage::setMT_2_2)
                 .def_property("mt_2_3", &CECAT7SubHeaderImage::mt_2_3, &CECAT7SubHeaderImage::setMT_2_3)
                 .def_property("mt_3_1", &CECAT7SubHeaderImage::mt_3_1, &CECAT7SubHeaderImage::setMT_3_1)
                 .def_property("mt_3_2", &CECAT7SubHeaderImage::mt_3_2, &CECAT7SubHeaderImage::setMT_3_2)
                 .def_property("mt_3_3", &CECAT7SubHeaderImage::mt_3_3, &CECAT7SubHeaderImage::setMT_3_3)
                 .def_property("rfilter_Cutoff", &CECAT7SubHeaderImage::rfilter_Cutoff, &CECAT7SubHeaderImage::setRFilter_Cutoff)
                 .def_property("rfilter_Resolution", &CECAT7SubHeaderImage::rfilter_Resolution, &CECAT7SubHeaderImage::setRFilter_Resolution)
                 .def_property("rfilter_Code", &CECAT7SubHeaderImage::rfilter_Code, &CECAT7SubHeaderImage::setRFilter_Code)
                 .def_property("rfilter_Order", &CECAT7SubHeaderImage::rfilter_Order, &CECAT7SubHeaderImage::setRFilter_Order)
                 .def_property("zfilter_Cutoff", &CECAT7SubHeaderImage::zfilter_Cutoff, &CECAT7SubHeaderImage::setZFilter_Cutoff)
                 .def_property("zfilter_Resolution", &CECAT7SubHeaderImage::zfilter_Resolution, &CECAT7SubHeaderImage::setZFilter_Resolution)
                 .def_property("zfilter_Code", &CECAT7SubHeaderImage::zfilter_Code, &CECAT7SubHeaderImage::setZFilter_Code)
                 .def_property("zfilter_Order", &CECAT7SubHeaderImage::zfilter_Order, &CECAT7SubHeaderImage::setZFilter_Order)
                 .def_property("mt_1_4", &CECAT7SubHeaderImage::mt_1_4, &CECAT7SubHeaderImage::setMT_1_4)
                 .def_property("mt_2_4", &CECAT7SubHeaderImage::mt_2_4, &CECAT7SubHeaderImage::setMT_2_4)
                 .def_property("mt_3_4", &CECAT7SubHeaderImage::mt_3_4, &CECAT7SubHeaderImage::setMT_3_4)
                 .def_property("scatter_Type", &CECAT7SubHeaderImage::scatter_Type, &CECAT7SubHeaderImage::setScatter_Type)
                 .def_property("recon_Type", &CECAT7SubHeaderImage::recon_Type, &CECAT7SubHeaderImage::setRecon_Type)
                 .def_property("recon_Views", &CECAT7SubHeaderImage::recon_Views, &CECAT7SubHeaderImage::setRecon_Views)
                 .def("cti_Reserved", &CECAT7SubHeaderImage::cti_Reserved)
                 .def("cti_Reserved", &CECAT7SubHeaderImage::setCTI_Reserved)
                 .def("user_Reserved", &CECAT7SubHeaderImage::user_Reserved)
                 .def("user_Reserved", &CECAT7SubHeaderImage::setUser_Reserved)
                 .def("suv_Scale_Factor", &CECAT7SubHeaderImage::suv_Scale_Factor, 
                    py::arg("ok"),
                    py::arg("mainHeader") = nullptr);
 
  py::enum_<CECAT7SubHeaderImage::Filter_Code>(ecat7SubHeaderImage, "Filter_Code")
    .value("NoFilter", CECAT7SubHeaderImage::NoFilter)
    .value("Ramp", CECAT7SubHeaderImage::Ramp)
    .value("Butterfield", CECAT7SubHeaderImage::Butterfield)
    .value("Hanning", CECAT7SubHeaderImage::Hanning)
    .value("Hamming", CECAT7SubHeaderImage::Hamming)
    .value("Parzen", CECAT7SubHeaderImage::Parzen)
    .value("Shepp", CECAT7SubHeaderImage::Shepp)
    .value("Butterworth", CECAT7SubHeaderImage::Butterworth)
    .value("Gaussian", CECAT7SubHeaderImage::Gaussian)
    .value("Median", CECAT7SubHeaderImage::Median)
    .value("Boxcar", CECAT7SubHeaderImage::Boxcar)
    .export_values();

  py::enum_<CECAT7SubHeaderImage::Scatter_Type>(ecat7SubHeaderImage, "Scatter_Type")
    .value("None", CECAT7SubHeaderImage::None)
    .value("Deconvolution", CECAT7SubHeaderImage::Deconvolution)
    .value("Simulated", CECAT7SubHeaderImage::Simulated)
    .value("DualEnergy", CECAT7SubHeaderImage::DualEnergy)
    .export_values();

  py::enum_<CECAT7SubHeaderImage::Recon_Type>(ecat7SubHeaderImage, "Recon_Type")
    .value("FilteredBackProjection", CECAT7SubHeaderImage::FilteredBackProjection)
    .value("ForwardProjection3D", CECAT7SubHeaderImage::ForwardProjection3D)
    .value("Ramp3D", CECAT7SubHeaderImage::Ramp3D)
    .value("Favor3D", CECAT7SubHeaderImage::Favor3D)
    .value("SSRB", CECAT7SubHeaderImage::SSRB)
    .value("MultiSliceRebinning", CECAT7SubHeaderImage::MultiSliceRebinning)
    .value("FORE", CECAT7SubHeaderImage::FORE)
    .value("BasicOsem", CECAT7SubHeaderImage::BasicOsem)
    .value("CPUReconBase", CECAT7SubHeaderImage::CPUReconBase)
    .value("FourierCPURecon", CECAT7SubHeaderImage::FourierCPURecon)
    .value("FilteredBackProjectionCPURecon", CECAT7SubHeaderImage::FilteredBackProjectionCPURecon)
    .value("IterativeCPURecon", CECAT7SubHeaderImage::IterativeCPURecon)
    .value("FourierCPUReconSmallFOV", CECAT7SubHeaderImage::FourierCPUReconSmallFOV)
    .value("FilteredBackProjectionCPUReconSmallFOV", CECAT7SubHeaderImage::FilteredBackProjectionCPUReconSmallFOV)
    .value("IterativeCPUReconSmallFOV", CECAT7SubHeaderImage::IterativeCPUReconSmallFOV)
    .export_values();

  py::enum_<CECAT7SubHeaderImage::Processing_Code>(ecat7SubHeaderImage, "Processing_Code")
    .value("NotProcessed", CECAT7SubHeaderImage::NotProcessed)
    .value("Normalized", CECAT7SubHeaderImage::Normalized)
    .value("MeasuredAttenCorr", CECAT7SubHeaderImage::MeasuredAttenCorr)
    .value("CalculatedAttenCorr", CECAT7SubHeaderImage::CalculatedAttenCorr)
    .value("XSmoothing", CECAT7SubHeaderImage::XSmoothing)
    .value("YSmoothing", CECAT7SubHeaderImage::YSmoothing)
    .value("ZSmoothing", CECAT7SubHeaderImage::ZSmoothing)
    .value("Scatter2DCorr", CECAT7SubHeaderImage::Scatter2DCorr)
    .value("Scatter3DCorr", CECAT7SubHeaderImage::Scatter3DCorr)
    .value("ArcCorrection", CECAT7SubHeaderImage::ArcCorrection)
    .value("DecayCorrection", CECAT7SubHeaderImage::DecayCorrection)
    .value("OnlineCompression", CECAT7SubHeaderImage::OnlineCompression)
    .value("FourierRebinning", CECAT7SubHeaderImage::FourierRebinning)
    .value("SingleSliceRebinning", CECAT7SubHeaderImage::SingleSliceRebinning)
    .value("SegmentZeroOnly", CECAT7SubHeaderImage::SegmentZeroOnly)
    .export_values();

  // CECAT7SubHeaderNorm
  py::class_<CECAT7SubHeaderNorm, CECATSubHeader> ecat7SubHeaderNorm(m, "ECAT7SubHeaderNorm");
  ecat7SubHeaderNorm.def(py::init<CECATFile*, CECATDirectoryItem*>(),
                     py::arg("ecatFile") = nullptr,
                     py::arg("dirItem") = nullptr)
                 .def_property("data_Type", &CECAT7SubHeaderNorm::data_Type, &CECAT7SubHeaderNorm::setData_Type)
                 .def_property("num_Dimensions", &CECAT7SubHeaderNorm::num_Dimensions, &CECAT7SubHeaderNorm::setNum_Dimensions)
                 .def_property("num_R_Elements", &CECAT7SubHeaderNorm::num_R_Elements, &CECAT7SubHeaderNorm::setNum_R_Elements)
                 .def_property("num_Angles", &CECAT7SubHeaderNorm::num_Angles, &CECAT7SubHeaderNorm::setNum_Angles)
                 .def_property("num_Z_Elements", &CECAT7SubHeaderNorm::num_Z_Elements, &CECAT7SubHeaderNorm::setNum_Z_Elements)
                 .def_property("ring_Difference", &CECAT7SubHeaderNorm::ring_Difference, &CECAT7SubHeaderNorm::setRing_Difference)
                 .def_property("scale_Factor", &CECAT7SubHeaderNorm::scale_Factor, &CECAT7SubHeaderNorm::setScale_Factor)
                 .def_property("norm_Min", &CECAT7SubHeaderNorm::norm_Min, &CECAT7SubHeaderNorm::setNorm_Min)
                 .def_property("norm_Max", &CECAT7SubHeaderNorm::norm_Max, &CECAT7SubHeaderNorm::setNorm_Max)
                 .def_property("fov_Source_Width", &CECAT7SubHeaderNorm::fov_Source_Width, &CECAT7SubHeaderNorm::setFOV_Source_Width)
                 .def_property("norm_Quality_Factor", &CECAT7SubHeaderNorm::norm_Quality_Factor, &CECAT7SubHeaderNorm::setNorm_Quality_Factor)
                 .def_property("norm_Quality_Factor_Code", &CECAT7SubHeaderNorm::norm_Quality_Factor_Code, &CECAT7SubHeaderNorm::setNorm_Quality_Factor_Code)
                 .def_property("storage_Order", &CECAT7SubHeaderNorm::storage_Order, &CECAT7SubHeaderNorm::setStorage_Order)
                 .def_property("span", &CECAT7SubHeaderNorm::span, &CECAT7SubHeaderNorm::setSpan)
                 .def("z_Elements", &CECAT7SubHeaderNorm::z_Elements)
                 .def("z_Elements", &CECAT7SubHeaderNorm::setZ_Elements)
                 .def("cti_Reserved", &CECAT7SubHeaderNorm::cti_Reserved)
                 .def("cti_Reserved", &CECAT7SubHeaderNorm::setCTI_Reserved)
                 .def("user_Reserved", &CECAT7SubHeaderNorm::user_Reserved)
                 .def("user_Reserved", &CECAT7SubHeaderNorm::setUser_Reserved);

  py::enum_<CECAT7SubHeaderNorm::Norm_Qual_Factor_Code>(ecat7SubHeaderNorm, "Norm_Qual_Factor_Code")
    .value("TBD", CECAT7SubHeaderNorm::TBD)
    .export_values();

  // CECAT7SubHeaderNorm3D
  py::class_<CECAT7SubHeaderNorm3D, CECATSubHeader> ecat7SubHeaderNorm3D(m, "ECAT7SubHeaderNorm3D");
  ecat7SubHeaderNorm3D.def(py::init<CECATFile*, CECATDirectoryItem*>(),
                     py::arg("ecatFile") = nullptr,
                     py::arg("dirItem") = nullptr)
                 .def_property("data_Type", &CECAT7SubHeaderNorm3D::data_Type, &CECAT7SubHeaderNorm3D::setData_Type)
                 .def_property("num_R_Elements", &CECAT7SubHeaderNorm3D::num_R_Elements, &CECAT7SubHeaderNorm3D::setNum_R_Elements)
                 .def_property("num_Transaxial_Crystals", &CECAT7SubHeaderNorm3D::num_Transaxial_Crystals, &CECAT7SubHeaderNorm3D::setNum_Transaxial_Crystals)
                 .def_property("num_Crystal_Rings", &CECAT7SubHeaderNorm3D::num_Crystal_Rings, &CECAT7SubHeaderNorm3D::setNum_Crystal_Rings)
                 .def_property("crystals_Per_Ring", &CECAT7SubHeaderNorm3D::crystals_Per_Ring, &CECAT7SubHeaderNorm3D::setCrystals_Per_Ring)
                 .def_property("num_Geo_Corr_Planes", &CECAT7SubHeaderNorm3D::num_Geo_Corr_Planes, &CECAT7SubHeaderNorm3D::setNum_Geo_Corr_Planes)
                 .def_property("uld", &CECAT7SubHeaderNorm3D::uld, &CECAT7SubHeaderNorm3D::setULD)
                 .def_property("lld", &CECAT7SubHeaderNorm3D::lld, &CECAT7SubHeaderNorm3D::setLLD)
                 .def_property("scatter_Energy", &CECAT7SubHeaderNorm3D::scatter_Energy, &CECAT7SubHeaderNorm3D::setScatter_Energy)
                 .def_property("norm_Quality_Factor", &CECAT7SubHeaderNorm3D::norm_Quality_Factor, &CECAT7SubHeaderNorm3D::setNorm_Quality_Factor)
                 .def_property("norm_Quality_Factor_Code", &CECAT7SubHeaderNorm3D::norm_Quality_Factor_Code, &CECAT7SubHeaderNorm3D::setNorm_Quality_Factor_Code)
                 .def("ring_DTCor1", &CECAT7SubHeaderNorm3D::ring_DTCor1)
                 .def("ring_DTCor1", &CECAT7SubHeaderNorm3D::setRing_DTCor1)
                 .def("ring_DTCor2", &CECAT7SubHeaderNorm3D::ring_DTCor2)
                 .def("ring_DTCor2", &CECAT7SubHeaderNorm3D::setRing_DTCor2)
                 .def("crystal_DTCor", &CECAT7SubHeaderNorm3D::crystal_DTCor)
                 .def("crystal_DTCor", &CECAT7SubHeaderNorm3D::setCrystal_DTCor)
                 .def_property("span", &CECAT7SubHeaderNorm3D::span, &CECAT7SubHeaderNorm3D::setSpan)
                 .def_property("max_Ring_Diff", &CECAT7SubHeaderNorm3D::max_Ring_Diff, &CECAT7SubHeaderNorm3D::setMax_Ring_Diff)
                 .def("cti_Reserved", &CECAT7SubHeaderNorm3D::cti_Reserved)
                 .def("cti_Reserved", &CECAT7SubHeaderNorm3D::setCTI_Reserved)
                 .def("user_Reserved", &CECAT7SubHeaderNorm3D::user_Reserved)
                 .def("user_Reserved", &CECAT7SubHeaderNorm3D::setUser_Reserved);

  py::enum_<CECAT7SubHeaderNorm3D::Norm_Qual_Factor_Code>(ecat7SubHeaderNorm3D, "Norm_Qual_Factor_Code")
    .value("TBD", CECAT7SubHeaderNorm3D::TBD)
    .export_values();

  // CECAT7SubHeaderPolarMap
  py::class_<CECAT7SubHeaderPolarMap, CECATSubHeader> ecat7SubHeaderPolarMap(m, "ECAT7SubHeaderPolarMap");
  ecat7SubHeaderPolarMap.def(py::init<CECATFile*, CECATDirectoryItem*>(),
                     py::arg("ecatFile") = nullptr,
                     py::arg("dirItem") = nullptr)
                 .def_property("data_Type", &CECAT7SubHeaderPolarMap::data_Type, &CECAT7SubHeaderPolarMap::setData_Type)
                 .def_property("polar_Map_Type", &CECAT7SubHeaderPolarMap::polar_Map_Type, &CECAT7SubHeaderPolarMap::setPolar_Map_Type)
                 .def_property("num_Rings", &CECAT7SubHeaderPolarMap::num_Rings, &CECAT7SubHeaderPolarMap::setNum_Rings)
                 .def("sectors_Per_Ring", &CECAT7SubHeaderPolarMap::sectors_Per_Ring)
                 .def("sectors_Per_Ring", &CECAT7SubHeaderPolarMap::setSectors_Per_Ring)
                 .def("ring_Position", &CECAT7SubHeaderPolarMap::ring_Position)
                 .def("ring_Position", &CECAT7SubHeaderPolarMap::setRing_Position)
                 .def("ring_Angle", &CECAT7SubHeaderPolarMap::ring_Angle)
                 .def("ring_Angle", &CECAT7SubHeaderPolarMap::setRing_Angle)
                 .def_property("start_Angle", &CECAT7SubHeaderPolarMap::start_Angle, &CECAT7SubHeaderPolarMap::setStart_Angle)
                 .def("long_Axis_Left", &CECAT7SubHeaderPolarMap::long_Axis_Left)
                 .def("long_Axis_Left", &CECAT7SubHeaderPolarMap::setLong_Axis_Left)
                 .def("long_Axis_Right", &CECAT7SubHeaderPolarMap::long_Axis_Right)
                 .def("long_Axis_Right", &CECAT7SubHeaderPolarMap::setLong_Axis_Right)
                 .def_property("position_Data", &CECAT7SubHeaderPolarMap::position_Data, &CECAT7SubHeaderPolarMap::setPosition_Data)
                 .def_property("image_Min", &CECAT7SubHeaderPolarMap::image_Min, &CECAT7SubHeaderPolarMap::setImage_Min)
                 .def_property("image_Max", &CECAT7SubHeaderPolarMap::image_Max, &CECAT7SubHeaderPolarMap::setImage_Max)
                 .def_property("scale_Factor", &CECAT7SubHeaderPolarMap::scale_Factor, &CECAT7SubHeaderPolarMap::setScale_Factor)
                 .def_property("pixel_Size", &CECAT7SubHeaderPolarMap::pixel_Size, &CECAT7SubHeaderPolarMap::setPixel_Size)
                 .def_property("frame_Duration", &CECAT7SubHeaderPolarMap::frame_Duration, &CECAT7SubHeaderPolarMap::setFrame_Duration)
                 .def_property("frame_Start_Time", &CECAT7SubHeaderPolarMap::frame_Start_Time, &CECAT7SubHeaderPolarMap::setFrame_Start_Time)
                 .def_property("processing_Code", &CECAT7SubHeaderPolarMap::processing_Code, &CECAT7SubHeaderPolarMap::setProcessing_Code)
                 .def_property("quant_Units", &CECAT7SubHeaderPolarMap::quant_Units, &CECAT7SubHeaderPolarMap::setQuant_Units)
                 .def_property("annotation", &CECAT7SubHeaderPolarMap::annotation, &CECAT7SubHeaderPolarMap::setAnnotation)
                 .def_property("gate_Duration", &CECAT7SubHeaderPolarMap::gate_Duration, &CECAT7SubHeaderPolarMap::setGate_Duration)
                 .def_property("r_Wave_Offset", &CECAT7SubHeaderPolarMap::r_Wave_Offset, &CECAT7SubHeaderPolarMap::setR_Wave_Offset)
                 .def_property("num_Accepted_Beats", &CECAT7SubHeaderPolarMap::num_Accepted_Beats, &CECAT7SubHeaderPolarMap::setNum_Accepted_Beats)
                 .def_property("polar_Map_Protocol", &CECAT7SubHeaderPolarMap::polar_Map_Protocol, &CECAT7SubHeaderPolarMap::setPolar_Map_Protocol)
                 .def_property("database_Name", &CECAT7SubHeaderPolarMap::database_Name, &CECAT7SubHeaderPolarMap::setDatabase_Name)
                 .def("cti_Reserved", &CECAT7SubHeaderPolarMap::cti_Reserved)
                 .def("cti_Reserved", &CECAT7SubHeaderPolarMap::setCTI_Reserved)
                 .def("user_Reserved", &CECAT7SubHeaderPolarMap::user_Reserved)
                 .def("user_Reserved", &CECAT7SubHeaderPolarMap::setUser_Reserved);
 
  py::enum_<CECAT7SubHeaderPolarMap::Processing_Code>(ecat7SubHeaderPolarMap, "Processing_Code")
    .value("MapType", CECAT7SubHeaderPolarMap::MapType)
    .value("ThresholdApplied", CECAT7SubHeaderPolarMap::ThresholdApplied)
    .value("SummedMap", CECAT7SubHeaderPolarMap::SummedMap)
    .value("SubstractedMap", CECAT7SubHeaderPolarMap::SubstractedMap)
    .value("ProductOfTwoMaps", CECAT7SubHeaderPolarMap::ProductOfTwoMaps)
    .value("RatioOfTwoMaps", CECAT7SubHeaderPolarMap::RatioOfTwoMaps)
    .value("Bias", CECAT7SubHeaderPolarMap::Bias)
    .value("Multiplier", CECAT7SubHeaderPolarMap::Multiplier)
    .value("Transform", CECAT7SubHeaderPolarMap::Transform)
    .value("PMapCalcProtoUsed", CECAT7SubHeaderPolarMap::PMapCalcProtoUsed)
    .export_values();

  // CECAT7SubHeaderScan
  py::class_<CECAT7SubHeaderScan, CECATSubHeader> ecat7SubHeaderScan(m, "ECAT7SubHeaderScan");
  ecat7SubHeaderScan.def(py::init<CECATFile*, CECATDirectoryItem*>(),
                     py::arg("ecatFile") = nullptr,
                     py::arg("dirItem") = nullptr)
                 .def_property("data_Type", &CECAT7SubHeaderScan::data_Type, &CECAT7SubHeaderScan::setData_Type)
                 .def_property("num_Dimensions", &CECAT7SubHeaderScan::num_Dimensions, &CECAT7SubHeaderScan::setNum_Dimensions)
                 .def_property("num_R_Elements", &CECAT7SubHeaderScan::num_R_Elements, &CECAT7SubHeaderScan::setNum_R_Elements)
                 .def_property("num_Angles", &CECAT7SubHeaderScan::num_Angles, &CECAT7SubHeaderScan::setNum_Angles)
                 .def_property("corrections_Applied", &CECAT7SubHeaderScan::corrections_Applied, &CECAT7SubHeaderScan::setCorrections_Applied)
                 .def_property("num_Z_Elements", &CECAT7SubHeaderScan::num_Z_Elements, &CECAT7SubHeaderScan::setNum_Z_Elements)
                 .def_property("ring_Difference", &CECAT7SubHeaderScan::ring_Difference, &CECAT7SubHeaderScan::setRing_Difference)
                 .def_property("x_Resolution", &CECAT7SubHeaderScan::x_Resolution, &CECAT7SubHeaderScan::setX_Resolution)
                 .def_property("y_Resolution", &CECAT7SubHeaderScan::y_Resolution, &CECAT7SubHeaderScan::setY_Resolution)
                 .def_property("z_Resolution", &CECAT7SubHeaderScan::z_Resolution, &CECAT7SubHeaderScan::setZ_Resolution)
                 .def_property("w_Resolution", &CECAT7SubHeaderScan::w_Resolution, &CECAT7SubHeaderScan::setW_Resolution)
                 .def("gate_Reserved", &CECAT7SubHeaderScan::gate_Reserved)
                 .def("gate_Reserved", &CECAT7SubHeaderScan::setGate_Reserved)
                 .def_property("gate_Duration", &CECAT7SubHeaderScan::gate_Duration, &CECAT7SubHeaderScan::setGate_Duration)
                 .def_property("r_Wave_Offset", &CECAT7SubHeaderScan::r_Wave_Offset, &CECAT7SubHeaderScan::setR_Wave_Offset)
                 .def_property("num_Accepted_Beats", &CECAT7SubHeaderScan::num_Accepted_Beats, &CECAT7SubHeaderScan::setNum_Accepted_Beats)
                 .def_property("scale_Factor", &CECAT7SubHeaderScan::scale_Factor, &CECAT7SubHeaderScan::setScale_Factor)
                 .def_property("scan_Min", &CECAT7SubHeaderScan::scan_Min, &CECAT7SubHeaderScan::setScan_Min)
                 .def_property("scan_Max", &CECAT7SubHeaderScan::scan_Max, &CECAT7SubHeaderScan::setScan_Max)
                 .def_property("prompts", &CECAT7SubHeaderScan::prompts, &CECAT7SubHeaderScan::setPrompts)
                 .def_property("delayed", &CECAT7SubHeaderScan::delayed, &CECAT7SubHeaderScan::setDelayed)
                 .def_property("multiples", &CECAT7SubHeaderScan::multiples, &CECAT7SubHeaderScan::setMultiples)
                 .def_property("net_Trues", &CECAT7SubHeaderScan::net_Trues, &CECAT7SubHeaderScan::setNet_Trues)
                 .def("cor_Singles", &CECAT7SubHeaderScan::cor_Singles)
                 .def("cor_Singles", &CECAT7SubHeaderScan::setCor_Singles)
                 .def("uncor_Singles", &CECAT7SubHeaderScan::uncor_Singles)
                 .def("uncor_Singles", &CECAT7SubHeaderScan::setUncor_Singles)
                 .def_property("tot_Avg_Cor", &CECAT7SubHeaderScan::tot_Avg_Cor, &CECAT7SubHeaderScan::setTot_Avg_Cor)
                 .def_property("tot_Avg_Uncor", &CECAT7SubHeaderScan::tot_Avg_Uncor, &CECAT7SubHeaderScan::setTot_Avg_Uncor)
                 .def_property("total_Coin_Rate", &CECAT7SubHeaderScan::total_Coin_Rate, &CECAT7SubHeaderScan::setTotal_Coin_Rate)
                 .def_property("frame_Start_Time", &CECAT7SubHeaderScan::frame_Start_Time, &CECAT7SubHeaderScan::setFrame_Start_Time)
                 .def_property("frame_Duration", &CECAT7SubHeaderScan::frame_Duration, &CECAT7SubHeaderScan::setFrame_Duration)
                 .def_property("deadtime_Correction_Factor", &CECAT7SubHeaderScan::deadtime_Correction_Factor, &CECAT7SubHeaderScan::setDeadtime_Correction_Factor)
                 .def("physical_Planes", &CECAT7SubHeaderScan::physical_Planes)
                 .def("physical_Planes", &CECAT7SubHeaderScan::setPhysical_Planes)
                 .def("cti_Reserved", &CECAT7SubHeaderScan::cti_Reserved)
                 .def("cti_Reserved", &CECAT7SubHeaderScan::setCTI_Reserved)
                 .def("user_Reserved", &CECAT7SubHeaderScan::user_Reserved)
                 .def("user_Reserved", &CECAT7SubHeaderScan::setUser_Reserved);
 
  py::enum_<CECAT7SubHeaderScan::Corrections_Applied>(ecat7SubHeaderScan, "Corrections_Applied")
    .value("Norm", CECAT7SubHeaderScan::Norm)
    .value("Atten", CECAT7SubHeaderScan::Atten)
    .value("Smooth", CECAT7SubHeaderScan::Smooth)
    .export_values();

  // CECAT7SubHeaderScan3D
  py::class_<CECAT7SubHeaderScan3D, CECATSubHeader> ecat7SubHeaderScan3D(m, "ECAT7SubHeaderScan3D");
  ecat7SubHeaderScan3D.def(py::init<CECATFile*, CECATDirectoryItem*>(),
                     py::arg("ecatFile") = nullptr,
                     py::arg("dirItem") = nullptr)
                 .def_property("data_Type", &CECAT7SubHeaderScan3D::data_Type, &CECAT7SubHeaderScan3D::setData_Type)
                 .def_property("num_Dimensions", &CECAT7SubHeaderScan3D::num_Dimensions, &CECAT7SubHeaderScan3D::setNum_Dimensions)
                 .def_property("num_R_Elements", &CECAT7SubHeaderScan3D::num_R_Elements, &CECAT7SubHeaderScan3D::setNum_R_Elements)
                 .def_property("num_Angles", &CECAT7SubHeaderScan3D::num_Angles, &CECAT7SubHeaderScan3D::setNum_Angles)
                 .def_property("corrections_Applied", &CECAT7SubHeaderScan3D::corrections_Applied, &CECAT7SubHeaderScan3D::setCorrections_Applied)
                 .def_property("num_Z_Elements", &CECAT7SubHeaderScan3D::num_Z_Elements, &CECAT7SubHeaderScan3D::setNum_Z_Elements)
                 .def_property("ring_Difference", &CECAT7SubHeaderScan3D::ring_Difference, &CECAT7SubHeaderScan3D::setRing_Difference)
                 .def_property("storage_Order", &CECAT7SubHeaderScan3D::storage_Order, &CECAT7SubHeaderScan3D::setStorage_Order)
                 .def_property("axial_Compression", &CECAT7SubHeaderScan3D::axial_Compression, &CECAT7SubHeaderScan3D::setAxial_Compression)
                 .def_property("x_Resolution", &CECAT7SubHeaderScan3D::x_Resolution, &CECAT7SubHeaderScan3D::setX_Resolution)
                 .def_property("v_Resolution", &CECAT7SubHeaderScan3D::v_Resolution, &CECAT7SubHeaderScan3D::setV_Resolution)
                 .def_property("z_Resolution", &CECAT7SubHeaderScan3D::z_Resolution, &CECAT7SubHeaderScan3D::setZ_Resolution)
                 .def_property("w_Resolution", &CECAT7SubHeaderScan3D::w_Resolution, &CECAT7SubHeaderScan3D::setW_Resolution)
                 .def("gate_Reserved", &CECAT7SubHeaderScan3D::gate_Reserved)
                 .def("gate_Reserved", &CECAT7SubHeaderScan3D::setGate_Reserved)
                 .def_property("gate_Duration", &CECAT7SubHeaderScan3D::gate_Duration, &CECAT7SubHeaderScan3D::setGate_Duration)
                 .def_property("r_Wave_Offset", &CECAT7SubHeaderScan3D::r_Wave_Offset, &CECAT7SubHeaderScan3D::setR_Wave_Offset)
                 .def_property("num_Accepted_Beats", &CECAT7SubHeaderScan3D::num_Accepted_Beats, &CECAT7SubHeaderScan3D::setNum_Accepted_Beats)
                 .def_property("scale_Factor", &CECAT7SubHeaderScan3D::scale_Factor, &CECAT7SubHeaderScan3D::setScale_Factor)
                 .def_property("scan_Min", &CECAT7SubHeaderScan3D::scan_Min, &CECAT7SubHeaderScan3D::setScan_Min)
                 .def_property("scan_Max", &CECAT7SubHeaderScan3D::scan_Max, &CECAT7SubHeaderScan3D::setScan_Max)
                 .def_property("prompts", &CECAT7SubHeaderScan3D::prompts, &CECAT7SubHeaderScan3D::setPrompts)
                 .def_property("delayed", &CECAT7SubHeaderScan3D::delayed, &CECAT7SubHeaderScan3D::setDelayed)
                 .def_property("multiples", &CECAT7SubHeaderScan3D::multiples, &CECAT7SubHeaderScan3D::setMultiples)
                 .def_property("net_Trues", &CECAT7SubHeaderScan3D::net_Trues, &CECAT7SubHeaderScan3D::setNet_Trues)
                 .def_property("tot_Avg_Cor", &CECAT7SubHeaderScan3D::tot_Avg_Cor, &CECAT7SubHeaderScan3D::setTot_Avg_Cor)
                 .def_property("tot_Avg_Uncor", &CECAT7SubHeaderScan3D::tot_Avg_Uncor, &CECAT7SubHeaderScan3D::setTot_Avg_Uncor)
                 .def_property("total_Coin_Rate", &CECAT7SubHeaderScan3D::total_Coin_Rate, &CECAT7SubHeaderScan3D::setTotal_Coin_Rate)
                 .def_property("frame_Start_Time", &CECAT7SubHeaderScan3D::frame_Start_Time, &CECAT7SubHeaderScan3D::setFrame_Start_Time)
                 .def_property("frame_Duration", &CECAT7SubHeaderScan3D::frame_Duration, &CECAT7SubHeaderScan3D::setFrame_Duration)
                 .def_property("deadtime_Correction_Factor", &CECAT7SubHeaderScan3D::deadtime_Correction_Factor, &CECAT7SubHeaderScan3D::setDeadtime_Correction_Factor)
                 .def("cti_Reserved", &CECAT7SubHeaderScan3D::cti_Reserved)
                 .def("cti_Reserved", &CECAT7SubHeaderScan3D::setCTI_Reserved)
                 .def("user_Reserved", &CECAT7SubHeaderScan3D::user_Reserved)
                 .def("user_Reserved", &CECAT7SubHeaderScan3D::setUser_Reserved)
                 .def("uncor_Singles", &CECAT7SubHeaderScan3D::uncor_Singles)
                 .def("uncor_Singles", &CECAT7SubHeaderScan3D::setUncor_Singles);
 
  py::enum_<CECAT7SubHeaderScan3D::Storage_Order>(ecat7SubHeaderScan3D, "Storage_Order")
    .value("ViewMode", CECAT7SubHeaderScan3D::ViewMode)
    .value("VolumeMode", CECAT7SubHeaderScan3D::VolumeMode)
    .export_values();

  py::enum_<CECAT7SubHeaderScan3D::Corrections_Applied>(ecat7SubHeaderScan3D, "Corrections_Applied")
    .value("Norm", CECAT7SubHeaderScan3D::Norm)
    .value("Atten", CECAT7SubHeaderScan3D::Atten)
    .value("Smooth", CECAT7SubHeaderScan3D::Smooth)
    .export_values();
}
