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

#include <algorithm>
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

  // Siemens: "%image info[n]" = {n - 1, z, x, y, ...} of the planes in the order of the
  // scan (from "start horizontal bed position (mm)"); the planes of the data file are
  // from the highest to the lowest z (verified with the DICOM series of samples scanned
  // in both directions, see docs/interfile.md): plane k is the entry with the k-th
  // highest z, "%image info[k + 1]" if z falls with n, otherwise "%image info[planes - k]"
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
    const int k = n - 1;
    given[k] = CInterfileKeys::toDouble(elements[1], &z[k]) && CInterfileKeys::toDouble(elements[2], &x[k]) &&
               CInterfileKeys::toDouble(elements[3], &y[k]);
  }
  if(given.contains(false) == false && nz > 1 && z[0] < z[nz - 1])
  {
    std::reverse(x.begin(), x.end());
    std::reverse(y.begin(), y.end());
    std::reverse(z.begin(), z.end());
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

    // the voxel size of the header (the z of "%image info" are rounded to 0.001 mm)
    const double step = -dz;
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
    // without "%image info": as found in the samples of docs/interfile.md; the planes from
    // the highest z, the start of the scan or its end (the bed moved towards higher z)
    const double gantryX = numberOf(keys, "gantry offset (mm)", QList<int>() << 1, 0.0);
    const double gantryY = numberOf(keys, "gantry offset (mm)", QList<int>() << 2, 0.0);
    const double bedY = numberOf(keys, "start vertical bed position (mm)", QList<int>(), 0.0);
    const double bedZ = numberOf(keys, "start horizontal bed position (mm)", QList<int>(), 0.0);
    const double bedEnd = numberOf(keys, "end horizontal bed position (mm)", QList<int>(), bedZ);
    m[0][0] = dx;
    m[1][1] = dy;
    m[2][2] = -dz;
    m[0][3] = -nx / 2.0 * dx - gantryX;
    m[1][3] = -ny / 2.0 * dy - bedY - gantryY;
    m[2][3] = (bedEnd > bedZ) ? bedZ + (nz - 1) * dz : bedZ;
    return;
  }

  // STIR: "first pixel offset (mm) [i]" is the position of the first voxel in the
  // coordinates of STIR, whose axes are the voxel axes; their directions in LPS depend
  // on the patient position (CInterfileImageLayout::stirSigns())
  if(keys.contains("first pixel offset (mm)", QList<int>() << 1) &&
     keys.contains("first pixel offset (mm)", QList<int>() << 2) &&
     keys.contains("first pixel offset (mm)", QList<int>() << 3))
  {
    double sign[3];
    if(CInterfileImageLayout::stirSigns(keys.value("patient orientation"), keys.value("patient rotation"), sign) == false)
      W("Interfile: unknown patient orientation/rotation '%s'/'%s', head_in/supine assumed",
        qPrintable(keys.value("patient orientation")), qPrintable(keys.value("patient rotation")));
    const double size[3] = { dx, dy, dz };
    for(int i = 0; i < 3; i++)
    {
      m[i][i] = sign[i] * size[i];
      m[i][3] = sign[i] * numberOf(keys, "first pixel offset (mm)", QList<int>() << (i + 1), 0.0);
    }
    return;
  }

  // other variants (Interfile 3.3, PMOD): towards patient left, posterior and superior,
  // centered at 0
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

namespace {

// a frame written into a new file
struct WrittenFrame
{
  WrittenFrame() : written(false), scale(1.0), start(0.0), duration(0.0) { }

  bool written;
  double scale;
  double start;
  double duration;
};

// the patient position (DICOM) of the main header of a new file, as orientation code
// of the voxel axes of STIR and the keys of STIR
struct StirPosition
{
  const char* dicom;
  const char* orientation;   // CMedIOVolume::orientation() of the voxel axes
  const char* patientOrientation;
  const char* patientRotation;
};

const StirPosition StirPositions[] = {
  { "HFS", "LPI", "head_in", "supine" },
  { "FFS", "RPS", "feet_in", "supine" },
  { "HFP", "RAI", "head_in", "prone" },
  { "FFP", "LAS", "feet_in", "prone" }
};

const StirPosition& stirPosition(const QString& dicom)
{
  for(unsigned i = 0; i < sizeof(StirPositions) / sizeof(StirPositions[0]); i++)
  {
    if(dicom == QLatin1String(StirPositions[i].dicom))
      return StirPositions[i];
  }
  return StirPositions[0];
}

QString numberText(double value)
{
  return QString::number(value, 'g', 15);
}

} // namespace

class CInterfileFilePrivate
{
  public:
    CInterfileFilePrivate()
      : writing(false), numFrames(0), dataType(CMedIOVolume::UnknownDataType), position(NULL), mainHeader(false)
    {
      for(int i = 0; i < 3; i++)
      {
        dims[i] = 0;
        for(int j = 0; j < 4; j++)
          affine[i][j] = 0.0;
      }
    }

    // writing
    bool writing;
    QFile data;                       //!< the data file of a new file
    int numFrames;                    //!< frames of the main header written (0: none written)
    QList<QPair<QString, QString> > attributeKeys;
    QString orientation;              //!< orientation code of the voxel axes
    QList<WrittenFrame> written;
    CMedIOVolume::DataType dataType;  //!< fixed by the first frame
    int dims[3];
    double affine[3][4];
    const StirPosition* position;

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
      writing = false;
      if(data.isOpen())
        data.close();
      numFrames = 0;
      attributeKeys.clear();
      orientation.clear();
      written.clear();
      dataType = CMedIOVolume::UnknownDataType;
      position = NULL;
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

  if(mode == QIODevice::WriteOnly)
  {
    // a new file: the header and the data file
    const QString data = dataFileNameOf(fileName());
    m_pData->data.setFileName(data);
    if(QFile::open(QIODevice::WriteOnly | QIODevice::Truncate) == false)
      error = QString("cannot create '%1': %2").arg(fileName(), QFile::errorString());
    else if(m_pData->data.open(QIODevice::ReadWrite | QIODevice::Truncate) == false)
    {
      error = QString("cannot create '%1': %2").arg(data, m_pData->data.errorString());
      QFile::close();
    }
    else
    {
      m_pData->writing = true;
      m_pData->headerFile = fileName();
      result = true;
    }

    if(result == false)
    {
      m_pData->clear();
      m_pData->error = error;
      E("%s", qPrintable(error));
    }
    m_iErrStatus = result ? MERR_Ok : MERR_Unspecified;
    RETURN(result);
    return result;
  }

  if(mode != QIODevice::ReadOnly)
    error = "Interfile files can be opened for reading (QIODevice::ReadOnly) or written as new files (QIODevice::WriteOnly)";
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

  if(m_pData->writing && m_pData->data.isOpen())
    m_pData->data.flush();
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

QString CInterfileFile::dataFileNameOf(const QString& headerFileName)
{
  const QFileInfo info(headerFileName);
  if(info.suffix().isEmpty() || info.suffix().toLower() == "v")
    return headerFileName + ".v";

  return info.dir().filePath(info.completeBaseName() + ".v");
}

QStringList CInterfileFile::allFileNames() const
{
  QStringList names;
  if(isOpen() == false || m_pData->writing)
    return QStringList() << fileName() << dataFileNameOf(fileName());

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

  const QString units = InterfileMapping::units(m_pData->sets.first().layout.frames.first().units);
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

  subHeader->setUnits(InterfileMapping::units(f.units));
  subHeader->setScaling(f.slope, f.intercept);

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

//=============================================================================================
// writing new files

CMedIOVolume::DataType CInterfileFile::writtenDataType() const
{
  return m_pData->dataType;
}

QString CInterfileFile::writtenOrientation() const
{
  return m_pData->orientation.isEmpty() ? QString("LPI") : m_pData->orientation;
}

bool CInterfileFile::writeMainHeader(const CInterfileMainHeader& mainHeader)
{
  ENTER();
  bool result = false;

  if(isOpen() == false || m_pData->writing == false)
    E("the file is not open for writing");
  else if(m_pData->numFrames > 0)
    E("the main header has been written already");
  else if(mainHeader.numFrames() < 1 || mainHeader.numFrames() > 32767)
    E("invalid number of frames %d", mainHeader.numFrames());
  else
  {
    const QJsonObject attributes = mainHeader.attributes();
    m_pData->numFrames = mainHeader.numFrames();
    m_pData->attributeKeys = InterfileMapping::keysOfAttributes(attributes);
    m_pData->position = &stirPosition(attributes.value("PatientPosition").toString());
    m_pData->orientation = QString::fromLatin1(m_pData->position->orientation);
    m_pData->written = QList<WrittenFrame>();
    for(int f = 0; f < m_pData->numFrames; f++)
      m_pData->written.append(WrittenFrame());
    result = writeHeader();
  }

  m_iErrStatus = result ? MERR_Ok : MERR_Unspecified;
  RETURN(result);
  return result;
}

bool CInterfileFile::writeHeader()
{
  ENTER();

  CInterfileKeys keys;
  keys.append("!INTERFILE", "");
  keys.append("!imaging modality", "PT");
  keys.append("!version of keys", "STIR6.0");
  keys.append("name of data file", QFileInfo(m_pData->data.fileName()).fileName());
  keys.appendComment("written by libmedio");
  keys.append("!GENERAL DATA", "");
  for(int i = 0; i < m_pData->attributeKeys.size(); i++)
    keys.append(m_pData->attributeKeys[i].first, m_pData->attributeKeys[i].second);
  keys.append("!GENERAL IMAGE DATA", "");
  keys.append("!type of data", "PET");
  keys.append("imagedata byte order", "LITTLEENDIAN");
  keys.append("!PET STUDY (General)", "");
  keys.append("!PET data type", "Image");
  keys.append("process status", "Reconstructed");

  // the matrix and its position (known after the first frame)
  if(m_pData->dataType != CMedIOVolume::UnknownDataType)
  {
    const bool integer = (m_pData->dataType == CMedIOVolume::Int16);
    keys.append("!number format", integer ? "signed integer" : "float");
    keys.append("!number of bytes per pixel", integer ? "2" : "4");
    keys.append("number of dimensions", "3");
    static const char* const labels[3] = { "x", "y", "z" };
    double sign[3];
    CInterfileImageLayout::stirSigns(m_pData->position->patientOrientation, m_pData->position->patientRotation, sign);
    for(int i = 0; i < 3; i++)
    {
      keys.append(QString("matrix axis label [%1]").arg(i + 1), labels[i]);
      keys.append(QString("!matrix size [%1]").arg(i + 1), QString::number(m_pData->dims[i]));
      keys.append(QString("scaling factor (mm/pixel) [%1]").arg(i + 1), numberText(std::fabs(m_pData->affine[i][i])));
    }
    for(int i = 0; i < 3; i++)
      keys.append(QString("first pixel offset (mm) [%1]").arg(i + 1), numberText(sign[i] * m_pData->affine[i][3] + 0.0));
    keys.append("patient orientation", m_pData->position->patientOrientation);
    keys.append("patient rotation", m_pData->position->patientRotation);
  }

  // the frames: one after the other in the data file
  keys.append("number of time frames", QString::number(m_pData->numFrames));
  const qint64 frameSize = static_cast<qint64>(m_pData->dims[0]) * m_pData->dims[1] * m_pData->dims[2] *
                           CMedIOVolume::bytesPerVoxel(m_pData->dataType);
  for(int f = 0; f < m_pData->written.size(); f++)
  {
    const WrittenFrame& frame = m_pData->written[f];
    if(frame.written == false)
      continue;
    keys.append(QString("image scaling factor[%1]").arg(f + 1), numberText(frame.scale));
    keys.append(QString("data offset in bytes[%1]").arg(f + 1), QString::number(f * frameSize));
    keys.append(QString("image duration (sec)[%1]").arg(f + 1), numberText(frame.duration));
    keys.append(QString("image relative start time (sec)[%1]").arg(f + 1), numberText(frame.start));
  }
  keys.append("!END OF INTERFILE", "");

  const QByteArray text = keys.toText();
  bool result = QFile::resize(0) && QFile::seek(0) && QFile::write(text) == text.size() && QFile::flush();
  if(result == false)
    E("cannot write '%s': %s", qPrintable(fileName()), qPrintable(QFile::errorString()));

  RETURN(result);
  return result;
}

bool CInterfileFile::writeMatrix(const QByteArray& matrixData, const CInterfileFrameHeader& subHeader, short frame)
{
  ENTER();
  bool result = false;

  if(frame == -1)
    frame = 1;

  const int nx = subHeader.dimension(0);
  const int ny = subHeader.dimension(1);
  const int nz = subHeader.dimension(2);
  const CMedIOVolume::DataType type = subHeader.dataType();
  const qint64 size = static_cast<qint64>(nx) * ny * nz * CMedIOVolume::bytesPerVoxel(type);
  double m[3][4];
  subHeader.affine(m);

  // the voxel axes have to be the axes of STIR for one of the patient positions
  double sign[3] = { 0.0, 0.0, 0.0 };
  bool diagonal = true;
  for(int i = 0; i < 3; i++)
  {
    for(int j = 0; j < 3; j++)
    {
      if(i != j && std::fabs(m[i][j]) > 1e-6 * std::fabs(m[j][j]))
        diagonal = false;
    }
    sign[i] = (m[i][i] > 0.0) ? 1.0 : -1.0;
    if(m[i][i] == 0.0 || std::isfinite(m[i][i]) == false || std::isfinite(m[i][3]) == false)
      diagonal = false;
  }
  const StirPosition* position = NULL;
  for(unsigned p = 0; p < sizeof(StirPositions) / sizeof(StirPositions[0]) && diagonal; p++)
  {
    double expected[3];
    CInterfileImageLayout::stirSigns(StirPositions[p].patientOrientation, StirPositions[p].patientRotation, expected);
    if(expected[0] == sign[0] && expected[1] == sign[1] && expected[2] == sign[2])
      position = &StirPositions[p];
  }

  if(isOpen() == false || m_pData->writing == false)
    E("the file is not open for writing");
  else if(m_pData->numFrames == 0)
    E("the main header has to be written first");
  else if(frame < 1 || frame > m_pData->numFrames)
    E("frame %d does not exist (the main header has %d frames)", frame, m_pData->numFrames);
  else if(type != CMedIOVolume::Int16 && type != CMedIOVolume::Float32)
    E("Interfile files are written with signed 16 bit integers or float32 (data type %d)", static_cast<int>(type));
  else if(nx < 1 || ny < 1 || nz < 1 || matrixData.size() != size)
    E("invalid matrix (%d x %d x %d voxels, %d bytes)", nx, ny, nz, matrixData.size());
  else if(subHeader.intercept() != 0.0 || subHeader.slope() == 0.0 || std::isfinite(subHeader.slope()) == false)
    E("invalid scaling (slope %g, intercept %g)", subHeader.slope(), subHeader.intercept());
  else if(position == NULL)
    E("the voxel axes are no axes of STIR for the patient positions HFS, FFS, HFP or FFP");
  else if(m_pData->dataType != CMedIOVolume::UnknownDataType && m_pData->dataType != type)
    E("frame %d has another number format than the frames written before", frame);
  else if(m_pData->dataType != CMedIOVolume::UnknownDataType &&
          (nx != m_pData->dims[0] || ny != m_pData->dims[1] || nz != m_pData->dims[2]))
    E("frame %d has another matrix than the frames written before", frame);
  else if(m_pData->dataType != CMedIOVolume::UnknownDataType && position != m_pData->position)
    E("frame %d has other voxel axes than the frames written before", frame);
  else
  {
    result = true;
    if(m_pData->dataType != CMedIOVolume::UnknownDataType)
    {
      // the same position as the frames written before
      for(int i = 0; i < 3 && result; i++)
      {
        for(int j = 0; j < 4 && result; j++)
          result = std::fabs(m[i][j] - m_pData->affine[i][j]) <= 1e-3 + 1e-6 * std::fabs(m_pData->affine[i][j]);
      }
      if(result == false)
        E("frame %d has another position than the frames written before", frame);
    }
  }

  if(result)
  {
    if(m_pData->dataType == CMedIOVolume::UnknownDataType)
    {
      m_pData->dataType = type;
      m_pData->dims[0] = nx;
      m_pData->dims[1] = ny;
      m_pData->dims[2] = nz;
      for(int i = 0; i < 3; i++)
        for(int j = 0; j < 4; j++)
          m_pData->affine[i][j] = m[i][j];
      m_pData->position = position;
      m_pData->orientation = QString::fromLatin1(position->orientation);
    }

    QByteArray data = matrixData;
    if(QSysInfo::ByteOrder == QSysInfo::BigEndian)
      swapBytes(data, CMedIOVolume::bytesPerVoxel(type));

    result = m_pData->data.seek((frame - 1) * size) && m_pData->data.write(data) == data.size() && m_pData->data.flush();
    if(result == false)
      E("cannot write '%s': %s", qPrintable(m_pData->data.fileName()), qPrintable(m_pData->data.errorString()));
  }

  if(result)
  {
    WrittenFrame& written = m_pData->written[frame - 1];
    written.written = true;
    written.scale = subHeader.slope();
    written.start = subHeader.frameStart();
    written.duration = subHeader.frameDuration();
    result = writeHeader();
  }

  m_iErrStatus = result ? MERR_Ok : MERR_Unspecified;
  RETURN(result);
  return result;
}
