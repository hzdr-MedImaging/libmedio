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

#include "CECAT6MainHeader.h"
#include "CECAT7MainHeader.h"
#include "CECATFile.h"
#include "CECATDirectoryItem.h"

#include <QDataStream>
#include <QFileInfo>

#include <rtdebug.h>

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CECAT6MainHeaderPrivate
{
  public:
    struct HeaderData
    {
      char    Original_File_Name[20];
      quint16  SW_Version;
      quint16  Data_Type;
      quint16  System_Type;
      quint16  File_Type;
      char    Node_ID[10];
      quint16  Scan_Start_Day;
      quint16  Scan_Start_Month;
      quint16  Scan_Start_Year;
      quint16  Scan_Start_Hour;
      quint16  Scan_Start_Minute;
      quint16  Scan_Start_Second;
      char    Isotope_Code[8];
      float    Isotope_Halflife;
      char    Radiopharmaceutical[32];
      float    Gantry_Tilt;
      float    Gantry_Rotation;
      float    Bed_Elevation;
      quint16  Rot_Source_Speed;
      quint16  Wobble_Speed;
      quint16  Transm_Source_Type;
      float    Axial_FOV;
      float    Transaxial_FOV;
      quint16  Transaxial_Samp_Mode;
      quint16  Coin_Samp_Mode;
      quint16  Axial_Samp_Mode;
      float    Calibration_Factor;
      quint16  Calibration_Units;
      quint16  Compression_Code;
      char    Study_Name[12];
      char    Patient_ID[16];
      char    Patient_Name[32];
      char    Patient_Sex[1];
      char    Patient_Age[10];
      char    Patient_Height[10];
      char    Patient_Weight[10];
      char    Patient_Dexterity[1];
      char    Physician_Name[32];
      char    Operator_Name[32];
      char    Study_Description[32];
      quint16  Acquisition_Type;
      quint16  Bed_Type;
      quint16  Septa_Type;
      char    Facility_Name[20];
      quint16  Num_Planes;
      quint16  Num_Frames;
      quint16  Num_Gates;
      quint16  Num_Bed_Pos;
      float    Init_Bed_Position;
      float    Bed_Offset[15];
      float    Plane_Separation;
      quint16  Lwr_Sctr_Thres;
      quint16  Lwr_True_Thres;
      quint16  Upr_True_Thres;
      float    Collimator;
      char    User_Process_Code[10];
      quint16  Acquisition_Mode;
      quint16  CTI_Reserved[33];
    } header;    
};

CECAT6MainHeader::CECAT6MainHeader(CECATFile* ecatFile,
                                   CECATMainHeader::Type fileType)
  : CECATMainHeader(ecatFile)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CECAT6MainHeaderPrivate();

  // this constructor signals us to create a empty ECAT6MainHeader
  // with prefilled data that is always the same for a ECAT6 main header
  // depending on the supplied fileType we have to have a different Magic
  // Number field
  clear();

  LEAVE();
}

CECAT6MainHeader::~CECAT6MainHeader()
{
  ENTER();

  delete m_pData;

  LEAVE();
}

CECAT6MainHeader::CECAT6MainHeader(const CECAT6MainHeader& src)
  : CECATMainHeader(src)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CECAT6MainHeaderPrivate(*(src.m_pData));

  LEAVE();
}

CECAT6MainHeader& CECAT6MainHeader::operator=(const CECAT6MainHeader& src)
{
  ENTER();

  if(m_pData != src.m_pData)
  {
    memcpy(&m_pData->header, 
           &src.m_pData->header, 
           sizeof(struct CECAT6MainHeaderPrivate::HeaderData));
  }

  LEAVE();
  return *this;
}

void CECAT6MainHeader::clear()
{
  ENTER();

  // clear our MainHeader structure first
  memset(&m_pData->header, 0, sizeof(struct CECAT6MainHeaderPrivate::HeaderData));
  
  // default values for our Main Header
  m_pData->header.SW_Version = 64;    // use v6.4 as the default ECAT6 version

  LEAVE();
}

bool CECAT6MainHeader::load(void)
{
  ENTER();

  // only go on if the device is readable at all
  if(m_pMedIOData == NULL ||
     m_pMedIOData->seek(0) == false ||
     m_pMedIOData->isReadable() == false)
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
  
  // we have to set the QDataStream version to the Qt4.5 version
  // because with Qt4.6 the floating point precision changed and
  // otherwise causes our streaming to fail
  stream.setVersion(QDataStream::Qt_4_5);

  // we read in the MainHeader data with one read
  // operation
  stream.readRawData(&m_pData->header.Original_File_Name[0], 20);    //   0: Original_File_Name
  stream >> m_pData->header.SW_Version;                              //  20: SW_Version
  stream >> m_pData->header.Data_Type;                                //  22: Data_Type
  stream >> m_pData->header.System_Type;                             //  24: System_Type
  stream >> m_pData->header.File_Type;                               //  26: File_Type
  stream.readRawData(&m_pData->header.Node_ID[0], 10);              //  28: Node_ID
  stream >> m_pData->header.Scan_Start_Day;                          //  38: Scan_Start_Day
  stream >> m_pData->header.Scan_Start_Month;                        //  40: Scan_Start_Month
  stream >> m_pData->header.Scan_Start_Year;                         //  42: Scan_Start_Year
  stream >> m_pData->header.Scan_Start_Hour;                         //  44: Scan_Start_Hour
  stream >> m_pData->header.Scan_Start_Minute;                       //  46: Scan_Start_Minute
  stream >> m_pData->header.Scan_Start_Second;                       //  48: Scan_Start_Minute
  stream.readRawData(&m_pData->header.Isotope_Code[0], 8);          //  50: Isotope_Code
  stream >> m_pData->header.Isotope_Halflife;                        //  58: Isotope_Halflife
  stream.readRawData(&m_pData->header.Radiopharmaceutical[0], 32);  //  62: Radiopharmaceutical
  stream >> m_pData->header.Gantry_Tilt;                              //  94: Gantry_Tilt
  stream >> m_pData->header.Gantry_Rotation;                          // 102: Gantry_Rotation
  stream >> m_pData->header.Bed_Elevation;                            // 106: Bed_Elevation
  stream >> m_pData->header.Rot_Source_Speed;                        // 110: Rot_Source_Speed
  stream >> m_pData->header.Wobble_Speed;                            // 112: Wobble_Speed
  stream >> m_pData->header.Transm_Source_Type;                      // 114: Transm_Source_Type
  stream >> m_pData->header.Axial_FOV;                                // 116: Axial_FOV
  stream >> m_pData->header.Transaxial_FOV;                          // 120: Transaxial_FOV
  stream >> m_pData->header.Coin_Samp_Mode;                          // 124: Coin_Samp_Mode
  stream >> m_pData->header.Axial_Samp_Mode;                          // 126: Axial_Samp_Mode
  stream >> m_pData->header.Calibration_Factor;                      // 128: Calibration_Factor
  stream >> m_pData->header.Calibration_Units;                        // 132: Calibration_Units
  stream >> m_pData->header.Compression_Code;                        // 134: Compression_Code
  stream.readRawData(&m_pData->header.Study_Name[0], 12);            // 136: Study_Name
  stream.readRawData(&m_pData->header.Patient_ID[0], 16);            // 148: Patient_ID
  stream.readRawData(&m_pData->header.Patient_Name[0], 32);          // 164: Patient_Name
  stream.readRawData(&m_pData->header.Patient_Sex[0], 1);            // 196: Patient_Sex
  stream.readRawData(&m_pData->header.Patient_Age[0], 10);          // 197: Patient_Age
  stream.readRawData(&m_pData->header.Patient_Height[0], 10);        // 207: Patient_Height
  stream.readRawData(&m_pData->header.Patient_Weight[0], 10);        // 217: Patient_Weight
  stream.readRawData(&m_pData->header.Patient_Dexterity[0], 1);      // 227: Patient_Dexterity
  stream.readRawData(&m_pData->header.Physician_Name[0], 32);        // 228: Physician_Name
  stream.readRawData(&m_pData->header.Operator_Name[0], 32);        // 260: Operator_Name
  stream.readRawData(&m_pData->header.Study_Description[0], 32);    // 292: Study_Description
  stream >> m_pData->header.Acquisition_Type;                        // 324: Acquisition_Type
  stream >> m_pData->header.Bed_Type;                                // 326: Bed_Type
  stream >> m_pData->header.Septa_Type;                              // 328: Septa_Type
  stream.readRawData(&m_pData->header.Facility_Name[0], 20);        // 330: Facility_Name
  stream >> m_pData->header.Num_Planes;                              // 350: Num_Planes
  stream >> m_pData->header.Num_Frames;                              // 352: Num_Frames
  stream >> m_pData->header.Num_Gates;                                // 354: Num_Gates
  stream >> m_pData->header.Num_Bed_Pos;                              // 356: Num_Bed_Pos
  stream >> m_pData->header.Init_Bed_Position;                        // 358: Init_Bed_Position
  for(int i=0; i < 15; i++)
    stream >> m_pData->header.Bed_Offset[i];                          // 362: Bed_Offset (15)
  stream >> m_pData->header.Plane_Separation;                        // 422: Plane_Separation
  stream >> m_pData->header.Lwr_Sctr_Thres;                          // 426: Lwr_Sctr_Thres
  stream >> m_pData->header.Lwr_True_Thres;                          // 430: Lwr_True_Thres
  stream >> m_pData->header.Upr_True_Thres;                          // 432: Upr_True_Thres
  stream >> m_pData->header.Collimator;                              // 434: Collimator
  stream.readRawData(&m_pData->header.User_Process_Code[0], 10);    // 438: User_Process_Code
  stream >> m_pData->header.Acquisition_Mode;                        // 448: Acquisition_Mode
  
  // some more debug output
#if defined(DEBUG)
  if(m_pData->header.SW_Version > 0 && m_pData->header.SW_Version <= 69)
  {
    D("ECAT6 Main Header loaded:");
    D("------------------------");
    D("Original_File_Name      : %s",                 m_pData->header.Original_File_Name);
    D("SW_Version              : %d",                 m_pData->header.SW_Version);
    D("Data_Type               : %d",                 m_pData->header.Data_Type);
    D("System_Type             : %d",                 m_pData->header.System_Type);
    D("File_Type               : %d",                 m_pData->header.File_Type);
    D("Node_ID                 : %s",                 m_pData->header.Node_ID);
    D("Scan_Start_Day          : %d",                 m_pData->header.Scan_Start_Day);
    D("Scan_Start_Month        : %d",                 m_pData->header.Scan_Start_Month);
    D("Scan_Start_Year         : %d",                 m_pData->header.Scan_Start_Year);
    D("Scan_Start_Hour         : %d",                 m_pData->header.Scan_Start_Hour);
    D("Scan_Start_Minute       : %d",                 m_pData->header.Scan_Start_Minute);
    D("Scan_Start_Second       : %d",                 m_pData->header.Scan_Start_Second);
    D("Isotope Name            : %s",                 m_pData->header.Isotope_Code);
    D("Isotope Halflife        : %f sec",          m_pData->header.Isotope_Halflife);
    D("Radiopharmaceutical     : %s",                m_pData->header.Radiopharmaceutical);
    D("Gantry Tilt             : %f°",            m_pData->header.Gantry_Tilt);
    D("Gantry Rotation         : %f°",              m_pData->header.Gantry_Rotation);
    D("Bed elevation           : %f cm",            m_pData->header.Bed_Elevation);
    D("Rot_Source_Speed        : %d",                 m_pData->header.Rot_Source_Speed);
    D("Wobble_Speed            : %d",                 m_pData->header.Wobble_Speed);
    D("Transm_Source_Type      : %d",                 m_pData->header.Transm_Source_Type);
    D("Axial_FOV               : %f cm",          m_pData->header.Axial_FOV);
    D("Transaxial_FOV          : %f cm",          m_pData->header.Transaxial_FOV);
    D("Transaxial_Samp_Mode    : %d",              m_pData->header.Transaxial_Samp_Mode);
    D("Coin_Samp_Mode          : %d",              m_pData->header.Coin_Samp_Mode);
    D("Axial_Samp_Mode         : %d",              m_pData->header.Axial_Samp_Mode);
    D("Calibration_Factor      : %f",             m_pData->header.Calibration_Factor);
    D("Calibration_Units       : %d",             m_pData->header.Calibration_Units);
    D("Compression Code        : %d",              m_pData->header.Compression_Code);
    D("Study_Name              : %s",              m_pData->header.Study_Name);
    D("Patient ID              : %s",              m_pData->header.Patient_ID);
    D("Patient Name            : %s",              m_pData->header.Patient_Name);
    D("Patient Sex             : %c",              m_pData->header.Patient_Sex[0]);
    D("Patient Age             : %f years",        m_pData->header.Patient_Age);
    D("Patient Height          : %f cm",          m_pData->header.Patient_Height);
    D("Patient Weight          : %f kg",          m_pData->header.Patient_Weight);
    D("Patient Dexterity       : %c",              m_pData->header.Patient_Dexterity[0]);
    D("Physician Name          : %s",              m_pData->header.Physician_Name);
    D("Operator Name           : %s",              m_pData->header.Operator_Name);
    D("Study Description       : %s",              m_pData->header.Study_Description);
    D("Acquisition Type        : %d",              m_pData->header.Acquisition_Type);
    D("Bed_Type                : %d",              m_pData->header.Bed_Type);
    D("Septa_Type              : %d",              m_pData->header.Septa_Type);
    D("Facility Name           : %s",              m_pData->header.Facility_Name);
    D("Planes                  : %d",              m_pData->header.Num_Planes);
    D("Frames                  : %d",              m_pData->header.Num_Frames);
    D("Gates                   : %d",              m_pData->header.Num_Gates);
    D("Bed positions           : %d",              m_pData->header.Num_Bed_Pos);
    D("Initial Bed position    : %f cm",          m_pData->header.Init_Bed_Position);
    for(int i=0; i < 15; i++)
    {
      D("Bed offset          [%2d]: %f cm", i+1, m_pData->header.Bed_Offset[i]);
    }
    D("Plane separation        : %f cm",          m_pData->header.Plane_Separation);
    D("Lowest Threshold Scatter: %d KeV",          m_pData->header.Lwr_Sctr_Thres);
    D("Lower Threshold Trues   : %d KeV",          m_pData->header.Lwr_True_Thres);
    D("Upper Threshold Trues   : %d KeV",          m_pData->header.Upr_True_Thres);
    D("Collimator              : %f",              m_pData->header.Collimator);
    D("User_Process_Code       : %s",              m_pData->header.User_Process_Code);
    D("Acquisition_Mode        : %d",              m_pData->header.Acquisition_Mode);
  }
#endif
  
  RETURN(true);
  return true;
}

bool CECAT6MainHeader::save(void) const
{
  ENTER();

  // only go on if the device is writeable at all
  if(m_pMedIOData == NULL ||
     m_pMedIOData->isWritable() == false ||
     m_pMedIOData->seek(0) == false)
  {
    RETURN(false);
    return false;
  }

  // before we can start reading out some data we have to collect some
  // out data beforehand which we use instead of the data stored in our
  // data structure (such as frames/planes/gates etc.)
  CECATFile* ecatFile = static_cast<CECATFile*>(m_pMedIOData);
  quint16 numPlanes = ecatFile->numPlanes();
  quint16 numFrames = ecatFile->numFrames();
  quint16 numGates  = ecatFile->numGates();
  quint16 numBedPos = ecatFile->numBedPos();

  // we write to a buffer first and write out later directly to the file
  QByteArray buffer(rawDataSize(), 0);
  QDataStream stream(&buffer, QIODevice::WriteOnly);

  // we have to set the QDataStream version to the Qt4.5 version
  // because with Qt4.6 the floating point precision changed and
  // otherwise causes our streaming to fail
  stream.setVersion(QDataStream::Qt_4_5);
 
  // we read in the MainHeader data with one read
  // operation
  stream.writeRawData(&m_pData->header.Original_File_Name[0], 20);  //   0: Original_File_Name
  stream << m_pData->header.SW_Version;                              //  20: SW_Version
  stream << m_pData->header.Data_Type;                                //  22: Data_Type
  stream << m_pData->header.System_Type;                             //  24: System_Type
  stream << m_pData->header.File_Type;                               //  26: File_Type
  stream.writeRawData(&m_pData->header.Node_ID[0], 10);              //  28: Node_ID
  stream << m_pData->header.Scan_Start_Day;                          //  38: Scan_Start_Day
  stream << m_pData->header.Scan_Start_Month;                        //  40: Scan_Start_Month
  stream << m_pData->header.Scan_Start_Year;                         //  42: Scan_Start_Year
  stream << m_pData->header.Scan_Start_Hour;                         //  44: Scan_Start_Hour
  stream << m_pData->header.Scan_Start_Minute;                       //  46: Scan_Start_Minute
  stream << m_pData->header.Scan_Start_Second;                       //  48: Scan_Start_Minute
  stream.writeRawData(&m_pData->header.Isotope_Code[0], 8);          //  50: Isotope_Code
  stream << m_pData->header.Isotope_Halflife;                        //  58: Isotope_Halflife
  stream.writeRawData(&m_pData->header.Radiopharmaceutical[0], 32);  //  62: Radiopharmaceutical
  stream << m_pData->header.Gantry_Tilt;                              //  94: Gantry_Tilt
  stream << m_pData->header.Gantry_Rotation;                          // 102: Gantry_Rotation
  stream << m_pData->header.Bed_Elevation;                            // 106: Bed_Elevation
  stream << m_pData->header.Rot_Source_Speed;                        // 110: Rot_Source_Speed
  stream << m_pData->header.Wobble_Speed;                            // 112: Wobble_Speed
  stream << m_pData->header.Transm_Source_Type;                      // 114: Transm_Source_Type
  stream << m_pData->header.Axial_FOV;                                // 116: Axial_FOV
  stream << m_pData->header.Transaxial_FOV;                          // 120: Transaxial_FOV
  stream << m_pData->header.Coin_Samp_Mode;                          // 124: Coin_Samp_Mode
  stream << m_pData->header.Axial_Samp_Mode;                          // 126: Axial_Samp_Mode
  stream << m_pData->header.Calibration_Factor;                      // 128: Calibration_Factor
  stream << m_pData->header.Calibration_Units;                        // 132: Calibration_Units
  stream << m_pData->header.Compression_Code;                        // 134: Compression_Code
  stream.writeRawData(&m_pData->header.Study_Name[0], 12);          // 136: Study_Name
  stream.writeRawData(&m_pData->header.Patient_ID[0], 16);          // 148: Patient_ID
  stream.writeRawData(&m_pData->header.Patient_Name[0], 32);        // 164: Patient_Name
  stream.writeRawData(&m_pData->header.Patient_Sex[0], 1);          // 196: Patient_Sex
  stream.writeRawData(&m_pData->header.Patient_Age[0], 10);          // 197: Patient_Age
  stream.writeRawData(&m_pData->header.Patient_Height[0], 10);      // 207: Patient_Height
  stream.writeRawData(&m_pData->header.Patient_Weight[0], 10);      // 217: Patient_Weight
  stream.writeRawData(&m_pData->header.Patient_Dexterity[0], 1);    // 227: Patient_Dexterity
  stream.writeRawData(&m_pData->header.Physician_Name[0], 32);      // 228: Physician_Name
  stream.writeRawData(&m_pData->header.Operator_Name[0], 32);        // 260: Operator_Name
  stream.writeRawData(&m_pData->header.Study_Description[0], 32);    // 292: Study_Description
  stream << m_pData->header.Acquisition_Type;                        // 324: Acquisition_Type
  stream << m_pData->header.Bed_Type;                                // 326: Bed_Type
  stream << m_pData->header.Septa_Type;                              // 328: Septa_Type
  stream.writeRawData(&m_pData->header.Facility_Name[0], 20);        // 330: Facility_Name
  stream << numPlanes;                                      // 350: Num_Planes
  stream << numFrames;                                      // 352: Num_Frames
  stream << numGates;                                        // 354: Num_Gates
  stream << numBedPos;                                      // 356: Num_Bed_Pos
  stream << m_pData->header.Init_Bed_Position;                        // 358: Init_Bed_Position
  for(int i=0; i < 15; i++)
    stream << m_pData->header.Bed_Offset[i];                          // 362: Bed_Offset (15)
  stream << m_pData->header.Plane_Separation;                        // 422: Plane_Separation
  stream << m_pData->header.Lwr_Sctr_Thres;                          // 426: Lwr_Sctr_Thres
  stream << m_pData->header.Lwr_True_Thres;                          // 430: Lwr_True_Thres
  stream << m_pData->header.Upr_True_Thres;                          // 432: Upr_True_Thres
  stream << m_pData->header.Collimator;                              // 434: Collimator
  stream.writeRawData(&m_pData->header.User_Process_Code[0], 10);    // 438: User_Process_Code
  stream << m_pData->header.Acquisition_Mode;                        // 448: Acquisition_Mode

  // now write out to our outStream
  bool result = false;
  if(m_pMedIOData->write(buffer) != -1)
  {
    ecatFile->mainHeaderWritten(*this);
    result = true;
  }

  RETURN(result);
  return result;
}

int CECAT6MainHeader::rawDataSize() const
{ 
  return 1*ECAT_BLOCKSIZE;
}

int CECAT6MainHeader::rtti() const
{ 
  return CECATMainHeader::ECAT6MainHeader;
}

bool CECAT6MainHeader::convertFrom(const CMedIOHeader* mainHeader, const CMedIOHeader* subHeader)
{
  ENTER();
  bool bResult = false;

  // depending on the MedIOHeader format we do have to 
  // distinguish between our copy operations.
  switch(mainHeader->headerFormat())
  {
    case CMedIOHeader::ECATMainHeader:
    {
      const CECATMainHeader* eMainHeader = static_cast<const CECATMainHeader*>(mainHeader);

      // depending on the source type we have to copy either every data or just 
      // some data of the src header
      switch(eMainHeader->rtti())
      {
        // if the source header is also an ECAT6 one we can copy it in whole
        // via a simple memcpy()
        case CECATMainHeader::ECAT6MainHeader:
        {
          // we use the assignment operator which will do the convertation
          // for us.
          *this = *static_cast<const CECAT6MainHeader*>(mainHeader);

          bResult = true;
        }
        break;

        // if this is an ECAT7 Mainheader we have to take care of the fact that
        // some information is missing in one of the headers.
        case CECATMainHeader::ECAT7MainHeader:
        {
          const CECAT7MainHeader* e7src = static_cast<const CECAT7MainHeader*>(mainHeader);
          clear();
          setOriginal_File_Name(e7src->original_File_Name());
          setSystem_Type(e7src->system_Type());

          bResult = true;
          #warning "ECAT7->ECAT6 copy not fully implemented yet!"
        }
        break;
      }
  
      // afterwards we have to make sure sensible data is restored
      m_pData->header.SW_Version = 64; // This header does conform to the ECAT 6.4 standard
    }

    case CMedIOHeader::ECATSubHeader:
    case CMedIOHeader::ConcordeMicroPetFrameHeader:
      // copying a sub header into a main header doesn't make much sense, so we
      // do nothing here
    break;

    case CMedIOHeader::ConcordeMicroPetMainHeader:
    {
      #warning "Concorde->ECAT6 copy missing"
    }
    break;

    case CMedIOHeader::Unknown:
      // for an unknown header type we do nothing
    break;
  }

  RETURN(bResult);
  return bResult;
}

CMedIOHeader* CECAT6MainHeader::clone() const
{
  ENTER();

  CECAT6MainHeader* pNewHead = new CECAT6MainHeader();
  pNewHead->convertFrom(this);

  RETURN(pNewHead);
  return pNewHead;
}

const char* CECAT6MainHeader::original_File_Name(void) const
{ 
  return m_pData->header.Original_File_Name;
}

short CECAT6MainHeader::sw_Version(void) const
{ 
  return m_pData->header.SW_Version;
}

short CECAT6MainHeader::system_Type(void) const
{ 
  return m_pData->header.System_Type;
}

CECAT6MainHeader::File_Type CECAT6MainHeader::file_Type(void)  const
{ 
  return static_cast<CECAT6MainHeader::File_Type>(m_pData->header.File_Type);
}

short CECAT6MainHeader::num_Planes(void) const
{ 
  return m_pData->header.Num_Planes;    
}

short CECAT6MainHeader::num_Frames(void) const  
{ 
  return m_pData->header.Num_Frames;
}

short CECAT6MainHeader::num_Gates(void) const
{  
  return m_pData->header.Num_Gates;    
}

short CECAT6MainHeader::num_Bed_Pos(void) const  
{ 
  return m_pData->header.Num_Bed_Pos;
}

// mutator methods
void CECAT6MainHeader::setOriginal_File_Name(const char* name)
{ 
  strncpy(m_pData->header.Original_File_Name, name, 20);
}

void CECAT6MainHeader::setSW_Version(const short ver)
{ 
  m_pData->header.SW_Version = ver;
}

void CECAT6MainHeader::setSystem_Type(const short type)
{ 
  m_pData->header.System_Type = type;
}

void CECAT6MainHeader::setFileType(const File_Type fType)
{ 
  m_pData->header.File_Type = fType;
} 

void CECAT6MainHeader::setNum_Planes(const short num)
{ 
  m_pData->header.Num_Planes = num;
}

void CECAT6MainHeader::setNum_Frames(const short num)
{ 
  m_pData->header.Num_Frames = num;
}

void CECAT6MainHeader::setNum_Gates(const short num)
{ 
  m_pData->header.Num_Gates = num;      
}

void CECAT6MainHeader::setNum_Bed_Pos(const short num)
{ 
  m_pData->header.Num_Bed_Pos = num;    
}
