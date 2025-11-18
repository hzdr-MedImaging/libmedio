/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio
 
  Copyright (C) 2004-2025 hzdr.de and contributors
 
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

#ifndef CKEYVALUE_H
#define CKEYVALUE_H

#include <QString>

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
