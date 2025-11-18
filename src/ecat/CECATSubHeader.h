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

#ifndef CECATSUBHEADER_H
#define CECATSUBHEADER_H

#include <QDataStream>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOHeader>
#else
#include <CMedIOHeader.h>
#endif

// forward declarations
class CECATDirectoryItem;

class CECATSubHeader : public CMedIOHeader
{
  public:
    enum Type  { Unknown=0, 
                ECAT7_AttenCorr, 
                ECAT7_Image, 
                ECAT7_Norm,
                ECAT7_Norm3D, 
                ECAT7_PolarMap, 
                ECAT7_Scan,
                ECAT7_Scan3D
              };

    enum Data_Type  {  UnknownDataType  =  0,
                      ByteData        =  1, // 1: char
                      VAX_Ix2          =  2, // 2: short - little endian
                      VAX_Ix4          =  3, // 4: long  - little endian
                      VAX_Rx4          =  4, // 4: float - little endian
                      IEEEFloat        =  5, // 4: float - big endian
                      SunShort        =  6, // 2: short - big endian
                      SunLong          =  7  // 4: long  - big endian
                    };  

    // constructor
    CECATSubHeader(CMedIOData* ecatFile,
                   CECATDirectoryItem* dItem = NULL);
    
    // copy constructur and default assignment operator
    CECATSubHeader(const CECATSubHeader& src);    
    CECATSubHeader& operator=(const CECATSubHeader& src);

    // header clear method
    virtual void clear() = 0;

    // our virtual load/save I/O routines
    virtual bool load(void) = 0;
    virtual bool save(void) const = 0;

    // size information in bytes of specific raw subheader structure
    virtual int rawDataSize() const = 0;

    // some must have accessor methods
    virtual Data_Type data_Type(void) const = 0;
    virtual void setData_Type(const Data_Type dType) = 0;

    // runtime type information methods
    CMedIOHeader::Format headerFormat() const;
    virtual CECATSubHeader::Type subHeaderType(void) const = 0;

    // internal method to set the directory Item of a subHeader
    void setDirectoryItem(CECATDirectoryItem* dItem);

    // create a new medio header of the same type as pSrc and
    // copy all meta information
    virtual CMedIOHeader* clone() const = 0;

    // try to convert the given medio header in this header type
    virtual bool convertFrom(const CMedIOHeader* subHeader, const CMedIOHeader* mainHeader=NULL) = 0;

  protected:
    CECATDirectoryItem* m_pDirItem; // the directory item to which this
                                    // subHeader belongs
};

#endif // CECATSUBHEADER_H
