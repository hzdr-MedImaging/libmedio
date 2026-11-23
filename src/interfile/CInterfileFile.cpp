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

//! @file CInterfileFile.cpp
//! @brief contains the implementation of the class CInterfileFile

#include "CInterfileFile.h"
#include "CInterfileFrameHeader.h"
#include "CInterfileHeaderMapping.h"
#include "CInterfileImageLayout.h"
#include "CInterfileKeys.h"
#include "CInterfileMainHeader.h"
#include "CMedIOHeaderMapping.h"
#include "MedIOLimits.h"

#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QSysInfo>

#include <cmath>

#include <rtdebug.h>

namespace {

// the header and the description of the image data of a data set: the image header of
// a Siemens main header, or the header that was opened
struct DataSet
{
  QString headerFile;
  CInterfileKeys keys;
  CInterfileImageLayout layout;
};

// a frame of the file: frame index of the layout of a data set
struct FrameRef
{
  int set;
  int index;
};

// true for a main header of data sets (Siemens .mhdr)
bool isMainHeader(const CInterfileKeys& keys)
{
  for(int i = 0; i < keys.count(); i++)
  {
    if(keys.at(i).comment == false && keys.at(i).name == "data set" && keys.at(i).indices.size() == 1)
      return true;
  }

  return false;
}

bool sameFile(const QString& a, const QString& b)
{
  const QString pathA = QFileInfo(a).canonicalFilePath();
  return pathA.isEmpty() == false && pathA == QFileInfo(b).canonicalFilePath();
}

// read the header of a data set and the description of its image data
bool readDataSet(const QString& headerFile, DataSet& set, QString* error)
{
  set.headerFile = headerFile;
  if(set.keys.read(headerFile, error) == false || set.layout.read(set.keys, headerFile, error) == false)
    return false;

  const qint64 size = QFileInfo(set.layout.dataFileName).size();
  if(QFileInfo(set.layout.dataFileName).isFile() == false)
  {
    *error = QString("the data file '%1' of '%2' does not exist").arg(set.layout.dataFileName, headerFile);
    return false;
  }
  if(size < set.layout.requiredDataSize())
  {
    *error = QString("the data file '%1' has %2 bytes, the header '%3' describes %4 bytes")
                 .arg(set.layout.dataFileName).arg(size).arg(headerFile).arg(set.layout.requiredDataSize());
    return false;
  }

  return true;
}

// the header of a file: the file itself if it is an Interfile header, otherwise a header
// next to it naming it as its data file
QString headerOf(const QString& fileName)
{
  if(CInterfileKeys::isInterfile(fileName))
    return fileName;
  if(QFileInfo(fileName).isFile() == false)
    return QString();

  const QFileInfo info(fileName);
  const QString base = info.dir().filePath(info.completeBaseName());
  const QStringList candidates = QStringList() << fileName + ".hdr" << fileName + ".HDR" << base + ".hdr"
                                               << base + ".HDR" << base + ".hv" << base + ".h33";
  foreach(const QString& candidate, candidates)
  {
    if(QFileInfo(candidate).isFile() == false || sameFile(candidate, fileName) ||
       CInterfileKeys::isInterfile(candidate) == false)
      continue;

    CInterfileKeys keys;
    CInterfileImageLayout layout;
    if(keys.read(candidate) && isMainHeader(keys) == false && layout.read(keys, candidate) &&
       layout.dataInHeaderFile == false && sameFile(layout.dataFileName, fileName))
      return candidate;
  }

  return QString();
}

CMedIOVolume::DataType dataTypeOf(const CInterfileFrame& frame)
{
  switch(frame.numberFormat)
  {
    case CInterfileFrame::SignedInteger:
      switch(frame.bytesPerPixel)
      {
        case 1: return CMedIOVolume::Int8;
        case 2: return CMedIOVolume::Int16;
        case 4: return CMedIOVolume::Int32;
        default: break;
      }
    break;

    case CInterfileFrame::UnsignedInteger:
      switch(frame.bytesPerPixel)
      {
        case 1: return CMedIOVolume::UInt8;
        case 2: return CMedIOVolume::UInt16;
        case 4: return CMedIOVolume::UInt32;
        default: break;
      }
    break;

    case CInterfileFrame::Float:
      switch(frame.bytesPerPixel)
      {
        case 4: return CMedIOVolume::Float32;
        case 8: return CMedIOVolume::Float64;
        default: break;
      }
    break;
  }

  return CMedIOVolume::UnknownDataType;
}

double numberOf(const CInterfileKeys& keys, const QString& name, const QList<int>& indices, double defaultValue)
{
  double value = defaultValue;
  const QString text = keys.value(name, indices);
  if(text.isEmpty() || CInterfileKeys::toDouble(text, &value) == false || std::isfinite(value) == false)
    return defaultValue;

  return value;
}

// the position of the voxels (see CInterfileFile.h and docs/interfile.md)
void geometry(const CInterfileKeys& keys, const CInterfileFrame& frame, double m[3][4])
{
  const int nx = frame.dims[0];
  const int ny = frame.dims[1];
  const int nz = frame.dims[2];
  const double dx = frame.voxelSize[0];
  const double dy = frame.voxelSize[1];
  const double dz = frame.voxelSize[2];

  for(int i = 0; i < 3; i++)
    for(int j = 0; j < 4; j++)
      m[i][j] = 0.0;

  // Siemens: "%image info[n]" = {n - 1, z, x, y, ...} of the planes; plane k of the
  // data file is "%image info[planes - k]"
  QVector<double> x(nz, 0.0);
  QVector<double> y(nz, 0.0);
  QVector<double> z(nz, 0.0);
  QVector<bool> given(nz, false);
  for(int i = 0; i < keys.count(); i++)
  {
    const CInterfileKey& key = keys.at(i);
    if(key.comment || key.name != "image info" || key.indices.size() != 1)
      continue;
    const int n = key.indices.first();
    const QStringList elements = CInterfileKeys::toList(key.value);
    if(n < 1 || n > nz || elements.size() < 4)
      continue;
    const int k = nz - n;
    given[k] = CInterfileKeys::toDouble(elements[1], &z[k]) && CInterfileKeys::toDouble(elements[2], &x[k]) &&
               CInterfileKeys::toDouble(elements[3], &y[k]);
  }

  const bool siemens = keys.contains("sms-mi header name space");
  if(given.contains(false) == false)
  {
    // the axes of the rows and columns ("%image orientation", DICOM Image Orientation)
    double row[3] = { 1.0, 0.0, 0.0 };
    double column[3] = { 0.0, 1.0, 0.0 };
    const QStringList orientation = CInterfileKeys::toList(keys.value("image orientation"));
    double values[6];
    bool valid = orientation.size() == 6;
    for(int i = 0; valid && i < 6; i++)
      valid = CInterfileKeys::toDouble(orientation[i], &values[i]);
    if(valid)
    {
      for(int i = 0; i < 3; i++)
      {
        row[i] = values[i];
        column[i] = values[3 + i];
      }
    }

    const double step = (nz > 1) ? (z[nz - 1] - z[0]) / (nz - 1) : -dz;
    for(int i = 0; i < 3; i++)
    {
      m[i][0] = row[i] * dx;
      m[i][1] = column[i] * dy;
    }
    m[2][2] = step;
    m[0][3] = x[0];
    m[1][3] = y[0];
    m[2][3] = z[0];
    return;
  }

  if(siemens)
  {
    // without "%image info": as found in the sample of docs/interfile.md
    const double gantryX = numberOf(keys, "gantry offset (mm)", QList<int>() << 1, 0.0);
    const double gantryY = numberOf(keys, "gantry offset (mm)", QList<int>() << 2, 0.0);
    const double bedY = numberOf(keys, "start vertical bed position (mm)", QList<int>(), 0.0);
    const double bedZ = numberOf(keys, "start horizontal bed position (mm)", QList<int>(), 0.0);
    m[0][0] = dx;
    m[1][1] = dy;
    m[2][2] = -dz;
    m[0][3] = -nx / 2.0 * dx - gantryX;
    m[1][3] = -ny / 2.0 * dy - bedY - gantryY;
    m[2][3] = bedZ + (nz - 1) * dz;
    return;
  }

  // other variants: towards patient left, posterior and superior, centered at 0
  m[0][0] = dx;
  m[1][1] = dy;
  m[2][2] = dz;
  m[0][3] = -(nx - 1) / 2.0 * dx;
  m[1][3] = -(ny - 1) / 2.0 * dy;
  m[2][3] = -(nz - 1) / 2.0 * dz;
}

void swapBytes(QByteArray& data, int bytesPerValue)
{
  if(bytesPerValue < 2)
    return;

  char* p = data.data();
  for(int i = 0; i + bytesPerValue <= data.size(); i += bytesPerValue)
  {
    for(int a = 0, b = bytesPerValue - 1; a < b; a++, b--)
      qSwap(p[i + a], p[i + b]);
  }
}

} // namespace

//=============================================================================================

class CInterfileFilePrivate
{
  public:
    CInterfileFilePrivate() : mainHeader(false) { }

    QString headerFile;          //!< the header that was opened
    CInterfileKeys mainKeys;     //!< its keys
    bool mainHeader;             //!< a Siemens main header of data sets
    QList<DataSet> sets;
    QList<FrameRef> frames;
    QString error;

    void clear()
    {
      headerFile.clear();
      mainKeys.clear();
      mainHeader = false;
      sets.clear();
      frames.clear();
    }

    // the range of the general keys of the main keys (before the first frame group)
    int generalEnd() const
    {
      if(mainHeader == false && sets.isEmpty() == false && sets.first().layout.frames.isEmpty() == false &&
         sets.first().layout.frames.first().groupBegin >= 0)
        return sets.first().layout.frames.first().groupBegin;
      return mainKeys.count();
    }
};

CInterfileFile::CInterfileFile(const QString& fileName)
  : CMedIOData(fileName), m_pData(new CInterfileFilePrivate())
{
}

CInterfileFile::~CInterfileFile()
{
  close();
  delete m_pData;
}

bool CInterfileFile::isOfType(const QString& fileName)
{
  ENTER();

  CInterfileFile file(fileName);
  const bool result = file.open(QIODevice::ReadOnly);
  file.close();

  RETURN(result);
  return result;
}

CMedIOData* CInterfileFile::createFromFile(const QString& fileName)
{
  ENTER();

  CMedIOData* result = isOfType(fileName) ? new CInterfileFile(fileName) : NULL;

  RETURN(result);
  return result;
}

bool CInterfileFile::open(QIODevice::OpenModeFlag mode)
{
  ENTER();

  if(isOpen())
    close();
  m_pData->clear();
  m_pData->error.clear();

  QString error;
  bool result = false;

  if(mode != QIODevice::ReadOnly)
    error = "Interfile files can be read only";
  else if((m_pData->headerFile = headerOf(fileName())).isEmpty())
    error = QString("'%1' is no Interfile header and no header next to it names it as data file").arg(fileName());
  else if(m_pData->mainKeys.read(m_pData->headerFile, &error))
  {
    result = true;
    m_pData->mainHeader = isMainHeader(m_pData->mainKeys);

    if(m_pData->mainHeader)
    {
      // Siemens: "%data set[n] := {0, <image header>, <data file>}" in the order of n
      QMap<int, QString> headers;
      for(int i = 0; i < m_pData->mainKeys.count(); i++)
      {
        const CInterfileKey& key = m_pData->mainKeys.at(i);
        if(key.comment || key.name != "data set" || key.indices.size() != 1)
          continue;
        const QStringList elements = CInterfileKeys::toList(key.value);
        if(elements.size() < 2 || elements[1].isEmpty())
        {
          error = QString("invalid data set '%1' in '%2'").arg(key.value, m_pData->headerFile);
          result = false;
          break;
        }
        headers[key.indices.first()] = QFileInfo(m_pData->headerFile).dir().filePath(elements[1]);
      }

      foreach(const QString& header, headers)
      {
        if(result == false)
          break;
        DataSet set;
        result = readDataSet(header, set, &error);
        if(result)
          m_pData->sets.append(set);
      }
    }
    else
    {
      DataSet set;
      result = readDataSet(m_pData->headerFile, set, &error);
      if(result)
        m_pData->sets.append(set);
    }
  }

  for(int s = 0; result && s < m_pData->sets.size(); s++)
  {
    for(int f = 0; f < m_pData->sets[s].layout.frames.size(); f++)
    {
      const FrameRef ref = { s, f };
      m_pData->frames.append(ref);
    }
  }

  if(result && (m_pData->frames.isEmpty() || m_pData->frames.size() > 32767))
  {
    error = QString("'%1' describes %2 frames").arg(m_pData->headerFile).arg(m_pData->frames.size());
    result = false;
  }

  if(result && QFile::open(QIODevice::ReadOnly) == false)
  {
    error = QString("cannot open '%1': %2").arg(fileName(), QFile::errorString());
    result = false;
  }

  if(result == false)
  {
    m_pData->clear();
    m_pData->error = error;
    D("%s", qPrintable(error));
  }

  m_iErrStatus = result ? MERR_Ok : MERR_Unspecified;
  RETURN(result);
  return result;
}

void CInterfileFile::close()
{
  ENTER();

  QFile::close();
  m_pData->clear();

  LEAVE();
}

CMedIOData::Format CInterfileFile::dataFormat() const
{
  return CMedIOData::Interfile;
}

QString CInterfileFile::headerFileName() const
{
  return m_pData->headerFile;
}

QStringList CInterfileFile::allFileNames() const
{
  QStringList names;
  if(m_pData->headerFile.isEmpty() == false)
    names << m_pData->headerFile;
  foreach(const DataSet& set, m_pData->sets)
  {
    if(names.contains(set.headerFile) == false)
      names << set.headerFile;
    if(names.contains(set.layout.dataFileName) == false)
      names << set.layout.dataFileName;
  }

  return names;
}

QString CInterfileFile::openError() const
{
  return m_pData->error;
}

int CInterfileFile::numFrames() const
{
  return isOpen() ? m_pData->frames.size() : 0;
}

bool CInterfileFile::readMainHeader(CInterfileMainHeader*& mainHeader)
{
  ENTER();
  mainHeader = NULL;

  if(isOpen() == false || m_pData->sets.isEmpty())
  {
    E("the file is not open");
    m_iErrStatus = MERR_Unspecified;
    RETURN(false);
    return false;
  }

  mainHeader = new CInterfileMainHeader(this);
  mainHeader->setKeys(m_pData->mainKeys);
  mainHeader->setHeaderFileName(m_pData->headerFile);
  mainHeader->setNumFrames(m_pData->frames.size());

  double factor = 1.0;
  const QString units = InterfileMapping::units(m_pData->sets.first().layout.frames.first().units, &factor);
  const QJsonObject attributes =
      InterfileMapping::mainAttributes(m_pData->mainKeys, 0, m_pData->generalEnd(),
                                       m_pData->mainHeader ? &m_pData->sets.first().keys : NULL, units);
  MedIOHeaderMapping::transfer(attributes, *mainHeader);

  m_iErrStatus = MERR_Ok;
  RETURN(true);
  return true;
}

bool CInterfileFile::readSubHeader(CInterfileFrameHeader*& subHeader, short frame)
{
  ENTER();
  subHeader = NULL;

  if(frame == -1)
    frame = 1;
  if(isOpen() == false || frame < 1 || frame > m_pData->frames.size())
  {
    E("frame %d does not exist (%d frames)", frame, numFrames());
    m_iErrStatus = MERR_Unspecified;
    RETURN(false);
    return false;
  }

  const FrameRef ref = m_pData->frames.at(frame - 1);
  const DataSet& set = m_pData->sets.at(ref.set);
  const CInterfileFrame& f = set.layout.frames.at(ref.index);

  subHeader = new CInterfileFrameHeader(this, frame);
  subHeader->setKeys(set.keys);
  subHeader->setHeaderFileName(set.headerFile);
  subHeader->setDataFileName(set.layout.dataFileName);
  subHeader->setDimensions(f.dims[0], f.dims[1], f.dims[2]);
  subHeader->setDataType(dataTypeOf(f));

  double factor = 1.0;
  subHeader->setUnits(InterfileMapping::units(f.units, &factor));
  subHeader->setScaling(f.slope * factor, f.intercept * factor);

  double m[3][4];
  geometry(set.keys, f, m);
  subHeader->setAffine(m);
  subHeader->setTiming(f.startTime, f.duration);

  // the keys of the frame: the image header of a data set, the frame group
  int begin = 0;
  int end = 0;
  if(m_pData->mainHeader)
    end = set.keys.count();
  else if(f.groupBegin >= 0)
  {
    begin = f.groupBegin;
    end = f.groupEnd;
  }
  MedIOHeaderMapping::transfer(InterfileMapping::frameAttributes(set.keys, begin, end, f.duration), *subHeader);

  m_iErrStatus = MERR_Ok;
  RETURN(true);
  return true;
}

bool CInterfileFile::readMatrix(QByteArray*& matrixData, CInterfileFrameHeader*& subHeader, short frame)
{
  ENTER();
  matrixData = NULL;

  if(readSubHeader(subHeader, frame) == false)
  {
    RETURN(false);
    return false;
  }

  const FrameRef ref = m_pData->frames.at(subHeader->frame() - 1);
  const DataSet& set = m_pData->sets.at(ref.set);
  const CInterfileFrame& f = set.layout.frames.at(ref.index);

  bool result = false;
  if(dataTypeOf(f) == CMedIOVolume::UnknownDataType)
    E("unsupported number format of frame %d", subHeader->frame());
  else if(MedIOLimits::fitsByteArray(f.byteSize()) == false)
    E("frame %d (%lld bytes) is too large for a QByteArray", subHeader->frame(), f.byteSize());
  else
  {
    QFile data(set.layout.dataFileName);
    if(data.open(QIODevice::ReadOnly) == false || data.seek(f.offset) == false)
      E("cannot read '%s'", qPrintable(set.layout.dataFileName));
    else
    {
      matrixData = new QByteArray(data.read(f.byteSize()));
      if(matrixData->size() != f.byteSize())
        E("'%s' ends within frame %d", qPrintable(set.layout.dataFileName), subHeader->frame());
      else
      {
        if(set.layout.bigEndian != (QSysInfo::ByteOrder == QSysInfo::BigEndian))
          swapBytes(*matrixData, f.bytesPerPixel);
        result = true;
      }
    }
  }

  if(result == false)
  {
    delete matrixData;
    matrixData = NULL;
    delete subHeader;
    subHeader = NULL;
  }

  m_iErrStatus = result ? MERR_Ok : MERR_Unspecified;
  RETURN(result);
  return result;
}
