#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/chrono.h>

#include <CMedIOData>
#include <CECATFile>
#include <CECAT7MainHeader>
#include <CECAT7SubHeaderImage>

#include <QIODevice>
#include <QString>

#include <iostream>

namespace py = pybind11;

typedef enum
{
  FRAMES = 'f',
  GATES = 'g',
  BEDS = 'b'
} VolumeSelection;

#define ROUND(a) (std::round((a) * 1000.0) / 1000.0)

// special helper MedIOImage class to embed all necessary
// information and always return the pet data as float data
class MedIOImage
{
  public:
    MedIOImage(CMedIOData* mioData=nullptr)
      : m_numdim(0),
        m_xdim(0),
        m_ydim(0),
        m_zdim(0),
        m_tdim(0),
        m_mhead(py::dict{}),
        m_shead(py::list{}),
        m_mioData(mioData),
        m_data(nullptr)
    { }

    MedIOImage(CMedIOData* mioData, const MedIOImage& img)
      : m_numdim(img.m_numdim),
        m_xdim(img.m_xdim),
        m_ydim(img.m_ydim),
        m_zdim(img.m_zdim),
        m_tdim(img.m_tdim),
        m_mhead(img.m_mhead),
        m_shead(img.m_shead),
        m_mioData(mioData),
        m_data(nullptr)
    {
      m_data = new float[m_xdim * m_ydim * m_zdim * m_tdim];
      memcpy(m_data, img.m_data, sizeof(float) * (m_xdim * m_ydim * m_zdim * m_tdim));
    }

    MedIOImage(const py::array_t<float>& img)
     : m_numdim(0),
       m_xdim(0),
       m_ydim(0),
       m_zdim(0),
       m_tdim(0),
       m_mhead(py::dict{}),
       m_shead(py::list{}),
       m_mioData(nullptr),
       m_data(nullptr)
    {
      py::buffer_info buf = img.request();
      m_numdim = buf.ndim;
      m_xdim = buf.shape[0];
      m_ydim = buf.shape[1];
      m_zdim = buf.shape[2];
      m_tdim = buf.shape[3];
      m_data = new float[m_xdim * m_ydim * m_zdim * m_tdim];
      memcpy(m_data, buf.ptr, sizeof(float) * (m_xdim * m_ydim * m_zdim * m_tdim));
    }

    MedIOImage(const size_t xdim, const size_t ydim=1, const size_t zdim=1, const size_t tdim=1)
      : m_numdim(4),
        m_xdim(xdim),
        m_ydim(ydim),
        m_zdim(zdim),
        m_tdim(tdim),
        m_mhead(py::dict{}),
        m_shead(py::list{}),
        m_mioData(nullptr),
        m_data(nullptr)
    {
      m_data = new float[m_xdim * m_ydim * m_zdim * m_tdim];
      memset(m_data, 0, sizeof(float) * (m_xdim * m_ydim * m_zdim * m_tdim));
    }
 
    MedIOImage(const MedIOImage& img)
      : m_numdim(img.m_numdim),
        m_xdim(img.m_xdim),
        m_ydim(img.m_ydim),
        m_zdim(img.m_zdim),
        m_tdim(img.m_tdim),
        m_mhead(img.m_mhead),
        m_shead(img.m_shead),
        m_mioData(img.m_mioData),
        m_data(nullptr)
    {
      m_data = new float[m_xdim * m_ydim * m_zdim * m_tdim];
      memcpy(m_data, img.m_data, sizeof(float) * (m_xdim * m_ydim * m_zdim * m_tdim));
    }

    MedIOImage(MedIOImage&& img)
      : m_numdim(img.m_numdim),
        m_xdim(img.m_xdim),
        m_ydim(img.m_ydim),
        m_zdim(img.m_zdim),
        m_tdim(img.m_tdim),
        m_mhead(img.m_mhead),
        m_shead(img.m_shead),
        m_mioData(img.m_mioData),
        m_data(img.m_data)
    {
      img.m_numdim = 0;
      img.m_xdim = 0;
      img.m_ydim = 0;
      img.m_zdim = 0;
      img.m_tdim = 0;
      img.m_mhead = py::dict{};
      img.m_shead = py::list{};
      img.m_mioData = nullptr;
      img.m_data = nullptr;
    }

    ~MedIOImage()
    {
      delete[] m_data;
    }

    MedIOImage& operator=(const MedIOImage& img)
    {
      if(this == &img)
        return *this;

      delete[] m_data;
      m_numdim = img.m_numdim;
      m_xdim = img.m_xdim;
      m_ydim = img.m_ydim;
      m_zdim = img.m_zdim;
      m_tdim = img.m_tdim;
      m_mhead = img.m_mhead;
      m_shead = img.m_shead;
      m_mioData = img.m_mioData;
      m_data = new float[m_xdim * m_ydim * m_zdim * m_tdim];
      memcpy(m_data, img.m_data, sizeof(float) * (m_xdim * m_ydim * m_zdim * m_tdim));
    }

    MedIOImage& operator=(MedIOImage&& img)
    {
      if(&img != this)
      {
        delete[] m_data;
        m_numdim = img.m_numdim;
        m_xdim = img.m_xdim;
        m_ydim = img.m_ydim;
        m_zdim = img.m_zdim;
        m_tdim = img.m_tdim;
        m_mhead = img.m_mhead;
        m_shead = img.m_shead;
        m_mioData = img.m_mioData;
        m_data = img.m_data;
        img.m_numdim = 0;
        img.m_xdim = 0;
        img.m_ydim = 0;
        img.m_zdim = 0;
        img.m_tdim = 0;
        img.m_mhead = py::dict{};
        img.m_shead = py::list{};
        img.m_mioData = nullptr;
        img.m_data = nullptr;
      }
      return *this;
    }

    float operator()(const size_t x, const size_t y=0, const size_t z=0, const size_t t=0) const
    {
      return m_data[x * m_xdim + y * m_ydim + z * m_zdim + t * m_tdim];
    } 

    float& operator()(const size_t x, const size_t y=0, const size_t z=0, const size_t t=0)
    {
      return m_data[x * m_xdim + y * m_ydim + z * m_zdim + t * m_tdim];
    } 

    float* data() { return m_data; }
    CMedIOData* medIOData() { return m_mioData; }
    size_t numdim() const { return m_numdim; }
    size_t ndim() const { return m_numdim; }
    size_t xdim() const { return m_xdim; }
    size_t ydim() const { return m_ydim; }
    size_t zdim() const { return m_zdim; }
    size_t tdim() const { return m_tdim; }

    bool parseMainHeader(const CMedIOHeader* mainHeader)
    {
      bool result = false;

      if(mainHeader->headerFormat() == CMedIOHeader::ECATMainHeader)
      {
        const CECATMainHeader* ecatMainHeader = static_cast<const CECATMainHeader*>(mainHeader);
        if(ecatMainHeader->mainHeaderType() == CECATMainHeader::ECAT7MainHeader)
        {
          const CECAT7MainHeader* e7MainHeader = static_cast<const CECAT7MainHeader*>(ecatMainHeader);

          std::string magic_number = e7MainHeader->magic_Number();
          std::string original_file_name = e7MainHeader->original_File_Name();
          short sw_version = e7MainHeader->sw_Version();
          short system_type = e7MainHeader->system_Type();
          int file_type = e7MainHeader->file_Type();
          std::string serial_number = e7MainHeader->serial_Number();
          std::chrono::system_clock::time_point scan_start_time = std::chrono::system_clock::from_time_t(e7MainHeader->scan_Start_Time());
          std::string isotope_name = e7MainHeader->isotope_Name();
          double isotope_halflife = ROUND(e7MainHeader->isotope_Halflife());
          std::string radiopharmaceutical = e7MainHeader->radiopharmaceutical();
          double gantry_tilt = ROUND(e7MainHeader->gantry_Tilt());
          double gantry_rotation = ROUND(e7MainHeader->gantry_Rotation());
          double bed_elevation = ROUND(e7MainHeader->bed_Elevation());
          double intrinsic_tilt = ROUND(e7MainHeader->intrinsic_Tilt());
          short wobble_speed = e7MainHeader->wobble_Speed();
          int transm_source_type = e7MainHeader->transm_Source_Type();
          double distance_scanned = ROUND(e7MainHeader->distance_Scanned());
          double transaxial_fov = ROUND(e7MainHeader->transaxial_FOV());
          int angular_compression = e7MainHeader->angular_Compression();
          int coin_samp_mode = e7MainHeader->coin_Samp_Mode();
          int axial_samp_mode = e7MainHeader->axial_Samp_Mode();
          double ecat_calibration_factor = ROUND(e7MainHeader->calibration_Factor());
          int calibration_units = e7MainHeader->calibration_Units();
          int calibration_units_label = e7MainHeader->calibration_Units_Label();

          int compression_code = e7MainHeader->compression_Code();
          std::string study_type = e7MainHeader->study_Type();
          std::string patient_id = e7MainHeader->patient_ID();
          std::string patient_name = e7MainHeader->patient_Name();
    
          char sex = static_cast<char>(e7MainHeader->patient_Sex());
          char dexterity = static_cast<char>(e7MainHeader->patient_Dexterity());
          std::string patient_sex;
          std::string patient_dexterity;
          patient_sex = sex;
          patient_dexterity = dexterity;
    
          double patient_age = ROUND(e7MainHeader->patient_Age());
          double patient_height = ROUND(e7MainHeader->patient_Height());
          double patient_weight = ROUND(e7MainHeader->patient_Weight());
          std::chrono::system_clock::time_point patient_birth_date = std::chrono::system_clock::from_time_t(e7MainHeader->patient_Birth_Date());
          std::string physician_name = e7MainHeader->physician_Name();
          std::string operator_name = e7MainHeader->operator_Name();
          std::string study_description = e7MainHeader->study_Description();
          int acquisition_type = e7MainHeader->acquisition_Type();
          int patient_orientation = e7MainHeader->patient_Orientation();
          std::string facility_name = e7MainHeader->facility_Name();
          short num_planes = e7MainHeader->num_Planes();
          int num_frames = e7MainHeader->num_Frames();
          int num_gates = e7MainHeader->num_Gates();
          int num_bed_pos = e7MainHeader->num_Bed_Pos();
          double init_bed_position = ROUND(e7MainHeader->init_Bed_Position());

          std::vector<double> bed_offset(15);
          for(unsigned short i=0; i < bed_offset.size(); ++i)
            bed_offset[i] = ROUND(e7MainHeader->bed_Offset(i));
    
          double plane_separation = ROUND(e7MainHeader->plane_Separation());
          short lwr_sctr_thres = e7MainHeader->lwr_Sctr_Thres();
          short lwr_true_thres = e7MainHeader->lwr_True_Thres();
          short upr_true_thres = e7MainHeader->upr_True_Thres();
          std::string user_process_code = e7MainHeader->user_Process_Code();
          int acquisition_mode = e7MainHeader->acquisition_Mode();
          double bin_size = ROUND(e7MainHeader->bin_Size());
          double branching_fraction = ROUND(e7MainHeader->branching_Fraction());
          std::chrono::system_clock::time_point dose_start_time = std::chrono::system_clock::from_time_t(e7MainHeader->dose_Start_Time());
          double dosage = ROUND(e7MainHeader->dosage());
          double well_counter_corr_factor = ROUND(e7MainHeader->well_Counter_Corr_Factor());
          std::string data_units = e7MainHeader->data_Units();
          int septa_state = e7MainHeader->septa_State();
    
          std::vector<short> cti_reserved(6);
          for(unsigned short i=0; i < cti_reserved.size(); ++i)
            cti_reserved[i] = e7MainHeader->cti_Reserved(i);

          // stuff everything into the py::dict
          m_mhead = py::dict{};
          m_mhead["magic_number"] = py::cast(magic_number);
          m_mhead["original_file_name"] = py::cast(original_file_name);
          m_mhead["sw_version"] = py::cast(sw_version);
          m_mhead["system_type"] = py::cast(system_type);
          m_mhead["file_type"] = py::cast(file_type);
          m_mhead["serial_number"] = py::cast(serial_number);
          m_mhead["scan_start_time"] = py::cast(scan_start_time);
          m_mhead["isotope_name"] = py::cast(isotope_name);
          m_mhead["isotope_halflife"] = py::cast(isotope_halflife);
          m_mhead["radiopharmaceutical"] = py::cast(radiopharmaceutical);
          m_mhead["gantry_tilt"] = py::cast(gantry_tilt);
          m_mhead["gantry_rotation"] = py::cast(gantry_rotation);
          m_mhead["bed_elevation"] = py::cast(bed_elevation);
          m_mhead["intrinsic_tilt"] = py::cast(intrinsic_tilt);
          m_mhead["wobble_speed"] = py::cast(wobble_speed);
          m_mhead["transm_source_type"] = py::cast(transm_source_type);
          m_mhead["distance_scanned"] = py::cast(distance_scanned);
          m_mhead["transaxial_fov"] = py::cast(transaxial_fov);
          m_mhead["angular_compression"] = py::cast(angular_compression);
          m_mhead["coin_samp_mode"] = py::cast(coin_samp_mode);
          m_mhead["axial_samp_mode"] = py::cast(axial_samp_mode);
          m_mhead["ecat_calibration_factor"] = py::cast(ecat_calibration_factor);
          m_mhead["calibration_units"] = py::cast(calibration_units);
          m_mhead["calibration_units_label"] = py::cast(calibration_units_label);
          m_mhead["compression_code"] = py::cast(compression_code);
          m_mhead["study_type"] = py::cast(study_type);
          m_mhead["patient_id"] = py::cast(patient_id);
          m_mhead["patient_name"] = py::cast(patient_name);
          m_mhead["patient_sex"] = py::cast(patient_sex);
          m_mhead["patient_dexterity"] = py::cast(patient_dexterity);
          m_mhead["patient_age"] = py::cast(patient_age);
          m_mhead["patient_height"] = py::cast(patient_height);
          m_mhead["patient_weight"] = py::cast(patient_weight);
          m_mhead["patient_birth_date"] = py::cast(patient_birth_date);
          m_mhead["physician_name"] = py::cast(physician_name);
          m_mhead["operator_name"] = py::cast(operator_name);
          m_mhead["study_description"] = py::cast(study_description);
          m_mhead["acquisition_type"] = py::cast(acquisition_type);
          m_mhead["patient_orientation"] = py::cast(patient_orientation);
          m_mhead["facility_name"] = py::cast(facility_name);
          m_mhead["num_planes"] = py::cast(num_planes);
          m_mhead["num_frames"] = py::cast(num_frames);
          m_mhead["num_gates"] = py::cast(num_gates);
          m_mhead["num_bed_pos"] = py::cast(num_bed_pos);
          m_mhead["init_bed_position"] = py::cast(init_bed_position);
          m_mhead["bed_offset"] = py::cast(bed_offset);
          m_mhead["plane_separation"] = py::cast(plane_separation);
          m_mhead["lwr_sctr_thres"] = py::cast(lwr_sctr_thres);
          m_mhead["lwr_true_thres"] = py::cast(lwr_true_thres);
          m_mhead["upr_true_thres"] = py::cast(upr_true_thres);
          m_mhead["user_process_code"] = py::cast(user_process_code);
          m_mhead["acquisition_mode"] = py::cast(acquisition_mode);
          m_mhead["bin_size"] = py::cast(bin_size);
          m_mhead["branching_fraction"] = py::cast(branching_fraction);
          m_mhead["dose_start_time"] = py::cast(dose_start_time);
          m_mhead["dosage"] = py::cast(dosage);
          m_mhead["well_counter_corr_factor"] = py::cast(well_counter_corr_factor);
          m_mhead["data_units"] = py::cast(data_units);
          m_mhead["septa_state"] = py::cast(septa_state);
          m_mhead["cti_reserved"] = py::cast(cti_reserved);

          result = true;
        }
      }

      return result;
    }

    bool parseSubHeader(const CMedIOHeader* subHeader)
    {
      bool result = false;

      if(subHeader->headerFormat() == CMedIOHeader::ECATSubHeader)
      {
        const CECATSubHeader* ecatSubHeader = static_cast<const CECATSubHeader*>(subHeader);
        if(ecatSubHeader->subHeaderType() == CECATSubHeader::ECAT7_Image)
        {
          const CECAT7SubHeaderImage* e7SubHeader = static_cast<const CECAT7SubHeaderImage*>(ecatSubHeader);

          int data_type = static_cast<int>(e7SubHeader->data_Type());
          short num_dimensions = e7SubHeader->num_Dimensions();
          short x_dimension = e7SubHeader->x_Dimension();
          short y_dimension = e7SubHeader->y_Dimension();
          short z_dimension = e7SubHeader->z_Dimension();
          double x_offset = ROUND(e7SubHeader->x_Offset());
          double y_offset = ROUND(e7SubHeader->y_Offset());
          double z_offset = ROUND(e7SubHeader->z_Offset());
          double recon_zoom = ROUND(e7SubHeader->recon_Zoom());
          double scale_factor = ROUND(e7SubHeader->scale_Factor());
          short image_min = e7SubHeader->image_Min();
          short image_max = e7SubHeader->image_Max();
          double x_pixelsize = ROUND(e7SubHeader->x_Pixel_Size());
          double y_pixelsize = ROUND(e7SubHeader->y_Pixel_Size());
          double z_pixelsize = ROUND(e7SubHeader->z_Pixel_Size());
          unsigned int frame_duration = e7SubHeader->frame_Duration();
          unsigned int frame_start = e7SubHeader->frame_Start_Time();

          int filter_code = e7SubHeader->filter_Code();
          double x_resolution = ROUND(e7SubHeader->x_Resolution());
          double y_resolution = ROUND(e7SubHeader->y_Resolution());
          double z_resolution = ROUND(e7SubHeader->z_Resolution());
          double num_r_elements = ROUND(e7SubHeader->num_R_Elements());
          double num_angles = ROUND(e7SubHeader->num_Angles());
          double z_rotation_angle = ROUND(e7SubHeader->z_Rotation_Angle());
          double decay_corr_fctr = ROUND(e7SubHeader->decay_Corr_Fctr());
          unsigned int processing_code = e7SubHeader->processing_Code();
          unsigned int gate_duration = e7SubHeader->gate_Duration();
          unsigned int r_wave_offset = e7SubHeader->r_Wave_Offset();
          unsigned int num_accepted_beats = e7SubHeader->num_Accepted_Beats();
          double filter_cutoff_frequency = ROUND(e7SubHeader->filter_Cutoff_Frequency());
          double filter_resolution = ROUND(e7SubHeader->filter_Resolution());
          double filter_ramp_slope = ROUND(e7SubHeader->filter_Ramp_Slope());
          short filter_order = e7SubHeader->filter_Order();
          double filter_scatter_fraction = ROUND(e7SubHeader->filter_Scatter_Fraction());
          double filter_scatter_slope = ROUND(e7SubHeader->filter_Scatter_Slope());
          std::string annotation = e7SubHeader->annotation();
          double mt_1_1 = ROUND(e7SubHeader->mt_1_1());
          double mt_1_2 = ROUND(e7SubHeader->mt_1_2());
          double mt_1_3 = ROUND(e7SubHeader->mt_1_3());
          double mt_2_1 = ROUND(e7SubHeader->mt_2_1());
          double mt_2_2 = ROUND(e7SubHeader->mt_2_2());
          double mt_2_3 = ROUND(e7SubHeader->mt_2_3());
          double mt_3_1 = ROUND(e7SubHeader->mt_3_1());
          double mt_3_2 = ROUND(e7SubHeader->mt_3_2());
          double mt_3_3 = ROUND(e7SubHeader->mt_3_3());
          double rfilter_cutoff = ROUND(e7SubHeader->rfilter_Cutoff());

          double rfilter_resolution = ROUND(e7SubHeader->rfilter_Resolution());
          int rfilter_code = e7SubHeader->rfilter_Code();
          short rfilter_order = e7SubHeader->rfilter_Order();
          double zfilter_cutoff = ROUND(e7SubHeader->zfilter_Cutoff());
          double zfilter_resolution = ROUND(e7SubHeader->zfilter_Resolution());
          int zfilter_code = e7SubHeader->zfilter_Code();
          short zfilter_order = e7SubHeader->zfilter_Order();
          double mt_1_4 = ROUND(e7SubHeader->mt_1_4());
          double mt_2_4 = ROUND(e7SubHeader->mt_2_4());
          double mt_3_4 = ROUND(e7SubHeader->mt_3_4());
          int scatter_type = e7SubHeader->scatter_Type();
          int recon_type = e7SubHeader->recon_Type();
          short recon_views = e7SubHeader->recon_Views();

          std::vector<short> cti_reserved(87);
          for(unsigned short i=0; i < cti_reserved.size(); ++i)
            cti_reserved[i] = e7SubHeader->cti_Reserved(i);

          std::vector<short> user_reserved(48);
          for(unsigned short i=0; i < user_reserved.size(); ++i)
            user_reserved[i] = e7SubHeader->user_Reserved(i);

          // stuff everything into a py::dict
          py::dict shead = py::dict{};
          shead["data_type"] = py::cast(data_type);
          shead["num_dimensions"] = py::cast(num_dimensions);
          shead["x_dimension"] = py::cast(x_dimension);
          shead["y_dimension"] = py::cast(y_dimension);
          shead["z_dimension"] = py::cast(z_dimension);
          shead["x_offset"] = py::cast(x_offset);
          shead["y_offset"] = py::cast(y_offset);
          shead["z_offset"] = py::cast(z_offset);
          shead["recon_zoom"] = py::cast(recon_zoom);
          shead["scale_factor"] = py::cast(scale_factor);
          shead["image_min"] = py::cast(image_min);
          shead["image_max"] = py::cast(image_max);
          shead["x_pixelsize"] = py::cast(x_pixelsize);
          shead["y_pixelsize"] = py::cast(y_pixelsize);
          shead["z_pixelsize"] = py::cast(z_pixelsize);
          shead["frame_duration"] = py::cast(frame_duration);
          shead["frame_start"] = py::cast(frame_start);

          shead["filter_code"] = py::cast(filter_code);
          shead["x_resolution"] = py::cast(x_resolution);
          shead["y_resolution"] = py::cast(y_resolution);
          shead["z_resolution"] = py::cast(z_resolution);
          shead["num_r_elements"] = py::cast(num_r_elements);
          shead["num_angles"] = py::cast(num_angles);
          shead["z_rotation_angle"] = py::cast(z_rotation_angle);
          shead["decay_corr_fctr"] = py::cast(decay_corr_fctr);
          shead["processing_code"] = py::cast(processing_code);
          shead["gate_duration"] = py::cast(gate_duration);
          shead["r_wave_offset"] = py::cast(r_wave_offset);
          shead["num_accepted_beats"] = py::cast(num_accepted_beats);
          shead["filter_cutoff_frequency"] = py::cast(filter_cutoff_frequency);
          shead["filter_resolution"] = py::cast(filter_resolution);
          shead["filter_ramp_slope"] = py::cast(filter_ramp_slope);
          shead["filter_order"] = py::cast(filter_order);
          shead["filter_scatter_fraction"] = py::cast(filter_scatter_fraction);
          shead["filter_scatter_slope"] = py::cast(filter_scatter_slope);
          shead["annotation"] = py::cast(annotation);
          shead["mt_1_1"] = py::cast(mt_1_1);
          shead["mt_1_2"] = py::cast(mt_1_2);
          shead["mt_1_3"] = py::cast(mt_1_3);
          shead["mt_2_1"] = py::cast(mt_2_1);
          shead["mt_2_2"] = py::cast(mt_2_2);
          shead["mt_2_3"] = py::cast(mt_2_3);
          shead["mt_3_1"] = py::cast(mt_3_1);
          shead["mt_3_2"] = py::cast(mt_3_2);
          shead["mt_3_3"] = py::cast(mt_3_3);
          shead["rfilter_cutoff"] = py::cast(rfilter_cutoff);
          shead["rfilter_resolution"] = py::cast(rfilter_resolution);
          shead["rfilter_code"] = py::cast(rfilter_code);
          shead["rfilter_order"] = py::cast(rfilter_order);
          shead["zfilter_cutoff"] = py::cast(zfilter_cutoff);
          shead["zfilter_resolution"] = py::cast(zfilter_resolution);
          shead["zfilter_code"] = py::cast(zfilter_code);
          shead["zfilter_order"] = py::cast(zfilter_order);
          shead["mt_1_4"] = py::cast(mt_1_4);
          shead["mt_2_4"] = py::cast(mt_2_4);
          shead["mt_3_4"] = py::cast(mt_3_4);
          shead["scatter_type"] = py::cast(scatter_type);
          shead["recon_type"] = py::cast(recon_type);
          shead["recon_views"] = py::cast(recon_views);

          shead["cti_reserved"] = py::cast(cti_reserved);
          shead["user_reserved"] = py::cast(user_reserved);

          // append this shead dict to the m_shead list
          m_shead.append(shead);

          result = true;
        }
      }

      return result;
    }

    MedIOImage& readImage(std::vector<int> volumes={},
                          std::vector<int> rows={},
                          std::vector<int> cols={},
                          std::vector<int> planes={})
    {
      if(m_mioData != nullptr)
      {
        if(m_mioData->open(QIODevice::ReadOnly))
        {
          if(m_mioData->dataFormat() == CMedIOData::ECAT)
          {
            CECATFile* eFile = static_cast<CECATFile*>(m_mioData);
            CECATMainHeader* ecatMainHeader = nullptr;
            if(eFile->readMainHeader(ecatMainHeader) == true)
            {
              // parse/fill mainHeader
              parseMainHeader(ecatMainHeader);

              if(ecatMainHeader->mainHeaderType() == CECATMainHeader::ECAT7MainHeader)
              {
                CECAT7MainHeader* e7MainHeader = static_cast<CECAT7MainHeader*>(ecatMainHeader);

                int num_frames = e7MainHeader->num_Frames();
                int num_gates = e7MainHeader->num_Gates();
                int num_bed_pos = e7MainHeader->num_Bed_Pos();
                double ecat_calibration_factor = e7MainHeader->calibration_Factor();

                VolumeSelection which_to_use = FRAMES;
                short frame = 1;
                short gate = 1;
                short bed = 0;
                int num_volumes = 0;

                // now we have to check wether to use frames, gates or beds as volumes
                if( (num_frames > 1) && (num_gates <= 1) && ((num_bed_pos+1) <= 1) )
                {
                  // use frames
                  which_to_use = FRAMES;
                  num_volumes = num_frames;
                }
                else if( (num_frames <= 1) && (num_gates > 1) && ((num_bed_pos+1) <= 1) )
                {
                  // use gates
                  which_to_use = GATES;
                  num_volumes = num_gates;
                }
                else if( (num_frames <= 1) && (num_gates <= 1) && ((num_bed_pos+1) > 1) )
                {
                  // use beds
                  which_to_use = BEDS;
                  num_volumes = num_bed_pos + 1;
                }
                else if( (num_frames <= 1) && (num_gates <=1) && ((num_bed_pos+1) <= 1) )
                {
                  // if there is none of them bigger then 1,
                  // we use frames
                  which_to_use = FRAMES;
                  num_volumes = num_frames;
                }
                else
                {
                  // ... at least we would have no idea how to assign the
                  // indices to the volumes...
                  throw std::runtime_error("can handle only a single dynamic index (one of frame/gate/bed)");
                }

                // check if volumes index array is given
                // we take all frames if nothing is specified
                if(volumes.size() == 0)
                {
                  for(int i = 1; i <= num_volumes; ++i)
                    volumes.push_back(i);
                }
                else
                {
                  // we have to check if the index std::vector exceeds the boundaries
                  std::vector<int>::iterator it;
                  for(it = volumes.begin(); it < volumes.end(); ++it)
                  {
                    if( (*it < 1) || (*it > num_volumes))
                      throw std::runtime_error("index in volumes (" + std::to_string(*it) + ") is exceeding boundaries: there are " + std::to_string(num_volumes) + " volumes in this file");
                  }
                }

                // safe 4th dimension into m_tdim
                m_tdim = volumes.size();

                // if we should process more than one volume we
                // put that in the 4th dimension
                //if(m_tdim > 1)
                  m_numdim = 4; // 4D data (frames/gates/bed)
                //else
                //  m_numdim = 3; // 3D data

                // clear m_shead list because we are now going to read in all subheaders
                m_shead = py::list{};

                int t=0;
                std::vector<int>::iterator it;
                for(it = volumes.begin(); it != volumes.end(); ++it, ++t)
                {
                  switch(which_to_use)
                  {
                    case FRAMES: frame = *it; break;
                    case GATES: gate = *it; break;
                    case BEDS: bed = *it - 1; break; // numbering of beds starts at 0
                  }

                  CECATSubHeader* ecatSubHeader = nullptr;
                  if(eFile->readSubHeader(ecatSubHeader, frame, 1, gate, bed, 0) == true)
                  {
                    parseSubHeader(ecatSubHeader);

                    if(ecatSubHeader->subHeaderType() == CECATSubHeader::ECAT7_Image)
                    {
                      CECAT7SubHeaderImage* e7SubHeaderImage = static_cast<CECAT7SubHeaderImage*>(ecatSubHeader);
                      int data_type = e7SubHeaderImage->data_Type();
                      double scale_factor = e7SubHeaderImage->scale_Factor();

                      // set global m_xdim once or check
                      if(m_xdim != 0)
                      {
                        if(m_xdim != static_cast<size_t>(e7SubHeaderImage->x_Dimension()))
                          throw std::runtime_error("unequal X dimensions in dataset (" + std::to_string(m_xdim) + " != " + std::to_string(e7SubHeaderImage->x_Dimension()) + ")");
                      }
                      else
                        m_xdim = e7SubHeaderImage->x_Dimension();

                      // set global m_ydim once or check
                      if(m_ydim != 0)
                      {
                        if(m_ydim != static_cast<size_t>(e7SubHeaderImage->y_Dimension()))
                          throw std::runtime_error("unequal Y dimensions in dataset (" + std::to_string(m_ydim) + " != " + std::to_string(e7SubHeaderImage->y_Dimension()) + ")");
                      }
                      else
                        m_ydim = e7SubHeaderImage->y_Dimension();

                      // set global m_zdim once or check
                      if(m_zdim != 0)
                      {
                        if(m_zdim != static_cast<size_t>(e7SubHeaderImage->z_Dimension()))
                          throw std::runtime_error("unequal Z dimensions in dataset (" + std::to_string(m_zdim) + " != " + std::to_string(e7SubHeaderImage->z_Dimension()) + ")");
                      }
                      else
                        m_zdim = e7SubHeaderImage->z_Dimension();

                      // check if rows, cols and/or planes index arrays are given
                      // if not take all of them
                      bool exceeding_boundaries = false;
                      if(rows.size() == 0)
                      {
                        for(size_t i = 1; i <= m_ydim; i++)
                          rows.push_back(i);
                      }
                      else
                      {
                        // we have to check if the index std::vector exceeds the boundaries
                        std::vector<int>::iterator it;
                        for(it = rows.begin(); it < rows.end(); ++it)
                        {
                          if( (*it < 1) || (*it > static_cast<int>(m_ydim)) )
                          {
                            throw std::runtime_error("index in rows vector (" + std::to_string(*it) + ") is out of bounds: there are " + std::to_string(m_ydim) + " rows in this file");
                            exceeding_boundaries = true;
                          }
                        }
                      }

                      if(cols.size() == 0)
                      {
                        for(size_t i = 1; i <= m_xdim; i++)
                          cols.push_back(i);
                      }
                      else
                      {
                        // we have to check if the index std::vector exceeds the boundaries
                        std::vector<int>::iterator it;
                        for(it = cols.begin(); it < cols.end(); ++it)
                        {
                          if( (*it < 1) || (*it > static_cast<int>(m_xdim)) )
                          {
                            throw std::runtime_error("index in cols vector (" + std::to_string(*it) + ") is out of bounds: there are " + std::to_string(m_xdim) + " cols in this file");
                            exceeding_boundaries = true;
                          }
                        }
                      }

                      if(planes.size() == 0)
                      {
                        for(size_t i = 1; i <= m_zdim; i++)
                          planes.push_back(i);
                      }
                      else
                      {
                        // we have to check if the index std::vector exceeds the boundaries
                        std::vector<int>::iterator it;
                        for(it = planes.begin(); it < planes.end(); ++it)
                        {
                          if( (*it < 1) || (*it > static_cast<int>(m_zdim)))
                          {
                            throw std::runtime_error("index in planes vector (" + std::to_string(*it) + ") is out of bounds: there are " + std::to_string(m_zdim) + " planes in this file");
                            exceeding_boundaries = true;
                          }
                        }
                      }
        
                      if(exceeding_boundaries == true)
                        break;

                      // create data array with (volumes * xdim * ydim * zdim) space
                      delete[] m_data;
                      m_data = new float[m_xdim * m_ydim * m_zdim * volumes.size()];

                      char* byteData = NULL;
                      unsigned int byteDataSize = 0;

                      if(eFile->readMatrix(byteData, byteDataSize, frame, 1, gate, bed, 0) == true)
                      {
                        size_t plane_size = m_xdim * m_ydim;
                        size_t volume_size = m_xdim * m_ydim * m_zdim;

                        int data_size = 0; // 0 == error
                        switch(data_type)
                        {
                          case CECATSubHeader::ByteData:
                            data_size = sizeof(qint8);
                          break;

                          case CECATSubHeader::SunShort:
                            data_size = sizeof(qint16);
                          break;

                          case CECATSubHeader::IEEEFloat:
                            data_size = sizeof(float);
                          break;

                          case CECATSubHeader::SunLong:
                            data_size = sizeof(qint32);
                          break;
                        }

                        if(data_size > 0)
                        {
                          void* mData = byteData;

                          for(size_t z=0; z < m_zdim; ++z)
                          {
                            for(size_t y=0; y < m_ydim; ++y)
                            {
                              for(size_t x=0; x < m_xdim; ++x)
                              {
                                size_t srcIndex = x + y * m_ydim + z * plane_size;
                                size_t dstIndex = (m_xdim-1 - x) + (m_ydim-1 - y) * m_ydim + (m_zdim-1 - z) * plane_size + (m_tdim-1 - t) * volume_size;

                                switch(data_type)
                                {
                                  case CECATSubHeader::ByteData:
                                  {
                                    qint8* pint8 = static_cast<qint8*>(mData);
                                    m_data[dstIndex] = static_cast<float>(pint8[srcIndex]) * scale_factor * ecat_calibration_factor;
                                  }
                                  break;

                                  case CECATSubHeader::SunShort:
                                  {
                                    qint16* pint16 = static_cast<qint16*>(mData);
                                    m_data[dstIndex] = static_cast<float>(pint16[srcIndex]) * scale_factor * ecat_calibration_factor;
                                  }
                                  break;

                                  case CECATSubHeader::IEEEFloat:
                                  {
                                    float* pfloat = static_cast<float*>(mData);
                                    m_data[dstIndex] = static_cast<float>(pfloat[srcIndex]) * scale_factor * ecat_calibration_factor;
                                  }
                                  break;

                                  case CECATSubHeader::SunLong:
                                  {
                                    qint32* pint32 = static_cast<qint32*>(mData);
                                    m_data[dstIndex] = static_cast<float>(pint32[srcIndex]) * scale_factor * ecat_calibration_factor;
                                  }
                                  break;
                                }
                              }
                            }
                          }
                        }

                        delete[] byteData;
                      }
                    }
                  }
                }
              }
            }
          }
          else
            throw std::runtime_error("unsupported data format: " + std::to_string(m_mioData->dataFormat()));

          m_mioData->close();
        }
        else
          throw std::runtime_error("could not open " + m_mioData->fileName().toStdString());
      }
      else
        throw std::runtime_error("no CMedIOData assigned");

      return *this;
    }

    bool writeImage(const QString& fileName,
                    const bool overwrite=false,
                    const QString& dtype="short",
                    const CMedIOData::Format& format=CMedIOData::Unknown)
    {
      bool result = false;
      if(m_mioData != nullptr)
      {
        // check that the CMedIOData object has the same
        // data format as the supplied argument
        if(format != CMedIOData::Unknown && m_mioData->dataFormat() != format)
          throw std::runtime_error("supplied medioData object is not of data format " + std::to_string(format));
      }

      return result;
    }
 
  public:
    size_t m_numdim;
    size_t m_xdim;
    size_t m_ydim;
    size_t m_zdim;
    size_t m_tdim;
    py::dict m_mhead;
    py::list m_shead;
 
  private:
    CMedIOData* m_mioData;
    float* m_data;
};

// pybind11 type caster from QString <> Python 'str'
namespace PYBIND11_NAMESPACE { namespace detail {
  template <> struct type_caster<QString> {
  public:
    /**
     * This macro establishes the name 'QString' in
     * function signatures and declares a local variable
     * 'value' of type QString
     */
    PYBIND11_TYPE_CASTER(QString, const_name("str"));

    /**
     * Conversion part 1 (Python->C++): convert a PyObject into a QString
     * instance or return false upon failure. The second argument
     * indicates whether implicit conversions should be applied.
     */
    bool load(handle src, bool) {
        /* Extract PyObject from handle */
        PyObject *source = src.ptr();

        /* check if unicode compliant */
        if (!PyUnicode_Check(source))
          return false;

        /* convert into a const char */
        Py_ssize_t size = 0;
        const char *ptr = PyUnicode_AsUTF8AndSize(source, &size);
        if (!ptr)
          return false;

        /* Now try to convert into a QString */
        value = QString::fromUtf8(ptr, size);

        /* Ensure return code was OK (to avoid out-of-range errors etc) */
        return (!PyErr_Occurred());
    }

    /**
     * Conversion part 2 (C++ -> Python): convert a QString instance into
     * a Python object. The second and third arguments are used to
     * indicate the return value policy and parent object (for
     * ``return_value_policy::reference_internal``) and are generally
     * ignored by implicit casters.
     */
    static handle cast(QString src, return_value_policy /* policy */, handle /* parent */) {
      return (PyUnicode_FromString( src.toUtf8().data()) );
    }
  };
}} // namespace PYBIND11_NAMESPACE::detail
 
// pybind11 type caster from QIODevice::OpenModeFlag <> Python 'int'
namespace PYBIND11_NAMESPACE { namespace detail {
  template <> struct type_caster<QIODevice::OpenModeFlag> {
  public:
    /**
     * This macro establishes the name 'QIODevice::OpenModeFlag' in
     * function signatures and declares a local variable
     * 'value' of type QIODevice::OpenModeFlag
     */
    PYBIND11_TYPE_CASTER(QIODevice::OpenModeFlag, const_name("int"));

    /**
     * Conversion part 1 (Python->C++): convert a PyObject into a QIODevice::OpenModeFlag
     * instance or return false upon failure. The second argument
     * indicates whether implicit conversions should be applied.
     */
    bool load(handle src, bool) {
        /* Extract PyObject from handle */
        PyObject *source = src.ptr();

        PyObject *tmp = PyNumber_Long(source);
        if (!tmp)
          return false;

        /* convert into a const char */
        value = static_cast<QIODevice::OpenModeFlag>(PyLong_AsLong(tmp));
        Py_DECREF(tmp);

        return !(value == -1 && !PyErr_Occurred());
    }

    /**
     * Conversion part 2 (C++ -> Python): convert a QIODevice::OpenModeFlag instance into
     * a Python object. The second and third arguments are used to
     * indicate the return value policy and parent object (for
     * ``return_value_policy::reference_internal``) and are generally
     * ignored by implicit casters.
     */
    static handle cast(QIODevice::OpenModeFlag src, return_value_policy /* policy */, handle /* parent */) {
      return PyLong_FromLong(src);
    }
  };
}} // namespace PYBIND11_NAMESPACE::detail
