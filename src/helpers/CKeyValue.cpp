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
