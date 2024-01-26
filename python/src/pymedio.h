#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <CMedIOData>
#include <CECATFile>
#include <CECAT7MainHeader>
#include <CECAT7SubHeaderImage>

#include <QIODevice>
#include <QString>

#include <iostream>

typedef enum
{
  FRAMES = 'f',
  GATES = 'g',
  BEDS = 'b'
} VolumeSelection;

// special helper MedIOImage class to embed all necessary
// information and always return the pet data as float data
class MedIOImage
{
  public:
    MedIOImage(CMedIOData* mioData, std::vector<short> volumes={}, std::vector<short> rows={}, std::vector<short> cols={}, std::vector<short> planes={})
      : m_mioData(mioData),
        m_numdim(0),
        m_xdim(0),
        m_ydim(0),
        m_zdim(0),
        m_tdim(0),
        m_data(nullptr)
    {
      if(mioData->open(QIODevice::ReadOnly))
      {
        if(mioData->dataFormat() == CMedIOData::ECAT)
        {
          CECATFile* eFile = static_cast<CECATFile*>(mioData);
          CECATMainHeader* ecatMainHeader = nullptr;
          if(eFile->readMainHeader(ecatMainHeader) == true)
          {
            if(ecatMainHeader->mainHeaderType() == CECATMainHeader::ECAT7MainHeader)
            {
              CECAT7MainHeader* e7MainHeader = static_cast<CECAT7MainHeader*>(ecatMainHeader);

              int num_frames = e7MainHeader->num_Frames();
              int num_gates = e7MainHeader->num_Gates();
              int num_bed_pos = e7MainHeader->num_Bed_Pos();
              float ecat_calibration_factor = e7MainHeader->calibration_Factor();

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
                std::vector<short>::iterator it;
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

              int t=0;
              std::vector<short>::iterator it;
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
                  if(ecatSubHeader->subHeaderType() == CECATSubHeader::ECAT7_Image)
                  {
                    CECAT7SubHeaderImage* e7SubHeaderImage = static_cast<CECAT7SubHeaderImage*>(ecatSubHeader);
                    int data_type = e7SubHeaderImage->data_Type();
                    float scale_factor = e7SubHeaderImage->scale_Factor();

                    // set global m_xdim once or check
                    if(m_xdim != 0)
                    {
                      if(m_xdim != e7SubHeaderImage->x_Dimension())
                        throw std::runtime_error("unequal X dimensions in dataset (" + std::to_string(m_xdim) + " != " + std::to_string(e7SubHeaderImage->x_Dimension()) + ")");
                    }
                    else
                      m_xdim = e7SubHeaderImage->x_Dimension();

                    // set global m_ydim once or check
                    if(m_ydim != 0)
                    {
                      if(m_ydim != e7SubHeaderImage->y_Dimension())
                        throw std::runtime_error("unequal Y dimensions in dataset (" + std::to_string(m_ydim) + " != " + std::to_string(e7SubHeaderImage->y_Dimension()) + ")");
                    }
                    else
                      m_ydim = e7SubHeaderImage->y_Dimension();

                    // set global m_zdim once or check
                    if(m_zdim != 0)
                    {
                      if(m_zdim != e7SubHeaderImage->z_Dimension())
                        throw std::runtime_error("unequal Z dimensions in dataset (" + std::to_string(m_zdim) + " != " + std::to_string(e7SubHeaderImage->z_Dimension()) + ")");
                    }
                    else
                      m_zdim = e7SubHeaderImage->z_Dimension();

                    // check if rows, cols and/or planes index arrays are given
                    // if not take all of them
                    bool exceeding_boundaries = false;
                    if(rows.size() == 0)
                    {
                      for(int i = 1; i <= m_ydim; i++)
                        rows.push_back(i);
                    }
                    else
                    {
                      // we have to check if the index std::vector exceeds the boundaries
                      std::vector<short>::iterator it;
                      for(it = rows.begin(); it < rows.end(); ++it)
                      {
                        if( (*it < 1) || (*it > m_ydim) )
                        {
                          throw std::runtime_error("index in rows vector (" + std::to_string(*it) + ") is out of bounds: there are " + std::to_string(m_ydim) + " rows in this file");
                          exceeding_boundaries = true;
                        }
                      }
                    }

                    if(cols.size() == 0)
                    {
                      for(int i = 1; i <= m_xdim; i++)
                        cols.push_back(i);
                    }
                    else
                    {
                      // we have to check if the index std::vector exceeds the boundaries
                      std::vector<short>::iterator it;
                      for(it = cols.begin(); it < cols.end(); ++it)
                      {
                        if( (*it < 1) || (*it > m_xdim) )
                        {
                          throw std::runtime_error("index in cols vector (" + std::to_string(*it) + ") is out of bounds: there are " + std::to_string(m_xdim) + " cols in this file");
                          exceeding_boundaries = true;
                        }
                      }
                    }

                    if(planes.size() == 0)
                    {
                      for(int i = 1; i <= m_zdim; i++)
                        planes.push_back(i);
                    }
                    else
                    {
                      // we have to check if the index std::vector exceeds the boundaries
                      std::vector<short>::iterator it;
                      for(it = planes.begin(); it < planes.end(); ++it)
                      {
                        if( (*it < 1) || (*it > m_zdim))
                        {
                          throw std::runtime_error("index in planes vector (" + std::to_string(*it) + ") is out of bounds: there are " + std::to_string(m_zdim) + " planes in this file");
                          exceeding_boundaries = true;
                        }
                      }
                    }
      
                    if(exceeding_boundaries == true)
                      break;

                    // create data array with (volumes * xdim * ydim * zdim) space
                    // if it does not exist yet
                    if(m_data == nullptr)
                      m_data = new float[m_xdim * m_ydim * m_zdim * volumes.size()];

                    char* byteData = NULL;
                    unsigned int byteDataSize = 0;

                    if(eFile->readMatrix(byteData, byteDataSize, frame, 1, gate, bed, 0) == true)
                    {
                      int plane_size = m_xdim * m_ydim;
                      int volume_size = m_xdim * m_ydim * m_zdim;

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
          throw std::runtime_error("unsupported data format: " + mioData->dataFormat());

        mioData->close();
      }
      else
        throw std::runtime_error("could not open " + mioData->fileName().toStdString());
    }

    MedIOImage(size_t xdim, size_t ydim=1, size_t zdim=1, size_t tdim=1)
      : m_mioData(nullptr),
        m_numdim(4),
        m_xdim(xdim),
        m_ydim(ydim),
        m_zdim(zdim),
        m_tdim(tdim)
    {
      m_data = new float[m_xdim * m_ydim * m_zdim * m_tdim];
      memset(m_data, 0, sizeof(float) * (m_xdim * m_ydim * m_zdim * m_tdim));
    }
 
    MedIOImage(const MedIOImage& img)
      : m_mioData(img.m_mioData),
        m_numdim(img.m_numdim),
        m_xdim(img.m_xdim),
        m_ydim(img.m_ydim),
        m_zdim(img.m_zdim),
        m_tdim(img.m_tdim)
    {
      m_data = new float[m_xdim * m_ydim * m_zdim * m_tdim];
      memcpy(m_data, img.m_data, sizeof(float) * (m_xdim * m_ydim * m_zdim * m_tdim));
    }

    MedIOImage(MedIOImage&& img)
      : m_mioData(img.m_mioData),
        m_numdim(img.m_numdim),
        m_xdim(img.m_xdim),
        m_ydim(img.m_ydim),
        m_zdim(img.m_zdim),
        m_tdim(img.m_tdim),
        m_data(img.m_data)
    {
      img.m_mioData = nullptr;
      img.m_numdim = 0;
      img.m_xdim = 0;
      img.m_ydim = 0;
      img.m_zdim = 0;
      img.m_tdim = 0;
      img.m_data = nullptr;
    }

    ~MedIOImage()
    {
      delete[] m_data;
    }

    MedIOImage& operator=(const MedIOImage& img) {
      if(this == &img)
        return *this;

      delete[] m_data;
      m_mioData = img.m_mioData;
      m_numdim = img.m_numdim;
      m_xdim = img.m_xdim;
      m_ydim = img.m_ydim;
      m_zdim = img.m_zdim;
      m_tdim = img.m_tdim;
      m_data = new float[m_xdim * m_ydim * m_zdim * m_tdim];
      memcpy(m_data, img.m_data, sizeof(float) * (m_xdim * m_ydim * m_zdim * m_tdim));
    }

    MedIOImage& operator=(MedIOImage&& img) {
      if(&img != this)
      {
        delete[] m_data;
        m_mioData = img.m_mioData;
        m_numdim = img.m_numdim;
        m_xdim = img.m_xdim;
        m_ydim = img.m_ydim;
        m_zdim = img.m_zdim;
        m_tdim = img.m_tdim;
        m_data = img.m_data;
        img.m_mioData = nullptr;
        img.m_numdim = 0;
        img.m_xdim = 0;
        img.m_ydim = 0;
        img.m_zdim = 0;
        img.m_tdim = 0;
        img.m_data = 0;
      }
      return *this;
    }

    float operator()(size_t x, size_t y=0, size_t z=0, size_t t=0) const {
      return m_data[x * m_xdim + y * m_ydim + z * m_zdim + t * m_tdim];
    } 

    float& operator()(size_t x, size_t y=0, size_t z=0, size_t t=0) {
      return m_data[x * m_xdim + y * m_ydim + z * m_zdim + t * m_tdim];
    } 

    float* data() { return m_data; }
    size_t numdim() const { return m_numdim; }
    size_t ndim() const { return m_numdim; }
    size_t xdim() const { return m_xdim; }
    size_t ydim() const { return m_ydim; }
    size_t zdim() const { return m_zdim; }
    size_t tdim() const { return m_tdim; }

  private:
    CMedIOData* m_mioData;
    size_t m_numdim;
    size_t m_xdim;
    size_t m_ydim;
    size_t m_zdim;
    size_t m_tdim;
    float *m_data;
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
