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

#ifndef CPHILIPSSUBHEADERSYNTEGRA_H
#define CPHILIPSSUBHEADERSYNTEGRA_H

#ifndef __MEDIO_PRIVATE__
#include <CPhilipsSubHeaderImage>
#else
#include <CPhilipsSubHeaderImage.h>
#endif

// forward declarations
class CPhilipsFile;
class CPhilipsDirectoryItem;

class CPhilipsSubHeaderSyntegra : public CPhilipsSubHeaderImage
{
  public:
    // constructors
    CPhilipsSubHeaderSyntegra(CPhilipsFile* philipsFile = NULL,
                              CPhilipsDirectoryItem* pDirItem = NULL);

    // copy constructur and default assignment operator
    CPhilipsSubHeaderSyntegra(const CPhilipsSubHeaderSyntegra& src);    

    // return the subHeader type
    CPhilipsSubHeader::Type subHeaderType(void) const;

    // clone methods
    CMedIOHeader* clone() const;
};

#endif // CPHILIPSSUBHEADERSYNTEGRA_H
