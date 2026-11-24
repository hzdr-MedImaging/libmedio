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
//   f = pmedio.open("image.nii.gz")          # ECAT7, NIfTI, Philips, Interfile, microPET
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

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QSet>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <algorithm>
#include <climits>
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

// absolute names of all files of a file (CMedIOData::fileNames(), e.g. both files of a
// NIfTI header/image pair)
QStringList absoluteFileNames(const CMedIOData* data)
{
  QStringList names;
  for(const QString& name : data->fileNames())
    names << QFileInfo(name).absoluteFilePath();
  return names;
}

class MedIOFile
{
  public:
    MedIOFile(CMedIOData* data, const QString& fileName, bool writable)
      : m_pData(data), m_fileName(QFileInfo(fileName).absoluteFilePath()), m_writable(writable),
        m_fileNames(writable ? absoluteFileNames(data) : QStringList()) { }
    ~MedIOFile() { close(); }

    // a file created for writing without a source file: its main header is made
    // from the first volume written (see writeVolume())
    struct Pending {
      CMedIOData::Format format;
      int frames;
      QJsonObject attributes;
    };

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

        // a file without any volume has no main header either, and a NIfTI file
        // without all of its volumes is incomplete: do not leave them behind
        const bool incomplete = (m_frames > 0 && m_written.size() < m_frames);
        if(m_pending || incomplete)
          removeFiles();
        if(incomplete && m_pending == nullptr)
          m_incomplete = QString("only %1 of the %2 frames of '%3' were written, the file was removed")
                         .arg(m_written.size()).arg(m_frames).arg(m_fileName);
      }
    }

    // the message of close() if it removed an incomplete NIfTI file (taken once)
    QString takeIncomplete() { const QString message = m_incomplete; m_incomplete.clear(); return message; }

    // frames written into a NIfTI file
    void written(short frame) { if(m_frames > 0) m_written.insert(frame == -1 ? 1 : frame); }

    void setPending(const Pending& pending)
    {
      m_pending.reset(new Pending(pending));
      if(pending.format == CMedIOData::NIFTI)
        m_frames = pending.frames;
    }
    // number of frames a file created from a source file needs (CMedIOData::requiredFrames())
    void setFrames(int frames) { m_frames = frames; }
    // number of frames of a NIfTI file, which needs all of them (0: any)
    int frames() const { return m_frames; }
    const Pending* pending() const { return m_pending.get(); }
    void setMainAttributes(const QJsonObject& attributes) { m_mainAttributes = attributes; m_pending.reset(); }
    const QJsonObject& mainAttributes() const { return m_mainAttributes; }

    // closes and removes a file whose main header could not be written
    void discard()
    {
      if(m_pData != nullptr) {
        m_pData->close();
        m_pData.reset();
      }
      m_pending.reset();
      removeFiles();
    }

  private:
    // removes all files of a file created for writing (both files of a NIfTI header/image pair)
    void removeFiles()
    {
      for(const QString& name : m_fileNames)
        QFile::remove(name);
    }

    std::unique_ptr<CMedIOData> m_pData;
    QString m_fileName;
    bool m_writable;
    QStringList m_fileNames;              // all files of a file created for writing
    std::unique_ptr<Pending> m_pending;   // main header not written yet
    int m_frames = 0;
    QSet<short> m_written;
    QString m_incomplete;
    QJsonObject m_mainAttributes;         // attributes of the main header written
};

// target format of a file created with create(): the format given or the one of the file name
CMedIOData::Format createFormat(const QString& fileName, const py::object& format)
{
  if(format.is_none() == false)
  {
    if(py::isinstance<py::str>(format) == false)
      throw std::runtime_error("the format has to be a str (pmedio.Format)");
    return formatOfName(py::cast<QString>(format));
  }

  const CMedIOData::Format result = CMedIODataFactory::formatOfFileName(fileName);
  if(result == CMedIOData::Unknown)
    raise(PyExc_ValueError, "unknown file format of '" + fileName + "' (.v: ECAT7, .nii/.nii.gz/.hdr: NIfTI, "
                            ".img/.syn: Philips); give the format explicitly");
  return result;
}

// header attributes given as dict (keys and values as returned by File.attributes())
QJsonObject attributesOf(const py::object& attributes)
{
  if(attributes.is_none())
    return QJsonObject();
  if(py::isinstance<py::dict>(attributes) == false)
    raise(PyExc_TypeError, "the attributes have to be a dict");

  // numpy scalars are given as the Python numbers they hold
  py::object item = py::cpp_function([](py::handle value) -> py::object {
    if(py::hasattr(value, "item"))
      return value.attr("item")();
    raise(PyExc_TypeError, QString("value of type %1 cannot be stored in a header")
                           .arg(py::cast<QString>(py::steal<py::object>(PyObject_Str(value.type().ptr())))));
  });
  const py::object text = py::module_::import_("json").attr("dumps")(attributes, py::arg("default") = item);
  const QByteArray utf8 = py::cast<QString>(text).toUtf8();
  return QJsonDocument::fromJson(utf8).object();
}

// apply the attributes to a main header; ValueError if one does not fit
void updateAttributes(CMedIOHeader* header, const QJsonObject& attributes)
{
  if(attributes.isEmpty() == false && header->updateAttributes(attributes) == false)
    raise(PyExc_ValueError, "invalid header attributes: a value does not fit its key or the format "
                            "(see pmedio.dictionary_entry() and File.attributes() of a file of the format)");
}

std::unique_ptr<MedIOFile> createFile(const py::object& path, const py::object& likeObject, const py::object& format,
                                      Flag overwrite, const py::object& framesObject, const py::object& attributesObject)
{
  const MedIOFile* like = nullptr;
  if(likeObject.is_none() == false) {
    if(py::isinstance<MedIOFile>(likeObject) == false)
      raise(PyExc_TypeError, "the source file 'like' has to be a pmedio.File opened with pmedio.open()");
    like = py::inst_ptr<MedIOFile>(likeObject);
    if(like->writable())
      raise(PyExc_ValueError, "the source file 'like' has to be opened for reading with pmedio.open()");
  }

  int frames = 1;
  if(framesObject.is_none() == false) {
    if(like != nullptr)
      raise(PyExc_ValueError, "the number of frames is taken from the source file 'like'");
    if(PyLong_Check(framesObject.ptr()) == false || PyBool_Check(framesObject.ptr()))
      raise(PyExc_TypeError, "the number of frames has to be an int");
    const long n = PyLong_AsLong(framesObject.ptr());
    if(n < 1 || n > SHRT_MAX) {
      PyErr_Clear();
      raise(PyExc_ValueError, QString("the number of frames has to be 1..%1").arg(SHRT_MAX));
    }
    frames = static_cast<int>(n);
  }
  const QJsonObject attributes = attributesOf(attributesObject);

  const QString fileName = pathOf(path);
  const CMedIOData::Format target = createFormat(fileName, format);

  // the file object (not opened yet); createForWriting() takes the format of the file name
  // for Unknown, which is no format to write when it is given explicitly
  std::unique_ptr<CMedIOData> file(target == CMedIOData::Unknown ? nullptr
                                   : CMedIODataFactory::createForWriting(fileName, target));
  if(file == nullptr)
    raise(PyExc_ValueError, QString("files of data format %1 cannot be written").arg(static_cast<int>(target)));

  // the new file replaces all of its files, e.g. also the image file of a NIfTI header/image pair
  if(overwrite == false) {
    const QStringList names = absoluteFileNames(file.get());
    for(int i=0; i < names.size(); i++) {
      if(QFileInfo::exists(names[i]) && i == 0)
        raise(PyExc_FileExistsError, "'" + fileName + "' exists already (use overwrite=True to replace it)");
      if(QFileInfo::exists(names[i]))
        raise(PyExc_FileExistsError, "'" + names[i] + "', the image file of '" + fileName + "', exists already "
                                     "(use overwrite=True to replace it)");
    }
  }

  // main header of the source in the representation of the target format, or (without a
  // source) a header of a volume of one voxel, which checks the attributes before the file
  // is created; the main header of the file is made from its first volume
  CMedIOHeader* mainHeader = nullptr;
  if(like != nullptr) {
    if(like->data()->readMainHeader(mainHeader, target) == false || mainHeader == nullptr) {
      delete mainHeader;
      raise(PyExc_RuntimeError, QString("unable to convert the main header of the source into data format %1").arg(static_cast<int>(target)));
    }
  } else {
    CMedIOVolume voxel;
    voxel.setData(1, 1, 1, CMedIOVolume::Float32, QByteArray(sizeof(float), '\0'));
    if(CMedIOData::mainHeaderFromVolume(voxel, target, frames, mainHeader) == false || mainHeader == nullptr) {
      delete mainHeader;
      raise(PyExc_RuntimeError, QString("unable to create a main header of data format %1").arg(static_cast<int>(target)));
    }
  }
  std::unique_ptr<CMedIOHeader> mainHeaderGuard(mainHeader);
  updateAttributes(mainHeader, attributes);

  CMedIOData* data = file.release();
  std::unique_ptr<MedIOFile> result(new MedIOFile(data, fileName, true));

  if(data->open(QIODevice::WriteOnly) == false)
    raise(PyExc_OSError, "unable to create '" + fileName + "'");

  if(like == nullptr) {
    result->setPending(MedIOFile::Pending{ target, frames, attributes });
    return result;
  }

  if(data->writeMainHeader(*mainHeader) == false) {
    // do not leave a partial file behind
    result->discard();
    raise(PyExc_RuntimeError, "unable to write the main header of '" + fileName + "'");
  }
  result->setMainAttributes(mainHeader->attributes());

  // a NIfTI file needs all frames of its main header (the frames of the source)
  result->setFrames(CMedIOData::requiredFrames(*mainHeader));

  return result;
}

// writes the main header of a file created without a source file, made from its first volume
void writePendingMainHeader(MedIOFile& file, const CMedIOVolume& volume)
{
  const MedIOFile::Pending pending = *file.pending();

  // A NIfTI file stores all frames with the data type and scaling of its main header:
  // a file of several frames keeps the physical values (float32, float64 for float64
  // data), as the frames can have other scale factors; a file of one frame keeps the
  // stored values of the volume.
  CMedIOVolume representative = volume;
  if(pending.format == CMedIOData::NIFTI && pending.frames > 1 && representative.convertToPhysicalValues() == false)
    raise(PyExc_ValueError, "the physical values of the volume are larger than 2 GiB");

  CMedIOHeader* header = nullptr;
  if(CMedIOData::mainHeaderFromVolume(representative, pending.format, pending.frames, header) == false || header == nullptr) {
    delete header;
    file.discard();
    raise(PyExc_RuntimeError, "unable to create the main header of '" + file.fileName() + "' from the volume");
  }
  std::unique_ptr<CMedIOHeader> guard(header);

  if((pending.attributes.isEmpty() == false && header->updateAttributes(pending.attributes) == false) ||
     file.data()->writeMainHeader(*header) == false) {
    file.discard();
    raise(PyExc_RuntimeError, "unable to write the main header of '" + file.fileName() + "'");
  }
  file.setMainAttributes(header->attributes());
}

// File.write_volume()
void writeVolume(MedIOFile& self, const CMedIOVolume& volume, short frame)
{
  if(self.writable() == false)
    raise(PyExc_ValueError, "the file was opened for reading, use pmedio.create()");
  if(self.frames() > 0 && (frame > self.frames() || frame < -1 || frame == 0))
    raise(PyExc_ValueError, QString("frame %1 does not exist: the NIfTI file was created for %2 frame(s) (the frames "
                                    "of the source file, or pmedio.create(..., frames=n))").arg(frame).arg(self.frames()));
  if(self.pending() != nullptr) {
    if(volume.isEmpty())
      raise(PyExc_ValueError, "the volume has no data");
    self.data();   // ValueError for a closed file
    writePendingMainHeader(self, volume);
  }
  if(self.data()->writeVolume(volume, frame) == false)
    raise(PyExc_RuntimeError, QString("unable to write frame %1 into '%2'").arg(frame).arg(self.fileName()));
  self.written(frame);
}

// File.attributes()
py::object fileAttributes(const MedIOFile& self)
{
  if(self.writable()) {
    self.data();   // ValueError for a closed file
    if(self.pending() != nullptr)
      raise(PyExc_ValueError, "the main header is made from the first volume written: no volume written yet");
    const QByteArray text = QJsonDocument(self.mainAttributes()).toJson(QJsonDocument::Compact);
    return py::module_::import_("json").attr("loads")(py::str(text.constData(), text.size()));
  }

  // the attributes of the main header and of all frames
  QJsonObject json;
  if(self.data()->readAttributes(json) == false)
    raise(PyExc_RuntimeError, "unable to read the header attributes of '" + self.fileName() + "'");
  const QByteArray text = QJsonDocument(json).toJson(QJsonDocument::Compact);
  return py::module_::import_("json").attr("loads")(py::str(text.constData(), text.size()));
}

// pmedio.open()
std::unique_ptr<MedIOFile> openFile(const py::object& path)
{
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
}

//---------------------------------------------------------------------------------------------
// pmedio.Image: all frames of an image file as one 4D array (load(), save())

struct Image
{
  py::object data;                     // ndarray (nx, ny, nz, nt)
  py::object affine;                   // ndarray 3 x 4 float64, LPS
  std::vector<double> frameStart;      // s, one per frame
  std::vector<double> frameDuration;   // s, one per frame
  py::object attributes;               // dict
  py::object format;                   // str, None for an image not loaded from a file
  py::object filename;                 // str, None for an image not loaded from a file
};

// 3 x 4 float64 array of an affine
py::object affineArray(const double m[3][4])
{
  py::object a = numpy().attr("empty")(py::make_tuple(3, 4), py::arg("dtype") = "float64");
  auto view = py::cast<py::ndarray<double, py::c_contig, py::device::cpu>>(a);
  for(int i=0; i < 3; i++)
    for(int j=0; j < 4; j++)
      view.data()[i * 4 + j] = m[i][j];
  return a;
}

py::object affineArray(const CMedIOVolume& volume)
{
  double m[3][4];
  for(int i=0; i < 3; i++)
    for(int j=0; j < 4; j++)
      m[i][j] = volume.affine(i, j);
  return affineArray(m);
}

// 4D voxel data of an image: a 3D array is one frame
py::object imageData(const py::object& data)
{
  py::object array = numpy().attr("asarray")(data);
  const size_t ndim = py::cast<size_t>(array.attr("ndim"));
  if(ndim == 3)
    array = numpy().attr("expand_dims")(array, 3);
  else if(ndim != 4)
    raise(PyExc_ValueError, QString("the voxel data has to be a 3D (x, y, z) or 4D (x, y, z, frame) array, not %1D").arg(ndim));
  if(volumeType(array.attr("dtype")) == CMedIOVolume::UnknownDataType)
    raise(PyExc_TypeError, QString("unsupported data type '%1': uint8, int8, int16, uint16, int32, uint32, float32 "
                                   "and float64 are supported").arg(py::cast<QString>(py::steal<py::object>(PyObject_Str(array.attr("dtype").ptr())))));
  const size_t frames = py::cast<size_t>(py::tuple(array.attr("shape"))[3]);
  if(frames < 1 || frames > SHRT_MAX)
    raise(PyExc_ValueError, QString("an image has 1..%1 frames, not %2").arg(SHRT_MAX).arg(frames));
  return array;
}

size_t imageFrames(const Image& image)
{
  return py::cast<size_t>(py::tuple(image.data.attr("shape"))[3]);
}

// frame start or duration of every frame (default: 0)
std::vector<double> imageTiming(const py::object& values, size_t frames, const char* name)
{
  if(values.is_none())
    return std::vector<double>(frames, 0.0);
  py::object array = numpy().attr("asarray")(values, py::arg("dtype") = "float64");
  if(py::cast<size_t>(array.attr("ndim")) != 1 || py::cast<size_t>(array.attr("size")) != frames)
    raise(PyExc_ValueError, QString("%1 has to give one value per frame (%2)").arg(name).arg(frames));
  return py::cast<std::vector<double>>(array.attr("tolist")());
}

py::object imageAttributes(const py::object& attributes)
{
  if(attributes.is_none())
    return py::dict();
  if(py::isinstance<py::dict>(attributes) == false)
    raise(PyExc_TypeError, "the attributes have to be a dict");
  return attributes.attr("copy")();
}

Image makeImage(const py::object& data, const py::object& affine, const py::object& frameStart,
                const py::object& frameDuration, const py::object& attributes)
{
  Image image;
  image.data = imageData(data);
  const size_t frames = imageFrames(image);

  double m[3][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 } };
  if(affine.is_none() == false)
    affineOf(affine, m);
  image.affine = affineArray(m);
  image.frameStart = imageTiming(frameStart, frames, "frame_start");
  image.frameDuration = imageTiming(frameDuration, frames, "frame_duration");
  image.attributes = imageAttributes(attributes);
  image.format = py::none();
  image.filename = py::none();
  return image;
}

//---------------------------------------------------------------------------------------------
// nibabel images: Image.to_nibabel(), Image.from_nibabel()
//
// nibabel uses RAS coordinates (x towards the patient's right, y towards anterior), pmedio
// DICOM patient coordinates (LPS): the affines differ in the sign of their first two rows.

// nibabel, an optional dependency of pmedio
py::module_ nibabel()
{
  try {
    return py::module_::import_("nibabel");
  } catch(py::python_error& e) {
    py::raise_from(e, PyExc_ImportError, "Image.to_nibabel() and Image.from_nibabel() need nibabel "
                   "(pip install nibabel)");
  }
}

// RAS <-> LPS: the first two rows of the affine change their sign
void flipLpsRas(double m[3][4])
{
  for(int i=0; i < 2; i++)
    for(int j=0; j < 4; j++)
      m[i][j] = -m[i][j];
}

// Image.to_nibabel()
py::object imageToNibabel(const Image& image)
{
  const py::module_ nib = nibabel();

  // 4 x 4 RAS affine
  double m[3][4];
  affineOf(image.affine, m);
  flipLpsRas(m);
  py::object affine = numpy().attr("eye")(4);
  auto view = py::cast<py::ndarray<double, py::c_contig, py::device::cpu>>(affine);
  for(int i=0; i < 3; i++)
    for(int j=0; j < 4; j++)
      view.data()[i * 4 + j] = m[i][j];

  // one frame: a 3D image, as NIfTI files of static images are
  const size_t frames = imageFrames(image);
  const py::object data = (frames == 1) ? image.data.attr("__getitem__")(py::make_tuple(py::ellipsis(), 0))
                                        : image.data;
  py::object result = nib.attr("Nifti1Image")(data, affine);

  // like the NIfTI files libmedio writes: scanner coordinates (qform and sform), mm and s,
  // and the frame duration as fourth voxel size if all frames have the same one
  result.attr("set_qform")(affine, py::arg("code") = 1);
  result.attr("set_sform")(affine, py::arg("code") = 1);
  py::object header = result.attr("header");
  header.attr("set_xyzt_units")("mm", "sec");
  if(frames > 1 && image.frameDuration.front() > 0.0 &&
     std::all_of(image.frameDuration.begin(), image.frameDuration.end(),
                 [&](double d) { return d == image.frameDuration.front(); })) {
    const py::tuple zooms(header.attr("get_zooms")());
    header.attr("set_zooms")(py::make_tuple(zooms[0], zooms[1], zooms[2], image.frameDuration.front()));
  }
  return result;
}

// Image.from_nibabel()
Image imageFromNibabel(const py::object& source, const py::object& frameStart, const py::object& frameDuration,
                       const py::object& attributes)
{
  if(py::hasattr(source, "dataobj") == false || py::hasattr(source, "affine") == false)
    raise(PyExc_TypeError, "the image has to be a nibabel image, e.g. returned by nibabel.load()");
  const py::object rasAffine = source.attr("affine");
  if(rasAffine.is_none())
    raise(PyExc_ValueError, "the nibabel image has no affine");

  // the values with the scaling of the image applied (the physical values)
  const py::object data = numpy().attr("asanyarray")(source.attr("dataobj"));

  double m[3][4];
  affineOf(rasAffine, m);
  flipLpsRas(m);
  return makeImage(data, affineArray(m), frameStart, frameDuration, attributes);
}

// frame k (0-based) as volume: the values of the image are the stored values (scale factor 1)
CMedIOVolume imageVolume(const Image& image, size_t k)
{
  const size_t frames = imageFrames(image);
  if(k >= frames)
    raise(PyExc_IndexError, QString("frame %1 does not exist (%2 frames)").arg(k).arg(frames));
  if(image.frameStart.size() != frames || image.frameDuration.size() != frames)
    raise(PyExc_ValueError, QString("frame_start and frame_duration have to give one value per frame (%1)").arg(frames));
  const py::object volume = image.data.attr("__getitem__")(py::make_tuple(py::ellipsis(), k));
  return volumeFromArray(volume, image.affine, 1.0, 0.0, image.frameStart[k], image.frameDuration[k]);
}

// pmedio.load()
Image loadImage(const py::object& path, const py::object& frames, const py::object& orientation)
{
  std::unique_ptr<MedIOFile> file = openFile(path);
  const QList<short> numbers = file->data()->frameNumbers();

  QList<short> selected = numbers;
  if(frames.is_none() == false) {
    selected.clear();
    for(const py::handle item : py::iter(frames)) {
      if(PyBool_Check(item.ptr()) || (PyLong_Check(item.ptr()) == false && py::hasattr(item, "__index__") == false))
        raise(PyExc_TypeError, "the frames have to be frame numbers (int)");
      const long n = PyLong_AsLong(item.ptr());
      if(PyErr_Occurred() || numbers.contains(static_cast<short>(n)) == false || n < SHRT_MIN || n > SHRT_MAX) {
        PyErr_Clear();
        raise(PyExc_ValueError, QString("frame %1 does not exist in '%2' (frames: %3)").arg(n).arg(file->fileName())
                                .arg(QStringList([&]() { QStringList l; for(short x : numbers) l << QString::number(x); return l; }()).join(", ")));
      }
      selected.append(static_cast<short>(n));
    }
  }
  if(selected.isEmpty())
    raise(PyExc_ValueError, "'" + file->fileName() + "' has no frames to read");

  // all frames, in the voxel order of the file or reoriented
  std::vector<CMedIOVolume> volumes;
  bool float64 = false;
  for(short n : selected) {
    CMedIOVolume volume;
    if(file->data()->readVolume(volume, n) == false)
      raise(PyExc_RuntimeError, QString("unable to read frame %1 of '%2'").arg(n).arg(file->fileName()));
    if(orientation.is_none() == false && volume.reorient(py::cast<QString>(orientation)) == false)
      raise(PyExc_ValueError, "invalid orientation '" + py::cast<QString>(orientation) + "' (e.g. \"RAS\" or \"LPI\")");

    // one 4D array: all frames have the same geometry
    if(volumes.empty() == false) {
      if(volumes.front().hasSameGeometry(volume) == false)
        raise(PyExc_ValueError, QString("frame %1 of '%2' has another geometry than frame %3: read the frames "
                                        "with pmedio.open()").arg(n).arg(file->fileName()).arg(selected.front()));
    }
    float64 = float64 || volume.dataType() == CMedIOVolume::Float64;
    volumes.push_back(volume);
  }

  // the physical values of all frames
  const CMedIOVolume& first = volumes.front();
  py::object data = numpy().attr("empty")(
    py::make_tuple(first.dimension(0), first.dimension(1), first.dimension(2), volumes.size()),
    py::arg("dtype") = float64 ? "float64" : "float32", py::arg("order") = "F");
  Image image;
  for(size_t k=0; k < volumes.size(); k++) {
    CMedIOVolume physical(volumes[k]);
    if(physical.convertToPhysicalValues() == false)
      raise(PyExc_ValueError, "the physical values of a frame are larger than 2 GiB");
    data.attr("__setitem__")(py::make_tuple(py::ellipsis(), k), storedArray(physical));
    image.frameStart.push_back(volumes[k].frameStart());
    image.frameDuration.push_back(volumes[k].frameDuration());
  }
  image.data = data;
  image.affine = affineArray(first);
  image.attributes = fileAttributes(*file);
  image.format = py::cast(file->data()->dataFormat());
  image.filename = py::cast(file->fileName());
  return image;
}

// pmedio.save() and Image.save(): nothing is left behind if writing fails
void saveImage(const py::object& path, const Image& image, const py::object& like, const py::object& format, Flag overwrite)
{
  const size_t frames = imageFrames(image);
  if(frames < 1 || frames > SHRT_MAX)
    raise(PyExc_ValueError, QString("an image has 1..%1 frames, not %2").arg(SHRT_MAX).arg(frames));
  if(image.frameStart.size() != frames || image.frameDuration.size() != frames)
    raise(PyExc_ValueError, QString("frame_start and frame_duration have to give one value per frame (%1)").arg(frames));

  // all volumes before the file is created, so that invalid data creates no file
  std::vector<CMedIOVolume> volumes;
  for(size_t k=0; k < frames; k++)
    volumes.push_back(imageVolume(image, k));

  const py::object number = like.is_none() ? py::object(py::int_(static_cast<long>(frames))) : py::none();
  std::unique_ptr<MedIOFile> file = createFile(path, like, format, overwrite, number, image.attributes);
  try {
    for(size_t k=0; k < frames; k++)
      writeVolume(*file, volumes[k], static_cast<short>(k + 1));
    file->close();
    const QString incomplete = file->takeIncomplete();
    if(incomplete.isEmpty() == false)
      raise(PyExc_ValueError, incomplete);
  } catch(...) {
    file->discard();
    throw;
  }
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
for ECAT7, Philips, microPET and Siemens Interfile (x towards left, y towards posterior,
planes from superior to inferior). ``reorient()`` reorders the voxels, every voxel keeps its position.
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
start with 1; NIfTI volumes, microPET and Interfile frames are numbered 1..n.
)doc")
    .def("read_volume", [](const MedIOFile& self, short frame) {
      if(self.writable())
        raise(PyExc_ValueError, "the file was created for writing");
      CMedIOVolume volume;
      if(self.data()->readVolume(volume, frame) == false)
        raise(PyExc_RuntimeError, QString("unable to read frame %1 of '%2'").arg(frame).arg(self.fileName()));
      return volume;
    }, py::arg("frame") = -1, "frame as pmedio.Volume in the voxel order of the file (-1: first frame)")
    .def("write_volume", &writeVolume, py::arg("volume"), py::arg("frame") = -1, R"doc(
Write the volume as frame of the file (-1: frame 1). The values are converted into the
representation of the file (e.g. the datatype and scaling of a NIfTI main header); it
fails without writing if they do not fit. The first volume written into a file created
without a source file gives the main header: geometry, and for NIfTI the datatype and
scaling (the physical values if the file has several frames).
)doc")

    .def("attributes", &fileAttributes, py::sig("def attributes(self) -> dict[str, typing.Any]"), R"doc(
Format independent header attributes of the file as dict (see CMedIOHeaderDictionary.h):
DICOM keywords with the units of DICOM (e.g. "PatientWeight" in kg), the native fields
of the format in a section named after it (e.g. "ECAT7") and in "Frames" the attributes
of every frame. For a file created for writing: the attributes of its main header as
written, without the frames.
)doc")

    .def("close", [](MedIOFile& self) {
      self.close();
      const QString incomplete = self.takeIncomplete();
      if(incomplete.isEmpty() == false)
        raise(PyExc_ValueError, incomplete);
    }, R"doc(
close the file (files created for writing are completed); a file created without a
source file is removed if no volume was written, a NIfTI file of several frames if not
all of them were written (ValueError)
)doc")
    .def("__enter__", [](py::object self) { return self; }, py::sig("def __enter__(self) -> File"))
    .def("__exit__", [](MedIOFile& self, const py::object& type, const py::object&, const py::object&) {
      self.close();
      // an incomplete file is reported unless an exception is on its way anyway
      const QString incomplete = self.takeIncomplete();
      if(incomplete.isEmpty() == false && type.is_none())
        raise(PyExc_ValueError, incomplete);
      // returns None: exceptions are never suppressed (which type checkers also see)
    }, py::arg("type").none(), py::arg("value").none(), py::arg("traceback").none())
    .def("__repr__", [](const MedIOFile& self) {
      if(self.closed())
        return std::string("<pmedio.File (closed)>");
      return QString("<pmedio.File '%1' (%2)>").arg(self.fileName())
               .arg(self.writable() ? "writing" : "reading").toStdString();
    });

  m.def("open", &openFile, py::arg("path"), py::sig("def open(path: str | os.PathLike[str]) -> File"), R"doc(
Open an ECAT7, NIfTI (.nii, .nii.gz, .hdr/.img), Philips (.img, .syn), Interfile (a
header, a Siemens main header .mhdr or a data file) or microPET file for reading.
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

  m.def("create", &createFile, py::arg("path"), py::arg("like").none() = py::none(),
        py::arg("format").none() = py::none(), py::arg("overwrite").none() = Flag{false},
        py::arg("frames").none() = py::none(), py::arg("attributes").none() = py::none(),
        py::sig("def create(path: str | os.PathLike[str], like: File | None = None, format: str | None = None, "
                "overwrite: bool | None = False, frames: int | None = None, "
                "attributes: dict[str, typing.Any] | None = None) -> File"), R"doc(
Create a file for writing. The format is taken from the file name (.v: ECAT7,
.nii/.nii.gz/.hdr: NIfTI-1, .img/.syn: Philips image/Syntegra) unless ``format``
(pmedio.Format) is given. An existing file is replaced only with ``overwrite=True``.

- With ``like`` (a file opened with ``pmedio.open()``) the file gets the main header of the
  source, converted into the target format: number of frames, patient and isotope data,
  calibration, and for NIfTI files the datatype, scaling and orientation.
- Without ``like`` the main header is made from the first volume written: geometry, and for
  NIfTI files the datatype and scaling. ``frames`` is the number of frames the file will
  hold (default 1; NIfTI files cannot hold more, the other formats take more frames
  anyway). A NIfTI file of several frames stores the physical values (float32, float64 for
  float64 data). A file closed without a volume is removed.

``attributes`` are header attributes to set, as returned by ``File.attributes()``: DICOM
keywords (e.g. ``{"PatientWeight": 72.5}``) or native fields in the section of the format.
Invalid values raise a ValueError before the file is created.
)doc");

  //-------------------------------------------------------------------------------------------
  // pmedio.Image, pmedio.load(), pmedio.save()
  py::class_<Image>(m, "Image", R"doc(
All frames of an image as one 4D array, independent of the file format: read with
``pmedio.load()``, written with ``pmedio.save()`` or ``Image.save()``.

- ``data``: the physical values as numpy array of shape (nx, ny, nz, frames), x running fastest
- ``affine``: 3 x 4 transformation of voxel (i, j, k), 0-based, into DICOM patient
  coordinates (LPS, mm), the same for all frames
- ``frame_start``, ``frame_duration``: timing of every frame (s)
- ``attributes``: header attributes as ``File.attributes()`` returns them (dict)
- ``format``, ``filename``: of the file the image was loaded from (None otherwise)
)doc")
    .def("__init__", [](Image* self, const py::object& data, const py::object& affine, const py::object& frameStart,
                        const py::object& frameDuration, const py::object& attributes) {
           new (self) Image(makeImage(data, affine, frameStart, frameDuration, attributes));
         },
         py::arg("data"), py::arg("affine").none() = py::none(), py::arg("frame_start").none() = py::none(),
         py::arg("frame_duration").none() = py::none(), py::arg("attributes").none() = py::none(),
         py::sig("def __init__(self, data: numpy.typing.ArrayLike, affine: numpy.typing.ArrayLike | None = None, "
                 "frame_start: collections.abc.Sequence[float] | None = None, "
                 "frame_duration: collections.abc.Sequence[float] | None = None, "
                 "attributes: dict[str, typing.Any] | None = None) -> None"), R"doc(
Image from voxel data (3D array for one frame or 4D array (x, y, z, frame) of uint8, int8,
int16, uint16, int32, uint32, float32 or float64, the physical values), the 3 x 4 (or 4 x 4)
LPS affine (default: identity), the timing of every frame (s, default: 0) and header
attributes.
)doc")
    .def_prop_rw("data",
      [](const Image& self) { return self.data; },
      [](Image& self, const py::object& data) {
        py::object array = imageData(data);
        const size_t frames = py::cast<size_t>(py::tuple(array.attr("shape"))[3]);
        if(frames != self.frameStart.size()) {
          // timing of other numbers of frames
          self.frameStart.assign(frames, 0.0);
          self.frameDuration.assign(frames, 0.0);
        }
        self.data = array;
      },
      py::for_getter(py::sig("def data(self, /) -> numpy.typing.NDArray[typing.Any]")),
      py::for_setter(py::sig("def data(self, data: numpy.typing.ArrayLike, /) -> None")),
      "the physical values (x, y, z, frame); setting data of another number of frames resets the timing")
    .def_prop_rw("affine",
      [](const Image& self) { return self.affine; },
      [](Image& self, const py::object& affine) {
        double m[3][4];
        affineOf(affine, m);
        self.affine = affineArray(m);
      },
      py::for_getter(py::sig("def affine(self, /) -> numpy.typing.NDArray[numpy.float64]")),
      py::for_setter(py::arg("affine").none()),
      py::for_setter(py::sig("def affine(self, affine: numpy.typing.ArrayLike, /) -> None")),
      "3 x 4 transformation of voxel (i, j, k) into DICOM patient coordinates (LPS, mm)")
    .def_prop_rw("frame_start",
      [](const Image& self) { return self.frameStart; },
      [](Image& self, const py::object& values) { self.frameStart = imageTiming(values, imageFrames(self), "frame_start"); },
      py::for_setter(py::sig("def frame_start(self, values: collections.abc.Sequence[float], /) -> None")),
      "start of every frame (s)")
    .def_prop_rw("frame_duration",
      [](const Image& self) { return self.frameDuration; },
      [](Image& self, const py::object& values) { self.frameDuration = imageTiming(values, imageFrames(self), "frame_duration"); },
      py::for_setter(py::sig("def frame_duration(self, values: collections.abc.Sequence[float], /) -> None")),
      "duration of every frame (s)")
    .def_prop_rw("attributes",
      [](const Image& self) { return self.attributes; },
      [](Image& self, const py::object& attributes) { self.attributes = imageAttributes(attributes); },
      py::for_getter(py::sig("def attributes(self, /) -> dict[str, typing.Any]")),
      py::for_setter(py::arg("attributes").none()),
      py::for_setter(py::sig("def attributes(self, attributes: dict[str, typing.Any] | None, /) -> None")),
      "header attributes as File.attributes() returns them, written by save()")
    .def_prop_ro("format", [](const Image& self) { return self.format; },
      py::sig("def format(self, /) -> str | None"), "data format of the file the image was loaded from (pmedio.Format)")
    .def_prop_ro("filename", [](const Image& self) { return self.filename; },
      py::sig("def filename(self, /) -> str | None"), "file the image was loaded from")
    .def_prop_ro("shape", [](const Image& self) { return py::tuple(self.data.attr("shape")); },
      py::sig("def shape(self, /) -> tuple[int, int, int, int]"), "dimensions (nx, ny, nz, frames)")
    .def("__len__", [](const Image& self) { return imageFrames(self); }, "number of frames")
    .def("volume", [](const Image& self, size_t frame) { return imageVolume(self, frame); }, py::arg("frame"),
      "frame (0-based) as pmedio.Volume")
    .def("save", [](const Image& self, const py::object& path, const py::object& like, const py::object& format, Flag overwrite) {
        saveImage(path, self, like, format, overwrite);
      }, py::arg("path"), py::arg("like").none() = py::none(), py::arg("format").none() = py::none(),
      py::arg("overwrite").none() = Flag{false},
      py::sig("def save(self, path: str | os.PathLike[str], like: File | None = None, format: str | None = None, "
              "overwrite: bool | None = False) -> None"), "write the image, see pmedio.save()")
    .def("to_nibabel", &imageToNibabel,
      py::sig("def to_nibabel(self) -> nibabel.nifti1.Nifti1Image"), R"doc(
The image as nibabel image (``nibabel.Nifti1Image``), e.g. to save it with nibabel or to
use it with tools that take nibabel images. The affine is converted into the RAS
coordinates of nibabel; the voxel order stays the same (use ``pmedio.load(...,
orientation="RAS")`` for the voxel order most NIfTI tools expect). An image of one frame
becomes a 3D image. The nibabel image shares the voxel data with this image. Frame timing
and header attributes are not part of it (``pmedio.save()`` writes them into NIfTI files);
the frame duration is its fourth voxel size if all frames have the same one. Needs nibabel.
)doc")
    .def_static("from_nibabel", &imageFromNibabel, py::arg("image"), py::arg("frame_start").none() = py::none(),
      py::arg("frame_duration").none() = py::none(), py::arg("attributes").none() = py::none(),
      py::sig("def from_nibabel(image: nibabel.spatialimages.SpatialImage, "
              "frame_start: collections.abc.Sequence[float] | None = None, "
              "frame_duration: collections.abc.Sequence[float] | None = None, "
              "attributes: dict[str, typing.Any] | None = None) -> Image"), R"doc(
Image from a nibabel image (e.g. ``nibabel.load()`` of a NIfTI or Analyze file, or an
image computed with nibabel) of 3D or 4D data: the values with the scaling of the image
applied, and its affine converted from RAS into DICOM patient coordinates (LPS). nibabel
images hold no frame timing or header attributes: give them as for ``pmedio.Image()``.
)doc")
    .def("__repr__", [](const Image& self) {
      const py::tuple shape(self.data.attr("shape"));
      QString text = QString("<pmedio.Image %1x%2x%3x%4 %5").arg(py::cast<long>(shape[0])).arg(py::cast<long>(shape[1]))
                       .arg(py::cast<long>(shape[2])).arg(py::cast<long>(shape[3]))
                       .arg(py::cast<QString>(py::steal<py::object>(PyObject_Str(self.data.attr("dtype").ptr()))));
      if(self.filename.is_none() == false)
        text += " '" + py::cast<QString>(self.filename) + "'";
      return (text + ">").toStdString();
    });

  m.def("load", &loadImage, py::arg("path"), py::arg("frames").none() = py::none(),
        py::arg("orientation").none() = py::none(),
        py::sig("def load(path: str | os.PathLike[str], frames: collections.abc.Sequence[int] | None = None, "
                "orientation: str | None = None) -> Image"), R"doc(
Load all frames of an ECAT7, NIfTI, Philips, Interfile or microPET file as pmedio.Image: the physical
values as one 4D array (float32, float64 for float64 data), affine, timing and header
attributes. ``frames`` selects frames by their numbers (``File.frame_numbers()``),
``orientation`` reorders the voxels (e.g. "RAS", the nibabel convention; default: the voxel
order of the file). All frames need the same geometry; read other files with pmedio.open().
)doc");

  m.def("save", [](const py::object& path, const py::object& image, const py::object& like,
                   const py::object& format, Flag overwrite) {
        if(py::isinstance<Image>(image))
          saveImage(path, py::cast<const Image&>(image), like, format, overwrite);
        else
          saveImage(path, makeImage(image, py::none(), py::none(), py::none(), py::none()), like, format, overwrite);
      }, py::arg("path"), py::arg("image"), py::arg("like").none() = py::none(), py::arg("format").none() = py::none(),
      py::arg("overwrite").none() = Flag{false},
      py::sig("def save(path: str | os.PathLike[str], image: Image | numpy.typing.ArrayLike, like: File | None = None, "
              "format: str | None = None, overwrite: bool | None = False) -> None"), R"doc(
Write an image (pmedio.Image, or an array with identity affine) into an ECAT7, NIfTI or Philips
file; the format is taken from the file name unless ``format`` is given. The main header is
made from the image (see pmedio.create()) or converted from ``like``; the attributes of the
image are set in it. Integer data is stored as integers, float data as float (NIfTI files of
several frames: float32/float64). Nothing is written if the image is invalid, and no file is
left behind if writing fails; an existing file is replaced only with ``overwrite=True``.
)doc");
}
