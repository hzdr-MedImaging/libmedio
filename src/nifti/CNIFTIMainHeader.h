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

#ifndef CNIFTIMAINHEADER_H
#define CNIFTIMAINHEADER_H

#include <QTextStream>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOHeader>
#else
#include <CMedIOHeader.h>
#endif

// forward declarations
class CNIFTIFile;

class CNIFTIMainHeader : public CMedIOHeader { // CNIFTIMainHeader class inherits from CMedIOHeader class, 
                                               //  which is a base class for medical data headers
  public:
    // possible NIfTI file types
    enum HeaderType { UnknownHeaderType=0,
                      NIFTI1MainHeader,
                      NIFTI2MainHeader
                    };
    
    enum Type { Unknown=0, 
                NIFTI1, 
                NIFTI2
              };
//==================================================================================================
// Constructor: it takes a pointer to a CMedIOData object, which represents the NIfTI file that this header belongs to
    CNIFTIMainHeader(CMedIOData* niftiFile);

// Copy constructor: it takes a reference to another CNIFTIMainHeader object and creates a new object that is a copy of it 
    CNIFTIMainHeader(const CNIFTIMainHeader& src); 

// Default assignment operator: to assign one CNIFTIMainHeader object to another
    CNIFTIMainHeader& operator=(const CNIFTIMainHeader& src);

//==================================================================================================
    // header clear method
    virtual void clear() = 0;  

//==================================================================================================
    // Pure virtual methods to load/save information
    // prepresented by this class
    virtual bool load()  = 0;
    virtual bool save() const = 0;

    // size information in bytes of specific raw mainheader structure
    virtual int rawDataSize() const = 0;  

//==================================================================================================
/*
    // ECAT compatibility methods required by the libmedio interface.
    // NIfTI implementations map these to their generic spatial/temporal dimensions (dim[3], dim[4]).
    virtual short num_Planes(void) const  = 0;
    virtual short num_Frames(void) const  = 0;
    virtual short num_Gates(void) const    = 0;
    virtual short num_Bed_Pos(void) const  = 0;
    
    virtual void setNum_Planes(short num)    = 0;
    virtual void setNum_Frames(short num)    = 0;
    virtual void setNum_Gates(short num)    = 0;
    virtual void setNum_Bed_Pos(short num)  = 0;
*/
//==================================================================================================
    // runtime type information methods
    CMedIOHeader::Format headerFormat() const;

    virtual CNIFTIMainHeader::HeaderType mainHeaderType() const = 0;

    // create a new medio header of the same type as pSrc and
    // copy all meta information
    virtual CMedIOHeader* clone() const = 0;

    // conversion methods
    virtual bool convertFrom(const CMedIOHeader* mainHeader, const CMedIOHeader* subHeader=NULL) = 0;
};

#endif // CNIFTIMAINHEADER_H
