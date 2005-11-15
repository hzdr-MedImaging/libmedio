#include "CDate.h"

#include <rtdebug.h>

void CDate::parseKeyValue(QString KeyValue)
{
	QDateTime tempDate = QDateTime::fromString(KeyValue);
	this->setTime_t(tempDate.toTime_t());
}

void CDate::wrapper_parseKeyValue(void* p2Object, QString KeyValue)
{
	CDate* myself = (CDate*)p2Object;
	myself->parseKeyValue(KeyValue);
}
