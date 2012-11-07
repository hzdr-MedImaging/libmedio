#ifndef CARRAY_H
#define CARRAY_H

#include <QVarLengthArray>
#include <float.h>

class QByteArray;

template<class T> class CDataArray : public QVarLengthArray<T>
{
  public:
    // constructors | destructors
    CDataArray(QByteArray* pArray, int iNumElements);
    float minValue();
    float maxValue();
    float maxDistance();
    float sumValue();

  protected:
    float m_minValue;
    float m_maxValue;
    float m_maxDistance;
    float m_sumValue;
};

#include <rtdebug.h>
#include <QByteArray>
#include <cmath>

template<class T> CDataArray<T>::CDataArray(QByteArray* pArray, int iNumElements)
  : QVarLengthArray<T>(iNumElements)
{
  ENTER();
  T* pData = (T*)pArray->constData();

  m_minValue = FLT_MAX;
  m_maxValue = -FLT_MAX;
  m_sumValue = 0;

  for(int i = 0; i < iNumElements; i++, pData++)
  {
    this->operator[](i) = *pData;

    if(*pData > m_maxValue)
      m_maxValue = *pData;

    if(*pData < m_minValue)
      m_minValue = *pData;

    m_sumValue += *pData;
  }

  // calculate the value range between min<>max
  if(fabs(m_maxValue) > fabs(m_minValue))
    m_maxDistance = fabs(m_maxValue);
  else
    m_maxDistance = fabs(m_minValue);

  D("minValue: %g, maxValue: %g, maxDistance: %g, sumValue: %g", m_minValue, m_maxValue, m_maxDistance, m_sumValue);

  LEAVE();
}

template<class T> float CDataArray<T>::minValue()
{
  return m_minValue;
}

template<class T> float CDataArray<T>::maxValue()
{
  return m_maxValue;
}

template<class T> float CDataArray<T>::maxDistance()
{
  return m_maxDistance;
}

template<class T> float CDataArray<T>::sumValue()
{
  return m_sumValue;
}

#endif // CARRAY_H //
