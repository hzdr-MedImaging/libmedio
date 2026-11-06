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

#include "CPhilipsVolume.h"
#include "CPhilipsFile.h"
#include "CPhilipsMainHeader.h"
#include "CPhilipsSubHeaderImage.h"
#include "CECAT7SubHeaderImage.h"

#include <cmath>

#include <rtdebug.h>

CPhilipsVolume::CPhilipsVolume()
  : m_DataType(CECATSubHeader::UnknownDataType),
    m_PhilipsDataType(CPhilipsSubHeader::UnknownDataType),
    m_Nx(0), m_Ny(0), m_Nz(0),
    m_Unscaled(false)
{
}

bool CPhilipsVolume::read(CPhilipsFile& file, const CPhilipsMainHeader* mainHeader, short frame, QString* error)
{
  ENTER();
  QString message;

  m_Data.clear();
  m_DataType = CECATSubHeader::UnknownDataType;
  m_PhilipsDataType = CPhilipsSubHeader::UnknownDataType;
  m_Nx = m_Ny = m_Nz = 0;
  m_SliceScales.clear();
  m_Unscaled = false;
  m_pFirstSubHeader.reset();

  if(frame == -1)
    frame = file.minFrame();

  const int minSlice = file.minSlice();
  const int maxSlice = file.maxSlice();
  m_Nz = file.numSlices(frame);

  // slice numbers of this frame (see CPhilipsFile::readMatrix())
  const int increment = (m_Nz > 1) ? (maxSlice - minSlice) / (m_Nz - 1) : 1;
  if(m_Nz < 1 || increment < 1)
    message = "invalid slice numbering in " + file.fileName();

  for(int k=0; k < m_Nz && message.isEmpty(); k++) {
    const int slice = minSlice + k * increment;
    QByteArray* data = NULL;
    CPhilipsSubHeader* subHeader = NULL;

    if(file.readMatrix(data, subHeader, frame, slice) == false || data == NULL || subHeader == NULL) {
      delete data;
      delete subHeader;
      message = QString("unable to read slice %1 of frame %2 of %3").arg(slice).arg(frame).arg(file.fileName());
      break;
    }
    QScopedPointer<QByteArray> dataGuard(data);
    QScopedPointer<CPhilipsSubHeader> subHeaderGuard(subHeader);

    if(subHeader->subHeaderType() != CPhilipsSubHeader::Image && subHeader->subHeaderType() != CPhilipsSubHeader::Syntegra) {
      message = "only Philips image data can be converted to the ECAT7 representation";
      break;
    }
    const CPhilipsSubHeaderImage* s = static_cast<const CPhilipsSubHeaderImage*>(subHeader);

    if(k == 0) {
      m_Nx = s->xdim();
      m_Ny = s->ydim();
      m_PhilipsDataType = s->datype();
    }

    if(s->xdim() != m_Nx || s->ydim() != m_Ny || m_Nx < 1 || m_Ny < 1) {
      message = "the slices of the Philips file differ in their dimensions";
      break;
    }
    if(s->datype() != m_PhilipsDataType) {
      message = "the slices of the Philips file differ in their data type";
      break;
    }

    // stored values as ECAT7 data type: unsigned 16 bit values are kept as 32 bit integers
    const qint64 voxels = static_cast<qint64>(m_Nx) * m_Ny;
    int bytes = 0;
    switch(m_PhilipsDataType) {
      case CPhilipsSubHeader::ByteData:      bytes = 1; m_DataType = CECATSubHeader::ByteData; break;
      case CPhilipsSubHeader::SignedShort:   bytes = 2; m_DataType = CECATSubHeader::SunShort; break;
      case CPhilipsSubHeader::UnsignedShort: bytes = 2; m_DataType = CECATSubHeader::SunLong; break;
      case CPhilipsSubHeader::Float:         bytes = 4; m_DataType = CECATSubHeader::IEEEFloat; break;
      default:
        message = QString("unsupported Philips data type %1").arg(m_PhilipsDataType);
      break;
    }
    if(message.isEmpty() == false)
      break;

    if(data->size() < voxels * bytes) {
      message = QString("slice %1 of frame %2 is too small").arg(slice).arg(frame);
      break;
    }

    if(m_PhilipsDataType == CPhilipsSubHeader::UnsignedShort) {
      const quint16* src = reinterpret_cast<const quint16*>(data->constData());
      QByteArray converted(voxels * 4, '\0');
      qint32* dst = reinterpret_cast<qint32*>(converted.data());
      for(qint64 i=0; i < voxels; i++)
        dst[i] = src[i];
      m_Data.append(converted);
    } else {
      m_Data.append(data->constData(), voxels * bytes);
    }

    bool ok = false;
    double scale = s->ecat_Scale_Factor(ok, mainHeader);
    if(ok == false || scale <= 0.0 || std::isfinite(scale) == false) {
      m_Unscaled = true;
      scale = 1.0;
    }
    m_SliceScales.append(scale);

    if(k == 0)
      m_pFirstSubHeader.reset(subHeaderGuard.take());
  }

  const bool result = message.isEmpty();
  if(result == false) {
    m_Data.clear();
    m_SliceScales.clear();
    m_pFirstSubHeader.reset();
    if(error != NULL)
      *error = message;
    D("%s", message.toLocal8Bit().constData());
  }

  RETURN(result);
  return result;
}

bool CPhilipsVolume::commonScale() const
{
  for(int k=1; k < m_SliceScales.size(); k++) {
    if(m_SliceScales[k] != m_SliceScales[0])
      return false;
  }

  return true;
}

void CPhilipsVolume::convertToPhysical()
{
  ENTER();

  if(m_Nz > 0 && m_SliceScales.size() == m_Nz) {
    const qint64 sliceVoxels = static_cast<qint64>(m_Nx) * m_Ny;
    QByteArray physical(sliceVoxels * m_Nz * static_cast<int>(sizeof(float)), '\0');
    float* dst = reinterpret_cast<float*>(physical.data());
    const char* src = m_Data.constData();

    for(int k=0; k < m_Nz; k++) {
      const double scale = m_SliceScales[k];
      for(qint64 i=k * sliceVoxels; i < (k + 1) * sliceVoxels; i++) {
        double value = 0.0;
        switch(m_DataType) {
          case CECATSubHeader::ByteData:  value = reinterpret_cast<const quint8*>(src)[i]; break;
          case CECATSubHeader::SunShort:  value = reinterpret_cast<const qint16*>(src)[i]; break;
          case CECATSubHeader::SunLong:   value = reinterpret_cast<const qint32*>(src)[i]; break;
          default:                        value = reinterpret_cast<const float*>(src)[i];  break;
        }
        dst[i] = static_cast<float>(value * scale);
      }
      m_SliceScales[k] = 1.0;
    }

    m_Data = physical;
    m_DataType = CECATSubHeader::IEEEFloat;
  }

  LEAVE();
}

bool CPhilipsVolume::convertSubHeader(CECAT7SubHeaderImage& subHeader, const CPhilipsMainHeader* mainHeader) const
{
  ENTER();
  bool result = false;

  if(m_pFirstSubHeader.isNull() == false && m_SliceScales.isEmpty() == false &&
     subHeader.convertFrom(m_pFirstSubHeader.data(), mainHeader))
  {
    subHeader.setZ_Dimension(m_Nz);
    subHeader.setData_Type(m_DataType);
    subHeader.setScale_Factor(m_SliceScales.first());
    result = true;
  }

  RETURN(result);
  return result;
}
