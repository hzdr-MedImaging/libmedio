#ifndef CKEYVALUE_H
#define CKEYVALUE_H

#include <qstring.h>

#include "CKeyType.h"

class CKeyValue
{
	public:
		//constructors
		CKeyValue(CKeyType::Type kType, void* value);
		CKeyValue(CKeyType::Type kType, void* p2Object, void (*p2Function)(void* p2Object, QString KeyValue));
		
		//detructors
		~CKeyValue(){Value = 0;};
		
		//methods
		CKeyType::Type getType(){return KType;};
		void* getPToValue(){return Value;};
		void callBack(QString KeyValue);

	private:
		CKeyType::Type KType;
		void* Value;
		void (*P2Function)(void* p2Object, QString KeyValue);
};

#endif
