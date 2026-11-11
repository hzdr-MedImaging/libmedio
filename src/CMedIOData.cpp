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

//! @file CMedIOData.cpp
//! @brief contains the implementation of the class CMedIOData

#include "CMedIOData.h"
#include "CMedIOHeader.h"
#include "CMedIOMatrix.h"
#include "CMedIOVolume.h"
#include "MedIOLimits.h"

#include <QString>

#include <cstring>

#include <rtdebug.h>

// matrix data given as pointer and length is wrapped into a QByteArray: false
// (error status set) if the length does not fit into one
static bool matrixFitsByteArray(unsigned int size, int& errorStatus)
{
  if(MedIOLimits::fitsByteArray(size))
    return true;

  E("the matrix data (%u bytes) is too large for a QByteArray", size);
  errorStatus = CMedIOData::MERR_Unspecified;
  return false;
}

//  Class: CMedIOData
//  Constructor: CMedIOData
//!
//! constructs a CMedIOData object
//!
////////////////////////////////////////////////////////////////////////////////
CMedIOData::CMedIOData(const QString& fileName) : QFile(fileName) {
  m_iErrStatus = MERR_Ok;
}

//  Class: CMedIOData
//  Destructor:  CMedIOData
//!
//! destructucts a CMedIOData object.
//! 
////////////////////////////////////////////////////////////////////////////////
CMedIOData::~CMedIOData()
{

}

//! @brief for querying error status information
//! @return integer value for the specific error reason
int CMedIOData::errorStatus(void) const
{ 
  return m_iErrStatus;
}

int CMedIOData::userRtti() const
{
  ENTER();

  RETURN(-1);
  return -1;
}

//=============================================================================================
// format independent access to the image matrices (see CMedIOMatrix.h)

bool CMedIOData::readMainHeader(CMedIOHeader*& mainHeader, CMedIOData::Format format)
{
  ENTER();
  bool result = false;
  mainHeader = NULL;

  if(format == dataFormat()) {
    result = MedIOMatrix::readNativeMainHeader(this, mainHeader);
  } else {
    CMedIOFrame f;
    result = MedIOMatrix::readFrame(this, -1, f) &&
             MedIOMatrix::mainHeaderFromFrame(f, format, this, mainHeader);
  }

  m_iErrStatus = result ? MERR_Ok : MERR_Unspecified;
  RETURN(result);
  return result;
}

bool CMedIOData::readMatrix(QByteArray*& matrixData, CMedIOHeader*& subHeader,
                            CMedIOData::Format format, short frame)
{
  ENTER();
  bool result = false;
  QByteArray data;

  matrixData = NULL;
  subHeader = NULL;

  // Philips: the slices are combined into one frame
  if(format == dataFormat() && format != CMedIOData::Philips) {
    result = MedIOMatrix::readNativeMatrix(this, frame, data, subHeader);
  } else {
    CMedIOFrame f;
    result = MedIOMatrix::readFrame(this, frame, f) &&
             MedIOMatrix::matrixFromFrame(f, format, data, subHeader);
  }

  if(result)
    matrixData = new QByteArray(data);

  m_iErrStatus = result ? MERR_Ok : MERR_Unspecified;
  RETURN(result);
  return result;
}

bool CMedIOData::readMatrix(char*& matrixData, unsigned int& len, CMedIOHeader*& subHeader,
                            CMedIOData::Format format, short frame)
{
  ENTER();
  QByteArray* data = NULL;

  matrixData = NULL;
  len = 0;

  bool result = readMatrix(data, subHeader, format, frame);
  if(result) {
    matrixData = new char[data->size()];
    memcpy(matrixData, data->constData(), data->size());
    len = static_cast<unsigned int>(data->size());
  }
  delete data;

  RETURN(result);
  return result;
}

bool CMedIOData::readMainHeader(CMedIOHeader*& mainHeader)
{
  return readMainHeader(mainHeader, dataFormat());
}

bool CMedIOData::readMatrix(QByteArray*& matrixData, CMedIOHeader*& subHeader, short frame)
{
  return readMatrix(matrixData, subHeader, dataFormat(), frame);
}

bool CMedIOData::readMatrix(char*& matrixData, unsigned int& len, CMedIOHeader*& subHeader, short frame)
{
  return readMatrix(matrixData, len, subHeader, dataFormat(), frame);
}

bool CMedIOData::writeMatrix(const QByteArray& matrixData, const CMedIOHeader& subHeader,
                             short frame, const CMedIOHeader* mainHeader)
{
  ENTER();
  bool result = false;

  const CMedIOData::Format format = MedIOMatrix::headerDataFormat(subHeader);
  if(MedIOMatrix::mainHeaderFitsMatrix(subHeader, mainHeader) == false) {
    // also rejected where it would not be used (NIfTI, matrices written unchanged)
    E("the main header does not belong to the format of the matrix");
  } else if(format == dataFormat() && format == CMedIOData::ECAT) {
    // matrices of the format of this file are written unchanged (except Philips slices)
    result = MedIOMatrix::writeNativeMatrix(this, frame, matrixData, subHeader);
  } else {
    CMedIOFrame f;
    result = MedIOMatrix::frameFromMatrix(matrixData, subHeader, mainHeader, f) &&
             MedIOMatrix::writeFrame(this, frame, f);
  }

  m_iErrStatus = result ? MERR_Ok : MERR_Unspecified;
  RETURN(result);
  return result;
}

bool CMedIOData::writeMatrix(const char* matrixData, unsigned int size, const CMedIOHeader& subHeader,
                             short frame, const CMedIOHeader* mainHeader)
{
  ENTER();
  bool result = (matrixData != NULL) && matrixFitsByteArray(size, m_iErrStatus) &&
                writeMatrix(QByteArray::fromRawData(matrixData, static_cast<int>(size)), subHeader, frame, mainHeader);
  RETURN(result);
  return result;
}

bool CMedIOData::writeMatrix(const QByteArray& matrixData, const CMedIOHeader& subHeader,
                             CMedIOData::Format format, short frame, const CMedIOHeader* mainHeader)
{
  ENTER();
  bool result = false;

  if(format == CMedIOData::Unknown || MedIOMatrix::headerDataFormat(subHeader) != format) {
    E("the sub header does not describe a matrix of data format %d", static_cast<int>(format));
    m_iErrStatus = MERR_Unspecified;
  } else {
    result = writeMatrix(matrixData, subHeader, frame, mainHeader);
  }

  RETURN(result);
  return result;
}

bool CMedIOData::writeMatrix(const char* matrixData, unsigned int size, const CMedIOHeader& subHeader,
                             CMedIOData::Format format, short frame, const CMedIOHeader* mainHeader)
{
  ENTER();
  bool result = (matrixData != NULL) && matrixFitsByteArray(size, m_iErrStatus) &&
                writeMatrix(QByteArray::fromRawData(matrixData, static_cast<int>(size)), subHeader, format, frame, mainHeader);
  RETURN(result);
  return result;
}

QList<short> CMedIOData::frameNumbers()
{
  ENTER();
  const QList<short> numbers = MedIOMatrix::frameNumbers(this);
  LEAVE();
  return numbers;
}

bool CMedIOData::readVolume(CMedIOVolume& volume, short frame)
{
  ENTER();
  bool result = MedIOMatrix::readVolume(this, frame, volume);
  m_iErrStatus = result ? MERR_Ok : MERR_Unspecified;
  RETURN(result);
  return result;
}

bool CMedIOData::writeVolume(const CMedIOVolume& volume, short frame)
{
  ENTER();
  CMedIOFrame f;
  bool result = MedIOMatrix::frameFromVolume(volume, f) &&
                MedIOMatrix::writeFrame(this, frame, f);
  m_iErrStatus = result ? MERR_Ok : MERR_Unspecified;
  RETURN(result);
  return result;
}
