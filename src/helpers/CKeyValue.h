#ifndef CKEYVALUE_H
#define CKEYVALUE_H

#include "CKeyType.h"

class CKeyValue
{
	public:
		//constructors
		CKeyValue(CKeyType::Type kType, void* value);
		
		//detructors
		~CKeyValue(){Value = 0;};
		
		//methods
		CKeyType::Type getType(){return KType;};
		void* getPToValue(){return Value;};

	private:
		CKeyType::Type KType;
		void* Value;
};

#endif
