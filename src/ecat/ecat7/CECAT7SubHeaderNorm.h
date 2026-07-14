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

#ifndef CECAT7SUBHEADERNORM_H
#define CECAT7SUBHEADERNORM_H

#include <QDataStream>

#ifndef __MEDIO_PRIVATE__
#include <CECATSubHeader>
#else
#include <CECATSubHeader.h>
#endif

// forward declarations
class CECAT7SubHeaderNormPrivate;
class CECATFile;
class CECATDirectoryItem;

class CECAT7SubHeaderNorm : public CECATSubHeader
{
  public:
    // public enumeration types
    enum Norm_Qual_Factor_Code  {  TBD };

    // constructors
    CECAT7SubHeaderNorm(CECATFile* ecatFile = NULL,
                        CECATDirectoryItem* pDirItem = NULL);
    ~CECAT7SubHeaderNorm();

    // copy constructur and default assignment operator
    CECAT7SubHeaderNorm(const CECAT7SubHeaderNorm& src);    
    CECAT7SubHeaderNorm& operator=(const CECAT7SubHeaderNorm& src);

    // header clear method
    void clear();

    // public methods                          
    bool load(void);
    bool save(void) const;
    
    // the number of bytes the data of that header requires on disk
    int rawDataSize() const;
    
    CECATSubHeader::Type subHeaderType(void) const;

    // clone methods
    CMedIOHeader* clone() const;

    // conversion methods
    bool convertFrom(const CMedIOHeader* subHeader, const CMedIOHeader* mainHeader=NULL);

    // data access methods
    CECATSubHeader::Data_Type data_Type(void) const;
    short num_Dimensions(void) const;
    short num_R_Elements(void) const;
    short num_Angles(void) const;
    short num_Z_Elements(void) const;
    short ring_Difference(void) const;
    float scale_Factor(void) const;
    float norm_Min(void) const;
    float norm_Max(void) const;
    float fov_Source_Width(void) const;
    float norm_Quality_Factor(void) const;
    Norm_Qual_Factor_Code norm_Quality_Factor_Code(void) const;
    short storage_Order(void) const;
    short span(void) const;
    short z_Elements(const short i) const;
    short cti_Reserved(const short i) const;
    short user_Reserved(const short i) const;
    
    // data mutator methods
    void setData_Type(const CECATSubHeader::Data_Type dType);
    void setNum_Dimensions(const short n);
    void setNum_R_Elements(const short n);
    void setNum_Angles(const short n);
    void setNum_Z_Elements(const short n);
    void setRing_Difference(const short n);
    void setScale_Factor(const float n);
    void setNorm_Min(const float n);
    void setNorm_Max(const float n);
    void setFOV_Source_Width(const float n);
    void setNorm_Quality_Factor(const float n);
    void setNorm_Quality_Factor_Code(const Norm_Qual_Factor_Code n);
    void setStorage_Order(const short n);
    void setSpan(const short n);
    void setZ_Elements(const short i, const short n);
    void setCTI_Reserved(const short i, const short n);
    void setUser_Reserved(const short i, const short n);

  private:
    CECAT7SubHeaderNormPrivate* m_pData;
};

#endif // CECAT7SUBHEADERNORM_H
