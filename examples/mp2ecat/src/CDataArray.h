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

#ifndef CARRAY_H
#define CARRAY_H

#include <math.h>

#include <QVarLengthArray>

class QByteArray;

template<class T> class CDataArray : public QVarLengthArray<T>
{
  public:
    // constructors | destructors
    CDataArray(QByteArray* pArray, int iNumElements);
    T minValue();
    T maxValue();
    T maxDistance();

  protected:
    T m_minValue;
    T m_maxValue;
    T m_maxDistance;
};

#include <rtdebug.h>
#include <QByteArray>

template<class T> CDataArray<T>::CDataArray(QByteArray* pArray, int iNumElements)
  : QVarLengthArray<T>(iNumElements)
{
  ENTER();
  T* pData = (T*)pArray->data();
  m_minValue = m_maxValue = *pData;
  for(int i = 0; i < iNumElements; i++, pData++)
  {
    this->operator[](i) = *pData;
    if(m_maxValue < *pData)
      m_maxValue = *pData;
    if(m_minValue > *pData)
      m_minValue = *pData;
  }

  // calculate the value range between min<>max
  if(fabs(m_maxValue) > fabs(m_minValue))
    m_maxDistance = fabs(m_maxValue);
  else
    m_maxDistance = fabs(m_minValue);

  D("minValue: %g, maxValue: %g, maxDistance: %g", (float)m_minValue, (float)m_maxValue, (float)m_maxDistance);

  LEAVE();
}

template<class T> T CDataArray<T>::minValue()
{
  return m_minValue;
}

template<class T> T CDataArray<T>::maxValue()
{
  return m_maxValue;
}

template<class T> T CDataArray<T>::maxDistance()
{
  return m_maxDistance;
}

#endif // CARRAY_H //
