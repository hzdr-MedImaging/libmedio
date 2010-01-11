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

#ifndef CECAT7SUBHEADERPOLARMAP_H
#define CECAT7SUBHEADERPOLARMAP_H

#include <QDataStream>

#ifndef __MEDIO_PRIVATE__
#include <CECATSubHeader>
#else
#include <CECATSubHeader.h>
#endif

// forward declarations
class CECAT7SubHeaderPolarMapPrivate;
class CECATFile;
class CECATDirectoryItem;

class CECAT7SubHeaderPolarMap : public CECATSubHeader
{
  public:
    // public BitMask field
    enum Processing_Code  {  MapType              = (0<<0),
                            ThresholdApplied    =  (1<<0),
                            SummedMap            =  (1<<1),
                            SubstractedMap      =  (1<<2),
                            ProductOfTwoMaps    =  (1<<3),
                            RatioOfTwoMaps      =  (1<<4),
                            Bias                =  (1<<5),
                            Multiplier          =  (1<<6),
                            Transform            =  (1<<7),
                            PMapCalcProtoUsed    =  (1<<8) };

    // constructors
    CECAT7SubHeaderPolarMap(CECATFile* ecatFile = NULL,
                            CECATDirectoryItem* pDirItem = NULL);
    ~CECAT7SubHeaderPolarMap();

    // copy constructur and default assignment operator
    CECAT7SubHeaderPolarMap(const CECAT7SubHeaderPolarMap& src);    
    CECAT7SubHeaderPolarMap& operator=(const CECAT7SubHeaderPolarMap& src);

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
  
    // data access methods
    CECATSubHeader::Data_Type data_Type(void) const;
    short polar_Map_Type(void) const;
    short num_Rings(void) const;
    short sectors_Per_Ring(const short i) const;
    float ring_Position(const short i) const;
    short ring_Angle(const short i) const;
    short start_Angle(void) const;
    short long_Axis_Left(const short i) const;
    short long_Axis_Right(const short i) const;
    short position_Data(void) const;
    short image_Min(void) const;
    short image_Max(void) const;
    float scale_Factor(void) const;
    float pixel_Size(void) const;
    unsigned int frame_Duration(void) const;
    unsigned int frame_Start_Time(void) const;
    unsigned int processing_Code(void) const;
    short quant_Units(void) const;
    const char* annotation(void) const;
    unsigned int gate_Duration(void) const;
    unsigned int r_Wave_Offset(void) const;
    unsigned int num_Accepted_Beats(void) const;
    const char* polar_Map_Protocol(void) const;
    const char* database_Name(void) const;
    short cti_Reserved(const short i) const;
    short user_Reserved(const short i) const;
    
    // data mutator methods
    void setData_Type(const CECATSubHeader::Data_Type dType);
    void setPolar_Map_Type(const short n);
    void setNum_Rings(const short n);
    void setSectors_Per_Ring(const short i, const short n);
    void setRing_Position(const short i, const float n);
    void setRing_Angle(const short i, const short n);
    void setStart_Angle(const short n);
    void setLong_Axis_Left(const short i, const short n);
    void setLong_Axis_Right(const short i, const short n);
    void setPosition_Data(const short n);
    void setImage_Min(const short n);
    void setImage_Max(const short n);
    void setScale_Factor(const float n);
    void setPixel_Size(const float n);
    void setFrame_Duration(const unsigned int n);
    void setFrame_Start_Time(const unsigned int n);
    void setProcessing_Code(const unsigned int n);
    void setQuant_Units(const short n);
    void setAnnotation(const char* str);
    void setGate_Duration(const unsigned int n);
    void setR_Wave_Offset(const unsigned int n);
    void setNum_Accepted_Beats(const unsigned int n);
    void setPolar_Map_Protocol(const char* str);
    void setDatabase_Name(const char* str);
    void setCTI_Reserved(const short i, const short n);
    void setUser_Reserved(const short i, const short n);

  private:
    CECAT7SubHeaderPolarMapPrivate* m_pData;
};

#endif // CECAT7SUBHEADERPOLARMAP_H
