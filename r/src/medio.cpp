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

// Format independent access to images of all formats of libmedio for R: all frames
// of a file as one 4D array with affine, frame timing and header attributes, like
// pmedio.load()/pmedio.save() of the Python bindings.

#include <Rcpp.h>

#include <climits>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

#include <QByteArray>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QString>
#include <QStringList>

#include <CHDF5File>
#include <CMedIOData>
#include <CMedIODataFactory>
#include <CMedIOHeader>
#include <CMedIOVolume>

#include "medio.h"

namespace {

// error in R with a message of Qt
[[noreturn]] void fail(const QString& message)
{
  Rcpp::stop(message.toUtf8().constData());
}

QString stringOf(SEXP value)
{
  return QString::fromUtf8(Rcpp::as<std::string>(value).c_str());
}

SEXP stringValue(const QString& text)
{
  return Rcpp::CharacterVector::create(Rcpp::String(text.toUtf8().constData(), CE_UTF8));
}

// an open file, closed when it goes out of scope
struct OpenFile
{
  std::unique_ptr<CMedIOData> data;
  ~OpenFile() { if(data) data->close(); }
};

//---------------------------------------------------------------------------------------------
// header attributes: JSON (CMedIOHeader::attributes()) <-> R lists
//
// JSON objects are named lists, arrays unnamed lists, numbers, strings and booleans
// vectors of length 1 and null NULL. Written back, an unnamed list is an array and a
// vector of another length than 1 an array of its elements, so the attributes read
// keep their structure.

SEXP fromJson(const QJsonValue& value);

Rcpp::List fromJsonObject(const QJsonObject& object)
{
  Rcpp::List list(object.size());
  Rcpp::CharacterVector names(object.size());
  int i = 0;
  for(QJsonObject::const_iterator it = object.begin(); it != object.end(); ++it, ++i) {
    list[i] = fromJson(it.value());
    names[i] = Rcpp::String(it.key().toUtf8().constData(), CE_UTF8);
  }
  list.attr("names") = names;
  return list;
}

SEXP fromJson(const QJsonValue& value)
{
  switch(value.type()) {
    case QJsonValue::Bool:   return Rcpp::LogicalVector::create(value.toBool());
    case QJsonValue::Double: return Rcpp::NumericVector::create(value.toDouble());
    case QJsonValue::String: return stringValue(value.toString());
    case QJsonValue::Object: return fromJsonObject(value.toObject());
    case QJsonValue::Array: {
      const QJsonArray array = value.toArray();
      Rcpp::List list(array.size());
      for(int i=0; i < array.size(); i++)
        list[i] = fromJson(array.at(i));
      return list;
    }
    default:                 return R_NilValue;
  }
}

QJsonValue toJson(SEXP value, const QString& key);

// element i of an atomic vector as JSON value
QJsonValue toJsonElement(SEXP value, R_xlen_t i, const QString& key)
{
  switch(TYPEOF(value)) {
    case LGLSXP:
      return LOGICAL(value)[i] == NA_LOGICAL ? QJsonValue() : QJsonValue(LOGICAL(value)[i] != 0);
    case INTSXP:
      return INTEGER(value)[i] == NA_INTEGER ? QJsonValue() : QJsonValue(INTEGER(value)[i]);
    case REALSXP: {
      const double v = REAL(value)[i];
      if(std::isnan(v))
        return QJsonValue();
      if(std::isinf(v))
        fail("the attribute '" + key + "' is infinite, which cannot be stored in a header");
      return QJsonValue(v);
    }
    case STRSXP:
      if(STRING_ELT(value, i) == NA_STRING)
        return QJsonValue();
      return QJsonValue(QString::fromUtf8(Rf_translateCharUTF8(STRING_ELT(value, i))));
    default:
      fail("the attribute '" + key + "' has a type that cannot be stored in a header");
  }
}

QJsonObject toJsonObject(SEXP list, const QString& key)
{
  QJsonObject object;
  const SEXP names = Rf_getAttrib(list, R_NamesSymbol);
  for(R_xlen_t i=0; i < Rf_xlength(list); i++) {
    const QString name = QString::fromUtf8(Rf_translateCharUTF8(STRING_ELT(names, i)));
    if(STRING_ELT(names, i) == NA_STRING || name.isEmpty())
      fail(QString("all elements of the list%1 of attributes have to be named")
            .arg(key.isEmpty() ? QString() : " '" + key + "'"));
    object.insert(name, toJson(VECTOR_ELT(list, i), key.isEmpty() ? name : key + "/" + name));
  }
  return object;
}

QJsonValue toJson(SEXP value, const QString& key)
{
  if(Rf_isNull(value))
    return QJsonValue();
  if(Rf_isFactor(value))
    fail("the attribute '" + key + "' is a factor: convert it with as.character()");

  if(TYPEOF(value) == VECSXP) {
    if(Rf_isNull(Rf_getAttrib(value, R_NamesSymbol)) == false)
      return toJsonObject(value, key);
    QJsonArray array;
    for(R_xlen_t i=0; i < Rf_xlength(value); i++)
      array.append(toJson(VECTOR_ELT(value, i), key));
    return array;
  }

  if(TYPEOF(value) != LGLSXP && TYPEOF(value) != INTSXP && TYPEOF(value) != REALSXP && TYPEOF(value) != STRSXP)
    fail("the attribute '" + key + "' has a type that cannot be stored in a header");
  if(Rf_xlength(value) == 1)
    return toJsonElement(value, 0, key);
  QJsonArray array;
  for(R_xlen_t i=0; i < Rf_xlength(value); i++)
    array.append(toJsonElement(value, i, key));
  return array;
}

//---------------------------------------------------------------------------------------------
// writing

// target format of a file: the format given (names of CMedIODataFactory::formatFromName())
// or the one of the file name
CMedIOData::Format targetFormat(const QString& fileName, SEXP format)
{
  CMedIOData::Format result = CMedIOData::Unknown;
  if(Rf_isNull(format) == false) {
    if(CMedIODataFactory::formatFromName(stringOf(format), result) == false)
      fail("unknown format '" + stringOf(format) + "' (\"ecat\", \"nifti\", \"philips\" or \"interfile\")");
  } else {
    result = CMedIODataFactory::formatOfFileName(fileName);
    if(result == CMedIOData::Unknown)
      fail("unknown file format of '" + fileName + "' (.v: ECAT7, .nii/.nii.gz/.hdr: NIfTI, "
           ".img/.syn: Philips, .hv: Interfile); give the format explicitly");
  }

  if(result != CMedIOData::ECAT && result != CMedIOData::NIFTI && result != CMedIOData::Philips &&
     result != CMedIOData::Interfile)
    fail(QString("files of format '%1' cannot be written (formats: \"ecat\", \"nifti\", \"philips\", \"interfile\")")
         .arg(CMedIODataFactory::formatName(result)));
  return result;
}

} // namespace

//---------------------------------------------------------------------------------------------
// readMedio()

RcppExport SEXP readMedio(SEXP fileName, SEXP frames, SEXP orientation)
{
  BEGIN_RCPP

  const QString name = stringOf(fileName);
  if(QFileInfo::exists(name) == false)
    fail("'" + name + "' does not exist");

  OpenFile file;
  file.data.reset(CMedIODataFactory::create(name));
  if(file.data == nullptr)
    fail("'" + name + "' is not a supported image file");
  if(file.data->open(QIODevice::ReadOnly) == false) {
    // HDF5 files of another layout, or a build without HDF5 support
    if(file.data->dataFormat() == CMedIOData::HDF5)
      fail("'" + name + "' is an HDF5 file: " + static_cast<CHDF5File*>(file.data.get())->openError());
    fail("unable to open '" + name + "'");
  }

  // the frames to read, by their numbers
  const QList<short> numbers = file.data->frameNumbers();
  QList<short> selected = numbers;
  if(Rf_isNull(frames) == false) {
    selected.clear();
    const Rcpp::IntegerVector requested(frames);
    for(R_xlen_t i=0; i < requested.size(); i++) {
      const int n = requested[i];
      if(n == NA_INTEGER || n < SHRT_MIN || n > SHRT_MAX || numbers.contains(static_cast<short>(n)) == false) {
        QStringList available;
        for(short x : numbers)
          available << QString::number(x);
        fail(QString("frame %1 does not exist in '%2' (frames: %3)")
              .arg(n == NA_INTEGER ? QString("NA") : QString::number(n)).arg(name).arg(available.join(", ")));
      }
      selected.append(static_cast<short>(n));
    }
  }
  if(selected.isEmpty())
    fail("'" + name + "' has no frames to read");

  // all frames, in the voxel order of the file or reoriented; they need the same geometry
  std::vector<CMedIOVolume> volumes;
  for(short n : selected) {
    CMedIOVolume volume;
    if(file.data->readVolume(volume, n) == false)
      fail(QString("unable to read frame %1 of '%2'").arg(n).arg(name));
    if(Rf_isNull(orientation) == false && volume.reorient(stringOf(orientation)) == false)
      fail("invalid orientation '" + stringOf(orientation) + "' (e.g. \"RAS\" or \"LPI\")");

    if(volumes.empty() == false && volumes.front().hasSameGeometry(volume) == false)
      fail(QString("frame %1 of '%2' has another geometry than frame %3: read the frames one at a time")
           .arg(n).arg(name).arg(selected.front()));
    volumes.push_back(volume);
  }

  // the physical values of all frames as doubles, x running fastest
  const CMedIOVolume& first = volumes.front();
  const R_xlen_t voxels = static_cast<R_xlen_t>(first.voxelCount());
  Rcpp::NumericVector data(voxels * static_cast<R_xlen_t>(volumes.size()));
  Rcpp::NumericVector frameStart(volumes.size()), frameDuration(volumes.size());
  for(size_t k=0; k < volumes.size(); k++) {
    double* values = data.begin() + voxels * static_cast<R_xlen_t>(k);
    for(R_xlen_t i=0; i < voxels; i++)
      values[i] = volumes[k].value(i);
    frameStart[k] = volumes[k].frameStart();
    frameDuration[k] = volumes[k].frameDuration();
  }
  data.attr("dim") = Rcpp::IntegerVector::create(first.dimension(0), first.dimension(1), first.dimension(2),
                                                 static_cast<int>(volumes.size()));

  Rcpp::NumericMatrix affine(3, 4);
  for(int i=0; i < 3; i++)
    for(int j=0; j < 4; j++)
      affine(i, j) = first.affine(i, j);

  // the attributes of the main header and of all frames
  QJsonObject attributes;
  if(file.data->readAttributes(attributes) == false)
    fail("unable to read the header attributes of '" + name + "'");

  return Rcpp::List::create(Rcpp::Named("data") = data,
                            Rcpp::Named("affine") = affine,
                            Rcpp::Named("frame_start") = frameStart,
                            Rcpp::Named("frame_duration") = frameDuration,
                            Rcpp::Named("attributes") = fromJsonObject(attributes),
                            Rcpp::Named("format") = CMedIODataFactory::formatName(file.data->dataFormat()),
                            Rcpp::Named("filename") = stringValue(QFileInfo(name).absoluteFilePath()));

  END_RCPP
}

//---------------------------------------------------------------------------------------------
// writeMedio()

RcppExport SEXP writeMedio(SEXP fileName, SEXP data, SEXP affine, SEXP frameStart, SEXP frameDuration,
                           SEXP attributes, SEXP like, SEXP format, SEXP overwrite)
{
  BEGIN_RCPP

  const QString name = stringOf(fileName);
  const CMedIOData::Format target = targetFormat(name, format);

  // the file object (not opened yet); the new file replaces all of its files, e.g. also the
  // image file of a NIfTI header/image pair
  std::unique_ptr<CMedIOData> newFile(CMedIODataFactory::createForWriting(name, target));
  if(newFile == nullptr)
    fail(QString("files of format '%1' cannot be written").arg(CMedIODataFactory::formatName(target)));
  if(Rcpp::as<bool>(overwrite) == false) {
    const QStringList names = newFile->fileNames();
    for(int i=0; i < names.size(); i++) {
      if(QFileInfo::exists(names[i]) && i == 0)
        fail("'" + name + "' exists already (use overwrite = TRUE to replace it)");
      if(QFileInfo::exists(names[i]))
        fail("'" + names[i] + "', the image file of '" + name + "', exists already (use overwrite = TRUE to replace it)");
    }
  }

  // the volumes of all frames before the file is created: the values as given, as
  // float32 (double data) or int32 (integer data) with scale factor 1
  const Rcpp::IntegerVector dim(Rf_getAttrib(data, R_DimSymbol));
  const int frames = dim[3];
  const R_xlen_t voxels = static_cast<R_xlen_t>(dim[0]) * dim[1] * dim[2];
  const Rcpp::NumericMatrix m(affine);
  double geometry[3][4];
  for(int i=0; i < 3; i++)
    for(int j=0; j < 4; j++)
      geometry[i][j] = m(i, j);
  const Rcpp::NumericVector start(frameStart), duration(frameDuration);

  if(voxels * static_cast<R_xlen_t>(sizeof(float)) > INT_MAX)
    fail("a frame of the image is too large (more than 2 GiB)");

  std::vector<CMedIOVolume> volumes(frames);
  for(int k=0; k < frames; k++) {
    QByteArray bytes;
    CMedIOVolume::DataType type;
    if(TYPEOF(data) == INTSXP) {
      const int* values = INTEGER(data) + voxels * k;
      for(R_xlen_t i=0; i < voxels; i++)
        if(values[i] == NA_INTEGER)
          fail("integer data with NA cannot be stored: convert it to double (NA is stored as NaN)");
      type = CMedIOVolume::Int32;
      bytes = QByteArray(reinterpret_cast<const char*>(values), static_cast<int>(voxels * sizeof(qint32)));
    } else {
      const double* values = REAL(data) + voxels * k;
      type = CMedIOVolume::Float32;
      bytes.resize(static_cast<int>(voxels * sizeof(float)));
      float* stored = reinterpret_cast<float*>(bytes.data());
      for(R_xlen_t i=0; i < voxels; i++)
        stored[i] = static_cast<float>(values[i]);
    }
    if(volumes[k].setData(dim[0], dim[1], dim[2], type, bytes) == false)
      fail("invalid voxel data");
    volumes[k].setAffine(geometry);
    volumes[k].setFrameStart(start[k]);
    volumes[k].setFrameDuration(duration[k]);
  }

  // main header of the source file in the representation of the target format, or made
  // from the first volume; the attributes are checked before the file is created
  CMedIOHeader* header = nullptr;
  if(Rf_isNull(like) == false) {
    const QString source = stringOf(like);
    OpenFile likeFile;
    likeFile.data.reset(QFileInfo::exists(source) ? CMedIODataFactory::create(source) : nullptr);
    if(likeFile.data == nullptr || likeFile.data->open(QIODevice::ReadOnly) == false)
      fail("unable to open the source file '" + source + "'");
    if(likeFile.data->readMainHeader(header, target) == false || header == nullptr) {
      delete header;
      fail(QString("unable to convert the main header of '%1' into format '%2'").arg(source)
           .arg(CMedIODataFactory::formatName(target)));
    }
    // a NIfTI file needs all frames of its main header
    const int sourceFrames = CMedIOData::requiredFrames(*header);
    if(sourceFrames > 0 && sourceFrames != frames) {
      delete header;
      fail(QString("the image has %1 frame(s), the NIfTI file made from '%2' needs %3").arg(frames).arg(source).arg(sourceFrames));
    }
  } else if(CMedIOData::mainHeaderFromVolume(volumes.front(), target, frames, header) == false || header == nullptr) {
    delete header;
    fail(QString("unable to create a main header of format '%1' from the image").arg(CMedIODataFactory::formatName(target)));
  }
  std::unique_ptr<CMedIOHeader> headerGuard(header);

  const QJsonObject json = toJsonObject(attributes, QString());
  if(json.isEmpty() == false && header->updateAttributes(json) == false)
    fail("invalid header attributes: a value does not fit its key or the format (see the attributes of a "
          "file of the format read with readMedio())");

  // the file; nothing is left behind if writing fails
  OpenFile file;
  file.data = std::move(newFile);
  if(file.data->open(QIODevice::WriteOnly) == false)
    fail("unable to create '" + name + "'");
  try {
    if(file.data->writeMainHeader(*header) == false)
      fail("unable to write the main header of '" + name + "'");
    for(int k=0; k < frames; k++)
      if(file.data->writeVolume(volumes[k], static_cast<short>(k + 1)) == false)
        fail(QString("unable to write frame %1 into '%2'").arg(k + 1).arg(name));
    file.data->close();
    file.data.reset();
  } catch(...) {
    if(file.data) {
      file.data->close();
      file.data->removeFiles();
      file.data.reset();
    }
    throw;
  }

  return stringValue(QFileInfo(name).absoluteFilePath());

  END_RCPP
}
