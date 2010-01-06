/* vim:set ts=2 sw=2 expandtab: ********************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2006-2010 by Jens Langner <Jens.Langner@light-speed.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

#ifndef CECAT7SUBHEADERNORM3D_H
#define CECAT7SUBHEADERNORM3D_H

#include <QDataStream>

#ifndef __MEDIO_PRIVATE__
#include <CECATSubHeader>
#else
#include <CECATSubHeader.h>
#endif

// forward declarations
class CECAT7SubHeaderNorm3DPrivate;
class CECATFile;
class CECATDirectoryItem;

class CECAT7SubHeaderNorm3D : public CECATSubHeader
{
  public:
    // public enumeration types
    enum Norm_Qual_Factor_Code { TBD };

    // constructors
    CECAT7SubHeaderNorm3D(CECATFile* ecatFile = NULL,
                          CECATDirectoryItem* pDirItem = NULL);
    ~CECAT7SubHeaderNorm3D();

    // copy constructur and default assignment operator
    CECAT7SubHeaderNorm3D(const CECAT7SubHeaderNorm3D& src);    
    CECAT7SubHeaderNorm3D& operator=(const CECAT7SubHeaderNorm3D& src);

    // header clear method    
    void clear();

    // public methods
    bool load(void);
    bool save(void) const;

    // the number of bytes the data of that header requires on disk
    int rawDataSize() const;
    
    CECATSubHeader::Type subHeaderType(void) const;

    // clone methods
    bool convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader* pHead2 = NULL);
    CMedIOHeader* clone() const;
    
    // access methods to get directly data out of
    // the SubHeader
    CECATSubHeader::Data_Type data_Type(void) const;
    short num_R_Elements(void) const;
    short num_Transaxial_Crystals(void) const;
    short num_Crystal_Rings(void) const;
    short crystals_Per_Ring(void) const;
    short num_Geo_Corr_Planes(void) const;
    short uld(void) const;
    short lld(void) const;
    short scatter_Energy(void) const;
    float norm_Quality_Factor(void) const;
    Norm_Qual_Factor_Code norm_Quality_Factor_Code(void) const;
    float ring_DTCor1(const short i) const;
    float ring_DTCor2(const short i) const;
    float crystal_DTCor(const short i) const;
    short span(void) const;
    short max_Ring_Diff(void) const;
    short cti_Reserved(const short i) const;
    short user_Reserved(const short i) const;

    // data mutator methods
    void setData_Type(const CECATSubHeader::Data_Type dType);
    void setNum_R_Elements(const short n);
    void setNum_Transaxial_Crystals(const short n);
    void setNum_Crystal_Rings(const short n);
    void setCrystals_Per_Ring(const short n);
    void setNum_Geo_Corr_Planes(const short n);
    void setULD(const short n);
    void setLLD(const short n);
    void setScatter_Energy(const short n);
    void setNorm_Quality_Factor(const float n);
    void setNorm_Quality_Factor_Code(const Norm_Qual_Factor_Code n);
    void setRing_DTCor1(const short i, const float n);
    void setRing_DTCor2(const short i, const float n);
    void setCrystal_DTCor(const short i, const float n);
    void setSpan(const short n);
    void setMax_Ring_Diff(const short n);
    void setCTI_Reserved(const short i, const short n);
    void setUser_Reserved(const short i, const short n);

  private:
    CECAT7SubHeaderNorm3DPrivate* m_pData;
};

#endif // CECAT7SUBHEADERNORM3D_H
