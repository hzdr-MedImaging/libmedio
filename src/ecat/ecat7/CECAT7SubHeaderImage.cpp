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

#include "CECAT7SubHeaderImage.h"
#include "CECAT7MainHeader.h"
#include "CECATDirectoryItem.h"
#include "CECATFile.h"
#include "CConcordeMainHeader.h"
#include "CConcordeFrameHeader.h"
#include "CPhilipsMainHeader.h"
#include "CPhilipsSubHeaderImage.h"

#include <QtCore/qmath.h>

#include <rtdebug.h>

#include "bswap.h"

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CECAT7SubHeaderImagePrivate
{
  public:
    #define SUBHEADER_SIZE 512
    #pragma pack(1)
    struct HeaderData
    {
      quint16  Data_Type;                  //   0: Data_Type
      quint16  Num_Dimensions;             //   2: Num_Dimensions
      quint16  X_Dimension;                //   4: X_Dimension
      quint16  Y_Dimension;                //   6: Y_Dimension
      quint16  Z_Dimension;                //   8: Z_Dimension
      float    X_Offset;                   //  10: X_Offset
      float    Y_Offset;                   //  14: Y_Offset
      float    Z_Offset;                   //  18: Z_Offset
      float    Recon_Zoom;                 //  22: Recon_Zoom
      float    Scale_Factor;               //  26: Scale_Factor
      quint16  Image_Min;                  //  30: Image_Min
      quint16  Image_Max;                  //  32: Image_Max
      float    X_Pixel_Size;               //  34: X_Pixel_Size
      float    Y_Pixel_Size;               //  38: Y_Pixel_Size
      float    Z_Pixel_Size;               //  42: Z_Pixel_Size
      quint32  Frame_Duration;             //  46: Frame_Duration
      quint32  Frame_Start_Time;           //  50: Frame_Start_Time
      quint16  Filter_Code;                //  54: Filter_Code
      float    X_Resolution;               //  56: X_Resolution
      float    Y_Resolution;               //  60: Y_Resolution
      float    Z_Resolution;               //  64: Z_Resolution
      float    Num_R_Elements;             //  70: Num_R_Elements
      float    Num_Angles;                 //  72: Num_Angles
      float    Z_Rotation_Angle;           //  76: Z_Rotation_Angle
      float    Decay_Corr_Fctr;            //  80: Decay_Corr_Fctr
      quint32  Processing_Code;            //  84: Processing_Code
      quint32  Gate_Duration;              //  88: Gate_Duration
      quint32  R_Wave_Offset;              //  92: R_Wave_Offset
      quint32  Num_Accepted_Beats;         //  96: Num_Accepted_Beats
      float    Filter_Cutoff_Frequency;    // 100: Filter_Cutoff_Frequency
      float    Filter_Resolution;          // 104: Filter_Resolution
      float    Filter_Ramp_Slope;          // 108: Filter_Ramp_Slope
      quint16  Filter_Order;               // 112: Filter_Order
      float    Filter_Scatter_Fraction;    // 116: Filter_Scatter_Fraction
      float    Filter_Scatter_Slope;       // 120: Filter_Scatter_Slope
      char     Annotation[40];             // 122: Annotation
      float    MT_1_1;                     // 162: MT_1_1
      float    MT_1_2;                     // 166: MT_1_2
      float    MT_1_3;                     // 170: MT_1_3
      float    MT_2_1;                     // 174: MT_2_1
      float    MT_2_2;                     // 178: MT_2_2
      float    MT_2_3;                     // 182: MT_2_3
      float    MT_3_1;                     // 186: MT_3_1
      float    MT_3_2;                     // 190: MT_3_2
      float    MT_3_3;                     // 194: MT_3_3
      float    RFilter_Cutoff;             // 198: RFilter_Cutoff
      float    RFilter_Resolution;         // 202: RFilter_Resolution
      quint16  RFilter_Code;               // 206: RFilter_Code
      quint16  RFilter_Order;              // 208: RFilter_Order
      float    ZFilter_Cutoff;             // 210: ZFilter_Cutoff
      float    ZFilter_Resolution;         // 214: ZFilter_Resolution
      quint16  ZFilter_Code;               // 218: ZFilter_Code
      quint16  ZFilter_Order;              // 220: ZFilter_Order
      float    MT_1_4;                     // 222: MT_1_4
      float    MT_2_4;                     // 226: MT_2_4
      float    MT_3_4;                     // 230: MT_3_4
      quint16  Scatter_Type;               // 230: Scatter_Type
      quint16  Recon_Type;                 // 236: Recon_Type
      quint16  Recon_Views;                // 238: Recon_Views
      quint16  CTI_Reserved[87];           // 240: CTI_Reserved
      quint16  User_Reserved[49];          // 414: User_Reserved
    } header;
    #pragma pack()
};

CECAT7SubHeaderImage::CECAT7SubHeaderImage(CECATFile* ecatFile,
                                           CECATDirectoryItem* pDirItem)
  : CECATSubHeader(ecatFile, pDirItem)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CECAT7SubHeaderImagePrivate();
  
  clear();

  LEAVE();
}

CECAT7SubHeaderImage::~CECAT7SubHeaderImage()
{
  ENTER();

  delete m_pData;

  LEAVE();
}

CECAT7SubHeaderImage::CECAT7SubHeaderImage(const CECAT7SubHeaderImage& src)
  : CECATSubHeader(src)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CECAT7SubHeaderImagePrivate(*(src.m_pData));

  LEAVE();
}

CECAT7SubHeaderImage& CECAT7SubHeaderImage::operator=(const CECAT7SubHeaderImage& src)
{
  ENTER();

  if(m_pData != src.m_pData)
  {
    memcpy(&m_pData->header, 
           &src.m_pData->header, 
           sizeof(struct CECAT7SubHeaderImagePrivate::HeaderData));
  }

  LEAVE();
  return *this;
}

void CECAT7SubHeaderImage::clear()
{
  ENTER();

  // clear our header structure first
  memset(&m_pData->header, 0, sizeof(struct CECAT7SubHeaderImagePrivate::HeaderData));
  
  setData_Type(CECATSubHeader::UnknownDataType);
  setFilter_Code(NoFilter);
  setScatter_Type(None);
  setRecon_Type(FilteredBackProjection);

  LEAVE();
}

bool CECAT7SubHeaderImage::load(void)
{
  ENTER();

  // check if the stream is readable or not and
  // set our MedIOData to the correct file position so that we can
  // read the subheader  
  if(m_pMedIOData == NULL ||
     m_pMedIOData->isReadable() == false ||
     m_pDirItem->dataBlock_Start() == 0 ||
     m_pMedIOData->seek(m_pDirItem->dataBlock_Start()) == false)
  {
    RETURN(false);
    return false;
  }

  SHOWVALUE(m_pMedIOData->pos());

  // we read in all data at once using read()
  ASSERT(sizeof(m_pData->header) == SUBHEADER_SIZE);
  if(m_pMedIOData->read(reinterpret_cast<char*>(&m_pData->header), sizeof(m_pData->header)) != SUBHEADER_SIZE)
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
    BSWAP_16(m_pData->header.X_Dimension);             
    BSWAP_16(m_pData->header.Y_Dimension);             
    BSWAP_16(m_pData->header.Z_Dimension);             
    BSWAP_FLT(m_pData->header.X_Offset);                
    BSWAP_FLT(m_pData->header.Y_Offset);                
    BSWAP_FLT(m_pData->header.Z_Offset);                
    BSWAP_FLT(m_pData->header.Recon_Zoom);              
    BSWAP_FLT(m_pData->header.Scale_Factor);            
    BSWAP_16(m_pData->header.Image_Min);               
    BSWAP_16(m_pData->header.Image_Max);               
    BSWAP_FLT(m_pData->header.X_Pixel_Size);            
    BSWAP_FLT(m_pData->header.Y_Pixel_Size);            
    BSWAP_FLT(m_pData->header.Z_Pixel_Size);            
    BSWAP_32(m_pData->header.Frame_Duration);           
    BSWAP_32(m_pData->header.Frame_Start_Time);
    BSWAP_16(m_pData->header.Filter_Code);
    BSWAP_FLT(m_pData->header.X_Resolution);
    BSWAP_FLT(m_pData->header.Y_Resolution);
    BSWAP_FLT(m_pData->header.Z_Resolution);
    BSWAP_FLT(m_pData->header.Num_R_Elements);
    BSWAP_FLT(m_pData->header.Num_Angles);   
    BSWAP_FLT(m_pData->header.Z_Rotation_Angle);
    BSWAP_FLT(m_pData->header.Decay_Corr_Fctr);
    BSWAP_32(m_pData->header.Processing_Code);
    BSWAP_32(m_pData->header.Gate_Duration); 
    BSWAP_32(m_pData->header.R_Wave_Offset);
    BSWAP_32(m_pData->header.Num_Accepted_Beats);
    BSWAP_FLT(m_pData->header.Filter_Cutoff_Frequency);
    BSWAP_FLT(m_pData->header.Filter_Resolution);
    BSWAP_FLT(m_pData->header.Filter_Ramp_Slope);
    BSWAP_16(m_pData->header.Filter_Order);
    BSWAP_FLT(m_pData->header.Filter_Scatter_Fraction);
    BSWAP_FLT(m_pData->header.Filter_Scatter_Slope);
    BSWAP_FLT(m_pData->header.MT_1_1);
    BSWAP_FLT(m_pData->header.MT_1_2);
    BSWAP_FLT(m_pData->header.MT_1_3);
    BSWAP_FLT(m_pData->header.MT_2_1);
    BSWAP_FLT(m_pData->header.MT_2_2);
    BSWAP_FLT(m_pData->header.MT_2_3);
    BSWAP_FLT(m_pData->header.MT_3_1);
    BSWAP_FLT(m_pData->header.MT_3_2);
    BSWAP_FLT(m_pData->header.MT_3_3);
    BSWAP_FLT(m_pData->header.RFilter_Cutoff);
    BSWAP_FLT(m_pData->header.RFilter_Resolution);
    BSWAP_16(m_pData->header.RFilter_Code);
    BSWAP_16(m_pData->header.RFilter_Order);
    BSWAP_FLT(m_pData->header.ZFilter_Cutoff);
    BSWAP_FLT(m_pData->header.ZFilter_Resolution);
    BSWAP_16(m_pData->header.ZFilter_Code);
    BSWAP_16(m_pData->header.ZFilter_Order);
    BSWAP_FLT(m_pData->header.MT_1_4);
    BSWAP_FLT(m_pData->header.MT_2_4);
    BSWAP_FLT(m_pData->header.MT_3_4);
    BSWAP_16(m_pData->header.Scatter_Type);
    BSWAP_16(m_pData->header.Recon_Type);
    BSWAP_16(m_pData->header.Recon_Views);
    for(int i=0; i < 87; i++)
      BSWAP_16(m_pData->header.CTI_Reserved[i]);

    for(int i=0; i < 49; i++)
      BSWAP_16(m_pData->header.User_Reserved[i]);
  }

  // some more debug output
#if defined(DEBUG)
  D("ECAT7 Image SubHeader loaded:");
  D("----------------------------");
  D("Data_Type                 : %d",       m_pData->header.Data_Type);
  D("Num_Dimensions            : %d",       m_pData->header.Num_Dimensions);
  D("X_Dimension               : %d",       m_pData->header.X_Dimension);
  D("Y_Dimension               : %d",       m_pData->header.Y_Dimension);
  D("Z_Dimension               : %d",       m_pData->header.Z_Dimension);
  D("X_Offset                  : %f cm",    m_pData->header.X_Offset);
  D("Y_Offset                  : %f cm",    m_pData->header.Y_Offset);
  D("Z_Offset                  : %f cm",    m_pData->header.Z_Offset);
  D("Recon_Zoom                : %fx",      m_pData->header.Recon_Zoom);
  D("Scale_Factor              : %f",       m_pData->header.Scale_Factor);
  D("Image_Min                 : %d",       m_pData->header.Image_Min);
  D("Image_Max                 : %d",       m_pData->header.Image_Max);
  D("X_Pixel_Size              : %f cm",    m_pData->header.X_Pixel_Size);
  D("Y_Pixel_Size              : %f cm",    m_pData->header.Y_Pixel_Size);
  D("Z_Pixel_Size              : %f cm",    m_pData->header.Z_Pixel_Size);
  D("Frame_Duration            : %d msec",  m_pData->header.Frame_Duration);
  D("Frame_Start_Time          : %d msec",  m_pData->header.Frame_Start_Time);
  D("Filter_Code               : %d",       m_pData->header.Filter_Code);
  D("X_Resolution              : %f cm",    m_pData->header.X_Resolution);
  D("Y_Resolution              : %f cm",    m_pData->header.Y_Resolution);
  D("Z_Resolution              : %f cm",    m_pData->header.Z_Resolution);
  D("Num_R_Elements            : %f",       m_pData->header.Num_R_Elements);
  D("Num_Angles                : %f",       m_pData->header.Num_Angles);
  D("Z_Rotation_Angle          : %f°",      m_pData->header.Z_Rotation_Angle);
  D("Decay_Corr_Fctr           : %f",       m_pData->header.Decay_Corr_Fctr);
  D("Processing_Code           : %d",       m_pData->header.Processing_Code);
  D("Gate_Duration             : %d msec",  m_pData->header.Gate_Duration);
  D("R_Wave_Offset             : %d msec",  m_pData->header.R_Wave_Offset);
  D("Num_Accepted_Beats        : %d",       m_pData->header.Num_Accepted_Beats);
  D("Filter_Cutoff_Frequency   : %f",       m_pData->header.Filter_Cutoff_Frequency);
  D("Filter_Resolution         : %f",       m_pData->header.Filter_Resolution);
  D("Filter_Ramp_Slope         : %f",       m_pData->header.Filter_Ramp_Slope);
  D("Filter_Order              : %d",       m_pData->header.Filter_Order);
  D("Filter_Scatter_Fraction   : %f",       m_pData->header.Filter_Scatter_Fraction);
  D("Filter_Scatter_Slope      : %f",       m_pData->header.Filter_Scatter_Slope);
  D("Annotation                : %s",       m_pData->header.Annotation);
  D("MT_1_1                    : %f",       m_pData->header.MT_1_1);
  D("MT_1_2                    : %f",       m_pData->header.MT_1_2);
  D("MT_1_3                    : %f",       m_pData->header.MT_1_3);
  D("MT_2_1                    : %f",       m_pData->header.MT_2_1);
  D("MT_2_2                    : %f",       m_pData->header.MT_2_2);
  D("MT_2_3                    : %f",       m_pData->header.MT_2_3);
  D("MT_3_1                    : %f",       m_pData->header.MT_3_1);
  D("MT_3_2                    : %f",       m_pData->header.MT_3_2);
  D("MT_3_3                    : %f",       m_pData->header.MT_3_3);
  D("RFilter_Cutoff            : %f",       m_pData->header.RFilter_Cutoff);
  D("RFilter_Resolution        : %f",       m_pData->header.RFilter_Resolution);
  D("RFilter_Code              : %d",       m_pData->header.RFilter_Code);
  D("RFilter_Order             : %d",       m_pData->header.RFilter_Order);
  D("ZFilter_Cutoff            : %f",       m_pData->header.ZFilter_Cutoff);
  D("ZFilter_Resolution        : %f",       m_pData->header.ZFilter_Resolution);
  D("ZFilter_Code              : %d",       m_pData->header.ZFilter_Code);
  D("ZFilter_Order             : %d",       m_pData->header.ZFilter_Order);
  D("MT_1_4                    : %f",       m_pData->header.MT_1_4);
  D("MT_2_4                    : %f",       m_pData->header.MT_2_4);
  D("MT_3_4                    : %f",       m_pData->header.MT_3_4);
  D("Scatter_Type              : %d",       m_pData->header.Scatter_Type);
  D("Recon_Type                : %d",       m_pData->header.Recon_Type);
  D("Recon_Views               : %d",       m_pData->header.Recon_Views);
#endif

  RETURN(true);
  return true;
}

bool CECAT7SubHeaderImage::save(void) const
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

  ASSERT(sizeof(m_pData->header) == SUBHEADER_SIZE);
  struct CECAT7SubHeaderImagePrivate::HeaderData* header = NULL;
  if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
  {
    header = new CECAT7SubHeaderImagePrivate::HeaderData;

    // copy the current m_pData->header to beHeader
    memcpy(header, &m_pData->header, sizeof(m_pData->header));

    // we only swap non-char elements of the header
    BSWAP_16(header->Data_Type);               
    BSWAP_16(header->Num_Dimensions);          
    BSWAP_16(header->X_Dimension);             
    BSWAP_16(header->Y_Dimension);             
    BSWAP_16(header->Z_Dimension);             
    BSWAP_FLT(header->X_Offset);                
    BSWAP_FLT(header->Y_Offset);                
    BSWAP_FLT(header->Z_Offset);                
    BSWAP_FLT(header->Recon_Zoom);              
    BSWAP_FLT(header->Scale_Factor);            
    BSWAP_16(header->Image_Min);               
    BSWAP_16(header->Image_Max);               
    BSWAP_FLT(header->X_Pixel_Size);            
    BSWAP_FLT(header->Y_Pixel_Size);            
    BSWAP_FLT(header->Z_Pixel_Size);            
    BSWAP_32(header->Frame_Duration);           
    BSWAP_32(header->Frame_Start_Time);
    BSWAP_16(header->Filter_Code);
    BSWAP_FLT(header->X_Resolution);
    BSWAP_FLT(header->Y_Resolution);
    BSWAP_FLT(header->Z_Resolution);
    BSWAP_FLT(header->Num_R_Elements);
    BSWAP_FLT(header->Num_Angles);   
    BSWAP_FLT(header->Z_Rotation_Angle);
    BSWAP_FLT(header->Decay_Corr_Fctr);
    BSWAP_32(header->Processing_Code);
    BSWAP_32(header->Gate_Duration); 
    BSWAP_32(header->R_Wave_Offset);
    BSWAP_32(header->Num_Accepted_Beats);
    BSWAP_FLT(header->Filter_Cutoff_Frequency);
    BSWAP_FLT(header->Filter_Resolution);
    BSWAP_FLT(header->Filter_Ramp_Slope);
    BSWAP_16(header->Filter_Order);
    BSWAP_FLT(header->Filter_Scatter_Fraction);
    BSWAP_FLT(header->Filter_Scatter_Slope);
    BSWAP_FLT(header->MT_1_1);
    BSWAP_FLT(header->MT_1_2);
    BSWAP_FLT(header->MT_1_3);
    BSWAP_FLT(header->MT_2_1);
    BSWAP_FLT(header->MT_2_2);
    BSWAP_FLT(header->MT_2_3);
    BSWAP_FLT(header->MT_3_1);
    BSWAP_FLT(header->MT_3_2);
    BSWAP_FLT(header->MT_3_3);
    BSWAP_FLT(header->RFilter_Cutoff);
    BSWAP_FLT(header->RFilter_Resolution);
    BSWAP_16(header->RFilter_Code);
    BSWAP_16(header->RFilter_Order);
    BSWAP_FLT(header->ZFilter_Cutoff);
    BSWAP_FLT(header->ZFilter_Resolution);
    BSWAP_16(header->ZFilter_Code);
    BSWAP_16(header->ZFilter_Order);
    BSWAP_FLT(header->MT_1_4);
    BSWAP_FLT(header->MT_2_4);
    BSWAP_FLT(header->MT_3_4);
    BSWAP_16(header->Scatter_Type);
    BSWAP_16(header->Recon_Type);
    BSWAP_16(header->Recon_Views);
    for(int i=0; i < 87; i++)
      BSWAP_16(header->CTI_Reserved[i]);

    for(int i=0; i < 49; i++)
      BSWAP_16(header->User_Reserved[i]);
  }
  else
    header = &m_pData->header;

  // now write out to our outStream
  bool result = false;
  if(m_pMedIOData->write(reinterpret_cast<char*>(header), sizeof(m_pData->header)) == SUBHEADER_SIZE)
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

int CECAT7SubHeaderImage::rawDataSize() const
{
  return 1*ECAT_BLOCKSIZE;
}

CECATSubHeader::Type CECAT7SubHeaderImage::subHeaderType(void) const
{
  return CECATSubHeader::ECAT7_Image;
}

bool CECAT7SubHeaderImage::convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader* pHead2) 
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
        case CECATSubHeader::ECAT7_Image:
        {
          // we use the assignment operator which will do the convertation
          // for us.
          *this = *static_cast<const CECAT7SubHeaderImage*>(pHead1);
          
          bResult = true;
        }
        break;

        case CECATSubHeader::Unknown:
          // for an unknown header type we do nothing
        break;
        
        #warning "non Image copy not complete"
      }
    }

    case CMedIOHeader::ECATMainHeader:
    case CMedIOHeader::ConcordeMicroPetMainHeader:
      // copying a main header into a sub header doesn't make much sense, so we
      // do nothing here
    break;

    case CMedIOHeader::ConcordeMicroPetFrameHeader:
    {
      const CConcordeFrameHeader* head = static_cast<const CConcordeFrameHeader*>(pHead1);
      // first clean up
      clear();
      // now convert 
      setData_Type(CECATSubHeader::SunShort);
      setNum_Dimensions(3);
      setRecon_Zoom(1.0);
      setScale_Factor(head->scaleFactor());
      setImage_Min(static_cast<short>(head->minimum()));
      setImage_Max(static_cast<short>(head->maximum()));
      setFrame_Duration(static_cast<int>(head->frameDuration()*1000.0));
      setFrame_Start_Time(static_cast<int>(head->frameStart()*1000.0));
      setDecay_Corr_Fctr(head->decayCorrection());
      setGate_Duration(0);
      // check for additional information
      if(pHead2)
      {
        switch(pHead2->headerFormat())
        {
          case CMedIOHeader::ConcordeMicroPetMainHeader:
          {
            D("Setting additional information to ECAT7 sub header");
            const CConcordeMainHeader* mainHeader = static_cast<const CConcordeMainHeader*>(pHead2);
            setX_Dimension(mainHeader->xDimension());
            setY_Dimension(mainHeader->yDimension());
            setZ_Dimension(mainHeader->zDimension());
            setX_Pixel_Size(mainHeader->pixelSize());
            setY_Pixel_Size(mainHeader->pixelSize());
            //since microPET header 1.711 axial_plane_size does not exist
            //anymore -> use axial_crystal_pitch/2: works for older header
            //versions too
            setZ_Pixel_Size(mainHeader->axialCrystalPitch()/2.0F);
            setNum_R_Elements(mainHeader->yDimension());
            setNum_Angles(mainHeader->xDimension());
          };break;
          default:break;
        }
      }
      bResult = true;
    }
    break;

    case CMedIOHeader::PhilipsSubHeader:
    {
      const CPhilipsSubHeaderImage* head = static_cast<const CPhilipsSubHeaderImage*>(pHead1);

      // first clean up
      clear();

      // now convert
      CECATSubHeader::Data_Type dtype = CECATSubHeader::UnknownDataType;
      switch(head->datype())
      {
        case CPhilipsSubHeader::UnknownDataType:
          W("Unknown DataType found while converting");
          // do nothing
        break;

        case CPhilipsSubHeader::ByteData:
          dtype = CECATSubHeader::ByteData;
        break;

        case CPhilipsSubHeader::SignedShort:
        case CPhilipsSubHeader::UnsignedShort:
          dtype = CECATSubHeader::SunShort;
        break;

        case CPhilipsSubHeader::Float:
          dtype = CECATSubHeader::IEEEFloat;
        break;
      }
      setData_Type(dtype);

      setNum_Dimensions(3);
      setX_Dimension(head->xdim());
      setY_Dimension(head->ydim());
      setRecon_Zoom(1.0);

      if(head->suvscl() != 0.0f)
        setScale_Factor(head->suvscl());
      else
        setScale_Factor(head->imgscl());

      setImage_Min(head->imgmin());
      setImage_Max(head->imgmax());
      setX_Pixel_Size(head->pix_spacing_x() / 10.0f); // mm -> cm
      setY_Pixel_Size(head->pix_spacing_y() / 10.0f); // mm -> cm
      setFrame_Duration((head->scnlen() * 1000) + head->mseclen());

      setGate_Duration(0);
      if(QString(head->scatter_corr()).contains("SIMUL"))
        setScatter_Type(CECAT7SubHeaderImage::Simulated);
      else
        setScatter_Type(CECAT7SubHeaderImage::None);
    
      setRecon_Type(CECAT7SubHeaderImage::IterativeCPURecon);

      short processing_code = CECAT7SubHeaderImage::NotProcessed;
      if(head->det_norm() == 1)
        processing_code |= CECAT7SubHeaderImage::Normalized;

      if(QString(head->atten_corr()).contains("AC"))
        processing_code |= CECAT7SubHeaderImage::MeasuredAttenCorr;

      if(QString(head->scatter_corr()).contains("SIMUL"))
        processing_code |= CECAT7SubHeaderImage::Scatter2DCorr;

      if(head->decay_corr() == 1)
        processing_code |= CECAT7SubHeaderImage::DecayCorrection;

      setProcessing_Code(processing_code);
        
      // check for additional information
      if(pHead2)
      {
        switch(pHead2->headerFormat())
        {
          case CMedIOHeader::PhilipsMainHeader:
          {
            const CPhilipsMainHeader* mainHeader = static_cast<const CPhilipsMainHeader*>(pHead2);

            setZ_Dimension(mainHeader->nslice());
            setZ_Pixel_Size(static_cast<float>(mainHeader->slcthk()) / 10.0f); // mm -> cm

            if(head->pix_spacing_x() == 0)
              setX_Pixel_Size(static_cast<float>(mainHeader->dmax()) / static_cast<float>(head->xdim()) / 10.0f); // mm -> cm

            if(head->pix_spacing_y() == 0)
              setY_Pixel_Size(static_cast<float>(mainHeader->dmax()) / static_cast<float>(head->ydim()) / 10.0f); // mm -> cm

            CECAT7SubHeaderImage::Filter_Code fcode = CECAT7SubHeaderImage::NoFilter;
            switch(mainHeader->fltr_type())
            {
              case CPhilipsMainHeader::Undefined_Fltr:
              case CPhilipsMainHeader::No_Fltr:
                // do nothing
              break;

              case CPhilipsMainHeader::Ramp_Fltr:
                fcode = CECAT7SubHeaderImage::Ramp;
              break;

              case CPhilipsMainHeader::Hanning_Fltr:
                fcode = CECAT7SubHeaderImage::Hanning;
              break;

              case CPhilipsMainHeader::Gaussian_Fltr:
                fcode = CECAT7SubHeaderImage::Gaussian;
              break;

              case CPhilipsMainHeader::Butterworth_Fltr:
                fcode = CECAT7SubHeaderImage::Butterworth;
              break;
            }
            setFilter_Code(fcode);

            // set the frame start time relative to the acq_date_time
            setFrame_Start_Time((head->start_date_time() - mainHeader->acq_date_time()) * 1000); // s -> ms

            // R_elements/Angles are usually only relevant for sinograms
            // however, we fill the stuff in for consistency reasons
            if(mainHeader->dmax() == 576)
              setNum_R_Elements(291);
            else if(mainHeader->dmax() == 265)
              setNum_R_Elements(193);

            if(mainHeader->phiMashing() == CPhilipsMainHeader::Mashing2)
              setNum_Angles(336/2);
            else
              setNum_Angles(336);

            if(mainHeader->decay_corr() == 1)
              setDecay_Corr_Fctr(1.0f);
            else
              setDecay_Corr_Fctr(0.0f);
          }
          break;

          default:
            // do nothing
          break;
        }
      }

      bResult = true;
    }
    break;

    case CMedIOHeader::Unknown:
      // for an unknown header type we do nothing
    break;
  }

  RETURN(bResult);
  return bResult;
}

CMedIOHeader* CECAT7SubHeaderImage::clone() const
{
  ENTER();

  CECAT7SubHeaderImage* pNewHead = new CECAT7SubHeaderImage();
  pNewHead->convertFrom(this);
  
  RETURN(pNewHead);
  return pNewHead;
}

// data acess methods
CECATSubHeader::Data_Type CECAT7SubHeaderImage::data_Type(void) const
{
  return static_cast<CECATSubHeader::Data_Type>(m_pData->header.Data_Type);
}

short CECAT7SubHeaderImage::num_Dimensions(void) const
{
  return m_pData->header.Num_Dimensions;
}

short CECAT7SubHeaderImage::x_Dimension(void) const
{
  return m_pData->header.X_Dimension;
}

short CECAT7SubHeaderImage::y_Dimension(void) const
{
  return m_pData->header.Y_Dimension;
}

short CECAT7SubHeaderImage::z_Dimension(void) const
{
  return m_pData->header.Z_Dimension;
}

float CECAT7SubHeaderImage::x_Offset(void) const
{
  return m_pData->header.X_Offset;
}

float CECAT7SubHeaderImage::y_Offset(void) const
{
  return m_pData->header.Y_Offset;
}

float CECAT7SubHeaderImage::z_Offset(void) const
{
  return m_pData->header.Z_Offset;
}

float CECAT7SubHeaderImage::recon_Zoom(void) const
{
  return m_pData->header.Recon_Zoom;
}

float CECAT7SubHeaderImage::scale_Factor(void) const
{
  return m_pData->header.Scale_Factor;
}

short CECAT7SubHeaderImage::image_Min(void) const
{
  return m_pData->header.Image_Min;
}

short CECAT7SubHeaderImage::image_Max(void) const
{
  return m_pData->header.Image_Max;
}

float CECAT7SubHeaderImage::x_Pixel_Size(void) const
{
  return m_pData->header.X_Pixel_Size;
}

float CECAT7SubHeaderImage::y_Pixel_Size(void) const
{
  return m_pData->header.Y_Pixel_Size;
}

float CECAT7SubHeaderImage::z_Pixel_Size(void) const
{
  return m_pData->header.Z_Pixel_Size;
}

unsigned int CECAT7SubHeaderImage::frame_Duration(void) const
{
  return m_pData->header.Frame_Duration;
}

unsigned int CECAT7SubHeaderImage::frame_Start_Time(void) const
{
  return m_pData->header.Frame_Start_Time;
}

CECAT7SubHeaderImage::Filter_Code CECAT7SubHeaderImage::filter_Code(void) const
{
  return static_cast<CECAT7SubHeaderImage::Filter_Code>(m_pData->header.Filter_Code);
}

float CECAT7SubHeaderImage::x_Resolution(void) const
{
  return m_pData->header.X_Resolution;
}

float CECAT7SubHeaderImage::y_Resolution(void) const
{
  return m_pData->header.Y_Resolution;
}

float CECAT7SubHeaderImage::z_Resolution(void) const
{
  return m_pData->header.Z_Resolution;
}

float CECAT7SubHeaderImage::num_R_Elements(void) const
{
  return m_pData->header.Num_R_Elements;
}

float CECAT7SubHeaderImage::num_Angles(void) const
{
  return m_pData->header.Num_Angles;
}

float CECAT7SubHeaderImage::z_Rotation_Angle(void) const
{
  return m_pData->header.Z_Rotation_Angle;
}

float CECAT7SubHeaderImage::decay_Corr_Fctr(void) const
{
  return m_pData->header.Decay_Corr_Fctr;
}

unsigned int CECAT7SubHeaderImage::processing_Code(void) const
{
  return m_pData->header.Processing_Code;
}

unsigned int CECAT7SubHeaderImage::gate_Duration(void) const
{
  return m_pData->header.Gate_Duration;
}

unsigned int CECAT7SubHeaderImage::r_Wave_Offset(void) const
{
  return m_pData->header.R_Wave_Offset;
}

unsigned int CECAT7SubHeaderImage::num_Accepted_Beats(void) const
{
  return m_pData->header.Num_Accepted_Beats;
}

float CECAT7SubHeaderImage::filter_Cutoff_Frequency(void) const
{
  return m_pData->header.Filter_Cutoff_Frequency;
}

float CECAT7SubHeaderImage::filter_Resolution(void) const
{
  return m_pData->header.Filter_Resolution;
}

float CECAT7SubHeaderImage::filter_Ramp_Slope(void) const
{
  return m_pData->header.Filter_Ramp_Slope;
}

short CECAT7SubHeaderImage::filter_Order(void) const
{
  return m_pData->header.Filter_Order;
}

float CECAT7SubHeaderImage::filter_Scatter_Fraction(void) const
{
  return m_pData->header.Filter_Scatter_Fraction;
}

float CECAT7SubHeaderImage::filter_Scatter_Slope(void) const
{
  return m_pData->header.Filter_Scatter_Slope;
}

const char* CECAT7SubHeaderImage::annotation(void) const 
{
  return m_pData->header.Annotation;
}

float CECAT7SubHeaderImage::mt_1_1(void) const
{
  return m_pData->header.MT_1_1;
}

float CECAT7SubHeaderImage::mt_1_2(void) const
{
  return m_pData->header.MT_1_2;
}

float CECAT7SubHeaderImage::mt_1_3(void) const
{
  return m_pData->header.MT_1_3;
}

float CECAT7SubHeaderImage::mt_2_1(void) const
{
  return m_pData->header.MT_2_1;
}

float CECAT7SubHeaderImage::mt_2_2(void) const
{
  return m_pData->header.MT_2_2;
}

float CECAT7SubHeaderImage::mt_2_3(void) const
{
  return m_pData->header.MT_2_3;
}

float CECAT7SubHeaderImage::mt_3_1(void) const
{
  return m_pData->header.MT_3_1;
}

float CECAT7SubHeaderImage::mt_3_2(void) const
{
  return m_pData->header.MT_3_2;
}

float CECAT7SubHeaderImage::mt_3_3(void) const
{
  return m_pData->header.MT_3_3;
}

float CECAT7SubHeaderImage::rfilter_Cutoff(void) const
{
  return m_pData->header.RFilter_Cutoff;
}

float CECAT7SubHeaderImage::rfilter_Resolution(void) const
{
  return m_pData->header.RFilter_Resolution;
}

CECAT7SubHeaderImage::Filter_Code CECAT7SubHeaderImage::rfilter_Code(void) const
{
  return static_cast<CECAT7SubHeaderImage::Filter_Code>(m_pData->header.RFilter_Code);
}

short CECAT7SubHeaderImage::rfilter_Order(void) const
{
  return m_pData->header.RFilter_Order;
}

float CECAT7SubHeaderImage::zfilter_Cutoff(void) const
{
  return m_pData->header.ZFilter_Cutoff;
}

float CECAT7SubHeaderImage::zfilter_Resolution(void) const
{
  return m_pData->header.ZFilter_Resolution;
}

CECAT7SubHeaderImage::Filter_Code CECAT7SubHeaderImage::zfilter_Code(void) const
{
  return static_cast<CECAT7SubHeaderImage::Filter_Code>(m_pData->header.ZFilter_Code);
}

short CECAT7SubHeaderImage::zfilter_Order(void) const
{
  return m_pData->header.ZFilter_Order;
}

float CECAT7SubHeaderImage::mt_1_4(void) const
{
  return m_pData->header.MT_1_4;
}

float CECAT7SubHeaderImage::mt_2_4(void) const
{
  return m_pData->header.MT_2_4;
}

float CECAT7SubHeaderImage::mt_3_4(void) const
{
  return m_pData->header.MT_3_4;
}

CECAT7SubHeaderImage::Scatter_Type CECAT7SubHeaderImage::scatter_Type(void) const
{
  return static_cast<CECAT7SubHeaderImage::Scatter_Type>(m_pData->header.Scatter_Type);
}

CECAT7SubHeaderImage::Recon_Type CECAT7SubHeaderImage::recon_Type(void) const
{
  return static_cast<CECAT7SubHeaderImage::Recon_Type>(m_pData->header.Recon_Type);
}

short CECAT7SubHeaderImage::recon_Views(void) const
{
  return m_pData->header.Recon_Views;
}

short CECAT7SubHeaderImage::cti_Reserved(const short i) const            
{
  return m_pData->header.CTI_Reserved[i];
}

short CECAT7SubHeaderImage::user_Reserved(const short i) const
{
  return m_pData->header.User_Reserved[i];
}


// methods to modify elements of the SubHeader
void CECAT7SubHeaderImage::setData_Type(const CECATSubHeader::Data_Type dType)
{
  m_pData->header.Data_Type = static_cast<quint16>(dType);
}

void CECAT7SubHeaderImage::setNum_Dimensions(const short dim)            
{
  m_pData->header.Num_Dimensions = dim;
}

void CECAT7SubHeaderImage::setX_Dimension(const short dim)            
{
  m_pData->header.X_Dimension = dim;
}

void CECAT7SubHeaderImage::setY_Dimension(const short dim)            
{
  m_pData->header.Y_Dimension = dim;
}

void CECAT7SubHeaderImage::setZ_Dimension(const short dim)            
{
  m_pData->header.Z_Dimension = dim;
}

void CECAT7SubHeaderImage::setX_Offset(const float off)            
{
  m_pData->header.X_Offset = off;
}

void CECAT7SubHeaderImage::setY_Offset(const float off)            
{
  m_pData->header.Y_Offset = off;
}

void CECAT7SubHeaderImage::setZ_Offset(const float off)            
{
  m_pData->header.Z_Offset = off;
}

void CECAT7SubHeaderImage::setRecon_Zoom(const float zoom)            
{
  m_pData->header.Recon_Zoom = zoom;
}

void CECAT7SubHeaderImage::setScale_Factor(const float scale)            
{
  m_pData->header.Scale_Factor = scale;
}

void CECAT7SubHeaderImage::setImage_Min(const short min)            
{
  m_pData->header.Image_Min = min;
}

void CECAT7SubHeaderImage::setImage_Max(const short max)            
{
  m_pData->header.Image_Max = max;
}

void CECAT7SubHeaderImage::setX_Pixel_Size(const float size)            
{
  m_pData->header.X_Pixel_Size = size;
}

void CECAT7SubHeaderImage::setY_Pixel_Size(const float size)            
{
  m_pData->header.Y_Pixel_Size = size;
}

void CECAT7SubHeaderImage::setZ_Pixel_Size(const float size)            
{
  m_pData->header.Z_Pixel_Size = size;
}

void CECAT7SubHeaderImage::setFrame_Duration(const unsigned int dur)    
{
  m_pData->header.Frame_Duration = dur;
}

void CECAT7SubHeaderImage::setFrame_Start_Time(const unsigned int time)    
{
  m_pData->header.Frame_Start_Time = time;
}

void CECAT7SubHeaderImage::setFilter_Code(const Filter_Code code)            
{
  m_pData->header.Filter_Code = static_cast<quint16>(code);
}

void CECAT7SubHeaderImage::setX_Resolution(const float res)            
{
  m_pData->header.X_Resolution = res;
}

void CECAT7SubHeaderImage::setY_Resolution(const float res)            
{
  m_pData->header.Y_Resolution = res;
}

void CECAT7SubHeaderImage::setZ_Resolution(const float res)            
{
  m_pData->header.Z_Resolution = res;
}

void CECAT7SubHeaderImage::setNum_R_Elements(const float elem)            
{
  m_pData->header.Num_R_Elements = elem;
}

void CECAT7SubHeaderImage::setNum_Angles(const float angles)              
{
  m_pData->header.Num_Angles = angles;
}

void CECAT7SubHeaderImage::setZ_Rotation_Angle(const float angle)            
{
  m_pData->header.Z_Rotation_Angle = angle;
}

void CECAT7SubHeaderImage::setDecay_Corr_Fctr(const float factor)              
{
  m_pData->header.Decay_Corr_Fctr = factor;
}

void CECAT7SubHeaderImage::setProcessing_Code(const unsigned int code)    
{
  m_pData->header.Processing_Code = code;
}

void CECAT7SubHeaderImage::setGate_Duration(const unsigned int dur)    
{
  m_pData->header.Gate_Duration = dur;
}

void CECAT7SubHeaderImage::setR_Wave_Offset(const unsigned int off)    
{
  m_pData->header.R_Wave_Offset = off;
}

void CECAT7SubHeaderImage::setNum_Accepted_Beats(const unsigned int beats)    
{
  m_pData->header.Num_Accepted_Beats = beats;
}

void CECAT7SubHeaderImage::setFilter_Cutoff_Frequency(const float freq)            
{
  m_pData->header.Filter_Cutoff_Frequency = freq;
}

void CECAT7SubHeaderImage::setFilter_Resolution(const float res)            
{
  m_pData->header.Filter_Resolution = res;
}

void CECAT7SubHeaderImage::setFilter_Ramp_Slope(const float slope)            
{
  m_pData->header.Filter_Ramp_Slope = slope;
}

void CECAT7SubHeaderImage::setFilter_Order(const short order)            
{
  m_pData->header.Filter_Order = order;
}

void CECAT7SubHeaderImage::setFilter_Scatter_Fraction(const float frac)            
{
  m_pData->header.Filter_Scatter_Fraction = frac;
}

void CECAT7SubHeaderImage::setFilter_Scatter_Slope(const float slope)            
{
  m_pData->header.Filter_Scatter_Slope = slope;
}

void CECAT7SubHeaderImage::setAnnotation(const char* str)                          
{
  strncpy(m_pData->header.Annotation, str, 40);
}

void CECAT7SubHeaderImage::setMT_1_1(const float value)            
{
  m_pData->header.MT_1_1 = value;
}

void CECAT7SubHeaderImage::setMT_1_2(const float value)            
{
  m_pData->header.MT_1_2 = value;
}

void CECAT7SubHeaderImage::setMT_1_3(const float value)            
{
  m_pData->header.MT_1_3 = value;
}

void CECAT7SubHeaderImage::setMT_2_1(const float value)            
{
  m_pData->header.MT_2_1 = value;
}

void CECAT7SubHeaderImage::setMT_2_2(const float value)            
{
  m_pData->header.MT_2_2 = value;
}

void CECAT7SubHeaderImage::setMT_2_3(const float value)            
{
  m_pData->header.MT_2_3 = value;
}

void CECAT7SubHeaderImage::setMT_3_1(const float value)            
{
  m_pData->header.MT_3_1 = value;
}

void CECAT7SubHeaderImage::setMT_3_2(const float value)            
{
  m_pData->header.MT_3_2 = value;
}

void CECAT7SubHeaderImage::setMT_3_3(const float value)            
{
  m_pData->header.MT_3_3 = value;
}

void CECAT7SubHeaderImage::setRFilter_Cutoff(const float cutoff)            
{
  m_pData->header.RFilter_Cutoff = cutoff;
}

void CECAT7SubHeaderImage::setRFilter_Resolution(const float res)            
{
  m_pData->header.RFilter_Resolution = res;
}

void CECAT7SubHeaderImage::setRFilter_Code(const CECAT7SubHeaderImage::Filter_Code code)            
{
  m_pData->header.RFilter_Code = code;
}

void CECAT7SubHeaderImage::setRFilter_Order(const short order)            
{
  m_pData->header.RFilter_Order = order;
}

void CECAT7SubHeaderImage::setZFilter_Cutoff(const float cutoff)            
{
  m_pData->header.ZFilter_Cutoff = cutoff;
}

void CECAT7SubHeaderImage::setZFilter_Resolution(const float res)            
{
  m_pData->header.ZFilter_Resolution = res;
}

void CECAT7SubHeaderImage::setZFilter_Code(const CECAT7SubHeaderImage::Filter_Code code)            
{
  m_pData->header.ZFilter_Code = code;
}

void CECAT7SubHeaderImage::setZFilter_Order(const short order)            
{
  m_pData->header.ZFilter_Order = order;
}

void CECAT7SubHeaderImage::setMT_1_4(const float value)            
{
  m_pData->header.MT_1_4 = value;
}

void CECAT7SubHeaderImage::setMT_2_4(const float value)            
{
  m_pData->header.MT_2_4 = value;
}

void CECAT7SubHeaderImage::setMT_3_4(const float value)            
{
  m_pData->header.MT_3_4 = value;
}

void CECAT7SubHeaderImage::setScatter_Type(const Scatter_Type type)            
{
  m_pData->header.Scatter_Type = static_cast<quint16>(type);
}

void CECAT7SubHeaderImage::setRecon_Type(const Recon_Type type)            
{
  m_pData->header.Recon_Type = static_cast<quint16>(type);
}

void CECAT7SubHeaderImage::setRecon_Views(const short views)            
{
  m_pData->header.Recon_Views = views;
}

void CECAT7SubHeaderImage::setCTI_Reserved(const short i, const short n)            
{
  m_pData->header.CTI_Reserved[i] = n;
}

void CECAT7SubHeaderImage::setUser_Reserved(const short i, const short n)            
{
  m_pData->header.User_Reserved[i] = n;
}

float CECAT7SubHeaderImage::suv_Scale_Factor(bool& ok) const
{
  ENTER();

  float suvScaleFactor = 1.0f;
  ok = false;

  // check if the SUV scaling factor has been set
  // in the subheader.
  if(scale_Factor() > 0.0f)
  {
    if(m_pMedIOData->isReadable())
    {
      CECATMainHeader* mainHeader = NULL;
      CECATFile* file = static_cast<CECATFile*>(m_pMedIOData);

      if(file->readMainHeader(mainHeader))
      {
        CECAT7MainHeader* e7MainHeader = static_cast<CECAT7MainHeader*>(mainHeader);

        // now calculate the scale factor
        float dosage = e7MainHeader->dosage(); // Bq
        float deltaT = static_cast<float>(e7MainHeader->scan_Start_Time() -
                                          e7MainHeader->dose_Start_Time()); // s
        float halfLife = e7MainHeader->isotope_Halflife(); // s
        float patientWeight = e7MainHeader->patient_Weight() * 1000.0f; // g

        if(patientWeight != 0 && halfLife != 0 &&
           dosage != 0)
        {
          // calculate the suv scaling factor by using dosage, deltaT, halfLife and
          // patient weight to get the suv scale factor commonly used with the philips
          // file formats.
          suvScaleFactor = scale_Factor() / (dosage * qExp(-qLn(2) * (deltaT/halfLife)) / patientWeight);
          ok = true;
        }

        delete e7MainHeader;
      }
    }
  }

  RETURN(suvScaleFactor);
  return suvScaleFactor;
}
