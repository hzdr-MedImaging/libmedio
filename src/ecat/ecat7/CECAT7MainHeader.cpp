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

#include "CECAT7MainHeader.h"
#include "CECAT6MainHeader.h"
#include "CECATFile.h"
#include "CECATDirectoryItem.h"
#include "CConcordeFrameHeader.h"
#include "CPhilipsMainHeader.h"
#include "CPhilipsExtendedMainHeader.h"
#include "MedIOUnits.h"

#include "config.h"

#include <QDataStream>
#include <QDateTime>
#include <QFileInfo>
#include <QTextStream>

#include <time.h>
#include <unistd.h>

#include <rtdebug.h>

#include <iostream>

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CECAT7MainHeaderPrivate
{
  public:
    void updateMagicNumber(void);

    // helper methods for converting the header from concorde to ecat7
    QString concorde2ECAT7dataUnits(CConcordeMainHeader::CalibrationUnits u) const;
    CECAT7MainHeader::Calibration_Units concorde2ECAT7calibrationUnits(CConcordeMainHeader::CalibrationUnits u) const;
    float concorde2ECAT7dosage(float d, CConcordeMainHeader::DoseUnits u) const;
    float concorde2ECAT7Height(float l, CConcordeMainHeader::SubjectLengthUnits u) const;
    float concorde2ECAT7Weight(float w, CConcordeMainHeader::SubjectWeightUnits u) const;
    CECAT7MainHeader::Patient_Sex concorde2ECAT7Sex(const QString& genus) const;
    CECAT7MainHeader::Patient_Orientation concorde2ECAT7Orientation(const CConcordeMainHeader::SubjectOrientation o) const;

    CECAT7MainHeader::Patient_Sex philips2Ecat7Sex(const char& genus) const;

    // MainHeader structure (should be 512bytes)
    struct HeaderData
    {
      char    Magic_Number[14];
      char    Original_File_Name[32];
      quint16  SW_Version;
      quint16  System_Type;
      quint16  File_Type;
      char    Serial_Number[10];
      qint32  Scan_Start_Time;
      char    Isotope_Name[8];
      float    Isotope_Halflife;
      char    Radiopharmaceutical[32];
      float    Gantry_Tilt;
      float    Gantry_Rotation;
      float    Bed_Elevation;
      float    Intrinsic_Tilt;
      quint16  Wobble_Speed;
      quint16  Transm_Source_Type;
      float    Distance_Scanned;
      float    Transaxial_FOV;
      quint16  Angular_Compression;
      quint16  Coin_Samp_Mode;
      quint16  Axial_Samp_Mode;
      float    Calibration_Factor;
      quint16  Calibration_Units;
      quint16  Calibration_Units_Label;
      quint16  Compression_Code;
      char    Study_Type[12];
      char    Patient_ID[16];
      char    Patient_Name[32];
      char    Patient_Sex[1];
      char    Patient_Dexterity[1];
      float    Patient_Age;
      float    Patient_Height;
      float    Patient_Weight;
      qint32  Patient_Birth_Date;
      char    Physician_Name[32];
      char    Operator_Name[32];
      char    Study_Description[32];
      quint16  Acquisition_Type;
      quint16  Patient_Orientation;
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
      char    User_Process_Code[10];
      quint16  Acquisition_Mode;
      float    Bin_Size;
      float    Branching_Fraction;
      qint32  Dose_Start_Time;
      float    Dosage;
      float    Well_Counter_Corr_Factor;
      char    Data_Units[32];
      quint16  Septa_State;
      quint16  CTI_Reserved[6];
    } header;    
};

CECAT7MainHeader::CECAT7MainHeader(CECATFile* ecatFile, 
                                   CECATMainHeader::Type fileType)
  : CECATMainHeader(ecatFile)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CECAT7MainHeaderPrivate();

  // this constructor signals us to create a empty ECAT7MainHeader
  // with prefilled data that is always the same for a ECAT7 main header
  // depending on the supplied fileType we have to have a different Magic
  // Number field
  clear();

  setFileType(fileType);

  LEAVE();
}

CECAT7MainHeader::CECAT7MainHeader(const CECAT7MainHeader& src)
  : CECATMainHeader(src)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CECAT7MainHeaderPrivate(*(src.m_pData));

  LEAVE();
}

CECAT7MainHeader& CECAT7MainHeader::operator=(const CECAT7MainHeader& src)
{
  ENTER();

  if(m_pData != src.m_pData)
  {
    memcpy(&m_pData->header, 
           &src.m_pData->header, 
           sizeof(struct CECAT7MainHeaderPrivate::HeaderData));
  }

  LEAVE();
  return *this;
}

CECAT7MainHeader::~CECAT7MainHeader()
{
  ENTER();

  delete m_pData;

  LEAVE();
}

void CECAT7MainHeader::clear()
{
  ENTER();
  
  // clear our MainHeader structure first
  memset(&m_pData->header, 0, sizeof(struct CECAT7MainHeaderPrivate::HeaderData));
  
  // set some default values for the new Main Header
  // (we mainly set these values like it is set on a EXACT HR+ scanner
  m_pData->header.SW_Version            = 72;          // use v7.2 as the default ECAT version
  m_pData->header.System_Type          = 962;        // set ECAT Model 962 (EXACT HR+) by default
  m_pData->header.Distance_Scanned      = 15.52;      // scanner depth    
  m_pData->header.Transaxial_FOV        = 58.3;        // transaxial FOV of an ECAT HR+
  m_pData->header.Angular_Compression  = static_cast<short>(Mash2);
  m_pData->header.Acquisition_Type      = static_cast<short>(DynamicEmission);
  m_pData->header.Acquisition_Mode      = static_cast<short>(Windowed);
  m_pData->header.Transm_Source_Type    = static_cast<short>(SRC_RING);
  m_pData->header.Lwr_True_Thres        = 350;        // energy levels of an ECAT HR+
  m_pData->header.Upr_True_Thres        = 650;
  m_pData->header.Patient_Orientation  = CECAT7MainHeader::Orient_Unknown;
  m_pData->header.Bin_Size              = 0.224983;    // cm
  m_pData->header.Branching_Fraction    = 1.0;
  m_pData->header.Septa_State          = static_cast<short>(Retracted);

  strcpy(m_pData->header.Serial_Number, "1");
  strcpy(m_pData->header.Study_Description, (QString("Created with libmedio v") + PACKAGE_VERSION).toAscii());

  LEAVE();
}

void CECAT7MainHeader::setFileType(CECATMainHeader::Type fType)
{
  ENTER();
  
  // to identify the correct fileType we need to do a proper mapping
  switch(fType)
  {
    case CECATMainHeader::Unknown:  m_pData->header.File_Type = Unknown;            break;
    case ECAT7_Sinogram:            m_pData->header.File_Type = Sinogram;          break;
    case ECAT7_Image16:              m_pData->header.File_Type = Image16;            break;
    case ECAT7_AttenCorr:            m_pData->header.File_Type = AttenuationCorr;    break;
    case ECAT7_Normalization:        m_pData->header.File_Type = Normalization;      break;
    case ECAT7_PolarMap:            m_pData->header.File_Type = PolarMap;          break;
    case ECAT7_Volume8:              m_pData->header.File_Type = Volume8;            break;
    case ECAT7_Volume16:            m_pData->header.File_Type = Volume16;          break;
    case ECAT7_Projection8:          m_pData->header.File_Type = Projection8;        break;
    case ECAT7_Projection16:        m_pData->header.File_Type = Projection16;      break;
    case ECAT7_Image8:              m_pData->header.File_Type = Image8;            break;
    case ECAT7_Sinogram3D_16:        m_pData->header.File_Type = Sinogram3D_16;      break;
    case ECAT7_Sinogram3D_8:        m_pData->header.File_Type = Sinogram3D_8;      break;
    case ECAT7_Normalization_3D:    m_pData->header.File_Type = Normalization_3D;  break;
    case ECAT7_Sinogram3D_Float:    m_pData->header.File_Type = Sinogram3D_Float;  break;
  }

  m_pData->updateMagicNumber();

  LEAVE();
}

void CECAT7MainHeaderPrivate::updateMagicNumber(void)
{
  ENTER();
  
  switch(header.File_Type)
  {    
    case CECAT7MainHeader::Sinogram:
    case CECAT7MainHeader::Sinogram3D_8:
    case CECAT7MainHeader::Sinogram3D_16:
    case CECAT7MainHeader::Sinogram3D_Float:
    {
      strcpy(header.Magic_Number, "MATRIX72S");
    }
    break;

    case CECAT7MainHeader::Volume8:
    case CECAT7MainHeader::Volume16:
    {
      strcpy(header.Magic_Number, "MATRIX72V");
    }
    break;

    case CECAT7MainHeader::Image8:
    case CECAT7MainHeader::Image16:
    {
      strcpy(header.Magic_Number, "MATRIX72I");
    }
    break;

    case CECAT7MainHeader::AttenuationCorr:
    {
      strcpy(header.Magic_Number, "MATRIX72A");
    }
    break;

    case CECAT7MainHeader::Normalization:
    case CECAT7MainHeader::Normalization_3D:
    {
      strcpy(header.Magic_Number, "MATRIX72N");
    }
    break;

    case CECAT7MainHeader::Projection8:
    case CECAT7MainHeader::Projection16:
    {
      strcpy(header.Magic_Number, "MATRIX72P");
    }
    break;

    case CECAT7MainHeader::PolarMap:
    {
      strcpy(header.Magic_Number, "MATRIX72M");
    }
    break;

    default:
    {
      strcpy(header.Magic_Number, "MATRIX72");
    }
  }  

  LEAVE();
}

bool CECAT7MainHeader::load(void)
{
  ENTER();

  // only go on if the device is readable at all
  if(m_pMedIOData == NULL ||
     m_pMedIOData->isReadable() == false ||
     m_pMedIOData->seek(0) == false)
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
  
  // we now read out the header information stepwise
  // as we have to care about big/little endianess, which
  // is automatically done by the QT framework
  stream.readRawData(&m_pData->header.Magic_Number[0], 14);         //   0: Magic_Number
  stream.readRawData(&m_pData->header.Original_File_Name[0], 32);   //  14: Original_File_Name
  stream >> m_pData->header.SW_Version;                              //  46: SW_Version
  stream >> m_pData->header.System_Type;                             //  48: System_Type
  stream >> m_pData->header.File_Type;                               //  50: File_Type
  stream.readRawData(&m_pData->header.Serial_Number[0], 10);        //  52: Original_File_Name
  stream >> m_pData->header.Scan_Start_Time;                         //  62: Scan_Start_Time
  stream.readRawData(&m_pData->header.Isotope_Name[0], 8);          //  66: Isotope_Name
  stream >> m_pData->header.Isotope_Halflife;                        //  74: Isotope_Halflife
  stream.readRawData(&m_pData->header.Radiopharmaceutical[0], 32);  //  78: Radiopharmaceutical
  stream >> m_pData->header.Gantry_Tilt;                              // 110: Gantry_Tilt
  stream >> m_pData->header.Gantry_Rotation;                          // 114: Gantry_Rotation
  stream >> m_pData->header.Bed_Elevation;                            // 118: Bed_Elevation
  stream >> m_pData->header.Intrinsic_Tilt;                          // 122: Intrinsic_Tilt
  stream >> m_pData->header.Wobble_Speed;                            // 126: Wobble_Speed
  stream >> m_pData->header.Transm_Source_Type;                      // 128: Transm_Source_Type
  stream >> m_pData->header.Distance_Scanned;                        // 130: Distance_Scanned
  stream >> m_pData->header.Transaxial_FOV;                          // 134: Transaxial_FOV
  stream >> m_pData->header.Angular_Compression;                      // 138: Angular_Compression
  stream >> m_pData->header.Coin_Samp_Mode;                          // 140: Coin_Samp_Mode
  stream >> m_pData->header.Axial_Samp_Mode;                          // 142: Axial_Samp_Mode
  stream >> m_pData->header.Calibration_Factor;                      // 144: Calibration_Factor
  stream >> m_pData->header.Calibration_Units;                        // 148: Calibration_Units
  stream >> m_pData->header.Calibration_Units_Label;                  // 150: Calibration_Units_Label
  stream >> m_pData->header.Compression_Code;                        // 152: Compression_Code
  stream.readRawData(&m_pData->header.Study_Type[0], 12);            // 154: Study_Type
  stream.readRawData(&m_pData->header.Patient_ID[0], 16);            // 166: Patient_ID
  stream.readRawData(&m_pData->header.Patient_Name[0], 32);          // 182: Patient_Name
  stream.readRawData(&m_pData->header.Patient_Sex[0], 1);            // 214: Patient_Sex
  stream.readRawData(&m_pData->header.Patient_Dexterity[0], 1);      // 215: Patient_Dexterity
  stream >> m_pData->header.Patient_Age;                              // 216: Patient_Age
  stream >> m_pData->header.Patient_Height;                          // 220: Patient_Height
  stream >> m_pData->header.Patient_Weight;                          // 224: Patient_Weight
  stream >> m_pData->header.Patient_Birth_Date;                      // 228: Patient_Birth_Date
  stream.readRawData(&m_pData->header.Physician_Name[0], 32);        // 232: Physician_Name
  stream.readRawData(&m_pData->header.Operator_Name[0], 32);        // 264: Operator_Name
  stream.readRawData(&m_pData->header.Study_Description[0], 32);    // 296: Study_Description
  stream >> m_pData->header.Acquisition_Type;                        // 328: Acquisition_Type
  stream >> m_pData->header.Patient_Orientation;                      // 330: Patient_Orientation
  stream.readRawData(&m_pData->header.Facility_Name[0], 20);        // 332: Facility_Name
  stream >> m_pData->header.Num_Planes;                              // 352: Num_Planes
  stream >> m_pData->header.Num_Frames;                              // 354: Num_Frames
  stream >> m_pData->header.Num_Gates;                                // 356: Num_Gates
  stream >> m_pData->header.Num_Bed_Pos;                              // 358: Num_Bed_Pos
  stream >> m_pData->header.Init_Bed_Position;                        // 360: Init_Bed_Position
  for(short i=0; i < 15; i++)
    stream >> m_pData->header.Bed_Offset[i];                          // 364: Bed_Offset (15)
  stream >> m_pData->header.Plane_Separation;                        // 424: Plane_Separation
  stream >> m_pData->header.Lwr_Sctr_Thres;                          // 428: Lwr_Sctr_Thres
  stream >> m_pData->header.Lwr_True_Thres;                          // 430: Lwr_True_Thres
  stream >> m_pData->header.Upr_True_Thres;                          // 432: Upr_True_Thres
  stream.readRawData(&m_pData->header.User_Process_Code[0], 10);    // 434: User_Process_Code
  stream >> m_pData->header.Acquisition_Mode;                        // 444: Acquisition_Mode
  stream >> m_pData->header.Bin_Size;                                // 446: Bin_Size
  stream >> m_pData->header.Branching_Fraction;                      // 450: Branching_Fraction
  stream >> m_pData->header.Dose_Start_Time;                          // 454: Dose_Start_Time
  stream >> m_pData->header.Dosage;                                  // 458: Dosage
  stream >> m_pData->header.Well_Counter_Corr_Factor;                // 462: Well_Counter_Corr_Factor
  stream.readRawData(&m_pData->header.Data_Units[0], 32);            // 466: Data_Units
  stream >> m_pData->header.Septa_State;                              // 498: Septa_State
  stream >> m_pData->header.CTI_Reserved[0];                          // 500: CTI_Reserved 1
  stream >> m_pData->header.CTI_Reserved[1];                          // 502: CTI_Reserved 2
  stream >> m_pData->header.CTI_Reserved[2];                          // 504: CTI_Reserved 3
  stream >> m_pData->header.CTI_Reserved[3];                          // 506: CTI_Reserved 4
  stream >> m_pData->header.CTI_Reserved[4];                          // 508: CTI_Reserved 5
  stream >> m_pData->header.CTI_Reserved[5];                          // 510: CTI_Reserved 6
    
  // some more debug output
#if defined(DEBUG)
  D("ECAT7 Main Header loaded:");
  D("------------------------");
  D("Magic_Number            : %s",              m_pData->header.Magic_Number);
  D("Original_File_Name      : %s",                 m_pData->header.Original_File_Name);
  D("SW_Version              : %d",                 m_pData->header.SW_Version);
  D("System_Type             : %d",                 m_pData->header.System_Type);
  D("File_Type               : %d",                 m_pData->header.File_Type);
  D("Serial_Number           : %s",                 m_pData->header.Serial_Number);
  D("Scan_Start_Time         : %s (%d)",          scan_Start_Time_Qt().toString().toAscii().constData(), m_pData->header.Scan_Start_Time);
  D("Isotope Name            : %s",                 m_pData->header.Isotope_Name);
  D("Isotope Halflife        : %f sec",          m_pData->header.Isotope_Halflife);
  D("Radiopharmaca           : %s",                m_pData->header.Radiopharmaceutical);
  D("Gantry Tilt             : %f°",            m_pData->header.Gantry_Tilt);
  D("Gantry Rotation         : %f°",              m_pData->header.Gantry_Rotation);
  D("Bed elevation           : %f cm",            m_pData->header.Bed_Elevation);
  D("Intrinsic Tilt          : %f°",            m_pData->header.Intrinsic_Tilt);
  D("Wobble Speed            : %d r/min",        m_pData->header.Wobble_Speed);
  D("TransmSrcType           : %d",              m_pData->header.Transm_Source_Type);
  D("Distance Scanned        : %d cm",          m_pData->header.Distance_Scanned);
  D("Transaxial FOV          : %d cm",          m_pData->header.Transaxial_FOV);
  D("Angular Compression     : %d",              m_pData->header.Angular_Compression);
  D("Coin Samp Mode          : %d",              m_pData->header.Coin_Samp_Mode);
  D("Axial Samp Mode         : %d",              m_pData->header.Axial_Samp_Mode);
  D("Calibration Factor      : %f",              m_pData->header.Calibration_Factor);
  D("Calibration Units       : %d",              m_pData->header.Calibration_Units);
  D("Calibration Units Label : %d",              m_pData->header.Calibration_Units_Label);
  D("Compression Code        : %d",              m_pData->header.Compression_Code);
  D("Studytype               : %s",              m_pData->header.Study_Type);
  D("Patient ID              : %s",              m_pData->header.Patient_ID);
  D("Patient Name            : %s",              m_pData->header.Patient_Name);
  D("Patient Sex             : %c",              m_pData->header.Patient_Sex[0]);
  D("Patient Dexterity       : %c",              m_pData->header.Patient_Dexterity[0]);
  D("Patient Age             : %f years",        m_pData->header.Patient_Age);
  D("Patient Height          : %f cm",          m_pData->header.Patient_Height);
  D("Patient Weight          : %f kg",          m_pData->header.Patient_Weight);
  D("Patient Birthdate       : %s (%d)",        patient_Birth_Date_Qt().toString().toAscii().constData(), m_pData->header.Patient_Birth_Date);
  D("Physician Name          : %s",              m_pData->header.Physician_Name);
  D("Operator Name           : %s",              m_pData->header.Operator_Name);
  D("Study Description       : %s",              m_pData->header.Study_Description);
  D("Acquisition Type        : %d",              m_pData->header.Acquisition_Type);
  D("Patient Orientation     : %d",              m_pData->header.Patient_Orientation);
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
  D("Plane seperation        : %f cm",          m_pData->header.Plane_Separation);
  D("Lowest Threshold Scatter: %d KeV",          m_pData->header.Lwr_Sctr_Thres);
  D("Lower Threshold Trues   : %d KeV",          m_pData->header.Lwr_True_Thres);
  D("Upper Threshold Trues   : %d KeV",          m_pData->header.Upr_True_Thres);
  D("User Process Code       : %s",              m_pData->header.User_Process_Code);
  D("Acquisition Mode        : %d",              m_pData->header.Acquisition_Mode);
  D("Bin Size                : %f cm",          m_pData->header.Bin_Size);
  D("Branching Fraction      : %f",              m_pData->header.Branching_Fraction);
  D("Dose start time         : %s (%d)",        dose_Start_Time_Qt().toString().toAscii().constData(), m_pData->header.Dose_Start_Time);
  D("Dosage                  : %f Bq/cc",        m_pData->header.Dosage);
  D("Well counter corr factor: %f",              m_pData->header.Well_Counter_Corr_Factor);
  D("Data units              : %s",              m_pData->header.Data_Units);
  D("Septa state             : %d",              m_pData->header.Septa_State);
  D("CTI reserved            : %lx",            m_pData->header.CTI_Reserved);
#endif

  RETURN(true);
  return true;
}

QTextStream& operator>>(QTextStream& stream, CECAT7MainHeader& mHeader)
{
  ENTER();
  bool success = true;
  
  // lets iterate through the textstream until
  // we are at the end.
  QString buf;
  while(stream.atEnd() == false)
  {
    // read in the information line by line
    buf = stream.readLine();
    if(buf.isEmpty() == false)
    {
      // read the first word of the line and compare
      // it to existing data we can set within our header
      QString typeString = buf.section(" ", 0, 0);
      QString dataString = buf.section(" ", 1);
      bool convertSuccess = true;

      if(dataString.isEmpty())
        dataString = "";

      // lets check if the typeString matches one of our known
      // types
      if(typeString == "MAGIC_NUMBER")
        strncpy(mHeader.m_pData->header.Magic_Number, dataString.toAscii(), 14);
      else if(typeString == "ORIGINAL_FILE_NAME")
        strncpy(mHeader.m_pData->header.Original_File_Name, dataString.toAscii(), 32);
      else if(typeString == "SW_VERSION")
        mHeader.m_pData->header.SW_Version = dataString.toShort(&convertSuccess);
      else if(typeString == "SYSTEM_TYPE")
        mHeader.m_pData->header.System_Type = dataString.toShort(&convertSuccess);
      else if(typeString == "FILE_TYPE")
        mHeader.m_pData->header.File_Type = dataString.toShort(&convertSuccess);
      else if(typeString == "SERIAL_NUMBER")
        strncpy(mHeader.m_pData->header.Serial_Number, dataString.toAscii(), 10);
      else if(typeString == "SCAN_START_TIME")
        mHeader.m_pData->header.Scan_Start_Time = dataString.toInt(&convertSuccess);
      else if(typeString == "ISOTOPE_NAME")
        strncpy(mHeader.m_pData->header.Isotope_Name, dataString.toAscii(), 8);
      else if(typeString == "ISOTOPE_HALFLIFE")
        mHeader.m_pData->header.Isotope_Halflife = dataString.toFloat(&convertSuccess);
      else if(typeString == "RADIOPHARMACEUTICAL")
        strncpy(mHeader.m_pData->header.Radiopharmaceutical, dataString.toAscii(), 32);
      else if(typeString == "GANTRY_TILT")
        mHeader.m_pData->header.Gantry_Tilt = dataString.toFloat(&convertSuccess);
      else if(typeString == "ROTATION_TILT")
        mHeader.m_pData->header.Gantry_Rotation = dataString.toFloat(&convertSuccess);
      else if(typeString == "BED_ELEVATION")
        mHeader.m_pData->header.Bed_Elevation = dataString.toFloat(&convertSuccess);
      else if(typeString == "INTRINSIC_TILT")
        mHeader.m_pData->header.Intrinsic_Tilt = dataString.toFloat(&convertSuccess);
      else if(typeString == "WOBBLE_SPEED")
        mHeader.m_pData->header.Wobble_Speed = dataString.toShort(&convertSuccess);
      else if(typeString == "TRANSM_SOURCE_TYPE")
        mHeader.m_pData->header.Transm_Source_Type = dataString.toShort(&convertSuccess);
      else if(typeString == "DISTANCE_SCANNED")
        mHeader.m_pData->header.Distance_Scanned = dataString.toFloat(&convertSuccess);
      else if(typeString == "TRANSAXIAL_FOV")
        mHeader.m_pData->header.Transaxial_FOV = dataString.toFloat(&convertSuccess);
      else if(typeString == "ANGULAR_COMPRESSION")
        mHeader.m_pData->header.Angular_Compression = dataString.toShort(&convertSuccess);
      else if(typeString == "COIN_SAMP_MODE")
        mHeader.m_pData->header.Coin_Samp_Mode = dataString.toShort(&convertSuccess);
      else if(typeString == "AXIAL_SAMP_MODE")
        mHeader.m_pData->header.Axial_Samp_Mode = dataString.toShort(&convertSuccess);
      else if(typeString == "ECAT_CALIBRATION_FACTOR")
        mHeader.m_pData->header.Calibration_Factor = dataString.toFloat(&convertSuccess);
      else if(typeString == "CALIBRATION_UNITS")
        mHeader.m_pData->header.Calibration_Units = dataString.toShort(&convertSuccess);
      else if(typeString == "CALIBRATION_UNITS_LABEL")
        mHeader.m_pData->header.Calibration_Units_Label = dataString.toShort(&convertSuccess);
      else if(typeString == "COMPRESSION_CODE")
        mHeader.m_pData->header.Compression_Code = dataString.toShort(&convertSuccess);
      else if(typeString == "STUDY_TYPE")
        strncpy(mHeader.m_pData->header.Study_Type, dataString.toAscii(), 12);
      else if(typeString == "PATIENT_ID")
        strncpy(mHeader.m_pData->header.Patient_ID, dataString.toAscii(), 16);
      else if(typeString == "PATIENT_NAME")
        strncpy(mHeader.m_pData->header.Patient_Name, dataString.toAscii(), 32);
      else if(typeString == "PATIENT_SEX")
        mHeader.m_pData->header.Patient_Sex[0] = dataString.toAscii()[0];
      else if(typeString == "PATIENT_DEXTERITY")
        mHeader.m_pData->header.Patient_Dexterity[0] = dataString.toAscii()[0];
      else if(typeString == "PATIENT_AGE")
        mHeader.m_pData->header.Patient_Age = dataString.toFloat(&convertSuccess);
      else if(typeString == "PATIENT_HEIGHT")
        mHeader.m_pData->header.Patient_Height = dataString.toFloat(&convertSuccess);
      else if(typeString == "PATIENT_WEIGHT")
        mHeader.m_pData->header.Patient_Weight = dataString.toFloat(&convertSuccess);
      else if(typeString == "PATIENT_BIRTH_DATE")
        mHeader.m_pData->header.Patient_Birth_Date = dataString.toInt(&convertSuccess);
      else if(typeString == "PHYSICIAN_NAME")
        strncpy(mHeader.m_pData->header.Physician_Name, dataString.toAscii(), 32);
      else if(typeString == "OPERATOR_NAME")
        strncpy(mHeader.m_pData->header.Operator_Name, dataString.toAscii(), 32);
      else if(typeString == "STUDY_DESCRIPTION")
        strncpy(mHeader.m_pData->header.Study_Description, dataString.toAscii(), 32);
      else if(typeString == "ACQUISITION_TYPE")
        mHeader.m_pData->header.Acquisition_Type = dataString.toShort(&convertSuccess);
      else if(typeString == "PATIENT_ORIENTATION")
        mHeader.m_pData->header.Patient_Orientation = dataString.toShort(&convertSuccess);
      else if(typeString == "FACILITY_NAME")
        strncpy(mHeader.m_pData->header.Facility_Name, dataString.toAscii(), 20);
      else if(typeString == "NUM_PLANES")
        mHeader.m_pData->header.Num_Planes = dataString.toShort(&convertSuccess);
      else if(typeString == "NUM_FRAMES")
        mHeader.m_pData->header.Num_Frames = dataString.toShort(&convertSuccess);
      else if(typeString == "NUM_GATES")
        mHeader.m_pData->header.Num_Gates = dataString.toShort(&convertSuccess);
      else if(typeString == "NUM_BED_POS")
        mHeader.m_pData->header.Num_Bed_Pos = dataString.toShort(&convertSuccess);
      else if(typeString == "INIT_BED_POSITION")
        mHeader.m_pData->header.Init_Bed_Position = dataString.toFloat(&convertSuccess);
      else if(typeString == "BED_POSITION")
      {
        for(int i=0; i < 15 && convertSuccess; i++)
        {
          QString subString = dataString.section(" ", i, i);
          if(subString.isEmpty())
            break;

          mHeader.m_pData->header.Bed_Offset[i] = subString.toFloat(&convertSuccess);
        }
      }
      else if(typeString == "PLANE_SEPARATION")
        mHeader.m_pData->header.Plane_Separation = dataString.toFloat(&convertSuccess);
      else if(typeString == "LWR_SCTR_THRES")
        mHeader.m_pData->header.Lwr_Sctr_Thres = dataString.toShort(&convertSuccess);
      else if(typeString == "LWR_TRUE_THRES")
        mHeader.m_pData->header.Lwr_True_Thres = dataString.toShort(&convertSuccess);
      else if(typeString == "UPR_TRUE_THRES")
        mHeader.m_pData->header.Upr_True_Thres = dataString.toShort(&convertSuccess);
      else if(typeString == "USER_PROCESS_CODE")
        strncpy(mHeader.m_pData->header.User_Process_Code, dataString.toAscii(), 10);
      else if(typeString == "ACQUISITION_MODE")
        mHeader.m_pData->header.Acquisition_Mode = dataString.toShort(&convertSuccess);
      else if(typeString == "BIN_SIZE")
        mHeader.m_pData->header.Bin_Size = dataString.toFloat(&convertSuccess);
      else if(typeString == "BRANCHING_FRACTION")
        mHeader.m_pData->header.Branching_Fraction = dataString.toFloat(&convertSuccess);
      else if(typeString == "DOSE_START_TIME")
        mHeader.m_pData->header.Dose_Start_Time = dataString.toInt(&convertSuccess);
      else if(typeString == "DOSAGE")
        mHeader.m_pData->header.Dosage = dataString.toFloat(&convertSuccess);
      else if(typeString == "WELL_COUNTER_CORR_FACTOR")
        mHeader.m_pData->header.Well_Counter_Corr_Factor = dataString.toFloat(&convertSuccess);
      else if(typeString == "DATA_UNITS")
        strncpy(mHeader.m_pData->header.Data_Units, dataString.toAscii(), 32);
      else if(typeString == "SEPTA_STATE")
        mHeader.m_pData->header.Septa_State = dataString.toShort(&convertSuccess);
      else if(typeString == "FILL")
      {
        for(int i=0; i < 6 && convertSuccess; i++)
        {
          QString subString = dataString.section(" ", i, i);
          if(subString.isEmpty())
            break;

          mHeader.m_pData->header.CTI_Reserved[i] = subString.toShort(&convertSuccess);
        }
      }

      if(convertSuccess == false)
      {
        E("'%s' - error while converting string '%s' to a numerical value.", typeString.toAscii().constData(), dataString.toAscii().constData());
        success = false;
      }
    }
  }

  RETURN(&stream);
  return stream;
}

bool CECAT7MainHeader::save(void) const
{
  ENTER();

  SHOWPOINTER(m_pMedIOData);

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
  // Please note that we do not specify any PLANES here as in ECAT7 the
  // planes are normally integrated in one matrix file. So we have to
  // allow the user to specify the planes himself.
  CECATFile* ecatFile = static_cast<CECATFile*>(m_pMedIOData);
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
  
  // we now read out the header information stepwise
  // as we have to care about big/little endianess, which
  // is automatically done by the QT framework
  stream.writeRawData(&m_pData->header.Magic_Number[0], 14);        //   0: Magic_Number
  stream.writeRawData(&m_pData->header.Original_File_Name[0], 32);  //  14: Original_File_Name
  stream << m_pData->header.SW_Version;                              //  46: SW_Version
  stream << m_pData->header.System_Type;                             //  48: System_Type
  stream << m_pData->header.File_Type;                               //  50: File_Type
  stream.writeRawData(&m_pData->header.Serial_Number[0], 10);       //  52: Original_File_Name
  stream << m_pData->header.Scan_Start_Time;                         //  62: Scan_Start_Time
  stream.writeRawData(&m_pData->header.Isotope_Name[0], 8);         //  66: Isotope_Name
  stream << m_pData->header.Isotope_Halflife;                        //  74: Isotope_Halflife
  stream.writeRawData(&m_pData->header.Radiopharmaceutical[0], 32);  //  78: Radiopharmaceutical
  stream << m_pData->header.Gantry_Tilt;                              // 110: Gantry_Tilt
  stream << m_pData->header.Gantry_Rotation;                          // 114: Gantry_Rotation
  stream << m_pData->header.Bed_Elevation;                            // 118: Bed_Elevation
  stream << m_pData->header.Intrinsic_Tilt;                          // 122: Intrinsic_Tilt
  stream << m_pData->header.Wobble_Speed;                            // 126: Wobble_Speed
  stream << m_pData->header.Transm_Source_Type;                      // 128: Transm_Source_Type
  stream << m_pData->header.Distance_Scanned;                        // 130: Distance_Scanned
  stream << m_pData->header.Transaxial_FOV;                          // 134: Transaxial_FOV
  stream << m_pData->header.Angular_Compression;                      // 138: Angular_Compression
  stream << m_pData->header.Coin_Samp_Mode;                          // 140: Coin_Samp_Mode
  stream << m_pData->header.Axial_Samp_Mode;                          // 142: Axial_Samp_Mode
  stream << m_pData->header.Calibration_Factor;                      // 144: Calibration_Factor
  stream << m_pData->header.Calibration_Units;                        // 148: Calibration_Units
  stream << m_pData->header.Calibration_Units_Label;                  // 150: Calibration_Units_Label
  stream << m_pData->header.Compression_Code;                        // 152: Compression_Code
  stream.writeRawData(&m_pData->header.Study_Type[0], 12);          // 154: Study_Type
  stream.writeRawData(&m_pData->header.Patient_ID[0], 16);          // 166: Patient_ID
  stream.writeRawData(&m_pData->header.Patient_Name[0], 32);        // 182: Patient_Name
  stream.writeRawData(&m_pData->header.Patient_Sex[0], 1);          // 214: Patient_Sex
  stream.writeRawData(&m_pData->header.Patient_Dexterity[0], 1);    // 215: Patient_Dexterity
  stream << m_pData->header.Patient_Age;                              // 216: Patient_Age
  stream << m_pData->header.Patient_Height;                          // 220: Patient_Height
  stream << m_pData->header.Patient_Weight;                          // 224: Patient_Weight
  stream << m_pData->header.Patient_Birth_Date;                      // 228: Patient_Birth_Date
  stream.writeRawData(&m_pData->header.Physician_Name[0], 32);      // 232: Physician_Name
  stream.writeRawData(&m_pData->header.Operator_Name[0], 32);        // 264: Operator_Name
  stream.writeRawData(&m_pData->header.Study_Description[0], 32);    // 296: Study_Description
  stream << m_pData->header.Acquisition_Type;                        // 328: Acquisition_Type
  stream << m_pData->header.Patient_Orientation;                      // 330: Patient_Orientation
  stream.writeRawData(&m_pData->header.Facility_Name[0], 20);        // 332: Facility_Name
  stream << m_pData->header.Num_Planes;                              // 352: Num_Planes
  stream << numFrames;                                      // 354: Num_Frames
  stream << numGates;                                        // 356: Num_Gates
  stream << numBedPos;                                      // 358: Num_Bed_Pos
  stream << m_pData->header.Init_Bed_Position;                        // 360: Init_Bed_Position
  stream << m_pData->header.Bed_Offset[0];                            // 364: Bed_Offset 1
  stream << m_pData->header.Bed_Offset[1];                            // 368: Bed_Offset 2
  stream << m_pData->header.Bed_Offset[2];                            // 372: Bed_Offset 3
  stream << m_pData->header.Bed_Offset[3];                            // 376: Bed_Offset 4
  stream << m_pData->header.Bed_Offset[4];                            // 380: Bed_Offset 5
  stream << m_pData->header.Bed_Offset[5];                            // 384: Bed_Offset 6
  stream << m_pData->header.Bed_Offset[6];                            // 388: Bed_Offset 7
  stream << m_pData->header.Bed_Offset[7];                            // 392: Bed_Offset 8
  stream << m_pData->header.Bed_Offset[8];                            // 396: Bed_Offset 9
  stream << m_pData->header.Bed_Offset[9];                            // 400: Bed_Offset 10
  stream << m_pData->header.Bed_Offset[10];                          // 404: Bed_Offset 11
  stream << m_pData->header.Bed_Offset[11];                          // 408: Bed_Offset 12
  stream << m_pData->header.Bed_Offset[12];                          // 412: Bed_Offset 13
  stream << m_pData->header.Bed_Offset[13];                          // 416: Bed_Offset 14
  stream << m_pData->header.Bed_Offset[14];                          // 420: Bed_Offset 15
  stream << m_pData->header.Plane_Separation;                        // 424: Plane_Separation
  stream << m_pData->header.Lwr_Sctr_Thres;                          // 428: Lwr_Sctr_Thres
  stream << m_pData->header.Lwr_True_Thres;                          // 430: Lwr_True_Thres
  stream << m_pData->header.Upr_True_Thres;                          // 432: Upr_True_Thres
  stream.writeRawData(&m_pData->header.User_Process_Code[0], 10);    // 434: User_Process_Code
  stream << m_pData->header.Acquisition_Mode;                        // 444: Acquisition_Mode
  stream << m_pData->header.Bin_Size;                                // 446: Bin_Size
  stream << m_pData->header.Branching_Fraction;                      // 450: Branching_Fraction
  stream << m_pData->header.Dose_Start_Time;                          // 454: Dose_Start_Time
  stream << m_pData->header.Dosage;                                  // 458: Dosage
  stream << m_pData->header.Well_Counter_Corr_Factor;                // 462: Well_Counter_Corr_Factor
  stream.writeRawData(&m_pData->header.Data_Units[0], 32);          // 466: Data_Units
  stream << m_pData->header.Septa_State;                              // 498: Septa_State
  stream << m_pData->header.CTI_Reserved[0];                          // 500: CTI_Reserved 1
  stream << m_pData->header.CTI_Reserved[1];                          // 502: CTI_Reserved 2
  stream << m_pData->header.CTI_Reserved[2];                          // 504: CTI_Reserved 3
  stream << m_pData->header.CTI_Reserved[3];                          // 506: CTI_Reserved 4
  stream << m_pData->header.CTI_Reserved[4];                          // 508: CTI_Reserved 5
  stream << m_pData->header.CTI_Reserved[5];                          // 510: CTI_Reserved 6

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

int CECAT7MainHeader::rawDataSize() const
{ 
  return 1*ECAT_BLOCKSIZE;
}

int CECAT7MainHeader::rtti() const
{ 
  return CECATMainHeader::ECAT7MainHeader;
}

bool CECAT7MainHeader::convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader* pHead2) 
{
  ENTER();
  bool bResult = false;

  // depending on the MedIOHeader format we do have to 
  // distinguish between our copy operations.
  switch(pHead1->headerFormat())
  {
    case CMedIOHeader::ECATMainHeader:
    {
      const CECATMainHeader* eMainHeader = static_cast<const CECATMainHeader*>(pHead1);

      // depending on the source type we have to copy either every data or just 
      // some data of the src header
      switch(eMainHeader->rtti())
      {
        // if the source header is also an ECAT7 one we can copy it in whole
        // via a simple memcpy()
        case CECATMainHeader::ECAT7MainHeader:
        {
          // we use the assignment operator which will do the convertation
          // for us.
          *this = *static_cast<const CECAT7MainHeader*>(pHead1);
          
          bResult = true;
        }
        break;

        // if this is an ECAT6 Mainheader we have to take care of the fact that
        // some information is missing in one of the headers.
        case CECATMainHeader::ECAT6MainHeader:
        {
          const CECAT6MainHeader* e6src = static_cast<const CECAT6MainHeader*>(pHead1);
          clear();
          setOriginal_File_Name(e6src->original_File_Name());
          setSystem_Type(e6src->system_Type());

          #warning "ECAT6->ECAT7 copy not fully implemented yet!"
          bResult = true;
        }
        break;
      }
      
      // afterwards we have to make sure sensible data is restored
      m_pData->header.SW_Version = 72; // This header does conform to the ECAT 7.2 standard
    }

    case CMedIOHeader::ECATSubHeader:
    case CMedIOHeader::ConcordeMicroPetFrameHeader:
    case CMedIOHeader::PhilipsSubHeader:
      // copying a sub header into a main header doesn't make much sense, so we
      // do nothing here
    break;

    case CMedIOHeader::ConcordeMicroPetMainHeader:
    {
      // we have to save the filetype
      CECAT7MainHeader::File_Type ft = file_Type();
      const CConcordeMainHeader* head = static_cast<const CConcordeMainHeader*>(pHead1);
      clear();
      // now set the old filetype again
      setFile_Type(ft);
      setOriginal_File_Name(QFileInfo(head->fileName()).fileName().toAscii());
      setSystem_Type(static_cast<short>(head->model()));
      setScan_Start_Time(head->scanTime());
      setIsotope_Name(head->isotope().toAscii());
      setIsotope_Halflife(head->isotopeHalfTime());
      setRadiopharmaceutical(head->injectedCompound().toAscii());
      
      setDistance_Scanned(head->radialFov());
      setTransaxial_FOV(head->radialFov());
      setCoin_Samp_Mode(CECAT7MainHeader::NetTrues);
      setCalibration_Factor(head->calibrationFactor());

      setStudy_Type(head->study().toAscii());
      setPatient_ID(head->subjectIdentifier().toAscii());
      setPatient_Name(head->subjectIdentifier().toAscii());
      setPatient_Sex(m_pData->concorde2ECAT7Sex(head->subjectGenus()));
      
      setPatient_Height(m_pData->concorde2ECAT7Height(head->subjectLength(), head->subjectLengthUnits()));
      setPatient_Weight(m_pData->concorde2ECAT7Weight(head->subjectWeight(), head->subjectWeightUnits()));
      
      setPhysician_Name(head->investigatorName().toAscii());
      setOperator_Name(head->operatorName().toAscii());
      setStudy_Description(head->study().toAscii());
      if(head->acquisitionMode() == CConcordeMainHeader::Emission)
        setAcquisition_Type(CECAT7MainHeader::StaticEmission);
      else
        setAcquisition_Type(CECAT7MainHeader::DynamicEmission);
      setPatient_Orientation((m_pData->concorde2ECAT7Orientation(head->subjectOrientation())));
      setFacility_Name(head->institution().toAscii());
      setNum_Planes(head->zDimension());
      setNum_Frames(head->totalFrames());
      setNum_Gates(1);
      setNum_Bed_Pos(0);
      setPlane_Separation(head->axialPlaneSize());
      setLwr_True_Thres(static_cast<short>(head->lld()));
      setUpr_True_Thres(static_cast<short>(head->uld()));
      setBranching_Fraction(head->isotopeBranchingFraction());

      setDose_Start_Time(head->injectionTime());
      setDosage(m_pData->concorde2ECAT7dosage(head->dose(), head->doseUnits()));
      setData_Units(m_pData->concorde2ECAT7dataUnits(head->calibrationUnits()).toAscii());
      setCalibration_Units(m_pData->concorde2ECAT7calibrationUnits(head->calibrationUnits()));
      
      //check if additional information is available
      if(pHead2)
      {
        switch(pHead2->headerFormat())
        {
          case CMedIOHeader::ConcordeMicroPetFrameHeader:
          {
            D("Setting additional information to ECAT7 main header");
            const CConcordeFrameHeader* frame = static_cast<const CConcordeFrameHeader*>(pHead2);
            
            setInit_Bed_Position(frame->bedOffset());
            setBed_Elevation(frame->verticalBedOffset());
          };break;
          default:break;
        }
      }
      bResult = true;
    }
    break;

    case CMedIOHeader::PhilipsMainHeader:
    {
      std::cout << "in philismainheader conversion." << std::endl;
      // we have to save the filetype
      CECAT7MainHeader::File_Type ft = file_Type();
      const CPhilipsMainHeader* head = static_cast<const CPhilipsMainHeader*>(pHead1);
      clear();
      // now set the old filetype again
      setFile_Type(ft);

      setPatient_Name(head->patient_Name());
      setPatient_Weight(head->weight() / 1000.0f); // g -> kg

      setNum_Planes(head->nslice());
      setNum_Frames(head->nframe());
      setNum_Gates(1);
      setNum_Bed_Pos(0);
      setPlane_Separation(head->slcthk() / 10.0f); // mm -> cm

      setIsotope_Halflife(head->halfLife() * 60.0f); // min -> sec
      setDosage(head->activity() * 1000000.0f);      // MBq -> Bq

      setCalibration_Units(CECAT7MainHeader::Uncalibrated);
      setCalibration_Factor(1.0f);
      setData_Units("BQML");
      
      //check if additional information is available
      if(pHead2)
      {
        switch(pHead2->headerFormat())
        {
          case CMedIOHeader::PhilipsExtendedMainHeader:
          {
            D("Setting additional information to ECAT7 main header");
            const CPhilipsExtendedMainHeader* extHeader = static_cast<const CPhilipsExtendedMainHeader*>(pHead2);

            setPatient_Sex(m_pData->philips2Ecat7Sex(extHeader->sex()));
            setPatient_ID(extHeader->Dpat_id());
            
          };break;
          default:break;
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

CMedIOHeader* CECAT7MainHeader::clone() const
{
  ENTER();

  CECAT7MainHeader* pNewHeader = new CECAT7MainHeader(*this);

  RETURN(pNewHeader);
  return pNewHeader;
}

const char* CECAT7MainHeader::magic_Number(void) const
{
  return m_pData->header.Magic_Number;
}

const char* CECAT7MainHeader::original_File_Name(void) const
{
  return m_pData->header.Original_File_Name;
}

short CECAT7MainHeader::sw_Version(void) const
{
  return m_pData->header.SW_Version;
}

short CECAT7MainHeader::system_Type(void) const
{
  return m_pData->header.System_Type;
}

CECAT7MainHeader::File_Type CECAT7MainHeader::file_Type(void)  const
{
  return static_cast<CECAT7MainHeader::File_Type>(m_pData->header.File_Type);
}

const char* CECAT7MainHeader::serial_Number(void) const
{
  return m_pData->header.Serial_Number;
}

time_t CECAT7MainHeader::scan_Start_Time(void) const
{
  return m_pData->header.Scan_Start_Time;
}

const char* CECAT7MainHeader::isotope_Name(void) const
{
  return m_pData->header.Isotope_Name;
}

float CECAT7MainHeader::isotope_Halflife(void) const
{
  return m_pData->header.Isotope_Halflife;
}

const char* CECAT7MainHeader::radiopharmaceutical(void) const
{
  return m_pData->header.Radiopharmaceutical;
}

float CECAT7MainHeader::gantry_Tilt(void) const
{
  return m_pData->header.Gantry_Tilt;
}

float CECAT7MainHeader::gantry_Rotation(void) const
{
  return m_pData->header.Gantry_Rotation;
}

float CECAT7MainHeader::bed_Elevation(void) const
{
  return m_pData->header.Bed_Elevation;
}

float CECAT7MainHeader::intrinsic_Tilt(void) const
{
  return m_pData->header.Intrinsic_Tilt;
}

short CECAT7MainHeader::wobble_Speed(void) const
{
  return m_pData->header.Wobble_Speed;
}

CECAT7MainHeader::Transm_Source_Type CECAT7MainHeader::transm_Source_Type(void) const
{
  return static_cast<CECAT7MainHeader::Transm_Source_Type>(m_pData->header.Transm_Source_Type);
}

float CECAT7MainHeader::distance_Scanned(void) const                          
{
  return m_pData->header.Distance_Scanned;
}

float CECAT7MainHeader::transaxial_FOV(void) const                            
{
  return m_pData->header.Transaxial_FOV;
}

CECAT7MainHeader::Angular_Compression CECAT7MainHeader::angular_Compression(void) const          
{
  return static_cast<CECAT7MainHeader::Angular_Compression>(m_pData->header.Angular_Compression);
}

CECAT7MainHeader::Coin_Samp_Mode CECAT7MainHeader::coin_Samp_Mode(void) const                    
{
  return static_cast<CECAT7MainHeader::Coin_Samp_Mode>(m_pData->header.Coin_Samp_Mode);
}

CECAT7MainHeader::Axial_Samp_Mode CECAT7MainHeader::axial_Samp_Mode(void) const                  
{
  return static_cast<CECAT7MainHeader::Axial_Samp_Mode>(m_pData->header.Axial_Samp_Mode);
}

float CECAT7MainHeader::calibration_Factor(void) const                        
{
  return m_pData->header.Calibration_Factor;
}

CECAT7MainHeader::Calibration_Units CECAT7MainHeader::calibration_Units(void) const              
{
  return static_cast<CECAT7MainHeader::Calibration_Units>(m_pData->header.Calibration_Units);
}

CECAT7MainHeader::Calibration_Units_Label CECAT7MainHeader::calibration_Units_Label(void) const  
{
  return static_cast<CECAT7MainHeader::Calibration_Units_Label>(m_pData->header.Calibration_Units_Label);
}

CECAT7MainHeader::Compression_Code CECAT7MainHeader::compression_Code(void) const              
{
  return static_cast<CECAT7MainHeader::Compression_Code>(m_pData->header.Compression_Code);
}

const char* CECAT7MainHeader::study_Type(void) const                        
{
  return m_pData->header.Study_Type;
}

const char* CECAT7MainHeader::patient_ID(void) const                          
{
  return m_pData->header.Patient_ID;
}

const char* CECAT7MainHeader::patient_Name(void) const                        
{
  return m_pData->header.Patient_Name;
}

CECAT7MainHeader::Patient_Sex CECAT7MainHeader::patient_Sex(void) const                          
{
  return static_cast<CECAT7MainHeader::Patient_Sex>(m_pData->header.Patient_Sex[0]);
}

CECAT7MainHeader::Patient_Dexterity CECAT7MainHeader::patient_Dexterity(void) const              
{
  return static_cast<CECAT7MainHeader::Patient_Dexterity>(m_pData->header.Patient_Dexterity[0]);
}

float CECAT7MainHeader::patient_Age(void) const                                
{
  return m_pData->header.Patient_Age;
}

float CECAT7MainHeader::patient_Height(void) const                            
{
  return m_pData->header.Patient_Height;
}

float CECAT7MainHeader::patient_Weight(void) const                            
{
  return m_pData->header.Patient_Weight;
}

time_t CECAT7MainHeader::patient_Birth_Date(void) const                
{
  return m_pData->header.Patient_Birth_Date;
}

const char* CECAT7MainHeader::physician_Name(void) const                    
{
  return m_pData->header.Physician_Name;
}

const char* CECAT7MainHeader::operator_Name(void) const                        
{
  return m_pData->header.Operator_Name;
}

const char* CECAT7MainHeader::study_Description(void) const                    
{
  return m_pData->header.Study_Description;
}

CECAT7MainHeader::Acquisition_Type CECAT7MainHeader::acquisition_Type(void) const              
{
  return static_cast<CECAT7MainHeader::Acquisition_Type>(m_pData->header.Acquisition_Type);
}

CECAT7MainHeader::Patient_Orientation CECAT7MainHeader::patient_Orientation(void) const                      
{
  return static_cast<CECAT7MainHeader::Patient_Orientation>(m_pData->header.Patient_Orientation);
}

const char* CECAT7MainHeader::facility_Name(void) const                      
{
  return m_pData->header.Facility_Name;
}

short CECAT7MainHeader::num_Planes(void) const                              
{
  return m_pData->header.Num_Planes;
}

short CECAT7MainHeader::num_Frames(void) const                              
{
  return m_pData->header.Num_Frames;
}

short CECAT7MainHeader::num_Gates(void) const                                
{
  return m_pData->header.Num_Gates;
}

short CECAT7MainHeader::num_Bed_Pos(void) const                              
{
  return m_pData->header.Num_Bed_Pos;
}

float CECAT7MainHeader::init_Bed_Position(void) const                        
{
  return m_pData->header.Init_Bed_Position;
}

float CECAT7MainHeader::bed_Offset(const short i) const                      
{
  return m_pData->header.Bed_Offset[i];
}

float CECAT7MainHeader::plane_Separation(void) const                        
{
  return m_pData->header.Plane_Separation;
}

short CECAT7MainHeader::lwr_Sctr_Thres(void) const                          
{
  return m_pData->header.Lwr_Sctr_Thres;
}

short CECAT7MainHeader::lwr_True_Thres(void) const                          
{
  return m_pData->header.Lwr_True_Thres;
}

short CECAT7MainHeader::upr_True_Thres(void) const                          
{
  return m_pData->header.Upr_True_Thres;
}

const char* CECAT7MainHeader::user_Process_Code(void) const                  
{
  return m_pData->header.User_Process_Code;
}

CECAT7MainHeader::Acquisition_Mode CECAT7MainHeader::acquisition_Mode(void) const              
{
  return static_cast<CECAT7MainHeader::Acquisition_Mode>(m_pData->header.Acquisition_Mode);
}

float CECAT7MainHeader::bin_Size(void) const                                
{
  return m_pData->header.Bin_Size;   }

float CECAT7MainHeader::branching_Fraction(void) const                      
{
  return m_pData->header.Branching_Fraction;
}

time_t CECAT7MainHeader::dose_Start_Time(void) const                  
{
  return m_pData->header.Dose_Start_Time;
}

float CECAT7MainHeader::dosage(void) const                                  
{
  return m_pData->header.Dosage;
}

float CECAT7MainHeader::well_Counter_Corr_Factor(void) const                
{
  return m_pData->header.Well_Counter_Corr_Factor;
}

const char* CECAT7MainHeader::data_Units(void) const                        
{
  return m_pData->header.Data_Units;
}

CECAT7MainHeader::Septa_State CECAT7MainHeader::septa_State(void) const                        
{
  return static_cast<CECAT7MainHeader::Septa_State>(m_pData->header.Septa_State);
}

short CECAT7MainHeader::cti_Reserved(const short i) const                    
{
  return m_pData->header.CTI_Reserved[i];
}


// mutator methods
void CECAT7MainHeader::setMagic_Number(const char* mn)
{
  strncpy(m_pData->header.Magic_Number, mn, 14);
}

void CECAT7MainHeader::setOriginal_File_Name(const char* name)
{
  strncpy(m_pData->header.Original_File_Name, name, 32);
}

void CECAT7MainHeader::setSW_Version(const short ver)
{
  m_pData->header.SW_Version = ver;
}

void CECAT7MainHeader::setSystem_Type(const short type)
{
  m_pData->header.System_Type = type;
}

void CECAT7MainHeader::setFile_Type(const File_Type fType)
{
  m_pData->header.File_Type = fType;
  m_pData->updateMagicNumber();
}

void CECAT7MainHeader::setSerial_Number(const char* num)
{
  strncpy(m_pData->header.Serial_Number, num, 10);
}

void CECAT7MainHeader::setScan_Start_Time(const time_t time)
{
  m_pData->header.Scan_Start_Time = static_cast<qint32>(time);
}

void CECAT7MainHeader::setIsotope_Name(const char* name)
{
  strncpy(m_pData->header.Isotope_Name, name, 8);
}

void CECAT7MainHeader::setIsotope_Halflife(const float hlf)                              
{
  m_pData->header.Isotope_Halflife = hlf;
}

void CECAT7MainHeader::setRadiopharmaceutical(const char* str)                          
{
  strncpy(m_pData->header.Radiopharmaceutical, str, 32);
}

void CECAT7MainHeader::setGantry_Tilt(const float tilt)                                  
{
  m_pData->header.Gantry_Tilt = tilt;
}

void CECAT7MainHeader::setGantry_Rotation(const float rot)                              
{
  m_pData->header.Gantry_Rotation = rot;
}

void CECAT7MainHeader::setBed_Elevation(const float elev)                                
{
  m_pData->header.Bed_Elevation = elev;
}

void CECAT7MainHeader::setIntrinsic_Tilt(const float tilt)                              
{
  m_pData->header.Intrinsic_Tilt = tilt;
}

void CECAT7MainHeader::setWobble_Speed(const short speed)                              
{
  m_pData->header.Wobble_Speed = speed;
}

void CECAT7MainHeader::setTransm_Source_Type(const Transm_Source_Type type)            
{
  m_pData->header.Transm_Source_Type = type;
}

void CECAT7MainHeader::setDistance_Scanned(const float dist)                          
{
  m_pData->header.Distance_Scanned = dist;
}

void CECAT7MainHeader::setTransaxial_FOV(const float fov)                                
{
  m_pData->header.Transaxial_FOV = fov;
}

void CECAT7MainHeader::setAngular_Compression(const Angular_Compression comp)            
{
  m_pData->header.Angular_Compression = comp;
}

void CECAT7MainHeader::setCoin_Samp_Mode(const Coin_Samp_Mode mode)
{
  m_pData->header.Coin_Samp_Mode = mode;
}

void CECAT7MainHeader::setAxial_Samp_Mode(const Axial_Samp_Mode mode)                    
{
  m_pData->header.Axial_Samp_Mode = mode;
}

void CECAT7MainHeader::setCalibration_Factor(const float factor)                        
{
  m_pData->header.Calibration_Factor = factor;
}

void CECAT7MainHeader::setCalibration_Units(const Calibration_Units unit)                
{
  m_pData->header.Calibration_Units = unit;
}

void CECAT7MainHeader::setCalibration_Units_Label(const Calibration_Units_Label label)
{
  m_pData->header.Calibration_Units_Label = label;
}

void CECAT7MainHeader::setCompression_Code(const Compression_Code code)  
{
  m_pData->header.Compression_Code = code;
}

void CECAT7MainHeader::setStudy_Type(const char* type)                                  
{
  strncpy(m_pData->header.Study_Type, type, 12);
}

void CECAT7MainHeader::setPatient_ID(const char* id)                                  
{
  strncpy(m_pData->header.Patient_ID, id, 16);
}

void CECAT7MainHeader::setPatient_Name(const char* name)
{
  strncpy(m_pData->header.Patient_Name, name, 32);
}

void CECAT7MainHeader::setPatient_Sex(const Patient_Sex sex)                            
{
  m_pData->header.Patient_Sex[0] = sex;
}

void CECAT7MainHeader::setPatient_Dexterity(const Patient_Dexterity dex)
{
  m_pData->header.Patient_Dexterity[0] = dex;
}

void CECAT7MainHeader::setPatient_Age(const float age)                                
{
  m_pData->header.Patient_Age = age;
}

void CECAT7MainHeader::setPatient_Height(const float height)                            
{
  m_pData->header.Patient_Height = height;
}

void CECAT7MainHeader::setPatient_Weight(const float weight)                          
{
  m_pData->header.Patient_Weight = weight;
}

void CECAT7MainHeader::setPatient_Birth_Date(const time_t date)                    
{
  m_pData->header.Patient_Birth_Date = static_cast<qint32>(date);
}

void CECAT7MainHeader::setPhysician_Name(const char* name)                              
{
  strncpy(m_pData->header.Physician_Name, name, 32);
}

void CECAT7MainHeader::setOperator_Name(const char* name)                              
{
  strncpy(m_pData->header.Operator_Name, name, 32);
}

void CECAT7MainHeader::setStudy_Description(const char* descr)                        
{
  strncpy(m_pData->header.Study_Description, descr, 32);
}

void CECAT7MainHeader::setAcquisition_Type(const Acquisition_Type type)                
{
  m_pData->header.Acquisition_Type = type;
}

void CECAT7MainHeader::setPatient_Orientation(const Patient_Orientation orient)                      
{
  m_pData->header.Patient_Orientation = orient;
}

void CECAT7MainHeader::setFacility_Name(const char* name)                              
{
  strncpy(m_pData->header.Facility_Name, name, 20);
}

void CECAT7MainHeader::setNum_Planes(const short num)                                  
{
  m_pData->header.Num_Planes = num;
}

void CECAT7MainHeader::setNum_Frames(const short num)                                  
{
  m_pData->header.Num_Frames = num;
}

void CECAT7MainHeader::setNum_Gates(const short num)                                  
{
  m_pData->header.Num_Gates = num;
}

void CECAT7MainHeader::setNum_Bed_Pos(const short num)                                
{
  m_pData->header.Num_Bed_Pos = num;
}

void CECAT7MainHeader::setInit_Bed_Position(const float pos)                          
{
  m_pData->header.Init_Bed_Position = pos;
}

void CECAT7MainHeader::setBed_Offset(const short i, const float offset)                
{
  m_pData->header.Bed_Offset[i] = offset;
}

void CECAT7MainHeader::setPlane_Separation(const float sep)                            
{
  m_pData->header.Plane_Separation = sep;
}

void CECAT7MainHeader::setLwr_Sctr_Thres(const short thres)                            
{
  m_pData->header.Lwr_Sctr_Thres = thres;
}

void CECAT7MainHeader::setLwr_True_Thres(const short thres)                            
{
  m_pData->header.Lwr_True_Thres = thres;
}

void CECAT7MainHeader::setUpr_True_Thres(const short thres)                            
{
  m_pData->header.Upr_True_Thres = thres;
}

void CECAT7MainHeader::setUser_Process_Code(const char* code)                          
{
  strncpy(m_pData->header.User_Process_Code, code, 10);
}

void CECAT7MainHeader::setAcquisition_Mode(const Acquisition_Mode mode)                
{
  m_pData->header.Acquisition_Mode = mode;
}

void CECAT7MainHeader::setBin_Size(const float size)                                  
{
  m_pData->header.Bin_Size = size;
}

void CECAT7MainHeader::setBranching_Fraction(const float frac)                        
{
  m_pData->header.Branching_Fraction = frac;
}

void CECAT7MainHeader::setDose_Start_Time(const time_t time)                    
{
  m_pData->header.Dose_Start_Time = static_cast<qint32>(time);
}

void CECAT7MainHeader::setDosage(const float dosage)                                  
{ 
  m_pData->header.Dosage = dosage;
}

void CECAT7MainHeader::setWell_Counter_Corr_Factor(const float fac)                    
{ 
  m_pData->header.Well_Counter_Corr_Factor = fac;
}

void CECAT7MainHeader::setData_Units(const char* units)                                
{ 
  strncpy(m_pData->header.Data_Units, units, 32);
}

void CECAT7MainHeader::setSepta_State(const Septa_State state)                        
{ 
  m_pData->header.Septa_State = state;
}

void CECAT7MainHeader::setCTI_Reserved(const short i, const short val)                
{ 
  m_pData->header.CTI_Reserved[i] = val;
}

QTextStream& operator<<(QTextStream& stream, const CECAT7MainHeader& mHeader)
{
  ENTER();

  // save all header data to the textstream
  stream << qSetRealNumberPrecision(6);
  stream << "MAGIC_NUMBER "              << mHeader.m_pData->header.Magic_Number              << endl;
  stream << "ORIGINAL_FILE_NAME "        << mHeader.m_pData->header.Original_File_Name        << endl;
  stream << "SW_VERSION "                << mHeader.m_pData->header.SW_Version                << endl;
  stream << "SYSTEM_TYPE "              << mHeader.m_pData->header.System_Type                << endl;
  stream << "FILE_TYPE "                << mHeader.m_pData->header.File_Type                  << endl;
  stream << "SERIAL_NUMBER "            << mHeader.m_pData->header.Serial_Number              << endl;
  stream << "SCAN_START_TIME "          << mHeader.m_pData->header.Scan_Start_Time            << endl;
  stream << "ISOTOPE_NAME "              << mHeader.m_pData->header.Isotope_Name              << endl;
  stream << "ISOTOPE_HALFLIFE "          << mHeader.m_pData->header.Isotope_Halflife          << endl;
  stream << "RADIOPHARMACEUTICAL "      << mHeader.m_pData->header.Radiopharmaceutical        << endl;
  stream << "GANTRY_TILT "              << mHeader.m_pData->header.Gantry_Tilt                << endl;
  stream << "ROTATION_TILT "            << mHeader.m_pData->header.Gantry_Rotation            << endl;
  stream << "BED_ELEVATION "            << mHeader.m_pData->header.Bed_Elevation              << endl;
  stream << "INTRINSIC_TILT "            << mHeader.m_pData->header.Intrinsic_Tilt            << endl;
  stream << "WOBBLE_SPEED "              << mHeader.m_pData->header.Wobble_Speed              << endl;
  stream << "TRANSM_SOURCE_TYPE "        << mHeader.m_pData->header.Transm_Source_Type        << endl;
  stream << "DISTANCE_SCANNED "          << mHeader.m_pData->header.Distance_Scanned          << endl;
  stream << "TRANSAXIAL_FOV "            << mHeader.m_pData->header.Transaxial_FOV            << endl;
  stream << "ANGULAR_COMPRESSION "      << mHeader.m_pData->header.Angular_Compression        << endl;
  stream << "COIN_SAMP_MODE "            << mHeader.m_pData->header.Coin_Samp_Mode            << endl;
  stream << "AXIAL_SAMP_MODE "          << mHeader.m_pData->header.Axial_Samp_Mode            << endl;
  stream << "ECAT_CALIBRATION_FACTOR "  << mHeader.m_pData->header.Calibration_Factor        << endl;
  stream << "CALIBRATION_UNITS "        << mHeader.m_pData->header.Calibration_Units          << endl;
  stream << "CALIBRATION_UNITS_LABEL "  << mHeader.m_pData->header.Calibration_Units_Label    << endl;
  stream << "COMPRESSION_CODE "          << mHeader.m_pData->header.Compression_Code          << endl;
  stream << "STUDY_TYPE "                << mHeader.m_pData->header.Study_Type                << endl;
  stream << "PATIENT_ID "                << mHeader.m_pData->header.Patient_ID                << endl;
  stream << "PATIENT_NAME "              << mHeader.m_pData->header.Patient_Name              << endl;
  stream << "PATIENT_SEX "              << mHeader.m_pData->header.Patient_Sex[0]            << endl;
  stream << "PATIENT_DEXTERITY "        << mHeader.m_pData->header.Patient_Dexterity[0]      << endl;
  stream << "PATIENT_AGE "              << mHeader.m_pData->header.Patient_Age                << endl;
  stream << "PATIENT_HEIGHT "            << mHeader.m_pData->header.Patient_Height            << endl;
  stream << "PATIENT_WEIGHT "            << mHeader.m_pData->header.Patient_Weight            << endl;
  stream << "PATIENT_BIRTH_DATE "        << mHeader.m_pData->header.Patient_Birth_Date        << endl;
  stream << "PHYSICIAN_NAME "            << mHeader.m_pData->header.Physician_Name            << endl;
  stream << "OPERATOR_NAME "            << mHeader.m_pData->header.Operator_Name              << endl;
  stream << "STUDY_DESCRIPTION "        << mHeader.m_pData->header.Study_Description          << endl;
  stream << "ACQUISITION_TYPE "          << mHeader.m_pData->header.Acquisition_Type          << endl;
  stream << "PATIENT_ORIENTATION "      << mHeader.m_pData->header.Patient_Orientation        << endl;
  stream << "FACILITY_NAME "            << mHeader.m_pData->header.Facility_Name              << endl;
  stream << "NUM_PLANES "                << mHeader.m_pData->header.Num_Planes                << endl;
  stream << "NUM_FRAMES "                << mHeader.m_pData->header.Num_Frames                << endl;
  stream << "NUM_GATES "                << mHeader.m_pData->header.Num_Gates                  << endl;
  stream << "NUM_BED_POS "              << mHeader.m_pData->header.Num_Bed_Pos                << endl;
  stream << "INIT_BED_POSITION "        << mHeader.m_pData->header.Init_Bed_Position          << endl;
  stream << "BED_POSITION";
  for(int i=0; i < 15; i++)
  {
    stream << " " << mHeader.m_pData->header.Bed_Offset[i];
  }
  stream << endl;
  stream << "PLANE_SEPARATION "          << mHeader.m_pData->header.Plane_Separation          << endl;
  stream << "LWR_SCTR_THRES "            << mHeader.m_pData->header.Lwr_Sctr_Thres            << endl;
  stream << "LWR_TRUE_THRES "            << mHeader.m_pData->header.Lwr_True_Thres            << endl;
  stream <<  "UPR_TRUE_THRES "            << mHeader.m_pData->header.Upr_True_Thres            << endl;
  stream <<  "USER_PROCESS_CODE "        << mHeader.m_pData->header.User_Process_Code          << endl;
  stream << "ACQUISITION_MODE "          << mHeader.m_pData->header.Acquisition_Mode          << endl;
  stream << "BIN_SIZE "                  << mHeader.m_pData->header.Bin_Size                  << endl;
  stream << "BRANCHING_FRACTION "        << mHeader.m_pData->header.Branching_Fraction        << endl;
  stream << "DOSE_START_TIME "          << mHeader.m_pData->header.Dose_Start_Time            << endl;
  stream <<  "DOSAGE "                    << mHeader.m_pData->header.Dosage                    << endl;
  stream <<  "WELL_COUNTER_CORR_FACTOR "  << mHeader.m_pData->header.Well_Counter_Corr_Factor  << endl;
  stream << "DATA_UNITS "                << mHeader.m_pData->header.Data_Units                << endl;
  stream <<  "SEPTA_STATE "              << mHeader.m_pData->header.Septa_State                << endl;
  stream <<  "FILL";
  for(int i=0; i < 6; i++)
  {
    stream << " " << mHeader.m_pData->header.CTI_Reserved[i];
  }
  stream << endl;
  
  RETURN(&stream);
  return stream;
}

QDate CECAT7MainHeader::patient_Birth_Date_Qt(void) const
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  QDateTime birthDate = Jan1970.addSecs(m_pData->header.Patient_Birth_Date);
  return birthDate.toLocalTime().date();
}

QDateTime CECAT7MainHeader::scan_Start_Time_Qt(void) const
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  QDateTime scanStartTime = Jan1970.addSecs(m_pData->header.Scan_Start_Time);
  return scanStartTime.toLocalTime();
}

QDateTime CECAT7MainHeader::dose_Start_Time_Qt(void) const
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  QDateTime doseStartTime = Jan1970.addSecs(m_pData->header.Dose_Start_Time);
  return doseStartTime.toLocalTime();
}

void CECAT7MainHeader::setPatient_Birth_Date_Qt(const QDate& birthDate)
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  SHOWSTRING(birthDate.toString().toAscii().constData());
  m_pData->header.Patient_Birth_Date = Jan1970.secsTo(QDateTime(birthDate, QTime()));
  SHOWVALUE(m_pData->header.Patient_Birth_Date);
}

void CECAT7MainHeader::setScan_Start_Time_Qt(const QDateTime& scanStartTime)
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  m_pData->header.Scan_Start_Time = Jan1970.secsTo(scanStartTime.toUTC());
}

void CECAT7MainHeader::setDose_Start_Time_Qt(const QDateTime& doseStartTime)
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  m_pData->header.Dose_Start_Time = Jan1970.secsTo(doseStartTime.toUTC());
}

QString CECAT7MainHeaderPrivate::concorde2ECAT7dataUnits(CConcordeMainHeader::CalibrationUnits u) const
{
  ENTER();

  CActivityConcentrationUnit calUnit;
  switch(u)
  {
    case CConcordeMainHeader::nCiPerCC: calUnit.setUnit(CActivityConcentrationUnit::NanoCuriesPerCubiccentimeter_units); break;
    case CConcordeMainHeader::BqPerCC: calUnit.setUnit(CActivityConcentrationUnit::BequerelsPerCubiccentimeter_units); break;
    default: calUnit.setUnit(CActivityConcentrationUnit::Unknown_units); break;
  }
  QString E7u = calUnit.unitAsString();

  RETURN(E7u.toAscii().data());
  return E7u;
}

CECAT7MainHeader::Calibration_Units CECAT7MainHeaderPrivate::concorde2ECAT7calibrationUnits(CConcordeMainHeader::CalibrationUnits u) const
{
  ENTER();

  CECAT7MainHeader::Calibration_Units E7u;
  switch(u)
  {
    case CConcordeMainHeader::nCiPerCC: E7u = CECAT7MainHeader::Uncalibrated; break;
    case CConcordeMainHeader::BqPerCC: E7u = CECAT7MainHeader::Calibrated; break;
    default: E7u = CECAT7MainHeader::Uncalibrated; break;
  }

  RETURN(E7u);
  return E7u;
}

float CECAT7MainHeaderPrivate::concorde2ECAT7dosage(float d, CConcordeMainHeader::DoseUnits u) const
{
  ENTER();

  // dosage in ECAT7 is Bq always
  CDosage dosage(d);

  switch(u)
  {
    case CConcordeMainHeader::mCi: dosage.setUnit(CDosage::Millicurie_units); break;
    case CConcordeMainHeader::MBq: dosage.setUnit(CDosage::Megabequerels_units); break;
    default: dosage.setUnit(CDosage::Unknown_units); break;
  }
  float E7d = dosage.convertTo(CDosage::Bequerels_units).dosage();

  RETURN(E7d);
  return E7d;
}

float CECAT7MainHeaderPrivate::concorde2ECAT7Height(float l, CConcordeMainHeader::SubjectLengthUnits u) const
{
  ENTER();
  
  // height in ECAT7 is cm always
  CLength height(l);

  switch(u)
  {
    case CConcordeMainHeader::Millimeters: height.setUnit(CLength::Millimeter_units); break;
    case CConcordeMainHeader::Centimeters: height.setUnit(CLength::Centimeter_units); break;
    case CConcordeMainHeader::Inches: height.setUnit(CLength::Inch_units); break;
    default: height.setUnit(CLength::Unknown_units); break;
  }
  float E7l = height.convertTo(CLength::Centimeter_units).length();

  RETURN(E7l);
  return E7l;
}

float CECAT7MainHeaderPrivate::concorde2ECAT7Weight(float w, CConcordeMainHeader::SubjectWeightUnits u) const
{
  ENTER();

  CWeight weight(w);

  // weight in ECAT7 is kilograms always
  switch(u)
  {
    case CConcordeMainHeader::Grams: weight.setUnit(CWeight::Gram_units); break;
    case CConcordeMainHeader::Ounces: weight.setUnit(CWeight::Ounce_units); break;
    case CConcordeMainHeader::Kilograms: weight.setUnit(CWeight::Kilogram_units); break;
    case CConcordeMainHeader::Pounds: weight.setUnit(CWeight::Pound_units); break;
    default: weight.setUnit(CWeight::Unknown_units); break;
  }
  float E7w = weight.convertTo(CWeight::Kilogram_units).weight();

  RETURN(E7w);
  return E7w;
}

CECAT7MainHeader::Patient_Sex CECAT7MainHeaderPrivate::concorde2ECAT7Sex(const QString& genus) const
{
  ENTER();

  CECAT7MainHeader::Patient_Sex E7s = CECAT7MainHeader::Sex_Unknown;
  if(genus.startsWith('m', Qt::CaseInsensitive))
    E7s = CECAT7MainHeader::Sex_Male;
  else if (genus.startsWith('f', Qt::CaseInsensitive))
    E7s = CECAT7MainHeader::Sex_Female;

  RETURN(E7s);
  return E7s;
}

CECAT7MainHeader::Patient_Orientation CECAT7MainHeaderPrivate::concorde2ECAT7Orientation(const CConcordeMainHeader::SubjectOrientation o) const
{
  ENTER();

  CECAT7MainHeader::Patient_Orientation E7o;
  switch(o)
  {
    case CConcordeMainHeader::UnknownSubjectOrientation: E7o = CECAT7MainHeader::FF_Prone; break;
    case CConcordeMainHeader::FeetFirstProne: E7o = CECAT7MainHeader::FF_Prone; break;
    case CConcordeMainHeader::HeadFirstProne: E7o = CECAT7MainHeader::HF_Prone; break;
    case CConcordeMainHeader::FeetFirstSupine: E7o = CECAT7MainHeader::FF_Supine; break;
    case CConcordeMainHeader::HeadFirstSupine: E7o = CECAT7MainHeader::HF_Supine; break;
    case CConcordeMainHeader::FeetFirstRight: E7o = CECAT7MainHeader::FF_Right; break;
    case CConcordeMainHeader::HeadFirstRight: E7o = CECAT7MainHeader::HF_Right; break;
    case CConcordeMainHeader::FeetFirstLeft: E7o = CECAT7MainHeader::FF_Left; break;
    case CConcordeMainHeader::HeadFirstLeft: E7o = CECAT7MainHeader::HF_Left; break;
    default: E7o = CECAT7MainHeader::Orient_Unknown; break;
  }

  RETURN(E7o);
  return E7o;
}


CECAT7MainHeader::Patient_Sex CECAT7MainHeaderPrivate::philips2Ecat7Sex(const char& genus) const
{
  ENTER();
  QString qGenus = QString(genus);
  CECAT7MainHeader::Patient_Sex E7s = CECAT7MainHeader::Sex_Unknown;
  if(qGenus.startsWith('m', Qt::CaseInsensitive))
    E7s = CECAT7MainHeader::Sex_Male;
  else if (qGenus.startsWith('f', Qt::CaseInsensitive))
    E7s = CECAT7MainHeader::Sex_Female;

  RETURN(E7s);
  return E7s;
}
