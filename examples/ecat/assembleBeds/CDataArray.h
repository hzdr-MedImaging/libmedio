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

#include <rtdebug.h>

#include <QVarLengthArray>
#include <QByteArray>

#include <math.h>

#define ABS(a)  a<0?-a:a
#define SIGNEDSHORT 32767.0

template<class T> class CDataArray : public QVarLengthArray<T>
{
  public:
    // constructors | destructors
    CDataArray(int iNumElements);
    CDataArray(const QByteArray* pArray, int iNumElements);
    T minValue();
    T maxValue();
    T valueRange();
    bool shortByteArray(QByteArray*& byteArray);

  protected:
    T m_minValue;
    T m_maxValue;
    T m_valueRange;
};

template<class T> CDataArray<T>::CDataArray(int iNumElements)
  : QVarLengthArray<T>(iNumElements)
{
  ENTER();
  m_minValue = 0;
  m_maxValue = 0;
  m_valueRange = 0;
  D("minValue: %f, maxValue: %f, valueRange: %f", (float)m_minValue, (float)m_maxValue, (float)m_valueRange);
  LEAVE();
}

template<class T> CDataArray<T>::CDataArray(const QByteArray* pArray, int iNumElements)
  : QVarLengthArray<T>(iNumElements)
{
  ENTER();
  m_minValue = 0;
  m_maxValue = 0;
  m_valueRange = 0;
  T* pData = (T*)pArray->data();
  m_minValue = m_maxValue = *pData;
  for(int i = 0; i < iNumElements; i++, pData++)
    this->operator[](i) = *pData;
  LEAVE();
}

template<class T> T CDataArray<T>::minValue()
{
  m_minValue = this->operator[](0);
  int iNumElements = QVarLengthArray<T>::size();
  for(int i = 0; i < iNumElements; i++)
  {
    if(m_minValue > this->operator[](i))
      m_minValue = this->operator[](i);
  }
  D("MinValue: %f", (float)m_minValue);
  return m_minValue;
}

template<class T> T CDataArray<T>::maxValue()
{
  int iNumElements = QVarLengthArray<T>::size();
  m_maxValue = this->operator[](0);
  for(int i = 0; i < iNumElements; i++)
  {
    if(m_maxValue < this->operator[](i))
      m_maxValue = this->operator[](i);
  }
  D("MaxValue: %f", (float)m_maxValue);
  return m_maxValue;
}

template<class T> T CDataArray<T>::valueRange()
{
  minValue();
  maxValue();
  m_valueRange = ((ABS(m_maxValue)) > (ABS(m_minValue)))?ABS(m_maxValue):ABS(m_minValue);
  D("ValueRange: %f", (float)m_valueRange);
  return m_valueRange;
}

template<class T> bool CDataArray<T>::shortByteArray(QByteArray*& byteArray)
{
  ENTER();
  bool bResult = true;
  valueRange();
  float fScaleFactor = SIGNEDSHORT/((float)m_valueRange);
  int iNumElements = QVarLengthArray<T>::size();
  int iArrayLength = iNumElements*sizeof(short);
  byteArray = new QByteArray(iArrayLength, 0);
  if(byteArray != NULL)
  {
    short* pArray = (short*)(byteArray->data());
    for(int i = 0; i < iNumElements; i++, pArray++)
    {
      *pArray = (short)floor(((float)this->operator[](i))*fScaleFactor);
    }
  }
  else
    bResult = false;
  RETURN(bResult);
  return bResult;
}

#endif // CARRAY_H //
