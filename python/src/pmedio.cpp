#include "pmedio.h"

#include <CMedIODataFactory>

#include <QString>
#include <QIODevice>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

// external modules
extern void init_medio(py::module_ &);
//extern void init_ecat(py::module_ &);
//extern void init_ecat7(py::module_ &);

// defines the main entries of the pybind module
PYBIND11_MODULE(pmedio, m) {
  // documentation
  m.doc() = "libmedio python interface";

  // specify __version__
  #ifdef VERSION_INFO
  m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
  #else
  m.attr("__version__") = "dev";
  #endif

  // CMedIO
  init_medio(m);

  // CECAT
  //init_ecat(m);

  // CECAT7
  //init_ecat7(m);

  ///////////////////////////////////////////////////////////
  // general purpose helper "MedIOImage" class to access PET image
  // volumes using the python buffer protocol interface and in a numpy
  // compatible fashion
  py::class_<MedIOImage> mi(m, "MedIOImage", py::buffer_protocol());

  // enum types
  py::enum_<CMedIOData::Format>(mi, "FormatType")
    .value("Unknown", CMedIOData::Unknown)
    .value("ConcordeMicropet", CMedIOData::ConcordeMicropet)
    .value("ECAT", CMedIOData::ECAT)
    .value("Philips", CMedIOData::Philips)
    .value("PhilipsBin", CMedIOData::PhilipsBin);

  py::enum_<enum dimTypes>(mi, "DimType")
    .value("UNKNOWN", DM_UNKNOWN)
    .value("FRAMES", DM_FRAMES)
    .value("GATES", DM_GATES)
    .value("BEDS", DM_BEDS)
    .value("DATA", DM_DATA);

  py::enum_<enum dataTypes>(mi, "DataType")
    .value("FLOAT", DT_FLOAT)
    .value("SHORT", DT_SHORT)
    .value("LONG", DT_LONG)
    .value("BYTE", DT_BYTE);

  // method definitions
  mi.def(py::init<const size_t,
                  const size_t,
                  const size_t,
                  const size_t,
                  const CMedIOData::Format&>(),
              py::arg("xdim"),
              py::arg("ydim"),
              py::arg("zdim"),
              py::arg("tdim"),
              py::arg("format") = CMedIOData::ECAT)

    // Construct from another MedIOImage object
    .def(py::init<const MedIOImage&>(),
              py::arg("img"))

    // Construct from another array
    .def(py::init<const py::array_t<float, py::array::f_style | py::array::forcecast>&,
                  const CMedIOData::Format&,
                  const enum dimTypes>(),
              py::arg("img"),
              py::arg("format") = CMedIOData::ECAT,
              py::arg("dimType") = DM_FRAMES)

    // Construct from another buffer
    .def(py::init<const py::buffer&, 
                  const CMedIOData::Format&,
                  const enum dimTypes>(),
              py::arg("img"),
              py::arg("format") = CMedIOData::ECAT,
              py::arg("dimType") = DM_FRAMES)

    // accessor methods
    .def_readonly("ndim", &MedIOImage::m_numdim)
    .def_readonly("xdim", &MedIOImage::m_xdim)
    .def_readonly("ydim", &MedIOImage::m_ydim)
    .def_readonly("zdim", &MedIOImage::m_zdim)
    .def_readonly("tdim", &MedIOImage::m_tdim)
    .def_readonly("dimType", &MedIOImage::m_tdimType)
    .def_readonly("format", &MedIOImage::m_format)
    .def_readwrite("mhead", &MedIOImage::m_mhead)
    .def_readwrite("shead", &MedIOImage::m_shead)
    .def_readwrite("filename", &MedIOImage::m_filename)

    // provide shape function similar to numpy.ndarray
    .def_property_readonly("shape", [](MedIOImage& self) -> py::tuple {
      return py::make_tuple(self.m_xdim, self.m_ydim, self.m_zdim, self.m_tdim);
    })
    .def_property_readonly("size", [](MedIOImage& self) -> ssize_t {
      return self.m_xdim * self.m_ydim * self.m_zdim * self.m_tdim;
    })
    .def_property_readonly("nbytes", [](MedIOImage& self) -> ssize_t {
      return self.m_xdim * self.m_ydim * self.m_zdim * self.m_tdim * sizeof(float);
    })

    // read/write interface
    .def("read", [](MedIOImage& self) -> MedIOImage {
      return self.readImage();
    })
    .def("write", [](MedIOImage& self,
                     const QString& fileName = QString(),
                     const bool overwrite = false) -> MedIOImage {
      if(fileName.isEmpty())
        return self.writeImage(self.m_filename, true);
      else
        return self.writeImage(fileName, overwrite);
    }, py::arg("filename") = QString(),
       py::arg("overwrite") = false)

    // Bare bones interface
    .def("__getitem__",
      [](const MedIOImage& self, std::tuple<py::ssize_t, py::ssize_t, py::ssize_t, py::ssize_t> i) {
        if(std::get<0>(i) >= static_cast<int>(self.m_xdim) || std::get<1>(i) >= static_cast<int>(self.m_ydim) || 
           std::get<2>(i) >= static_cast<int>(self.m_zdim) || std::get<3>(i) >= static_cast<int>(self.m_tdim)) {
          throw py::index_error();
        }
        return self(std::get<0>(i), std::get<1>(i), std::get<2>(i), std::get<3>(i));
    })
    .def("__setitem__",
      [](MedIOImage& self, std::tuple<py::ssize_t, py::ssize_t, py::ssize_t, py::ssize_t> i, float v) {
        if(std::get<0>(i) >= static_cast<int>(self.m_xdim) || std::get<1>(i) >= static_cast<int>(self.m_ydim) || 
           std::get<2>(i) >= static_cast<int>(self.m_zdim) || std::get<3>(i) >= static_cast<int>(self.m_tdim)) {
          throw py::index_error();
        }
        self(std::get<0>(i), std::get<1>(i), std::get<2>(i), std::get<3>(i)) = v;
    })

    // toarray method to directly return as numpy.array
    .def("toarray",
      [](MedIOImage& self, const bool copy=false) -> py::array_t<float, py::array::f_style | py::array::forcecast> {
      float* data = self.data();
      py::capsule capsule;
      if(copy == true)
      {
        // create a data clone
        ssize_t size = self.m_xdim * self.m_ydim * self.m_zdim * self.m_tdim;
        float* clone = new float[size];
        for(ssize_t i = 0; i < size; ++i)
          clone[i] = data[i];
       
        // Create a Python object that will free the allocated
        // memory when destroyed:
        capsule = py::capsule(clone, [](void* f) {
          float* clone = reinterpret_cast<float*>(f);
          delete[] clone;
        });

        // replace data ptr with clone
        data = clone;
      }

      return py::array_t<float, py::array::f_style | py::array::forcecast>(py::buffer_info(
        data,                                                     // Pointer to buffer
        sizeof(float),                                            // Size of one scalar
        py::format_descriptor<float>::format(),                   // Python struct-style format descriptor
        self.m_numdim,                                            // Number of dimensions
        { self.m_xdim, self.m_ydim, self.m_zdim, self.m_tdim },   // Buffer dimensions
        {                                                         // Strides (in bytes) for each index
          sizeof(float),                                          // X
          sizeof(float) * self.m_xdim,                            // Y
          sizeof(float) * self.m_xdim * self.m_ydim,              // Z
          sizeof(float) * self.m_xdim * self.m_ydim * self.m_zdim // T (frames/gates/bed/data)
        }
      ), capsule);
    }, py::arg("copy") = false)

    // Provide buffer access
    .def_buffer([](MedIOImage& self) -> py::buffer_info {
      return py::buffer_info(
        self.data(),                                              // Pointer to buffer
        sizeof(float),                                            // Size of one scalar
        py::format_descriptor<float>::format(),                   // Python struct-style format descriptor
        self.m_numdim,                                            // Number of dimensions
        { self.m_xdim, self.m_ydim, self.m_zdim, self.m_tdim },   // Buffer dimensions
        {                                                         // Strides (in bytes) for each index
          sizeof(float),                                          // X
          sizeof(float) * self.m_xdim,                            // Y
          sizeof(float) * self.m_xdim * self.m_ydim,              // Z
          sizeof(float) * self.m_xdim * self.m_ydim * self.m_zdim // T (frames/gates/bed/data)
        }
      );
    });

  // general pmedio.read() function to read in MedIO compatible
  // data file and return the medioData object accordingly.
  m.def("read", [](const QString& fileName,
                   const std::vector<int>& volumes = {},
                   const std::vector<int>& rows = {},
                   const std::vector<int>& cols = {},
                   const std::vector<int>& planes = {}
                  ) -> MedIOImage
  {
    CMedIOData* medioData = CMedIODataFactory::create(fileName);
    if(medioData != nullptr)
    {
      MedIOImage medioImage(medioData);
      return medioImage.readImage(volumes, rows, cols, planes);
    }
    else
    {
      throw std::runtime_error("invalid/unsupported file supplied");
    }
  }, py::arg("fileName"),
     py::arg("volumes") = std::vector<int>(),
     py::arg("rows") = std::vector<int>(),
     py::arg("cols") = std::vector<int>(),
     py::arg("planes") = std::vector<int>());

  // general pmedio.write() function to write out a medioimage data
  m.def("write", [](MedIOImage& img,
                    const QString& fileName,
                    const bool overwrite=false,
                    const enum dimTypes dimType=DM_UNKNOWN,
                    const enum dataTypes dataType=DT_FLOAT,
                    const CMedIOData::Format& format=CMedIOData::Unknown) -> MedIOImage
  {
    return img.writeImage(fileName, overwrite, dimType, dataType, format);
  }, py::arg("img"),
     py::arg("fileName"),
     py::arg("overwrite") = false,
     py::arg("dimType") = DM_UNKNOWN,
     py::arg("dataType") = DT_FLOAT,
     py::arg("format") = CMedIOData::Unknown);

  m.def("write", [](const py::array_t<float, py::array::f_style | py::array::forcecast>& img,
                    const QString& fileName,
                    const bool overwrite=false,
                    const enum dimTypes dimType=DM_FRAMES,
                    const enum dataTypes dataType=DT_FLOAT,
                    const CMedIOData::Format& format=CMedIOData::ECAT) -> MedIOImage
  {
    MedIOImage medioImage(img, format, dimType);
    return medioImage.writeImage(fileName, overwrite, dimType, dataType, format);
  }, py::arg("img"),
     py::arg("fileName"),
     py::arg("overwrite") = false,
     py::arg("dimType") = DM_FRAMES,
     py::arg("dataType") = DT_FLOAT,
     py::arg("format") = CMedIOData::ECAT);
}
