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

#include "CECAT7SubHeaderNorm3D.h"
#include "CECATDirectoryItem.h"
#include "CECATFile.h"

#include <rtdebug.h>

#include "bswap.h"

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CECAT7SubHeaderNorm3DPrivate
{
  public:
    #define SUBHEADER_SIZE 512
    #pragma pack(1)
    struct HeaderData
    {
      quint16  Data_Type;                //   0: Data_Type
      quint16  Num_R_Elements;           //   2: Num_R_Elements
      quint16  Num_Transaxial_Crystals;  //   4: Num_Transaxial_Crystals
      quint16  Num_Crystal_Rings;        //   6: Num_Crystal_Rings
      quint16  Crystals_Per_Ring;        //   8: Crystals_Per_Ring
      quint16  Num_Geo_Corr_Planes;      //  10: Num_Geo_Corr_Planes
      quint16  ULD;                      //  12: ULD
      quint16  LLD;                      //  14: LLD
      quint16  Scatter_Energy;           //  16: Scatter_Energy
      float    Norm_Quality_Factor;      //  18: Norm_Quality_Factor
      quint16  Norm_Quality_Factor_Code; //  22: Norm_Quality_Factor
      float    Ring_DTCor1[32];          //  24: Ring_DTCor1 (32)                               
      float    Ring_DTCor2[32];          // 152: Ring_DTCor2 (32)
      float    Crystal_DTCor[8];         // 280: Crystal_DTCor (8)                                
      quint16  Span;                     // 312: Span
      quint16  Max_Ring_Diff;            // 314: Max_Ring_Diff                               
      quint16  CTI_reserved[48];         // 316: CTI_reserved
      quint16  User_Reserved[50];        // 412: User_Reserved 
    } header;                             
    #pragma pack()
};                                                                       
                                          
CECAT7SubHeaderNorm3D::CECAT7SubHeaderNorm3D(CECATFile* ecatFile,
                                             CECATDirectoryItem* pDirItem)
  : CECATSubHeader(ecatFile, pDirItem)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CECAT7SubHeaderNorm3DPrivate();
  
  clear();

  LEAVE();
}

CECAT7SubHeaderNorm3D::~CECAT7SubHeaderNorm3D()
{
  ENTER();

  delete m_pData;

  LEAVE();
}

CECAT7SubHeaderNorm3D::CECAT7SubHeaderNorm3D(const CECAT7SubHeaderNorm3D& src)
  : CECATSubHeader(src)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CECAT7SubHeaderNorm3DPrivate(*(src.m_pData));

  LEAVE();
}

CECAT7SubHeaderNorm3D& CECAT7SubHeaderNorm3D::operator=(const CECAT7SubHeaderNorm3D& src)
{
  ENTER();

  if(m_pData != src.m_pData)
  {
    memcpy(&m_pData->header, 
           &src.m_pData->header, 
           sizeof(struct CECAT7SubHeaderNorm3DPrivate::HeaderData));
  }

  LEAVE();
  return *this;
}

void CECAT7SubHeaderNorm3D::clear()
{
  ENTER();

  // then clear the structure
  memset(&m_pData->header, 0, sizeof(struct CECAT7SubHeaderNorm3DPrivate::HeaderData));

  setData_Type(CECATSubHeader::UnknownDataType);
  setNorm_Quality_Factor_Code(TBD);

  LEAVE();
}

bool CECAT7SubHeaderNorm3D::load(void)
{
  ENTER();
  CMedIOData* mData = medIOData();

  // check if the stream is readable or not and
  // set our MedIOData to the correct file position so that we can
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
    BSWAP_16(m_pData->header.Num_R_Elements);
    BSWAP_16(m_pData->header.Num_Transaxial_Crystals);
    BSWAP_16(m_pData->header.Num_Crystal_Rings);
    BSWAP_16(m_pData->header.Crystals_Per_Ring);
    BSWAP_16(m_pData->header.Num_Geo_Corr_Planes);
    BSWAP_16(m_pData->header.ULD);
    BSWAP_16(m_pData->header.LLD);
    BSWAP_16(m_pData->header.Scatter_Energy);
    BSWAP_FLT(m_pData->header.Norm_Quality_Factor);
    BSWAP_16(m_pData->header.Norm_Quality_Factor_Code);
    for(int i=0; i < 32; i++)
    {
      BSWAP_FLT(m_pData->header.Ring_DTCor1[i]);
      BSWAP_FLT(m_pData->header.Ring_DTCor2[i]);
    }

    for(int i=0; i < 8; i++)
      BSWAP_FLT(m_pData->header.Crystal_DTCor[i]);

    BSWAP_16(m_pData->header.Span);
    BSWAP_16(m_pData->header.Max_Ring_Diff);

    for(int i=0; i < 48; i++)
      BSWAP_16(m_pData->header.CTI_reserved[i]);

    for(int i=0; i < 50; i++)
      BSWAP_16(m_pData->header.User_Reserved[i]);
  }

  // some more debug output
#if defined(DEBUG)
  D("ECAT7 Normalization3D SubHeader loaded:");
  D("------------------------------------");
  D("Data_Type                 : %d",        m_pData->header.Data_Type);
  D("Num_R_Elements            : %d",        m_pData->header.Num_R_Elements);
  D("Num_Transaxial_Crystals   : %d",        m_pData->header.Num_Transaxial_Crystals);
  D("Num_Crystal_Rings         : %d",        m_pData->header.Num_Crystal_Rings);
  D("Crystals_Per_Ring         : %d",        m_pData->header.Crystals_Per_Ring);
  D("Num_Geo_Corr_Planes       : %d",        m_pData->header.Num_Geo_Corr_Planes);
  D("ULD                       : %d",        m_pData->header.ULD);
  D("LLD                       : %d",        m_pData->header.LLD);
  D("Scatter_Energy            : %d",        m_pData->header.Scatter_Energy);
  D("Norm_Quality_Factor       : %g",        m_pData->header.Norm_Quality_Factor);
  D("Norm_Quality_Factor_Code  : %d",        m_pData->header.Norm_Quality_Factor_Code);
  for(int i=0; i < 32; i++)
  {
    D("Ring_DTCOR1           [%02d]: %g", i+1, m_pData->header.Ring_DTCor1[i]);
  }
  for(int i=0; i < 32; i++)
  {
    D("Ring_DTCOR2           [%02d]: %g", i+1, m_pData->header.Ring_DTCor2[i]);
  }
  for(int i=0; i < 8; i++)
  {
    D("Crystal_DTCor         [%02d]: %g", i+1, m_pData->header.Crystal_DTCor[i]);
  }
  D("Span                      : %d",        m_pData->header.Span);
  D("Max_Ring_Diff             : %d",        m_pData->header.Max_Ring_Diff);
#endif

  RETURN(true);
  return true;
}

bool CECAT7SubHeaderNorm3D::save(void) const
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
  struct CECAT7SubHeaderNorm3DPrivate::HeaderData* header = NULL;
  if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
  {
    header = new CECAT7SubHeaderNorm3DPrivate::HeaderData;

    // copy the current m_pData->header to beHeader
    memcpy(header, &m_pData->header, sizeof(m_pData->header));

    // we only swap non-char elements of the header
    BSWAP_16(header->Data_Type);
    BSWAP_16(header->Num_R_Elements);
    BSWAP_16(header->Num_Transaxial_Crystals);
    BSWAP_16(header->Num_Crystal_Rings);
    BSWAP_16(header->Crystals_Per_Ring);
    BSWAP_16(header->Num_Geo_Corr_Planes);
    BSWAP_16(header->ULD);
    BSWAP_16(header->LLD);
    BSWAP_16(header->Scatter_Energy);
    BSWAP_FLT(header->Norm_Quality_Factor);
    BSWAP_16(header->Norm_Quality_Factor_Code);
    for(int i=0; i < 32; i++)
    {
      BSWAP_FLT(header->Ring_DTCor1[i]);
      BSWAP_FLT(header->Ring_DTCor2[i]);
    }

    for(int i=0; i < 8; i++)
      BSWAP_FLT(header->Crystal_DTCor[i]);

    BSWAP_16(header->Span);
    BSWAP_16(header->Max_Ring_Diff);

    for(int i=0; i < 48; i++)
      BSWAP_16(header->CTI_reserved[i]);

    for(int i=0; i < 50; i++)
      BSWAP_16(header->User_Reserved[i]);
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

int CECAT7SubHeaderNorm3D::rawDataSize() const
{
  return 1*ECAT_BLOCKSIZE;
}

CECATSubHeader::Type CECAT7SubHeaderNorm3D::subHeaderType(void) const
{
  return CECATSubHeader::ECAT7_Norm3D;
}

bool CECAT7SubHeaderNorm3D::convertFrom(const CMedIOHeader* subHeader, const CMedIOHeader*)
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
        case CECATSubHeader::ECAT7_Norm3D:
        {
          // we use the assignment operator which will do the convertation
          // for us.
          *this = *static_cast<const CECAT7SubHeaderNorm3D*>(subHeader);

          bResult = true;
        }
        break;

        case CECATSubHeader::ECAT7_AttenCorr:
        case CECATSubHeader::ECAT7_Image:
        case CECATSubHeader::ECAT7_Norm:
        case CECATSubHeader::ECAT7_PolarMap:
        case CECATSubHeader::ECAT7_Scan:
        case CECATSubHeader::ECAT7_Scan3D:
        {
          E("medio subheader %d conversion not implemented!", eSubHeader->subHeaderType());
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
      // copying a main header into a sub header doesn't make much sense, so we
      // do nothing here
    break;

    case CMedIOHeader::ConcordeMicroPetFrameHeader:
    case CMedIOHeader::PhilipsSubHeader:
    case CMedIOHeader::PhilipsListviewHeader:
    {
      E("medio subheader %d conversion not implemented!", subHeader->headerFormat());
    }
    break;

    case CMedIOHeader::Unknown:
      // for an unknown header type we do nothing
    break;
  }

  RETURN(bResult);
  return bResult;
}

CMedIOHeader* CECAT7SubHeaderNorm3D::clone() const
{
  ENTER();

  CECAT7SubHeaderNorm3D* pNewHead = new CECAT7SubHeaderNorm3D();
  pNewHead->convertFrom(this);

  RETURN(pNewHead);
  return pNewHead;
}

// access methods to get directly data out of
// the SubHeader
CECATSubHeader::Data_Type CECAT7SubHeaderNorm3D::data_Type(void) const
{
  return static_cast<CECATSubHeader::Data_Type>(m_pData->header.Data_Type);  }

short CECAT7SubHeaderNorm3D::num_R_Elements(void) const
{
  return m_pData->header.Num_R_Elements;
}

short CECAT7SubHeaderNorm3D::num_Transaxial_Crystals(void) const
{
  return m_pData->header.Num_Transaxial_Crystals;
}

short CECAT7SubHeaderNorm3D::num_Crystal_Rings(void) const
{
  return m_pData->header.Num_Crystal_Rings;
}

short CECAT7SubHeaderNorm3D::crystals_Per_Ring(void) const
{
  return m_pData->header.Crystals_Per_Ring;
}

short CECAT7SubHeaderNorm3D::num_Geo_Corr_Planes(void) const
{
  return m_pData->header.Num_Geo_Corr_Planes;
}

short CECAT7SubHeaderNorm3D::uld(void) const
{
  return m_pData->header.ULD;
}

short CECAT7SubHeaderNorm3D::lld(void) const
{
  return m_pData->header.LLD;
}

short CECAT7SubHeaderNorm3D::scatter_Energy(void) const
{
  return m_pData->header.Scatter_Energy;
}

float CECAT7SubHeaderNorm3D::norm_Quality_Factor(void) const
{
  return m_pData->header.Norm_Quality_Factor;
}

CECAT7SubHeaderNorm3D::Norm_Qual_Factor_Code CECAT7SubHeaderNorm3D::norm_Quality_Factor_Code(void) const
{
  return static_cast<CECAT7SubHeaderNorm3D::Norm_Qual_Factor_Code>(m_pData->header.Norm_Quality_Factor_Code);
}

float CECAT7SubHeaderNorm3D::ring_DTCor1(const short i) const
{
  return m_pData->header.Ring_DTCor1[i];
}

float CECAT7SubHeaderNorm3D::ring_DTCor2(const short i) const
{
  return m_pData->header.Ring_DTCor2[i];
}

float CECAT7SubHeaderNorm3D::crystal_DTCor(const short i) const
{
  return m_pData->header.Crystal_DTCor[i];
}

short CECAT7SubHeaderNorm3D::span(void) const
{
  return m_pData->header.Span;
}

short CECAT7SubHeaderNorm3D::max_Ring_Diff(void) const
{
  return m_pData->header.Max_Ring_Diff;
}

short CECAT7SubHeaderNorm3D::cti_Reserved(const short i) const
{
  return m_pData->header.CTI_reserved[i];
}

short CECAT7SubHeaderNorm3D::user_Reserved(const short i) const
{
  return m_pData->header.User_Reserved[i];
}


void CECAT7SubHeaderNorm3D::setData_Type(const CECATSubHeader::Data_Type dType)
{
  m_pData->header.Data_Type = static_cast<quint16>(dType);
}      

void CECAT7SubHeaderNorm3D::setNum_R_Elements(const short n)
{
  m_pData->header.Num_R_Elements = n;
}

void CECAT7SubHeaderNorm3D::setNum_Transaxial_Crystals(const short n)
{
  m_pData->header.Num_Transaxial_Crystals = n;
}

void CECAT7SubHeaderNorm3D::setNum_Crystal_Rings(const short n)
{
  m_pData->header.Num_Crystal_Rings = n;
}

void CECAT7SubHeaderNorm3D::setCrystals_Per_Ring(const short n)
{
  m_pData->header.Crystals_Per_Ring = n;
}

void CECAT7SubHeaderNorm3D::setNum_Geo_Corr_Planes(const short n)
{
  m_pData->header.Num_Geo_Corr_Planes = n;
}

void CECAT7SubHeaderNorm3D::setULD(const short n)
{
  m_pData->header.ULD = n;
}

void CECAT7SubHeaderNorm3D::setLLD(const short n)
{
  m_pData->header.LLD = n;
}

void CECAT7SubHeaderNorm3D::setScatter_Energy(const short n)
{
  m_pData->header.Scatter_Energy = n;
}

void CECAT7SubHeaderNorm3D::setNorm_Quality_Factor(const float n)
{
  m_pData->header.Norm_Quality_Factor = n;
}

void CECAT7SubHeaderNorm3D::setNorm_Quality_Factor_Code(const Norm_Qual_Factor_Code n)
{
  m_pData->header.Norm_Quality_Factor_Code = static_cast<quint16>(n);
}

void CECAT7SubHeaderNorm3D::setRing_DTCor1(const short i, const float n)
{
  m_pData->header.Ring_DTCor1[i] = n;
}

void CECAT7SubHeaderNorm3D::setRing_DTCor2(const short i, const float n)
{
  m_pData->header.Ring_DTCor2[i] = n;
}

void CECAT7SubHeaderNorm3D::setCrystal_DTCor(const short i, const float n)
{
  m_pData->header.Crystal_DTCor[i] = n;
}

void CECAT7SubHeaderNorm3D::setSpan(const short n)
{
  m_pData->header.Span = n;
}

void CECAT7SubHeaderNorm3D::setMax_Ring_Diff(const short n)
{
  m_pData->header.Max_Ring_Diff = n;
}

void CECAT7SubHeaderNorm3D::setCTI_Reserved(const short i, const short n)
{
  m_pData->header.CTI_reserved[i] = n;
}

void CECAT7SubHeaderNorm3D::setUser_Reserved(const short i, const short n)
{
  m_pData->header.User_Reserved[i] = n;
}

