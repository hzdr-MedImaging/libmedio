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

#include "pmedio.h"

#include <CMedIODataFactory>

#include <QString>
#include <QIODevice>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = nanobind;

// external modules
extern void init_medio(py::module_ &);
extern void init_volume(py::module_ &);
//extern void init_ecat(py::module_ &);
//extern void init_ecat7(py::module_ &);

// float32 array in the voxel order of MedIOImage of an array or of an object numpy
// converts to an array (other data types and memory layouts, nested lists, ...)
static FloatArray floatArray(const py::object& img)
{
  return py::cast<FloatArray>(py::module_::import_("numpy").attr("asarray")(img, py::arg("dtype") = "float32",
                                                                              py::arg("order") = "F"));
}

// buffer protocol of MedIOImage: nanobind has no counterpart of
// py::buffer_protocol()/def_buffer(), the type slots are filled directly
static int medioImageGetBuffer(PyObject* exporter, Py_buffer* view, int flags)
{
  MedIOImage* self = py::inst_ptr<MedIOImage>(py::handle(exporter));
  const Py_ssize_t dims[4] = { static_cast<Py_ssize_t>(self->m_xdim), static_cast<Py_ssize_t>(self->m_ydim),
                               static_cast<Py_ssize_t>(self->m_zdim), static_cast<Py_ssize_t>(self->m_tdim) };

  view->obj = nullptr;
  if((flags & PyBUF_C_CONTIGUOUS) == PyBUF_C_CONTIGUOUS)
  {
    PyErr_SetString(PyExc_BufferError, "the voxels of a MedIOImage are not C contiguous");
    return -1;
  }
  if((flags & PyBUF_STRIDES) != PyBUF_STRIDES)
  {
    PyErr_SetString(PyExc_BufferError, "a MedIOImage is a multidimensional Fortran contiguous buffer");
    return -1;
  }

  // shape and strides live as long as the view
  Py_ssize_t* layout = new Py_ssize_t[8];
  Py_ssize_t stride = sizeof(float);
  for(int i = 0; i < 4; ++i)
  {
    layout[i] = dims[i];
    layout[4 + i] = stride;
    stride *= dims[i];
  }

  view->buf = self->data();
  view->len = stride;
  view->itemsize = sizeof(float);
  view->readonly = 0;
  view->ndim = 4;
  view->format = (flags & PyBUF_FORMAT) == PyBUF_FORMAT ? const_cast<char*>("f") : nullptr;
  view->shape = layout;
  view->strides = layout + 4;
  view->suboffsets = nullptr;
  view->internal = layout;
  view->obj = exporter;
  Py_INCREF(exporter);

  return 0;
}

static void medioImageReleaseBuffer(PyObject*, Py_buffer* view)
{
  delete[] static_cast<Py_ssize_t*>(view->internal);
  view->internal = nullptr;
}

static PyType_Slot medioImageSlots[] = {
  { Py_bf_getbuffer, reinterpret_cast<void*>(medioImageGetBuffer) },
  { Py_bf_releasebuffer, reinterpret_cast<void*>(medioImageReleaseBuffer) },
  { 0, nullptr }
};

// classes that only hold the names of formats, dimension and data types
struct FormatNames { };
struct DimTypeNames { };
struct DataTypeNames { };

// defines the main entries of the module
NB_MODULE(pmedio, m) {
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
  py::class_<MedIOImage> mi(m, "MedIOImage", py::type_slots(medioImageSlots));

  // the formats, dimension types and data types are passed and returned as
  // strings, the constants are these strings
  py::class_<FormatNames> formatType(mi, "FormatType");
  formatType.attr("Unknown") = py::str(formatName(CMedIOData::Unknown));
  formatType.attr("ConcordeMicropet") = py::str(formatName(CMedIOData::ConcordeMicropet));
  formatType.attr("ECAT") = py::str(formatName(CMedIOData::ECAT));
  formatType.attr("Philips") = py::str(formatName(CMedIOData::Philips));
  formatType.attr("PhilipsBin") = py::str(formatName(CMedIOData::PhilipsBin));
  formatType.attr("NIFTI") = py::str(formatName(CMedIOData::NIFTI));

  py::class_<DimTypeNames> dimType(mi, "DimType");
  dimType.attr("UNKNOWN") = py::str(dimTypeName(DM_UNKNOWN));
  dimType.attr("FRAMES") = py::str(dimTypeName(DM_FRAMES));
  dimType.attr("GATES") = py::str(dimTypeName(DM_GATES));
  dimType.attr("BEDS") = py::str(dimTypeName(DM_BEDS));
  dimType.attr("DATA") = py::str(dimTypeName(DM_DATA));

  py::class_<DataTypeNames> dataType(mi, "DataType");
  dataType.attr("FLOAT") = py::str(dataTypeName(DT_FLOAT));
  dataType.attr("SHORT") = py::str(dataTypeName(DT_SHORT));
  dataType.attr("LONG") = py::str(dataTypeName(DT_LONG));
  dataType.attr("BYTE") = py::str(dataTypeName(DT_BYTE));

  // method definitions; format, dimension type and data type are taken as
  // str and converted here, so that unknown names raise a RuntimeError
  mi.def("__init__", [](MedIOImage* self, const size_t xdim, const size_t ydim, const size_t zdim,
                        const size_t tdim, const QString& format) {
           new (self) MedIOImage(xdim, ydim, zdim, tdim, formatOfName(format));
         },
              py::arg("xdim"),
              py::arg("ydim"),
              py::arg("zdim"),
              py::arg("tdim"),
              py::arg("format") = formatName(CMedIOData::ECAT))

    // Construct from another MedIOImage object
    .def(py::init<const MedIOImage&>(),
              py::arg("img"))

    // Construct from an array or an object numpy converts to an array
    .def("__init__", [](MedIOImage* self, const py::object& img, const QString& format, const QString& dimType) {
           new (self) MedIOImage(floatArray(img), formatOfName(format), dimTypeOfName(dimType));
         },
              py::arg("img"),
              py::arg("format") = formatName(CMedIOData::ECAT),
              py::arg("dimType") = dimTypeName(DM_FRAMES))

    // accessor methods
    .def_ro("ndim", &MedIOImage::m_numdim)
    .def_ro("xdim", &MedIOImage::m_xdim)
    .def_ro("ydim", &MedIOImage::m_ydim)
    .def_ro("zdim", &MedIOImage::m_zdim)
    .def_ro("tdim", &MedIOImage::m_tdim)
    .def_ro("dimType", &MedIOImage::m_tdimType)
    .def_ro("format", &MedIOImage::m_format)
    .def_rw("mhead", &MedIOImage::m_mhead)
    .def_rw("shead", &MedIOImage::m_shead)
    .def_rw("filename", &MedIOImage::m_filename)

    // provide shape function similar to numpy.ndarray
    .def_prop_ro("shape", [](MedIOImage& self) -> py::tuple {
      return py::make_tuple(self.m_xdim, self.m_ydim, self.m_zdim, self.m_tdim);
    })
    .def_prop_ro("size", [](MedIOImage& self) -> ssize_t {
      return self.m_xdim * self.m_ydim * self.m_zdim * self.m_tdim;
    })
    .def_prop_ro("nbytes", [](MedIOImage& self) -> ssize_t {
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
      [](const MedIOImage& self, std::tuple<Py_ssize_t, Py_ssize_t, Py_ssize_t, Py_ssize_t> i) {
        // negative indices are not supported and must not address memory in front of the data
        if(std::get<0>(i) < 0 || std::get<1>(i) < 0 || std::get<2>(i) < 0 || std::get<3>(i) < 0 ||
           std::get<0>(i) >= static_cast<int>(self.m_xdim) || std::get<1>(i) >= static_cast<int>(self.m_ydim) ||
           std::get<2>(i) >= static_cast<int>(self.m_zdim) || std::get<3>(i) >= static_cast<int>(self.m_tdim)) {
          throw py::index_error();
        }
        return self(std::get<0>(i), std::get<1>(i), std::get<2>(i), std::get<3>(i));
    })
    .def("__setitem__",
      [](MedIOImage& self, std::tuple<Py_ssize_t, Py_ssize_t, Py_ssize_t, Py_ssize_t> i, float v) {
        // negative indices are not supported and must not address memory in front of the data
        if(std::get<0>(i) < 0 || std::get<1>(i) < 0 || std::get<2>(i) < 0 || std::get<3>(i) < 0 ||
           std::get<0>(i) >= static_cast<int>(self.m_xdim) || std::get<1>(i) >= static_cast<int>(self.m_ydim) ||
           std::get<2>(i) >= static_cast<int>(self.m_zdim) || std::get<3>(i) >= static_cast<int>(self.m_tdim)) {
          throw py::index_error();
        }
        self(std::get<0>(i), std::get<1>(i), std::get<2>(i), std::get<3>(i)) = v;
    })

    // toarray method to directly return as numpy.array; the array is a copy
    // of the voxels with and without copy
    .def("toarray",
      [](MedIOImage& self, const bool /* copy */) {
      const size_t size = self.m_xdim * self.m_ydim * self.m_zdim * self.m_tdim;
      float* clone = new float[size];
      memcpy(clone, self.data(), size * sizeof(float));

      // Python object that frees the copy when the array is destroyed
      py::capsule owner(clone, [](void* p) noexcept { delete[] static_cast<float*>(p); });

      return py::ndarray<py::numpy, float, py::f_contig>(clone, { self.m_xdim, self.m_ydim, self.m_zdim, self.m_tdim },
                                                         owner);
    }, py::arg("copy") = false,
       "Voxels as numpy array (x, y, z, t) of float32. The array is always a copy of the\n"
       "voxels, with and without ``copy``; ``numpy.asarray(img)`` uses the memory of the\n"
       "image instead.");

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
     py::arg("planes") = std::vector<int>(),
     "Read an ECAT7 image file as MedIOImage.\n\n"
     "``volumes`` selects the frames (or gates, bed positions, data indices) to read in\n"
     "the given order, numbered from 1 in the order of the file; by default all of them\n"
     "are read. ``img[..., k]`` and ``img.shead[k]`` belong to the same volume.\n"
     "``rows``, ``cols`` and ``planes`` (numbered from 1) are only checked against the\n"
     "dimensions of the file, the complete volumes are returned.");

  // general pmedio.write() function to write out a medioimage data
  m.def("write", [](MedIOImage& img,
                    const QString& fileName,
                    const bool overwrite,
                    const QString& dimType,
                    const QString& dataType,
                    const QString& format) -> MedIOImage
  {
    return img.writeImage(fileName, overwrite, dimTypeOfName(dimType), dataTypeOfName(dataType), formatOfName(format));
  }, py::arg("img"),
     py::arg("fileName"),
     py::arg("overwrite") = false,
     py::arg("dimType") = dimTypeName(DM_UNKNOWN),
     py::arg("dataType") = dataTypeName(DT_FLOAT),
     py::arg("format") = formatName(CMedIOData::Unknown));

  m.def("write", [](const py::object& img,
                    const QString& fileName,
                    const bool overwrite,
                    const QString& dimTypeName,
                    const QString& dataTypeName,
                    const QString& formatName) -> MedIOImage
  {
    const enum dimTypes dimType = dimTypeOfName(dimTypeName);
    const enum dataTypes dataType = dataTypeOfName(dataTypeName);
    const CMedIOData::Format format = formatOfName(formatName);
    MedIOImage medioImage(floatArray(img), format, dimType);
    return medioImage.writeImage(fileName, overwrite, dimType, dataType, format);
  }, py::arg("img"),
     py::arg("fileName"),
     py::arg("overwrite") = false,
     py::arg("dimType") = dimTypeName(DM_FRAMES),
     py::arg("dataType") = dataTypeName(DT_FLOAT),
     py::arg("format") = formatName(CMedIOData::ECAT));

  // format independent access to the frames of all formats (pmedio.Volume,
  // pmedio.File, pmedio.open(), pmedio.create()), see volume.cpp
  init_volume(m);
}
