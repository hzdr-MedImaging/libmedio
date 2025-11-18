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

#ifndef CECAT7SUBHEADERATTENCORR_H
#define CECAT7SUBHEADERATTENCORR_H

#include <QDataStream>

#ifndef __MEDIO_PRIVATE__
#include <CECATSubHeader>
#else
#include <CECATSubHeader.h>
#endif

// forward declarations
class CECAT7SubHeaderAttenCorrPrivate;
class CECATFile;
class CECATDirectoryItem;

class CECAT7SubHeaderAttenCorr : public CECATSubHeader
{
  public:
    // public available enumeration types
    enum Attenuation_Type { Atten_None=0,
                            Atten_Meas,
                            Atten_Calc };

    // constructors
    CECAT7SubHeaderAttenCorr(CECATFile* ecatFile = NULL,
                             CECATDirectoryItem* pDirItem = NULL);
    ~CECAT7SubHeaderAttenCorr();

    // copy constructur and default assignment operator
    CECAT7SubHeaderAttenCorr(const CECAT7SubHeaderAttenCorr& src);    
    CECAT7SubHeaderAttenCorr& operator=(const CECAT7SubHeaderAttenCorr& src);
    
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

    // convert methods
    bool convertFrom(const CMedIOHeader* subHeader, const CMedIOHeader* mainHeader=NULL);

    // data access methods
    CECATSubHeader::Data_Type data_Type(void) const;
    short  num_Dimensions(void) const;
    Attenuation_Type attenuation_Type(void) const;
    short num_R_Elements(void) const;
    short num_Angles(void) const;
    short num_Z_Elements(void) const;
    short ring_Difference(void) const;
    float x_Resolution(void) const;
    float y_Resolution(void) const;
    float  z_Resolution(void) const;
    float w_Resolution(void) const;
    float  scale_Factor(void) const;
    float  x_Offset(void) const;
    float  y_Offset(void) const;
    float  x_Radius(void) const;
    float  y_Radius(void) const;
    float  tilt_Angle(void) const;
    float attenuation_Coeff(void) const;
    float  attenuation_Min(void) const;
    float  attenuation_Max(void) const;
    float  skull_Thickness(void) const;
    short  num_Additional_Atten_Coeff(void) const;
    float  additional_Atten_Coeff(int n) const;
    float  edge_Finding_Threshold(void) const;
    short storage_Order(void) const;
    short span(void) const;
    short z_Elements(int n) const;
    short  unused(int n) const;
    short cti_Reserved(int n) const;
    
    // data mutator methods
    void setData_Type(const CECATSubHeader::Data_Type dType);
    void setNum_Dimensions(const short n);
    void setAttenuation_Type(const Attenuation_Type n);
    void setNum_R_Elements(const short n);
    void setNum_Angles(const short n);
    void setNum_Z_Elements(const short n);
    void setRing_Difference(const short n);
    void setX_Resolution(const float n);
    void setY_Resolution(const float n);
    void setZ_Resolution(const float n);
    void setW_Resolution(const float n);
    void setScale_Factor(const float n);
    void setX_Offset(const float n);
    void setY_Offset(const float n);
    void setX_Radius(const float n);
    void setY_Radius(const float n);
    void setTilt_Angle(const float n);
    void setAttenuation_Coeff(const float n);
    void setAttenuation_Min(const float n);
    void setAttenuation_Max(const float n);
    void setSkull_Thickness(const float n);
    void setNum_Additional_Atten_Coeff(const short n);
    void setAdditional_Atten_Coeff(const short i, const float n);
    void setEdge_Finding_Threshold(const float n);
    void setStorage_Order(const short n);
    void setSpan(const short n);
    void setZ_Elements(const short i, const short n);
    void setUnused(const short i, const short n);
    void setCTI_Reserved(const short i, const short n);

  private:
    CECAT7SubHeaderAttenCorrPrivate* m_pData;
};

#endif // CECAT7SUBHEADERATTENCORR_H
