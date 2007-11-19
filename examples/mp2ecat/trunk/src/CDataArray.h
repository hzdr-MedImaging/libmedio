#ifndef CARRAY_H
#define CARRAY_H

#include <QVarLengthArray>

class QByteArray;

template<class T> class CDataArray : public QVarLengthArray<T>
{
	public:
		// constructors | destructors
		CDataArray(QByteArray* pArray, int iNumElements);
		T minValue();
		T maxValue();
		T valueRange();

	protected:
		T m_minValue;
		T m_maxValue;
		T m_valueRange;
};

#include <rtdebug.h>
#include <QByteArray>

#define ABS(a)	a<0?-a:a

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
	m_valueRange = (ABS(m_maxValue) > ABS(m_minValue))?ABS(m_maxValue):ABS(m_minValue);
	D("minValue: %f, maxValue: %f, valueRange: %f", (float)m_minValue, (float)m_maxValue, (float)m_valueRange);
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

template<class T> T CDataArray<T>::valueRange()
{
	return m_valueRange;
}

#endif // CARRAY_H //
