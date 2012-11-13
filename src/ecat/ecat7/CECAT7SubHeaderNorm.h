/* -*- mode: c++; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 * vim:set ts=2 sw=2 expandtab: *********************************************
 *
 * libmedio - C++ I/O Library for loading/saving medical data formats
 * Copyright (C) 2006-2010 by Jens Langner <Jens.Langner@light-speed.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id$
 *
 **************************************************************************/

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
    void SetScale_Factor(const float n);
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
