#include "CIntVector.h"

#include <rtdebug.h>

void CIntVector::parseKeyValue(QString KeyValue)
{
	int start_index = KeyValue.indexOf(" ");
	QString Value = KeyValue.right(KeyValue.length()-(start_index + 1));
	//D("String was : %s", Value.toAscii().data());
	append(Value.toInt());
}

void CIntVector::wrapper_parseKeyValue(void* p2Object, QString KeyValue)
{
	CIntVector* myself = (CIntVector*)p2Object;
	myself->parseKeyValue(KeyValue);
}
