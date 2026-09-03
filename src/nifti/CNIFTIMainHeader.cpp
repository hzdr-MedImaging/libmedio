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

#include "CNIFTIMainHeader.h"

#include <rtdebug.h>

//=============================================================================================
// Constructors 
CNIFTIMainHeader::CNIFTIMainHeader(CMedIOData* niftiFile) : CMedIOHeader(niftiFile) {
  ENTER();

  LEAVE();
}

// Copy constructor
CNIFTIMainHeader::CNIFTIMainHeader(const CNIFTIMainHeader& src) : CMedIOHeader(src) {
  ENTER();

  // do nothing

  LEAVE();
}

// Default assignment operator
CNIFTIMainHeader& CNIFTIMainHeader::operator=(const CNIFTIMainHeader& src) {
  ENTER();

  convertFrom(&src);
  
  LEAVE();
  
  return *this;
}

// runtime type information
CMedIOHeader::Format CNIFTIMainHeader::headerFormat() const { 
  return CMedIOHeader::NIFTIMainHeader;
}
