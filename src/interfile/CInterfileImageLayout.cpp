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

//! @file CInterfileImageLayout.cpp
//! @brief private: the image data described by an Interfile header (see CInterfileImageLayout.h)

#include "CInterfileImageLayout.h"
#include "CInterfileKeys.h"

#include <QDir>
#include <QFileInfo>

#include <rtdebug.h>

namespace {

// a range of header lines [begin, end) in which keys are looked up
struct Scope
{
  int begin;
  int end;
};

// the value of the first of the keys (normalized names) with the indices, looked up
// in the scopes in their order; a null string if none is given
QString lookup(const CInterfileKeys& keys, const QList<Scope>& scopes, const QStringList& names,
               const QList<int>& indices = QList<int>())
{
  foreach(const Scope& scope, scopes)
  {
    foreach(const QString& name, names)
    {
      const int i = keys.indexOf(name, indices, scope.begin);
      if(i >= 0 && i < scope.end)
      {
        const QString value = keys.at(i).value;
        return value.isNull() ? QString("") : value;
      }
    }
  }

  return QString();
}

QString lookup(const CInterfileKeys& keys, const QList<Scope>& scopes, const QString& name,
               const QList<int>& indices = QList<int>())
{
  return lookup(keys, scopes, QStringList() << name, indices);
}

// a number given for a key; false (and error set) if the value is no number
bool number(const QString& value, const QString& key, double* result, QString* error)
{
  if(CInterfileKeys::toDouble(value, result))
    return true;

  *error = QString("the value '%1' of \"%2\" is no number").arg(value, key);
  return false;
}

bool integer(const QString& value, const QString& key, qint64* result, QString* error)
{
  if(CInterfileKeys::toInt(value, result))
    return true;

  // integers are also written as "37.0"
  double d = 0.0;
  if(CInterfileKeys::toDouble(value, &d) && d == qint64(d))
  {
    *result = qint64(d);
    return true;
  }

  *error = QString("the value '%1' of \"%2\" is no integer").arg(value, key);
  return false;
}

// the number format, size of a value, matrix size, voxel size and origin of the
// frames described in the scopes
bool readImage(const CInterfileKeys& keys, const QList<Scope>& scopes, CInterfileFrame& frame, QString* error)
{
  // number format and size of a value (Interfile 3.3: unsigned integer if not given)
  const QString format = lookup(keys, scopes, "number format").toLower().simplified();
  const QString bytes = lookup(keys, scopes, "number of bytes per pixel");
  qint64 size = 0;
  if(bytes.isNull() == false && integer(bytes, "number of bytes per pixel", &size, error) == false)
    return false;

  if(format.isEmpty() || format == "unsigned integer")
    frame.numberFormat = CInterfileFrame::UnsignedInteger;
  else if(format == "signed integer")
    frame.numberFormat = CInterfileFrame::SignedInteger;
  else if(format == "float" || format == "short float" || format == "long float")
  {
    frame.numberFormat = CInterfileFrame::Float;
    if(size == 0)
      size = (format == "long float") ? 8 : 4;
  }
  else
  {
    *error = QString("the number format '%1' is not supported").arg(format);
    return false;
  }

  const bool integerFormat = frame.numberFormat != CInterfileFrame::Float;
  if((integerFormat && size != 1 && size != 2 && size != 4 && size != 8) ||
     (integerFormat == false && size != 4 && size != 8))
  {
    *error = (size == 0) ? QString("no \"number of bytes per pixel\" given")
                         : QString("%1 bytes per pixel are not supported for the number format '%2'")
                               .arg(size).arg(format.isEmpty() ? QString("unsigned integer") : format);
    return false;
  }
  frame.bytesPerPixel = int(size);

  // matrix size: columns, rows and planes; without "matrix size [3]" the planes are
  // given by "number of slices" (Interfile 3.3 tomographic data)
  frame.dims = QVector<int>(3, 1);
  for(int i = 1; i <= 3; i++)
  {
    QString value = lookup(keys, scopes, "matrix size", QList<int>() << i);
    if(value.isNull() && i == 3)
      value = lookup(keys, scopes, "number of slices");
    if(value.isNull())
    {
      if(i < 3)
      {
        *error = QString("no \"matrix size [%1]\" given").arg(i);
        return false;
      }
      continue;
    }
    if(value.trimmed().startsWith('{'))
    {
      *error = QString("a matrix size of different values per plane ('%1') is not supported").arg(value);
      return false;
    }
    qint64 n = 0;
    if(integer(value, QString("matrix size [%1]").arg(i), &n, error) == false)
      return false;
    if(n < 1 || n > 0x7fffffff)
    {
      *error = QString("the matrix size [%1] %2 is invalid").arg(i).arg(n);
      return false;
    }
    frame.dims[i - 1] = int(n);
  }

  // more dimensions (e.g. sinograms, gates) are not supported for images
  for(int i = 4; i <= 8; i++)
  {
    const QString value = lookup(keys, scopes, "matrix size", QList<int>() << i);
    qint64 n = 1;
    if(value.isNull() == false && (CInterfileKeys::toInt(value, &n) == false || n != 1))
    {
      *error = QString("the matrix size [%1] '%2' (more than 3 dimensions) is not supported").arg(i).arg(value);
      return false;
    }
  }

  // voxel size in mm ("scale factor" of Siemens); the plane distance of Interfile 3.3
  // tomographic data may be given as slice thickness in pixels
  frame.voxelSize = QVector<double>(3, 1.0);
  for(int i = 1; i <= 3; i++)
  {
    const QList<int> index = QList<int>() << i;
    const QString value = lookup(keys, scopes, QStringList() << "scaling factor (mm/pixel)"
                                                             << "scale factor (mm/pixel)", index);
    if(value.isNull() == false)
    {
      if(number(value, QString("scaling factor (mm/pixel) [%1]").arg(i), &frame.voxelSize[i - 1], error) == false)
        return false;
    }
    else if(i == 3)
    {
      const QString thickness = lookup(keys, scopes, "slice thickness (pixels)");
      double pixels = 0.0;
      if(thickness.isNull() == false)
      {
        if(number(thickness, "slice thickness (pixels)", &pixels, error) == false)
          return false;
        frame.voxelSize[2] = pixels * frame.voxelSize[0];
      }
    }
  }

  // position of the first voxel (STIR: "first pixel offset (mm)", PMOD: "origin (mm)")
  frame.origin = QVector<double>(3, 0.0);
  frame.hasOrigin = false;
  for(int i = 1; i <= 3; i++)
  {
    const QString value = lookup(keys, scopes, QStringList() << "first pixel offset (mm)" << "origin (mm)",
                                 QList<int>() << i);
    if(value.isNull())
      continue;
    if(number(value, QString("first pixel offset (mm) [%1]").arg(i), &frame.origin[i - 1], error) == false)
      return false;
    frame.hasOrigin = true;
  }

  return true;
}

// scaling and units of the frame with the (1 based) number f, looked up in the scopes
bool readScaling(const CInterfileKeys& keys, const QList<Scope>& scopes, int f, CInterfileFrame& frame, QString* error)
{
  const QList<int> index = QList<int>() << f;

  // PMOD: physical value = stored value * data rescale slope + data rescale offset
  frame.slope = 1.0;
  frame.intercept = 0.0;
  const QString slope = lookup(keys, scopes, "data rescale slope");
  if(slope.isNull() == false && number(slope, "data rescale slope", &frame.slope, error) == false)
    return false;
  const QString intercept = lookup(keys, scopes, "data rescale offset");
  if(intercept.isNull() == false && number(intercept, "data rescale offset", &frame.intercept, error) == false)
    return false;

  // Siemens: physical value = stored value * %image slope + %image intercept
  const QString siemensSlope = lookup(keys, scopes, "image slope");
  const QString siemensIntercept = lookup(keys, scopes, "image intercept");
  double value = 0.0;
  if(siemensSlope.isNull() == false)
  {
    if(number(siemensSlope, "image slope", &value, error) == false)
      return false;
    frame.intercept *= value;
    frame.slope *= value;
  }
  if(siemensIntercept.isNull() == false)
  {
    if(number(siemensIntercept, "image intercept", &value, error) == false)
      return false;
    frame.intercept += value;
  }

  // PET proposal/STIR: physical value = stored value * image scaling factor[f]
  QString factor = lookup(keys, scopes, "image scaling factor", index);
  if(factor.isNull())
    factor = lookup(keys, scopes, "image scaling factor");
  double scale = 1.0;
  if(factor.isNull() == false && number(factor, "image scaling factor", &scale, error) == false)
    return false;

  // "quantification units" is a factor in the files of STIR (Louvain-la-Neuve reader),
  // equal to the image scaling factor, and the units in the PET proposal
  const QString quantification = lookup(keys, scopes, "quantification units");
  double quantificationFactor = 0.0;
  if(quantification.isNull() == false && CInterfileKeys::toDouble(quantification, &quantificationFactor))
  {
    if(factor.isNull())
      scale = quantificationFactor;
  }
  else if(quantification.isEmpty() == false)
    frame.units = quantification;

  frame.slope *= scale;
  frame.intercept *= scale;

  const QString units = lookup(keys, scopes, "units");
  if(units.isEmpty() == false)
    frame.units = units;

  return true;
}

// the (relative) start time and duration of frame f; a start time not given is the
// end of the previous frame
bool readTiming(const CInterfileKeys& keys, const QList<Scope>& scopes, int f, double previousEnd,
                CInterfileFrame& frame, QString* error)
{
  const QList<int> index = QList<int>() << f;

  QString duration = lookup(keys, scopes, "image duration (sec)", index);
  if(duration.isNull() && f == 1)
    duration = lookup(keys, scopes, "image duration (sec)");
  frame.duration = 0.0;
  if(duration.isNull() == false && number(duration, "image duration (sec)", &frame.duration, error) == false)
    return false;

  QString start = lookup(keys, scopes, "image relative start time (sec)", index);
  if(start.isNull() && f == 1)
    start = lookup(keys, scopes, "image relative start time (sec)");
  frame.startTime = previousEnd;
  if(start.isNull() == false && number(start, "image relative start time (sec)", &frame.startTime, error) == false)
    return false;

  return true;
}

// true for a main header of data sets (Siemens .mhdr), reason names its first data set
bool isMainHeader(const CInterfileKeys& keys, QString* reason)
{
  for(int i = 0; i < keys.count(); i++)
  {
    const CInterfileKey& key = keys.at(i);
    if(key.comment == false && key.name == "data set" && key.indices.isEmpty() == false)
    {
      *reason = QString("a main header of data sets (e.g. \"data set[%1] := %2\"), the image data is described "
                        "by the headers of the data sets").arg(key.indices.first()).arg(key.value);
      return true;
    }
  }

  return false;
}

} // namespace

//-----------------------------------------------------------------------------------------

CInterfileFrame::CInterfileFrame()
  : numberFormat(UnsignedInteger), bytesPerPixel(0), dims(3, 1), voxelSize(3, 1.0), hasOrigin(false),
    origin(3, 0.0), startTime(0.0), duration(0.0), slope(1.0), intercept(0.0), offset(0), groupBegin(-1),
    groupEnd(-1)
{
}

qint64 CInterfileFrame::voxelCount() const
{
  qint64 count = 1;
  foreach(int n, dims)
    count *= n;

  return count;
}

qint64 CInterfileFrame::byteSize() const
{
  return voxelCount() * bytesPerPixel;
}

CInterfileImageLayout::CInterfileImageLayout()
  : dataInHeaderFile(false), bigEndian(true)
{
}

qint64 CInterfileImageLayout::requiredDataSize() const
{
  qint64 size = 0;
  foreach(const CInterfileFrame& frame, frames)
    size = qMax(size, frame.offset + frame.byteSize());

  return size;
}

bool CInterfileImageLayout::read(const CInterfileKeys& keys, const QString& headerFileName, QString* error)
{
  ENTER();

  QString reason;
  frames.clear();

  // the frame groups of dynamic Interfile 3.3 studies: from each "frame group number"
  // to the next one; the keys before the first group apply to all groups
  QList<Scope> groups;
  for(int i = keys.indexOf("frame group number"); i >= 0; i = keys.indexOf("frame group number", QList<int>(), i + 1))
  {
    if(groups.isEmpty() == false)
      groups.last().end = i;
    Scope group = { i, keys.count() };
    groups.append(group);
  }
  const Scope general = { 0, groups.isEmpty() ? keys.count() : groups.first().begin };
  const QList<Scope> generalScopes = QList<Scope>() << general;

  // a main header of Siemens (.mhdr) names the headers and data files of its data
  // sets ("%data set[1] := {0, <header>, <data file>}") and describes no image data
  if(isMainHeader(keys, &reason))
  {
    if(error)
      *error = reason;
    RETURN(false);
    return false;
  }

  versionOfKeys = lookup(keys, generalScopes, "version of keys");
  typeOfData = lookup(keys, generalScopes, "type of data").toLower().simplified();

  // byte order (Interfile 3.3: big endian if not given; Siemens: "image data byte order")
  const QString byteOrder = lookup(keys, generalScopes, QStringList() << "imagedata byte order"
                                                                      << "image data byte order")
                                .toUpper().simplified();
  if(byteOrder.isEmpty() || byteOrder == "BIGENDIAN")
    bigEndian = true;
  else if(byteOrder == "LITTLEENDIAN")
    bigEndian = false;
  else
  {
    reason = QString("the byte order '%1' is not supported").arg(byteOrder);
    goto failure;
  }

  {
    // the data file: relative to the header, the header file itself if none is named
    const QFileInfo header(headerFileName);
    const QString name = lookup(keys, generalScopes, "name of data file");
    dataInHeaderFile = name.isEmpty();
    if(dataInHeaderFile)
      dataFileName = header.filePath();
    else
    {
      const QFileInfo data(QDir(header.path()), name);
      dataFileName = data.filePath();
      // a path of another machine: the file next to the header
      if(data.exists() == false)
      {
        const QFileInfo besideHeader(QDir(header.path()), QFileInfo(QString(name).replace('\\', '/')).fileName());
        if(besideHeader.exists())
          dataFileName = besideHeader.filePath();
      }
    }

    // offset of the data in the data file
    qint64 offset = 0;
    const QString bytes = lookup(keys, generalScopes, "data offset in bytes");
    const QString block = lookup(keys, generalScopes, "data starting block");
    if(bytes.isEmpty() == false)
    {
      if(integer(bytes, "data offset in bytes", &offset, &reason) == false)
        goto failure;
    }
    else if(block.isEmpty() == false)
    {
      if(integer(block, "data starting block", &offset, &reason) == false)
        goto failure;
      offset *= 2048;
    }
    else if(dataInHeaderFile && keys.contains("data offset in bytes", QList<int>() << 1) == false)
    {
      reason = "the data is in the header file, but no \"data offset in bytes\" or \"data starting block\" is given";
      goto failure;
    }
    if(offset < 0)
    {
      reason = QString("the data offset %1 is invalid").arg(offset);
      goto failure;
    }

    double end = 0.0;
    if(groups.isEmpty())
    {
      // static, tomographic or PET data: "number of time frames" frames with the same
      // matrix, vectored keys per frame (PET proposal, STIR)
      qint64 count = 1;
      const QString countValue = lookup(keys, generalScopes, "number of time frames");
      if(countValue.isEmpty() == false && integer(countValue, "number of time frames", &count, &reason) == false)
        goto failure;
      if(count < 1)
      {
        reason = QString("the number of time frames %1 is invalid").arg(count);
        goto failure;
      }

      CInterfileFrame image;
      if(readImage(keys, generalScopes, image, &reason) == false)
        goto failure;

      for(int f = 1; f <= count; f++)
      {
        CInterfileFrame frame = image;
        if(readScaling(keys, generalScopes, f, frame, &reason) == false ||
           readTiming(keys, generalScopes, f, end, frame, &reason) == false)
          goto failure;

        const QString frameOffset = lookup(keys, generalScopes, "data offset in bytes", QList<int>() << f);
        if(frameOffset.isEmpty() == false)
        {
          if(integer(frameOffset, QString("data offset in bytes[%1]").arg(f), &offset, &reason) == false)
            goto failure;
        }
        frame.offset = offset;
        offset += frame.byteSize();
        end = frame.startTime + frame.duration;
        frames.append(frame);
      }
    }
    else
    {
      // dynamic Interfile 3.3 data: frame groups of frames with the same duration
      foreach(const Scope& group, groups)
      {
        const QList<Scope> scopes = QList<Scope>() << group << general;

        CInterfileFrame image;
        if(readImage(keys, scopes, image, &reason) == false ||
           readScaling(keys, scopes, 1, image, &reason) == false)
          goto failure;

        // "number of images this frame group" counts planes (tomographic data) or
        // 2D images (planar data)
        qint64 images = image.dims[2];
        const QString imagesValue = lookup(keys, scopes, "number of images this frame group");
        if(imagesValue.isEmpty() == false &&
           integer(imagesValue, "number of images this frame group", &images, &reason) == false)
          goto failure;
        if(images < 1 || images % image.dims[2] != 0)
        {
          reason = QString("%1 images in a frame group of %2 planes").arg(images).arg(image.dims[2]);
          goto failure;
        }

        double duration = 0.0;
        double pause = 0.0;
        double groupPause = 0.0;
        const QString durationValue = lookup(keys, scopes, "image duration (sec)");
        const QString pauseValue = lookup(keys, scopes, "pause between images (sec)");
        const QString groupPauseValue = lookup(keys, scopes, "pause between frame groups (sec)");
        if((durationValue.isEmpty() == false && number(durationValue, "image duration (sec)", &duration, &reason) == false) ||
           (pauseValue.isEmpty() == false && number(pauseValue, "pause between images (sec)", &pause, &reason) == false) ||
           (groupPauseValue.isEmpty() == false &&
            number(groupPauseValue, "pause between frame groups (sec)", &groupPause, &reason) == false))
          goto failure;

        for(qint64 i = 0; i < images / image.dims[2]; i++)
        {
          CInterfileFrame frame = image;
          frame.groupBegin = group.begin;
          frame.groupEnd = group.end;
          frame.startTime = (i == 0) ? end : end + pause;
          frame.duration = duration;
          frame.offset = offset;
          offset += frame.byteSize();
          end = frame.startTime + frame.duration;
          frames.append(frame);
        }
        end += groupPause;
      }
    }
  }

  RETURN(true);
  return true;

failure:
  frames.clear();
  if(error)
    *error = reason;
  RETURN(false);
  return false;
}
