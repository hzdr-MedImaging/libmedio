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

//! @file CMedIOVolume.cpp
//! @brief contains the implementation of the class CMedIOVolume

#include "CMedIOVolume.h"

#include <QVector>

#include <cmath>
#include <cstring>

#include <rtdebug.h>

#include "MedIOLimits.h"

// largest voxel data of a volume (a QByteArray)
static const qint64 MaxDataSize = MedIOLimits::MaxByteArraySize;

class CMedIOVolumePrivate {
  public:
    CMedIOVolumePrivate() { clear(); }

    void clear()
    {
      dims[0] = dims[1] = dims[2] = 0;
      type = CMedIOVolume::UnknownDataType;
      data.clear();
      scales.clear();
      intercept = 0.0;
      for(int i=0; i < 3; i++)
        for(int j=0; j < 4; j++)
          affine[i][j] = (i == j) ? 1.0 : 0.0;
      frameStart = 0.0;
      frameDuration = 0.0;
    }

    int dims[3];
    CMedIOVolume::DataType type;
    QByteArray data;
    QVector<double> scales;   // one per plane
    double intercept;
    double affine[3][4];
    double frameStart;
    double frameDuration;
};

//=============================================================================================

CMedIOVolume::CMedIOVolume() : m_pData(new CMedIOVolumePrivate())
{
}

CMedIOVolume::CMedIOVolume(const CMedIOVolume& src) : m_pData(new CMedIOVolumePrivate(*src.m_pData))
{
}

CMedIOVolume& CMedIOVolume::operator=(const CMedIOVolume& src)
{
  if(this != &src)
    *m_pData = *src.m_pData;
  return *this;
}

CMedIOVolume::~CMedIOVolume()
{
  delete m_pData;
}

void CMedIOVolume::clear()
{
  m_pData->clear();
}

bool CMedIOVolume::isEmpty() const
{
  return m_pData->data.isEmpty();
}

int CMedIOVolume::bytesPerVoxel(CMedIOVolume::DataType type)
{
  switch(type) {
    case UInt8:
    case Int8:    return 1;
    case Int16:
    case UInt16:  return 2;
    case Int32:
    case UInt32:
    case Float32: return 4;
    case Float64: return 8;
    case UnknownDataType: break;
  }
  return 0;
}

int CMedIOVolume::dimension(int axis) const
{
  return (axis >= 0 && axis < 3) ? m_pData->dims[axis] : 0;
}

qint64 CMedIOVolume::voxelCount() const
{
  return static_cast<qint64>(m_pData->dims[0]) * m_pData->dims[1] * m_pData->dims[2];
}

bool CMedIOVolume::setData(int nx, int ny, int nz, CMedIOVolume::DataType type, const QByteArray& data)
{
  ENTER();

  const qint64 size = static_cast<qint64>(nx) * ny * nz * bytesPerVoxel(type);
  if(nx < 1 || ny < 1 || nz < 1 || bytesPerVoxel(type) == 0 || data.size() != size) {
    E("invalid volume: %d x %d x %d voxels of type %d, %d bytes", nx, ny, nz, static_cast<int>(type), data.size());
    RETURN(false);
    return false;
  }

  m_pData->dims[0] = nx;
  m_pData->dims[1] = ny;
  m_pData->dims[2] = nz;
  m_pData->type = type;
  m_pData->data = data;
  m_pData->scales = QVector<double>(nz, 1.0);
  m_pData->intercept = 0.0;

  RETURN(true);
  return true;
}

CMedIOVolume::DataType CMedIOVolume::dataType() const
{
  return m_pData->type;
}

const QByteArray& CMedIOVolume::data() const
{
  return m_pData->data;
}

double CMedIOVolume::storedValue(qint64 index) const
{
  const char* d = m_pData->data.constData();

  switch(m_pData->type) {
    case UInt8:   return reinterpret_cast<const quint8*>(d)[index];
    case Int8:    return reinterpret_cast<const qint8*>(d)[index];
    case Int16:   return reinterpret_cast<const qint16*>(d)[index];
    case UInt16:  return reinterpret_cast<const quint16*>(d)[index];
    case Int32:   return reinterpret_cast<const qint32*>(d)[index];
    case UInt32:  return reinterpret_cast<const quint32*>(d)[index];
    case Float32: return reinterpret_cast<const float*>(d)[index];
    case Float64: return reinterpret_cast<const double*>(d)[index];
    case UnknownDataType: break;
  }
  return 0.0;
}

double CMedIOVolume::value(qint64 index) const
{
  const qint64 planeVoxels = static_cast<qint64>(m_pData->dims[0]) * m_pData->dims[1];
  const int plane = static_cast<int>(index / planeVoxels);
  return storedValue(index) * m_pData->scales[plane] + m_pData->intercept;
}

void CMedIOVolume::setScaleFactor(double factor)
{
  m_pData->scales.fill(factor);
}

void CMedIOVolume::setScaleFactor(int plane, double factor)
{
  if(plane >= 0 && plane < m_pData->scales.size())
    m_pData->scales[plane] = factor;
}

double CMedIOVolume::scaleFactor(int plane) const
{
  return (plane >= 0 && plane < m_pData->scales.size()) ? m_pData->scales[plane] : 1.0;
}

bool CMedIOVolume::hasUniformScaleFactor() const
{
  for(int k=1; k < m_pData->scales.size(); k++) {
    if(m_pData->scales[k] != m_pData->scales[0])
      return false;
  }
  return true;
}

void CMedIOVolume::setIntercept(double intercept)
{
  m_pData->intercept = intercept;
}

double CMedIOVolume::intercept() const
{
  return m_pData->intercept;
}

bool CMedIOVolume::convertToPhysicalValues()
{
  ENTER();

  const qint64 voxels = voxelCount();
  const bool doubles = (m_pData->type == Float64);
  const qint64 size = voxels * (doubles ? 8 : 4);
  if(size > MaxDataSize) {
    E("the physical values of %lld voxels (%lld bytes) do not fit into a volume", voxels, size);
    RETURN(false);
    return false;
  }

  QByteArray physical(static_cast<int>(size), '\0');

  for(qint64 i=0; i < voxels; i++) {
    if(doubles)
      reinterpret_cast<double*>(physical.data())[i] = value(i);
    else
      reinterpret_cast<float*>(physical.data())[i] = static_cast<float>(value(i));
  }

  m_pData->data = physical;
  m_pData->type = doubles ? Float64 : Float32;
  m_pData->scales.fill(1.0);
  m_pData->intercept = 0.0;

  RETURN(true);
  return true;
}

void CMedIOVolume::setAffine(const double m[3][4])
{
  memcpy(m_pData->affine, m, sizeof(m_pData->affine));
}

void CMedIOVolume::affine(double m[3][4]) const
{
  memcpy(m, m_pData->affine, sizeof(m_pData->affine));
}

double CMedIOVolume::affine(int row, int column) const
{
  return (row >= 0 && row < 3 && column >= 0 && column < 4) ? m_pData->affine[row][column] : 0.0;
}

void CMedIOVolume::position(double i, double j, double k, double p[3]) const
{
  const double (&m)[3][4] = m_pData->affine;
  for(int r=0; r < 3; r++)
    p[r] = m[r][0] * i + m[r][1] * j + m[r][2] * k + m[r][3];
}

double CMedIOVolume::voxelSize(int axis) const
{
  if(axis < 0 || axis > 2)
    return 0.0;

  const double (&m)[3][4] = m_pData->affine;
  return std::sqrt(m[0][axis] * m[0][axis] + m[1][axis] * m[1][axis] + m[2][axis] * m[2][axis]);
}

//---------------------------------------------------------------------------------------------
// orientation

namespace {

// patient axis (0: x, 1: y, 2: z) and direction (+1/-1, LPS) of an orientation letter
bool orientationLetter(QChar c, int& axis, int& sign)
{
  switch(c.toUpper().toLatin1()) {
    case 'L': axis = 0; sign =  1; return true;
    case 'R': axis = 0; sign = -1; return true;
    case 'P': axis = 1; sign =  1; return true;
    case 'A': axis = 1; sign = -1; return true;
    case 'S': axis = 2; sign =  1; return true;
    case 'I': axis = 2; sign = -1; return true;
  }
  return false;
}

// patient axis the voxel axis a is (mostly) aligned with, -1 for a null axis
int closestAxis(const double m[3][4], int a)
{
  int w = 0;
  for(int i=1; i < 3; i++) {
    if(std::fabs(m[i][a]) > std::fabs(m[w][a]))
      w = i;
  }
  return (m[w][a] != 0.0) ? w : -1;
}

} // namespace

QString CMedIOVolume::orientation() const
{
  static const char letters[3][2] = { { 'R', 'L' }, { 'A', 'P' }, { 'I', 'S' } };
  QString code;

  for(int a=0; a < 3; a++) {
    const int w = closestAxis(m_pData->affine, a);
    if(w < 0)
      return QString();
    code += QChar(letters[w][m_pData->affine[w][a] > 0 ? 1 : 0]);
  }

  return code;
}

bool CMedIOVolume::reorient(const QString& code, bool* oblique)
{
  ENTER();
  CMedIOVolumePrivate* d = m_pData;

  if(oblique)
    *oblique = false;

  // patient axis and direction of the target axes
  int targetAxis[3], targetSign[3];
  bool usedTarget[3] = { false, false, false };
  bool valid = (code.size() == 3);
  for(int t=0; t < 3 && valid; t++) {
    valid = orientationLetter(code[t], targetAxis[t], targetSign[t]) && usedTarget[targetAxis[t]] == false;
    if(valid)
      usedTarget[targetAxis[t]] = true;
  }
  if(valid == false) {
    E("invalid orientation code '%s'", code.toLatin1().constData());
    RETURN(false);
    return false;
  }

  // voxel axis providing every patient axis
  int sourceOf[3] = { -1, -1, -1 };
  for(int a=0; a < 3; a++) {
    const int w = closestAxis(d->affine, a);
    if(w < 0 || sourceOf[w] >= 0) {
      E("the voxel axes cannot be assigned to the patient axes");
      RETURN(false);
      return false;
    }
    sourceOf[w] = a;

    if(oblique && std::fabs(d->affine[w][a]) < voxelSize(a) * (1.0 - 1e-6))
      *oblique = true;
  }

  int perm[3];   // voxel axis providing target axis t
  bool flip[3];  // target axis t runs opposite to the voxel axis
  bool identity = true;
  for(int t=0; t < 3; t++) {
    perm[t] = sourceOf[targetAxis[t]];
    flip[t] = ((d->affine[targetAxis[t]][perm[t]] > 0) ? 1 : -1) != targetSign[t];
    if(perm[t] != t || flip[t])
      identity = false;
  }

  if(identity) {
    RETURN(true);
    return true;
  }

  // scale factors per plane only survive if the planes stay planes
  if(perm[2] != 2 && hasUniformScaleFactor() == false && convertToPhysicalValues() == false) {
    RETURN(false);
    return false;
  }

  const int n = bytesPerVoxel(d->type);
  const qint64 srcDims[3] = { d->dims[0], d->dims[1], d->dims[2] };
  const int dims[3] = { d->dims[perm[0]], d->dims[perm[1]], d->dims[perm[2]] };
  QByteArray data(d->data.size(), '\0');
  char* dst = data.data();

  // (only the geometry of a volume without data)
  for(int k=0; k < dims[2] && n > 0 && isEmpty() == false; k++)
    for(int j=0; j < dims[1]; j++)
      for(int i=0; i < dims[0]; i++, dst += n) {
        const int e[3] = { i, j, k };
        qint64 v[3];
        for(int t=0; t < 3; t++)
          v[perm[t]] = flip[t] ? dims[t] - 1 - e[t] : e[t];
        memcpy(dst, d->data.constData() + (v[0] + srcDims[0] * (v[1] + srcDims[1] * v[2])) * n, n);
      }

  // geometry: columns of the target axes, position of target voxel (0, 0, 0)
  double m[3][4];
  double v0[3];
  for(int t=0; t < 3; t++) {
    for(int r=0; r < 3; r++)
      m[r][t] = d->affine[r][perm[t]] * (flip[t] ? -1.0 : 1.0);
    v0[perm[t]] = flip[t] ? dims[t] - 1 : 0;
  }
  for(int r=0; r < 3; r++)
    m[r][3] = d->affine[r][0] * v0[0] + d->affine[r][1] * v0[1] + d->affine[r][2] * v0[2] + d->affine[r][3];

  // scale factors of the planes (the planes are kept or reversed if perm[2] == 2)
  QVector<double> scales(dims[2], 1.0);
  for(int k=0; k < dims[2] && d->scales.size() == srcDims[2]; k++)
    scales[k] = (perm[2] == 2) ? d->scales[flip[2] ? dims[2] - 1 - k : k] : d->scales[0];

  d->data = data;
  memcpy(d->dims, dims, sizeof(dims));
  memcpy(d->affine, m, sizeof(m));
  d->scales = scales;

  RETURN(true);
  return true;
}

void CMedIOVolume::setFrameStart(double seconds)
{
  m_pData->frameStart = seconds;
}

double CMedIOVolume::frameStart() const
{
  return m_pData->frameStart;
}

void CMedIOVolume::setFrameDuration(double seconds)
{
  m_pData->frameDuration = seconds;
}

double CMedIOVolume::frameDuration() const
{
  return m_pData->frameDuration;
}
