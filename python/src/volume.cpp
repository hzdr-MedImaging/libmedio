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

// Python interface to the format independent matrix access of libmedio
// (CMedIOVolume, CMedIOData::frameNumbers(), readVolume(), writeVolume()):
//
//   f = pmedio.open("image.nii.gz")          # ECAT7, NIfTI, Philips, microPET
//   for n in f.frame_numbers():
//       v = f.read_volume(n)                 # pmedio.Volume
//
//   out = pmedio.create("out.v", like=f)     # main header converted from f
//   out.write_volume(v, 1)
//   out.close()

#include "pmedio.h"

#include <nanobind/stl/unique_ptr.h>

#include <CMedIOData>
#include <CMedIODataFactory>
#include <CMedIOHeader>
#include <CMedIOHeaderDictionary>
#include <CMedIOVolume>
#include <CECATFile>
#include <CECATMainHeader>
#include <CNIFTIFile>
#include <CNIFTIMainHeader>
#include <CPhilipsFile>
#include <CPhilipsMainHeader>

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace py = nanobind;

namespace {

//---------------------------------------------------------------------------------------------
// helpers

// raise a Python exception of the given type
[[noreturn]] void raise(PyObject* type, const QString& message)
{
  PyErr_SetString(type, message.toUtf8().constData());
  throw py::python_error();
}

// file name of a str or os.PathLike object
QString pathOf(const py::object& path)
{
  py::object fspath = py::module_::import_("os").attr("fspath")(path);
  if(py::isinstance<py::str>(fspath) == false)
    raise(PyExc_TypeError, "the file name has to be a str or an os.PathLike object of str");
  return py::cast<QString>(fspath);
}

py::module_ numpy()
{
  return py::module_::import_("numpy");
}

// name of the numpy dtype of a CMedIOVolume data type
const char* numpyTypeName(CMedIOVolume::DataType type)
{
  switch(type) {
    case CMedIOVolume::UInt8:   return "uint8";
    case CMedIOVolume::Int8:    return "int8";
    case CMedIOVolume::Int16:   return "int16";
    case CMedIOVolume::UInt16:  return "uint16";
    case CMedIOVolume::Int32:   return "int32";
    case CMedIOVolume::UInt32:  return "uint32";
    case CMedIOVolume::Float32: return "float32";
    case CMedIOVolume::Float64: return "float64";
    default: break;
  }
  raise(PyExc_ValueError, "the volume has no data");
}

// numpy dtype (native byte order) of a CMedIOVolume data type
py::object numpyType(CMedIOVolume::DataType type)
{
  return numpy().attr("dtype")(numpyTypeName(type));
}

// CMedIOVolume data type of a numpy dtype (UnknownDataType if not supported)
CMedIOVolume::DataType volumeType(const py::object& dtype)
{
  const std::string kindName = py::cast<std::string>(dtype.attr("kind"));
  const char kind = kindName.size() == 1 ? kindName[0] : '?';
  const Py_ssize_t size = py::cast<Py_ssize_t>(dtype.attr("itemsize"));

  if(kind == 'u') {
    switch(size) {
      case 1: return CMedIOVolume::UInt8;
      case 2: return CMedIOVolume::UInt16;
      case 4: return CMedIOVolume::UInt32;
    }
  } else if(kind == 'i') {
    switch(size) {
      case 1: return CMedIOVolume::Int8;
      case 2: return CMedIOVolume::Int16;
      case 4: return CMedIOVolume::Int32;
    }
  } else if(kind == 'f') {
    switch(size) {
      case 4: return CMedIOVolume::Float32;
      case 8: return CMedIOVolume::Float64;
    }
  }

  return CMedIOVolume::UnknownDataType;
}

// voxels of a 3D array with x running fastest (Fortran order), in the byte
// order of this machine
QByteArray voxelBytes(const py::object& array, CMedIOVolume::DataType type)
{
  py::object native = numpy().attr("asarray")(array, py::arg("dtype") = numpyType(type), py::arg("order") = "F");
  auto a = py::cast<py::ndarray<py::ro, py::f_contig, py::device::cpu>>(native);
  if(a.nbytes() > static_cast<size_t>(std::numeric_limits<int>::max()))
    raise(PyExc_ValueError, "the voxel data is larger than 2 GiB");
  return QByteArray(reinterpret_cast<const char*>(a.data()), static_cast<int>(a.nbytes()));
}

// stored values of a volume as numpy array of shape (nx, ny, nz), x running fastest
py::object storedArray(const CMedIOVolume& volume)
{
  if(volume.isEmpty())
    raise(PyExc_ValueError, "the volume has no data");

  py::object result = numpy().attr("empty")(py::make_tuple(volume.dimension(0), volume.dimension(1), volume.dimension(2)),
                                            py::arg("dtype") = numpyType(volume.dataType()), py::arg("order") = "F");
  auto a = py::cast<py::ndarray<py::f_contig, py::device::cpu>>(result);
  std::memcpy(a.data(), volume.data().constData(), static_cast<size_t>(volume.data().size()));
  return result;
}

// 3 x 4 or 4 x 4 (last row 0 0 0 1) affine
void affineOf(const py::object& object, double m[3][4])
{
  py::ndarray<const double, py::c_contig, py::device::cpu> a;
  try {
    a = py::cast<py::ndarray<const double, py::c_contig, py::device::cpu>>(
          numpy().attr("asarray")(object, py::arg("dtype") = "float64", py::arg("order") = "C"));
  } catch(const std::exception&) {
    PyErr_Clear();
  }
  if(!a.is_valid() || a.ndim() != 2 || a.shape(1) != 4 || (a.shape(0) != 3 && a.shape(0) != 4))
    raise(PyExc_ValueError, "the affine has to be a 3 x 4 or 4 x 4 array");

  const double* r = a.data();
  if(a.shape(0) == 4 && (r[12] != 0.0 || r[13] != 0.0 || r[14] != 0.0 || r[15] != 1.0))
    raise(PyExc_ValueError, "the last row of a 4 x 4 affine has to be 0 0 0 1");

  for(int i=0; i < 3; i++)
    for(int j=0; j < 4; j++)
      m[i][j] = r[i * 4 + j];
}

// volume from a 3D numpy array (stored values)
CMedIOVolume volumeFromArray(const py::object& data, const py::object& affine, double scaleFactor,
                             double intercept, double frameStart, double frameDuration)
{
  const py::object array = numpy().attr("asarray")(data);
  const size_t ndim = py::cast<size_t>(array.attr("ndim"));
  if(ndim != 3)
    raise(PyExc_ValueError, QString("the voxel data has to be a 3D array (x, y, z), not %1D").arg(ndim));

  const CMedIOVolume::DataType type = volumeType(array.attr("dtype"));
  if(type == CMedIOVolume::UnknownDataType)
    raise(PyExc_TypeError, QString("unsupported data type '%1': uint8, int8, int16, uint16, int32, uint32, float32 "
                                   "and float64 are supported").arg(py::cast<QString>(py::steal<py::object>(PyObject_Str(array.attr("dtype").ptr())))));
  const QByteArray bytes = voxelBytes(array, type);
  const py::tuple shape = array.attr("shape");

  CMedIOVolume volume;
  if(volume.setData(py::cast<int>(shape[0]), py::cast<int>(shape[1]), py::cast<int>(shape[2]),
                    type, bytes) == false)
    raise(PyExc_ValueError, "invalid voxel data");

  if(affine.is_none() == false) {
    double m[3][4];
    affineOf(affine, m);
    volume.setAffine(m);
  }
  volume.setScaleFactor(scaleFactor);
  volume.setIntercept(intercept);
  volume.setFrameStart(frameStart);
  volume.setFrameDuration(frameDuration);
  return volume;
}

//---------------------------------------------------------------------------------------------
// an image file opened for reading (open()) or writing (create())

class MedIOFile
{
  public:
    MedIOFile(CMedIOData* data, const QString& fileName, bool writable)
      : m_pData(data), m_fileName(QFileInfo(fileName).absoluteFilePath()), m_writable(writable) { }
    ~MedIOFile() { close(); }

    MedIOFile(const MedIOFile&) = delete;
    MedIOFile& operator=(const MedIOFile&) = delete;

    CMedIOData* data() const
    {
      if(m_pData == nullptr)
        raise(PyExc_ValueError, "I/O operation on a closed file");
      return m_pData.get();
    }

    // the file name given to open()/create() (a compressed NIfTI file is
    // read from or written to a temporary file by CNIFTIFile)
    const QString& fileName() const { return m_fileName; }
    bool writable() const { return m_writable; }
    bool closed() const { return m_pData == nullptr; }

    void close()
    {
      if(m_pData != nullptr) {
        m_pData->close();
        m_pData.reset();
      }
    }

  private:
    std::unique_ptr<CMedIOData> m_pData;
    QString m_fileName;
    bool m_writable;
};

// target format and Philips file type of a file name created with create()
CMedIOData::Format createFormat(const QString& fileName, const py::object& format,
                                CPhilipsMainHeader::File_Type& philipsType)
{
  const QString name = fileName.toLower();
  philipsType = name.endsWith(".syn") ? CPhilipsMainHeader::Syntegra : CPhilipsMainHeader::Image;

  if(format.is_none() == false)
  {
    if(py::isinstance<py::str>(format) == false)
      throw std::runtime_error("the format has to be a str (pmedio.Format)");
    return formatOfName(py::cast<QString>(format));
  }

  if(name.endsWith(".v"))
    return CMedIOData::ECAT;
  if(name.endsWith(".nii") || name.endsWith(".nii.gz") || name.endsWith(".hdr") || name.endsWith(".hdr.gz"))
    return CMedIOData::NIFTI;
  if(name.endsWith(".img") || name.endsWith(".syn"))
    return CMedIOData::Philips;

  raise(PyExc_ValueError, "unknown file format of '" + fileName + "' (.v: ECAT7, .nii/.nii.gz/.hdr: NIfTI, "
                          ".img/.syn: Philips); give the format explicitly");
}

// write the main header in the representation of the file format into a new file
bool writeMainHeader(CMedIOData* file, CMedIOHeader* mainHeader, CMedIOData::Format format,
                     CPhilipsMainHeader::File_Type philipsType)
{
  bool result = false;
  mainHeader->setMedIOData(file);

  switch(format) {
    case CMedIOData::ECAT:
      result = static_cast<CECATFile*>(file)->writeMainHeader(*static_cast<CECATMainHeader*>(mainHeader));
    break;

    case CMedIOData::NIFTI:
      result = static_cast<CNIFTIFile*>(file)->writeMainHeader(*static_cast<CNIFTIMainHeader*>(mainHeader));
    break;

    case CMedIOData::Philips:
      static_cast<CPhilipsMainHeader*>(mainHeader)->setFiltyp(philipsType);
      result = static_cast<CPhilipsFile*>(file)->writeMainHeader(*static_cast<CPhilipsMainHeader*>(mainHeader));
    break;

    default:
    break;
  }

  mainHeader->setMedIOData(nullptr);
  return result;
}

std::unique_ptr<MedIOFile> createFile(const py::object& path, const MedIOFile& like, const py::object& format, Flag overwrite)
{
  if(like.writable())
    raise(PyExc_ValueError, "the source file 'like' has to be opened for reading with pmedio.open()");

  const QString fileName = pathOf(path);
  CPhilipsMainHeader::File_Type philipsType = CPhilipsMainHeader::Image;
  const CMedIOData::Format target = createFormat(fileName, format, philipsType);

  if(target != CMedIOData::ECAT && target != CMedIOData::NIFTI && target != CMedIOData::Philips)
    raise(PyExc_ValueError, QString("files of data format %1 cannot be written").arg(static_cast<int>(target)));

  if(QFileInfo::exists(fileName) && overwrite == false)
    raise(PyExc_FileExistsError, "'" + fileName + "' exists already (use overwrite=True to replace it)");

  // main header of the source in the representation of the target format
  CMedIOHeader* mainHeader = nullptr;
  if(like.data()->readMainHeader(mainHeader, target) == false || mainHeader == nullptr) {
    delete mainHeader;
    raise(PyExc_RuntimeError, QString("unable to convert the main header of the source into data format %1").arg(static_cast<int>(target)));
  }
  std::unique_ptr<CMedIOHeader> mainHeaderGuard(mainHeader);

  CMedIOData* file = nullptr;
  switch(target) {
    case CMedIOData::ECAT:    file = new CECATFile(fileName, CECATMainHeader::ECAT7_Volume16); break;
    case CMedIOData::NIFTI:   file = new CNIFTIFile(fileName, CNIFTIMainHeader::NIFTI1); break;
    case CMedIOData::Philips: file = new CPhilipsFile(fileName, philipsType); break;
    default:
      raise(PyExc_ValueError, QString("files of data format %1 cannot be written").arg(static_cast<int>(target)));
  }
  std::unique_ptr<MedIOFile> result(new MedIOFile(file, fileName, true));

  if(file->open(QIODevice::WriteOnly) == false)
    raise(PyExc_OSError, "unable to create '" + fileName + "'");

  if(writeMainHeader(file, mainHeader, target, philipsType) == false) {
    // do not leave a partial file behind
    result->close();
    QFile::remove(fileName);
    raise(PyExc_RuntimeError, "unable to write the main header of '" + fileName + "'");
  }

  return result;
}

} // namespace

//=============================================================================================

void init_volume(py::module_& m)
{
  // the data formats (shared with MedIOImage.FormatType)
  m.attr("Format") = m.attr("MedIOImage").attr("FormatType");

  //-------------------------------------------------------------------------------------------
  // pmedio.Volume (CMedIOVolume)
  py::class_<CMedIOVolume> volume(m, "Volume", R"doc(
One frame of image data, independent of the file format (CMedIOVolume).

- ``stored()``: the stored voxel values as numpy array of shape (nx, ny, nz)
- ``values()``: the physical values, stored value x scale factor of the plane + intercept
- ``affine``: 3 x 4 transformation of voxel (i, j, k), 0-based, into DICOM patient
  coordinates (LPS, mm: x towards the patient's left, y towards posterior, z towards superior)
- ``frame_start``, ``frame_duration``: frame timing (s)

Volumes read with ``File.read_volume()`` are in the voxel order of the file, e.g. "LPI"
for ECAT7, Philips and microPET (x towards left, y towards posterior, planes from superior
to inferior). ``reorient()`` reorders the voxels, every voxel keeps its position.
)doc");

  volume.def(py::init<>())
    .def("__init__", [](CMedIOVolume* self, const py::object& data, const py::object& affine, double scaleFactor,
                        double intercept, double frameStart, double frameDuration) {
           new (self) CMedIOVolume(volumeFromArray(data, affine, scaleFactor, intercept, frameStart, frameDuration));
         },
         py::arg("data"), py::arg("affine").none() = py::none(), py::arg("scale_factor") = 1.0, py::arg("intercept") = 0.0,
         py::arg("frame_start") = 0.0, py::arg("frame_duration") = 0.0,
         py::sig("def __init__(self, data: numpy.typing.ArrayLike, affine: numpy.typing.ArrayLike | None = None, "
                 "scale_factor: float = 1.0, intercept: float = 0.0, frame_start: float = 0.0, "
                 "frame_duration: float = 0.0) -> None"), R"doc(
Volume from the stored values ``data`` (3D array (x, y, z) of uint8, int8, int16, uint16,
int32, uint32, float32 or float64) with the 3 x 4 (or 4 x 4) LPS affine (default: identity),
scale factor, intercept and frame timing (s).
)doc")

    .def_prop_ro("shape", [](const CMedIOVolume& self) {
      return py::make_tuple(self.dimension(0), self.dimension(1), self.dimension(2));
    }, py::sig("def shape(self, /) -> tuple[int, int, int]"), "dimensions (nx, ny, nz)")
    .def_prop_ro("dtype", [](const CMedIOVolume& self) {
      return numpyType(self.dataType());
    }, py::sig("def dtype(self, /) -> numpy.dtype[typing.Any]"), "numpy data type of the stored values")
    .def("__len__", [](const CMedIOVolume& self) {
      return static_cast<Py_ssize_t>(self.voxelCount());
    })
    .def("__bool__", [](const CMedIOVolume& self) {
      return self.isEmpty() == false;
    })

    .def("stored", &storedArray, py::sig("def stored(self) -> numpy.typing.NDArray[typing.Any]"),
         "the stored values as numpy array of shape (nx, ny, nz) (a copy)")
    .def("values", [](const CMedIOVolume& self) {
      CMedIOVolume physical(self);
      if(physical.convertToPhysicalValues() == false)
        raise(PyExc_ValueError, "the physical values are larger than 2 GiB");
      return storedArray(physical);
    }, py::sig("def values(self) -> numpy.typing.NDArray[numpy.float32] | numpy.typing.NDArray[numpy.float64]"),
       "the physical values as float32 array (float64 for float64 data) of shape (nx, ny, nz)")
    .def("convert_to_physical_values", [](CMedIOVolume& self) {
      if(self.convertToPhysicalValues() == false)
        raise(PyExc_ValueError, "the physical values are larger than 2 GiB");
    }, "store the physical values (float32, float64 for float64 data) with scale factor 1 and intercept 0")

    .def_prop_rw("scale_factor",
      [](const CMedIOVolume& self) {
        if(self.hasUniformScaleFactor() == false)
          raise(PyExc_ValueError, "the planes have different scale factors, see scale_factors");
        return self.scaleFactor();
      },
      [](CMedIOVolume& self, double factor) { self.setScaleFactor(factor); },
      "scale factor of all planes (setting it sets all planes)")
    .def_prop_ro("scale_factors", [](const CMedIOVolume& self) {
      std::vector<double> factors;
      for(int k=0; k < self.dimension(2); k++)
        factors.push_back(self.scaleFactor(k));
      return factors;
    }, "scale factor of every plane")
    .def("set_scale_factor", [](CMedIOVolume& self, int plane, double factor) {
      if(plane < 0 || plane >= self.dimension(2))
        raise(PyExc_IndexError, QString("plane %1 does not exist (%2 planes)").arg(plane).arg(self.dimension(2)));
      self.setScaleFactor(plane, factor);
    }, py::arg("plane"), py::arg("factor"), "set the scale factor of a plane (0-based)")
    .def_prop_rw("intercept", &CMedIOVolume::intercept, &CMedIOVolume::setIntercept,
                  "physical value = stored value x scale factor + intercept")

    .def_prop_rw("affine",
      [](const CMedIOVolume& self) {
        double* w = new double[12];
        for(int i=0; i < 3; i++)
          for(int j=0; j < 4; j++)
            w[i * 4 + j] = self.affine(i, j);
        py::capsule owner(w, [](void* p) noexcept { delete[] static_cast<double*>(p); });
        return py::ndarray<py::numpy, double, py::c_contig>(w, { 3, 4 }, owner);
      },
      [](CMedIOVolume& self, const py::object& affine) {
        double m[3][4];
        affineOf(affine, m);
        self.setAffine(m);
      },
      py::rv_policy::move,
      // None is reported like every other object that is no affine
      py::for_setter(py::arg("affine").none()),
      py::for_setter(py::sig("def affine(self, affine: numpy.typing.ArrayLike) -> None")),
      "3 x 4 transformation of voxel (i, j, k) into DICOM patient coordinates (LPS, mm); "
      "a 4 x 4 matrix with the last row 0 0 0 1 is accepted as well")
    .def("position", [](const CMedIOVolume& self, double i, double j, double k) {
      double p[3];
      self.position(i, j, k, p);
      return py::make_tuple(p[0], p[1], p[2]);
    }, py::arg("i"), py::arg("j"), py::arg("k"),
       py::sig("def position(self, i: float, j: float, k: float) -> tuple[float, float, float]"),
       "position (LPS, mm) of voxel (i, j, k), 0-based")
    .def_prop_ro("voxel_size", [](const CMedIOVolume& self) {
      return py::make_tuple(self.voxelSize(0), self.voxelSize(1), self.voxelSize(2));
    }, py::sig("def voxel_size(self, /) -> tuple[float, float, float]"), "voxel size (mm) along the voxel axes")
    .def_prop_ro("orientation", [](const CMedIOVolume& self) {
      return self.orientation();
    }, "patient direction of every voxel axis, e.g. 'LPI' or 'RAS'")
    .def("reorient", [](CMedIOVolume& self, const QString& orientation) {
      bool oblique = false;
      if(self.reorient(orientation, &oblique) == false)
        raise(PyExc_ValueError, "invalid orientation '" + orientation + "' (three of L/R, P/A, I/S, e.g. 'RAS')");
      return oblique;
    }, py::arg("orientation"), R"doc(
Reorder the voxels so that the voxel axes point to the patient directions of the
orientation code (e.g. "RAS", the nibabel convention, or "LPI"). The affine is adjusted, so
every voxel keeps its position. Returns True if oblique axes were assigned to the closest
patient axis.
)doc")

    .def_prop_rw("frame_start", &CMedIOVolume::frameStart, &CMedIOVolume::setFrameStart, "frame start (s)")
    .def_prop_rw("frame_duration", &CMedIOVolume::frameDuration, &CMedIOVolume::setFrameDuration, "frame duration (s)")

    .def("__repr__", [](const CMedIOVolume& self) {
      if(self.isEmpty())
        return std::string("<pmedio.Volume (empty)>");
      return QString("<pmedio.Volume %1x%2x%3 %4 %5>").arg(self.dimension(0)).arg(self.dimension(1)).arg(self.dimension(2))
               .arg(numpyTypeName(self.dataType())).arg(self.orientation()).toStdString();
    });

  //-------------------------------------------------------------------------------------------
  // pmedio.File (CMedIOData)
  py::class_<MedIOFile>(m, "File", R"doc(
An image file of any supported format, opened with ``pmedio.open()`` for reading or created
with ``pmedio.create()`` for writing. It can be used as context manager.
)doc")
    .def_prop_ro("format", [](const MedIOFile& self) {
      return self.data()->dataFormat();
    }, "data format of the file (pmedio.Format)")
    .def_prop_ro("filename", &MedIOFile::fileName, "absolute file name")
    .def_prop_ro("writable", &MedIOFile::writable, "True for files created with pmedio.create()")
    .def_prop_ro("closed", &MedIOFile::closed)

    .def("frame_numbers", [](const MedIOFile& self) {
      const QList<short> frames = self.data()->frameNumbers();
      std::vector<short> numbers;
      for(short n : frames)
        numbers.push_back(n);
      return numbers;
    }, R"doc(
Numbers of the frames of the file in ascending order, as passed to ``read_volume()``.
ECAT7 and Philips files use the frame numbers stored in the file, which do not have to
start with 1; NIfTI volumes and microPET frames are numbered 1..n.
)doc")
    .def("read_volume", [](const MedIOFile& self, short frame) {
      if(self.writable())
        raise(PyExc_ValueError, "the file was created for writing");
      CMedIOVolume volume;
      if(self.data()->readVolume(volume, frame) == false)
        raise(PyExc_RuntimeError, QString("unable to read frame %1 of '%2'").arg(frame).arg(self.fileName()));
      return volume;
    }, py::arg("frame") = -1, "frame as pmedio.Volume in the voxel order of the file (-1: first frame)")
    .def("write_volume", [](const MedIOFile& self, const CMedIOVolume& volume, short frame) {
      if(self.writable() == false)
        raise(PyExc_ValueError, "the file was opened for reading, use pmedio.create()");
      if(self.data()->writeVolume(volume, frame) == false)
        raise(PyExc_RuntimeError, QString("unable to write frame %1 into '%2'").arg(frame).arg(self.fileName()));
    }, py::arg("volume"), py::arg("frame") = -1, R"doc(
Write the volume as frame of the file (-1: frame 1). The values are converted into the
representation of the file (e.g. the datatype and scaling of a NIfTI main header); it
fails without writing if they do not fit.
)doc")

    .def("attributes", [](const MedIOFile& self) {
      if(self.writable())
        raise(PyExc_ValueError, "the file was created for writing");

      // the NIfTI representation of the main header holds the frame attributes of all frames
      CMedIOHeader* header = nullptr;
      if(self.data()->readMainHeader(header, CMedIOData::NIFTI) == false || header == nullptr) {
        delete header;
        raise(PyExc_RuntimeError, "unable to read the header attributes of '" + self.fileName() + "'");
      }
      std::unique_ptr<CMedIOHeader> guard(header);

      QJsonObject json = header->attributes();
      if(self.data()->dataFormat() != CMedIOData::NIFTI)
        json.remove("NIfTI");
      const QByteArray text = QJsonDocument(json).toJson(QJsonDocument::Compact);
      return py::module_::import_("json").attr("loads")(py::str(text.constData(), text.size()));
    }, py::sig("def attributes(self) -> dict[str, typing.Any]"), R"doc(
Format independent header attributes of the file as dict (see CMedIOHeaderDictionary.h):
DICOM keywords with the units of DICOM (e.g. "PatientWeight" in kg), the native fields
of the format in a section named after it (e.g. "ECAT7") and in "Frames" the attributes
of every frame.
)doc")

    .def("close", &MedIOFile::close, "close the file (files created for writing are completed)")
    .def("__enter__", [](py::object self) { return self; }, py::sig("def __enter__(self) -> File"))
    .def("__exit__", [](MedIOFile& self, const py::object&, const py::object&, const py::object&) {
      self.close();
      return false;
    }, py::arg("type").none(), py::arg("value").none(), py::arg("traceback").none())
    .def("__repr__", [](const MedIOFile& self) {
      if(self.closed())
        return std::string("<pmedio.File (closed)>");
      return QString("<pmedio.File '%1' (%2)>").arg(self.fileName())
               .arg(self.writable() ? "writing" : "reading").toStdString();
    });

  m.def("open", [](const py::object& path) {
    const QString fileName = pathOf(path);
    if(QFileInfo::exists(fileName) == false)
      raise(PyExc_FileNotFoundError, "'" + fileName + "' does not exist");

    CMedIOData* data = CMedIODataFactory::create(fileName);
    if(data == nullptr)
      raise(PyExc_ValueError, "'" + fileName + "' is not a supported image file");

    std::unique_ptr<MedIOFile> file(new MedIOFile(data, fileName, false));
    if(data->open(QIODevice::ReadOnly) == false)
      raise(PyExc_OSError, "unable to open '" + fileName + "'");
    return file;
  }, py::arg("path"), py::sig("def open(path: str | os.PathLike[str]) -> File"), R"doc(
Open an ECAT7, NIfTI (.nii, .nii.gz, .hdr/.img), Philips (.img, .syn) or microPET file
for reading.
)doc");

  m.def("dictionary_entry", [](const QString& keywordOrTag) -> py::object {
    const CMedIOHeaderDictionary::Entry* entry = CMedIOHeaderDictionary::entry(keywordOrTag);
    if(entry == nullptr)
      return py::none();

    py::dict d;
    d["keyword"] = entry->keyword;
    d["tag"] = CMedIOHeaderDictionary::tagString(entry->tag).toStdString();
    d["vr"] = entry->vr;
    d["vm"] = entry->vm;
    d["unit"] = entry->unit;
    d["sequence"] = entry->sequence ? py::object(py::cast(CMedIOHeaderDictionary::tagString(entry->sequence)))
                                    : py::object(py::none());
    d["scope"] = (entry->scope == CMedIOHeaderDictionary::FrameHeader) ? "frame" : "main";
    return py::object(d);
  }, py::arg("keyword_or_tag"),
     py::sig("def dictionary_entry(keyword_or_tag: str) -> dict[str, typing.Any] | None"), R"doc(
Entry of the dictionary of the header attributes for a DICOM keyword ("PatientWeight")
or tag ("(0010,1030)"): keyword, tag, VR, VM, unit, sequence holding it and scope
("main" or "frame"); None if the dictionary has no entry for it.
)doc");

  m.def("create", &createFile, py::arg("path"), py::arg("like"), py::arg("format").none() = py::none(),
        py::arg("overwrite").none() = Flag{false},
        py::sig("def create(path: str | os.PathLike[str], like: File, format: str | None = None, "
                "overwrite: bool | None = False) -> File"), R"doc(
Create a file for writing with the main header of the file ``like`` (opened with
``pmedio.open()``), converted into the target format: number of frames, patient and
isotope data, calibration, and for NIfTI files the datatype, scaling and orientation.
The format is taken from the file name (.v: ECAT7, .nii/.nii.gz/.hdr: NIfTI-1,
.img/.syn: Philips image/Syntegra) unless ``format`` (pmedio.Format) is given. An
existing file is replaced only with ``overwrite=True``.
)doc");
}
