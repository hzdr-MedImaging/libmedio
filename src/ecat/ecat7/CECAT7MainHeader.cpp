/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004 by Jens Langner <Jens.Langner@light-speed.de>

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
#include "CECATFile.h"

#include <qdatastream.h>
#include <qdatetime.h>
#include <qfileinfo.h>

#include <unistd.h>

#include "scanner.h"
#include "debug.h"

CECAT7MainHeader::CECAT7MainHeader(CECATMainHeader::Type fileType)
	: CECATMainHeader()
{
	// make 100% sure that the ECAT7 MainHeader is just 512bytes long.
  ASSERT(sizeof(struct ECAT7MainHeader) == 512);
	
	// clear our MainHeader structure first
	memset(&m_Data, 0, sizeof(struct ECAT7MainHeader));
	
	// this constructor signals us to create a empty ECAT7MainHeader
	// with prefilled data that is always the same for a ECAT7 main header
	// depending on the supplied fileType we have to have a different Magic
	// Number field

	// default values for our Main Header
	m_Data.SW_Version						= 72;		// use v7.2 as the default ECAT version
	m_Data.System_Type					= 962;	// set ECAT Model 962 (EXACT HR+) by default
	m_Data.Scan_Start_Time			= time(NULL);
	m_Data.Isotope_Halflife			= 6586.2;
	m_Data.Distance_Scanned			= 15.52;
	m_Data.Transaxial_FOV				= 58.3;
	m_Data.Angular_Compression	= static_cast<short>(Mash2); // 0=no mash, 1=mash 2, 2=mash 4
	m_Data.Acquisition_Type			= static_cast<short>(DynamicEmission);
	m_Data.Acquisition_Mode			= static_cast<short>(Windowed);
	m_Data.Transm_Source_Type		= static_cast<short>(SRC_RING);
	m_Data.Lwr_True_Thres				= 350;
	m_Data.Upr_True_Thres				= 650;
	m_Data.Patient_Orientation	= 3;		// Head first
	m_Data.Branching_Fraction		= 1.0;
	m_Data.Plane_Separation			= SCANNER_PLANE_DEPTH/10.0;
	m_Data.Bin_Size							= SCANNER_X_RESOLUTION;
	m_Data.Septa_State					= static_cast<short>(Retracted);

	setFileType(fileType);
	strcpy(m_Data.Serial_Number, "1");
	strcpy(m_Data.Isotope_Name, "F-18");
	strcpy(m_Data.Radiopharmaceutical, "FDG");
	strcpy(m_Data.Study_Description, "created by libecat++ v0.3");
}

void CECAT7MainHeader::setStudyData(const CECAT7MainHeader& mh)
{
	// lets gather all study relevant information from the supplied
	// main header object and copy it into our own data
	setScan_Start_Time(mh.scan_Start_Time());
	setIsotope_Name(mh.isotope_Name());
	setIsotope_Halflife(mh.isotope_Halflife());
	setRadiopharmaceutical(mh.radiopharmaceutical());
	setBed_Elevation(mh.bed_Elevation());
	setDistance_Scanned(mh.distance_Scanned());
	setStudy_Type(mh.study_Type());
	setPatient_ID(mh.patient_ID());
	setPatient_Name(mh.patient_Name());
	setPatient_Sex(mh.patient_Sex());
	setPatient_Dexterity(mh.patient_Dexterity());
	setPatient_Age(mh.patient_Age());
	setPatient_Height(mh.patient_Height());
	setPatient_Weight(mh.patient_Weight());
	setPatient_Birth_Date(mh.patient_Birth_Date());
	setPhysician_Name(mh.physician_Name());
	setOperator_Name(mh.operator_Name());
	setStudy_Description(mh.study_Description());
	setPatient_Orientation(mh.patient_Orientation());
	setFacility_Name(mh.facility_Name());
	setUser_Process_Code(mh.user_Process_Code());
	setDose_Start_Time(mh.dose_Start_Time());
	setDosage(mh.dosage());
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

bool CECAT7MainHeader::load(CECATFile* pFile)
{
	// open the ECAT7 file and get out our Main Header
	// information
	if(pFile->open(IO_ReadOnly))
	{
		QDataStream stream(pFile);

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
		for(int i=0; i < 15; i++)
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
		D("Scan_Start_Time         : %s",  	 	 	 	    time.toString().ascii());
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
		D("Patient Birthdate       : %s",							time.toString().ascii());
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
		D("Dose start time         : %s",							time.toString().ascii());
		D("Dosage                  : %f Bq/cc",				m_Data.Dosage);
		D("Well counter corr factor: %f",							m_Data.Well_Counter_Corr_Factor);
		D("Data units              : %s",							m_Data.Data_Units);
		D("Septa state             : %d",							m_Data.Septa_State);
		D("CTI reserved            : %lx",						m_Data.CTI_Reserved);
#endif

		pFile->close();

		return true;
	}

	return false;
}

bool CECAT7MainHeader::load(QTextStream& stream)
{
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
				strncpy(m_Data.Magic_Number, dataString.ascii(), 14);
			else if(typeString == "ORIGINAL_FILE_NAME")
				strncpy(m_Data.Original_File_Name, dataString.ascii(), 32);
			else if(typeString == "SW_VERSION")
				m_Data.SW_Version = dataString.toShort(&convertSuccess);
			else if(typeString == "SYSTEM_TYPE")
				m_Data.System_Type = dataString.toShort(&convertSuccess);
			else if(typeString == "FILE_TYPE")
				m_Data.File_Type = dataString.toShort(&convertSuccess);
			else if(typeString == "SERIAL_NUMBER")
				strncpy(m_Data.Serial_Number, dataString.ascii(), 10);
			else if(typeString == "SCAN_START_TIME")
				m_Data.Scan_Start_Time = dataString.toLong(&convertSuccess);
			else if(typeString == "ISOTOPE_NAME")
				strncpy(m_Data.Isotope_Name, dataString.ascii(), 8);
			else if(typeString == "ISOTOPE_HALFLIFE")
				m_Data.Isotope_Halflife = dataString.toFloat(&convertSuccess);
			else if(typeString == "RADIOPHARMACEUTICAL")
				strncpy(m_Data.Radiopharmaceutical, dataString.ascii(), 32);
			else if(typeString == "GANTRY_TILT")
				m_Data.Gantry_Tilt = dataString.toFloat(&convertSuccess);
			else if(typeString == "ROTATION_TILT")
				m_Data.Gantry_Rotation = dataString.toFloat(&convertSuccess);
			else if(typeString == "BED_ELEVATION")
				m_Data.Bed_Elevation = dataString.toFloat(&convertSuccess);
			else if(typeString == "INTRINSIC_TILT")
				m_Data.Intrinsic_Tilt = dataString.toFloat(&convertSuccess);
			else if(typeString == "WOBBLE_SPEED")
				m_Data.Wobble_Speed = dataString.toShort(&convertSuccess);
			else if(typeString == "TRANSM_SOURCE_TYPE")
				m_Data.Transm_Source_Type = dataString.toShort(&convertSuccess);
			else if(typeString == "DISTANCE_SCANNED")
				m_Data.Distance_Scanned = dataString.toFloat(&convertSuccess);
			else if(typeString == "TRANSAXIAL_FOV")
				m_Data.Transaxial_FOV = dataString.toFloat(&convertSuccess);
			else if(typeString == "ANGULAR_COMPRESSION")
				m_Data.Angular_Compression = dataString.toShort(&convertSuccess);
			else if(typeString == "COIN_SAMP_MODE")
				m_Data.Coin_Samp_Mode = dataString.toShort(&convertSuccess);
			else if(typeString == "AXIAL_SAMP_MODE")
				m_Data.Axial_Samp_Mode = dataString.toShort(&convertSuccess);
			else if(typeString == "ECAT_CALIBRATION_FACTOR")
				m_Data.Calibration_Factor = dataString.toFloat(&convertSuccess);
			else if(typeString == "CALIBRATION_UNITS")
				m_Data.Calibration_Units = dataString.toShort(&convertSuccess);
			else if(typeString == "CALIBRATION_UNITS_LABEL")
				m_Data.Calibration_Units_Label = dataString.toShort(&convertSuccess);
			else if(typeString == "COMPRESSION_CODE")
				m_Data.Compression_Code = dataString.toShort(&convertSuccess);
			else if(typeString == "STUDY_TYPE")
				strncpy(m_Data.Study_Type, dataString.ascii(), 12);
			else if(typeString == "PATIENT_ID")
				strncpy(m_Data.Patient_ID, dataString.ascii(), 16);
			else if(typeString == "PATIENT_NAME")
				strncpy(m_Data.Patient_Name, dataString.ascii(), 32);
			else if(typeString == "PATIENT_SEX")
				m_Data.Patient_Sex[0] = dataString.ascii()[0];
			else if(typeString == "PATIENT_DEXTERITY")
				m_Data.Patient_Dexterity[0] = dataString.ascii()[0];
			else if(typeString == "PATIENT_AGE")
				m_Data.Patient_Age = dataString.toFloat(&convertSuccess);
			else if(typeString == "PATIENT_HEIGHT")
				m_Data.Patient_Height = dataString.toFloat(&convertSuccess);
			else if(typeString == "PATIENT_WEIGHT")
				m_Data.Patient_Weight = dataString.toFloat(&convertSuccess);
			else if(typeString == "PATIENT_BIRTH_DATE")
				m_Data.Patient_Birth_Date = dataString.toLong(&convertSuccess);
			else if(typeString == "PHYSICIAN_NAME")
				strncpy(m_Data.Physician_Name, dataString.ascii(), 32);
			else if(typeString == "OPERATOR_NAME")
				strncpy(m_Data.Operator_Name, dataString.ascii(), 32);
			else if(typeString == "STUDY_DESCRIPTION")
				strncpy(m_Data.Study_Description, dataString.ascii(), 32);
			else if(typeString == "ACQUISITION_TYPE")
				m_Data.Acquisition_Type = dataString.toShort(&convertSuccess);
			else if(typeString == "PATIENT_ORIENTATION")
				m_Data.Patient_Orientation = dataString.toShort(&convertSuccess);
			else if(typeString == "FACILITY_NAME")
				strncpy(m_Data.Facility_Name, dataString.ascii(), 20);
			else if(typeString == "NUM_PLANES")
				m_Data.Num_Planes = dataString.toShort(&convertSuccess);
			else if(typeString == "NUM_FRAMES")
				m_Data.Num_Frames = dataString.toShort(&convertSuccess);
			else if(typeString == "NUM_GATES")
				m_Data.Num_Gates = dataString.toShort(&convertSuccess);
			else if(typeString == "NUM_BED_POS")
				m_Data.Num_Bed_Pos = dataString.toShort(&convertSuccess);
			else if(typeString == "INIT_BED_POSITION")
				m_Data.Init_Bed_Position = dataString.toFloat(&convertSuccess);
			else if(typeString == "BED_POSITION")
			{
				for(int i=0; i < 15 && convertSuccess; i++)
				{
					QString subString = dataString.section(" ", i, i);
					if(subString.isEmpty())
						break;

					m_Data.Bed_Offset[i] = subString.toFloat(&convertSuccess);
				}
			}
			else if(typeString == "PLANE_SEPARATION")
				m_Data.Plane_Separation = dataString.toFloat(&convertSuccess);
			else if(typeString == "LWR_SCTR_THRES")
				m_Data.Lwr_Sctr_Thres = dataString.toShort(&convertSuccess);
			else if(typeString == "LWR_TRUE_THRES")
				m_Data.Lwr_True_Thres = dataString.toShort(&convertSuccess);
			else if(typeString == "UPR_TRUE_THRES")
				m_Data.Upr_True_Thres = dataString.toShort(&convertSuccess);
			else if(typeString == "USER_PROCESS_CODE")
				strncpy(m_Data.User_Process_Code, dataString.ascii(), 10);
			else if(typeString == "ACQUISITION_MODE")
				m_Data.Acquisition_Mode = dataString.toShort(&convertSuccess);
			else if(typeString == "BIN_SIZE")
				m_Data.Bin_Size = dataString.toFloat(&convertSuccess);
			else if(typeString == "BRANCHING_FRACTION")
				m_Data.Branching_Fraction = dataString.toFloat(&convertSuccess);
			else if(typeString == "DOSE_START_TIME")
				m_Data.Dose_Start_Time = dataString.toLong(&convertSuccess);
			else if(typeString == "DOSAGE")
				m_Data.Dosage = dataString.toFloat(&convertSuccess);
			else if(typeString == "WELL_COUNTER_CORR_FACTOR")
				m_Data.Well_Counter_Corr_Factor = dataString.toFloat(&convertSuccess);
			else if(typeString == "DATA_UNITS")
				strncpy(m_Data.Data_Units, dataString.ascii(), 32);
			else if(typeString == "SEPTA_STATE")
				m_Data.Septa_State = dataString.toShort(&convertSuccess);
			else if(typeString == "FILL")
			{
				for(int i=0; i < 6 && convertSuccess; i++)
				{
					QString subString = dataString.section(" ", i, i);
					if(subString.isEmpty())
						break;

					m_Data.CTI_Reserved[i] = subString.toShort(&convertSuccess);
				}
			}

			if(convertSuccess == false)
			{
				E("'%s' - error while converting string '%s' to a numerical value.", typeString.ascii(), dataString.ascii());
				success = false;
			}
		}
	}

	return success;
}

bool CECAT7MainHeader::save(CECATFile* pFile)
{
	// open the ECAT7 file to write out our main header
	// to the ecat file.
	if(pFile->open(IO_WriteOnly))
	{
		QDataStream stream(pFile);

		// before we write out the MainHeader we set the OriginalFilename
		// to the current one
		strncpy(m_Data.Original_File_Name, QFileInfo(pFile->name()).fileName().ascii(), 32);

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
    stream << m_Data.Num_Frames;															// 354: Num_Frames
    stream << m_Data.Num_Gates;																// 356: Num_Gates
    stream << m_Data.Num_Bed_Pos;															// 358: Num_Bed_Pos
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

		// and then close it again
		pFile->close();

		return true;
	}

	return false;
}

bool CECAT7MainHeader::save(QTextStream& stream)
{
	bool success = true;

	// save all header data to the textstream
	stream << qSetPrecision(6);
	stream << "MAGIC_NUMBER "							<< m_Data.Magic_Number							<< endl;
	stream << "ORIGINAL_FILE_NAME "				<< m_Data.Original_File_Name				<< endl;
	stream << "SW_VERSION "								<< m_Data.SW_Version								<< endl;
	stream << "SYSTEM_TYPE "							<< m_Data.System_Type								<< endl;
	stream << "FILE_TYPE "								<< m_Data.File_Type									<< endl;
	stream << "SERIAL_NUMBER "						<< m_Data.Serial_Number							<< endl;
	stream << "SCAN_START_TIME "					<< m_Data.Scan_Start_Time						<< endl;
	stream << "ISOTOPE_NAME "							<< m_Data.Isotope_Name							<< endl;
	stream << "ISOTOPE_HALFLIFE "					<< m_Data.Isotope_Halflife					<< endl;
	stream << "RADIOPHARMACEUTICAL "			<< m_Data.Radiopharmaceutical				<< endl;
	stream << "GANTRY_TILT "							<< m_Data.Gantry_Tilt								<< endl;
	stream << "ROTATION_TILT "						<< m_Data.Gantry_Rotation						<< endl;
	stream << "BED_ELEVATION "						<< m_Data.Bed_Elevation							<< endl;
	stream << "INTRINSIC_TILT "						<< m_Data.Intrinsic_Tilt						<< endl;
	stream << "WOBBLE_SPEED "							<< m_Data.Wobble_Speed							<< endl;
	stream << "TRANSM_SOURCE_TYPE "				<< m_Data.Transm_Source_Type				<< endl;
	stream << "DISTANCE_SCANNED "					<< m_Data.Distance_Scanned					<< endl;
	stream << "TRANSAXIAL_FOV "						<< m_Data.Transaxial_FOV						<< endl;
	stream << "ANGULAR_COMPRESSION "			<< m_Data.Angular_Compression				<< endl;
	stream << "COIN_SAMP_MODE "						<< m_Data.Coin_Samp_Mode						<< endl;
	stream << "AXIAL_SAMP_MODE "					<< m_Data.Axial_Samp_Mode						<< endl;
	stream << "ECAT_CALIBRATION_FACTOR "	<< m_Data.Calibration_Factor				<< endl;
	stream << "CALIBRATION_UNITS "				<< m_Data.Calibration_Units					<< endl;
	stream << "CALIBRATION_UNITS_LABEL "	<< m_Data.Calibration_Units_Label		<< endl;
	stream << "COMPRESSION_CODE "					<< m_Data.Compression_Code					<< endl;
	stream << "STUDY_TYPE "								<< m_Data.Study_Type								<< endl;
	stream << "PATIENT_ID "								<< m_Data.Patient_ID								<< endl;
	stream << "PATIENT_NAME "							<< m_Data.Patient_Name							<< endl;
	stream << "PATIENT_SEX "							<< m_Data.Patient_Sex[0]						<< endl;
	stream << "PATIENT_DEXTERITY "				<< m_Data.Patient_Dexterity[0]			<< endl;
	stream << "PATIENT_AGE "							<< m_Data.Patient_Age								<< endl;
	stream << "PATIENT_HEIGHT "						<< m_Data.Patient_Height						<< endl;
	stream << "PATIENT_WEIGHT "						<< m_Data.Patient_Weight						<< endl;
	stream << "PATIENT_BIRTH_DATE "				<< m_Data.Patient_Birth_Date				<< endl;
	stream << "PHYSICIAN_NAME "						<< m_Data.Physician_Name						<< endl;
	stream << "OPERATOR_NAME "						<< m_Data.Operator_Name							<< endl;
	stream << "STUDY_DESCRIPTION "				<< m_Data.Study_Description					<< endl;
	stream << "ACQUISITION_TYPE "					<< m_Data.Acquisition_Type					<< endl;
	stream << "PATIENT_ORIENTATION "			<< m_Data.Patient_Orientation				<< endl;
	stream << "FACILITY_NAME "						<< m_Data.Facility_Name							<< endl;
	stream << "NUM_PLANES "								<< m_Data.Num_Planes								<< endl;
	stream << "NUM_FRAMES "								<< m_Data.Num_Frames								<< endl;
	stream << "NUM_GATES "								<< m_Data.Num_Gates									<< endl;
	stream << "NUM_BED_POS "							<< m_Data.Num_Bed_Pos								<< endl;
	stream << "INIT_BED_POSITION "				<< m_Data.Init_Bed_Position					<< endl;
	stream << "BED_POSITION";
	for(int i=0; i < 15; i++)
	{
		stream << " " << m_Data.Bed_Offset[i];
	}
	stream << endl;
	stream << "PLANE_SEPARATION "					<< m_Data.Plane_Separation					<< endl;
	stream << "LWR_SCTR_THRES "						<< m_Data.Lwr_Sctr_Thres						<< endl;
	stream << "LWR_TRUE_THRES "						<< m_Data.Lwr_True_Thres						<< endl;
	stream <<	"UPR_TRUE_THRES "						<< m_Data.Upr_True_Thres						<< endl;
	stream <<	"USER_PROCESS_CODE "				<< m_Data.User_Process_Code					<< endl;
	stream << "ACQUISITION_MODE "					<< m_Data.Acquisition_Mode					<< endl;
	stream << "BIN_SIZE "									<< m_Data.Bin_Size									<< endl;
	stream << "BRANCHING_FRACTION "				<< m_Data.Branching_Fraction				<< endl;
	stream << "DOSE_START_TIME "					<< m_Data.Dose_Start_Time						<< endl;
	stream <<	"DOSAGE "										<< m_Data.Dosage										<< endl;
	stream <<	"WELL_COUNTER_CORR_FACTOR "	<< m_Data.Well_Counter_Corr_Factor	<< endl;
	stream << "DATA_UNITS "								<< m_Data.Data_Units								<< endl;
	stream <<	"SEPTA_STATE "							<< m_Data.Septa_State								<< endl;
	stream <<	"FILL";
	for(int i=0; i < 6; i++)
	{
		stream << " " << m_Data.CTI_Reserved[i];
	}
	stream << endl;
	
	return success;
}
