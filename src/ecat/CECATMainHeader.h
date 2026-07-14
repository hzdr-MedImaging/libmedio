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

#ifndef CECATMAINHEADER_H
#define CECATMAINHEADER_H

#include <QTextStream>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOHeader>
#else
#include <CMedIOHeader.h>
#endif

// forward declarations
class CECATFile;

class CECATMainHeader : public CMedIOHeader
{
  public:
    // possible ECAT6/7 file types
    enum HeaderType { UnknownHeaderType=0,
                      ECAT6MainHeader,
                      ECAT7MainHeader
                    };
    
    enum Type { Unknown=0, ECAT7_Sinogram, ECAT7_Image16, ECAT7_AttenCorr,
                ECAT7_Normalization, ECAT7_PolarMap, ECAT7_Volume8,
                ECAT7_Volume16, ECAT7_Projection8, ECAT7_Projection16,
                ECAT7_Image8, ECAT7_Sinogram3D_16, ECAT7_Sinogram3D_8,
                ECAT7_Normalization_3D, ECAT7_Sinogram3D_Float
              };

    // constructor
    CECATMainHeader(CMedIOData* ecatFile);

    // copy constructur and default assignment operator
    CECATMainHeader(const CECATMainHeader& src);    
    CECATMainHeader& operator=(const CECATMainHeader& src);

    // header clear method
    virtual void clear() = 0;  

    // some pure virtual methods to load/save information
    // prepresented by this class
    virtual bool load()  = 0;
    virtual bool save() const = 0;

    // size information in bytes of specific raw mainheader structure
    virtual int rawDataSize() const = 0;  

    // interface methods that are part of the ECAT6 and
    // ECAT7 standard and each ECAT6/7 implementation has
    // to implement it.
    virtual short num_Planes(void) const  = 0;
    virtual short num_Frames(void) const  = 0;
    virtual short num_Gates(void) const    = 0;
    virtual short num_Bed_Pos(void) const  = 0;
    
    virtual void setNum_Planes(short num)    = 0;
    virtual void setNum_Frames(short num)    = 0;
    virtual void setNum_Gates(short num)    = 0;
    virtual void setNum_Bed_Pos(short num)  = 0;

    // runtime type information methods
    CMedIOHeader::Format headerFormat() const;
    virtual CECATMainHeader::HeaderType mainHeaderType() const = 0;

    // create a new medio header of the same type as pSrc and
    // copy all meta information
    virtual CMedIOHeader* clone() const = 0;

    // conversion methods
    virtual bool convertFrom(const CMedIOHeader* mainHeader, const CMedIOHeader* subHeader=NULL) = 0;
};

#endif // CECATMAINHEADER_H
