#include "pymedio.h"

#include <CMedIODataFactory>

#include <QString>
#include <QIODevice>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)
#define PYBIND11_DETAILED_ERROR_MESSAGES

namespace py = pybind11;

// external modules
extern void init_medio(py::module_ &);
extern void init_ecat(py::module_ &);
extern void init_ecat7(py::module_ &);

// defines the main entries of the pybind module
PYBIND11_MODULE(pymedio, m) {
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
  init_ecat(m);

  // CECAT7
  init_ecat7(m);

  ///////////////////////////////////////////////////////////
  // general purpose helper functions in pymedio namespace

  // general pymedio.read() function to read in MedIO compatible
  // data file and return the medioData object accordingly.
  m.def("read", [](const QString& fileName,
                   const std::vector<int>& volumes = {},
                   const std::vector<int>& rows = {},
                   const std::vector<int>& cols = {},
                   const std::vector<int>& planes = {}
                  )
  {
    CMedIOData* medioData = CMedIODataFactory::create(fileName);
    if(medioData != NULL)
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

  // general pymedio.write() function to write out a medioimage data
  m.def("write", [](const MedIOImage& img,
                    const QString& fileName,
                    const bool overwrite=false,
                    const QString& dtype="short",
                    const QString& format="original")
  {
    return true;
  }, py::arg("img"),
     py::arg("fileName"),
     py::arg("overwrite") = false,
     py::arg("dtype") = "short",
     py::arg("format") = "original");

  m.def("write", [](const py::array_t<float>& img,
                    const QString& fileName,
                    const bool overwrite=false,
                    const QString& dtype="short",
                    const QString& format="ecat7")
  {
    return true;
  }, py::arg("img"),
     py::arg("fileName"),
     py::arg("overwrite") = false,
     py::arg("dtype") = "short",
     py::arg("format") = "ecat7");

  // general purpose helper "MedIOImage" class to access our PET image
  // volumes using the python buffer protocol interface and in a numpy
  // compatible fashion
  pybind11::class_<MedIOImage>(m, "MedIOImage", pybind11::buffer_protocol())
    .def(py::init<py::ssize_t, py::ssize_t, py::ssize_t, py::ssize_t>())

    // Construct from a buffer
    .def(py::init([](const py::buffer& b) {
      py::buffer_info info = b.request();
      if(info.format != py::format_descriptor<float>::format() || info.ndim != 4) {
        throw std::runtime_error("incompatible buffer format!");
      }

      auto *v = new MedIOImage(info.shape[0], info.shape[1], info.shape[2], info.shape[3]);
      memcpy(v->data(), info.ptr, sizeof(float) * v->xdim() * v->ydim() * v->zdim() * v->tdim());
      return v;
    }))

    // accessor methods
    .def("ndim", &MedIOImage::numdim)
    .def("xdim", &MedIOImage::xdim)
    .def("ydim", &MedIOImage::ydim)
    .def("zdim", &MedIOImage::zdim)
    .def("tdim", &MedIOImage::tdim)

    // Bare bones interface
    .def("__getitem__",
      [](const MedIOImage& img, std::tuple<py::ssize_t, py::ssize_t, py::ssize_t, py::ssize_t> i) {
        if(std::get<0>(i) >= static_cast<int>(img.xdim()) || std::get<1>(i) >= static_cast<int>(img.ydim()) || 
           std::get<2>(i) >= static_cast<int>(img.zdim()) || std::get<3>(i) >= static_cast<int>(img.tdim())) {
          throw py::index_error();
        }
        return img(std::get<0>(i), std::get<1>(i), std::get<2>(i), std::get<3>(i));
    })
    .def("__setitem__",
      [](MedIOImage& img, std::tuple<py::ssize_t, py::ssize_t, py::ssize_t, py::ssize_t> i, float v) {
        if(std::get<0>(i) >= static_cast<int>(img.xdim()) || std::get<1>(i) >= static_cast<int>(img.ydim()) || 
           std::get<2>(i) >= static_cast<int>(img.zdim()) || std::get<3>(i) >= static_cast<int>(img.tdim())) {
          throw py::index_error();
        }
        img(std::get<0>(i), std::get<1>(i), std::get<2>(i), std::get<3>(i)) = v;
    })

    // Provide buffer access
    .def_buffer([](MedIOImage& self) -> py::buffer_info {
      return py::buffer_info(
        self.data(),                                              // Pointer to buffer
        sizeof(float),                                            // Size of one scalar
        py::format_descriptor<float>::format(),                   // Python struct-style format descriptor
        self.numdim(),                                            // Number of dimensions
        { self.xdim(), self.ydim(), self.zdim(), self.tdim() },   // Buffer dimensions
        {                                                         // Strides (in bytes) for each index
          sizeof(float),                                          // X
          sizeof(float) * self.xdim(),                            // Y
          sizeof(float) * self.xdim() * self.ydim(),              // Z
          sizeof(float) * self.xdim() * self.ydim() * self.zdim() // T (frames/gates/bed)
        }
      );
    });
}
