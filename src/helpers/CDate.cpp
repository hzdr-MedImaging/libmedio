#include "CDate.h"

#include <rtdebug.h>

void CDate::parseKeyValue(QString KeyValue)
{
	KeyValue = KeyValue.stripWhiteSpace();
	E("String: %s", KeyValue.ascii());
	if(!KeyValue.isEmpty() && !KeyValue.isNull())
	{
		QDateTime tempDate = QDateTime::fromString(KeyValue);
		if(tempDate.isValid())
		{
			E("DateTime: %s", tempDate.toString().ascii());
			this->setTime_t(tempDate.toTime_t());
		}
		else
			this->setTime_t(0);
	}
	else
		this->setTime_t(0);
}

void CDate::wrapper_parseKeyValue(void* p2Object, QString KeyValue)
{
	CDate* myself = (CDate*)p2Object;
	myself->parseKeyValue(KeyValue);
}
