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

#include "CECAT7SubHeaderScan3D.h"
#include "CECATDirectoryItem.h"
#include "CECATFile.h"

#include <QDataStream>

#include <rtdebug.h>

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CECAT7SubHeaderScan3DPrivate
{
  public:
    struct HeaderData
    {
      quint16  Data_Type;
      quint16  Num_Dimensions;
      quint16  Num_R_Elements;
      quint16  Num_Angles;
      quint16  Corrections_Applied;
      quint16  Num_Z_Elements[64];
      quint16  Ring_Difference;
      quint16  Storage_Order;
      quint16  Axial_Compression;
      float    X_Resolution;
      float    V_Resolution;
      float    Z_Resolution;
      float    W_Resolution;
      quint16  Gate_Reserved[6];
      quint32  Gate_Duration;
      quint32  R_Wave_Offset;
      quint32  Num_Accepted_Beats;
      float    Scale_Factor;
      qint16  Scan_Min;
      qint16  Scan_Max;
      quint32  Prompts;
      quint32  Delayed;
      quint32  Multiples;
      quint32  Net_Trues;
      float    Tot_Avg_Cor;
      float    Tot_Avg_Uncor;
      quint32  Total_Coin_Rate;
      quint32  Frame_Start_Time;
      quint32  Frame_Duration;
      float    Deadtime_Correction_Factor;
      quint16  CTI_reserved[90];
      quint16  User_Reserved[50];
      float    Uncor_Singles[128];
    } header;
};

CECAT7SubHeaderScan3D::CECAT7SubHeaderScan3D(CECATFile* ecatFile,
                                             CECATDirectoryItem* pDirItem)
  : CECATSubHeader(ecatFile, pDirItem)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CECAT7SubHeaderScan3DPrivate();
  
  clear();

  LEAVE();
}

CECAT7SubHeaderScan3D::~CECAT7SubHeaderScan3D()
{
  ENTER();

  delete m_pData;

  LEAVE();
}

CECAT7SubHeaderScan3D::CECAT7SubHeaderScan3D(const CECAT7SubHeaderScan3D& src)
  : CECATSubHeader(src)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CECAT7SubHeaderScan3DPrivate(*(src.m_pData));

  LEAVE();
}

CECAT7SubHeaderScan3D& CECAT7SubHeaderScan3D::operator=(const CECAT7SubHeaderScan3D& src)
{
  ENTER();

  if(m_pData != src.m_pData)
  {
    memcpy(&m_pData->header, 
           &src.m_pData->header, 
           sizeof(struct CECAT7SubHeaderScan3DPrivate::HeaderData));
  }

  LEAVE();
  return *this;
}

void CECAT7SubHeaderScan3D::clear()
{
  ENTER();

  // then clear the structure
  memset(&m_pData->header, 0, sizeof(struct CECAT7SubHeaderScan3DPrivate::HeaderData));

  setData_Type(CECATSubHeader::UnknownDataType);
  setStorage_Order(ViewMode);

  LEAVE();
}

bool CECAT7SubHeaderScan3D::load(void)
{
  ENTER();

  // check if the stream is readable or not and
  // set our MedIOData to the correct file position so that we can
  // read the subheader
  if(m_pMedIOData->isReadable() == false ||
     m_pDirItem->dataBlock_Start() == 0 ||
     m_pMedIOData->seek(m_pDirItem->dataBlock_Start()) == false)
  {
    RETURN(false);
    return false;
  }

  // we use a ByteArray buffer to speed up the endianess
  // decoding
  QByteArray buffer(rawDataSize(), 0);
  if(m_pMedIOData->read(buffer.data(), buffer.size()) != rawDataSize())
  {
    RETURN(false);
    return false;
  }

  // now we generate a QDataStream on our buffer so that we can read
  // out of the buffer instead of the raw file (> speed)
  QDataStream stream(buffer);

  // lets read in each single data element of our
  // data structure to maintain the correct endianess of the
  // data
  stream >> m_pData->header.Data_Type;                      //   0: Data_Type
  stream >> m_pData->header.Num_Dimensions;                //   2: Num_Dimensions
  stream >> m_pData->header.Num_R_Elements;                //   4: Num_R_Elements
  stream >> m_pData->header.Num_Angles;                    //   6: Num_Angles
  stream >> m_pData->header.Corrections_Applied;            //   8: Corrections_Applied
  for(int i=0; i < 64; i++)
    stream >> m_pData->header.Num_Z_Elements[i];            //  10: Num_Z_Elements
  stream >> m_pData->header.Ring_Difference;                // 138: Ring_Difference
  stream >> m_pData->header.Storage_Order;                  // 140: Storage_Order
  stream >> m_pData->header.Axial_Compression;              // 142: Axial_Compression
  stream >> m_pData->header.X_Resolution;                  // 144: X_Resolution
  stream >> m_pData->header.V_Resolution;                  // 148: V_Resolution
  stream >> m_pData->header.Z_Resolution;                  // 152: Z_Resolution
  stream >> m_pData->header.W_Resolution;                  // 156: W_Resolution
  for(int i=0; i < 6; i++)
    stream >> m_pData->header.Gate_Reserved[i];            // 160: Gate_Reserved
  stream >> m_pData->header.Gate_Duration;                  // 172: Gate_Duration
  stream >> m_pData->header.R_Wave_Offset;                  // 176: R_Wave_Offset
  stream >> m_pData->header.Num_Accepted_Beats;            // 180: Num_Accepted_Beats
  stream >> m_pData->header.Scale_Factor;                  // 184: Scale_Factor
  stream >> m_pData->header.Scan_Min;                      // 188: Scan_Min
  stream >> m_pData->header.Scan_Max;                      // 190: Scan_Max
  stream >> m_pData->header.Prompts;                        // 192: Prompts
  stream >> m_pData->header.Delayed;                        // 196: Delayed
  stream >> m_pData->header.Multiples;                      // 200: Multiples
  stream >> m_pData->header.Net_Trues;                      // 204: Net_Trues
  stream >> m_pData->header.Tot_Avg_Cor;                    // 208: Tot_Avg_Cor
  stream >> m_pData->header.Tot_Avg_Uncor;                  // 212: Tot_Avg_Uncor
  stream >> m_pData->header.Total_Coin_Rate;                // 216: Total_Coin_Rate
  stream >> m_pData->header.Frame_Start_Time;              // 220: Frame_Start_Time
  stream >> m_pData->header.Frame_Duration;                // 224: Frame_Duration
  stream >> m_pData->header.Deadtime_Correction_Factor;    // 228: Deadtime_Correction_Factor
  for(int i=0; i < 90; i++)
    stream >> m_pData->header.CTI_reserved[i];              // 232: CTI_reserved
  for(int i=0; i < 50; i++)
    stream >> m_pData->header.User_Reserved[i];            // 412: User_Reserved  
  for(int i=0; i < 128; i++)
    stream >> m_pData->header.Uncor_Singles[i];            // 512: Uncor_Singles
    
  // some more debug output
#if defined(DEBUG)
  D("ECAT7 Scan3D SubHeader loaded:");
  D("-----------------------------");
  D("Data_Type                 : %d",        m_pData->header.Data_Type);
  D("Num_Dimensions            : %d",        m_pData->header.Num_Dimensions);
  D("Num_R_Elements            : %d",        m_pData->header.Num_R_Elements);
  D("Num_Angles                : %d",        m_pData->header.Num_Angles);
  D("Corrections_Applied       : %d",        m_pData->header.Corrections_Applied);
  for(int i=0; i < 64; i++)
  {
    D("Num_Z_Elements        [%02d]: %d", i+1, m_pData->header.Num_Z_Elements[i]);
  }
  D("Storage_Order             : %d",        m_pData->header.Storage_Order);
  D("Axial_Compression         : %d",        m_pData->header.Axial_Compression);
  D("X_Resolution              : %f cm",    m_pData->header.X_Resolution);
  D("V_Resolution              : %f rad",    m_pData->header.V_Resolution);
  D("Z_Resolution              : %f cm",    m_pData->header.Z_Resolution);
  D("W_Resolution              : %f",        m_pData->header.W_Resolution);
  for(int i=0; i < 6; i++)
  {
    D("Gate_Reserved          [%d]: %d", i+1, m_pData->header.Gate_Reserved[i]);
  }
  D("Gate_Duration             : %d msec",  m_pData->header.Gate_Duration);
  D("R_Wave_Offset             : %d",        m_pData->header.R_Wave_Offset);
  D("Num_Accepted_Beats        : %d",        m_pData->header.Num_Accepted_Beats);
  D("Scale_Factor              : %f",        m_pData->header.Scale_Factor);
  D("Scan_Min                  : %d",        m_pData->header.Scan_Min);
  D("Scan_Max                  : %d",        m_pData->header.Scan_Max);
  D("Prompts                   : %d",        m_pData->header.Prompts);
  D("Delayed                   : %d",        m_pData->header.Delayed);
  D("Multiples                 : %d",        m_pData->header.Multiples);
  D("Net_Trues                 : %d",        m_pData->header.Net_Trues);
  D("Tot_Avg_Cor               : %f",        m_pData->header.Tot_Avg_Cor);
  D("Tot_Avg_Uncor             : %f",        m_pData->header.Tot_Avg_Uncor);
  D("Total_Coin_Rate           : %d",        m_pData->header.Total_Coin_Rate);
  D("Frame_Start_Time          : %d msec",  m_pData->header.Frame_Start_Time);
  D("Frame_Duration            : %d msec",  m_pData->header.Frame_Duration);
  D("Deadtime_Correction_Factor: %f",        m_pData->header.Deadtime_Correction_Factor);
  for(int i=0; i < 8; i++)
  {
    D("Uncor_Singles         [%03d]: %f", i+1, m_pData->header.Uncor_Singles[i]);
  }
#endif

  RETURN(true);
  return true;
}

bool CECAT7SubHeaderScan3D::save(void) const
{
  ENTER();

  // check if this stream is writeable or not
  if(m_pMedIOData == NULL || m_pMedIOData->isWritable() == false ||
     m_pDirItem == NULL || m_pDirItem->dataBlock_Start() == 0 ||
     m_pMedIOData->seek(m_pDirItem->dataBlock_Start()) == false)
  {
    RETURN(false);
    return false;
  }

  SHOWVALUE(m_pMedIOData->pos());

  // we write to a buffer first and write out later directly to the file
  QByteArray buffer(rawDataSize(), 0);
  QDataStream stream(&buffer, QIODevice::WriteOnly);
  
  // lets write out each single data element of our
  // data structure to maintain the correct endianess of the
  // data
  stream << m_pData->header.Data_Type;                      //   0: Data_Type
  stream << m_pData->header.Num_Dimensions;                //   2: Num_Dimensions
  stream << m_pData->header.Num_R_Elements;                //   4: Num_R_Elements
  stream << m_pData->header.Num_Angles;                    //   6: Num_Angles
  stream << m_pData->header.Corrections_Applied;            //   8: Corrections_Applied
  for(int i=0; i < 64; i++)
    stream << m_pData->header.Num_Z_Elements[i];            //  10: Num_Z_Elements
  stream << m_pData->header.Ring_Difference;                // 138: Ring_Difference
  stream << m_pData->header.Storage_Order;                  // 140: Storage_Order
  stream << m_pData->header.Axial_Compression;              // 142: Axial_Compression
  stream << m_pData->header.X_Resolution;                  // 144: X_Resolution
  stream << m_pData->header.V_Resolution;                  // 148: V_Resolution
  stream << m_pData->header.Z_Resolution;                  // 152: Z_Resolution
  stream << m_pData->header.W_Resolution;                  // 156: W_Resolution
  for(int i=0; i < 6; i++)
    stream << m_pData->header.Gate_Reserved[i];            // 160: Gate_Reserved
  stream << m_pData->header.Gate_Duration;                  // 172: Gate_Duration
  stream << m_pData->header.R_Wave_Offset;                  // 176: R_Wave_Offset
  stream << m_pData->header.Num_Accepted_Beats;            // 180: Num_Accepted_Beats
  stream << m_pData->header.Scale_Factor;                  // 184: Scale_Factor
  stream << m_pData->header.Scan_Min;                      // 188: Scan_Min
  stream << m_pData->header.Scan_Max;                      // 190: Scan_Max
  stream << m_pData->header.Prompts;                        // 192: Prompts
  stream << m_pData->header.Delayed;                        // 196: Delayed
  stream << m_pData->header.Multiples;                      // 200: Multiples
  stream << m_pData->header.Net_Trues;                      // 204: Net_Trues
  stream << m_pData->header.Tot_Avg_Cor;                    // 208: Tot_Avg_Cor
  stream << m_pData->header.Tot_Avg_Uncor;                  // 212: Tot_Avg_Uncor
  stream << m_pData->header.Total_Coin_Rate;                // 216: Total_Coin_Rate
  stream << m_pData->header.Frame_Start_Time;              // 220: Frame_Start_Time
  stream << m_pData->header.Frame_Duration;                // 224: Frame_Duration
  stream << m_pData->header.Deadtime_Correction_Factor;    // 228: Deadtime_Correction_Factor
  for(int i=0; i < 90; i++)
    stream << m_pData->header.CTI_reserved[i];              // 232: CTI_reserved
  for(int i=0; i < 50; i++)
    stream << m_pData->header.User_Reserved[i];            // 412: User_Reserved  
  for(int i=0; i < 128; i++)
    stream << m_pData->header.Uncor_Singles[i];            // 512: Uncor_Singles
  
  // now write out to our outStream
  bool result = false;
  if(m_pMedIOData->write(buffer) != -1)
  {
    m_pDirItem->subHeaderWritten(*this);
    result = true;
  }

  RETURN(result);
  return result;
}

int CECAT7SubHeaderScan3D::rawDataSize() const
{
  return 2*ECAT_BLOCKSIZE;
}

CECATSubHeader::Type CECAT7SubHeaderScan3D::subHeaderType(void) const
{
  return CECATSubHeader::ECAT7_Scan3D;
}

bool CECAT7SubHeaderScan3D::convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader*) 
{
  ENTER();

  bool bResult = false;
  // depending on the MedIOHeader format we do have to 
  // distinguish between our copy operations.
  switch(pHead1->headerFormat())
  {
    case CMedIOHeader::ECATSubHeader:
    {
      const CECATSubHeader* eSubHeader = static_cast<const CECATSubHeader*>(pHead1);

      // depending on the source type we have to copy either every data or just 
      // some data of the src header
      switch(eSubHeader->subHeaderType())
      {
        // if the source header is also an ECAT7 one we can copy it in whole
        // via a simple memcpy()
        case CECATSubHeader::ECAT7_Scan3D:
        {
          // we use the assignment operator which will do the convertation
          // for us.
          *this = *static_cast<const CECAT7SubHeaderScan3D*>(pHead1);

          bResult = true;
        }
        break;

        case CECATSubHeader::Unknown:
          // for an unknown header type we do nothing
        break;
        
        #warning "non Scan3D copy not complete"
      }
    }

    case CMedIOHeader::ECATMainHeader:
    case CMedIOHeader::ConcordeMicroPetMainHeader:
      // copying a main header into a sub header doesn't make much sense, so we
      // do nothing here
    break;

    case CMedIOHeader::ConcordeMicroPetFrameHeader:
    {
      #warning "Concorde->ECAT7SubHeader copy missing"
    }
    break;

    case CMedIOHeader::Unknown:
      // for an unknown header type we do nothing
    break;
  }

  RETURN(bResult);
  return bResult;
}

CMedIOHeader* CECAT7SubHeaderScan3D::clone() const
{
  ENTER();

  CECAT7SubHeaderScan3D* pNewHead = new CECAT7SubHeaderScan3D();
  pNewHead->convertFrom(this);

  RETURN(pNewHead);
  return pNewHead;
}

// methods to access elements of the SubHeader
CECATSubHeader::Data_Type CECAT7SubHeaderScan3D::data_Type(void)  const
{
  return static_cast<CECATSubHeader::Data_Type>(m_pData->header.Data_Type);
}

short CECAT7SubHeaderScan3D::num_Dimensions(void) const            
{
  return m_pData->header.Num_Dimensions;
}

short CECAT7SubHeaderScan3D::num_R_Elements(void) const            
{
  return m_pData->header.Num_R_Elements;
}

short CECAT7SubHeaderScan3D::num_Angles(void) const                
{
  return m_pData->header.Num_Angles;
}

short CECAT7SubHeaderScan3D::corrections_Applied(void) const        
{
  return m_pData->header.Corrections_Applied;
}

short CECAT7SubHeaderScan3D::num_Z_Elements(const short i) const    
{
  return m_pData->header.Num_Z_Elements[i];
}

short CECAT7SubHeaderScan3D::ring_Difference(void) const          
{  
  return m_pData->header.Ring_Difference;
}

CECAT7SubHeaderScan3D::Storage_Order CECAT7SubHeaderScan3D::storage_Order(void) const      
{
  return static_cast<CECAT7SubHeaderScan3D::Storage_Order>(m_pData->header.Storage_Order);
}

short CECAT7SubHeaderScan3D::axial_Compression(void) const        
{  
  return m_pData->header.Axial_Compression;
}

float CECAT7SubHeaderScan3D::X_Resolution(void) const            
{
  return m_pData->header.X_Resolution;
}

float CECAT7SubHeaderScan3D::V_Resolution(void) const            
{  
  return m_pData->header.V_Resolution;
}

float CECAT7SubHeaderScan3D::Z_Resolution(void) const            
{  
  return m_pData->header.Z_Resolution;  
}

float CECAT7SubHeaderScan3D::W_Resolution(void) const              
{  
  return m_pData->header.W_Resolution;  
}

short CECAT7SubHeaderScan3D::gate_Reserved(const short i) const    
{
  return m_pData->header.Gate_Reserved[i];
}

unsigned int CECAT7SubHeaderScan3D::gate_Duration(void) const    
{
  return m_pData->header.Gate_Duration;
}

unsigned int CECAT7SubHeaderScan3D::r_Wave_Offset(void) const    
{
  return m_pData->header.R_Wave_Offset;
}

unsigned int CECAT7SubHeaderScan3D::num_Accepted_Beats(void) const  
{  
  return m_pData->header.Num_Accepted_Beats;
}

float CECAT7SubHeaderScan3D::scale_Factor(void) const            
{
  return m_pData->header.Scale_Factor;
}

short CECAT7SubHeaderScan3D::scan_Min(void) const                
{
  return m_pData->header.Scan_Min;
}

short CECAT7SubHeaderScan3D::scan_Max(void) const                  
{
  return m_pData->header.Scan_Max;
}

unsigned int CECAT7SubHeaderScan3D::prompts(void) const            
{
  return m_pData->header.Prompts;
}

unsigned int CECAT7SubHeaderScan3D::delayed(void) const          
{
  return m_pData->header.Delayed;
}

unsigned int CECAT7SubHeaderScan3D::multiples(void) const        
{
  return m_pData->header.Multiples;
}

unsigned int CECAT7SubHeaderScan3D::net_Trues(void) const        
{
  return m_pData->header.Net_Trues;
}

float CECAT7SubHeaderScan3D::tot_Avg_Cor(void) const               
{
  return m_pData->header.Tot_Avg_Cor;
}

float CECAT7SubHeaderScan3D::tot_Avg_Uncor(void) const            
{
  return m_pData->header.Tot_Avg_Uncor;
}

unsigned int CECAT7SubHeaderScan3D::total_Coin_Rate(void) const  
{
  return m_pData->header.Total_Coin_Rate;
}

unsigned int CECAT7SubHeaderScan3D::frame_Start_Time(void) const  
{
  return m_pData->header.Frame_Start_Time;
}

unsigned int CECAT7SubHeaderScan3D::frame_Duration(void) const    
{
  return m_pData->header.Frame_Duration;
}

float CECAT7SubHeaderScan3D::deadtime_Correction_Factor(void) const
{
  return m_pData->header.Deadtime_Correction_Factor;  }

short CECAT7SubHeaderScan3D::cti_Reserved(const short i) const        
{
  return m_pData->header.CTI_reserved[i];
}

short CECAT7SubHeaderScan3D::user_Reserved(const short i) const    
{
  return m_pData->header.User_Reserved[i];
}

float CECAT7SubHeaderScan3D::uncor_Singles(const short bucket) const
{  
  return m_pData->header.Uncor_Singles[bucket];
}


// methods to modify elements of the SubHeader
void CECAT7SubHeaderScan3D::setData_Type(const CECATSubHeader::Data_Type dType)  
{
  m_pData->header.Data_Type = static_cast<quint16>(dType);
}

void CECAT7SubHeaderScan3D::setNum_Dimensions(const short dim)            
{
  m_pData->header.Num_Dimensions = dim;
}

void CECAT7SubHeaderScan3D::setNum_R_Elements(const short elem)            
{
  m_pData->header.Num_R_Elements = elem;
}

void CECAT7SubHeaderScan3D::setNum_Angles(const short angles)              
{
  m_pData->header.Num_Angles = angles;
}

void CECAT7SubHeaderScan3D::setCorrections_Applied(const short cor)      
{
  m_pData->header.Corrections_Applied = cor;
}

void CECAT7SubHeaderScan3D::setNum_Z_Elements(const short i, const short num)  
{  m_pData->header.Num_Z_Elements[i] = num;
}

void CECAT7SubHeaderScan3D::setRing_Difference(const short diff)          
{  m_pData->header.Ring_Difference = diff;
}

void CECAT7SubHeaderScan3D::setStorage_Order(const Storage_Order order)  
{
  m_pData->header.Storage_Order = static_cast<quint16>(order);
}

void CECAT7SubHeaderScan3D::setAxial_Compression(const short comp)      
{  m_pData->header.Axial_Compression = comp;
}

void CECAT7SubHeaderScan3D::setX_Resolution(const float res)            
{
  m_pData->header.X_Resolution = res;
}

void CECAT7SubHeaderScan3D::setV_Resolution(const float res)            
{
  m_pData->header.V_Resolution = res;
}

void CECAT7SubHeaderScan3D::setZ_Resolution(const float res)            
{
  m_pData->header.Z_Resolution = res;
}

void CECAT7SubHeaderScan3D::setW_Resolution(const float res)            
{
  m_pData->header.W_Resolution = res;
}

void CECAT7SubHeaderScan3D::setGate_Reserved(const short i, const short val)  
{
  m_pData->header.Gate_Reserved[i] = val;
}

void CECAT7SubHeaderScan3D::setGate_Duration(const unsigned int dur)    
{  m_pData->header.Gate_Duration = dur;  }

void CECAT7SubHeaderScan3D::setR_Wave_Offset(const unsigned int off)    
{
  m_pData->header.R_Wave_Offset = off;  }

void CECAT7SubHeaderScan3D::setNum_Accepted_Beats(const unsigned int b)    
{
  m_pData->header.Num_Accepted_Beats = b;
}

void CECAT7SubHeaderScan3D::setScale_Factor(const float factor)          
{
  m_pData->header.Scale_Factor = factor;
}

void CECAT7SubHeaderScan3D::setScan_Min(const short min)                
{
  m_pData->header.Scan_Min = min;
}

void CECAT7SubHeaderScan3D::setScan_Max(const short max)                
{
  m_pData->header.Scan_Max = max;
}

void CECAT7SubHeaderScan3D::setPrompts(const unsigned int n)            
{
  m_pData->header.Prompts = n;
}

void CECAT7SubHeaderScan3D::setDelayed(const unsigned int n)            
{
  m_pData->header.Delayed = n;  }

void CECAT7SubHeaderScan3D::setMultiples(const unsigned int n)          
{
  m_pData->header.Multiples = n;  }

void CECAT7SubHeaderScan3D::setNet_Trues(const unsigned int n)          
{
  m_pData->header.Net_Trues = n;
}

void CECAT7SubHeaderScan3D::setTot_Avg_Cor(const float value)            
{
  m_pData->header.Tot_Avg_Cor = value;
}

void CECAT7SubHeaderScan3D::setTot_Avg_Uncor(const float value)          
{
  m_pData->header.Tot_Avg_Uncor = value;
}

void CECAT7SubHeaderScan3D::setTotal_Coin_Rate(const unsigned int n)    
{
  m_pData->header.Total_Coin_Rate = n;
}

void CECAT7SubHeaderScan3D::setFrame_Start_Time(const unsigned int t)    
{
  m_pData->header.Frame_Start_Time = t;
}

void CECAT7SubHeaderScan3D::setFrame_Duration(const unsigned int t)      
{
  m_pData->header.Frame_Duration = t;
}

void CECAT7SubHeaderScan3D::setDeadtime_Correction_Factor(const float f)
{
  m_pData->header.Deadtime_Correction_Factor = f;
}

void CECAT7SubHeaderScan3D::setCTI_Reserved(const short i, const short value)    
{
  m_pData->header.CTI_reserved[i] = value;
}

void CECAT7SubHeaderScan3D::setUser_Reserved(const short i, const short value)
{
  m_pData->header.User_Reserved[i] = value;
}

void CECAT7SubHeaderScan3D::setUncor_Singles(const short b, const float value)
{ 
  m_pData->header.Uncor_Singles[b] = value;
}

