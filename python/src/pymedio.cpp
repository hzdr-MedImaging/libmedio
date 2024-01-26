#include "pymedio.h"

#include <CMedIODataFactory>

#include <QString>
#include <QIODevice>

namespace py = pybind11;

// external modules
extern void init_medio(py::module_ &);
extern void init_ecat(py::module_ &);
extern void init_ecat7(py::module_ &);

PYBIND11_MAKE_OPAQUE(std::vector<short>);

// defines the main entries of the pybind module
PYBIND11_MODULE(pymedio, m) {
  // documentation
  m.doc() = "libmedio python interface";

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
  m.def("read", [](const QString& fileName)
  {
    CMedIOData* pMedIOData = nullptr;
    pMedIOData = CMedIODataFactory::create(fileName);
    if(pMedIOData != nullptr)
    {
      if(pMedIOData->open(QIODevice::ReadOnly) == false)
        pMedIOData = nullptr;
    }
    return pMedIOData;
  }, py::arg("fileName"));

  // general pymedio.write() function to write out a numpy compatible
  // 4D array to a medIO compatible format
  m.def("write", [](const QString& fileName, const MedIOImage& img)
  {
    return true;
  }, py::arg("fileName"),
     py::arg("img"));

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
    .def("numdim", &MedIOImage::numdim)
    .def("xdim", &MedIOImage::xdim)
    .def("ydim", &MedIOImage::ydim)
    .def("zdim", &MedIOImage::zdim)
    .def("tdim", &MedIOImage::zdim)

    // Bare bones interface
    .def("__getitem__",
      [](const MedIOImage& img, std::tuple<py::ssize_t, py::ssize_t, py::ssize_t, py::ssize_t> i) {
        if(std::get<0>(i) >= img.xdim() || std::get<1>(i) >= img.ydim() || 
           std::get<2>(i) >= img.zdim() || std::get<3>(i) >= img.tdim()) {
          throw py::index_error();
        }
        return img(std::get<0>(i), std::get<1>(i), std::get<2>(i), std::get<3>(i));
    })
    .def("__setitem__",
      [](MedIOImage& img, std::tuple<py::ssize_t, py::ssize_t, py::ssize_t, py::ssize_t> i, float v) {
        if(std::get<0>(i) >= img.xdim() || std::get<1>(i) >= img.ydim() || 
           std::get<2>(i) >= img.zdim() || std::get<3>(i) >= img.tdim()) {
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
