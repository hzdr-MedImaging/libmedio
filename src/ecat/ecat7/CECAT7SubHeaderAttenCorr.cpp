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

#include "CECAT7SubHeaderAttenCorr.h"
#include "CECATDirectoryItem.h"
#include "CECATFile.h"

#include <QDataStream>

#include <rtdebug.h>

#include "bswap.h"

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CECAT7SubHeaderAttenCorrPrivate
{
  public:
    // SubHeder structure (should be 512 bytes)
    #define SUBHEADER_SIZE 512
    #pragma pack(1)
    struct HeaderData
    {
      quint16  Data_Type;                  //   0: Data_Type
      quint16  Num_Dimensions;             //   2: Num_Dimensions
      quint16  Attenuation_Type;           //   4: Attenuation_Type
      quint16  Num_R_Elements;             //   6: Num_R_Elements
      quint16  Num_Angles;                 //   8: Num_Angles
      quint16  Num_Z_Elements;             //  10: Num_Z_Elements
      quint16  Ring_Difference;            //  12: Ring_Difference
      float    X_Resolution;               //  14: X_Resolution
      float    Y_Resolution;               //  18: Y_Resolution
      float    Z_Resolution;               //  22: Z_Resolution
      float    W_Resolution;               //  26: W_Resolution
      float    Scale_Factor;               //  30: Scale_Factor
      float    X_Offset;                   //  34: X_Offset
      float    Y_Offset;                   //  38: Y_Offset
      float    X_Radius;                   //  42: X_Radius
      float    Y_Radius;                   //  46: Y_Radius
      float    Tilt_Angle;                 //  50: Tilt_Angle
      float    Attenuation_Coeff;          //  54: Attenuation_Coeff
      float    Attenuation_Min;            //  58: Attenuation_Min
      float    Attenuation_Max;            //  62: Attenuation_Max
      float    Skull_Thickness;            //  66: Skull_Thickness
      quint16  Num_Additional_Atten_Coeff; //  70: Num_Additional_Atten_Coeff
      float    Additional_Atten_Coeff[8];  //  72: Additional_Atten_Coeff (8)
      float    Edge_Finding_Threshold;     // 104: Edge_Finding_Threshold
      quint16  Storage_Order;              // 108: Storage_Order
      quint16  Span;                       // 110: Span
      quint16  Z_Elements[64];             // 112: Z_Elements (64)                                      
      quint16  Unused[86];                 // 240: Unused (86)
      quint16  CTI_Reserved[50];           // 412: CTI_Reserved (50)
    } header; 
    #pragma pack()
};

CECAT7SubHeaderAttenCorr::CECAT7SubHeaderAttenCorr(CECATFile* ecatFile,
                                                   CECATDirectoryItem* pDirItem)
  : CECATSubHeader(ecatFile, pDirItem)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CECAT7SubHeaderAttenCorrPrivate();
  
  clear();

  LEAVE();
}

CECAT7SubHeaderAttenCorr::~CECAT7SubHeaderAttenCorr()
{
  ENTER();

  delete m_pData;

  LEAVE();
}

CECAT7SubHeaderAttenCorr::CECAT7SubHeaderAttenCorr(const CECAT7SubHeaderAttenCorr& src)
  : CECATSubHeader(src)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CECAT7SubHeaderAttenCorrPrivate(*(src.m_pData));

  LEAVE();
}

CECAT7SubHeaderAttenCorr& CECAT7SubHeaderAttenCorr::operator=(const CECAT7SubHeaderAttenCorr& src)
{
  ENTER();

  if(m_pData != src.m_pData)
  {
    memcpy(&m_pData->header, 
           &src.m_pData->header, 
           sizeof(struct CECAT7SubHeaderAttenCorrPrivate::HeaderData));
  }

  LEAVE();
  return *this;
}

void CECAT7SubHeaderAttenCorr::clear()
{
  ENTER();
  
  // clear our header structure first
  memset(&m_pData->header, 0, sizeof(struct CECAT7SubHeaderAttenCorrPrivate::HeaderData));
    
  setData_Type(CECATSubHeader::UnknownDataType);
  setAttenuation_Type(Atten_None);

  LEAVE();
}

bool CECAT7SubHeaderAttenCorr::load(void)
{
  ENTER();
  CMedIOData* mData = medIOData();

  // check if the stream is readable or not and
  // set our mData to the correct file position so that we can
  // read the subheader  
  if(mData == NULL ||
     mData->isReadable() == false ||
     m_pDirItem->dataBlock_Start() == 0 ||
     mData->seek(m_pDirItem->dataBlock_Start()) == false)
  {
    RETURN(false);
    return false;
  }

  // we read in all data at once using read()
  ASSERT(sizeof(m_pData->header) == SUBHEADER_SIZE);
  if(mData->read(reinterpret_cast<char*>(&m_pData->header), sizeof(m_pData->header)) != SUBHEADER_SIZE)
  {
    RETURN(false);
    return false;
  }

  // now that we have streamed in all data in one run we
  // have to take care of correct endianness in the non-char
  // entries in the header structure in case this is a little endian
  // machine
  if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
  {
    // we only swap non-char elements of the header
    BSWAP_16(m_pData->header.Data_Type);
    BSWAP_16(m_pData->header.Num_Dimensions);
    BSWAP_16(m_pData->header.Attenuation_Type);
    BSWAP_16(m_pData->header.Num_R_Elements);
    BSWAP_16(m_pData->header.Num_Angles);
    BSWAP_16(m_pData->header.Num_Z_Elements);
    BSWAP_16(m_pData->header.Ring_Difference);
    BSWAP_FLT(m_pData->header.X_Resolution);
    BSWAP_FLT(m_pData->header.Y_Resolution);
    BSWAP_FLT(m_pData->header.Z_Resolution);
    BSWAP_FLT(m_pData->header.W_Resolution);
    BSWAP_FLT(m_pData->header.Scale_Factor);
    BSWAP_FLT(m_pData->header.X_Offset);
    BSWAP_FLT(m_pData->header.Y_Offset);
    BSWAP_FLT(m_pData->header.X_Radius);
    BSWAP_FLT(m_pData->header.Y_Radius);
    BSWAP_FLT(m_pData->header.Tilt_Angle);
    BSWAP_FLT(m_pData->header.Attenuation_Coeff);
    BSWAP_FLT(m_pData->header.Attenuation_Min);
    BSWAP_FLT(m_pData->header.Attenuation_Max);
    BSWAP_FLT(m_pData->header.Skull_Thickness);
    BSWAP_16(m_pData->header.Num_Additional_Atten_Coeff);

    for(int i=0; i < 8; i++)
      BSWAP_FLT(m_pData->header.Additional_Atten_Coeff[i]);

    BSWAP_FLT(m_pData->header.Edge_Finding_Threshold);
    BSWAP_16(m_pData->header.Storage_Order);
    BSWAP_16(m_pData->header.Span);

    for(int i=0; i < 64; i++)
      BSWAP_16(m_pData->header.Z_Elements[i]);

    for(int i=0; i < 86; i++)
      BSWAP_16(m_pData->header.Unused[i]);

    for(int i=0; i < 50; i++)
      BSWAP_16(m_pData->header.CTI_Reserved[i]);
  }

  // some more debug output
#if defined(DEBUG)
  D("ECAT7 Attenuation Correction SubHeader loaded:");
  D("---------------------------------------------");
  D("Data_Type                 : %d",       m_pData->header.Data_Type);
  D("Num_Dimensions            : %d",       m_pData->header.Num_Dimensions);
  D("Attenuation_Type          : %d",       m_pData->header.Attenuation_Type);
  D("Num_R_Elements            : %d",       m_pData->header.Num_R_Elements);
  D("Num_Angles                : %d",       m_pData->header.Num_Angles);
  D("Num_Z_Elements            : %d",       m_pData->header.Num_Z_Elements);
  D("Ring_Difference           : %d",       m_pData->header.Ring_Difference);
  D("X_Resolution              : %f cm",    m_pData->header.X_Resolution);
  D("Y_Resolution              : %f cm",    m_pData->header.Y_Resolution);
  D("Z_Resolution              : %f cm",    m_pData->header.Z_Resolution);
  D("W_Resolution              : %f",       m_pData->header.W_Resolution);
  D("Scale_Factor              : %f",       m_pData->header.Scale_Factor);
  D("X_Offset                  : %f cm",    m_pData->header.X_Offset);
  D("Y_Offset                  : %f cm",    m_pData->header.Y_Offset);
  D("X_Radius                  : %f cm",    m_pData->header.X_Radius);
  D("Y_Radius                  : %f cm",    m_pData->header.Y_Radius);
  D("Tilt_Angle                : %f°",      m_pData->header.Tilt_Angle);
  D("Attenuation_Coeff         : %f cm^-1", m_pData->header.Attenuation_Coeff);
  D("Attenuation_Min           : %f",       m_pData->header.Attenuation_Min);
  D("Attenuation_Max           : %f",       m_pData->header.Attenuation_Max);
  D("Skull_Thickness           : %f cm",    m_pData->header.Skull_Thickness);
  D("Num_Additional_Atten_Coeff: %d",       m_pData->header.Num_Additional_Atten_Coeff);
  for(int i=0; i < m_pData->header.Num_Additional_Atten_Coeff; i++)
  {
    D("Additional_Atten_Coeff[%d]: %f", i+1, m_pData->header.Additional_Atten_Coeff[i]);
  }
  D("Edge_Finding_Threshold    : %f",       m_pData->header.Edge_Finding_Threshold);
  D("Storage_Order             : %d",       m_pData->header.Storage_Order);
  D("Span                      : %d",       m_pData->header.Span);
  for(int i=0; i < 64; i++)
  {
    D("Z_Elements              [%d]: %f", i+1, m_pData->header.Z_Elements[i]);
  }
#endif

  RETURN(true);
  return true;
}

bool CECAT7SubHeaderAttenCorr::save(void) const
{
  ENTER();
  CMedIOData* mData = medIOData();

  // check if this stream is writeable or not
  if(mData == NULL || mData->isWritable() == false ||
     m_pDirItem == NULL || m_pDirItem->dataBlock_Start() == 0 ||
     mData->seek(m_pDirItem->dataBlock_Start()) == false)
  {
    RETURN(false);
    return false;
  }

  SHOWVALUE(mData->pos());

  ASSERT(sizeof(m_pData->header) == SUBHEADER_SIZE);
  struct CECAT7SubHeaderAttenCorrPrivate::HeaderData* header = NULL;
  if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
  {
    header = new CECAT7SubHeaderAttenCorrPrivate::HeaderData;

    // copy the current m_pData->header to beHeader
    memcpy(header, &m_pData->header, sizeof(m_pData->header));

    // we only swap non-char elements of the header
    BSWAP_16(header->Data_Type);
    BSWAP_16(header->Num_Dimensions);
    BSWAP_16(header->Attenuation_Type);
    BSWAP_16(header->Num_R_Elements);
    BSWAP_16(header->Num_Angles);
    BSWAP_16(header->Num_Z_Elements);
    BSWAP_16(header->Ring_Difference);
    BSWAP_FLT(header->X_Resolution);
    BSWAP_FLT(header->Y_Resolution);
    BSWAP_FLT(header->Z_Resolution);
    BSWAP_FLT(header->W_Resolution);
    BSWAP_FLT(header->Scale_Factor);
    BSWAP_FLT(header->X_Offset);
    BSWAP_FLT(header->Y_Offset);
    BSWAP_FLT(header->X_Radius);
    BSWAP_FLT(header->Y_Radius);
    BSWAP_FLT(header->Tilt_Angle);
    BSWAP_FLT(header->Attenuation_Coeff);
    BSWAP_FLT(header->Attenuation_Min);
    BSWAP_FLT(header->Attenuation_Max);
    BSWAP_FLT(header->Skull_Thickness);
    BSWAP_16(header->Num_Additional_Atten_Coeff);

    for(int i=0; i < 8; i++)
      BSWAP_FLT(header->Additional_Atten_Coeff[i]);

    BSWAP_FLT(header->Edge_Finding_Threshold);
    BSWAP_16(header->Storage_Order);
    BSWAP_16(header->Span);

    for(int i=0; i < 64; i++)
      BSWAP_16(header->Z_Elements[i]);

    for(int i=0; i < 86; i++)
      BSWAP_16(header->Unused[i]);

    for(int i=0; i < 50; i++)
      BSWAP_16(header->CTI_Reserved[i]);
  }
  else
    header = &m_pData->header;

  // now write out to our outStream
  bool result = false;
  if(mData->write(reinterpret_cast<char*>(header), sizeof(m_pData->header)) == SUBHEADER_SIZE)
  {
    m_pDirItem->subHeaderWritten(*this);
    result = true;
  }

  // if we byte swapped we have to delete the
  // temporary byte swapped header structures
  if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
    delete header;

  RETURN(result);
  return result;
}

int CECAT7SubHeaderAttenCorr::rawDataSize() const 
{ 
  return 1*ECAT_BLOCKSIZE;
}

CECATSubHeader::Type CECAT7SubHeaderAttenCorr::subHeaderType(void) const
{
  return CECATSubHeader::ECAT7_AttenCorr;
}

bool CECAT7SubHeaderAttenCorr::convertFrom(const CMedIOHeader* subHeader, const CMedIOHeader*) 
{
  ENTER();
  bool bResult = false;

  // depending on the MedIOHeader format we do have to 
  // distinguish between our copy operations.
  switch(subHeader->headerFormat())
  {
    case CMedIOHeader::ECATSubHeader:
    {
      const CECATSubHeader* eSubHeader = static_cast<const CECATSubHeader*>(subHeader);

      // depending on the source type we have to copy either every data or just 
      // some data of the src header
      switch(eSubHeader->subHeaderType())
      {
        // if the source header is also an ECAT7 one we can copy it in whole
        // via a simple memcpy()
        case CECATSubHeader::ECAT7_AttenCorr:
        {
          // we use the assignment operator which will do the convertation
          // for us.
          *this = *static_cast<const CECAT7SubHeaderAttenCorr*>(subHeader);
          
          bResult = true;
        }
        break;

        case CECATSubHeader::ECAT7_Image:
        case CECATSubHeader::ECAT7_Norm:
        case CECATSubHeader::ECAT7_Norm3D:
        case CECATSubHeader::ECAT7_PolarMap:
        case CECATSubHeader::ECAT7_Scan:
        case CECATSubHeader::ECAT7_Scan3D:
        {
          Error("medio subheader %d conversion not implemented!", eSubHeader->subHeaderType());
        }
        break;

        case CECATSubHeader::Unknown:
          // for an unknown header type we do nothing
        break;
      }
    }

    case CMedIOHeader::ECATMainHeader:
    case CMedIOHeader::ConcordeMicroPetMainHeader:
    case CMedIOHeader::PhilipsMainHeader:
    {
      // copying a main header into a sub header doesn't make much sense, so we
      // do nothing here
    }
    break;

    case CMedIOHeader::ConcordeMicroPetFrameHeader:
    case CMedIOHeader::PhilipsSubHeader:
    case CMedIOHeader::PhilipsListviewHeader:
    {
      Error("medio subheader %d conversion not implemented!", subHeader->headerFormat());
    }
    break;

    case CMedIOHeader::Unknown:
    {
      // for an unknown header type we do nothing
    }
    break;
  }

  RETURN(bResult);
  return bResult;
}

CMedIOHeader* CECAT7SubHeaderAttenCorr::clone() const
{
  ENTER();

  CECAT7SubHeaderAttenCorr* pNewHead = new CECAT7SubHeaderAttenCorr();
  pNewHead->convertFrom(this);

  RETURN(pNewHead);
  return pNewHead;
}

// data access methods
CECATSubHeader::Data_Type CECAT7SubHeaderAttenCorr::data_Type(void) const
{
  return static_cast<CECATSubHeader::Data_Type>(m_pData->header.Data_Type);
}

short  CECAT7SubHeaderAttenCorr::num_Dimensions(void) const
{
  return m_pData->header.Num_Dimensions;
}

CECAT7SubHeaderAttenCorr::Attenuation_Type CECAT7SubHeaderAttenCorr::attenuation_Type(void) const
{
  return static_cast<CECAT7SubHeaderAttenCorr::Attenuation_Type>(m_pData->header.Attenuation_Type);
}

short CECAT7SubHeaderAttenCorr::num_R_Elements(void) const
{
  return m_pData->header.Num_R_Elements;
}

short CECAT7SubHeaderAttenCorr::num_Angles(void) const
{
  return m_pData->header.Num_Angles;
}

short CECAT7SubHeaderAttenCorr::num_Z_Elements(void) const
{
  return m_pData->header.Num_Z_Elements;
}

short CECAT7SubHeaderAttenCorr::ring_Difference(void) const
{
  return m_pData->header.Ring_Difference;
}

float CECAT7SubHeaderAttenCorr::x_Resolution(void) const
{
  return m_pData->header.X_Resolution;
}

float CECAT7SubHeaderAttenCorr::y_Resolution(void) const
{
  return m_pData->header.Y_Resolution;
}

float  CECAT7SubHeaderAttenCorr::z_Resolution(void) const
{
  return m_pData->header.Z_Resolution;
}

float CECAT7SubHeaderAttenCorr::w_Resolution(void) const
{
  return m_pData->header.W_Resolution;
}

float  CECAT7SubHeaderAttenCorr::scale_Factor(void) const
{
  return m_pData->header.Scale_Factor;
}

float  CECAT7SubHeaderAttenCorr::x_Offset(void) const
{
  return m_pData->header.X_Offset;
}

float  CECAT7SubHeaderAttenCorr::y_Offset(void) const
{
  return m_pData->header.Y_Offset;
}

float  CECAT7SubHeaderAttenCorr::x_Radius(void) const
{
  return m_pData->header.X_Radius;
}

float  CECAT7SubHeaderAttenCorr::y_Radius(void) const
{
  return m_pData->header.Y_Radius;
}

float  CECAT7SubHeaderAttenCorr::tilt_Angle(void) const
{
  return m_pData->header.Tilt_Angle;
}

float CECAT7SubHeaderAttenCorr::attenuation_Coeff(void) const
{
  return m_pData->header.Attenuation_Coeff;
}

float  CECAT7SubHeaderAttenCorr::attenuation_Min(void) const
{
  return m_pData->header.Attenuation_Min;
}

float  CECAT7SubHeaderAttenCorr::attenuation_Max(void) const
{
  return m_pData->header.Attenuation_Max;
}

float  CECAT7SubHeaderAttenCorr::skull_Thickness(void) const
{
  return m_pData->header.Skull_Thickness;
}

short  CECAT7SubHeaderAttenCorr::num_Additional_Atten_Coeff(void) const
{
  return m_pData->header.Num_Additional_Atten_Coeff;
}

float  CECAT7SubHeaderAttenCorr::additional_Atten_Coeff(int n) const
{
  return m_pData->header.Additional_Atten_Coeff[n];
}

float  CECAT7SubHeaderAttenCorr::edge_Finding_Threshold(void) const
{
  return m_pData->header.Edge_Finding_Threshold;
}

short CECAT7SubHeaderAttenCorr::storage_Order(void) const
{
  return m_pData->header.Storage_Order;
}

short CECAT7SubHeaderAttenCorr::span(void) const
{
  return m_pData->header.Span;
}

short CECAT7SubHeaderAttenCorr::z_Elements(int n) const
{
  return m_pData->header.Z_Elements[n];
}

short  CECAT7SubHeaderAttenCorr::unused(int n) const
{
  return m_pData->header.Unused[n];
}

short CECAT7SubHeaderAttenCorr::cti_Reserved(int n) const
{
  return m_pData->header.CTI_Reserved[n];
}

void CECAT7SubHeaderAttenCorr::setData_Type(const CECATSubHeader::Data_Type dType)
{
  m_pData->header.Data_Type = static_cast<quint16>(dType);
}

void CECAT7SubHeaderAttenCorr::setNum_Dimensions(const short n)
{
  m_pData->header.Num_Dimensions = n;
}

void CECAT7SubHeaderAttenCorr::setAttenuation_Type(const Attenuation_Type n)
{
  m_pData->header.Attenuation_Type = static_cast<quint16>(n);
}

void CECAT7SubHeaderAttenCorr::setNum_R_Elements(const short n)
{
  m_pData->header.Num_R_Elements = n;
}

void CECAT7SubHeaderAttenCorr::setNum_Angles(const short n)
{
  m_pData->header.Num_Angles = n;
}

void CECAT7SubHeaderAttenCorr::setNum_Z_Elements(const short n)
{
  m_pData->header.Num_Z_Elements = n;
}

void CECAT7SubHeaderAttenCorr::setRing_Difference(const short n)
{
  m_pData->header.Ring_Difference = n;
}

void CECAT7SubHeaderAttenCorr::setX_Resolution(const float n)
{
  m_pData->header.X_Resolution = n;
}

void CECAT7SubHeaderAttenCorr::setY_Resolution(const float n)
{
  m_pData->header.Y_Resolution = n;
}

void CECAT7SubHeaderAttenCorr::setZ_Resolution(const float n)
{
  m_pData->header.Z_Resolution = n;
}

void CECAT7SubHeaderAttenCorr::setW_Resolution(const float n)
{
  m_pData->header.W_Resolution = n;
}

void CECAT7SubHeaderAttenCorr::setScale_Factor(const float n)
{
  m_pData->header.Scale_Factor = n;
}

void CECAT7SubHeaderAttenCorr::setX_Offset(const float n)
{
  m_pData->header.X_Offset = n;
}

void CECAT7SubHeaderAttenCorr::setY_Offset(const float n)
{
  m_pData->header.Y_Offset = n;
}

void CECAT7SubHeaderAttenCorr::setX_Radius(const float n)
{
  m_pData->header.X_Radius = n;
}

void CECAT7SubHeaderAttenCorr::setY_Radius(const float n)
{
  m_pData->header.Y_Radius = n;
}

void CECAT7SubHeaderAttenCorr::setTilt_Angle(const float n)
{
  m_pData->header.Tilt_Angle = n;
}

void CECAT7SubHeaderAttenCorr::setAttenuation_Coeff(const float n)
{
  m_pData->header.Attenuation_Coeff = n;
}

void CECAT7SubHeaderAttenCorr::setAttenuation_Min(const float n)
{
  m_pData->header.Attenuation_Min = n;
}

void CECAT7SubHeaderAttenCorr::setAttenuation_Max(const float n)
{
  m_pData->header.Attenuation_Max = n;
}

void CECAT7SubHeaderAttenCorr::setSkull_Thickness(const float n)
{
  m_pData->header.Skull_Thickness = n;
}

void CECAT7SubHeaderAttenCorr::setNum_Additional_Atten_Coeff(const short n)
{
  m_pData->header.Num_Additional_Atten_Coeff = n;
}

void CECAT7SubHeaderAttenCorr::setAdditional_Atten_Coeff(const short i, const float n)
{
  m_pData->header.Additional_Atten_Coeff[i] = n;
}

void CECAT7SubHeaderAttenCorr::setEdge_Finding_Threshold(const float n)
{
  m_pData->header.Edge_Finding_Threshold = n;
}

void CECAT7SubHeaderAttenCorr::setStorage_Order(const short n)
{
  m_pData->header.Storage_Order = n;
}

void CECAT7SubHeaderAttenCorr::setSpan(const short n)
{
  m_pData->header.Span = n;
}

void CECAT7SubHeaderAttenCorr::setZ_Elements(const short i, const short n)
{
  m_pData->header.Z_Elements[i] = n;
}

void CECAT7SubHeaderAttenCorr::setUnused(const short i, const short n)
{
  m_pData->header.Unused[i] = n;
}

void CECAT7SubHeaderAttenCorr::setCTI_Reserverd(const short i, const short n)
{
  m_pData->header.CTI_Reserved[i] = n;
}
