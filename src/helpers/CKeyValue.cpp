#include "CKeyValue.h"

CKeyValue::CKeyValue(CKeyType::Type kType, void* value)
{
	KType = kType;
	Value = value;
}
