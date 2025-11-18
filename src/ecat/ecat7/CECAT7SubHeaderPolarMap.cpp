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

#include "CECAT7SubHeaderPolarMap.h"
#include "CECATDirectoryItem.h"
#include "CECATFile.h"

#include <rtdebug.h>

#include "bswap.h"

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CECAT7SubHeaderPolarMapPrivate
{
  public:
    #define SUBHEADER_SIZE 512
    #pragma pack(1)
    struct HeaderData
    {
      quint16  Data_Type;              //   0: Data_Type
      quint16  Polar_Map_Type;         //   2: Polar_Map_Type
      quint16  Num_Rings;              //   4: Num_Rings
      quint16  Sectors_Per_Ring[32];   //   6: Sectors_Per_Ring (32)                              
      float    Ring_Position[32];      //  70: Ring_Position
      quint16  Ring_Angle[32];         // 198: Ring_Angle                              
      quint16  Start_Angle;            // 262: Start_Angle
      quint16  Long_Axis_Left[3];      // 264: Long_Axis_Left                             
      quint16  Long_Axis_Right[3];     // 270: Long_Axis_Right
      quint16  Position_Data;          // 276: Position_Data
      quint16  Image_Min;              // 278: Image_Min                             
      quint16  Image_Max;              // 280: Image_Max
      float    Scale_Factor;           // 282: Scale_Factor                              
      float    Pixel_Size;             // 286: Pixel_Size
      quint32  Frame_Duration;         // 290: Frame_Duration 
      quint32  Frame_Start_Time;       // 294: Frame_Start_Time 
      quint16  Processing_Code;        // 298: Processing_Code
      quint16  Quant_Units;            // 300: Quant_Units
      char     Annotation[40];         // 302: Annotation
      quint32  Gate_Duration;          // 342: Gate_Duration
      quint32  R_Wave_Offset;          // 346: R_Wave_Offset
      quint32  Num_Accepted_Beats;     // 350: Num_Accepted_Beats 
      char     Polar_Map_Protocol[20]; // 354: Polar_Map_Protocol 
      char     Database_Name[30];      // 374: Database_Name
      quint16  CTI_reserved[27];       // 404: CTI_reserved 
      quint16  User_Reserved[27];      // 464: User_Reserved 
    } header;                         
    #pragma pack()
};                                    
                                     
CECAT7SubHeaderPolarMap::CECAT7SubHeaderPolarMap(CECATFile* ecatFile,
                                                 CECATDirectoryItem* pDirItem)
  : CECATSubHeader(ecatFile, pDirItem)
{                                             
  ENTER();

  // allocate data from our private instance class
  m_pData = new CECAT7SubHeaderPolarMapPrivate();
  
  clear();

  LEAVE();
}

CECAT7SubHeaderPolarMap::~CECAT7SubHeaderPolarMap()
{
  ENTER();

  delete m_pData;

  LEAVE();
}

CECAT7SubHeaderPolarMap::CECAT7SubHeaderPolarMap(const CECAT7SubHeaderPolarMap& src)
  : CECATSubHeader(src)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CECAT7SubHeaderPolarMapPrivate(*(src.m_pData));

  LEAVE();
}

CECAT7SubHeaderPolarMap& CECAT7SubHeaderPolarMap::operator=(const CECAT7SubHeaderPolarMap& src)
{
  ENTER();

  if(m_pData != src.m_pData)
  {
    memcpy(&m_pData->header, 
           &src.m_pData->header, 
           sizeof(struct CECAT7SubHeaderPolarMapPrivate::HeaderData));
  }

  LEAVE();
  return *this;
}

void CECAT7SubHeaderPolarMap::clear()
{
  ENTER();

  // then clear the structure
  memset(&m_pData->header, 0, sizeof(struct CECAT7SubHeaderPolarMapPrivate::HeaderData));      

  setData_Type(CECATSubHeader::UnknownDataType);

  LEAVE();
}

bool CECAT7SubHeaderPolarMap::load(void)
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
    BSWAP_16(m_pData->header.Polar_Map_Type);        
    BSWAP_16(m_pData->header.Num_Rings);             
    for(int i=0; i < 32; i++)
    {
      BSWAP_16(m_pData->header.Sectors_Per_Ring[i]);
      BSWAP_FLT(m_pData->header.Ring_Position[i]);
      BSWAP_16(m_pData->header.Ring_Angle[i]);
    }
    BSWAP_16(m_pData->header.Start_Angle);           
    for(int i=0; i < 3; i++)
    {
      BSWAP_16(m_pData->header.Long_Axis_Left[i]); 
      BSWAP_16(m_pData->header.Long_Axis_Right[i]);
    }
    BSWAP_16(m_pData->header.Position_Data);         
    BSWAP_16(m_pData->header.Image_Min);             
    BSWAP_16(m_pData->header.Image_Max);             
    BSWAP_FLT(m_pData->header.Scale_Factor);          
    BSWAP_FLT(m_pData->header.Pixel_Size);            
    BSWAP_32(m_pData->header.Frame_Duration);        
    BSWAP_32(m_pData->header.Frame_Start_Time);      
    BSWAP_16(m_pData->header.Processing_Code);       
    BSWAP_16(m_pData->header.Quant_Units);           
    BSWAP_32(m_pData->header.Gate_Duration);         
    BSWAP_32(m_pData->header.R_Wave_Offset);         
    BSWAP_32(m_pData->header.Num_Accepted_Beats);    
    for(int i=0; i < 27; i++)
    {
      BSWAP_16(m_pData->header.CTI_reserved[i]);
      BSWAP_16(m_pData->header.User_Reserved[i]);
    }
  }

  // some more debug output
#if defined(DEBUG)
  D("ECAT7 PolarMap SubHeader loaded:");
  D("-------------------------------");
  D("Data_Type                 : %d",        m_pData->header.Data_Type);
  D("Polar_Map_Type            : %d",        m_pData->header.Polar_Map_Type);
  D("Num_Rings                 : %d",        m_pData->header.Num_Rings);
  for(int i=0; i < 32; i++)
  {
    D("Sectors_Per_Ring      [%02d]: %d", i+1, m_pData->header.Sectors_Per_Ring[i]);
  }
  for(int i=0; i < 32; i++)
  {
    D("Ring_Position         [%02d]: %f", i+1, m_pData->header.Ring_Position[i]);
  }
  for(int i=0; i < 32; i++)
  {
    D("Ring_Angle            [%02d]: %d", i+1, m_pData->header.Ring_Angle[i]);
  }
  D("Start_Angle               : %d",        m_pData->header.Start_Angle);
  for(int i=0; i < 3; i++)
  {
    D("Long_Axis_Left         [%d]: %d", i+1, m_pData->header.Long_Axis_Left[i]);
  }
  for(int i=0; i < 3; i++)
  {
    D("Long_Axis_Right        [%d]: %d", i+1, m_pData->header.Long_Axis_Right[i]);
  }
  D("Position_Data             : %d",        m_pData->header.Position_Data);
  D("Image_Min                 : %d",        m_pData->header.Image_Min);
  D("Image_Max                 : %d",        m_pData->header.Image_Max);
  D("Scale_Factor              : %f",        m_pData->header.Scale_Factor);
  D("Pixel_Size                : %f",        m_pData->header.Pixel_Size);
  D("Frame_Duration            : %d",        m_pData->header.Frame_Duration);
  D("Frame_Start_Time          : %d",        m_pData->header.Frame_Start_Time);
  D("Processing_Code           : %d",        m_pData->header.Processing_Code);
  D("Quant_Units               : %d",        m_pData->header.Quant_Units);
  D("Annotation                : %s",        m_pData->header.Annotation);
  D("Gate_Duration             : %d msec",  m_pData->header.Gate_Duration);
  D("R_Wave_Offset             : %d msec",  m_pData->header.R_Wave_Offset);
  D("Num_Accepted_Beats        : %d",        m_pData->header.Num_Accepted_Beats);
  D("Polar_Map_Protocol        : %s",        m_pData->header.Polar_Map_Protocol);
  D("Database_Name             : %s",        m_pData->header.Database_Name);
#endif

  RETURN(true);
  return true;
}

bool CECAT7SubHeaderPolarMap::save(void) const
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
  struct CECAT7SubHeaderPolarMapPrivate::HeaderData* header = NULL;
  if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
  {
    header = new CECAT7SubHeaderPolarMapPrivate::HeaderData;

    // copy the current m_pData->header to beHeader
    memcpy(header, &m_pData->header, sizeof(m_pData->header));

    // we only swap non-char elements of the header
    BSWAP_16(header->Data_Type);             
    BSWAP_16(header->Polar_Map_Type);        
    BSWAP_16(header->Num_Rings);             
    for(int i=0; i < 32; i++)
    {
      BSWAP_16(header->Sectors_Per_Ring[i]);
      BSWAP_FLT(header->Ring_Position[i]);
      BSWAP_16(header->Ring_Angle[i]);
    }
    BSWAP_16(header->Start_Angle);           
    for(int i=0; i < 3; i++)
    {
      BSWAP_16(header->Long_Axis_Left[i]); 
      BSWAP_16(header->Long_Axis_Right[i]);
    }
    BSWAP_16(header->Position_Data);         
    BSWAP_16(header->Image_Min);             
    BSWAP_16(header->Image_Max);             
    BSWAP_FLT(header->Scale_Factor);          
    BSWAP_FLT(header->Pixel_Size);            
    BSWAP_32(header->Frame_Duration);        
    BSWAP_32(header->Frame_Start_Time);      
    BSWAP_16(header->Processing_Code);       
    BSWAP_16(header->Quant_Units);           
    BSWAP_32(header->Gate_Duration);         
    BSWAP_32(header->R_Wave_Offset);         
    BSWAP_32(header->Num_Accepted_Beats);    
    for(int i=0; i < 27; i++)
    {
      BSWAP_16(header->CTI_reserved[i]);
      BSWAP_16(header->User_Reserved[i]);
    }
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

int CECAT7SubHeaderPolarMap::rawDataSize() const
{
  return 1*ECAT_BLOCKSIZE;
}

CECATSubHeader::Type CECAT7SubHeaderPolarMap::subHeaderType(void) const
{
  return CECATSubHeader::ECAT7_PolarMap;
}

bool CECAT7SubHeaderPolarMap::convertFrom(const CMedIOHeader* subHeader, const CMedIOHeader*)
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
        case CECATSubHeader::ECAT7_PolarMap:
        {
          // we use the assignment operator which will do the convertation
          // for us.
          *this = *static_cast<const CECAT7SubHeaderPolarMap*>(subHeader);

          bResult = true;
        }
        break;

        case CECATSubHeader::ECAT7_AttenCorr:
        case CECATSubHeader::ECAT7_Image:
        case CECATSubHeader::ECAT7_Norm:
        case CECATSubHeader::ECAT7_Norm3D:
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

CMedIOHeader* CECAT7SubHeaderPolarMap::clone() const
{
  ENTER();

  CECAT7SubHeaderPolarMap* pNewHead = new CECAT7SubHeaderPolarMap();
  pNewHead->convertFrom(this);

  RETURN(pNewHead);
  return pNewHead;
}

// data access methods
CECATSubHeader::Data_Type CECAT7SubHeaderPolarMap::data_Type(void) const
{
  return static_cast<CECATSubHeader::Data_Type>(m_pData->header.Data_Type);
}

short CECAT7SubHeaderPolarMap::polar_Map_Type(void) const
{
  return m_pData->header.Polar_Map_Type;
}

short CECAT7SubHeaderPolarMap::num_Rings(void) const
{
  return m_pData->header.Num_Rings;
}

short CECAT7SubHeaderPolarMap::sectors_Per_Ring(const short i) const
{
  return m_pData->header.Sectors_Per_Ring[i];
}

float CECAT7SubHeaderPolarMap::ring_Position(const short i) const
{
  return m_pData->header.Ring_Position[i];
}

short CECAT7SubHeaderPolarMap::ring_Angle(const short i) const
{
  return m_pData->header.Ring_Angle[i];
}

short CECAT7SubHeaderPolarMap::start_Angle(void) const
{
  return m_pData->header.Start_Angle;
}

short CECAT7SubHeaderPolarMap::long_Axis_Left(const short i) const
{
  return m_pData->header.Long_Axis_Left[i];
}

short CECAT7SubHeaderPolarMap::long_Axis_Right(const short i) const
{
  return m_pData->header.Long_Axis_Right[i];
}

short CECAT7SubHeaderPolarMap::position_Data(void) const
{
  return m_pData->header.Position_Data;
}

short CECAT7SubHeaderPolarMap::image_Min(void) const
{
  return m_pData->header.Image_Min;
}

short CECAT7SubHeaderPolarMap::image_Max(void) const
{
  return m_pData->header.Image_Max;
}

float CECAT7SubHeaderPolarMap::scale_Factor(void) const
{
  return m_pData->header.Scale_Factor;
}

float CECAT7SubHeaderPolarMap::pixel_Size(void) const
{
  return m_pData->header.Pixel_Size;
}

unsigned int CECAT7SubHeaderPolarMap::frame_Duration(void) const
{
  return m_pData->header.Frame_Duration;
}

unsigned int CECAT7SubHeaderPolarMap::frame_Start_Time(void) const
{
  return m_pData->header.Frame_Start_Time;
}

unsigned int CECAT7SubHeaderPolarMap::processing_Code(void) const
{
  return m_pData->header.Processing_Code;
}

short CECAT7SubHeaderPolarMap::quant_Units(void) const
{
  return m_pData->header.Quant_Units;
}

const char* CECAT7SubHeaderPolarMap::annotation(void) const
{
  return m_pData->header.Annotation;
}

unsigned int CECAT7SubHeaderPolarMap::gate_Duration(void) const
{
  return m_pData->header.Gate_Duration;
}

unsigned int CECAT7SubHeaderPolarMap::r_Wave_Offset(void) const
{
  return m_pData->header.R_Wave_Offset;
}

unsigned int CECAT7SubHeaderPolarMap::num_Accepted_Beats(void) const
{
  return m_pData->header.Num_Accepted_Beats;
}

const char* CECAT7SubHeaderPolarMap::polar_Map_Protocol(void) const
{
  return m_pData->header.Polar_Map_Protocol;
}

const char* CECAT7SubHeaderPolarMap::database_Name(void) const
{
  return m_pData->header.Database_Name;
}

short CECAT7SubHeaderPolarMap::cti_Reserved(const short i) const
{
  return m_pData->header.CTI_reserved[i];
}

short CECAT7SubHeaderPolarMap::user_Reserved(const short i) const
{
  return m_pData->header.User_Reserved[i];
}


void CECAT7SubHeaderPolarMap::setData_Type(const CECATSubHeader::Data_Type dType)
{
  m_pData->header.Data_Type = static_cast<quint16>(dType);
}    

void CECAT7SubHeaderPolarMap::setPolar_Map_Type(const short n)
{
  m_pData->header.Polar_Map_Type = n;
}

void CECAT7SubHeaderPolarMap::setNum_Rings(const short n)
{
  m_pData->header.Num_Rings = n;
}

void CECAT7SubHeaderPolarMap::setSectors_Per_Ring(const short i, const short n)
{
  m_pData->header.Sectors_Per_Ring[i] = n;
}

void CECAT7SubHeaderPolarMap::setRing_Position(const short i, const float n)
{
  m_pData->header.Ring_Position[i] = n;
}

void CECAT7SubHeaderPolarMap::setRing_Angle(const short i, const short n)
{
  m_pData->header.Ring_Angle[i] = n;
}

void CECAT7SubHeaderPolarMap::setStart_Angle(const short n)
{
  m_pData->header.Start_Angle = n;
}

void CECAT7SubHeaderPolarMap::setLong_Axis_Left(const short i, const short n)
{
  m_pData->header.Long_Axis_Left[i] = n;
}

void CECAT7SubHeaderPolarMap::setLong_Axis_Right(const short i, const short n)
{
  m_pData->header.Long_Axis_Right[i] = n;
}

void CECAT7SubHeaderPolarMap::setPosition_Data(const short n)
{
  m_pData->header.Position_Data = n;
}

void CECAT7SubHeaderPolarMap::setImage_Min(const short n)
{
  m_pData->header.Image_Min = n;
}

void CECAT7SubHeaderPolarMap::setImage_Max(const short n)
{
  m_pData->header.Image_Max = n;
}

void CECAT7SubHeaderPolarMap::setScale_Factor(const float n)
{
  m_pData->header.Scale_Factor = n;
}

void CECAT7SubHeaderPolarMap::setPixel_Size(const float n)
{
  m_pData->header.Pixel_Size = n;
}

void CECAT7SubHeaderPolarMap::setFrame_Duration(const unsigned int n)
{
  m_pData->header.Frame_Duration = n;
}

void CECAT7SubHeaderPolarMap::setFrame_Start_Time(const unsigned int n)
{
  m_pData->header.Frame_Start_Time = n;
}

void CECAT7SubHeaderPolarMap::setProcessing_Code(const unsigned int n)
{
  m_pData->header.Processing_Code = n;
}

void CECAT7SubHeaderPolarMap::setQuant_Units(const short n)
{
  m_pData->header.Quant_Units = n;
}

void CECAT7SubHeaderPolarMap::setAnnotation(const char* str)
{
  strncpy(m_pData->header.Annotation, str, sizeof(m_pData->header.Annotation)-1);
}

void CECAT7SubHeaderPolarMap::setGate_Duration(const unsigned int n)
{
  m_pData->header.Gate_Duration = n;
}

void CECAT7SubHeaderPolarMap::setR_Wave_Offset(const unsigned int n)
{
  m_pData->header.R_Wave_Offset = n;
}

void CECAT7SubHeaderPolarMap::setNum_Accepted_Beats(const unsigned int n)
{
  m_pData->header.Num_Accepted_Beats = n;
}

void CECAT7SubHeaderPolarMap::setPolar_Map_Protocol(const char* str)
{
  strncpy(m_pData->header.Polar_Map_Protocol, str, sizeof(m_pData->header.Polar_Map_Protocol)-1);
}

void CECAT7SubHeaderPolarMap::setDatabase_Name(const char* str)
{
  strncpy(m_pData->header.Database_Name, str, sizeof(m_pData->header.Database_Name)-1);
}

void CECAT7SubHeaderPolarMap::setCTI_Reserved(const short i, const short n)
{
  m_pData->header.CTI_reserved[i] = n;
}

void CECAT7SubHeaderPolarMap::setUser_Reserved(const short i, const short n)
{
  m_pData->header.User_Reserved[i] = n;
}

