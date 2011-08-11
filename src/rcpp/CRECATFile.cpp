#include "CRECATFile.h"

#include <iostream>

#include <Rcpp.h>

#include "CECATFile.h"
#include "CECAT7MainHeader.h"
#include "CECAT7SubHeaderImage.h"

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

      vector<float> bed_offset(15);
      for(unsigned short i=0; i < bed_offset.size(); ++i)
        bed_offset[i] = mHeader->bed_Offset(i);

      float plane_separation = mHeader->plane_Separation();
      short lwr_sctr_thres = mHeader->lwr_Sctr_Thres();
      short lwr_true_thres = mHeader->lwr_True_Thres();
      short upr_true_thres = mHeader->upr_True_Thres();
      string user_process_code = mHeader->user_Process_Code();
      int acquisition_mode = mHeader->acquisition_Mode();
      float bin_size = mHeader->bin_Size();
      float branching_fraction = mHeader->branching_Fraction();
      Rcpp::Datetime dose_start_time = Rcpp::Datetime(mHeader->dose_Start_Time());
      float dosage = mHeader->dosage();
      float well_counter_corr_factor = mHeader->well_Counter_Corr_Factor();
      string data_units = mHeader->data_Units();
      int septa_state = mHeader->septa_State();

      vector<short> cti_reserved(6);
      for(unsigned short i=0; i < cti_reserved.size(); ++i)
        cti_reserved[i] = mHeader->cti_Reserved(i);

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
      mainHeader.push_back(bed_offset, "bed_offset");
      mainHeader.push_back(plane_separation, "plane_separation");
      mainHeader.push_back(lwr_sctr_thres, "lwr_sctr_thres");
      mainHeader.push_back(lwr_true_thres, "lwr_true_thres");
      mainHeader.push_back(upr_true_thres, "upr_true_thres");
      mainHeader.push_back(user_process_code, "user_process_code");
      mainHeader.push_back(acquisition_mode, "acquisition_mode");
      mainHeader.push_back(bin_size, "bin_size");
      mainHeader.push_back(branching_fraction, "branching_fraction");
      mainHeader.push_back(dose_start_time, "dose_start_time");
      mainHeader.push_back(dosage, "dosage");
      mainHeader.push_back(well_counter_corr_factor, "well_counter_corr_factor");
      mainHeader.push_back(data_units, "data_units");
      mainHeader.push_back(septa_state, "septa_state");
      mainHeader.push_back(cti_reserved, "cti_reserved");
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
      int data_type = static_cast<int>(sHeadImage->data_Type());
      short num_dimensions = sHeadImage->num_Dimensions();
      short x_dimension = sHeadImage->x_Dimension();
      short y_dimension = sHeadImage->y_Dimension();
      short z_dimension = sHeadImage->z_Dimension();
      float x_offset = sHeadImage->x_Offset();
      float y_offset = sHeadImage->y_Offset();
      float z_offset = sHeadImage->z_Offset();
      float recon_zoom = sHeadImage->recon_Zoom();
      float scale_factor = sHeadImage->scale_Factor();
      short image_min = sHeadImage->image_Min();
      short image_max = sHeadImage->image_Max();
      float x_pixelsize = sHeadImage->x_Pixel_Size();
      float y_pixelsize = sHeadImage->y_Pixel_Size();
      float z_pixelsize = sHeadImage->z_Pixel_Size();
      unsigned int frame_duration = sHeadImage->frame_Duration();
      unsigned int frame_start = sHeadImage->frame_Start_Time();

      int filter_code = sHeadImage->filter_Code();
      float x_resolution = sHeadImage->x_Resolution();
      float y_resolution = sHeadImage->y_Resolution();
      float z_resolution = sHeadImage->z_Resolution();
      float num_r_elements = sHeadImage->num_R_Elements();
      float num_angles = sHeadImage->num_Angles();
      float z_rotation_angle = sHeadImage->z_Rotation_Angle();
      float decay_corr_fctr = sHeadImage->decay_Corr_Fctr();
      unsigned int processing_code = sHeadImage->processing_Code();
      unsigned int gate_duration = sHeadImage->gate_Duration();
      unsigned int r_wave_offset = sHeadImage->r_Wave_Offset();
      unsigned int num_accepted_beats = sHeadImage->num_Accepted_Beats();
      float filter_cutoff_frequency = sHeadImage->filter_Cutoff_Frequency();
      float filter_resolution = sHeadImage->filter_Resolution();
      float filter_ramp_slope = sHeadImage->filter_Ramp_Slope();
      short filter_order = sHeadImage->filter_Order();
      float filter_scatter_fraction = sHeadImage->filter_Scatter_Fraction();
      float filter_scatter_slope = sHeadImage->filter_Scatter_Slope();
      string annotation = sHeadImage->annotation();
      float mt_1_1 = sHeadImage->mt_1_1();
      float mt_1_2 = sHeadImage->mt_1_2();
      float mt_1_3 = sHeadImage->mt_1_3();
      float mt_2_1 = sHeadImage->mt_2_1();
      float mt_2_2 = sHeadImage->mt_2_2();
      float mt_2_3 = sHeadImage->mt_2_3();
      float mt_3_1 = sHeadImage->mt_3_1();
      float mt_3_2 = sHeadImage->mt_3_2();
      float mt_3_3 = sHeadImage->mt_3_3();
      float rfilter_cutoff = sHeadImage->rfilter_Cutoff();
      float rfilter_resolution = sHeadImage->rfilter_Resolution();
      int rfilter_code = sHeadImage->rfilter_Code();
      short rfilter_order = sHeadImage->rfilter_Order();
      float zfilter_cutoff = sHeadImage->zfilter_Cutoff();
      float zfilter_resolution = sHeadImage->zfilter_Resolution();
      int zfilter_code = sHeadImage->zfilter_Code();
      short zfilter_order = sHeadImage->zfilter_Order();
      float mt_1_4 = sHeadImage->mt_1_4();
      float mt_2_4 = sHeadImage->mt_2_4();
      float mt_3_4 = sHeadImage->mt_3_4();
      int scatter_type = sHeadImage->scatter_Type();
      int recon_type = sHeadImage->recon_Type();
      short recon_views = sHeadImage->recon_Views();

      vector<short> cti_reserved(87);
      for(unsigned short i=0; i < cti_reserved.size(); ++i)
        cti_reserved[i] = sHeadImage->cti_Reserved(i);

      vector<short> user_reserved(48);
      for(unsigned short i=0; i < user_reserved.size(); ++i)
        user_reserved[i] = sHeadImage->user_Reserved(i);

      subHeader.push_back(data_type, "data_type");
      subHeader.push_back(num_dimensions, "num_dimensions");
      subHeader.push_back(x_dimension, "x_dimension");
      subHeader.push_back(y_dimension, "y_dimension");
      subHeader.push_back(z_dimension, "z_dimension");
      subHeader.push_back(x_offset, "x_offset");
      subHeader.push_back(y_offset, "y_offset");
      subHeader.push_back(z_offset, "z_offset");
      subHeader.push_back(recon_zoom, "recon_zoom");
      subHeader.push_back(scale_factor, "scale_factor");
      subHeader.push_back(image_min, "image_min");
      subHeader.push_back(image_max, "image_max");
      subHeader.push_back(x_pixelsize, "x_pixelsize");
      subHeader.push_back(y_pixelsize, "y_pixelsize");
      subHeader.push_back(z_pixelsize, "z_pixelsize");
      subHeader.push_back(frame_duration, "frame_duration");
      subHeader.push_back(frame_start, "frame_start");

      subHeader.push_back(filter_code, "filter_code");
      subHeader.push_back(x_resolution, "x_resolution");
      subHeader.push_back(y_resolution, "y_resolution");
      subHeader.push_back(z_resolution, "z_resolution");
      subHeader.push_back(num_r_elements, "num_r_elements");
      subHeader.push_back(num_angles, "num_angles");
      subHeader.push_back(z_rotation_angle, "z_rotation_angle");
      subHeader.push_back(decay_corr_fctr, "decay_corr_fctr");
      subHeader.push_back(processing_code, "processing_code");
      subHeader.push_back(gate_duration, "gate_duration");
      subHeader.push_back(r_wave_offset, "r_wave_offset");
      subHeader.push_back(num_accepted_beats, "num_accepted_beats");
      subHeader.push_back(filter_cutoff_frequency, "filter_cutoff_frequency");
      subHeader.push_back(filter_resolution, "filter_resolution");
      subHeader.push_back(filter_ramp_slope, "filter_ramp_slope");
      subHeader.push_back(filter_order, "filter_order");
      subHeader.push_back(filter_scatter_fraction, "filter_scatter_fraction");
      subHeader.push_back(filter_scatter_slope, "filter_scatter_slope");
      subHeader.push_back(annotation, "annotation");
      subHeader.push_back(mt_1_1, "mt_1_1");
      subHeader.push_back(mt_1_2, "mt_1_2");
      subHeader.push_back(mt_1_3, "mt_1_3");
      subHeader.push_back(mt_2_1, "mt_2_1");
      subHeader.push_back(mt_2_2, "mt_2_2");
      subHeader.push_back(mt_2_3, "mt_2_3");
      subHeader.push_back(mt_3_1, "mt_3_1");
      subHeader.push_back(mt_3_2, "mt_3_2");
      subHeader.push_back(mt_3_3, "mt_3_3");
      subHeader.push_back(rfilter_cutoff, "rfilter_cutoff");
      subHeader.push_back(rfilter_resolution, "rfilter_resolution");
      subHeader.push_back(rfilter_code, "rfilter_code");
      subHeader.push_back(rfilter_order, "rfilter_order");
      subHeader.push_back(zfilter_cutoff, "zfilter_cutoff");
      subHeader.push_back(zfilter_resolution, "zfilter_resolution");
      subHeader.push_back(zfilter_code, "zfilter_code");
      subHeader.push_back(zfilter_order, "zfilter_order");
      subHeader.push_back(mt_1_4, "mt_1_4");
      subHeader.push_back(mt_2_4, "mt_2_4");
      subHeader.push_back(mt_3_4, "mt_3_4");
      subHeader.push_back(scatter_type, "scatter_type");
      subHeader.push_back(recon_type, "recon_type");
      subHeader.push_back(recon_views, "recon_views");

      subHeader.push_back(cti_reserved, "cti_reserved");
      subHeader.push_back(user_reserved, "user_reserved");

      result = true;
    }
    else
      cerr << "ERROR: provided input file is not an EACAT7 file." << endl;

    delete sHead;
  }

  return result;
}

bool CRECATFile::writeMainHeader_Rcpp(Rcpp::List& mainHeader)
{
  bool result = false;

  // first we have to create a new ecat main header
  CECATMainHeader* mHeader = NULL;
  mHeader = createMainHeaderFromRcppMainHeader(mainHeader);
  if(mHeader == NULL)
  {
    cerr << "ERROR: could not create new main header in destination file." << endl;
    result = false;
  }
  else
  {
    result = writeMainHeader(*mHeader);
    delete mHeader;
  }

  return result;
}

bool CRECATFile::writeSubHeader_Rcpp(Rcpp::List& subHeader,
                         short frame, short plane, short gate, short bed, short data)
{
  bool result = false;

  if(format() == CECATFile::ECAT7)
  {
    int data_type = Rcpp::as<int>(subHeader["data_type"]);
    short num_dimensions = Rcpp::as<short>(subHeader["num_dimensions"]);
    short x_dimension = Rcpp::as<short>(subHeader["x_dimension"]);
    short y_dimension = Rcpp::as<short>(subHeader["y_dimension"]);
    short z_dimension = Rcpp::as<short>(subHeader["z_dimension"]);
    float scale_factor = Rcpp::as<float>(subHeader["scale_factor"]);
    float x_pixelsize = Rcpp::as<float>(subHeader["x_pixelsize"]);
    float y_pixelsize = Rcpp::as<float>(subHeader["y_pixelsize"]);
    float z_pixelsize = Rcpp::as<float>(subHeader["z_pixelsize"]);

    float x_offset = Rcpp::as<float>(subHeader["x_offset"]);
    float y_offset = Rcpp::as<float>(subHeader["y_offset"]);
    float z_offset = Rcpp::as<float>(subHeader["z_offset"]);
    float recon_zoom = Rcpp::as<float>(subHeader["recon_zoom"]);
    
    short image_min = Rcpp::as<short>(subHeader["image_min"]);
    short image_max = Rcpp::as<short>(subHeader["image_max"]);

    unsigned int frame_duration = Rcpp::as<unsigned int>(subHeader["frame_duration"]);
    unsigned int frame_start = Rcpp::as<unsigned int>(subHeader["frame_start"]);

    int filter_code = Rcpp::as<int>(subHeader["filter_code"]);
    float x_resolution = Rcpp::as<float>(subHeader["x_resolution"]);
    float y_resolution = Rcpp::as<float>(subHeader["y_resolution"]);
    float z_resolution = Rcpp::as<float>(subHeader["z_resolution"]);
    float num_r_elements = Rcpp::as<float>(subHeader["num_r_elements"]);
    float num_angles = Rcpp::as<float>(subHeader["num_angles"]);
    float z_rotation_angle = Rcpp::as<float>(subHeader["z_rotation_angle"]);
    float decay_corr_fctr = Rcpp::as<float>(subHeader["decay_corr_fctr"]);

    unsigned int processing_code = Rcpp::as<unsigned int>(subHeader["processing_code"]);
    unsigned int gate_duration = Rcpp::as<unsigned int>(subHeader["gate_duration"]);
    unsigned int r_wave_offset = Rcpp::as<unsigned int>(subHeader["r_wave_offset"]);
    unsigned int num_accepted_beats = Rcpp::as<unsigned int>(subHeader["num_accepted_beats"]);

    float filter_cutoff_frequency = Rcpp::as<float>(subHeader["filter_cutoff_frequency"]);
    float filter_resolution = Rcpp::as<float>(subHeader["filter_resolution"]);
    float filter_ramp_slope = Rcpp::as<float>(subHeader["filter_ramp_slope"]);
    short filter_order = Rcpp::as<short>(subHeader["filter_order"]);
    float filter_scatter_fraction = Rcpp::as<float>(subHeader["filter_scatter_fraction"]);
    float filter_scatter_slope = Rcpp::as<float>(subHeader["filter_scatter_slope"]);
    string annotation = Rcpp::as<string>(subHeader["annotation"]);
    float mt_1_1 = Rcpp::as<float>(subHeader["mt_1_1"]);
    float mt_1_2 = Rcpp::as<float>(subHeader["mt_1_2"]);
    float mt_1_3 = Rcpp::as<float>(subHeader["mt_1_3"]);
    float mt_2_1 = Rcpp::as<float>(subHeader["mt_2_1"]);
    float mt_2_2 = Rcpp::as<float>(subHeader["mt_2_2"]);
    float mt_2_3 = Rcpp::as<float>(subHeader["mt_2_3"]);
    float mt_3_1 = Rcpp::as<float>(subHeader["mt_3_1"]);
    float mt_3_2 = Rcpp::as<float>(subHeader["mt_3_2"]);
    float mt_3_3 = Rcpp::as<float>(subHeader["mt_3_3"]);
    float rfilter_cutoff = Rcpp::as<float>(subHeader["rfilter_cutoff"]);
    float rfilter_resolution = Rcpp::as<float>(subHeader["rfilter_resolution"]);
    int rfilter_code = Rcpp::as<int>(subHeader["rfilter_code"]);
    short rfilter_order = Rcpp::as<short>(subHeader["rfilter_order"]);
    float zfilter_cutoff = Rcpp::as<float>(subHeader["zfilter_cutoff"]);
    float zfilter_resolution = Rcpp::as<float>(subHeader["zfilter_resolution"]);
    int zfilter_code = Rcpp::as<int>(subHeader["zfilter_code"]);
    short zfilter_order = Rcpp::as<short>(subHeader["zfilter_order"]);
    float mt_1_4 = Rcpp::as<float>(subHeader["mt_1_4"]);
    float mt_2_4 = Rcpp::as<float>(subHeader["mt_2_4"]);
    float mt_3_4 = Rcpp::as<float>(subHeader["mt_3_4"]);
    int scatter_type = Rcpp::as<int>(subHeader["scatter_type"]);
    int recon_type = Rcpp::as<int>(subHeader["recon_type"]);
    short recon_views = Rcpp::as<short>(subHeader["recon_views"]);
    vector<short> cti_reserved = Rcpp::as<vector<short> >(subHeader["cti_reserved"]);
    vector<short> user_reserved = Rcpp::as<vector<short> >(subHeader["user_reserved"]);

    CECAT7SubHeaderImage imageSubHeader;
    imageSubHeader.setData_Type(static_cast<CECATSubHeader::Data_Type>(data_type));
    imageSubHeader.setNum_Dimensions(num_dimensions);
    imageSubHeader.setX_Dimension(x_dimension);
    imageSubHeader.setY_Dimension(y_dimension);
    imageSubHeader.setZ_Dimension(z_dimension);
    imageSubHeader.setScale_Factor(scale_factor);
    imageSubHeader.setX_Pixel_Size(x_pixelsize);
    imageSubHeader.setY_Pixel_Size(y_pixelsize);
    imageSubHeader.setZ_Pixel_Size(z_pixelsize);
    imageSubHeader.setX_Offset(x_offset);
    imageSubHeader.setY_Offset(y_offset);
    imageSubHeader.setZ_Offset(z_offset);
    imageSubHeader.setRecon_Zoom(recon_zoom);
    imageSubHeader.setScale_Factor(scale_factor);
    imageSubHeader.setImage_Min(image_min);
    imageSubHeader.setImage_Max(image_max);
    imageSubHeader.setFrame_Duration(frame_duration);
    imageSubHeader.setFrame_Start_Time(frame_start);
    imageSubHeader.setFilter_Code(static_cast<CECAT7SubHeaderImage::Filter_Code>(filter_code));
    imageSubHeader.setX_Resolution(x_resolution);
    imageSubHeader.setY_Resolution(y_resolution);
    imageSubHeader.setZ_Resolution(z_resolution);
    imageSubHeader.setNum_R_Elements(num_r_elements);
    imageSubHeader.setNum_Angles(num_angles);
    imageSubHeader.setZ_Rotation_Angle(z_rotation_angle);
    imageSubHeader.setDecay_Corr_Fctr(decay_corr_fctr);
    imageSubHeader.setProcessing_Code(processing_code);
    imageSubHeader.setGate_Duration(gate_duration);
    imageSubHeader.setR_Wave_Offset(r_wave_offset);
    imageSubHeader.setNum_Accepted_Beats(num_accepted_beats);
    imageSubHeader.setFilter_Cutoff_Frequency(filter_cutoff_frequency);
    imageSubHeader.setFilter_Resolution(filter_resolution);
    imageSubHeader.setFilter_Ramp_Slope(filter_ramp_slope);
    imageSubHeader.setFilter_Order(filter_order);
    imageSubHeader.setFilter_Scatter_Fraction(filter_scatter_fraction);
    imageSubHeader.setFilter_Scatter_Slope(filter_scatter_slope);
    imageSubHeader.setAnnotation(annotation.c_str());
    imageSubHeader.setMT_1_1(mt_1_1);
    imageSubHeader.setMT_1_2(mt_1_2);
    imageSubHeader.setMT_1_3(mt_1_3);
    imageSubHeader.setMT_2_1(mt_2_1);
    imageSubHeader.setMT_2_2(mt_2_2);
    imageSubHeader.setMT_2_3(mt_2_3);
    imageSubHeader.setMT_3_1(mt_3_1);
    imageSubHeader.setMT_3_2(mt_3_2);
    imageSubHeader.setMT_3_3(mt_3_3);
    imageSubHeader.setRFilter_Cutoff(rfilter_cutoff);
    imageSubHeader.setRFilter_Resolution(rfilter_resolution);
    imageSubHeader.setRFilter_Code(static_cast<CECAT7SubHeaderImage::Filter_Code>(rfilter_code));
    imageSubHeader.setRFilter_Order(rfilter_order);
    imageSubHeader.setZFilter_Cutoff(zfilter_cutoff);
    imageSubHeader.setZFilter_Resolution(zfilter_resolution);
    imageSubHeader.setZFilter_Code(static_cast<CECAT7SubHeaderImage::Filter_Code>(zfilter_code));
    imageSubHeader.setZFilter_Order(zfilter_order);
    imageSubHeader.setMT_1_4(mt_1_4);
    imageSubHeader.setMT_2_4(mt_2_4);
    imageSubHeader.setMT_3_4(mt_3_4);
    imageSubHeader.setScatter_Type(static_cast<CECAT7SubHeaderImage::Scatter_Type>(scatter_type));
    imageSubHeader.setRecon_Type(static_cast<CECAT7SubHeaderImage::Recon_Type>(recon_type));
    imageSubHeader.setRecon_Views(recon_views);

    for(unsigned int i=0; i < cti_reserved.size(); ++i)
      imageSubHeader.setCTI_Reserved(i, cti_reserved[i]);

    for(unsigned int i=0; i < user_reserved.size(); ++i)
      imageSubHeader.setUser_Reserved(i, user_reserved[i]);

    result = writeSubHeader(imageSubHeader, frame, plane, gate, bed, data);
  }
  else
    cerr << "ERROR: provided input file is not an EACAT7 file." << endl;

  return result;
}


CECATMainHeader* CRECATFile::createMainHeaderFromRcppMainHeader(Rcpp::List& rMainHeader)
{
  CECATMainHeader* pMainHeader = NULL;

  if(isOpen())
  {
    switch(format())
    {
      case CECATFile::ECAT7:
      {
        pMainHeader = createEmptyMainHeader();
        CECAT7MainHeader *ecat7MainHeader = static_cast<CECAT7MainHeader*>(pMainHeader);

        try
        { 
          string magic_number = Rcpp::as<string>(rMainHeader["magic_number"]);
          string original_file_name = Rcpp::as<string>(rMainHeader["original_file_name"]);
          short sw_version = Rcpp::as<short>(rMainHeader["sw_version"]);
          short system_type = Rcpp::as<short>(rMainHeader["system_type"]);
          int file_type = Rcpp::as<int>(rMainHeader["file_type"]);
          string serial_number = Rcpp::as<string>(rMainHeader["serial_number"]);
          time_t scan_start_time = Rcpp::as<time_t>(rMainHeader["scan_start_time"]);
          string isotope_name = Rcpp::as<string>(rMainHeader["isotope_name"]);
          float isotope_halflife = Rcpp::as<float>(rMainHeader["isotope_halflife"]);
          string radiopharmaceutical = Rcpp::as<string>(rMainHeader["radiopharmaceutical"]);
          float gantry_tilt = Rcpp::as<float>(rMainHeader["gantry_tilt"]);
          float gantry_rotation = Rcpp::as<float>(rMainHeader["gantry_rotation"]);
          float bed_elevation = Rcpp::as<float>(rMainHeader["bed_elevation"]);
          float intrinsic_tilt = Rcpp::as<float>(rMainHeader["intrinsic_tilt"]);
          short wobble_speed = Rcpp::as<short>(rMainHeader["wobble_speed"]);
          int transm_source_type = Rcpp::as<int>(rMainHeader["transm_source_type"]);
          float distance_scanned = Rcpp::as<float>(rMainHeader["distance_scanned"]);
          float transaxial_fov = Rcpp::as<float>(rMainHeader["transaxial_fov"]);
          int angular_compression = Rcpp::as<int>(rMainHeader["angular_compression"]);
          int coin_samp_mode = Rcpp::as<int>(rMainHeader["coin_samp_mode"]);
          int axial_samp_mode = Rcpp::as<int>(rMainHeader["axial_samp_mode"]);
          float ecat_calibration_factor = Rcpp::as<float>(rMainHeader["ecat_calibration_factor"]);
          int calibration_units = Rcpp::as<int>(rMainHeader["calibration_units"]);
          int calibration_units_label = Rcpp::as<int>(rMainHeader["calibration_units_label"]);
          int compression_code = Rcpp::as<int>(rMainHeader["compression_code"]);
          string study_type = Rcpp::as<string>(rMainHeader["study_type"]);
          string patient_id = Rcpp::as<string>(rMainHeader["patient_id"]);
          string patient_name = Rcpp::as<string>(rMainHeader["patient_name"]);

          string sex = Rcpp::as<string>(rMainHeader["patient_sex"]);
          CECAT7MainHeader::Patient_Sex patient_sex;
          switch(sex[0])
          {
            case 'M': patient_sex = CECAT7MainHeader::Sex_Male; break;
            case 'F': patient_sex = CECAT7MainHeader::Sex_Female; break;
            case 'U':
            default: patient_sex = CECAT7MainHeader::Sex_Unknown; break;
          }

          string dexterity = Rcpp::as<string>(rMainHeader["patient_dexterity"]);
          CECAT7MainHeader::Patient_Dexterity patient_dexterity;
          switch(dexterity[0])
          {
            case 'R': patient_dexterity = CECAT7MainHeader::Dext_RT; break;
            case 'L': patient_dexterity = CECAT7MainHeader::Dext_LF; break;
            case 'U':
            default: patient_dexterity = CECAT7MainHeader::Dext_Unknown; break;
          }

          float patient_age = Rcpp::as<float>(rMainHeader["patient_age"]);
          float patient_height = Rcpp::as<float>(rMainHeader["patient_height"]);
          float patient_weight = Rcpp::as<float>(rMainHeader["patient_weight"]);
          time_t patient_birth_datetime = Rcpp::as<time_t>(rMainHeader["patient_birth_date"]);
          string physician_name = Rcpp::as<string>(rMainHeader["physician_name"]);
          string operator_name = Rcpp::as<string>(rMainHeader["operator_name"]);
          string study_description = Rcpp::as<string>(rMainHeader["study_description"]);
          int acquisition_type = Rcpp::as<int>(rMainHeader["acquisition_type"]);
          int patient_orientation = Rcpp::as<int>(rMainHeader["patient_orientation"]);
          string facility_name = Rcpp::as<string>(rMainHeader["facility_name"]);
          short num_planes = Rcpp::as<short>(rMainHeader["num_planes"]);
          int num_frames = Rcpp::as<int>(rMainHeader["num_frames"]);
          int num_gates = Rcpp::as<int>(rMainHeader["num_gates"]);
          int num_bed_pos = Rcpp::as<int>(rMainHeader["num_bed_pos"]);
          float init_bed_position = Rcpp::as<float>(rMainHeader["init_bed_position"]);
          vector<float> bed_offset = Rcpp::as<vector<float> >(rMainHeader["bed_offset"]);
          float plane_separation = Rcpp::as<float>(rMainHeader["plane_separation"]);
          short lwr_sctr_thres = Rcpp::as<short>(rMainHeader["lwr_sctr_thres"]);
          short lwr_true_thres = Rcpp::as<short>(rMainHeader["lwr_true_thres"]);
          short upr_true_thres = Rcpp::as<short>(rMainHeader["upr_true_thres"]);
          string user_process_code = Rcpp::as<string>(rMainHeader["user_process_code"]);
          int acquisition_mode = Rcpp::as<int>(rMainHeader["acquisition_mode"]);
          float bin_size = Rcpp::as<float>(rMainHeader["bin_size"]);
          float branching_fraction = Rcpp::as<float>(rMainHeader["branching_fraction"]);
          time_t dose_start_time = Rcpp::as<time_t>(rMainHeader["dose_start_time"]);
          float dosage = Rcpp::as<float>(rMainHeader["dosage"]);
          float well_counter_corr_factor = Rcpp::as<float>(rMainHeader["well_counter_corr_factor"]);
          string data_units = Rcpp::as<string>(rMainHeader["data_units"]);
          int septa_state = Rcpp::as<int>(rMainHeader["septa_state"]);
          vector<short> cti_reserved = Rcpp::as<vector<short> >(rMainHeader["cti_reserved"]);

          ecat7MainHeader->setMagic_Number(magic_number.c_str());
          ecat7MainHeader->setOriginal_File_Name(original_file_name.c_str());
          ecat7MainHeader->setSW_Version(sw_version);
          ecat7MainHeader->setSystem_Type(system_type);
          ecat7MainHeader->setFile_Type(static_cast<CECAT7MainHeader::File_Type>(file_type));
          ecat7MainHeader->setSerial_Number(serial_number.c_str());
          ecat7MainHeader->setScan_Start_Time(scan_start_time);
          ecat7MainHeader->setIsotope_Name(isotope_name.c_str());
          ecat7MainHeader->setIsotope_Halflife(isotope_halflife);
          ecat7MainHeader->setRadiopharmaceutical(radiopharmaceutical.c_str());
          ecat7MainHeader->setGantry_Tilt(gantry_tilt);
          ecat7MainHeader->setGantry_Rotation(gantry_rotation);
          ecat7MainHeader->setBed_Elevation(bed_elevation);
          ecat7MainHeader->setIntrinsic_Tilt(intrinsic_tilt);
          ecat7MainHeader->setWobble_Speed(wobble_speed);
          ecat7MainHeader->setTransm_Source_Type(static_cast<CECAT7MainHeader::Transm_Source_Type>(transm_source_type));
          ecat7MainHeader->setDistance_Scanned(distance_scanned);
          ecat7MainHeader->setTransaxial_FOV(transaxial_fov);
          ecat7MainHeader->setAngular_Compression(static_cast<CECAT7MainHeader::Angular_Compression>(angular_compression));
          ecat7MainHeader->setCoin_Samp_Mode(static_cast<CECAT7MainHeader::Coin_Samp_Mode>(coin_samp_mode));
          ecat7MainHeader->setAxial_Samp_Mode(static_cast<CECAT7MainHeader::Axial_Samp_Mode>(axial_samp_mode));
          ecat7MainHeader->setCalibration_Factor(ecat_calibration_factor);
          ecat7MainHeader->setCalibration_Units(static_cast<CECAT7MainHeader::Calibration_Units>(calibration_units));
          ecat7MainHeader->setCalibration_Units_Label(static_cast<CECAT7MainHeader::Calibration_Units_Label>(calibration_units_label));
          ecat7MainHeader->setCompression_Code(static_cast<CECAT7MainHeader::Compression_Code>(compression_code));
          ecat7MainHeader->setStudy_Type(study_type.c_str());
          ecat7MainHeader->setPatient_ID(patient_id.c_str());
          ecat7MainHeader->setPatient_Name(patient_name.c_str());
          ecat7MainHeader->setPatient_Sex(patient_sex);
          ecat7MainHeader->setPatient_Dexterity(patient_dexterity);
          ecat7MainHeader->setPatient_Age(patient_age);
          ecat7MainHeader->setPatient_Height(patient_height);
          ecat7MainHeader->setPatient_Weight(patient_weight);
          ecat7MainHeader->setPatient_Birth_Date(patient_birth_datetime);
          ecat7MainHeader->setPhysician_Name(physician_name.c_str());
          ecat7MainHeader->setOperator_Name(operator_name.c_str());
          ecat7MainHeader->setStudy_Description(study_description.c_str());
          ecat7MainHeader->setAcquisition_Type(static_cast<CECAT7MainHeader::Acquisition_Type>(acquisition_type));
          ecat7MainHeader->setPatient_Orientation(static_cast<CECAT7MainHeader::Patient_Orientation>(patient_orientation));
          ecat7MainHeader->setFacility_Name(facility_name.c_str());
          ecat7MainHeader->setNum_Planes(num_planes);
          ecat7MainHeader->setNum_Frames(num_frames);
          ecat7MainHeader->setNum_Gates(num_gates);
          ecat7MainHeader->setNum_Bed_Pos(num_bed_pos);
          ecat7MainHeader->setInit_Bed_Position(init_bed_position);

          for(unsigned int i=0; i < bed_offset.size(); ++i)
            ecat7MainHeader->setBed_Offset(i, bed_offset[i]);

          ecat7MainHeader->setPlane_Separation(plane_separation);
          ecat7MainHeader->setLwr_Sctr_Thres(lwr_sctr_thres);
          ecat7MainHeader->setLwr_True_Thres(lwr_true_thres);
          ecat7MainHeader->setUpr_True_Thres(upr_true_thres);
          ecat7MainHeader->setUser_Process_Code(user_process_code.c_str());
          ecat7MainHeader->setAcquisition_Mode(static_cast<CECAT7MainHeader::Acquisition_Mode>(acquisition_mode));
          ecat7MainHeader->setBin_Size(bin_size);
          ecat7MainHeader->setBranching_Fraction(branching_fraction);
          ecat7MainHeader->setDose_Start_Time(dose_start_time);
          ecat7MainHeader->setDosage(dosage);
          ecat7MainHeader->setWell_Counter_Corr_Factor(well_counter_corr_factor);
          ecat7MainHeader->setData_Units(data_units.c_str());
          ecat7MainHeader->setSepta_State(static_cast<CECAT7MainHeader::Septa_State>(septa_state));

          for(unsigned int i=0; i < cti_reserved.size(); ++i)
            ecat7MainHeader->setCTI_Reserved(i, cti_reserved[i]);
        }
        catch(Rcpp::index_out_of_bounds e)
        {
          // there is something wrong with the header entries
          cerr << "ERROR: there is something wrong with the header entries" << endl;
          delete pMainHeader;
          pMainHeader = NULL;
        }
        
        break;
      }

      default:
        cerr << "ERROR: only ECAT7 is supported." << endl;
    }
  }

  return pMainHeader;
}
