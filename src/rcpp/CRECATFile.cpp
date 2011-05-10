#include "CRECATFile.h"

#include <iostream>
#include <Rcpp.h>

#include <CECATFile.h>
#include <CECAT7MainHeader.h>
#include <CECAT7SubHeaderImage.h>

using namespace std;

CRECATFile::CRECATFile(const QString& filename, CECATMainHeader::Type fileType )
  : CECATFile(filename, fileType)
{
}

bool CRECATFile::readMainHeader_Rcpp(Rcpp::List& mainHeader)
{
  bool result = false;

  CECATMainHeader* cppMainHeader = NULL;
  if(readMainHeader(cppMainHeader) == true)
  {
    if(format() == CECATFile::ECAT7)
    {
      CECAT7MainHeader *mHeader = static_cast<CECAT7MainHeader*>(cppMainHeader);
      
      string magic_number = mHeader->magic_Number();
      string original_file_name = mHeader->original_File_Name();
      short sw_version = mHeader->sw_Version();
      short system_type = mHeader->system_Type();
      int file_type = mHeader->file_Type();
      string serial_number = mHeader->serial_Number();
      Rcpp::Datetime scan_start_time = Rcpp::Datetime(mHeader->scan_Start_Time());
      string isotope_name = mHeader->isotope_Name();
      float isotope_halflife = mHeader->isotope_Halflife();
      string radiopharmaceutical = mHeader->radiopharmaceutical();
      float gantry_tilt = mHeader->gantry_Tilt();
      float gantry_rotation = mHeader->gantry_Rotation();
      float bed_elevation = mHeader->bed_Elevation();
      float intrinsic_tilt = mHeader->intrinsic_Tilt();
      short wobble_speed = mHeader->wobble_Speed();
      int transm_source_type = mHeader->transm_Source_Type();
      float distance_scanned = mHeader->distance_Scanned();
      float transaxial_fov = mHeader->transaxial_FOV();
      int angular_compression = mHeader->angular_Compression();
      int coin_samp_mode = mHeader->coin_Samp_Mode();
      int axial_samp_mode = mHeader->axial_Samp_Mode();
      float ecat_calibration_factor = mHeader->calibration_Factor();
      int calibration_units = mHeader->calibration_Units();
      int calibration_units_label = mHeader->calibration_Units_Label();
      int compression_code = mHeader->compression_Code();
      string study_type = mHeader->study_Type();
      string patient_id = mHeader->patient_ID();
      string patient_name = mHeader->patient_Name();
   
      char sex = static_cast<char>(mHeader->patient_Sex());
      char dexterity = static_cast<char>(mHeader->patient_Dexterity());
      string patient_sex;
      string patient_dexterity;
      patient_sex = sex;
      patient_dexterity = dexterity;

      float patient_age = mHeader->patient_Age();
      float patient_height = mHeader->patient_Height();
      float patient_weight = mHeader->patient_Weight();
      Rcpp::Datetime patient_birth_date = Rcpp::Datetime(mHeader->patient_Birth_Date());
      string physician_name = mHeader->physician_Name();
      string operator_name = mHeader->operator_Name();
      string study_description = mHeader->study_Description();
      int acquisition_type = mHeader->acquisition_Type();
      int patient_orientation = mHeader->patient_Orientation();
      string facility_name = mHeader->facility_Name();
      short num_planes = mHeader->num_Planes();
      int num_frames = mHeader->num_Frames();
      int num_gates = mHeader->num_Gates();
      int num_bed_pos = mHeader->num_Bed_Pos();
      float init_bed_position = mHeader->init_Bed_Position();
      // float bed_offset = mHeader->bed_Offset();
      float plane_separation = mHeader->plane_Separation();
      short lwr_sctr_thres = mHeader->lwr_Sctr_Thres();
      short lwr_true_thres = mHeader->lwr_True_Thres();
      short upr_true_thres = mHeader->upr_True_Thres();
      string user_process_code = mHeader->user_Process_Code();
      int acquisition_mode = mHeader->acquisition_Mode();
      float bin_size = mHeader->bin_Size();
      float branching_fraction = mHeader->branching_Fraction();
      Rcpp::Datetime dose_start_time = Rcpp::Datetime(mHeader->dose_Start_Time());
      float  dosage = mHeader->dosage();
      float  well_counter_corr_factor = mHeader->well_Counter_Corr_Factor();
      string data_units = mHeader->data_Units();
      int septa_state = mHeader->septa_State();
      // short cti_reserved = mHeader->cti_Reserved(const short i) const;

      mainHeader.push_back(magic_number, "magic_number");
      mainHeader.push_back(original_file_name, "original_file_name");
      mainHeader.push_back(sw_version, "sw_version");
      mainHeader.push_back(system_type, "system_type");
      mainHeader.push_back(file_type, "file_type");
      mainHeader.push_back(serial_number, "serial_number");
      mainHeader.push_back(scan_start_time, "scan_start_time");
      mainHeader.push_back(isotope_name, "isotope_name");
      mainHeader.push_back(isotope_halflife, "isotope_halflife");
      mainHeader.push_back(radiopharmaceutical, "radiopharmaceutical");
      mainHeader.push_back(gantry_tilt, "gantry_tilt");
      mainHeader.push_back(gantry_rotation, "gantry_rotation");
      mainHeader.push_back(bed_elevation, "bed_elevation");
      mainHeader.push_back(intrinsic_tilt, "intrinsic_tilt");
      mainHeader.push_back(wobble_speed, "wobble_speed");
      mainHeader.push_back(transm_source_type, "transm_source_type");
      mainHeader.push_back(distance_scanned, "distance_scanned");
      mainHeader.push_back(transaxial_fov, "transaxial_fov");
      mainHeader.push_back(angular_compression, "angular_compression");
      mainHeader.push_back(coin_samp_mode, "coin_samp_mode");
      mainHeader.push_back(axial_samp_mode, "axial_samp_mode");
      mainHeader.push_back(ecat_calibration_factor, "ecat_calibration_factor");
      mainHeader.push_back(calibration_units, "calibration_units");
      mainHeader.push_back(calibration_units_label, "calibration_units_label");
      mainHeader.push_back(compression_code, "compression_code");
      mainHeader.push_back(study_type, "study_type");
      mainHeader.push_back(patient_id, "patient_id");
      mainHeader.push_back(patient_name, "patient_name");
      mainHeader.push_back(patient_sex, "patient_sex");
      mainHeader.push_back(patient_dexterity, "patient_dexterity");
      mainHeader.push_back(patient_age, "patient_age");
      mainHeader.push_back(patient_height, "patient_height");
      mainHeader.push_back(patient_weight, "patient_weight");
      mainHeader.push_back(patient_birth_date, "patient_birth_date");
      mainHeader.push_back(physician_name, "physician_name");
      mainHeader.push_back(operator_name, "operator_name");
      mainHeader.push_back(study_description, "study_description");
      mainHeader.push_back(acquisition_type, "acquisition_type");
      mainHeader.push_back(patient_orientation, "patient_orientation");
      mainHeader.push_back(facility_name, "facility_name");
      mainHeader.push_back(num_planes, "num_planes");
      mainHeader.push_back(num_frames, "num_frames");
      mainHeader.push_back(num_gates, "num_gates");
      mainHeader.push_back(num_bed_pos, "num_bed_pos");
      mainHeader.push_back(init_bed_position, "init_bed_position");
      // float bed_offset = mHeader->bed_Offset();
      mainHeader.push_back(plane_separation, "plane_separation");
      mainHeader.push_back(lwr_sctr_thres, "lwr_sctr_thres");
      mainHeader.push_back(lwr_true_thres, "lwr_true_thres");
      mainHeader.push_back(upr_true_thres, "upr_true_thres");
      mainHeader.push_back(user_process_code, "user_process_code");
      mainHeader.push_back(acquisition_mode, "acquisition_mode");
      mainHeader.push_back(bin_size, "bin_size");
      mainHeader.push_back(branching_fraction, "branching_fraction");
      mainHeader.push_back(dose_start_time, "dose_start_time");
      mainHeader.push_back( dosage, "dosage");
      mainHeader.push_back( well_counter_corr_factor, "well_counter_corr_factor");
      mainHeader.push_back(data_units, "data_units");
      mainHeader.push_back(septa_state, "septa_state");

      result = true;
    }
    else
      cerr << "ERROR: provided input file is not an EACAT7 file." << endl;

    delete cppMainHeader;
  }

  return result;
}

bool CRECATFile::readSubHeader_Rcpp(Rcpp::List& subHeader,
                                    short frame, short plane, short gate, short bed, short data)
{
  bool result = false;
  CECATSubHeader* sHead = NULL;

  if(readSubHeader(sHead, frame, plane, gate, bed, data) == true)
  {
    if(format() == CECATFile::ECAT7)
    {
      CECAT7SubHeaderImage* sHeadImage = static_cast<CECAT7SubHeaderImage*>(sHead);
      int data_type = sHeadImage->data_Type();
      short x_dimension = sHeadImage->x_Dimension();
      short y_dimension = sHeadImage->y_Dimension();
      short z_dimension = sHeadImage->z_Dimension();
      float scale_factor = sHeadImage->scale_Factor();
      float x_pixelsize = sHeadImage->x_Pixel_Size();
      float y_pixelsize = sHeadImage->y_Pixel_Size();
      float z_pixelsize = sHeadImage->z_Pixel_Size();
      unsigned int frame_duration = sHeadImage->frame_Duration() / 1000; // [s]
      unsigned int frame_start = sHeadImage->frame_Start_Time() / 1000;  // [s]

      subHeader = Rcpp::List::create(Rcpp::Named("data_type") = data_type,
                                     Rcpp::Named("x_dimension") = x_dimension,
                                     Rcpp::Named("y_dimension") = y_dimension,
                                     Rcpp::Named("z_dimension") = z_dimension,
                                     Rcpp::Named("scale_factor") = scale_factor,
                                     Rcpp::Named("x_pixelsize") = x_pixelsize,
                                     Rcpp::Named("y_pixelsize") = y_pixelsize,
                                     Rcpp::Named("z_pixelsize") = z_pixelsize,
                                     Rcpp::Named("frame_duration") = frame_duration,
                                     Rcpp::Named("frame_start") = frame_start);

      result = true;
    }
    else
      cerr << "ERROR: provided input file is not an EACAT7 file." << endl;

    delete sHead;
  }

  return result;
}
