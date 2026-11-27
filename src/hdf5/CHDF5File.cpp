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

//! @file CHDF5File.cpp
//! @brief contains the implementation of the class CHDF5File

#include "CHDF5File.h"
#include "CHDF5FrameHeader.h"
#include "CHDF5MainHeader.h"
#include "CMedIOHeaderDictionary.h"
#include "CMedIOHeaderMapping.h"
#include "MedIOLimits.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <QSet>
#include <QStringList>
#include <QVector>

#include <cmath>
#include <limits>

#include <rtdebug.h>

#ifdef MEDIO_HAVE_HDF5
#include <hdf5.h>
#endif

namespace {

// the signature of an HDF5 file (format specification: "\211HDF\r\n\032\n")
const char signature[] = { '\x89', 'H', 'D', 'F', '\r', '\n', '\x1a', '\n' };
const qint64 signatureSize = sizeof(signature);

// the signature is at the start of the file or, after a user block, at offset
// 512, 1024, 2048, ... (the size of a user block is a power of two from 512)
const qint64 firstUserBlockSize = 512;

// the layout read (docs/hdf5.md)
const char* const layoutName = "MEDIO-IMAGE";
const int layoutMajorVersion = 0;

// largest number of values of an attribute or of a dataset with one value per volume
const qint64 maxValues = 32767;

#ifdef MEDIO_HAVE_HDF5

// the HDF5 library is used by one thread at a time
Q_GLOBAL_STATIC(QMutex, hdf5Mutex)

// holds the lock of the HDF5 library and keeps it from printing its error stack
// (failing calls are expected when a file is examined); the error handler of the
// application is restored afterwards
class HDF5Lock
{
  public:
    HDF5Lock() : m_locker(hdf5Mutex()), m_function(NULL), m_data(NULL)
    {
      H5Eget_auto2(H5E_DEFAULT, &m_function, &m_data);
      H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
    }
    ~HDF5Lock()
    {
      H5Eset_auto2(H5E_DEFAULT, m_function, m_data);
    }

  private:
    HDF5Lock(const HDF5Lock&);
    HDF5Lock& operator=(const HDF5Lock&);

    QMutexLocker m_locker;
    H5E_auto2_t m_function;
    void* m_data;
};

// an identifier of the HDF5 library, closed with the object
class HDF5Id
{
  public:
    enum Kind { File, Group, Dataset, Space, Type, Attribute };

    HDF5Id(Kind kind, hid_t id) : m_kind(kind), m_id(id) { }
    ~HDF5Id() { close(); }

    operator hid_t() const { return m_id; }
    bool isValid() const { return m_id >= 0; }

    void close()
    {
      if(m_id < 0)
        return;
      switch(m_kind)
      {
        case File:      H5Fclose(m_id); break;
        case Group:     H5Gclose(m_id); break;
        case Dataset:   H5Dclose(m_id); break;
        case Space:     H5Sclose(m_id); break;
        case Type:      H5Tclose(m_id); break;
        case Attribute: H5Aclose(m_id); break;
      }
      m_id = -1;
    }

  private:
    HDF5Id(const HDF5Id&);
    HDF5Id& operator=(const HDF5Id&);

    Kind m_kind;
    hid_t m_id;
};

// true if every group of the path and the object itself exist
bool exists(hid_t location, const QByteArray& path)
{
  QByteArray partial;
  foreach(const QByteArray& name, path.split('/'))
  {
    partial += (partial.isEmpty() ? "" : "/") + name;
    if(H5Lexists(location, partial.constData(), H5P_DEFAULT) <= 0)
      return false;
  }
  return true;
}

// number of values of a dataspace: 1 for a scalar, the size of one dimension,
// -1 for other dataspaces; scalar is set accordingly
qint64 valueCount(hid_t space, bool& scalar)
{
  scalar = false;
  const H5S_class_t type = H5Sget_simple_extent_type(space);
  if(type == H5S_SCALAR)
  {
    scalar = true;
    return 1;
  }
  if(type != H5S_SIMPLE || H5Sget_simple_extent_ndims(space) != 1)
    return -1;

  hsize_t size = 0;
  if(H5Sget_simple_extent_dims(space, &size, NULL) != 1 || size > static_cast<hsize_t>(maxValues))
    return -1;
  return static_cast<qint64>(size);
}

void reclaim(hid_t type, hid_t space, void* buffer)
{
#if H5_VERSION_GE(1, 12, 0)
  H5Treclaim(type, space, H5P_DEFAULT, buffer);
#else
  H5Dvlen_reclaim(type, space, H5P_DEFAULT, buffer);
#endif
}

// the strings of an attribute or dataset (read: H5Aread or H5Dread), variable or
// fixed length, UTF-8 or ASCII
template<typename Read>
bool readStrings(hid_t fileType, hid_t space, qint64 count, Read read, QStringList& strings)
{
  HDF5Id memoryType(HDF5Id::Type, H5Tcopy(H5T_C_S1));
  if(memoryType.isValid() == false || H5Tset_cset(memoryType, H5Tget_cset(fileType)) < 0)
    return false;

  if(H5Tis_variable_str(fileType) > 0)
  {
    if(H5Tset_size(memoryType, H5T_VARIABLE) < 0)
      return false;
    QVector<char*> buffer(static_cast<int>(count), NULL);
    if(read(memoryType, buffer.data()) < 0)
      return false;
    for(int i = 0; i < buffer.size(); i++)
      strings.append(QString::fromUtf8(buffer[i]));
    reclaim(memoryType, space, buffer.data());
    return true;
  }

  // fixed length: with room for the terminating zero of strings padded otherwise
  const size_t size = H5Tget_size(fileType);
  if(size == 0 || size > static_cast<size_t>(MedIOLimits::MaxByteArraySize) / static_cast<size_t>(count) - 1 ||
     H5Tset_size(memoryType, size + 1) < 0 || H5Tset_strpad(memoryType, H5T_STR_NULLTERM) < 0)
    return false;
  QByteArray buffer(static_cast<int>(count * (size + 1)), '\0');
  if(read(memoryType, buffer.data()) < 0)
    return false;
  for(qint64 i = 0; i < count; i++)
  {
    const char* text = buffer.constData() + i * (size + 1);
    strings.append(QString::fromUtf8(text, static_cast<int>(qstrnlen(text, static_cast<uint>(size)))));
  }
  return true;
}

struct ReadAttribute
{
  hid_t id;
  herr_t operator()(hid_t type, void* buffer) const { return H5Aread(id, type, buffer); }
};

struct ReadDataset
{
  hid_t id;
  herr_t operator()(hid_t type, void* buffer) const { return H5Dread(id, type, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer); }
};

// value of an attribute: a string or number for a scalar, an array for one
// dimension, Undefined for anything else
QJsonValue attributeValue(hid_t object, const char* name)
{
  HDF5Id attribute(HDF5Id::Attribute, H5Aopen(object, name, H5P_DEFAULT));
  if(attribute.isValid() == false)
    return QJsonValue(QJsonValue::Undefined);
  HDF5Id type(HDF5Id::Type, H5Aget_type(attribute));
  HDF5Id space(HDF5Id::Space, H5Aget_space(attribute));
  bool scalar = false;
  const qint64 count = (type.isValid() && space.isValid()) ? valueCount(space, scalar) : -1;
  if(count < 0)
    return QJsonValue(QJsonValue::Undefined);

  QJsonArray values;
  const H5T_class_t typeClass = H5Tget_class(type);
  if(typeClass == H5T_STRING)
  {
    QStringList strings;
    const ReadAttribute read = { attribute };
    if(count > 0 && readStrings(type, space, count, read, strings) == false)
      return QJsonValue(QJsonValue::Undefined);
    foreach(const QString& string, strings)
      values.append(string);
  }
  else if(typeClass == H5T_INTEGER || typeClass == H5T_FLOAT)
  {
    QVector<double> numbers(static_cast<int>(count));
    if(count > 0 && H5Aread(attribute, H5T_NATIVE_DOUBLE, numbers.data()) < 0)
      return QJsonValue(QJsonValue::Undefined);
    foreach(double number, numbers)
    {
      if(std::isfinite(number) == false)   // no JSON value
        return QJsonValue(QJsonValue::Undefined);
      values.append(number);
    }
  }
  else
    return QJsonValue(QJsonValue::Undefined);

  if(scalar)
    return values.at(0);
  return values;
}

// string of a scalar string attribute (false if there is none)
bool stringAttribute(hid_t object, const char* name, QString& string)
{
  if(H5Aexists(object, name) <= 0)
    return false;
  const QJsonValue value = attributeValue(object, name);
  if(value.isString() == false)
    return false;
  string = value.toString();
  return true;
}

herr_t collectName(hid_t, const char* name, const H5A_info_t*, void* data)
{
  static_cast<QList<QByteArray>*>(data)->append(QByteArray(name));
  return 0;
}

// names of the attributes of an object
QList<QByteArray> attributeNames(hid_t object)
{
  QList<QByteArray> names;
  hsize_t index = 0;
  H5Aiterate2(object, H5_INDEX_NAME, H5_ITER_INC, &index, collectName, &names);
  return names;
}

// the values of a dataset of numbers with the given dimensions
bool readNumbers(hid_t file, const char* path, const QVector<hsize_t>& dimensions, QVector<double>& numbers)
{
  HDF5Id dataset(HDF5Id::Dataset, H5Dopen2(file, path, H5P_DEFAULT));
  if(dataset.isValid() == false)
    return false;
  HDF5Id type(HDF5Id::Type, H5Dget_type(dataset));
  HDF5Id space(HDF5Id::Space, H5Dget_space(dataset));
  if(type.isValid() == false || space.isValid() == false)
    return false;
  const H5T_class_t typeClass = H5Tget_class(type);
  if(typeClass != H5T_INTEGER && typeClass != H5T_FLOAT)
    return false;

  QVector<hsize_t> size(dimensions.size());
  if(H5Sget_simple_extent_type(space) != H5S_SIMPLE || H5Sget_simple_extent_ndims(space) != dimensions.size() ||
     H5Sget_simple_extent_dims(space, size.data(), NULL) != dimensions.size() || size != dimensions)
    return false;

  qint64 count = 1;
  foreach(hsize_t n, dimensions)
    count *= static_cast<qint64>(n);
  numbers.resize(static_cast<int>(count));
  return H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, numbers.data()) >= 0;
}

bool allFinite(const QVector<double>& numbers)
{
  foreach(double number, numbers)
  {
    if(std::isfinite(number) == false)
      return false;
  }
  return true;
}

#endif // MEDIO_HAVE_HDF5

} // namespace

class CHDF5FilePrivate
{
  public:
    CHDF5FilePrivate() { clear(); }

    void clear()
    {
#ifdef MEDIO_HAVE_HDF5
      file = -1;
      dataset = -1;
#endif
      version.clear();
      for(int i = 0; i < 3; i++)
        dimensions[i] = 0;
      dataType = CMedIOVolume::UnknownDataType;
      units.clear();
      affines.clear();
      frames.clear();
      gates.clear();
      beds.clear();
      starts.clear();
      durations.clear();
      frameValues.clear();
      mainAttributes = QJsonObject();
      frameObjects = QJsonArray();
      firstGate.clear();
    }

    int volumes() const { return frames.size(); }

#ifdef MEDIO_HAVE_HDF5
    bool read(QString& reason);
    bool readHeader(QString& reason);

    hid_t file;                          //!< the open file, -1 if none
    hid_t dataset;                       //!< /image/data
#endif

    QString error;                       //!< the reason why open() failed
    QString version;                     //!< FormatVersion
    int dimensions[3];                   //!< nx, ny, nz
    CMedIOVolume::DataType dataType;     //!< Float32 or Float64
    QString units;                       //!< DICOM Units of the values
    QVector<double> affines;             //!< 12 values per volume
    QVector<int> frames, gates, beds;    //!< index of the volumes
    QVector<double> starts, durations;   //!< frame timing (s)
    QMap<QString, QVector<double> > frameValues;   //!< further datasets of /image/volumes
    QJsonObject mainAttributes;          //!< attributes of the main header
    QJsonArray frameObjects;             //!< "Frames" of the JSON object, one entry per volume or empty
    QList<int> firstGate;                //!< volumes of the first gate and bed position
};

#ifdef MEDIO_HAVE_HDF5

// read the description of the image of an open file (the lock is held)
bool CHDF5FilePrivate::read(QString& reason)
{
  // the layout and its version
  QString format;
  if(stringAttribute(file, "Format", format) == false || format != QLatin1String(layoutName))
  {
    reason = QString("the HDF5 file has another layout than %1 (see docs/hdf5.md)").arg(layoutName);
    return false;
  }
  bool majorOk = false, minorOk = false;
  const bool hasVersion = stringAttribute(file, "FormatVersion", version);
  const QStringList parts = version.split('.');
  const int major = parts.value(0).toInt(&majorOk);
  parts.value(1).toInt(&minorOk);
  if(hasVersion == false || parts.size() != 2 || majorOk == false || minorOk == false)
  {
    reason = QString("invalid FormatVersion '%1'").arg(version);
    return false;
  }
  if(major != layoutMajorVersion)
  {
    reason = QString("version %1 of the layout %2 is not supported").arg(version, layoutName);
    return false;
  }

  // the voxels
  if(exists(file, "image/data") == false || (dataset = H5Dopen2(file, "image/data", H5P_DEFAULT)) < 0)
  {
    reason = "the dataset /image/data is missing";
    return false;
  }
  HDF5Id type(HDF5Id::Type, H5Dget_type(dataset));
  HDF5Id space(HDF5Id::Space, H5Dget_space(dataset));
  hsize_t size[4] = { 0, 0, 0, 0 };
  if(type.isValid() == false || space.isValid() == false || H5Sget_simple_extent_type(space) != H5S_SIMPLE ||
     H5Sget_simple_extent_ndims(space) != 4 || H5Sget_simple_extent_dims(space, size, NULL) != 4)
  {
    reason = "/image/data is no dataset with the dimensions [volume][z][y][x]";
    return false;
  }
  if(H5Tget_class(type) == H5T_FLOAT && H5Tget_size(type) == 4)
    dataType = CMedIOVolume::Float32;
  else if(H5Tget_class(type) == H5T_FLOAT && H5Tget_size(type) == 8)
    dataType = CMedIOVolume::Float64;
  else
  {
    reason = "the values of /image/data are neither float32 nor float64";
    return false;
  }
  const hsize_t limit = static_cast<hsize_t>(std::numeric_limits<short>::max());
  if(size[0] < 1 || size[0] > limit || size[1] < 1 || size[1] > limit || size[2] < 1 || size[2] > limit ||
     size[3] < 1 || size[3] > limit ||
     MedIOLimits::fitsByteArray(static_cast<qint64>(size[1] * size[2] * size[3]) *
                                CMedIOVolume::bytesPerVoxel(dataType)) == false)
  {
    reason = "the dimensions of /image/data are not supported";
    return false;
  }
  const int count = static_cast<int>(size[0]);
  dimensions[0] = static_cast<int>(size[3]);
  dimensions[1] = static_cast<int>(size[2]);
  dimensions[2] = static_cast<int>(size[1]);

  // the unit of the values: both attributes or none
  const bool hasUnits = stringAttribute(dataset, "Units", units);
  if(hasUnits != (H5Aexists(dataset, "units") > 0))
  {
    reason = "/image/data has only one of the attributes Units and units";
    return false;
  }

  // the position of the voxels
  QVector<double> affine;
  if(exists(file, "image/affine") == false ||
     readNumbers(file, "image/affine", QVector<hsize_t>() << 3 << 4, affine) == false || allFinite(affine) == false)
  {
    reason = "/image/affine is missing or no dataset of 3 x 4 numbers";
    return false;
  }
  if(exists(file, "image/volumes/affine"))
  {
    if(readNumbers(file, "image/volumes/affine", QVector<hsize_t>() << static_cast<hsize_t>(count) << 3 << 4, affines) == false ||
       allFinite(affines) == false)
    {
      reason = "/image/volumes/affine is no dataset of 3 x 4 numbers per volume";
      return false;
    }
  }
  else
  {
    for(int i = 0; i < count; i++)
      affines += affine;
  }

  // one row per volume
  const QVector<hsize_t> perVolume = QVector<hsize_t>() << static_cast<hsize_t>(count);
  static const char* const required[] = { "Frame", "FrameTimesStart", "FrameDuration" };
  QVector<double> values[3];
  for(int i = 0; i < 3; i++)
  {
    const QByteArray path = QByteArray("image/volumes/") + required[i];
    if(exists(file, path) == false || readNumbers(file, path.constData(), perVolume, values[i]) == false ||
       allFinite(values[i]) == false)
    {
      reason = QString("/%1 is missing or has not one number per volume").arg(QString::fromLatin1(path));
      return false;
    }
  }
  starts = values[1];
  durations = values[2];

  static const char* const indexNames[] = { "Frame", "Gate", "Bed" };
  QVector<int>* const indices[] = { &frames, &gates, &beds };
  for(int i = 0; i < 3; i++)
  {
    QVector<double> numbers = values[0];
    const QByteArray path = QByteArray("image/volumes/") + indexNames[i];
    if(i > 0)
    {
      numbers = QVector<double>(count, i == 1 ? 1.0 : 0.0);   // one gate, one bed position
      if(exists(file, path) && readNumbers(file, path.constData(), perVolume, numbers) == false)
      {
        reason = QString("/%1 has not one number per volume").arg(QString::fromLatin1(path));
        return false;
      }
    }
    foreach(double number, numbers)
    {
      // frames and gates start with 1, bed positions with 0
      if(number != std::floor(number) || number < (i == 2 ? 0.0 : 1.0) || number > static_cast<double>(limit))
      {
        reason = QString("/%1 has invalid values").arg(QString::fromLatin1(path));
        return false;
      }
      indices[i]->append(static_cast<int>(number));
    }
  }

  // every volume has its own frame, gate and bed position
  QSet<QString> seen;
  int firstGateNumber = gates[0], firstBedNumber = beds[0];
  for(int i = 0; i < count; i++)
  {
    const QString key = QString("%1/%2/%3").arg(frames[i]).arg(gates[i]).arg(beds[i]);
    if(seen.contains(key) || durations[i] < 0.0)
    {
      reason = seen.contains(key) ? "several volumes have the same frame, gate and bed position"
                                  : "/image/volumes/FrameDuration has negative values";
      return false;
    }
    seen.insert(key);
    firstGateNumber = qMin(firstGateNumber, gates[i]);
    firstBedNumber = qMin(firstBedNumber, beds[i]);
  }
  for(int i = 0; i < count; i++)
  {
    if(gates[i] == firstGateNumber && beds[i] == firstBedNumber)
      firstGate.append(i);
  }

  // further values of the volumes
  static const char* const optional[] = { "FrameReferenceTime", "DecayFactor", "DeadTimeFactor", "TriggerTime",
                                          "TablePosition" };
  for(unsigned i = 0; i < sizeof(optional) / sizeof(optional[0]); i++)
  {
    const QByteArray path = QByteArray("image/volumes/") + optional[i];
    if(exists(file, path) == false)
      continue;
    QVector<double> numbers;
    if(readNumbers(file, path.constData(), perVolume, numbers) == false || allFinite(numbers) == false)
    {
      reason = QString("/%1 has not one number per volume").arg(QString::fromLatin1(path));
      return false;
    }
    frameValues[QString::fromLatin1(optional[i])] = numbers;
  }

  return readHeader(reason);
}

// the attributes of the study: the JSON object of the file and the typed
// attributes, which are the valid ones if both give a value
bool CHDF5FilePrivate::readHeader(QString& reason)
{
  if(exists(file, "header") == false)
  {
    if(units.isEmpty() == false)
      mainAttributes["Units"] = units;
    return true;
  }
  HDF5Id header(HDF5Id::Group, H5Gopen2(file, "header", H5P_DEFAULT));
  if(header.isValid() == false)
  {
    reason = "/header is no group";
    return false;
  }

  if(exists(header, "json"))
  {
    HDF5Id json(HDF5Id::Dataset, H5Dopen2(header, "json", H5P_DEFAULT));
    HDF5Id type(HDF5Id::Type, json.isValid() ? H5Dget_type(json) : -1);
    HDF5Id space(HDF5Id::Space, json.isValid() ? H5Dget_space(json) : -1);
    QStringList strings;
    const ReadDataset read = { json };
    QJsonParseError error;
    QJsonDocument document;
    if(type.isValid() && space.isValid() && H5Tget_class(type) == H5T_STRING &&
       H5Sget_simple_extent_type(space) == H5S_SCALAR && readStrings(type, space, 1, read, strings))
      document = QJsonDocument::fromJson(strings.value(0).toUtf8(), &error);
    if(document.isObject() == false)
    {
      reason = "/header/json is no JSON object";
      return false;
    }
    mainAttributes = document.object();

    // the frame attributes of all volumes
    const QJsonArray objects = mainAttributes.take(QString::fromLatin1(CMedIOHeaderDictionary::FramesKey)).toArray();
    if(objects.size() == volumes())
      frameObjects = objects;
  }

  foreach(const QByteArray& name, attributeNames(header))
  {
    const QJsonValue value = attributeValue(header, name.constData());
    if(value.isUndefined() == false)
      mainAttributes[QString::fromUtf8(name)] = value;
  }

  // code sequences: groups with the strings of the code
  foreach(const CMedIOHeaderDictionary::Entry* entry, CMedIOHeaderDictionary::entries())
  {
    if(entry->type != CMedIOHeaderDictionary::Code || exists(header, entry->keyword) == false)
      continue;
    HDF5Id group(HDF5Id::Group, H5Gopen2(header, entry->keyword, H5P_DEFAULT));
    if(group.isValid() == false)
      continue;
    QJsonObject code;
    foreach(const QByteArray& name, attributeNames(group))
    {
      const QJsonValue value = attributeValue(group, name.constData());
      if(value.isString())
        code[QString::fromUtf8(name)] = value;
    }
    mainAttributes[QString::fromLatin1(entry->keyword)] = code;
  }

  // the unit given with the values is the valid one
  if(units.isEmpty() == false)
    mainAttributes["Units"] = units;

  return true;
}

#endif // MEDIO_HAVE_HDF5

CHDF5File::CHDF5File(const QString& fileName)
  : CMedIOData(fileName),
    m_pData(new CHDF5FilePrivate)
{
  ENTER();
  LEAVE();
}

CHDF5File::~CHDF5File()
{
  ENTER();

  close();
  delete m_pData;

  LEAVE();
}

bool CHDF5File::isOfType(const QString& fileName)
{
  ENTER();

  bool result = false;
  QFile file(fileName);
  if(file.open(QIODevice::ReadOnly))
  {
    const qint64 size = file.size();
    for(qint64 offset = 0; result == false && offset + signatureSize <= size;
        offset = (offset == 0) ? firstUserBlockSize : offset * 2)
    {
      result = file.seek(offset) && file.read(signatureSize) == QByteArray(signature, signatureSize);
    }
    file.close();
  }

  RETURN(result);
  return result;
}

CMedIOData* CHDF5File::createFromFile(const QString& fileName)
{
  ENTER();

  CMedIOData* result = isOfType(fileName) ? new CHDF5File(fileName) : NULL;

  RETURN(result);
  return result;
}

bool CHDF5File::hasSupport()
{
#ifdef MEDIO_HAVE_HDF5
  return true;
#else
  return false;
#endif
}

QString CHDF5File::libraryVersion()
{
  ENTER();

  QString result;
#ifdef MEDIO_HAVE_HDF5
  const HDF5Lock lock;
  unsigned major = 0, minor = 0, release = 0;
  if(H5get_libversion(&major, &minor, &release) >= 0)
    result = QString("%1.%2.%3").arg(major).arg(minor).arg(release);
#endif

  LEAVE();
  return result;
}

bool CHDF5File::open(QIODevice::OpenModeFlag mode)
{
  ENTER();

  close();
  m_pData->error.clear();
  bool result = false;

#ifdef MEDIO_HAVE_HDF5
  if(mode != QIODevice::ReadOnly)
    m_pData->error = QString("HDF5 files cannot be written yet");
  else if(isOfType(fileName()) == false)
    m_pData->error = QString("no HDF5 file");
  else
  {
    {
      const HDF5Lock lock;
      // the HDF5 library opens the file by its name
      m_pData->file = H5Fopen(QFile::encodeName(fileName()).constData(), H5F_ACC_RDONLY, H5P_DEFAULT);
      if(m_pData->file < 0)
        m_pData->error = QString("the HDF5 library cannot open the file");
      else
        result = m_pData->read(m_pData->error);
    }

    // the QFile is opened as well: the open state of the file is that of the QFile
    if(result && QFile::open(QIODevice::ReadOnly) == false)
    {
      m_pData->error = errorString();
      result = false;
    }
    if(result == false)
    {
      const QString error = m_pData->error;
      close();
      m_pData->error = error;
    }
  }
#else
  Q_UNUSED(mode);
  m_pData->error = QString("libmedio was built without HDF5 support");
#endif

  if(result == false)
    E("%s: %s", qPrintable(fileName()), qPrintable(m_pData->error));

  RETURN(result);
  return result;
}

void CHDF5File::close()
{
  ENTER();

#ifdef MEDIO_HAVE_HDF5
  if(m_pData->file >= 0 || m_pData->dataset >= 0)
  {
    const HDF5Lock lock;
    if(m_pData->dataset >= 0)
      H5Dclose(m_pData->dataset);
    if(m_pData->file >= 0)
      H5Fclose(m_pData->file);
  }
#endif
  const QString error = m_pData->error;
  m_pData->clear();
  m_pData->error = error;

  if(isOpen())
    QFile::close();

  LEAVE();
}

CMedIOData::Format CHDF5File::dataFormat() const
{
  return CMedIOData::HDF5;
}

QString CHDF5File::openError() const
{
  return m_pData->error;
}

int CHDF5File::numVolumes() const
{
  return m_pData->volumes();
}

QList<short> CHDF5File::firstGateFrames() const
{
  QList<short> numbers;
  foreach(int volume, m_pData->firstGate)
    numbers.append(static_cast<short>(m_pData->frames[volume]));
  return numbers;
}

int CHDF5File::volumeOfFrame(short frame) const
{
  foreach(int volume, m_pData->firstGate)
  {
    if(frame == -1 || m_pData->frames[volume] == frame)
      return volume;
  }
  return -1;
}

bool CHDF5File::readMainHeader(CHDF5MainHeader*& mainHeader)
{
  ENTER();
  mainHeader = NULL;

  if(isOpen() == false || m_pData->volumes() == 0)
  {
    E("the file %s is not open", qPrintable(fileName()));
    RETURN(false);
    return false;
  }

  mainHeader = new CHDF5MainHeader(this);
  mainHeader->setFormatVersion(m_pData->version);
  mainHeader->setNumVolumes(m_pData->volumes());
  mainHeader->setNumFrames(m_pData->firstGate.size());
  MedIOHeaderMapping::transfer(m_pData->mainAttributes, *mainHeader);

  RETURN(true);
  return true;
}

bool CHDF5File::readSubHeader(CHDF5FrameHeader*& frameHeader, short frame)
{
  ENTER();

  const bool result = readVolumeHeader(frameHeader, volumeOfFrame(frame));

  RETURN(result);
  return result;
}

bool CHDF5File::readVolumeHeader(CHDF5FrameHeader*& frameHeader, int volume)
{
  ENTER();
  frameHeader = NULL;

  if(isOpen() == false || volume < 0 || volume >= m_pData->volumes())
  {
    E("no volume %d in the file %s", volume, qPrintable(fileName()));
    RETURN(false);
    return false;
  }

  frameHeader = new CHDF5FrameHeader(this, static_cast<short>(m_pData->frames[volume]));
  frameHeader->setVolume(volume);
  frameHeader->setIndex(static_cast<short>(m_pData->frames[volume]), static_cast<short>(m_pData->gates[volume]),
                        static_cast<short>(m_pData->beds[volume]));
  frameHeader->setDimensions(m_pData->dimensions[0], m_pData->dimensions[1], m_pData->dimensions[2]);
  frameHeader->setDataType(m_pData->dataType);
  frameHeader->setUnits(m_pData->units);
  double m[3][4];
  for(int i = 0; i < 12; i++)
    m[i / 4][i % 4] = m_pData->affines[volume * 12 + i];
  frameHeader->setAffine(m);
  frameHeader->setTiming(m_pData->starts[volume], m_pData->durations[volume]);

  // the frame attributes: the entry of the JSON object, replaced by the values of the datasets
  QJsonObject attributes = m_pData->frameObjects.at(volume).toObject();
  attributes["ActualFrameDuration"] = static_cast<double>(qRound64(m_pData->durations[volume] * 1000.0));
  for(QMap<QString, QVector<double> >::const_iterator it = m_pData->frameValues.constBegin();
      it != m_pData->frameValues.constEnd(); ++it)
    attributes[it.key()] = MedIOHeaderMapping::number(it.value()[volume]);
  MedIOHeaderMapping::transfer(attributes, *frameHeader);

  RETURN(true);
  return true;
}

bool CHDF5File::readMatrix(QByteArray*& data, CHDF5FrameHeader*& frameHeader, short frame)
{
  ENTER();

  const bool result = readVolumeMatrix(data, frameHeader, volumeOfFrame(frame));

  RETURN(result);
  return result;
}

bool CHDF5File::readVolumeMatrix(QByteArray*& data, CHDF5FrameHeader*& frameHeader, int volume)
{
  ENTER();
  data = NULL;
  bool result = readVolumeHeader(frameHeader, volume);

#ifdef MEDIO_HAVE_HDF5
  if(result)
  {
    // the hyperslab data[volume] in the byte order and number format of this machine
    const hsize_t nz = static_cast<hsize_t>(m_pData->dimensions[2]);
    const hsize_t ny = static_cast<hsize_t>(m_pData->dimensions[1]);
    const hsize_t nx = static_cast<hsize_t>(m_pData->dimensions[0]);
    const hsize_t start[4] = { static_cast<hsize_t>(volume), 0, 0, 0 };
    const hsize_t count[4] = { 1, nz, ny, nx };
    const hsize_t size[3] = { nz, ny, nx };
    const int bytes = static_cast<int>(nz * ny * nx) * CMedIOVolume::bytesPerVoxel(m_pData->dataType);

    data = new QByteArray(bytes, '\0');
    {
      const HDF5Lock lock;
      HDF5Id fileSpace(HDF5Id::Space, H5Dget_space(m_pData->dataset));
      HDF5Id memorySpace(HDF5Id::Space, H5Screate_simple(3, size, NULL));
      result = fileSpace.isValid() && memorySpace.isValid() &&
               H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start, NULL, count, NULL) >= 0 &&
               H5Dread(m_pData->dataset, m_pData->dataType == CMedIOVolume::Float64 ? H5T_NATIVE_DOUBLE : H5T_NATIVE_FLOAT,
                       memorySpace, fileSpace, H5P_DEFAULT, data->data()) >= 0;
    }
    if(result == false)
      E("cannot read volume %d of the file %s", volume, qPrintable(fileName()));
  }
#else
  result = false;
#endif

  if(result == false)
  {
    delete data;
    delete frameHeader;
    data = NULL;
    frameHeader = NULL;
  }

  RETURN(result);
  return result;
}
