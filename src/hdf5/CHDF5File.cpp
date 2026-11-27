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
#include "config.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <QSet>
#include <QStringList>
#include <QTemporaryFile>
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

// the layout read and written (docs/hdf5.md)
const char* const layoutName = "MEDIO-IMAGE";
const int layoutMajorVersion = 0;
const char* const layoutVersion = "0.1";

#ifdef MEDIO_HAVE_HDF5

// a chunk of the voxels holds whole planes of one volume with at most this much data
const qint64 maxChunkBytes = 1 << 20;
const unsigned deflateLevel = 4;

// the datasets of /image/volumes with a value of the frame attributes and their units
struct FrameDataset { const char* name; const char* units; };
const FrameDataset frameDatasets[] = { { "FrameReferenceTime", "ms" }, { "DecayFactor", "1" }, { "DeadTimeFactor", "1" },
                                       { "TriggerTime", "ms" }, { "TablePosition", "mm" } };
const int frameDatasetCount = sizeof(frameDatasets) / sizeof(frameDatasets[0]);

// DICOM Units (and the further terms of libmedio) in UCUM notation, NULL for other terms
const char* ucumOfUnits(const QString& units)
{
  static const char* const table[][2] = { { "BQML", "Bq/mL" }, { "KBQCC", "kBq/mL" }, { "CNTS", "{counts}" },
                                          { "CPS", "{counts}/s" }, { "NONE", "1" }, { "CM2", "cm2" },
                                          { "PCNT", "%" }, { "GML", "g/mL{SUVbw}" } };
  for(unsigned i = 0; i < sizeof(table) / sizeof(table[0]); i++)
  {
    if(units == QLatin1String(table[i][0]))
      return table[i][1];
  }
  return NULL;
}

// largest number of values of an attribute or of a dataset with one value per volume
const qint64 maxValues = 32767;

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

//---------------------------------------------------------------------------------------------
// writing

// a variable-length UTF-8 string type
hid_t createTextType()
{
  const hid_t type = H5Tcopy(H5T_C_S1);
  if(type >= 0 && (H5Tset_size(type, H5T_VARIABLE) < 0 || H5Tset_cset(type, H5T_CSET_UTF8) < 0))
  {
    H5Tclose(type);
    return -1;
  }
  return type;
}

// a string attribute (scalar) or an attribute with one dimension of strings
bool writeTextAttribute(hid_t object, const char* name, const QStringList& strings, bool scalar)
{
  QList<QByteArray> utf8;
  QVector<const char*> pointers;
  foreach(const QString& string, strings)
    utf8.append(string.toUtf8());
  for(int i = 0; i < utf8.size(); i++)
    pointers.append(utf8[i].constData());

  const hsize_t count = static_cast<hsize_t>(pointers.size());
  HDF5Id type(HDF5Id::Type, createTextType());
  HDF5Id space(HDF5Id::Space, scalar ? H5Screate(H5S_SCALAR) : H5Screate_simple(1, &count, NULL));
  if(type.isValid() == false || space.isValid() == false || count == 0)
    return false;
  HDF5Id attribute(HDF5Id::Attribute, H5Acreate2(object, name, type, space, H5P_DEFAULT, H5P_DEFAULT));
  return attribute.isValid() && H5Awrite(attribute, type, pointers.constData()) >= 0;
}

bool writeTextAttribute(hid_t object, const char* name, const QString& string)
{
  return writeTextAttribute(object, name, QStringList() << string, true);
}

// a scalar number attribute: little-endian float64, or int32 for whole numbers of integer attributes
bool writeNumberAttribute(hid_t object, const char* name, double number, bool integer)
{
  const bool int32 = integer && number == std::floor(number) &&
                     number >= static_cast<double>(std::numeric_limits<qint32>::min()) &&
                     number <= static_cast<double>(std::numeric_limits<qint32>::max());
  HDF5Id space(HDF5Id::Space, H5Screate(H5S_SCALAR));
  if(space.isValid() == false)
    return false;
  HDF5Id attribute(HDF5Id::Attribute, H5Acreate2(object, name, int32 ? H5T_STD_I32LE : H5T_IEEE_F64LE, space,
                                                 H5P_DEFAULT, H5P_DEFAULT));
  return attribute.isValid() && H5Awrite(attribute, H5T_NATIVE_DOUBLE, &number) >= 0;
}

// a dataset of numbers with the given dimensions and, if given, the attribute units
bool writeNumbers(hid_t location, const char* name, hid_t fileType, const QVector<hsize_t>& dimensions,
                  const QVector<double>& numbers, const char* units)
{
  HDF5Id space(HDF5Id::Space, H5Screate_simple(dimensions.size(), dimensions.constData(), NULL));
  if(space.isValid() == false)
    return false;
  HDF5Id dataset(HDF5Id::Dataset, H5Dcreate2(location, name, fileType, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
  return dataset.isValid() &&
         H5Dwrite(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, numbers.constData()) >= 0 &&
         (units == NULL || writeTextAttribute(dataset, "units", QString::fromLatin1(units)));
}

// true if the two affines (12 values) give the same position of the voxels, as
// CMedIOVolume::hasSameGeometry() compares them
bool sameAffine(const double* a, const double* b)
{
  for(int i = 0; i < 12; i++)
  {
    if(std::fabs(a[i] - b[i]) > 1e-3 + 1e-6 * std::fabs(a[i]))
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
      writing = false;
      headerWritten = false;
      failed = false;
      temporaryName.clear();
      frameDefaults = QJsonArray();
    }

    int volumes() const { return frames.size(); }

#ifdef MEDIO_HAVE_HDF5
    bool read(QString& reason);
    bool readHeader(QString& reason);
    bool create(const QString& fileName, QString& reason);
    bool createDataset();
    bool writeVolume(int volume, const QByteArray& data);
    bool writeTables();
    bool writeHeader();

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

    // a new file
    bool writing;                        //!< opened for writing
    bool headerWritten;                  //!< the main header (mainAttributes) was given
    bool failed;                         //!< writing failed: the file is discarded
    QString temporaryName;               //!< the name the file is written under
    QJsonArray frameDefaults;            //!< "Frames" of the main header written
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
  for(int i = 0; i < frameDatasetCount; i++)
  {
    const QByteArray path = QByteArray("image/volumes/") + frameDatasets[i].name;
    if(exists(file, path) == false)
      continue;
    QVector<double> numbers;
    if(readNumbers(file, path.constData(), perVolume, numbers) == false || allFinite(numbers) == false)
    {
      reason = QString("/%1 has not one number per volume").arg(QString::fromLatin1(path));
      return false;
    }
    frameValues[QString::fromLatin1(frameDatasets[i].name)] = numbers;
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

// create the new file under a temporary name in the directory of fileName (the lock is held)
bool CHDF5FilePrivate::create(const QString& fileName, QString& reason)
{
  // a unique name: the HDF5 library creates the file by its name
  {
    QTemporaryFile temporary(fileName + ".XXXXXX.tmp");
    temporary.setAutoRemove(false);
    if(temporary.open() == false)
    {
      reason = QString("cannot create a file in the directory of %1: %2").arg(fileName, temporary.errorString());
      return false;
    }
    temporaryName = temporary.fileName();
  }

  // the lowest file format versions, so that every HDF5 library reads the file
  const hid_t properties = H5Pcreate(H5P_FILE_ACCESS);
  if(properties >= 0)
  {
#if H5_VERSION_GE(1, 10, 2)
    if(H5Pset_libver_bounds(properties, H5F_LIBVER_EARLIEST, H5F_LIBVER_V18) < 0)
      H5Pset_libver_bounds(properties, H5F_LIBVER_V18, H5F_LIBVER_V18);
#endif
    file = H5Fcreate(QFile::encodeName(temporaryName).constData(), H5F_ACC_TRUNC, H5P_DEFAULT, properties);
    H5Pclose(properties);
  }
  if(file < 0)
  {
    reason = "the HDF5 library cannot create the file";
    return false;
  }

  HDF5Id image(HDF5Id::Group, H5Gcreate2(file, "image", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
  HDF5Id volumes(HDF5Id::Group, image.isValid() ? H5Gcreate2(image, "volumes", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) : -1);
  if(volumes.isValid() == false ||
     writeTextAttribute(file, "Format", QString::fromLatin1(layoutName)) == false ||
     writeTextAttribute(file, "FormatVersion", QString::fromLatin1(layoutVersion)) == false ||
     writeTextAttribute(file, "Creator", QString("%1 %2").arg(PROJECT_LONGNAME, PROJECT_VERSION)) == false)
  {
    reason = "cannot write the HDF5 file";
    return false;
  }

  return true;
}

// /image/data for volumes of dimensions and dataType: chunks of whole planes of one
// volume, shuffle and deflate, any number of volumes
bool CHDF5FilePrivate::createDataset()
{
  const hsize_t nz = static_cast<hsize_t>(dimensions[2]);
  const hsize_t ny = static_cast<hsize_t>(dimensions[1]);
  const hsize_t nx = static_cast<hsize_t>(dimensions[0]);
  const qint64 planeBytes = static_cast<qint64>(nx * ny) * CMedIOVolume::bytesPerVoxel(dataType);
  const hsize_t planes = static_cast<hsize_t>(qBound<qint64>(1, maxChunkBytes / planeBytes, static_cast<qint64>(nz)));
  const hsize_t size[4] = { 0, nz, ny, nx };
  const hsize_t maximum[4] = { H5S_UNLIMITED, nz, ny, nx };
  const hsize_t chunk[4] = { 1, planes, ny, nx };

  HDF5Id space(HDF5Id::Space, H5Screate_simple(4, size, maximum));
  const hid_t properties = H5Pcreate(H5P_DATASET_CREATE);
  bool result = space.isValid() && properties >= 0 && H5Pset_chunk(properties, 4, chunk) >= 0;
  if(result && H5Zfilter_avail(H5Z_FILTER_DEFLATE) > 0)
    result = H5Pset_shuffle(properties) >= 0 && H5Pset_deflate(properties, deflateLevel) >= 0;
  if(result)
    dataset = H5Dcreate2(file, "image/data", dataType == CMedIOVolume::Float64 ? H5T_IEEE_F64LE : H5T_IEEE_F32LE, space,
                         H5P_DEFAULT, properties, H5P_DEFAULT);
  if(properties >= 0)
    H5Pclose(properties);

  return result && dataset >= 0;
}

// the voxels of a volume; the dataset grows by the volumes written
bool CHDF5FilePrivate::writeVolume(int volume, const QByteArray& data)
{
  const hsize_t nz = static_cast<hsize_t>(dimensions[2]);
  const hsize_t ny = static_cast<hsize_t>(dimensions[1]);
  const hsize_t nx = static_cast<hsize_t>(dimensions[0]);

  HDF5Id current(HDF5Id::Space, H5Dget_space(dataset));
  hsize_t size[4] = { 0, 0, 0, 0 };
  if(current.isValid() == false || H5Sget_simple_extent_dims(current, size, NULL) != 4)
    return false;
  if(static_cast<hsize_t>(volume) >= size[0])
  {
    size[0] = static_cast<hsize_t>(volume) + 1;
    if(H5Dset_extent(dataset, size) < 0)
      return false;
  }

  const hsize_t start[4] = { static_cast<hsize_t>(volume), 0, 0, 0 };
  const hsize_t count[4] = { 1, nz, ny, nx };
  const hsize_t memorySize[3] = { nz, ny, nx };
  HDF5Id fileSpace(HDF5Id::Space, H5Dget_space(dataset));
  HDF5Id memorySpace(HDF5Id::Space, H5Screate_simple(3, memorySize, NULL));
  return fileSpace.isValid() && memorySpace.isValid() &&
         H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start, NULL, count, NULL) >= 0 &&
         H5Dwrite(dataset, dataType == CMedIOVolume::Float64 ? H5T_NATIVE_DOUBLE : H5T_NATIVE_FLOAT, memorySpace,
                  fileSpace, H5P_DEFAULT, data.constData()) >= 0;
}

// the unit of the values, the affines and the rows of the volumes
bool CHDF5FilePrivate::writeTables()
{
  const int count = volumes();
  const QVector<hsize_t> perVolume = QVector<hsize_t>() << static_cast<hsize_t>(count);

  // the unit in both notations, or none
  const QString unitsTerm = mainAttributes.value("Units").toString();
  if(ucumOfUnits(unitsTerm) != NULL &&
     (writeTextAttribute(dataset, "Units", unitsTerm) == false ||
      writeTextAttribute(dataset, "units", QString::fromLatin1(ucumOfUnits(unitsTerm))) == false))
    return false;

  bool sameGeometry = true;
  for(int i = 1; i < count; i++)
    sameGeometry = sameGeometry && sameAffine(affines.constData(), affines.constData() + i * 12);
  if(writeNumbers(file, "image/affine", H5T_IEEE_F64LE, QVector<hsize_t>() << 3 << 4, affines.mid(0, 12), "mm") == false ||
     (sameGeometry == false &&
      writeNumbers(file, "image/volumes/affine", H5T_IEEE_F64LE, QVector<hsize_t>() << static_cast<hsize_t>(count) << 3 << 4,
                   affines, "mm") == false))
    return false;

  // the index: gates and bed positions only if there are several
  const QVector<int>* const indices[] = { &frames, &gates, &beds };
  static const char* const indexNames[] = { "image/volumes/Frame", "image/volumes/Gate", "image/volumes/Bed" };
  for(int i = 0; i < 3; i++)
  {
    QVector<double> numbers;
    bool needed = (i == 0);
    foreach(int number, *indices[i])
    {
      numbers.append(number);
      needed = needed || number != (i == 1 ? 1 : 0);
    }
    if(needed && writeNumbers(file, indexNames[i], H5T_STD_I32LE, perVolume, numbers, NULL) == false)
      return false;
  }

  if(writeNumbers(file, "image/volumes/FrameTimesStart", H5T_IEEE_F64LE, perVolume, starts, "s") == false ||
     writeNumbers(file, "image/volumes/FrameDuration", H5T_IEEE_F64LE, perVolume, durations, "s") == false)
    return false;

  // the values of the frame attributes every volume has
  for(int i = 0; i < frameDatasetCount; i++)
  {
    const QString name = QString::fromLatin1(frameDatasets[i].name);
    QVector<double> numbers;
    foreach(const QJsonValue& frame, frameObjects)
    {
      const QJsonValue value = frame.toObject().value(name);
      if(value.isDouble() && std::isfinite(value.toDouble()))
        numbers.append(value.toDouble());
    }
    if(numbers.size() == count &&
       writeNumbers(file, (QByteArray("image/volumes/") + frameDatasets[i].name).constData(), H5T_IEEE_F64LE, perVolume,
                    numbers, frameDatasets[i].units) == false)
      return false;
  }

  return true;
}

// /header: the attributes of the dictionary as typed attributes, and all attributes
// incl. those of the volumes ("Frames") as JSON object
bool CHDF5FilePrivate::writeHeader()
{
  HDF5Id header(HDF5Id::Group, H5Gcreate2(file, "header", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
  if(header.isValid() == false)
    return false;

  for(QJsonObject::const_iterator it = mainAttributes.constBegin(); it != mainAttributes.constEnd(); ++it)
  {
    const CMedIOHeaderDictionary::Entry* entry = CMedIOHeaderDictionary::entry(it.key());
    if(entry == NULL || entry->scope != CMedIOHeaderDictionary::MainHeader ||
       CMedIOHeaderDictionary::isValid(*entry, it.value()) == false)
      continue;   // native sections, unknown keys: in the JSON object only

    const QJsonValue value = it.value();
    const QString vr = QString::fromLatin1(entry->vr);
    bool written = true;
    switch(entry->type)
    {
      case CMedIOHeaderDictionary::Number:
        if(value.isDouble())
          written = writeNumberAttribute(header, entry->keyword, value.toDouble(),
                                         vr == "IS" || vr == "US" || vr == "UL" || vr == "SS" || vr == "SL");
      break;

      case CMedIOHeaderDictionary::String:
      case CMedIOHeaderDictionary::Date:
      case CMedIOHeaderDictionary::Time:
      case CMedIOHeaderDictionary::DateTime:
        if(value.isString())
          written = writeTextAttribute(header, entry->keyword, value.toString());
      break;

      case CMedIOHeaderDictionary::Strings:
      {
        QStringList strings;
        foreach(const QJsonValue& string, value.toArray())
          strings.append(string.toString());
        if(strings.isEmpty() == false)
          written = writeTextAttribute(header, entry->keyword, strings, false);
      }
      break;

      case CMedIOHeaderDictionary::Code:
      {
        const QJsonObject code = value.toObject();
        HDF5Id group(HDF5Id::Group, code.isEmpty() ? -1 : H5Gcreate2(header, entry->keyword, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
        written = code.isEmpty() || group.isValid();
        for(QJsonObject::const_iterator item = code.constBegin(); written && item != code.constEnd(); ++item)
        {
          if(item.value().isString())
            written = writeTextAttribute(group, item.key().toLatin1().constData(), item.value().toString());
        }
      }
      break;
    }
    if(written == false)
      return false;
  }

  QJsonObject all = mainAttributes;
  all[QString::fromLatin1(CMedIOHeaderDictionary::FramesKey)] = frameObjects;
  const QByteArray json = QJsonDocument(all).toJson(QJsonDocument::Compact);
  const char* const text = json.constData();
  HDF5Id type(HDF5Id::Type, createTextType());
  HDF5Id space(HDF5Id::Space, H5Screate(H5S_SCALAR));
  if(type.isValid() == false || space.isValid() == false)
    return false;
  HDF5Id dataset(HDF5Id::Dataset, H5Dcreate2(header, "json", type, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
  return dataset.isValid() && H5Dwrite(dataset, type, H5S_ALL, H5S_ALL, H5P_DEFAULT, &text) >= 0;
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
  if(mode == QIODevice::WriteOnly)
  {
    {
      const HDF5Lock lock;
      result = m_pData->create(fileName(), m_pData->error);
    }
    if(result)
    {
      // The file is written by the HDF5 library under its temporary name. The QFile of
      // this object, which has the name of the new file, is not opened (that would
      // create or truncate it): only its open state is set, unbuffered and after exists()
      // has made its file engine, so that its own read and write methods fail.
      exists();
      m_pData->writing = true;
      result = QIODevice::open(QIODevice::WriteOnly | QIODevice::Unbuffered);
    }
    if(result == false)
    {
      const QString error = m_pData->error.isEmpty() ? QString("cannot open the file") : m_pData->error;
      m_pData->writing = true;   // remove the temporary file
      m_pData->failed = true;
      finish();
      m_pData->error = error;
    }
  }
  else if(mode != QIODevice::ReadOnly)
    m_pData->error = QString("HDF5 files are opened for reading or as new file for writing");
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

  finish();

  LEAVE();
}

bool CHDF5File::finish()
{
  ENTER();
  bool result = true;
  QString error = m_pData->error;

#ifdef MEDIO_HAVE_HDF5
  const bool writing = m_pData->writing;
  const QString temporaryName = m_pData->temporaryName;
  if(writing)
  {
    // a new file is complete with its main header and at least one volume
    error.clear();
    if(m_pData->failed)
      error = QString("writing the file failed");
    else if(m_pData->headerWritten == false || m_pData->volumes() == 0)
      error = QString("no main header or no volume was written");
    result = error.isEmpty();
  }

  if(m_pData->file >= 0 || m_pData->dataset >= 0)
  {
    const HDF5Lock lock;
    if(writing && result && (m_pData->writeTables() == false || m_pData->writeHeader() == false))
    {
      error = QString("cannot write the HDF5 file");
      result = false;
    }
    if(m_pData->dataset >= 0 && H5Dclose(m_pData->dataset) < 0 && writing && result)
      result = false;
    if(m_pData->file >= 0 && H5Fclose(m_pData->file) < 0 && writing && result)
      result = false;
    if(writing && result == false && error.isEmpty())
      error = QString("cannot write the HDF5 file");
  }

  if(writing)
  {
    // the complete file gets its name, replacing an existing file; otherwise nothing is left
    if(result && QFile::exists(fileName()) && QFile::remove(fileName()) == false)
    {
      error = QString("cannot replace the existing file");
      result = false;
    }
    if(result && QFile::rename(temporaryName, fileName()) == false)
    {
      error = QString("cannot rename the temporary file %1").arg(temporaryName);
      result = false;
    }
    if(result == false && temporaryName.isEmpty() == false)
      QFile::remove(temporaryName);
  }
#endif

  const bool wasWriting = m_pData->writing;
  m_pData->clear();
  m_pData->error = error;

  // a file opened for writing has only the open state of the QFile set (see open())
  if(isOpen())
  {
    if(wasWriting)
      QIODevice::close();
    else
      QFile::close();
  }

  if(result == false)
  {
    m_iErrStatus = MERR_Unspecified;
    E("%s: %s", qPrintable(fileName()), qPrintable(error));
  }

  RETURN(result);
  return result;
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

  if(isOpen() == false || m_pData->writing || m_pData->volumes() == 0)
  {
    E("the file %s is not open for reading", qPrintable(fileName()));
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

  if(isOpen() == false || m_pData->writing || volume < 0 || volume >= m_pData->volumes())
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

bool CHDF5File::writeMainHeader(const CHDF5MainHeader& mainHeader)
{
  ENTER();
  bool result = false;

  if(isOpen() == false || m_pData->writing == false)
    E("the file %s is not open for writing", qPrintable(fileName()));
  else if(m_pData->headerWritten)
    E("the main header of %s was written already", qPrintable(fileName()));
  else
  {
    // written when the file is closed, with the attributes of the volumes; "Frames" of the
    // main header gives the attributes of the volumes written without attributes
    m_pData->mainAttributes = mainHeader.attributes();
    m_pData->frameDefaults = m_pData->mainAttributes.take(QString::fromLatin1(CMedIOHeaderDictionary::FramesKey)).toArray();
    m_pData->headerWritten = true;
    result = true;
  }

  m_iErrStatus = result ? MERR_Ok : MERR_Unspecified;
  RETURN(result);
  return result;
}

bool CHDF5File::writeMatrix(const QByteArray& data, const CHDF5FrameHeader& frameHeader, short frame)
{
  ENTER();
  bool result = false;

#ifdef MEDIO_HAVE_HDF5
  const CMedIOVolume::DataType type = frameHeader.dataType();
  const int nx = frameHeader.dimension(0), ny = frameHeader.dimension(1), nz = frameHeader.dimension(2);
  const qint64 bytes = static_cast<qint64>(nx) * ny * nz * CMedIOVolume::bytesPerVoxel(type);
  double m[3][4];
  frameHeader.affine(m);
  bool finite = std::isfinite(frameHeader.frameStart()) && std::isfinite(frameHeader.frameDuration());
  for(int i = 0; i < 12; i++)
    finite = finite && std::isfinite(m[i / 4][i % 4]);
  const int count = m_pData->volumes();

  // the index of the volume
  if(frame == -1)
    frame = frameHeader.frame() > 0 ? frameHeader.frame() : static_cast<short>(qMin(count + 1, static_cast<int>(maxValues)));
  const short gate = frameHeader.gate(), bed = frameHeader.bed();

  if(isOpen() == false || m_pData->writing == false || m_pData->headerWritten == false || m_pData->failed)
    E("the file %s is not open for writing, or its main header was not written", qPrintable(fileName()));
  else if((type != CMedIOVolume::Float32 && type != CMedIOVolume::Float64) || nx < 1 || ny < 1 || nz < 1 ||
          MedIOLimits::fitsByteArray(bytes) == false || data.size() != bytes)
    E("the matrix (%d bytes) is no volume of %d x %d x %d float32 or float64 values", data.size(), nx, ny, nz);
  else if(count > 0 && (type != m_pData->dataType || nx != m_pData->dimensions[0] || ny != m_pData->dimensions[1] ||
                        nz != m_pData->dimensions[2]))
    E("the volumes of an HDF5 file have the same dimensions and data type");
  else if(finite == false || frameHeader.frameDuration() < 0.0 || frame < 1 || gate < 1 || bed < 0)
    E("invalid affine, frame timing or index (frame %d, gate %d, bed %d) of the volume", frame, gate, bed);
  else
  {
    // a volume of the same frame, gate and bed position is replaced
    int volume = count;
    for(int i = 0; i < count; i++)
    {
      if(m_pData->frames[i] == frame && m_pData->gates[i] == gate && m_pData->beds[i] == bed)
        volume = i;
    }

    if(volume >= maxValues)
      E("too many volumes");
    else
    {
      {
        const HDF5Lock lock;
        if(count == 0)
        {
          m_pData->dataType = type;
          m_pData->dimensions[0] = nx;
          m_pData->dimensions[1] = ny;
          m_pData->dimensions[2] = nz;
          result = m_pData->createDataset();
        }
        else
          result = true;
        result = result && m_pData->writeVolume(volume, data);
      }

      if(result)
      {
        if(volume == count)
        {
          m_pData->frames.append(frame);
          m_pData->gates.append(gate);
          m_pData->beds.append(bed);
          m_pData->starts.append(0.0);
          m_pData->durations.append(0.0);
          m_pData->affines.resize((count + 1) * 12);
          m_pData->frameObjects.append(QJsonObject());
        }
        m_pData->starts[volume] = frameHeader.frameStart();
        m_pData->durations[volume] = frameHeader.frameDuration();
        for(int i = 0; i < 12; i++)
          m_pData->affines[volume * 12 + i] = m[i / 4][i % 4];
        QJsonObject attributes = frameHeader.attributes();
        if(attributes.isEmpty())
          attributes = m_pData->frameDefaults.at(volume).toObject();
        m_pData->frameObjects[volume] = attributes;
      }
      else
      {
        // the file cannot be completed anymore
        m_pData->failed = true;
        E("cannot write volume %d of the file %s", volume, qPrintable(fileName()));
      }
    }
  }
#else
  Q_UNUSED(data);
  Q_UNUSED(frameHeader);
  Q_UNUSED(frame);
#endif

  m_iErrStatus = result ? MERR_Ok : MERR_Unspecified;
  RETURN(result);
  return result;
}
