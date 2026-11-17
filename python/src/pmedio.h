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

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/chrono.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/vector.h>

#include "casters.h"

#include <CMedIOData>
#include <CECATFile>
#include <CECAT7MainHeader>
#include <CECAT7SubHeaderImage>

#include <QIODevice>
#include <QString>
#include <QFileInfo>

#include <iostream>
#include <cfloat>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

namespace py = nanobind;

// float32 array in the voxel order of MedIOImage (x runs fastest)
typedef py::ndarray<const float, py::f_contig, py::device::cpu> FloatArray;

// defined for specifiying the type
// special helper MedIOImage class to embed all necessary
// information and always return the pet data as float data
class MedIOImage
{
  public:
    // takes the ownership of mioData
    MedIOImage(CMedIOData* mioData=nullptr)
      : m_numdim(0),
        m_xdim(0),
        m_ydim(0),
        m_zdim(0),
        m_tdim(0),
        m_tdimType(DM_UNKNOWN),
        m_mhead(py::dict{}),
        m_shead(py::list{}),
        m_filename(),
        m_format(CMedIOData::Unknown),
        m_mioData(mioData),
        m_data(nullptr)
    { }

    MedIOImage(const FloatArray& img,
               const CMedIOData::Format& format = CMedIOData::ECAT,
               const enum dimTypes type = DM_FRAMES)
     :
       // the order of evaluation of the arguments is unspecified: check for each of them
       MedIOImage(checkedArray(img).shape(0),
                  checkedArray(img).shape(1),
                  checkedArray(img).shape(2),
                  checkedArray(img).ndim() > 3 ? img.shape(3) : 1, format, type)
    {
      // check format and create a fresh medio object
      if(format != CMedIOData::ECAT)
        throw std::runtime_error("only ECAT supported as target data format");

      // copy data from buffer to fresh data object
      memcpy(m_data, img.data(), img.size() * sizeof(float));
    }

    MedIOImage(const size_t xdim,
               const size_t ydim=1,
               const size_t zdim=1,
               const size_t tdim=1,
               const CMedIOData::Format& format = CMedIOData::ECAT,
               const enum dimTypes type = DM_FRAMES)
      : m_numdim(4), // always 4d
        m_xdim(xdim),
        m_ydim(ydim),
        m_zdim(zdim),
        m_tdim(tdim),
        m_tdimType(type),
        m_mhead(py::dict{}),
        m_shead(py::list{}),
        m_filename(),
        m_format(format),
        m_mioData(nullptr),
        m_data(nullptr)
    {
      // check format and create a fresh medio object
      if(format != CMedIOData::ECAT)
        throw std::runtime_error("only ECAT supported as target data format");

      // create fresh mioData object
      m_mioData.reset(new CECATFile(CECATMainHeader::ECAT7_Volume16));
      if(m_mioData != nullptr)
      {
        // populate with fresh mainheader entries
        m_mhead = createEmptyMainHeader();

        // lets fill the main header with sensible data
        m_mhead["original_file_name"] = py::cast("");
        m_mhead["isotope_name"] = py::cast("F-18");
        m_mhead["isotope_halflife"] = py::cast(6586.2);
        m_mhead["radiopharmaceutical"] = py::cast("F-18 FDG");
        switch(type)
        {
          case DM_FRAMES:
            m_mhead["num_planes"] = py::cast(m_zdim);
            m_mhead["num_frames"] = py::cast(m_tdim);
            m_mhead["num_gates"] = py::cast(0);
            m_mhead["num_bed_pos"] = py::cast(0);
          break;

          case DM_GATES:
            m_mhead["num_planes"] = py::cast(m_zdim);
            m_mhead["num_frames"] = py::cast(0);
            m_mhead["num_gates"] = py::cast(m_tdim);
            m_mhead["num_bed_pos"] = py::cast(0);
          break;

          case DM_BEDS:
            m_mhead["num_planes"] = py::cast(m_zdim);
            m_mhead["num_frames"] = py::cast(0);
            m_mhead["num_gates"] = py::cast(0);
            // number of the last bed position, the numbering starts at 0
            m_mhead["num_bed_pos"] = py::cast(m_tdim > 0 ? m_tdim - 1 : 0);
          break;

          case DM_DATA:
            m_mhead["num_planes"] = py::cast(m_zdim);
            m_mhead["num_frames"] = py::cast(0);
            m_mhead["num_gates"] = py::cast(0);
            m_mhead["num_bed_pos"] = py::cast(0);
          break;

          case DM_UNKNOWN:
            /* nothing */
          break;
        }
        m_mhead["ecat_calibration_factor"] = py::cast(1.0f);
        m_mhead["patient_orientation"] = py::cast(static_cast<short>(CECAT7MainHeader::HF_Prone));

        for(size_t t=0; t < m_tdim; ++t)
        {
          py::dict shead = createEmptySubHeader();

          // lets fill the sub header with sensible data
          shead["data_type"] = py::cast(static_cast<short>(CECATSubHeader::IEEEFloat)); // use float per default
          shead["num_dimensions"] = py::cast(3);
          shead["recon_zoom"] = py::cast(1.0f);
          shead["x_dimension"] = py::cast(m_xdim);
          shead["y_dimension"] = py::cast(m_ydim);
          shead["z_dimension"] = py::cast(m_zdim);
          shead["num_r_elements"] = py::cast(288);
          shead["num_angles"] = py::cast(144);
          shead["frame_duration"] = py::cast(1);
    
          // assume voxelsize 1,1,1 if no voxsiz attribute has been given.
          shead["x_pixelsize"] = py::cast(1.0f);
          shead["y_pixelsize"] = py::cast(1.0f);
          shead["z_pixelsize"] = py::cast(1.0f);
    
          m_shead.append(shead);
        }
      }

      m_data = new float[m_xdim * m_ydim * m_zdim * m_tdim];
      memset(m_data, 0, sizeof(float) * (m_xdim * m_ydim * m_zdim * m_tdim));
    }
 
    MedIOImage(const MedIOImage& img)
      : m_numdim(img.m_numdim),
        m_xdim(img.m_xdim),
        m_ydim(img.m_ydim),
        m_zdim(img.m_zdim),
        m_tdim(img.m_tdim),
        m_tdimType(img.m_tdimType),
        m_mhead(img.m_mhead),
        m_shead(img.m_shead),
        m_filename(img.m_filename),
        m_format(img.m_format),
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
        m_tdimType(img.m_tdimType),
        m_mhead(img.m_mhead),
        m_shead(img.m_shead),
        m_filename(img.m_filename),
        m_format(img.m_format),
        m_mioData(img.m_mioData),
        m_data(img.m_data)
    {
      img.m_numdim = 0;
      img.m_xdim = 0;
      img.m_ydim = 0;
      img.m_zdim = 0;
      img.m_tdim = 0;
      img.m_tdimType = DM_UNKNOWN;
      img.m_mhead = py::dict{};
      img.m_shead = py::list{};
      img.m_filename = QString();
      img.m_format = CMedIOData::Unknown;
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
      m_tdimType = img.m_tdimType;
      m_mhead = img.m_mhead;
      m_shead = img.m_shead;
      m_filename = img.m_filename;
      m_format = img.m_format;
      m_mioData = img.m_mioData;
      m_data = new float[m_xdim * m_ydim * m_zdim * m_tdim];
      memcpy(m_data, img.m_data, sizeof(float) * (m_xdim * m_ydim * m_zdim * m_tdim));

      return *this;
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
        m_tdimType = img.m_tdimType;
        m_mhead = img.m_mhead;
        m_shead = img.m_shead;
        m_filename = img.m_filename;
        m_format = img.m_format;
        m_mioData = img.m_mioData;
        m_data = img.m_data;
        img.m_numdim = 0;
        img.m_xdim = 0;
        img.m_ydim = 0;
        img.m_zdim = 0;
        img.m_tdim = 0;
        img.m_tdimType = DM_UNKNOWN;
        img.m_mhead = py::dict{};
        img.m_shead = py::list{};
        img.m_filename = QString();
        img.m_format = CMedIOData::Unknown;
        img.m_mioData = nullptr;
        img.m_data = nullptr;
      }
      return *this;
    }

    float operator()(const size_t x, const size_t y=0, const size_t z=0, const size_t t=0) const
    {
      return m_data[x + y * m_xdim + z * m_xdim*m_ydim + t * m_xdim*m_ydim*m_zdim];
    } 

    float& operator()(const size_t x, const size_t y=0, const size_t z=0, const size_t t=0)
    {
      return m_data[x + y * m_xdim + z * m_xdim*m_ydim + t * m_xdim*m_ydim*m_zdim];
    } 

    // returns the array if it has 3 or 4 dimensions and throws otherwise
    static const FloatArray& checkedArray(const FloatArray& img)
    {
      if(img.ndim() < 3 || img.ndim() > 4)
        throw std::runtime_error("only 3 and 4 dimensional data supported yet");

      return img;
    }

    // throws if the voxels cannot be stored with the data type: the integer data types have
    // no NaN and infinity, and ECAT byte data is unsigned
    void checkStorable(const enum dataTypes dtype) const
    {
      if(dtype == DT_FLOAT || m_data == nullptr)
        return;

      const size_t size = m_xdim * m_ydim * m_zdim * m_tdim;
      for(size_t i = 0; i < size; ++i)
      {
        if(std::isfinite(m_data[i]) == false)
          throw std::runtime_error("NaN and infinite values cannot be stored with an integer data type, use the data type float");
        if(dtype == DT_BYTE && m_data[i] < 0.0f)
          throw std::runtime_error("negative values cannot be stored as byte data (0 ... 255), use the data type short, int or float");
      }
    }

    float* data() { return m_data; }
    CMedIOData* medIOData() { return m_mioData.get(); }
    size_t numdim() const { return m_numdim; }
    size_t ndim() const { return m_numdim; }
    size_t xdim() const { return m_xdim; }
    size_t ydim() const { return m_ydim; }
    size_t zdim() const { return m_zdim; }
    size_t tdim() const { return m_tdim; }

    py::dict parseMainHeader(const CMedIOHeader* mainHeader)
    {
      py::dict mhead;

      if(mainHeader->headerFormat() == CMedIOHeader::ECATMainHeader)
      {
        const CECATMainHeader* ecatMainHeader = static_cast<const CECATMainHeader*>(mainHeader);
        if(ecatMainHeader->mainHeaderType() == CECATMainHeader::ECAT7MainHeader)
        {
          const CECAT7MainHeader* e7MainHeader = static_cast<const CECAT7MainHeader*>(ecatMainHeader);

          QString magic_number = QLatin1String(e7MainHeader->magic_Number());
          QString original_file_name = QLatin1String(e7MainHeader->original_File_Name());
          short sw_version = e7MainHeader->sw_Version();
          short system_type = e7MainHeader->system_Type();
          int file_type = e7MainHeader->file_Type();
          QString serial_number = QLatin1String(e7MainHeader->serial_Number());
          std::chrono::system_clock::time_point scan_start_time = std::chrono::system_clock::from_time_t(e7MainHeader->scan_Start_Time());
          QString isotope_name = QLatin1String(e7MainHeader->isotope_Name());
          float isotope_halflife = e7MainHeader->isotope_Halflife();
          QString radiopharmaceutical = QLatin1String(e7MainHeader->radiopharmaceutical());
          float gantry_tilt = e7MainHeader->gantry_Tilt();
          float gantry_rotation = e7MainHeader->gantry_Rotation();
          float bed_elevation = e7MainHeader->bed_Elevation();
          float intrinsic_tilt = e7MainHeader->intrinsic_Tilt();
          short wobble_speed = e7MainHeader->wobble_Speed();
          int transm_source_type = e7MainHeader->transm_Source_Type();
          float distance_scanned = e7MainHeader->distance_Scanned();
          float transaxial_fov = e7MainHeader->transaxial_FOV();
          int angular_compression = e7MainHeader->angular_Compression();
          int coin_samp_mode = e7MainHeader->coin_Samp_Mode();
          int axial_samp_mode = e7MainHeader->axial_Samp_Mode();
          float ecat_calibration_factor = e7MainHeader->calibration_Factor();
          int calibration_units = e7MainHeader->calibration_Units();
          int calibration_units_label = e7MainHeader->calibration_Units_Label();

          int compression_code = e7MainHeader->compression_Code();
          QString study_type = QLatin1String(e7MainHeader->study_Type());
          QString patient_id = QLatin1String(e7MainHeader->patient_ID());
          QString patient_name = QLatin1String(e7MainHeader->patient_Name());
          QString patient_sex = QString(QLatin1Char(static_cast<char>(e7MainHeader->patient_Sex())));
          QString patient_dexterity = QString(QLatin1Char(static_cast<char>(e7MainHeader->patient_Dexterity())));
    
          float patient_age = e7MainHeader->patient_Age();
          float patient_height = e7MainHeader->patient_Height();
          float patient_weight = e7MainHeader->patient_Weight();
          std::chrono::system_clock::time_point patient_birth_date = std::chrono::system_clock::from_time_t(e7MainHeader->patient_Birth_Date());
          QString physician_name = QLatin1String(e7MainHeader->physician_Name());
          QString operator_name = QLatin1String(e7MainHeader->operator_Name());
          QString study_description = QLatin1String(e7MainHeader->study_Description());
          int acquisition_type = e7MainHeader->acquisition_Type();
          int patient_orientation = e7MainHeader->patient_Orientation();
          QString facility_name = QLatin1String(e7MainHeader->facility_Name());
          short num_planes = e7MainHeader->num_Planes();
          int num_frames = e7MainHeader->num_Frames();
          int num_gates = e7MainHeader->num_Gates();
          int num_bed_pos = e7MainHeader->num_Bed_Pos();
          float init_bed_position = e7MainHeader->init_Bed_Position();

          std::vector<float> bed_offset(15);
          for(unsigned short i=0; i < bed_offset.size(); ++i)
            bed_offset[i] = e7MainHeader->bed_Offset(i);
    
          float plane_separation = e7MainHeader->plane_Separation();
          short lwr_sctr_thres = e7MainHeader->lwr_Sctr_Thres();
          short lwr_true_thres = e7MainHeader->lwr_True_Thres();
          short upr_true_thres = e7MainHeader->upr_True_Thres();
          QString user_process_code = QLatin1String(e7MainHeader->user_Process_Code());
          int acquisition_mode = e7MainHeader->acquisition_Mode();
          float bin_size = e7MainHeader->bin_Size();
          float branching_fraction = e7MainHeader->branching_Fraction();
          std::chrono::system_clock::time_point dose_start_time = std::chrono::system_clock::from_time_t(e7MainHeader->dose_Start_Time());
          float dosage = e7MainHeader->dosage();
          float well_counter_corr_factor = e7MainHeader->well_Counter_Corr_Factor();
          QString data_units = QLatin1String(e7MainHeader->data_Units());
          int septa_state = e7MainHeader->septa_State();
    
          std::vector<short> cti_reserved(6);
          for(unsigned short i=0; i < cti_reserved.size(); ++i)
            cti_reserved[i] = e7MainHeader->cti_Reserved(i);

          // stuff everything into the py::dict
          mhead["magic_number"] = py::cast(magic_number);
          mhead["original_file_name"] = py::cast(original_file_name);
          mhead["sw_version"] = py::cast(sw_version);
          mhead["system_type"] = py::cast(system_type);
          mhead["file_type"] = py::cast(file_type);
          mhead["serial_number"] = py::cast(serial_number);
          mhead["scan_start_time"] = py::cast(scan_start_time);
          mhead["isotope_name"] = py::cast(isotope_name);
          mhead["isotope_halflife"] = py::cast(isotope_halflife);
          mhead["radiopharmaceutical"] = py::cast(radiopharmaceutical);
          mhead["gantry_tilt"] = py::cast(gantry_tilt);
          mhead["gantry_rotation"] = py::cast(gantry_rotation);
          mhead["bed_elevation"] = py::cast(bed_elevation);
          mhead["intrinsic_tilt"] = py::cast(intrinsic_tilt);
          mhead["wobble_speed"] = py::cast(wobble_speed);
          mhead["transm_source_type"] = py::cast(transm_source_type);
          mhead["distance_scanned"] = py::cast(distance_scanned);
          mhead["transaxial_fov"] = py::cast(transaxial_fov);
          mhead["angular_compression"] = py::cast(angular_compression);
          mhead["coin_samp_mode"] = py::cast(coin_samp_mode);
          mhead["axial_samp_mode"] = py::cast(axial_samp_mode);
          mhead["ecat_calibration_factor"] = py::cast(ecat_calibration_factor);
          mhead["calibration_units"] = py::cast(calibration_units);
          mhead["calibration_units_label"] = py::cast(calibration_units_label);
          mhead["compression_code"] = py::cast(compression_code);
          mhead["study_type"] = py::cast(study_type);
          mhead["patient_id"] = py::cast(patient_id);
          mhead["patient_name"] = py::cast(patient_name);
          mhead["patient_sex"] = py::cast(patient_sex);
          mhead["patient_dexterity"] = py::cast(patient_dexterity);
          mhead["patient_age"] = py::cast(patient_age);
          mhead["patient_height"] = py::cast(patient_height);
          mhead["patient_weight"] = py::cast(patient_weight);
          mhead["patient_birth_date"] = py::cast(patient_birth_date);
          mhead["physician_name"] = py::cast(physician_name);
          mhead["operator_name"] = py::cast(operator_name);
          mhead["study_description"] = py::cast(study_description);
          mhead["acquisition_type"] = py::cast(acquisition_type);
          mhead["patient_orientation"] = py::cast(patient_orientation);
          mhead["facility_name"] = py::cast(facility_name);
          mhead["num_planes"] = py::cast(num_planes);
          mhead["num_frames"] = py::cast(num_frames);
          mhead["num_gates"] = py::cast(num_gates);
          mhead["num_bed_pos"] = py::cast(num_bed_pos);
          mhead["init_bed_position"] = py::cast(init_bed_position);
          mhead["bed_offset"] = py::cast(bed_offset);
          mhead["plane_separation"] = py::cast(plane_separation);
          mhead["lwr_sctr_thres"] = py::cast(lwr_sctr_thres);
          mhead["lwr_true_thres"] = py::cast(lwr_true_thres);
          mhead["upr_true_thres"] = py::cast(upr_true_thres);
          mhead["user_process_code"] = py::cast(user_process_code);
          mhead["acquisition_mode"] = py::cast(acquisition_mode);
          mhead["bin_size"] = py::cast(bin_size);
          mhead["branching_fraction"] = py::cast(branching_fraction);
          mhead["dose_start_time"] = py::cast(dose_start_time);
          mhead["dosage"] = py::cast(dosage);
          mhead["well_counter_corr_factor"] = py::cast(well_counter_corr_factor);
          mhead["data_units"] = py::cast(data_units);
          mhead["septa_state"] = py::cast(septa_state);
          mhead["cti_reserved"] = py::cast(cti_reserved);
        }
      }

      return mhead;
    }

    py::dict parseSubHeader(const CMedIOHeader* subHeader)
    {
      py::dict shead;

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
          float x_offset = e7SubHeader->x_Offset();
          float y_offset = e7SubHeader->y_Offset();
          float z_offset = e7SubHeader->z_Offset();
          float recon_zoom = e7SubHeader->recon_Zoom();
          float scale_factor = e7SubHeader->scale_Factor();
          short image_min = e7SubHeader->image_Min();
          short image_max = e7SubHeader->image_Max();
          float x_pixelsize = e7SubHeader->x_Pixel_Size();
          float y_pixelsize = e7SubHeader->y_Pixel_Size();
          float z_pixelsize = e7SubHeader->z_Pixel_Size();
          unsigned int frame_duration = e7SubHeader->frame_Duration();
          unsigned int frame_start = e7SubHeader->frame_Start_Time();

          int filter_code = e7SubHeader->filter_Code();
          float x_resolution = e7SubHeader->x_Resolution();
          float y_resolution = e7SubHeader->y_Resolution();
          float z_resolution = e7SubHeader->z_Resolution();
          float num_r_elements = e7SubHeader->num_R_Elements();
          float num_angles = e7SubHeader->num_Angles();
          float z_rotation_angle = e7SubHeader->z_Rotation_Angle();
          float decay_corr_fctr = e7SubHeader->decay_Corr_Fctr();
          unsigned int processing_code = e7SubHeader->processing_Code();
          unsigned int gate_duration = e7SubHeader->gate_Duration();
          unsigned int r_wave_offset = e7SubHeader->r_Wave_Offset();
          unsigned int num_accepted_beats = e7SubHeader->num_Accepted_Beats();
          float filter_cutoff_frequency = e7SubHeader->filter_Cutoff_Frequency();
          float filter_resolution = e7SubHeader->filter_Resolution();
          float filter_ramp_slope = e7SubHeader->filter_Ramp_Slope();
          short filter_order = e7SubHeader->filter_Order();
          float filter_scatter_fraction = e7SubHeader->filter_Scatter_Fraction();
          float filter_scatter_slope = e7SubHeader->filter_Scatter_Slope();
          QString annotation = QLatin1String(e7SubHeader->annotation());
          float mt_1_1 = e7SubHeader->mt_1_1();
          float mt_1_2 = e7SubHeader->mt_1_2();
          float mt_1_3 = e7SubHeader->mt_1_3();
          float mt_2_1 = e7SubHeader->mt_2_1();
          float mt_2_2 = e7SubHeader->mt_2_2();
          float mt_2_3 = e7SubHeader->mt_2_3();
          float mt_3_1 = e7SubHeader->mt_3_1();
          float mt_3_2 = e7SubHeader->mt_3_2();
          float mt_3_3 = e7SubHeader->mt_3_3();
          float rfilter_cutoff = e7SubHeader->rfilter_Cutoff();

          float rfilter_resolution = e7SubHeader->rfilter_Resolution();
          int rfilter_code = e7SubHeader->rfilter_Code();
          short rfilter_order = e7SubHeader->rfilter_Order();
          float zfilter_cutoff = e7SubHeader->zfilter_Cutoff();
          float zfilter_resolution = e7SubHeader->zfilter_Resolution();
          int zfilter_code = e7SubHeader->zfilter_Code();
          short zfilter_order = e7SubHeader->zfilter_Order();
          float mt_1_4 = e7SubHeader->mt_1_4();
          float mt_2_4 = e7SubHeader->mt_2_4();
          float mt_3_4 = e7SubHeader->mt_3_4();
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
        }
      }

      return shead;
    }

    py::dict createEmptyMainHeader()
    {
      py::dict mhead;

      if(m_mioData != nullptr)
      {
        switch(m_mioData->dataFormat())
        {
          case CMedIOData::ECAT:
          {
            CECATFile* ecatFile = static_cast<CECATFile*>(m_mioData.get());
            std::unique_ptr<CECATMainHeader> pMainHeader(ecatFile->createEmptyMainHeader());

            // parse main header
            mhead = parseMainHeader(pMainHeader.get());
          }
          break;

          default:
            throw std::runtime_error("only ECAT format supported");
          break;
        }
      }

      return mhead;
    }

    std::unique_ptr<CMedIOHeader> createMainHeader()
    {
      std::unique_ptr<CMedIOHeader> pMainHeader;

      if(m_mioData != nullptr)
      {
        switch(m_mioData->dataFormat())
        {
          case CMedIOData::ECAT:
          {
            CECATFile* ecatFile = static_cast<CECATFile*>(m_mioData.get());

            pMainHeader.reset(ecatFile->createEmptyMainHeader());
            CECAT7MainHeader* ecat7MainHeader = static_cast<CECAT7MainHeader*>(pMainHeader.get());

            QString magic_number = py::cast<QString>(m_mhead["magic_number"]);
            QString original_file_name = py::cast<QString>(m_mhead["original_file_name"]);
            short sw_version = py::cast<short>(m_mhead["sw_version"]);
            short system_type = py::cast<short>(m_mhead["system_type"]);
            int file_type = py::cast<int>(m_mhead["file_type"]);
            QString serial_number = py::cast<QString>(m_mhead["serial_number"]);
            time_t scan_start_time = std::chrono::system_clock::to_time_t(py::cast<std::chrono::system_clock::time_point>(m_mhead["scan_start_time"]));
            QString isotope_name = py::cast<QString>(m_mhead["isotope_name"]);
            float isotope_halflife = py::cast<float>(m_mhead["isotope_halflife"]);
            QString radiopharmaceutical = py::cast<QString>(m_mhead["radiopharmaceutical"]);
            float gantry_tilt = py::cast<float>(m_mhead["gantry_tilt"]);
            float gantry_rotation = py::cast<float>(m_mhead["gantry_rotation"]);
            float bed_elevation = py::cast<float>(m_mhead["bed_elevation"]);
            float intrinsic_tilt = py::cast<float>(m_mhead["intrinsic_tilt"]);
            short wobble_speed = py::cast<short>(m_mhead["wobble_speed"]);
            int transm_source_type = py::cast<int>(m_mhead["transm_source_type"]);
            float distance_scanned = py::cast<float>(m_mhead["distance_scanned"]);
            float transaxial_fov = py::cast<float>(m_mhead["transaxial_fov"]);
            int angular_compression = py::cast<int>(m_mhead["angular_compression"]);
            int coin_samp_mode = py::cast<int>(m_mhead["coin_samp_mode"]);
            int axial_samp_mode = py::cast<int>(m_mhead["axial_samp_mode"]);
            float ecat_calibration_factor = py::cast<float>(m_mhead["ecat_calibration_factor"]);
            int calibration_units = py::cast<int>(m_mhead["calibration_units"]);
            int calibration_units_label = py::cast<int>(m_mhead["calibration_units_label"]);
            int compression_code = py::cast<int>(m_mhead["compression_code"]);
            QString study_type = py::cast<QString>(m_mhead["study_type"]);
            QString patient_id = py::cast<QString>(m_mhead["patient_id"]);
            QString patient_name = py::cast<QString>(m_mhead["patient_name"]);

            QString sex = py::cast<QString>(m_mhead["patient_sex"]);
            CECAT7MainHeader::Patient_Sex patient_sex;
            switch(sex[0].toLatin1())
            {
              case 'M': patient_sex = CECAT7MainHeader::Sex_Male; break;
              case 'F': patient_sex = CECAT7MainHeader::Sex_Female; break;
              case 'U':
              default: patient_sex = CECAT7MainHeader::Sex_Unknown; break;
            }

            QString dexterity = py::cast<QString>(m_mhead["patient_dexterity"]);
            CECAT7MainHeader::Patient_Dexterity patient_dexterity;
            switch(dexterity[0].toLatin1())
            {
              case 'R': patient_dexterity = CECAT7MainHeader::Dext_RT; break;
              case 'L': patient_dexterity = CECAT7MainHeader::Dext_LF; break;
              case 'U':
              default: patient_dexterity = CECAT7MainHeader::Dext_Unknown; break;
            }

            float patient_age = py::cast<float>(m_mhead["patient_age"]);
            float patient_height = py::cast<float>(m_mhead["patient_height"]);
            float patient_weight = py::cast<float>(m_mhead["patient_weight"]);
            time_t patient_birth_datetime = std::chrono::system_clock::to_time_t(py::cast<std::chrono::system_clock::time_point>(m_mhead["patient_birth_date"]));
            QString physician_name = py::cast<QString>(m_mhead["physician_name"]);
            QString operator_name = py::cast<QString>(m_mhead["operator_name"]);
            QString study_description = py::cast<QString>(m_mhead["study_description"]);
            int acquisition_type = py::cast<int>(m_mhead["acquisition_type"]);
            int patient_orientation = py::cast<int>(m_mhead["patient_orientation"]);
            QString facility_name = py::cast<QString>(m_mhead["facility_name"]);
            short num_planes = py::cast<short>(m_mhead["num_planes"]);
            int num_frames = py::cast<int>(m_mhead["num_frames"]);
            int num_gates = py::cast<int>(m_mhead["num_gates"]);
            int num_bed_pos = py::cast<int>(m_mhead["num_bed_pos"]);
            float init_bed_position = py::cast<float>(m_mhead["init_bed_position"]);
            std::vector<float> bed_offset = py::cast<std::vector<float> >(m_mhead["bed_offset"]);
            float plane_separation = py::cast<float>(m_mhead["plane_separation"]);
            short lwr_sctr_thres = py::cast<short>(m_mhead["lwr_sctr_thres"]);
            short lwr_true_thres = py::cast<short>(m_mhead["lwr_true_thres"]);
            short upr_true_thres = py::cast<short>(m_mhead["upr_true_thres"]);
            QString user_process_code = py::cast<QString>(m_mhead["user_process_code"]);
            int acquisition_mode = py::cast<int>(m_mhead["acquisition_mode"]);
            float bin_size = py::cast<float>(m_mhead["bin_size"]);
            float branching_fraction = py::cast<float>(m_mhead["branching_fraction"]);
            time_t dose_start_time = std::chrono::system_clock::to_time_t(py::cast<std::chrono::system_clock::time_point>(m_mhead["dose_start_time"]));
            float dosage = py::cast<float>(m_mhead["dosage"]);
            float well_counter_corr_factor = py::cast<float>(m_mhead["well_counter_corr_factor"]);
            QString data_units = py::cast<QString>(m_mhead["data_units"]);
            int septa_state = py::cast<int>(m_mhead["septa_state"]);
            std::vector<short> cti_reserved = py::cast<std::vector<short> >(m_mhead["cti_reserved"]);

            ecat7MainHeader->setMagic_Number(magic_number.toLatin1().constData());
            ecat7MainHeader->setOriginal_File_Name(original_file_name.toLatin1().constData());
            ecat7MainHeader->setSW_Version(sw_version);
            ecat7MainHeader->setSystem_Type(system_type);
            ecat7MainHeader->setFile_Type(static_cast<CECAT7MainHeader::File_Type>(file_type));
            ecat7MainHeader->setSerial_Number(serial_number.toLatin1().constData());
            ecat7MainHeader->setScan_Start_Time(scan_start_time);
            ecat7MainHeader->setIsotope_Name(isotope_name.toLatin1().constData());
            ecat7MainHeader->setIsotope_Halflife(isotope_halflife);
            ecat7MainHeader->setRadiopharmaceutical(radiopharmaceutical.toLatin1().constData());
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
            ecat7MainHeader->setStudy_Type(study_type.toLatin1().constData());
            ecat7MainHeader->setPatient_ID(patient_id.toLatin1().constData());
            ecat7MainHeader->setPatient_Name(patient_name.toLatin1().constData());
            ecat7MainHeader->setPatient_Sex(patient_sex);
            ecat7MainHeader->setPatient_Dexterity(patient_dexterity);
            ecat7MainHeader->setPatient_Age(patient_age);
            ecat7MainHeader->setPatient_Height(patient_height);
            ecat7MainHeader->setPatient_Weight(patient_weight);
            ecat7MainHeader->setPatient_Birth_Date(patient_birth_datetime);
            ecat7MainHeader->setPhysician_Name(physician_name.toLatin1().constData());
            ecat7MainHeader->setOperator_Name(operator_name.toLatin1().constData());
            ecat7MainHeader->setStudy_Description(study_description.toLatin1().constData());
            ecat7MainHeader->setAcquisition_Type(static_cast<CECAT7MainHeader::Acquisition_Type>(acquisition_type));
            ecat7MainHeader->setPatient_Orientation(static_cast<CECAT7MainHeader::Patient_Orientation>(patient_orientation));
            ecat7MainHeader->setFacility_Name(facility_name.toLatin1().constData());
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
            ecat7MainHeader->setUser_Process_Code(user_process_code.toLatin1().constData());
            ecat7MainHeader->setAcquisition_Mode(static_cast<CECAT7MainHeader::Acquisition_Mode>(acquisition_mode));
            ecat7MainHeader->setBin_Size(bin_size);
            ecat7MainHeader->setBranching_Fraction(branching_fraction);
            ecat7MainHeader->setDose_Start_Time(dose_start_time);
            ecat7MainHeader->setDosage(dosage);
            ecat7MainHeader->setWell_Counter_Corr_Factor(well_counter_corr_factor);
            ecat7MainHeader->setData_Units(data_units.toLatin1().constData());
            ecat7MainHeader->setSepta_State(static_cast<CECAT7MainHeader::Septa_State>(septa_state));

            for(unsigned int i=0; i < cti_reserved.size(); ++i)
              ecat7MainHeader->setCTI_Reserved(i, cti_reserved[i]);
          }
          break;
          
          default:
            throw std::runtime_error("could not create main header for unsupported data format");
          break;
        }
      }

      return pMainHeader;
    }

    py::dict createEmptySubHeader()
    {
      py::dict shead;

      if(m_mioData != nullptr)
      {
        switch(m_mioData->dataFormat())
        {
          case CMedIOData::ECAT:
          {
            CECATFile* ecatFile = static_cast<CECATFile*>(m_mioData.get());
            std::unique_ptr<CECATSubHeader> pSubHeader(ecatFile->createEmptySubHeader());

            // parse sub header
            shead = parseSubHeader(pSubHeader.get());
          }
          break;

          default:
            throw std::runtime_error("only ECAT format supported");
          break;
        }
      }

      return shead;
    }

    std::unique_ptr<CMedIOHeader> createSubHeader(const py::dict& shead)
    {
      std::unique_ptr<CMedIOHeader> pSubHeader;

      if(m_mioData != nullptr)
      {
        switch(m_mioData->dataFormat())
        {
          case CMedIOData::ECAT:
          {
            CECATFile* ecatFile = static_cast<CECATFile*>(m_mioData.get());

            pSubHeader.reset(ecatFile->createEmptySubHeader());
            CECAT7SubHeaderImage* ecat7SubHeaderImage = static_cast<CECAT7SubHeaderImage*>(pSubHeader.get());

            int data_type = py::cast<int>(shead["data_type"]);
            short num_dimensions = py::cast<short>(shead["num_dimensions"]);
            short x_dimension = py::cast<short>(shead["x_dimension"]);
            short y_dimension = py::cast<short>(shead["y_dimension"]);
            short z_dimension = py::cast<short>(shead["z_dimension"]);
            float scale_factor = py::cast<float>(shead["scale_factor"]);
            float x_pixelsize = py::cast<float>(shead["x_pixelsize"]);
            float y_pixelsize = py::cast<float>(shead["y_pixelsize"]);
            float z_pixelsize = py::cast<float>(shead["z_pixelsize"]);
            
            float x_offset = py::cast<float>(shead["x_offset"]);
            float y_offset = py::cast<float>(shead["y_offset"]);
            float z_offset = py::cast<float>(shead["z_offset"]);
            float recon_zoom = py::cast<float>(shead["recon_zoom"]);

            short image_min = py::cast<short>(shead["image_min"]);
            short image_max = py::cast<short>(shead["image_max"]);

            unsigned int frame_duration = py::cast<unsigned int>(shead["frame_duration"]);
            unsigned int frame_start = py::cast<unsigned int>(shead["frame_start"]);

            int filter_code = py::cast<int>(shead["filter_code"]);
            float x_resolution = py::cast<float>(shead["x_resolution"]);
            float y_resolution = py::cast<float>(shead["y_resolution"]);
            float z_resolution = py::cast<float>(shead["z_resolution"]);
            float num_r_elements = py::cast<float>(shead["num_r_elements"]);
            float num_angles = py::cast<float>(shead["num_angles"]);
            float z_rotation_angle = py::cast<float>(shead["z_rotation_angle"]);
            float decay_corr_fctr = py::cast<float>(shead["decay_corr_fctr"]);

            unsigned int processing_code = py::cast<unsigned int>(shead["processing_code"]);
            unsigned int gate_duration = py::cast<unsigned int>(shead["gate_duration"]);
            unsigned int r_wave_offset = py::cast<unsigned int>(shead["r_wave_offset"]);
            unsigned int num_accepted_beats = py::cast<unsigned int>(shead["num_accepted_beats"]);

            float filter_cutoff_frequency = py::cast<float>(shead["filter_cutoff_frequency"]);
            float filter_resolution = py::cast<float>(shead["filter_resolution"]);
            float filter_ramp_slope = py::cast<float>(shead["filter_ramp_slope"]);
            short filter_order = py::cast<short>(shead["filter_order"]);
            float filter_scatter_fraction = py::cast<float>(shead["filter_scatter_fraction"]);
            float filter_scatter_slope = py::cast<float>(shead["filter_scatter_slope"]);
            QString annotation = py::cast<QString>(shead["annotation"]);
            float mt_1_1 = py::cast<float>(shead["mt_1_1"]);
            float mt_1_2 = py::cast<float>(shead["mt_1_2"]);
            float mt_1_3 = py::cast<float>(shead["mt_1_3"]);
            float mt_2_1 = py::cast<float>(shead["mt_2_1"]);
            float mt_2_2 = py::cast<float>(shead["mt_2_2"]);
            float mt_2_3 = py::cast<float>(shead["mt_2_3"]);
            float mt_3_1 = py::cast<float>(shead["mt_3_1"]);
            float mt_3_2 = py::cast<float>(shead["mt_3_2"]);
            float mt_3_3 = py::cast<float>(shead["mt_3_3"]);
            float rfilter_cutoff = py::cast<float>(shead["rfilter_cutoff"]);
            float rfilter_resolution = py::cast<float>(shead["rfilter_resolution"]);
            int rfilter_code = py::cast<int>(shead["rfilter_code"]);
            short rfilter_order = py::cast<short>(shead["rfilter_order"]);
            float zfilter_cutoff = py::cast<float>(shead["zfilter_cutoff"]);
            float zfilter_resolution = py::cast<float>(shead["zfilter_resolution"]);
            int zfilter_code = py::cast<int>(shead["zfilter_code"]);
            short zfilter_order = py::cast<short>(shead["zfilter_order"]);
            float mt_1_4 = py::cast<float>(shead["mt_1_4"]);
            float mt_2_4 = py::cast<float>(shead["mt_2_4"]);
            float mt_3_4 = py::cast<float>(shead["mt_3_4"]);
            int scatter_type = py::cast<int>(shead["scatter_type"]);
            int recon_type = py::cast<int>(shead["recon_type"]);
            short recon_views = py::cast<short>(shead["recon_views"]);
            std::vector<short> cti_reserved = py::cast<std::vector<short> >(shead["cti_reserved"]);
            std::vector<short> user_reserved = py::cast<std::vector<short> >(shead["user_reserved"]);

            ecat7SubHeaderImage->setData_Type(static_cast<CECATSubHeader::Data_Type>(data_type));
            ecat7SubHeaderImage->setNum_Dimensions(num_dimensions);
            ecat7SubHeaderImage->setX_Dimension(x_dimension);
            ecat7SubHeaderImage->setY_Dimension(y_dimension);
            ecat7SubHeaderImage->setZ_Dimension(z_dimension);
            ecat7SubHeaderImage->setScale_Factor(scale_factor);
            ecat7SubHeaderImage->setX_Pixel_Size(x_pixelsize);
            ecat7SubHeaderImage->setY_Pixel_Size(y_pixelsize);
            ecat7SubHeaderImage->setZ_Pixel_Size(z_pixelsize);
            ecat7SubHeaderImage->setX_Offset(x_offset);
            ecat7SubHeaderImage->setY_Offset(y_offset);
            ecat7SubHeaderImage->setZ_Offset(z_offset);
            ecat7SubHeaderImage->setRecon_Zoom(recon_zoom);
            ecat7SubHeaderImage->setScale_Factor(scale_factor);
            ecat7SubHeaderImage->setImage_Min(image_min);
            ecat7SubHeaderImage->setImage_Max(image_max);
            ecat7SubHeaderImage->setFrame_Duration(frame_duration);
            ecat7SubHeaderImage->setFrame_Start_Time(frame_start);
            ecat7SubHeaderImage->setFilter_Code(static_cast<CECAT7SubHeaderImage::Filter_Code>(filter_code));
            ecat7SubHeaderImage->setX_Resolution(x_resolution);
            ecat7SubHeaderImage->setY_Resolution(y_resolution);
            ecat7SubHeaderImage->setZ_Resolution(z_resolution);
            ecat7SubHeaderImage->setNum_R_Elements(num_r_elements);
            ecat7SubHeaderImage->setNum_Angles(num_angles);
            ecat7SubHeaderImage->setZ_Rotation_Angle(z_rotation_angle);
            ecat7SubHeaderImage->setDecay_Corr_Fctr(decay_corr_fctr);
            ecat7SubHeaderImage->setProcessing_Code(processing_code);
            ecat7SubHeaderImage->setGate_Duration(gate_duration);
            ecat7SubHeaderImage->setR_Wave_Offset(r_wave_offset);
            ecat7SubHeaderImage->setNum_Accepted_Beats(num_accepted_beats);
            ecat7SubHeaderImage->setFilter_Cutoff_Frequency(filter_cutoff_frequency);
            ecat7SubHeaderImage->setFilter_Resolution(filter_resolution);
            ecat7SubHeaderImage->setFilter_Ramp_Slope(filter_ramp_slope);
            ecat7SubHeaderImage->setFilter_Order(filter_order);
            ecat7SubHeaderImage->setFilter_Scatter_Fraction(filter_scatter_fraction);
            ecat7SubHeaderImage->setFilter_Scatter_Slope(filter_scatter_slope);
            ecat7SubHeaderImage->setAnnotation(annotation.toLatin1().constData());
            ecat7SubHeaderImage->setMT_1_1(mt_1_1);
            ecat7SubHeaderImage->setMT_1_2(mt_1_2);
            ecat7SubHeaderImage->setMT_1_3(mt_1_3);
            ecat7SubHeaderImage->setMT_2_1(mt_2_1);
            ecat7SubHeaderImage->setMT_2_2(mt_2_2);
            ecat7SubHeaderImage->setMT_2_3(mt_2_3);
            ecat7SubHeaderImage->setMT_3_1(mt_3_1);
            ecat7SubHeaderImage->setMT_3_2(mt_3_2);
            ecat7SubHeaderImage->setMT_3_3(mt_3_3);
            ecat7SubHeaderImage->setRFilter_Cutoff(rfilter_cutoff);
            ecat7SubHeaderImage->setRFilter_Resolution(rfilter_resolution);
            ecat7SubHeaderImage->setRFilter_Code(static_cast<CECAT7SubHeaderImage::Filter_Code>(rfilter_code));
            ecat7SubHeaderImage->setRFilter_Order(rfilter_order);
            ecat7SubHeaderImage->setZFilter_Cutoff(zfilter_cutoff);
            ecat7SubHeaderImage->setZFilter_Resolution(zfilter_resolution);
            ecat7SubHeaderImage->setZFilter_Code(static_cast<CECAT7SubHeaderImage::Filter_Code>(zfilter_code));
            ecat7SubHeaderImage->setZFilter_Order(zfilter_order);
            ecat7SubHeaderImage->setMT_1_4(mt_1_4);
            ecat7SubHeaderImage->setMT_2_4(mt_2_4);
            ecat7SubHeaderImage->setMT_3_4(mt_3_4);
            ecat7SubHeaderImage->setScatter_Type(static_cast<CECAT7SubHeaderImage::Scatter_Type>(scatter_type));
            ecat7SubHeaderImage->setRecon_Type(static_cast<CECAT7SubHeaderImage::Recon_Type>(recon_type));
            ecat7SubHeaderImage->setRecon_Views(recon_views);
        
            for(unsigned int i=0; i < cti_reserved.size(); ++i)
              ecat7SubHeaderImage->setCTI_Reserved(i, cti_reserved[i]);
        
            for(unsigned int i=0; i < user_reserved.size(); ++i)
              ecat7SubHeaderImage->setUser_Reserved(i, user_reserved[i]);
          }
          break;
          
          default:
            throw std::runtime_error("could not create sub header for unsupported data format");
          break;
        }
      }

      return pSubHeader;
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
          // the file is closed as well if reading fails
          struct Closer {
            CMedIOData* file;
            ~Closer() { file->close(); }
          } closer = { m_mioData.get() };

          if(m_mioData->dataFormat() == CMedIOData::ECAT)
          {
            CECATFile* eFile = static_cast<CECATFile*>(m_mioData.get());

            m_filename = QFileInfo(*eFile).absoluteFilePath();
            m_format = CMedIOData::ECAT;

            CECATMainHeader* ecatMainHeader = nullptr;
            if(eFile->readMainHeader(ecatMainHeader) == true)
            {
              // the header is a copy that belongs to us
              std::unique_ptr<CECATMainHeader> mainHeaderOwner(ecatMainHeader);

              // parse/fill mainHeader
              m_mhead = parseMainHeader(ecatMainHeader);

              if(ecatMainHeader->mainHeaderType() == CECATMainHeader::ECAT7MainHeader)
              {
                CECAT7MainHeader* e7MainHeader = static_cast<CECAT7MainHeader*>(ecatMainHeader);

                int num_frames = e7MainHeader->num_Frames();
                int num_gates = e7MainHeader->num_Gates();
                int num_bed_pos = e7MainHeader->num_Bed_Pos();
                // the main header has no number of data indices, it comes from
                // the matrix directory (the numbering starts at 0)
                int num_data = eFile->maxData() + 1;
                double ecatCalibrationFactor = e7MainHeader->calibration_Factor();

                enum dimTypes which_to_use = DM_FRAMES;
                short frame = 1;
                short gate = 1;
                short bed = 0;
                short data = 0;
                int num_volumes = 0;

                // now we have to check wether to use frames, gates or beds as volumes
                if( (num_frames <= 1) && (num_gates <= 1) && ((num_bed_pos+1) <= 1) && (num_data > 1) )
                {
                  // use the data index
                  which_to_use = DM_DATA;
                  num_volumes = num_data;
                }
                else if(num_data > 1)
                {
                  throw std::runtime_error("can handle only a single dynamic index (one of frame/gate/bed/data)");
                }
                else if( (num_frames > 1) && (num_gates <= 1) && ((num_bed_pos+1) <= 1) )
                {
                  // use frames
                  which_to_use = DM_FRAMES;
                  num_volumes = num_frames;
                }
                else if( (num_frames <= 1) && (num_gates > 1) && ((num_bed_pos+1) <= 1) )
                {
                  // use gates
                  which_to_use = DM_GATES;
                  num_volumes = num_gates;
                }
                else if( (num_frames <= 1) && (num_gates <= 1) && ((num_bed_pos+1) > 1) )
                {
                  // use beds
                  which_to_use = DM_BEDS;
                  num_volumes = num_bed_pos + 1;
                }
                else if( (num_frames <= 1) && (num_gates <=1) && ((num_bed_pos+1) <= 1) )
                {
                  // if there is none of them bigger then 1,
                  // we use frames
                  which_to_use = DM_FRAMES;
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
                m_tdimType = which_to_use;

                // if we should process more than one volume we
                // put that in the 4th dimension
                //if(m_tdim > 1)
                  m_numdim = 4; // 4D data (frames/gates/bed/data)
                //else
                //  m_numdim = 3; // 3D data

                // clear instance data we are going to fill
                m_shead = py::list{};
                delete[] m_data;
                m_data = nullptr;

                std::vector<int>::iterator it;
                size_t t=0;
                for(t=0, it = volumes.begin(); it != volumes.end(); ++it, ++t)
                {
                  switch(which_to_use)
                  {
                    case DM_FRAMES: frame = *it; break;
                    case DM_GATES: gate = *it; break;
                    case DM_BEDS: bed = *it - 1; break; // numbering of beds starts at 0
                    case DM_DATA: data = *it -1; break; // numbering of datas starts at 0
                    case DM_UNKNOWN: /* nothing */ break;
                  }

                  CECATSubHeader* ecatSubHeader = nullptr;
                  if(eFile->readSubHeader(ecatSubHeader, frame, 1, gate, bed, data) == true)
                  {
                    // the header is a copy that belongs to us
                    std::unique_ptr<CECATSubHeader> subHeaderOwner(ecatSubHeader);

                    // parse sub header and append to sub header
                    // list
                    m_shead.append(parseSubHeader(ecatSubHeader));

                    if(ecatSubHeader->subHeaderType() == CECATSubHeader::ECAT7_Image)
                    {
                      CECAT7SubHeaderImage* e7SubHeaderImage = static_cast<CECAT7SubHeaderImage*>(ecatSubHeader);
                      int dataType = e7SubHeaderImage->data_Type();
                      double scaleFactor = e7SubHeaderImage->scale_Factor();

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
                      if(m_data == nullptr)
                        m_data = new float[m_xdim * m_ydim * m_zdim * volumes.size()];

                      char* byteData = NULL;
                      unsigned int byteDataSize = 0;

                      if(eFile->readMatrix(byteData, byteDataSize, frame, 1, gate, bed, data) == true)
                      {
                        size_t planeSize = m_xdim * m_ydim;
                        size_t volumeSize = m_xdim * m_ydim * m_zdim;

                        if(volumeSize > 0)
                        {
                          void* mData = byteData;

                          for(size_t z=0; z < m_zdim; ++z)
                          {
                            for(size_t y=0; y < m_ydim; ++y)
                            {
                              for(size_t x=0; x < m_xdim; ++x)
                              {
                                size_t srcIndex = x + y * m_xdim + z * planeSize;
                                size_t dstIndex = (m_xdim-1 - x) + (m_ydim-1 - y) * m_xdim + (m_zdim-1 - z) * planeSize + t * volumeSize;

                                switch(dataType)
                                {
                                  case CECATSubHeader::ByteData:
                                  {
                                    // unsigned, like CMedIOVolume::UInt8 of libmedio
                                    quint8* puint8 = static_cast<quint8*>(mData);
                                    m_data[dstIndex] = static_cast<float>(puint8[srcIndex]) * scaleFactor * ecatCalibrationFactor;
                                  }
                                  break;

                                  case CECATSubHeader::SunShort:
                                  {
                                    qint16* pint16 = static_cast<qint16*>(mData);
                                    m_data[dstIndex] = static_cast<float>(pint16[srcIndex]) * scaleFactor * ecatCalibrationFactor;
                                  }
                                  break;

                                  case CECATSubHeader::IEEEFloat:
                                  {
                                    float* pfloat = static_cast<float*>(mData);
                                    m_data[dstIndex] = static_cast<float>(pfloat[srcIndex]) * scaleFactor * ecatCalibrationFactor;
                                  }
                                  break;

                                  case CECATSubHeader::SunLong:
                                  {
                                    qint32* pint32 = static_cast<qint32*>(mData);
                                    m_data[dstIndex] = static_cast<float>(pint32[srcIndex]) * scaleFactor * ecatCalibrationFactor;
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

    MedIOImage& writeImage(const QString& fileName,
                           const bool overwrite=false,
                           const enum dimTypes tdimType=DM_UNKNOWN,
                           const enum dataTypes dtype=DT_FLOAT,
                           const CMedIOData::Format& format=CMedIOData::Unknown)
    {
      // check if we have a valid MedIOData object and if not we
      // generate one on the fly
      if(m_mioData == nullptr)
      {
        if(format != CMedIOData::Unknown && format != CMedIOData::ECAT)
          throw std::runtime_error("only ECAT supported as target data format");
        if(fileName.isEmpty())
          throw std::runtime_error("no target filename specified");

        m_mioData.reset(new CECATFile(CECATMainHeader::ECAT7_Volume16));
      }

      // if we have a valid MedIOData object now go and use
      // it accordingly.
      if(m_mioData != nullptr)
      {
        // check that the CMedIOData object has the same
        // data format as the supplied argument
        if(format != CMedIOData::Unknown && m_mioData->dataFormat() != format)
          throw std::runtime_error("supplied medioData object is not of data format " + std::to_string(format));

        // check that a valid data type was supplied
        if(m_mioData->dataFormat() != CMedIOData::ECAT)
          throw std::runtime_error("only ECAT supported as target data format");

        CECATFile* ecatFile = static_cast<CECATFile*>(m_mioData.get());
        ecatFile->setFileType(CECATMainHeader::ECAT7_Volume16);

        // check if mioData is valid
        if(ecatFile->format() != CECATFile::ECAT7)
          throw std::runtime_error("only ECAT7 supported as target data format");
        if(ecatFile->fileType() != CECATMainHeader::ECAT7_Volume16)
          throw std::runtime_error("only ECAT7 Volume16 image data format supported");

        // values that cannot be stored are reported before a file is created or replaced
        checkStorable(dtype);

        // check if overwrite mode is enabled or not
        if(QFileInfo(fileName).exists())
        {
          if(overwrite == false)
            throw std::runtime_error("output file '" + fileName.toStdString() + "' already exists.");
          else
            QFile(fileName).remove();
        }

        // check if user wants a different tdim type
        if(tdimType == DM_UNKNOWN)
        {
          if(m_tdimType == DM_UNKNOWN)
            throw std::runtime_error("no tdim dimension type specified and original dataset does not specify the tdim type");
        }
        else
          m_tdimType = tdimType;

        // set new output filename
        ecatFile->setFileName(fileName);
        if(ecatFile->open(QIODevice::ReadWrite) == false || ecatFile->format() == CECATFile::Undefined)
        {
          // do not report a file as written that could not be created
          ecatFile->close();
          throw std::runtime_error("could not open output file '" + fileName.toStdString() + "' for writing");
        }

        // nothing is left behind if writing fails
        try
        {
          m_filename = QFileInfo(*ecatFile).absoluteFilePath();
          m_format = CMedIOData::ECAT;

          // create fresh main header and write it out
          std::unique_ptr<CMedIOHeader> mainHeader = createMainHeader();
          CECAT7MainHeader* e7MainHeader = static_cast<CECAT7MainHeader*>(mainHeader.get());
          if(ecatFile->writeMainHeader(*e7MainHeader) == false)
            throw std::runtime_error("could not write target main header.");

          // get ecat calibration factor
          const double ecatCalibrationFactor = e7MainHeader->calibration_Factor();

          const size_t planeSize = m_xdim * m_ydim;
          const size_t volumeSize = m_xdim * m_ydim * m_zdim;

          // now we write the matrix data to the file
          // therefore we need to check and use tdim as the volume/time dimension
          for(size_t t=0; t < m_tdim; ++t)
          {
            // get subheader
            py::dict shead = m_shead[t];

            // size, range and ECAT data type of the stored values
            size_t dataTypeSize = 0;
            double storedMin = 0.0;
            double storedMax = 0.0;
            switch(dtype)
            {
              case DT_BYTE: // unsigned, like CMedIOVolume::UInt8 of libmedio
                dataTypeSize = sizeof(quint8);
                storedMin = 0.0;
                storedMax = std::numeric_limits<quint8>::max();
                shead["data_type"] = static_cast<short>(CECATSubHeader::ByteData);
              break;

              case DT_SHORT:
                dataTypeSize = sizeof(qint16);
                storedMin = std::numeric_limits<qint16>::min();
                storedMax = std::numeric_limits<qint16>::max();
                shead["data_type"] = static_cast<short>(CECATSubHeader::SunShort);
              break;

              case DT_FLOAT:
                dataTypeSize = sizeof(float);
                shead["data_type"] = static_cast<short>(CECATSubHeader::IEEEFloat);
              break;

              case DT_LONG:
                dataTypeSize = sizeof(qint32);
                storedMin = std::numeric_limits<qint32>::min();
                storedMax = std::numeric_limits<qint32>::max();
                shead["data_type"] = static_cast<short>(CECATSubHeader::SunLong);
              break;
            }

            if(dataTypeSize == 0)
              throw std::runtime_error("invalid dataTypeSize");

            // 3D matrix of the volume
            std::vector<char> matrix(volumeSize * dataTypeSize);
            char* matrixData = matrix.data();
            const unsigned int matrixDataSize = static_cast<unsigned int>(matrix.size());

            // The integer data types store value / scale factor, rounded to an integer. The
            // scale factor makes the largest value use the full range of the data type, so
            // that the rounding error is small compared to the values. Volumes that only
            // contain integers within the range of the data type are stored as they are
            // (scale factor 1) and stay exact.
            double newMin = 0.0;
            double newMax = 0.0;
            double newScaleFactor = 1.0;
            if(dtype != DT_FLOAT && volumeSize > 0)
            {
              bool integers = true;
              newMin = std::numeric_limits<double>::max();
              newMax = -std::numeric_limits<double>::max();
              for(size_t i = 0; i < volumeSize; ++i)
              {
                const double voxel = static_cast<double>(m_data[i + t * volumeSize]) / ecatCalibrationFactor;
                newMin = qMin(newMin, voxel);
                newMax = qMax(newMax, voxel);
                if(voxel != std::rint(voxel))
                  integers = false;
              }

              if(integers == false || newMax > storedMax || newMin < storedMin)
              {
                // smallest factor that brings the largest and the smallest value into the
                // range of the data type
                double factor = 0.0;
                if(newMax > 0.0)
                  factor = newMax / storedMax;
                if(newMin < 0.0 && storedMin < 0.0)
                  factor = qMax(factor, newMin / storedMin);

                // the sub header stores the scale factor as float: use the stored value
                const float stored = static_cast<float>(factor);
                if(stored > 0.0f && std::isfinite(stored))
                  newScaleFactor = static_cast<double>(stored);
              }
            }

            // now we iterate through our volumeVector. But we do this
            // here in a fashion that is compatible to the way the ECAT format
            // stores the volume data.
            void* mData = matrixData;
            for(size_t z = 0; z < m_zdim; ++z) // plane
            {
              for(size_t y = 0; y < m_ydim; ++y) // row
              {
                for(size_t x = 0; x < m_xdim; ++x) // column
                {
                  // calculate the srcIndex in our m_data float data vector and the dstIndex
                  // to put the data in an order the ECAT format defines.
                  size_t srcIndex = (m_xdim-1 - x) + (m_ydim-1 - y) * m_xdim + (m_zdim-1 - z) * planeSize + t * volumeSize;
                  size_t dstIndex = x + y * m_xdim + z * planeSize;

                  // calculate the voxel value which we store in the ecat file
                  const double val = static_cast<double>(m_data[srcIndex]) / newScaleFactor / ecatCalibrationFactor;

                  // now depending on the data type of the src data we have to convert
                  // differently; the rounded value is kept within the range of the type
                  // (the scale factor stored as float can exceed it by a rounding error)
                  switch(dtype)
                  {
                    case DT_BYTE:
                    {
                      quint8* mData8 = static_cast<quint8*>(mData);
                      mData8[dstIndex] = static_cast<quint8>(qBound(storedMin, std::rint(val), storedMax));
                    }
                    break;

                    case DT_SHORT:
                    {
                      qint16* mData16 = static_cast<qint16*>(mData);
                      mData16[dstIndex] = static_cast<qint16>(qBound(storedMin, std::rint(val), storedMax));
                    }
                    break;

                    case DT_FLOAT:
                    {
                      float* mDataflt = static_cast<float*>(mData);
                      mDataflt[dstIndex] = static_cast<float>(val);
                    }
                    break;

                    case DT_LONG:
                    {
                      qint32* mData32 = static_cast<qint32*>(mData);
                      mData32[dstIndex] = static_cast<qint32>(qBound(storedMin, std::rint(val), storedMax));
                    }
                    break;
                  }
                }
              }
            }

            // fill subheader; image_min and image_max are 16 bit values
            const double shortMin = std::numeric_limits<short>::min();
            const double shortMax = std::numeric_limits<short>::max();
            shead["scale_factor"] = static_cast<float>(newScaleFactor);
            shead["image_min"] = static_cast<short>(qBound(shortMin, std::rint(newMin / newScaleFactor), shortMax));
            shead["image_max"] = static_cast<short>(qBound(shortMin, std::rint(newMax / newScaleFactor), shortMax));

            // specify target dataset based on
            // tdim type the user selected
            size_t f=1;
            size_t p=1;
            size_t g=1;
            size_t b=0;
            size_t d=0;
            switch(m_tdimType)
            {
              case DM_FRAMES: f=t+1; p=1; g=1, b=0; d=0; break;
              case DM_GATES:  f=1; p=1; g=t+1, b=0; d=0; break;
              case DM_BEDS:   f=1; p=1; g=1; b=t; d=0; break; // numbering of beds starts at 0
              case DM_DATA:   f=1; p=1; g=1; b=0; d=t; break; // numbering of datas starts at 0
              case DM_UNKNOWN: /* nothing */ break;
            }

            // create fresh sub header and write it out
            std::unique_ptr<CMedIOHeader> subHeader = createSubHeader(shead);
            CECAT7SubHeaderImage* e7SubHeader = static_cast<CECAT7SubHeaderImage*>(subHeader.get());
            if(ecatFile->writeSubHeader(*e7SubHeader, f, p, g, b, d) == false)
              throw std::runtime_error("writing subheader of volume " + std::to_string(t) + " failed.");
            if(ecatFile->writeMatrix(matrixData, matrixDataSize, f, p, g, b, d) == false)
              throw std::runtime_error("writing matrix data failed for volume " + std::to_string(t));
          }

          ecatFile->close();
        }
        catch(...)
        {
          ecatFile->close();
          ecatFile->remove();
          throw;
        }
      }
      else
        throw std::runtime_error("no CMedIOData object assigned");

      return *this;
    }
 
  public:
    size_t m_numdim;
    size_t m_xdim;
    size_t m_ydim;
    size_t m_zdim;
    size_t m_tdim;
    enum dimTypes m_tdimType;
    py::dict m_mhead;
    py::list m_shead;
    QString m_filename;
    CMedIOData::Format m_format;
 
  private:
    // file object of the image, shared by the copies of an image
    std::shared_ptr<CMedIOData> m_mioData;
    float* m_data;
};

