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

#include "CDate.h"

#include <rtdebug.h>

void CDate::parseKeyValue(QString KeyValue)
{
  KeyValue = KeyValue.trimmed();
  E("String: %s", KeyValue.toLatin1().data());
  if(!KeyValue.isEmpty() && !KeyValue.isNull())
  {
    QDateTime tempDate = QDateTime::fromString(KeyValue);
    if(tempDate.isValid())
    {
      E("DateTime: %s", tempDate.toString().toLatin1().data());
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
