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

#include "CECAT7SubHeaderNorm.h"
#include "CECATDirectoryItem.h"
#include "CECATFile.h"

#include <rtdebug.h>

#include "bswap.h"

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CECAT7SubHeaderNormPrivate
{
  public:
    #define SUBHEADER_SIZE 512
    #pragma pack(1)
    struct HeaderData
    {
      quint16  Data_Type;                //   0: Data_Type
      quint16  Num_Dimensions;           //   2: Num_Dimensions
      quint16  Num_R_Elements;           //   4: Num_R_Elements
      quint16  Num_Angles;               //   6: Num_Angles
      quint16  Num_Z_Elements;           //   8: Num_Z_Elements
      quint16  Ring_Difference;          //  10: Ring_Difference
      float    Scale_Factor;             //  12: Scale_Factor
      float    Norm_Min;                 //  16: Norm_Min
      float    Norm_Max;                 //  20: Norm_Max
      float    FOV_Source_Width;         //  24: FOV_Source_Width
      float    Norm_Quality_Factor;      //  28: Norm_Quality_Factor
      quint16  Norm_Quality_Factor_Code; //  32: Norm_Quality_Factor_Code
      quint16  Storage_Order;            //  34: Storage_Order
      quint16  Span;                     //  36: Span
      quint16  Z_Elements[64];           //  38: Z_Elements (64)
      quint16  CTI_reserved[123];        // 123: CTI_reserved (123)
      quint16  User_Reserved[50];        // 412: User_Reserved (50)
    } header;                            
    #pragma pack()                                                       
};                                      

CECAT7SubHeaderNorm::CECAT7SubHeaderNorm(CECATFile* ecatFile,
                                         CECATDirectoryItem* pDirItem)
  : CECATSubHeader(ecatFile, pDirItem)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CECAT7SubHeaderNormPrivate();
  
  clear();

  LEAVE();

}

CECAT7SubHeaderNorm::~CECAT7SubHeaderNorm()
{
  ENTER();

  delete m_pData;

  LEAVE();
}

CECAT7SubHeaderNorm::CECAT7SubHeaderNorm(const CECAT7SubHeaderNorm& src)
  : CECATSubHeader(src)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CECAT7SubHeaderNormPrivate(*(src.m_pData));

  LEAVE();
}

CECAT7SubHeaderNorm& CECAT7SubHeaderNorm::operator=(const CECAT7SubHeaderNorm& src)
{
  ENTER();

  if(m_pData != src.m_pData)
  {
    memcpy(&m_pData->header, 
           &src.m_pData->header, 
           sizeof(struct CECAT7SubHeaderNormPrivate::HeaderData));
  }
  
  LEAVE();
  return *this;
}

void CECAT7SubHeaderNorm::clear()
{
  ENTER();

  // then clear the structure
  memset(&m_pData->header, 0, sizeof(struct CECAT7SubHeaderNormPrivate::HeaderData));

  setData_Type(CECATSubHeader::UnknownDataType);
  setNorm_Quality_Factor_Code(TBD);
    
  LEAVE();
}

bool CECAT7SubHeaderNorm::load(void)
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
  
  // check that we seeked correctly
  ASSERT(mData->pos() == m_pDirItem->dataBlock_Start());

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
    BSWAP_16(m_pData->header.Num_R_Elements);
    BSWAP_16(m_pData->header.Num_Angles);
    BSWAP_16(m_pData->header.Num_Z_Elements);
    BSWAP_16(m_pData->header.Ring_Difference);
    BSWAP_FLT(m_pData->header.Scale_Factor);
    BSWAP_FLT(m_pData->header.Norm_Min);
    BSWAP_FLT(m_pData->header.Norm_Max);
    BSWAP_FLT(m_pData->header.FOV_Source_Width);
    BSWAP_FLT(m_pData->header.Norm_Quality_Factor);
    BSWAP_16(m_pData->header.Norm_Quality_Factor_Code);
    BSWAP_16(m_pData->header.Storage_Order);
    BSWAP_16(m_pData->header.Span);
    for(int i=0; i < 64; i++)
      BSWAP_16(m_pData->header.Z_Elements[i]);

    for(int i=0; i < 123; i++)
      BSWAP_16(m_pData->header.CTI_reserved[i]);

    for(int i=0; i < 50; i++)
      BSWAP_16(m_pData->header.User_Reserved[i]);
  }

  // some more debug output
#if defined(DEBUG)
  D("ECAT7 Normalization SubHeader loaded:");
  D("------------------------------------");
  D("Data_Type                 : %d",        m_pData->header.Data_Type);
  D("Num_Dimensions            : %d",        m_pData->header.Num_Dimensions);
  D("Num_R_Elements            : %d",        m_pData->header.Num_R_Elements);
  D("Num_Angles                : %d",        m_pData->header.Num_Angles);
  D("Num_Z_Elements            : %d",        m_pData->header.Num_Z_Elements);
  D("Ring_Difference           : %d",        m_pData->header.Ring_Difference);
  D("Scale_Factor              : %f",        m_pData->header.Scale_Factor);
  D("Norm_Min                  : %f",        m_pData->header.Norm_Min);
  D("Norm_Max                  : %f",        m_pData->header.Norm_Max);
  D("FOV_Source_Width          : %f cm",     m_pData->header.FOV_Source_Width);
  D("Norm_Quality_Factor       : %f",        m_pData->header.Norm_Quality_Factor);
  D("Norm_Quality_Factor_Code  : %d",        m_pData->header.Norm_Quality_Factor_Code);
  D("Storage_Order             : %d",        m_pData->header.Storage_Order);
  D("Span                      : %d",        m_pData->header.Span);
  for(int i=0; i < 64; i++)
    D("Z_Elements              [%d]: %f", i+1, m_pData->header.Z_Elements[i]);
#endif

  RETURN(true);
  return true;
}

bool CECAT7SubHeaderNorm::save(void) const
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

  // check that we seeked correctly
  ASSERT(mData->pos() == m_pDirItem->dataBlock_Start());

  ASSERT(sizeof(m_pData->header) == SUBHEADER_SIZE);
  struct CECAT7SubHeaderNormPrivate::HeaderData* header = NULL;
  if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
  {
    header = new CECAT7SubHeaderNormPrivate::HeaderData;

    // copy the current m_pData->header to beHeader
    memcpy(header, &m_pData->header, sizeof(m_pData->header));

    // we only swap non-char elements of the header
    BSWAP_16(header->Data_Type);
    BSWAP_16(header->Num_Dimensions);
    BSWAP_16(header->Num_R_Elements);
    BSWAP_16(header->Num_Angles);
    BSWAP_16(header->Num_Z_Elements);
    BSWAP_16(header->Ring_Difference);
    BSWAP_FLT(header->Scale_Factor);
    BSWAP_FLT(header->Norm_Min);
    BSWAP_FLT(header->Norm_Max);
    BSWAP_FLT(header->FOV_Source_Width);
    BSWAP_FLT(header->Norm_Quality_Factor);
    BSWAP_16(header->Norm_Quality_Factor_Code);
    BSWAP_16(header->Storage_Order);
    BSWAP_16(header->Span);
    for(int i=0; i < 64; i++)
      BSWAP_16(header->Z_Elements[i]);

    for(int i=0; i < 123; i++)
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

int CECAT7SubHeaderNorm::rawDataSize() const
{
  return 1*ECAT_BLOCKSIZE;
}

CECATSubHeader::Type CECAT7SubHeaderNorm::subHeaderType(void) const
{
  return CECATSubHeader::ECAT7_Norm;
}

bool CECAT7SubHeaderNorm::convertFrom(const CMedIOHeader* subHeader, const CMedIOHeader*)
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
        case CECATSubHeader::ECAT7_Norm:
        {
          // we use the assignment operator which will do the convertation
          // for us.
          *this = *static_cast<const CECAT7SubHeaderNorm*>(subHeader);
          
          bResult = true;
        }
        break;

        case CECATSubHeader::ECAT7_AttenCorr:
        case CECATSubHeader::ECAT7_Image:
        case CECATSubHeader::ECAT7_Norm3D:
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

CMedIOHeader* CECAT7SubHeaderNorm::clone() const
{
  ENTER();

  CECAT7SubHeaderNorm* pNewHead = new CECAT7SubHeaderNorm();
  pNewHead->convertFrom(this);

  RETURN(pNewHead);
  return pNewHead;
}

// data access methods
CECATSubHeader::Data_Type CECAT7SubHeaderNorm::data_Type(void) const
{
  return static_cast<CECATSubHeader::Data_Type>(m_pData->header.Data_Type);  }

short CECAT7SubHeaderNorm::num_Dimensions(void) const
{
  return m_pData->header.Num_Dimensions;
}

short CECAT7SubHeaderNorm::num_R_Elements(void) const
{
  return m_pData->header.Num_R_Elements;
}

short CECAT7SubHeaderNorm::num_Angles(void) const
{
  return m_pData->header.Num_Angles;
}

short CECAT7SubHeaderNorm::num_Z_Elements(void) const
{
  return m_pData->header.Num_Z_Elements;
}

short CECAT7SubHeaderNorm::ring_Difference(void) const
{
  return m_pData->header.Ring_Difference;
}

float CECAT7SubHeaderNorm::scale_Factor(void) const
{
  return m_pData->header.Scale_Factor;
}

float CECAT7SubHeaderNorm::norm_Min(void) const
{
  return m_pData->header.Norm_Min;
}

float CECAT7SubHeaderNorm::norm_Max(void) const
{
  return m_pData->header.Norm_Max;
}

float CECAT7SubHeaderNorm::fov_Source_Width(void) const
{
  return m_pData->header.FOV_Source_Width;
}

float CECAT7SubHeaderNorm::norm_Quality_Factor(void) const
{
  return m_pData->header.Norm_Quality_Factor;
}

CECAT7SubHeaderNorm::Norm_Qual_Factor_Code CECAT7SubHeaderNorm::norm_Quality_Factor_Code(void) const
{
  return static_cast<CECAT7SubHeaderNorm::Norm_Qual_Factor_Code>(m_pData->header.Norm_Quality_Factor_Code);
}

short CECAT7SubHeaderNorm::storage_Order(void) const
{
  return m_pData->header.Storage_Order;
}

short CECAT7SubHeaderNorm::span(void) const
{
  return m_pData->header.Span;
}

short CECAT7SubHeaderNorm::z_Elements(const short i) const
{
  return m_pData->header.Z_Elements[i];
}

short CECAT7SubHeaderNorm::cti_Reserved(const short i) const
{
  return m_pData->header.CTI_reserved[i];
}

short CECAT7SubHeaderNorm::user_Reserved(const short i) const
{
  return m_pData->header.User_Reserved[i];
}


void CECAT7SubHeaderNorm::setData_Type(const CECATSubHeader::Data_Type dType)
{
  m_pData->header.Data_Type = static_cast<quint16>(dType);
}  

void CECAT7SubHeaderNorm::setNum_Dimensions(const short n)
{
  m_pData->header.Num_Dimensions = n;
}

void CECAT7SubHeaderNorm::setNum_R_Elements(const short n)
{
  m_pData->header.Num_R_Elements = n;
}

void CECAT7SubHeaderNorm::setNum_Angles(const short n)
{
  m_pData->header.Num_Angles = n;
}

void CECAT7SubHeaderNorm::setNum_Z_Elements(const short n)
{
  m_pData->header.Num_Z_Elements = n;
}

void CECAT7SubHeaderNorm::setRing_Difference(const short n)
{
  m_pData->header.Ring_Difference = n;
}

void CECAT7SubHeaderNorm::setScale_Factor(const float n)
{
  m_pData->header.Scale_Factor = n;
}

void CECAT7SubHeaderNorm::setNorm_Min(const float n)
{
  m_pData->header.Norm_Min = n;
}

void CECAT7SubHeaderNorm::setNorm_Max(const float n)
{
  m_pData->header.Norm_Max = n;
}

void CECAT7SubHeaderNorm::setFOV_Source_Width(const float n)
{
  m_pData->header.FOV_Source_Width = n;
}

void CECAT7SubHeaderNorm::setNorm_Quality_Factor(const float n)
{
  m_pData->header.Norm_Quality_Factor = n;
}

void CECAT7SubHeaderNorm::setNorm_Quality_Factor_Code(const Norm_Qual_Factor_Code n)
{
  m_pData->header.Norm_Quality_Factor_Code = static_cast<quint16>(n);
}

void CECAT7SubHeaderNorm::setStorage_Order(const short n)
{
  m_pData->header.Storage_Order = n;
}

void CECAT7SubHeaderNorm::setSpan(const short n)
{
  m_pData->header.Span = n;
}

void CECAT7SubHeaderNorm::setZ_Elements(const short i, const short n)
{
  m_pData->header.Z_Elements[i] = n;
}

void CECAT7SubHeaderNorm::setCTI_Reserved(const short i, const short n)
{
  m_pData->header.CTI_reserved[i] = n;
}

void CECAT7SubHeaderNorm::setUser_Reserved(const short i, const short n)
{
  m_pData->header.User_Reserved[i] = n;
}

