/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004-2005 by Jens Langner <Jens.Langner@light-speed.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

#include "CECAT7MainHeader.h"
#include "CECAT6MainHeader.h"
#include "CConcordeMainHeader.h"
#include "CECATFile.h"
#include "config.h"

#include <qcstring.h>
#include <qdatastream.h>
#include <qdatetime.h>
#include <qfileinfo.h>

#include <time.h>
#include <unistd.h>

#include <rtdebug.h>

CECAT7MainHeader::CECAT7MainHeader(CECATFile* ecatFile, 
																	 CECATMainHeader::Type fileType)
	: CECATMainHeader(ecatFile)
{
	// clear our MainHeader structure first
	memset(&m_Data, 0, sizeof(struct ECAT7MainHeader));
	
	// this constructor signals us to create a empty ECAT7MainHeader
	// with prefilled data that is always the same for a ECAT7 main header
	// depending on the supplied fileType we have to have a different Magic
	// Number field

	// set some default values for the new Main Header
	// (we mainly set these values like it is set on a EXACT HR+ scanner
	m_Data.SW_Version						= 72;					// use v7.2 as the default ECAT version
	m_Data.System_Type					= 962;				// set ECAT Model 962 (EXACT HR+) by default
	m_Data.Scan_Start_Time			= time(NULL);	// set the start time to the current time
	m_Data.Isotope_Halflife			= 6586.2;			// halflife time of FDOPA
	m_Data.Distance_Scanned			= 15.52;		  // scanner depth		
	m_Data.Transaxial_FOV				= 58.3;				// transaxial FOV of an ECAT HR+
	m_Data.Angular_Compression	= static_cast<short>(Mash2);
	m_Data.Acquisition_Type			= static_cast<short>(DynamicEmission);
	m_Data.Acquisition_Mode			= static_cast<short>(Windowed);
	m_Data.Transm_Source_Type		= static_cast<short>(SRC_RING);
	m_Data.Lwr_True_Thres				= 350;				// energy levels of an ECAT HR+
	m_Data.Upr_True_Thres				= 650;
	m_Data.Num_Planes						= 1;					// per default a minimum of 1 planes
	m_Data.Patient_Orientation	= 3;					// Head first + supine
	m_Data.Plane_Separation			= 0.2425;			// cm
	m_Data.Bin_Size							= 0.224983;		// cm
	m_Data.Branching_Fraction		= 1.0;
	m_Data.Septa_State					= static_cast<short>(Retracted);
	m_Data.Dose_Start_Time			= time(NULL);	// set the dose start time to the current time	

	setFileType(fileType);
	strcpy(m_Data.Serial_Number, "1");
	strcpy(m_Data.Isotope_Name, "F-18");
	strcpy(m_Data.Radiopharmaceutical, "F-18 Dopa");
	strcpy(m_Data.Study_Description, "created with libmedio v0.2");
}

void CECAT7MainHeader::setFileType(CECATMainHeader::Type fType)
{
	// to identify the correct fileType we need to do a proper mapping
	switch(fType)
	{
		case CECATMainHeader::Unknown:	m_Data.File_Type = Unknown;						break;
		case ECAT7_Sinogram:						m_Data.File_Type = Sinogram;					break;
		case ECAT7_Image16:							m_Data.File_Type = Image16;						break;
		case ECAT7_AttenCorr:						m_Data.File_Type = AttenuationCorr;		break;
		case ECAT7_Normalization:				m_Data.File_Type = Normalization;			break;
		case ECAT7_PolarMap:						m_Data.File_Type = PolarMap;					break;
		case ECAT7_Volume8:							m_Data.File_Type = Volume8;						break;
		case ECAT7_Volume16:						m_Data.File_Type = Volume16;					break;
		case ECAT7_Projection8:					m_Data.File_Type = Projection8;				break;
		case ECAT7_Projection16:				m_Data.File_Type = Projection16;			break;
		case ECAT7_Image8:							m_Data.File_Type = Image8;						break;
		case ECAT7_Sinogram3D_16:				m_Data.File_Type = Sinogram3D_16;			break;
		case ECAT7_Sinogram3D_8:				m_Data.File_Type = Sinogram3D_8;			break;
		case ECAT7_Normalization_3D:		m_Data.File_Type = Normalization_3D;	break;
		case ECAT7_Sinogram3D_Float:		m_Data.File_Type = Sinogram3D_Float;	break;
	}

	updateMagicNumber();
}

void CECAT7MainHeader::updateMagicNumber(void)
{
	switch(m_Data.File_Type)
	{		
		case Sinogram:
		case Sinogram3D_8:
		case Sinogram3D_16:
		case Sinogram3D_Float:
		{
			strcpy(m_Data.Magic_Number, "MATRIX72S");
		}
		break;

		case Volume8:
		case Volume16:
		{
			strcpy(m_Data.Magic_Number, "MATRIX72V");
		}
		break;

		case Image8:
		case Image16:
		{
			strcpy(m_Data.Magic_Number, "MATRIX72I");
		}
		break;

		case AttenuationCorr:
		{
			strcpy(m_Data.Magic_Number, "MATRIX72A");
		}
		break;

		case Normalization:
		case Normalization_3D:
		{
			strcpy(m_Data.Magic_Number, "MATRIX72N");
		}
		break;

		case Projection8:
		case Projection16:
		{
			strcpy(m_Data.Magic_Number, "MATRIX72P");
		}
		break;

		case PolarMap:
		{
			strcpy(m_Data.Magic_Number, "MATRIX72M");
		}
		break;

		default:
		{
			strcpy(m_Data.Magic_Number, "MATRIX72");
		}
	}	
}

bool CECAT7MainHeader::load(void)
{
	ENTER();

	// only go on if the device is readable at all
	if(m_pMedIOData == NULL ||
		 m_pMedIOData->isReadable() == false ||
		 m_pMedIOData->at(0) == false)
	{
		RETURN(false);
		return false;
	}

	// we use a ByteArray buffer to speed up the endianess
	// decoding
	QByteArray buffer(rawDataSize());
	if(m_pMedIOData->readBlock(buffer.data(), buffer.size()) != rawDataSize())
	{
		RETURN(false);
		return false;
	}

	// now we generate a QDataStream on our buffer so that we can read
	// out of the buffer instead of the raw file (> speed)
	QDataStream stream(buffer, IO_ReadOnly);

	// we now read out the header information stepwise
  // as we have to care about big/little endianess, which
	// is automatically done by the QT framework
  stream.readRawBytes(&m_Data.Magic_Number[0], 14);         //   0: Magic_Number
	stream.readRawBytes(&m_Data.Original_File_Name[0], 32);   //  14: Original_File_Name
	stream >> m_Data.SW_Version;                              //  46: SW_Version
	stream >> m_Data.System_Type;                             //  48: System_Type
	stream >> m_Data.File_Type;                               //  50: File_Type
	stream.readRawBytes(&m_Data.Serial_Number[0], 10);        //  52: Original_File_Name
	stream >> m_Data.Scan_Start_Time;                         //  62: Scan_Start_Time
	stream.readRawBytes(&m_Data.Isotope_Name[0], 8);          //  66: Isotope_Name
	stream >> m_Data.Isotope_Halflife;                        //  74: Isotope_Halflife
	stream.readRawBytes(&m_Data.Radiopharmaceutical[0], 32);	//  78: Radiopharmaceutical
  stream >> m_Data.Gantry_Tilt;															// 110: Gantry_Tilt
  stream >> m_Data.Gantry_Rotation;													// 114: Gantry_Rotation
  stream >> m_Data.Bed_Elevation;														// 118: Bed_Elevation
  stream >> m_Data.Intrinsic_Tilt;													// 122: Intrinsic_Tilt
  stream >> m_Data.Wobble_Speed;														// 126: Wobble_Speed
  stream >> m_Data.Transm_Source_Type;											// 128: Transm_Source_Type
  stream >> m_Data.Distance_Scanned;												// 130: Distance_Scanned
  stream >> m_Data.Transaxial_FOV;													// 134: Transaxial_FOV
  stream >> m_Data.Angular_Compression;											// 138: Angular_Compression
  stream >> m_Data.Coin_Samp_Mode;													// 140: Coin_Samp_Mode
  stream >> m_Data.Axial_Samp_Mode;													// 142: Axial_Samp_Mode
  stream >> m_Data.Calibration_Factor;											// 144: Calibration_Factor
  stream >> m_Data.Calibration_Units;												// 148: Calibration_Units
  stream >> m_Data.Calibration_Units_Label;									// 150: Calibration_Units_Label
  stream >> m_Data.Compression_Code;												// 152: Compression_Code
  stream.readRawBytes(&m_Data.Study_Type[0], 12);						// 154: Study_Type
  stream.readRawBytes(&m_Data.Patient_ID[0], 16);						// 166: Patient_ID
  stream.readRawBytes(&m_Data.Patient_Name[0], 32);					// 182: Patient_Name
  stream.readRawBytes(&m_Data.Patient_Sex[0], 1);						// 214: Patient_Sex
  stream.readRawBytes(&m_Data.Patient_Dexterity[0], 1);			// 215: Patient_Dexterity
  stream >> m_Data.Patient_Age;															// 216: Patient_Age
  stream >> m_Data.Patient_Height;													// 220: Patient_Height
  stream >> m_Data.Patient_Weight;													// 224: Patient_Weight
  stream >> m_Data.Patient_Birth_Date;											// 228: Patient_Birth_Date
  stream.readRawBytes(&m_Data.Physician_Name[0], 32);				// 232: Physician_Name
  stream.readRawBytes(&m_Data.Operator_Name[0], 32);				// 264: Operator_Name
  stream.readRawBytes(&m_Data.Study_Description[0], 32);		// 296: Study_Description
  stream >> m_Data.Acquisition_Type;												// 328: Acquisition_Type
  stream >> m_Data.Patient_Orientation;											// 330: Patient_Orientation
  stream.readRawBytes(&m_Data.Facility_Name[0], 20);				// 332: Facility_Name
  stream >> m_Data.Num_Planes;															// 352: Num_Planes
  stream >> m_Data.Num_Frames;															// 354: Num_Frames
  stream >> m_Data.Num_Gates;																// 356: Num_Gates
  stream >> m_Data.Num_Bed_Pos;															// 358: Num_Bed_Pos
  stream >> m_Data.Init_Bed_Position;												// 360: Init_Bed_Position
	for(short i=0; i < 15; i++)
		stream >> m_Data.Bed_Offset[i];													// 364: Bed_Offset (15)
  stream >> m_Data.Plane_Separation;												// 424: Plane_Separation
  stream >> m_Data.Lwr_Sctr_Thres;													// 428: Lwr_Sctr_Thres
  stream >> m_Data.Lwr_True_Thres;													// 430: Lwr_True_Thres
  stream >> m_Data.Upr_True_Thres;													// 432: Upr_True_Thres
  stream.readRawBytes(&m_Data.User_Process_Code[0], 10);		// 434: User_Process_Code
  stream >> m_Data.Acquisition_Mode;												// 444: Acquisition_Mode
  stream >> m_Data.Bin_Size;																// 446: Bin_Size
  stream >> m_Data.Branching_Fraction;											// 450: Branching_Fraction
  stream >> m_Data.Dose_Start_Time;													// 454: Dose_Start_Time
  stream >> m_Data.Dosage;																	// 458: Dosage
  stream >> m_Data.Well_Counter_Corr_Factor;								// 462: Well_Counter_Corr_Factor
  stream.readRawBytes(&m_Data.Data_Units[0], 32);						// 466: Data_Units
  stream >> m_Data.Septa_State;															// 498: Septa_State
  stream >> m_Data.CTI_Reserved[0];													// 500: CTI_Reserved 1
  stream >> m_Data.CTI_Reserved[1];													// 502: CTI_Reserved 2
  stream >> m_Data.CTI_Reserved[2];													// 504: CTI_Reserved 3
  stream >> m_Data.CTI_Reserved[3];													// 506: CTI_Reserved 4
  stream >> m_Data.CTI_Reserved[4];													// 508: CTI_Reserved 5
  stream >> m_Data.CTI_Reserved[5];													// 510: CTI_Reserved 6
    
	// some more debug output
#if defined(DEBUG)
	QDateTime time;
	D("ECAT7 Main Header loaded:");
	D("------------------------");
	D("Magic_Number            : %s",					    m_Data.Magic_Number);
	D("Original_File_Name      : %s",  	 	 	 	    m_Data.Original_File_Name);
	D("SW_Version              : %d",  	 	 	 	    m_Data.SW_Version);
	D("System_Type             : %d",  	 	 	 	    m_Data.System_Type);
	D("File_Type               : %d",  	 	 	 	    m_Data.File_Type);
	D("Serial_Number           : %s",  	 	 	 	    m_Data.Serial_Number);
	time.setTime_t(m_Data.Scan_Start_Time);
	D("Scan_Start_Time         : %s (%d)", 	 	    time.toString().ascii(), m_Data.Scan_Start_Time);
	D("Isotope Name            : %s",  	 	 	 	    m_Data.Isotope_Name);
	D("Isotope Halflife        : %f sec",				  m_Data.Isotope_Halflife);
	D("Radiopharmaca           : %s",			 	 		  m_Data.Radiopharmaceutical);
	D("Gantry Tilt             : %f°",					  m_Data.Gantry_Tilt);
	D("Gantry Rotation         : %f°",			 	 	  m_Data.Gantry_Rotation);
	D("Bed elevation           : %f cm",		 	 	  m_Data.Bed_Elevation);
	D("Intrinsic Tilt          : %f°",						m_Data.Intrinsic_Tilt);
	D("Wobble Speed            : %d r/min",				m_Data.Wobble_Speed);
	D("TransmSrcType           : %d",				      m_Data.Transm_Source_Type);
	D("Distance Scanned        : %d cm",		      m_Data.Distance_Scanned);
	D("Transaxial FOV          : %d cm",		      m_Data.Transaxial_FOV);
	D("Angular Compression     : %d",				      m_Data.Angular_Compression);
	D("Coin Samp Mode          : %d",				      m_Data.Coin_Samp_Mode);
	D("Axial Samp Mode         : %d",				      m_Data.Axial_Samp_Mode);
	D("Calibration Factor      : %f",				      m_Data.Calibration_Factor);
	D("Calibration Units	     : %d",				      m_Data.Calibration_Units);
	D("Calibration Units Label : %d",							m_Data.Calibration_Units_Label);
	D("Compression Code        : %d",							m_Data.Compression_Code);
	D("Studytype               : %s",							m_Data.Study_Type);
	D("Patient ID              : %s",							m_Data.Patient_ID);
	D("Patient Name            : %s",							m_Data.Patient_Name);
	D("Patient Sex             : %c",							m_Data.Patient_Sex[0]);
	D("Patient Dexterity       : %c",							m_Data.Patient_Dexterity[0]);
	D("Patient Age             : %f years",				m_Data.Patient_Age);
	D("Patient Height          : %f cm",					m_Data.Patient_Height);
	D("Patient Weight          : %f kg",					m_Data.Patient_Weight);
	time.setTime_t(m_Data.Patient_Birth_Date);
	D("Patient Birthdate       : %s (%d)",				time.toString().ascii(), m_Data.Patient_Birth_Date);
	D("Physician Name          : %s",							m_Data.Physician_Name);
	D("Operator Name           : %s",							m_Data.Operator_Name);
	D("Study Description       : %s",							m_Data.Study_Description);
	D("Acquisition Type        : %d",							m_Data.Acquisition_Type);
	D("Patient Orientation     : %d",							m_Data.Patient_Orientation);
	D("Facility Name           : %s",							m_Data.Facility_Name);
	D("Planes                  : %d",							m_Data.Num_Planes);
	D("Frames                  : %d",							m_Data.Num_Frames);
	D("Gates                   : %d",							m_Data.Num_Gates);
	D("Bed positions           : %d",							m_Data.Num_Bed_Pos);
	D("Initial Bed position    : %f cm",					m_Data.Init_Bed_Position);
	for(int i=0; i < m_Data.Num_Bed_Pos; i++)
	{
		D("Bed offset          [%2d]: %f cm", i+1, m_Data.Bed_Offset[i]);
	}
	D("Plane seperation        : %f cm",					m_Data.Plane_Separation);
	D("Lowest Threshold Scatter: %d KeV",					m_Data.Lwr_Sctr_Thres);
	D("Lower Threshold Trues   : %d KeV",					m_Data.Lwr_True_Thres);
	D("Upper Threshold Trues   : %d KeV",					m_Data.Upr_True_Thres);
	D("User Process Code       : %s",							m_Data.User_Process_Code);
	D("Acquisition Mode        : %d",							m_Data.Acquisition_Mode);
	D("Bin Size                : %f cm",					m_Data.Bin_Size);
	D("Branching Fraction      : %f",							m_Data.Branching_Fraction);
	time.setTime_t(m_Data.Dose_Start_Time);
	D("Dose start time         : %s (%d)",				time.toString().ascii(), m_Data.Dose_Start_Time);
	D("Dosage                  : %f Bq/cc",				m_Data.Dosage);
	D("Well counter corr factor: %f",							m_Data.Well_Counter_Corr_Factor);
	D("Data units              : %s",							m_Data.Data_Units);
	D("Septa state             : %d",							m_Data.Septa_State);
	D("CTI reserved            : %lx",						m_Data.CTI_Reserved);
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
				strncpy(mHeader.m_Data.Magic_Number, dataString.ascii(), 14);
			else if(typeString == "ORIGINAL_FILE_NAME")
				strncpy(mHeader.m_Data.Original_File_Name, dataString.ascii(), 32);
			else if(typeString == "SW_VERSION")
				mHeader.m_Data.SW_Version = dataString.toShort(&convertSuccess);
			else if(typeString == "SYSTEM_TYPE")
				mHeader.m_Data.System_Type = dataString.toShort(&convertSuccess);
			else if(typeString == "FILE_TYPE")
				mHeader.m_Data.File_Type = dataString.toShort(&convertSuccess);
			else if(typeString == "SERIAL_NUMBER")
				strncpy(mHeader.m_Data.Serial_Number, dataString.ascii(), 10);
			else if(typeString == "SCAN_START_TIME")
				mHeader.m_Data.Scan_Start_Time = dataString.toLong(&convertSuccess);
			else if(typeString == "ISOTOPE_NAME")
				strncpy(mHeader.m_Data.Isotope_Name, dataString.ascii(), 8);
			else if(typeString == "ISOTOPE_HALFLIFE")
				mHeader.m_Data.Isotope_Halflife = dataString.toFloat(&convertSuccess);
			else if(typeString == "RADIOPHARMACEUTICAL")
				strncpy(mHeader.m_Data.Radiopharmaceutical, dataString.ascii(), 32);
			else if(typeString == "GANTRY_TILT")
				mHeader.m_Data.Gantry_Tilt = dataString.toFloat(&convertSuccess);
			else if(typeString == "ROTATION_TILT")
				mHeader.m_Data.Gantry_Rotation = dataString.toFloat(&convertSuccess);
			else if(typeString == "BED_ELEVATION")
				mHeader.m_Data.Bed_Elevation = dataString.toFloat(&convertSuccess);
			else if(typeString == "INTRINSIC_TILT")
				mHeader.m_Data.Intrinsic_Tilt = dataString.toFloat(&convertSuccess);
			else if(typeString == "WOBBLE_SPEED")
				mHeader.m_Data.Wobble_Speed = dataString.toShort(&convertSuccess);
			else if(typeString == "TRANSM_SOURCE_TYPE")
				mHeader.m_Data.Transm_Source_Type = dataString.toShort(&convertSuccess);
			else if(typeString == "DISTANCE_SCANNED")
				mHeader.m_Data.Distance_Scanned = dataString.toFloat(&convertSuccess);
			else if(typeString == "TRANSAXIAL_FOV")
				mHeader.m_Data.Transaxial_FOV = dataString.toFloat(&convertSuccess);
			else if(typeString == "ANGULAR_COMPRESSION")
				mHeader.m_Data.Angular_Compression = dataString.toShort(&convertSuccess);
			else if(typeString == "COIN_SAMP_MODE")
				mHeader.m_Data.Coin_Samp_Mode = dataString.toShort(&convertSuccess);
			else if(typeString == "AXIAL_SAMP_MODE")
				mHeader.m_Data.Axial_Samp_Mode = dataString.toShort(&convertSuccess);
			else if(typeString == "ECAT_CALIBRATION_FACTOR")
				mHeader.m_Data.Calibration_Factor = dataString.toFloat(&convertSuccess);
			else if(typeString == "CALIBRATION_UNITS")
				mHeader.m_Data.Calibration_Units = dataString.toShort(&convertSuccess);
			else if(typeString == "CALIBRATION_UNITS_LABEL")
				mHeader.m_Data.Calibration_Units_Label = dataString.toShort(&convertSuccess);
			else if(typeString == "COMPRESSION_CODE")
				mHeader.m_Data.Compression_Code = dataString.toShort(&convertSuccess);
			else if(typeString == "STUDY_TYPE")
				strncpy(mHeader.m_Data.Study_Type, dataString.ascii(), 12);
			else if(typeString == "PATIENT_ID")
				strncpy(mHeader.m_Data.Patient_ID, dataString.ascii(), 16);
			else if(typeString == "PATIENT_NAME")
				strncpy(mHeader.m_Data.Patient_Name, dataString.ascii(), 32);
			else if(typeString == "PATIENT_SEX")
				mHeader.m_Data.Patient_Sex[0] = dataString.ascii()[0];
			else if(typeString == "PATIENT_DEXTERITY")
				mHeader.m_Data.Patient_Dexterity[0] = dataString.ascii()[0];
			else if(typeString == "PATIENT_AGE")
				mHeader.m_Data.Patient_Age = dataString.toFloat(&convertSuccess);
			else if(typeString == "PATIENT_HEIGHT")
				mHeader.m_Data.Patient_Height = dataString.toFloat(&convertSuccess);
			else if(typeString == "PATIENT_WEIGHT")
				mHeader.m_Data.Patient_Weight = dataString.toFloat(&convertSuccess);
			else if(typeString == "PATIENT_BIRTH_DATE")
				mHeader.m_Data.Patient_Birth_Date = dataString.toLong(&convertSuccess);
			else if(typeString == "PHYSICIAN_NAME")
				strncpy(mHeader.m_Data.Physician_Name, dataString.ascii(), 32);
			else if(typeString == "OPERATOR_NAME")
				strncpy(mHeader.m_Data.Operator_Name, dataString.ascii(), 32);
			else if(typeString == "STUDY_DESCRIPTION")
				strncpy(mHeader.m_Data.Study_Description, dataString.ascii(), 32);
			else if(typeString == "ACQUISITION_TYPE")
				mHeader.m_Data.Acquisition_Type = dataString.toShort(&convertSuccess);
			else if(typeString == "PATIENT_ORIENTATION")
				mHeader.m_Data.Patient_Orientation = dataString.toShort(&convertSuccess);
			else if(typeString == "FACILITY_NAME")
				strncpy(mHeader.m_Data.Facility_Name, dataString.ascii(), 20);
			else if(typeString == "NUM_PLANES")
				mHeader.m_Data.Num_Planes = dataString.toShort(&convertSuccess);
			else if(typeString == "NUM_FRAMES")
				mHeader.m_Data.Num_Frames = dataString.toShort(&convertSuccess);
			else if(typeString == "NUM_GATES")
				mHeader.m_Data.Num_Gates = dataString.toShort(&convertSuccess);
			else if(typeString == "NUM_BED_POS")
				mHeader.m_Data.Num_Bed_Pos = dataString.toShort(&convertSuccess);
			else if(typeString == "INIT_BED_POSITION")
				mHeader.m_Data.Init_Bed_Position = dataString.toFloat(&convertSuccess);
			else if(typeString == "BED_POSITION")
			{
				for(int i=0; i < 15 && convertSuccess; i++)
				{
					QString subString = dataString.section(" ", i, i);
					if(subString.isEmpty())
						break;

					mHeader.m_Data.Bed_Offset[i] = subString.toFloat(&convertSuccess);
				}
			}
			else if(typeString == "PLANE_SEPARATION")
				mHeader.m_Data.Plane_Separation = dataString.toFloat(&convertSuccess);
			else if(typeString == "LWR_SCTR_THRES")
				mHeader.m_Data.Lwr_Sctr_Thres = dataString.toShort(&convertSuccess);
			else if(typeString == "LWR_TRUE_THRES")
				mHeader.m_Data.Lwr_True_Thres = dataString.toShort(&convertSuccess);
			else if(typeString == "UPR_TRUE_THRES")
				mHeader.m_Data.Upr_True_Thres = dataString.toShort(&convertSuccess);
			else if(typeString == "USER_PROCESS_CODE")
				strncpy(mHeader.m_Data.User_Process_Code, dataString.ascii(), 10);
			else if(typeString == "ACQUISITION_MODE")
				mHeader.m_Data.Acquisition_Mode = dataString.toShort(&convertSuccess);
			else if(typeString == "BIN_SIZE")
				mHeader.m_Data.Bin_Size = dataString.toFloat(&convertSuccess);
			else if(typeString == "BRANCHING_FRACTION")
				mHeader.m_Data.Branching_Fraction = dataString.toFloat(&convertSuccess);
			else if(typeString == "DOSE_START_TIME")
				mHeader.m_Data.Dose_Start_Time = dataString.toLong(&convertSuccess);
			else if(typeString == "DOSAGE")
				mHeader.m_Data.Dosage = dataString.toFloat(&convertSuccess);
			else if(typeString == "WELL_COUNTER_CORR_FACTOR")
				mHeader.m_Data.Well_Counter_Corr_Factor = dataString.toFloat(&convertSuccess);
			else if(typeString == "DATA_UNITS")
				strncpy(mHeader.m_Data.Data_Units, dataString.ascii(), 32);
			else if(typeString == "SEPTA_STATE")
				mHeader.m_Data.Septa_State = dataString.toShort(&convertSuccess);
			else if(typeString == "FILL")
			{
				for(int i=0; i < 6 && convertSuccess; i++)
				{
					QString subString = dataString.section(" ", i, i);
					if(subString.isEmpty())
						break;

					mHeader.m_Data.CTI_Reserved[i] = subString.toShort(&convertSuccess);
				}
			}

			if(convertSuccess == false)
			{
				E("'%s' - error while converting string '%s' to a numerical value.", typeString.ascii(), dataString.ascii());
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

	// only go on if the device is writeable at all
	if(m_pMedIOData == NULL ||
		 m_pMedIOData->isWritable() == false ||
		 m_pMedIOData->at(0) == false)
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
	Q_UINT16 numFrames = ecatFile->numFrames();
	Q_UINT16 numGates  = ecatFile->numGates();
	Q_UINT16 numBedPos = ecatFile->numBedPos();
	
	// we write to a buffer first and write out later directly to the file
	QByteArray buffer(rawDataSize());
	QDataStream stream(buffer, IO_WriteOnly);

  // we now read out the header information stepwise
  // as we have to care about big/little endianess, which
	// is automatically done by the QT framework
	stream.writeRawBytes(&m_Data.Magic_Number[0], 14);        //   0: Magic_Number
  stream.writeRawBytes(&m_Data.Original_File_Name[0], 32);  //  14: Original_File_Name
  stream << m_Data.SW_Version;                              //  46: SW_Version
  stream << m_Data.System_Type;                             //  48: System_Type
  stream << m_Data.File_Type;                               //  50: File_Type
  stream.writeRawBytes(&m_Data.Serial_Number[0], 10);       //  52: Original_File_Name
  stream << m_Data.Scan_Start_Time;                         //  62: Scan_Start_Time
  stream.writeRawBytes(&m_Data.Isotope_Name[0], 8);         //  66: Isotope_Name
  stream << m_Data.Isotope_Halflife;                        //  74: Isotope_Halflife
  stream.writeRawBytes(&m_Data.Radiopharmaceutical[0], 32);	//  78: Radiopharmaceutical
  stream << m_Data.Gantry_Tilt;															// 110: Gantry_Tilt
  stream << m_Data.Gantry_Rotation;													// 114: Gantry_Rotation
  stream << m_Data.Bed_Elevation;														// 118: Bed_Elevation
  stream << m_Data.Intrinsic_Tilt;													// 122: Intrinsic_Tilt
  stream << m_Data.Wobble_Speed;														// 126: Wobble_Speed
  stream << m_Data.Transm_Source_Type;											// 128: Transm_Source_Type
  stream << m_Data.Distance_Scanned;												// 130: Distance_Scanned
  stream << m_Data.Transaxial_FOV;													// 134: Transaxial_FOV
  stream << m_Data.Angular_Compression;											// 138: Angular_Compression
  stream << m_Data.Coin_Samp_Mode;													// 140: Coin_Samp_Mode
  stream << m_Data.Axial_Samp_Mode;													// 142: Axial_Samp_Mode
  stream << m_Data.Calibration_Factor;											// 144: Calibration_Factor
  stream << m_Data.Calibration_Units;												// 148: Calibration_Units
  stream << m_Data.Calibration_Units_Label;									// 150: Calibration_Units_Label
  stream << m_Data.Compression_Code;												// 152: Compression_Code
  stream.writeRawBytes(&m_Data.Study_Type[0], 12);					// 154: Study_Type
  stream.writeRawBytes(&m_Data.Patient_ID[0], 16);					// 166: Patient_ID
  stream.writeRawBytes(&m_Data.Patient_Name[0], 32);				// 182: Patient_Name
  stream.writeRawBytes(&m_Data.Patient_Sex[0], 1);					// 214: Patient_Sex
  stream.writeRawBytes(&m_Data.Patient_Dexterity[0], 1);		// 215: Patient_Dexterity
  stream << m_Data.Patient_Age;															// 216: Patient_Age
  stream << m_Data.Patient_Height;													// 220: Patient_Height
  stream << m_Data.Patient_Weight;													// 224: Patient_Weight
  stream << m_Data.Patient_Birth_Date;											// 228: Patient_Birth_Date
  stream.writeRawBytes(&m_Data.Physician_Name[0], 32);			// 232: Physician_Name
  stream.writeRawBytes(&m_Data.Operator_Name[0], 32);				// 264: Operator_Name
  stream.writeRawBytes(&m_Data.Study_Description[0], 32);		// 296: Study_Description
  stream << m_Data.Acquisition_Type;												// 328: Acquisition_Type
  stream << m_Data.Patient_Orientation;											// 330: Patient_Orientation
  stream.writeRawBytes(&m_Data.Facility_Name[0], 20);				// 332: Facility_Name
  stream << m_Data.Num_Planes;															// 352: Num_Planes
  stream << numFrames;																			// 354: Num_Frames
  stream << numGates;																				// 356: Num_Gates
  stream << numBedPos;																			// 358: Num_Bed_Pos
  stream << m_Data.Init_Bed_Position;												// 360: Init_Bed_Position
  stream << m_Data.Bed_Offset[0];														// 364: Bed_Offset 1
  stream << m_Data.Bed_Offset[1];														// 368: Bed_Offset 2
  stream << m_Data.Bed_Offset[2];														// 372: Bed_Offset 3
  stream << m_Data.Bed_Offset[3];														// 376: Bed_Offset 4
  stream << m_Data.Bed_Offset[4];														// 380: Bed_Offset 5
  stream << m_Data.Bed_Offset[5];														// 384: Bed_Offset 6
  stream << m_Data.Bed_Offset[6];														// 388: Bed_Offset 7
  stream << m_Data.Bed_Offset[7];														// 392: Bed_Offset 8
  stream << m_Data.Bed_Offset[8];														// 396: Bed_Offset 9
  stream << m_Data.Bed_Offset[9];														// 400: Bed_Offset 10
  stream << m_Data.Bed_Offset[10];													// 404: Bed_Offset 11
  stream << m_Data.Bed_Offset[11];													// 408: Bed_Offset 12
  stream << m_Data.Bed_Offset[12];													// 412: Bed_Offset 13
  stream << m_Data.Bed_Offset[13];													// 416: Bed_Offset 14
  stream << m_Data.Bed_Offset[14];													// 420: Bed_Offset 15
  stream << m_Data.Plane_Separation;												// 424: Plane_Separation
  stream << m_Data.Lwr_Sctr_Thres;													// 428: Lwr_Sctr_Thres
  stream << m_Data.Lwr_True_Thres;													// 430: Lwr_True_Thres
  stream << m_Data.Upr_True_Thres;													// 432: Upr_True_Thres
  stream.writeRawBytes(&m_Data.User_Process_Code[0], 10);		// 434: User_Process_Code
  stream << m_Data.Acquisition_Mode;												// 444: Acquisition_Mode
  stream << m_Data.Bin_Size;																// 446: Bin_Size
  stream << m_Data.Branching_Fraction;											// 450: Branching_Fraction
  stream << m_Data.Dose_Start_Time;													// 454: Dose_Start_Time
  stream << m_Data.Dosage;																	// 458: Dosage
  stream << m_Data.Well_Counter_Corr_Factor;								// 462: Well_Counter_Corr_Factor
  stream.writeRawBytes(&m_Data.Data_Units[0], 32);					// 466: Data_Units
  stream << m_Data.Septa_State;															// 498: Septa_State
  stream << m_Data.CTI_Reserved[0];													// 500: CTI_Reserved 1
  stream << m_Data.CTI_Reserved[1];													// 502: CTI_Reserved 2
  stream << m_Data.CTI_Reserved[2];													// 504: CTI_Reserved 3
  stream << m_Data.CTI_Reserved[3];													// 506: CTI_Reserved 4
  stream << m_Data.CTI_Reserved[4];													// 508: CTI_Reserved 5
  stream << m_Data.CTI_Reserved[5];													// 510: CTI_Reserved 6

	// now write out to our outStream
	bool result = false;
	if(m_pMedIOData->writeBlock(buffer) != -1)
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
					memcpy(&(this->m_Data), &(static_cast<const CECAT7MainHeader*>(pHead1)->m_Data), sizeof(struct ECAT7MainHeader));
					bResult = true;
				}
				break;

				// if this is an ECAT6 Mainheader we have to take care of the fact that
				// some information is missing in one of the headers.
				case CECATMainHeader::ECAT6MainHeader:
				{
					const CECAT6MainHeader* e6src = static_cast<const CECAT6MainHeader*>(pHead1);
					setOriginal_File_Name(e6src->original_File_Name());
					setSystem_Type(e6src->system_Type());

					#warning "ECAT6->ECAT7 copy not fully implemented yet!"
					bResult = true;
				}
				break;
			}
			
			// afterwards we have to make sure sensible data is restored
			m_Data.SW_Version = 72; // This header does conform to the ECAT 7.2 standard
		}

		case CMedIOHeader::ECATSubHeader:
		case CMedIOHeader::ConcordeMicroPetFrameHeader:
			// copying a sub header into a main header doesn't make much sense, so we
			// do nothing here
		break;

		case CMedIOHeader::ConcordeMicroPetMainHeader:
		{
			CConcordeMainHeader* head = (CConcordeMainHeader*)pHead1;
			setOriginal_File_Name(head->fileName().ascii());
			setSystem_Type((short)head->model());
			setScan_Start_Time(head->scanTime());
			setIsotope_Name(head->isotope().ascii());
			setIsotope_Halflife(head->isotopeHalfTime());
			
			setDistance_Scanned(head->radialFov());
			setTransaxial_FOV(head->radialFov());
			setCoin_Samp_Mode(CECAT7MainHeader::NetTrues);
			setCalibration_Factor(head->calibrationFactor());

			if(head->calibrationFactor() == 1.0F)
				setCalibration_Units(CECAT7MainHeader::Uncalibrated);
			else
				setCalibration_Units(CECAT7MainHeader::Calibrated);
			setStudy_Type(head->study().ascii());
			setPatient_ID(head->subjectIdentifier().ascii());
			setPatient_Name(head->subjectIdentifier().ascii());
			setPatient_Sex(CECAT7MainHeader::Sex_Male);
			
			setPatient_Height(head->subjectLength());
			setPatient_Weight(head->subjectWeight()/1000.0);
			setPhysician_Name(head->investigatorName().ascii());
			setOperator_Name(head->operatorName().ascii());
			setStudy_Description(head->studyIdentifier().ascii());
			if(head->acquisitionMode() == CConcordeMainHeader::Emission)
				setAcquisition_Type(CECAT7MainHeader::StaticEmission);
			else
				setAcquisition_Type(CECAT7MainHeader::DynamicEmission);
			switch(head->subjectOrientation())
			{
				case CConcordeMainHeader::UnknownSubjectOrientation: setPatient_Orientation(CECAT7MainHeader::FF_Prone); break;
				case CConcordeMainHeader::FeetFirstProne: setPatient_Orientation(CECAT7MainHeader::FF_Prone); break;
				case CConcordeMainHeader::HeadFirstProne: setPatient_Orientation(CECAT7MainHeader::HF_Prone); break;
				case CConcordeMainHeader::FeetFirstSupine: setPatient_Orientation(CECAT7MainHeader::FF_Supine); break;
				case CConcordeMainHeader::HeadFirstSupine: setPatient_Orientation(CECAT7MainHeader::HF_Supine); break;
				case CConcordeMainHeader::FeetFirstRight: setPatient_Orientation(CECAT7MainHeader::FF_Right); break;
				case CConcordeMainHeader::HeadFirstRight: setPatient_Orientation(CECAT7MainHeader::HF_Right); break;
				case CConcordeMainHeader::FeetFirstLeft: setPatient_Orientation(CECAT7MainHeader::FF_Left); break;
				case CConcordeMainHeader::HeadFirstLeft: setPatient_Orientation(CECAT7MainHeader::HF_Left); break;
			}
			setFacility_Name(head->institution().ascii());
			setNum_Planes(head->zDimension());
			setNum_Frames(head->totalFrames());
			setLwr_True_Thres((short)head->lld());
			setUpr_True_Thres((short)head->uld());
			setBranching_Fraction(head->isotopeBranchingFraction());
			setNum_Gates(1);
			setNum_Bed_Pos(0);
			setDose_Start_Time(head->injectionTime());

			setDosage(head->dose()*1000000.0);

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

		case CMedIOHeader::Unknown:
			// for an unknown header type we do nothing
		break;
	}

	LEAVE();
	return bResult;
}

CMedIOHeader* CECAT7MainHeader::clone() const
{
	CECAT7MainHeader* pNewHead = new CECAT7MainHeader();
	pNewHead->convertFrom(this);
	return pNewHead;
}

const char* CECAT7MainHeader::magic_Number(void) const
{
	return m_Data.Magic_Number;
}

const char* CECAT7MainHeader::original_File_Name(void) const
{
	return m_Data.Original_File_Name;
}

short CECAT7MainHeader::sw_Version(void) const
{
	return m_Data.SW_Version;
}

short CECAT7MainHeader::system_Type(void) const
{
	return m_Data.System_Type;
}

CECAT7MainHeader::File_Type CECAT7MainHeader::file_Type(void)	const
{
	return static_cast<CECAT7MainHeader::File_Type>(m_Data.File_Type);
}

const char* CECAT7MainHeader::serial_Number(void) const
{
	return m_Data.Serial_Number;
}

time_t CECAT7MainHeader::scan_Start_Time(void) const
{
	return m_Data.Scan_Start_Time;
}

const char* CECAT7MainHeader::isotope_Name(void) const
{
	return m_Data.Isotope_Name;
}

float CECAT7MainHeader::isotope_Halflife(void) const
{
	return m_Data.Isotope_Halflife;
}

const char* CECAT7MainHeader::radiopharmaceutical(void) const
{
	return m_Data.Radiopharmaceutical;
}

float CECAT7MainHeader::gantry_Tilt(void) const
{
	return m_Data.Gantry_Tilt;
}

float CECAT7MainHeader::gantry_Rotation(void) const
{
	return m_Data.Gantry_Rotation;
}

float CECAT7MainHeader::bed_Elevation(void) const
{
	return m_Data.Bed_Elevation;
}

float CECAT7MainHeader::intrinsic_Tilt(void) const
{
	return m_Data.Intrinsic_Tilt;
}

short CECAT7MainHeader::wobble_Speed(void) const
{
	return m_Data.Wobble_Speed;
}

CECAT7MainHeader::Transm_Source_Type CECAT7MainHeader::transm_Source_Type(void) const
{
	return static_cast<CECAT7MainHeader::Transm_Source_Type>(m_Data.Transm_Source_Type);
}

float CECAT7MainHeader::distance_Scanned(void) const													
{
	return m_Data.Distance_Scanned;
}

float CECAT7MainHeader::transaxial_FOV(void) const														
{
	return m_Data.Transaxial_FOV;
}

CECAT7MainHeader::Angular_Compression CECAT7MainHeader::angular_Compression(void) const					
{
	return static_cast<CECAT7MainHeader::Angular_Compression>(m_Data.Angular_Compression);
}

CECAT7MainHeader::Coin_Samp_Mode CECAT7MainHeader::coin_Samp_Mode(void) const										
{
	return static_cast<CECAT7MainHeader::Coin_Samp_Mode>(m_Data.Coin_Samp_Mode);
}

CECAT7MainHeader::Axial_Samp_Mode CECAT7MainHeader::axial_Samp_Mode(void) const									
{
	return static_cast<CECAT7MainHeader::Axial_Samp_Mode>(m_Data.Axial_Samp_Mode);
}

float CECAT7MainHeader::calibration_Factor(void) const												
{
	return m_Data.Calibration_Factor;
}

CECAT7MainHeader::Calibration_Units CECAT7MainHeader::calibration_Units(void) const							
{
	return static_cast<CECAT7MainHeader::Calibration_Units>(m_Data.Calibration_Units);
}

CECAT7MainHeader::Calibration_Units_Label CECAT7MainHeader::calibration_Units_Label(void) const	
{
	return static_cast<CECAT7MainHeader::Calibration_Units_Label>(m_Data.Calibration_Units_Label);
}

CECAT7MainHeader::Compression_Code CECAT7MainHeader::compression_Code(void) const							
{
	return static_cast<CECAT7MainHeader::Compression_Code>(m_Data.Compression_Code);
}

const char* CECAT7MainHeader::study_Type(void) const												
{
	return m_Data.Study_Type;
}

const char* CECAT7MainHeader::patient_ID(void) const													
{
	return m_Data.Patient_ID;
}

const char* CECAT7MainHeader::patient_Name(void) const												
{
	return m_Data.Patient_Name;
}

CECAT7MainHeader::Patient_Sex CECAT7MainHeader::patient_Sex(void) const													
{
	return static_cast<CECAT7MainHeader::Patient_Sex>(m_Data.Patient_Sex[0]);
}

CECAT7MainHeader::Patient_Dexterity CECAT7MainHeader::patient_Dexterity(void) const							
{
	return static_cast<CECAT7MainHeader::Patient_Dexterity>(m_Data.Patient_Dexterity[0]);
}

float CECAT7MainHeader::patient_Age(void) const																
{
	return m_Data.Patient_Age;
}

float CECAT7MainHeader::patient_Height(void) const														
{
	return m_Data.Patient_Height;
}

float CECAT7MainHeader::patient_Weight(void) const														
{
	return m_Data.Patient_Weight;
}

time_t CECAT7MainHeader::patient_Birth_Date(void) const								
{
	return m_Data.Patient_Birth_Date;
}

const char* CECAT7MainHeader::physician_Name(void) const										
{
	return m_Data.Physician_Name;
}

const char* CECAT7MainHeader::operator_Name(void) const												
{
	return m_Data.Operator_Name;
}

const char* CECAT7MainHeader::study_Description(void) const										
{
	return m_Data.Study_Description;
}

CECAT7MainHeader::Acquisition_Type CECAT7MainHeader::acquisition_Type(void) const							
{
	return static_cast<CECAT7MainHeader::Acquisition_Type>(m_Data.Acquisition_Type);
}

CECAT7MainHeader::Patient_Orientation CECAT7MainHeader::patient_Orientation(void) const											
{
	return static_cast<CECAT7MainHeader::Patient_Orientation>(m_Data.Patient_Orientation);
}

const char* CECAT7MainHeader::facility_Name(void) const											
{
	return m_Data.Facility_Name;
}

short CECAT7MainHeader::num_Planes(void) const															
{
	return m_Data.Num_Planes;
}

short CECAT7MainHeader::num_Frames(void) const															
{
	return m_Data.Num_Frames;
}

short CECAT7MainHeader::num_Gates(void) const																
{
	return m_Data.Num_Gates;
}

short CECAT7MainHeader::num_Bed_Pos(void) const															
{
	return m_Data.Num_Bed_Pos;
}

float CECAT7MainHeader::init_Bed_Position(void) const												
{
	return m_Data.Init_Bed_Position;
}

float CECAT7MainHeader::bed_Offset(const short i) const											
{
	return m_Data.Bed_Offset[i];
}

float CECAT7MainHeader::plane_Separation(void) const												
{
	return m_Data.Plane_Separation;
}

short CECAT7MainHeader::lwr_Sctr_Thres(void) const													
{
	return m_Data.Lwr_Sctr_Thres;
}

short CECAT7MainHeader::lwr_True_Thres(void) const													
{
	return m_Data.Lwr_True_Thres;
}

short CECAT7MainHeader::upr_True_Thres(void) const													
{
	return m_Data.Upr_True_Thres;
}

const char* CECAT7MainHeader::user_Process_Code(void) const									
{
	return m_Data.User_Process_Code;
}

CECAT7MainHeader::Acquisition_Mode CECAT7MainHeader::acquisition_Mode(void) const							
{
	return static_cast<CECAT7MainHeader::Acquisition_Mode>(m_Data.Acquisition_Mode);
}

float CECAT7MainHeader::bin_Size(void) const																
{
	return m_Data.Bin_Size;	 }

float CECAT7MainHeader::branching_Fraction(void) const											
{
	return m_Data.Branching_Fraction;
}

time_t CECAT7MainHeader::dose_Start_Time(void) const									
{
	return m_Data.Dose_Start_Time;
}

float CECAT7MainHeader::dosage(void) const																	
{
	return m_Data.Dosage;
}

float CECAT7MainHeader::well_Counter_Corr_Factor(void) const								
{
	return m_Data.Well_Counter_Corr_Factor;
}

const char* CECAT7MainHeader::data_Units(void) const												
{
	return m_Data.Data_Units;
}

CECAT7MainHeader::Septa_State CECAT7MainHeader::septa_State(void) const												
{
	return static_cast<CECAT7MainHeader::Septa_State>(m_Data.Septa_State);
}

short CECAT7MainHeader::cti_Reserved(const short i) const										
{
	return m_Data.CTI_Reserved[i];
}


// mutator methods
void CECAT7MainHeader::setMagic_Number(const char* mn)
{
	strncpy(m_Data.Magic_Number, mn, 14);
}

void CECAT7MainHeader::setOriginal_File_Name(const char* name)
{
	strncpy(m_Data.Original_File_Name, name, 32);
}

void CECAT7MainHeader::setSW_Version(const short ver)
{
	m_Data.SW_Version = ver;
}

void CECAT7MainHeader::setSystem_Type(const short type)
{
	m_Data.System_Type = type;
}

void CECAT7MainHeader::setFileType(const File_Type fType)
{
	m_Data.File_Type = fType;	updateMagicNumber();
}

void CECAT7MainHeader::setSerial_Number(const char* num)
{
	strncpy(m_Data.Serial_Number, num, 10);
}

void CECAT7MainHeader::setScan_Start_Time(const time_t time)
{
	m_Data.Scan_Start_Time = static_cast<Q_INT32>(time);
}

void CECAT7MainHeader::setIsotope_Name(const char* name)
{
	strncpy(m_Data.Isotope_Name, name, 8);
}

void CECAT7MainHeader::setIsotope_Halflife(const float hlf)															
{
	m_Data.Isotope_Halflife = hlf;
}

void CECAT7MainHeader::setRadiopharmaceutical(const char* str)													
{
	strncpy(m_Data.Radiopharmaceutical, str, 32);
}

void CECAT7MainHeader::setGantry_Tilt(const float tilt)																	
{
	m_Data.Gantry_Tilt = tilt;
}

void CECAT7MainHeader::setGantry_Rotation(const float rot)															
{
	m_Data.Gantry_Rotation = rot;
}

void CECAT7MainHeader::setBed_Elevation(const float elev)																
{
	m_Data.Bed_Elevation = elev;
}

void CECAT7MainHeader::setIntrinsic_Tilt(const float tilt)															
{
	m_Data.Intrinsic_Tilt = tilt;
}

void CECAT7MainHeader::setWobble_Speed(const short speed)															
{
	m_Data.Wobble_Speed = speed;
}

void CECAT7MainHeader::setTransm_Source_Type(const Transm_Source_Type type)						
{
	m_Data.Transm_Source_Type = type;
}

void CECAT7MainHeader::setDistance_Scanned(const float dist)													
{
	m_Data.Distance_Scanned = dist;
}

void CECAT7MainHeader::setTransaxial_FOV(const float fov)																
{
	m_Data.Transaxial_FOV = fov;
}

void CECAT7MainHeader::setAngular_Compression(const Angular_Compression comp)						
{
	m_Data.Angular_Compression = comp;
}

void CECAT7MainHeader::setCoin_Samp_Mode(const Coin_Samp_Mode mode)
{
	m_Data.Coin_Samp_Mode = mode;
}

void CECAT7MainHeader::setAxial_Samp_Mode(const Axial_Samp_Mode mode)										
{
	m_Data.Axial_Samp_Mode = mode;
}

void CECAT7MainHeader::setCalibration_Factor(const float factor)												
{
	m_Data.Calibration_Factor = factor;
}

void CECAT7MainHeader::setCalibration_Units(const Calibration_Units unit)								
{
	m_Data.Calibration_Units = unit;
}

void CECAT7MainHeader::setCalibration_Units_Label(const Calibration_Units_Label label)
{
	m_Data.Calibration_Units_Label = label;
}

void CECAT7MainHeader::setCompression_Code(const Compression_Code code)	
{
	m_Data.Compression_Code = code;
}

void CECAT7MainHeader::setStudy_Type(const char* type)																	
{
	strncpy(m_Data.Study_Type, type, 12);
}

void CECAT7MainHeader::setPatient_ID(const char* id)																	
{
	strncpy(m_Data.Patient_ID, id, 16);
}

void CECAT7MainHeader::setPatient_Name(const char* name)
{
	strncpy(m_Data.Patient_Name, name, 32);
}

void CECAT7MainHeader::setPatient_Sex(const Patient_Sex sex)														
{
	m_Data.Patient_Sex[0] = sex;
}

void CECAT7MainHeader::setPatient_Dexterity(const Patient_Dexterity dex)
{
	m_Data.Patient_Dexterity[0] = dex;
}

void CECAT7MainHeader::setPatient_Age(const float age)																
{
	m_Data.Patient_Age = age;
}

void CECAT7MainHeader::setPatient_Height(const float height)														
{
	m_Data.Patient_Height = height;
}

void CECAT7MainHeader::setPatient_Weight(const float weight)													
{
	m_Data.Patient_Weight = weight;
}

void CECAT7MainHeader::setPatient_Birth_Date(const time_t date)										
{
	m_Data.Patient_Birth_Date = static_cast<Q_INT32>(date);
}

void CECAT7MainHeader::setPhysician_Name(const char* name)															
{
	strncpy(m_Data.Physician_Name, name, 32);
}

void CECAT7MainHeader::setOperator_Name(const char* name)															
{
	strncpy(m_Data.Operator_Name, name, 32);
}

void CECAT7MainHeader::setStudy_Description(const char* descr)												
{
	strncpy(m_Data.Study_Description, descr, 32);
}

void CECAT7MainHeader::setAcquisition_Type(const Acquisition_Type type)								
{
	m_Data.Acquisition_Type = type;
}

void CECAT7MainHeader::setPatient_Orientation(const Patient_Orientation orient)											
{
	m_Data.Patient_Orientation = orient;
}

void CECAT7MainHeader::setFacility_Name(const char* name)															
{
	strncpy(m_Data.Facility_Name, name, 20);
}

void CECAT7MainHeader::setNum_Planes(const short num)																	
{
	m_Data.Num_Planes = num;
}

void CECAT7MainHeader::setNum_Frames(const short num)																	
{
	m_Data.Num_Frames = num;
}

void CECAT7MainHeader::setNum_Gates(const short num)																	
{
	m_Data.Num_Gates = num;
}

void CECAT7MainHeader::setNum_Bed_Pos(const short num)																
{
	m_Data.Num_Bed_Pos = num;
}

void CECAT7MainHeader::setInit_Bed_Position(const float pos)													
{
	m_Data.Init_Bed_Position = pos;
}

void CECAT7MainHeader::setBed_Offset(const short i, const float offset)								
{
	m_Data.Bed_Offset[i] = offset;
}

void CECAT7MainHeader::setPlane_Separation(const float sep)														
{
	m_Data.Plane_Separation = sep;
}

void CECAT7MainHeader::setLwr_Sctr_Thres(const short thres)														
{
	m_Data.Lwr_Sctr_Thres = thres;
}

void CECAT7MainHeader::setLwr_True_Thres(const short thres)														
{
	m_Data.Lwr_True_Thres = thres;
}

void CECAT7MainHeader::setUpr_True_Thres(const short thres)														
{
	m_Data.Upr_True_Thres = thres;
}

void CECAT7MainHeader::setUser_Process_Code(const char* code)													
{
	strncpy(m_Data.User_Process_Code, code, 10);
}

void CECAT7MainHeader::setAcquisition_Mode(const Acquisition_Mode mode)								
{
	m_Data.Acquisition_Mode = mode;
}

void CECAT7MainHeader::setBin_Size(const float size)																	
{
	m_Data.Bin_Size = size;
}

void CECAT7MainHeader::setBranching_Fraction(const float frac)												
{
	m_Data.Branching_Fraction = frac;
}

void CECAT7MainHeader::setDose_Start_Time(const time_t time)										
{
	m_Data.Dose_Start_Time = static_cast<Q_INT32>(time);
}

void CECAT7MainHeader::setDosage(const float dosage)																	
{ 
	m_Data.Dosage = dosage;
}

void CECAT7MainHeader::setWell_Counter_Corr_Factor(const float fac)										
{ 
	m_Data.Well_Counter_Corr_Factor = fac;
}

void CECAT7MainHeader::setData_Units(const char* units)																
{ 
	strncpy(m_Data.Data_Units, units, 32);
}

void CECAT7MainHeader::setSepta_State(const Septa_State state)												
{ 
	m_Data.Septa_State = state;
}

void CECAT7MainHeader::setCTI_Reserved(const short i, const short val)								
{ 
	m_Data.CTI_Reserved[i] = val;
}

 CMedIOHeader& CECAT7MainHeader::copyData(const CMedIOHeader& src)
{
	ENTER();

	// depending on the MedIOHeader format we do have to 
	// distinguish between our copy operations.
	switch(src.headerFormat())
	{
		case CMedIOHeader::ECATMainHeader:
		{
			const CECATMainHeader* eMainHeader = static_cast<const CECATMainHeader*>(&src);

			// depending on the source type we have to copy either every data or just 
			// some data of the src header
			switch(eMainHeader->rtti())
			{
				// if the source header is also an ECAT7 one we can copy it in whole
				// via a simple memcpy()
				case CECATMainHeader::ECAT7MainHeader:
				{
					memcpy(&(this->m_Data), &(static_cast<const CECAT7MainHeader*>(&src)->m_Data), sizeof(struct ECAT7MainHeader));
				}
				break;

				// if this is an ECAT6 Mainheader we have to take care of the fact that
				// some information is missing in one of the headers.
				case CECATMainHeader::ECAT6MainHeader:
				{
					const CECAT6MainHeader* e6src = static_cast<const CECAT6MainHeader*>(&src);
					setOriginal_File_Name(e6src->original_File_Name());
					setSystem_Type(e6src->system_Type());

					#warning "ECAT6->ECAT7 copy not fully implemented yet!"
				}
				break;
			}
			
			// afterwards we have to make sure sensible data is restored
			m_Data.SW_Version = 72; // This header does conform to the ECAT 7.2 standard
		}

		case CMedIOHeader::ECATSubHeader:
		case CMedIOHeader::ConcordeMicroPetFrameHeader:
			// copying a sub header into a main header doesn't make much sense, so we
			// do nothing here
		break;

		case CMedIOHeader::ConcordeMicroPetMainHeader:
		{
			CConcordeMainHeader* head = (CConcordeMainHeader*)&src;
			setOriginal_File_Name(head->fileName().ascii());
			setSystem_Type((short)head->model());
			setScan_Start_Time(head->scanTime());
			setIsotope_Name(head->isotope().ascii());
			setIsotope_Halflife(head->isotopeHalfTime());
			
			setDistance_Scanned(head->radialFov());
			setTransaxial_FOV(head->radialFov());
			setCoin_Samp_Mode(CECAT7MainHeader::NetTrues);
			setCalibration_Factor(head->calibrationFactor());

			if(head->calibrationFactor() == 1.0F)
				setCalibration_Units(CECAT7MainHeader::Uncalibrated);
			else
				setCalibration_Units(CECAT7MainHeader::Calibrated);
			setStudy_Type(head->study().ascii());
			setPatient_ID(head->subjectIdentifier().ascii());
			setPatient_Name(head->subjectIdentifier().ascii());
			setPatient_Sex(CECAT7MainHeader::Sex_Male);
			
			setPatient_Height(head->subjectLength());
			setPatient_Weight(head->subjectWeight()/1000.0);
			setPhysician_Name(head->investigatorName().ascii());
			setOperator_Name(head->operatorName().ascii());
			setStudy_Description(head->studyIdentifier().ascii());
			if(head->acquisitionMode() == CConcordeMainHeader::Emission)
				setAcquisition_Type(CECAT7MainHeader::StaticEmission);
			else
				setAcquisition_Type(CECAT7MainHeader::DynamicEmission);
			switch(head->subjectOrientation())
			{
				case CConcordeMainHeader::UnknownSubjectOrientation: setPatient_Orientation(CECAT7MainHeader::FF_Prone); break;
				case CConcordeMainHeader::FeetFirstProne: setPatient_Orientation(CECAT7MainHeader::FF_Prone); break;
				case CConcordeMainHeader::HeadFirstProne: setPatient_Orientation(CECAT7MainHeader::HF_Prone); break;
				case CConcordeMainHeader::FeetFirstSupine: setPatient_Orientation(CECAT7MainHeader::FF_Supine); break;
				case CConcordeMainHeader::HeadFirstSupine: setPatient_Orientation(CECAT7MainHeader::HF_Supine); break;
				case CConcordeMainHeader::FeetFirstRight: setPatient_Orientation(CECAT7MainHeader::FF_Right); break;
				case CConcordeMainHeader::HeadFirstRight: setPatient_Orientation(CECAT7MainHeader::HF_Right); break;
				case CConcordeMainHeader::FeetFirstLeft: setPatient_Orientation(CECAT7MainHeader::FF_Left); break;
				case CConcordeMainHeader::HeadFirstLeft: setPatient_Orientation(CECAT7MainHeader::HF_Left); break;
			}
			setFacility_Name(head->institution().ascii());
			setNum_Planes(head->zDimension());
			setNum_Frames(head->totalFrames());
			setLwr_True_Thres((short)head->lld());
			setUpr_True_Thres((short)head->uld());
			setBranching_Fraction(head->isotopeBranchingFraction());
			setNum_Gates(1);
			setNum_Bed_Pos(0);
			setDose_Start_Time(head->injectionTime());

			setDosage(head->dose()*1000000.0);

			//check if additional information is available
			if(head->fileObject())
			{
				D("Setting additional information to ECAT7 main header");
				CConcordeFrameHeader* frame;
				((CConcordeFile*)head->fileObject())->readSubHeader(frame, 1);
				
				setInit_Bed_Position(frame->bedOffset());
				setBed_Elevation(frame->verticalBedOffset());
			}
		}
		break;

		case CMedIOHeader::Unknown:
			// for an unknown header type we do nothing
		break;
	}

	LEAVE();
	return *this;
}

QTextStream& operator<<(QTextStream& stream, const CECAT7MainHeader& mHeader)
{
	ENTER();

	// save all header data to the textstream
	stream << qSetPrecision(6);
	stream << "MAGIC_NUMBER "							<< mHeader.m_Data.Magic_Number							<< endl;
	stream << "ORIGINAL_FILE_NAME "				<< mHeader.m_Data.Original_File_Name				<< endl;
	stream << "SW_VERSION "								<< mHeader.m_Data.SW_Version								<< endl;
	stream << "SYSTEM_TYPE "							<< mHeader.m_Data.System_Type								<< endl;
	stream << "FILE_TYPE "								<< mHeader.m_Data.File_Type									<< endl;
	stream << "SERIAL_NUMBER "						<< mHeader.m_Data.Serial_Number							<< endl;
	stream << "SCAN_START_TIME "					<< mHeader.m_Data.Scan_Start_Time						<< endl;
	stream << "ISOTOPE_NAME "							<< mHeader.m_Data.Isotope_Name							<< endl;
	stream << "ISOTOPE_HALFLIFE "					<< mHeader.m_Data.Isotope_Halflife					<< endl;
	stream << "RADIOPHARMACEUTICAL "			<< mHeader.m_Data.Radiopharmaceutical				<< endl;
	stream << "GANTRY_TILT "							<< mHeader.m_Data.Gantry_Tilt								<< endl;
	stream << "ROTATION_TILT "						<< mHeader.m_Data.Gantry_Rotation						<< endl;
	stream << "BED_ELEVATION "						<< mHeader.m_Data.Bed_Elevation							<< endl;
	stream << "INTRINSIC_TILT "						<< mHeader.m_Data.Intrinsic_Tilt						<< endl;
	stream << "WOBBLE_SPEED "							<< mHeader.m_Data.Wobble_Speed							<< endl;
	stream << "TRANSM_SOURCE_TYPE "				<< mHeader.m_Data.Transm_Source_Type				<< endl;
	stream << "DISTANCE_SCANNED "					<< mHeader.m_Data.Distance_Scanned					<< endl;
	stream << "TRANSAXIAL_FOV "						<< mHeader.m_Data.Transaxial_FOV						<< endl;
	stream << "ANGULAR_COMPRESSION "			<< mHeader.m_Data.Angular_Compression				<< endl;
	stream << "COIN_SAMP_MODE "						<< mHeader.m_Data.Coin_Samp_Mode						<< endl;
	stream << "AXIAL_SAMP_MODE "					<< mHeader.m_Data.Axial_Samp_Mode						<< endl;
	stream << "ECAT_CALIBRATION_FACTOR "	<< mHeader.m_Data.Calibration_Factor				<< endl;
	stream << "CALIBRATION_UNITS "				<< mHeader.m_Data.Calibration_Units					<< endl;
	stream << "CALIBRATION_UNITS_LABEL "	<< mHeader.m_Data.Calibration_Units_Label		<< endl;
	stream << "COMPRESSION_CODE "					<< mHeader.m_Data.Compression_Code					<< endl;
	stream << "STUDY_TYPE "								<< mHeader.m_Data.Study_Type								<< endl;
	stream << "PATIENT_ID "								<< mHeader.m_Data.Patient_ID								<< endl;
	stream << "PATIENT_NAME "							<< mHeader.m_Data.Patient_Name							<< endl;
	stream << "PATIENT_SEX "							<< mHeader.m_Data.Patient_Sex[0]						<< endl;
	stream << "PATIENT_DEXTERITY "				<< mHeader.m_Data.Patient_Dexterity[0]			<< endl;
	stream << "PATIENT_AGE "							<< mHeader.m_Data.Patient_Age								<< endl;
	stream << "PATIENT_HEIGHT "						<< mHeader.m_Data.Patient_Height						<< endl;
	stream << "PATIENT_WEIGHT "						<< mHeader.m_Data.Patient_Weight						<< endl;
	stream << "PATIENT_BIRTH_DATE "				<< mHeader.m_Data.Patient_Birth_Date				<< endl;
	stream << "PHYSICIAN_NAME "						<< mHeader.m_Data.Physician_Name						<< endl;
	stream << "OPERATOR_NAME "						<< mHeader.m_Data.Operator_Name							<< endl;
	stream << "STUDY_DESCRIPTION "				<< mHeader.m_Data.Study_Description					<< endl;
	stream << "ACQUISITION_TYPE "					<< mHeader.m_Data.Acquisition_Type					<< endl;
	stream << "PATIENT_ORIENTATION "			<< mHeader.m_Data.Patient_Orientation				<< endl;
	stream << "FACILITY_NAME "						<< mHeader.m_Data.Facility_Name							<< endl;
	stream << "NUM_PLANES "								<< mHeader.m_Data.Num_Planes								<< endl;
	stream << "NUM_FRAMES "								<< mHeader.m_Data.Num_Frames								<< endl;
	stream << "NUM_GATES "								<< mHeader.m_Data.Num_Gates									<< endl;
	stream << "NUM_BED_POS "							<< mHeader.m_Data.Num_Bed_Pos								<< endl;
	stream << "INIT_BED_POSITION "				<< mHeader.m_Data.Init_Bed_Position					<< endl;
	stream << "BED_POSITION";
	for(int i=0; i < 15; i++)
	{
		stream << " " << mHeader.m_Data.Bed_Offset[i];
	}
	stream << endl;
	stream << "PLANE_SEPARATION "					<< mHeader.m_Data.Plane_Separation					<< endl;
	stream << "LWR_SCTR_THRES "						<< mHeader.m_Data.Lwr_Sctr_Thres						<< endl;
	stream << "LWR_TRUE_THRES "						<< mHeader.m_Data.Lwr_True_Thres						<< endl;
	stream <<	"UPR_TRUE_THRES "						<< mHeader.m_Data.Upr_True_Thres						<< endl;
	stream <<	"USER_PROCESS_CODE "				<< mHeader.m_Data.User_Process_Code					<< endl;
	stream << "ACQUISITION_MODE "					<< mHeader.m_Data.Acquisition_Mode					<< endl;
	stream << "BIN_SIZE "									<< mHeader.m_Data.Bin_Size									<< endl;
	stream << "BRANCHING_FRACTION "				<< mHeader.m_Data.Branching_Fraction				<< endl;
	stream << "DOSE_START_TIME "					<< mHeader.m_Data.Dose_Start_Time						<< endl;
	stream <<	"DOSAGE "										<< mHeader.m_Data.Dosage										<< endl;
	stream <<	"WELL_COUNTER_CORR_FACTOR "	<< mHeader.m_Data.Well_Counter_Corr_Factor	<< endl;
	stream << "DATA_UNITS "								<< mHeader.m_Data.Data_Units								<< endl;
	stream <<	"SEPTA_STATE "							<< mHeader.m_Data.Septa_State								<< endl;
	stream <<	"FILL";
	for(int i=0; i < 6; i++)
	{
		stream << " " << mHeader.m_Data.CTI_Reserved[i];
	}
	stream << endl;
	
	RETURN(&stream);
	return stream;
}

QDate CECAT7MainHeader::patient_Birth_Date_Qt(void) const
{
	QDateTime birthDate;

	// unfortunatley the QDateTime class seems not to
	// properly handle time_t values which are normally
	// defined as a signed value. In fact Qt seems to
	// handle that stuff unsigned which makes it impossible
	// to place a negative value to QDateTime
	if(m_Data.Patient_Birth_Date > 0)
		birthDate.setTime_t(m_Data.Patient_Birth_Date);
	else
	{
		// do the calculations on our own with help of POSIX
		// ctime() :(
		time_t dateval = static_cast<time_t>(m_Data.Patient_Birth_Date);

		#if defined(HAVE_CTIME_R)
		char buf[40];
		QString dateString(ctime_r(&dateval, buf));
		#elif defined(HAVE_CTIME)
		#warning "no ctime_r() function used -> libmedio not threadsafe!"
		QString dateString(ctime(&dateval));
		#else
		#error "no ctime_r() or ctime() function found!"
		#endif

		birthDate = QDateTime::fromString(dateString.stripWhiteSpace());
	}

	return birthDate.date();
}

QDateTime CECAT7MainHeader::scan_Start_Time_Qt(void) const
{
	QDateTime scanStartTime;

	// unfortunatley the QDateTime class seems not to
	// properly handle time_t values which are normally
	// defined as a signed value. In fact Qt seems to
	// handle that stuff unsigned which makes it impossible
	// to place a negative value to QDateTime
	if(m_Data.Scan_Start_Time > 0)
		scanStartTime.setTime_t(m_Data.Scan_Start_Time);
	else
	{
		// do the calculations on our own with help of POSIX
		// ctime() :(
		time_t dateval = static_cast<time_t>(m_Data.Scan_Start_Time);

		#if defined(HAVE_CTIME_R)
		char buf[40];
		QString dateString(ctime_r(&dateval, buf));
		#elif defined(HAVE_CTIME)
		#warning "no ctime_r() function used -> libmedio not threadsafe!"
		QString dateString(ctime(&dateval));
		#else
		#error "no ctime_r() or ctime() function found!"
		#endif

		scanStartTime = QDateTime::fromString(dateString.stripWhiteSpace());
	}

	return scanStartTime;
}

QDateTime CECAT7MainHeader::dose_Start_Time_Qt(void) const
{
	QDateTime doseStartTime;

	// unfortunatley the QDateTime class seems not to
	// properly handle time_t values which are normally
	// defined as a signed value. In fact Qt seems to
	// handle that stuff unsigned which makes it impossible
	// to place a negative value to QDateTime
	if(m_Data.Dose_Start_Time > 0)
		doseStartTime.setTime_t(m_Data.Dose_Start_Time);
	else
	{
		// do the calculations on our own with help of POSIX
		// ctime() :(
		time_t dateval = static_cast<time_t>(m_Data.Dose_Start_Time);

		#if defined(HAVE_CTIME_R)
		char buf[40];
		QString dateString(ctime_r(&dateval, buf));
		#elif defined(HAVE_CTIME)
		#warning "no ctime_r() function used -> libmedio not threadsafe!"
		QString dateString(ctime(&dateval));
		#else
		#error "no ctime_r() or ctime() function found!"
		#endif

		doseStartTime = QDateTime::fromString(dateString.stripWhiteSpace());
		
		SHOWSTRING(doseStartTime.toString().ascii());
	}

	return doseStartTime;
}

void CECAT7MainHeader::setPatient_Birth_Date_Qt(const QDate& birthDate)
{
	// unfortunatley the QDateTime class seems not to
	// properly handle time_t values which are normally
	// defined as a signed value. In fact Qt seems to
	// handle that stuff unsigned which makes it impossible
	// to place a negative value to QDateTime
	QDateTime Jan1970(QDate(1970, 1, 1), QTime());
	QDateTime birthDateTime(birthDate, QTime());

	SHOWSTRING(birthDateTime.toString().ascii());
	if(birthDateTime < Jan1970)
	{
		// we have to convert the birthDateTime on our own by
		// using standard POSIX functions.
		struct tm timeVal;
		timeVal.tm_sec   = 0;
		timeVal.tm_min   = 0;
		timeVal.tm_hour  = 0;
		timeVal.tm_mday  = birthDate.day();
		timeVal.tm_mon   = birthDate.month()-1;
		timeVal.tm_year  = birthDate.year()-1900;
		timeVal.tm_wday  = birthDate.dayOfWeek()-1;
		timeVal.tm_yday  = birthDate.dayOfYear()-1;
		timeVal.tm_isdst = 0;

		m_Data.Patient_Birth_Date = mktime(&timeVal);
	}
	else
		m_Data.Patient_Birth_Date = birthDateTime.toTime_t();
}

void CECAT7MainHeader::setScan_Start_Time_Qt(const QDateTime& scanStartTime)
{
	// unfortunatley the QDateTime class seems not to
	// properly handle time_t values which are normally
	// defined as a signed value. In fact Qt seems to
	// handle that stuff unsigned which makes it impossible
	// to place a negative value to QDateTime
	QDateTime Jan1970(QDate(1970, 1, 1), QTime());

	SHOWSTRING(scanStartTime.toString().ascii());
	if(scanStartTime < Jan1970)
	{
		QDate scanDate = scanStartTime.date();
		QTime scanTime = scanStartTime.time();
		
		// we have to convert the birthDateTime on our own by
		// using standard POSIX functions.
		struct tm timeVal;
		timeVal.tm_sec   = scanTime.second();
		timeVal.tm_min   = scanTime.minute();
		timeVal.tm_hour  = scanTime.hour();
		timeVal.tm_mday  = scanDate.day();
		timeVal.tm_mon   = scanDate.month()-1;
		timeVal.tm_year  = scanDate.year()-1900;
		timeVal.tm_wday  = scanDate.dayOfWeek()-1;
		timeVal.tm_yday  = scanDate.dayOfYear()-1;
		timeVal.tm_isdst = 0;

		m_Data.Scan_Start_Time = mktime(&timeVal);
	}
	else
		m_Data.Scan_Start_Time = scanStartTime.toTime_t();	
}

void CECAT7MainHeader::setDose_Start_Time_Qt(const QDateTime& doseStartTime)
{
	// unfortunatley the QDateTime class seems not to
	// properly handle time_t values which are normally
	// defined as a signed value. In fact Qt seems to
	// handle that stuff unsigned which makes it impossible
	// to place a negative value to QDateTime
	QDateTime Jan1970(QDate(1970, 1, 1), QTime());

	SHOWSTRING(doseStartTime.toString().ascii());
	if(doseStartTime < Jan1970)
	{
		QDate doseDate = doseStartTime.date();
		QTime doseTime = doseStartTime.time();
		
		// we have to convert the birthDateTime on our own by
		// using standard POSIX functions.
		struct tm timeVal;
		timeVal.tm_sec   = doseTime.second();
		timeVal.tm_min   = doseTime.minute();
		timeVal.tm_hour  = doseTime.hour();
		timeVal.tm_mday  = doseDate.day();
		timeVal.tm_mon   = doseDate.month()-1;
		timeVal.tm_year  = doseDate.year()-1900;
		timeVal.tm_wday  = doseDate.dayOfWeek()-1;
		timeVal.tm_yday  = doseDate.dayOfYear()-1;
		timeVal.tm_isdst = 0;

		m_Data.Dose_Start_Time = mktime(&timeVal);
	}
	else
		m_Data.Dose_Start_Time = doseStartTime.toTime_t();		
}
