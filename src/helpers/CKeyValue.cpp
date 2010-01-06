#include "CKeyValue.h"

CKeyValue::CKeyValue(CKeyType::Type kType, void* value)
{
  KType = kType;
  Value = value;
  P2Function = NULL;
}

CKeyValue::CKeyValue(CKeyType::Type kType, void* p2Object, void (*p2Function)(void* p2Object, QString KeyValue))
{
  KType = kType;
  Value = p2Object;
  P2Function = p2Function;
}

void CKeyValue::callBack(QString KeyValue)
{
  if(P2Function!=NULL)
    P2Function(Value,KeyValue);
}
