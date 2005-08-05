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

#include "CECAT6MainHeader.h"
#include "CECAT7MainHeader.h"
#include "CECATFile.h"

#include <QDataStream>
#include <QFileInfo>

#include <rtdebug.h>

CECAT6MainHeader::CECAT6MainHeader(const CECAT6MainHeader& mh)
	: CECATMainHeader(mh.m_pMedIOData)
{
	// make 100% sure that the ECAT6 MainHeader is just 512bytes long.
  ASSERT(sizeof(struct ECAT6MainHeader) == 512);
	
	// lets copy the mainHeader data but keep an eye on the file type
	memcpy(&m_Data, &mh.m_Data, sizeof(struct ECAT6MainHeader));
}

CECAT6MainHeader::CECAT6MainHeader(CECATFile* ecatFile,
																	 CECATMainHeader::Type fileType)
	: CECATMainHeader(ecatFile)
{
	// make 100% sure that the ECAT6 MainHeader is just 512bytes long.
  ASSERT(sizeof(struct ECAT6MainHeader) == 512);
	
	// clear our MainHeader structure first
	memset(&m_Data, 0, sizeof(struct ECAT6MainHeader));
	
	// this constructor signals us to create a empty ECAT6MainHeader
	// with prefilled data that is always the same for a ECAT6 main header
	// depending on the supplied fileType we have to have a different Magic
	// Number field

	// default values for our Main Header
	m_Data.SW_Version	= 64;		// use v6.4 as the default ECAT6 version
}

bool CECAT6MainHeader::load(void)
{
	ENTER();

	// only go on if the device is readable at all
	if(m_pMedIOData == NULL ||
		 m_pMedIOData->at(0) == false ||
		 m_pMedIOData->isReadable() == false)
	{
		RETURN(false);
		return false;
	}

	// we use a ByteArray buffer to speed up the endianess
	// decoding
	QByteArray buffer(sizeof(struct ECAT6MainHeader));
	if(m_pMedIOData->readBlock(buffer.data(), 
		 sizeof(struct ECAT6MainHeader)) != sizeof(struct ECAT6MainHeader))
	{
		RETURN(false);
		return false;
	}

	// now we generate a QDataStream on our buffer so that we can read
	// out of the buffer instead of the raw file (> speed)
	QDataStream stream(&buffer, QIODevice::ReadOnly);
	
	// we read in the MainHeader data with one read
	// operation
	stream.readRawBytes(&m_Data.Original_File_Name[0], 20);		//   0: Original_File_Name
  stream >> m_Data.SW_Version;                              //  20: SW_Version
  stream >> m_Data.Data_Type;	                              //  22: Data_Type
  stream >> m_Data.System_Type;                             //  24: System_Type
  stream >> m_Data.File_Type;                               //  26: File_Type
	stream.readRawBytes(&m_Data.Node_ID[0], 10);							//  28: Node_ID
  stream >> m_Data.Scan_Start_Day;                          //  38: Scan_Start_Day
  stream >> m_Data.Scan_Start_Month;                        //  40: Scan_Start_Month
  stream >> m_Data.Scan_Start_Year;                         //  42: Scan_Start_Year
  stream >> m_Data.Scan_Start_Hour;                         //  44: Scan_Start_Hour
  stream >> m_Data.Scan_Start_Minute;                       //  46: Scan_Start_Minute
  stream >> m_Data.Scan_Start_Second;                       //  48: Scan_Start_Minute
	stream.readRawBytes(&m_Data.Isotope_Code[0], 8);					//  50: Isotope_Code
  stream >> m_Data.Isotope_Halflife;	                      //  58: Isotope_Halflife
	stream.readRawBytes(&m_Data.Radiopharmaceutical[0], 32);	//  62: Radiopharmaceutical
  stream >> m_Data.Gantry_Tilt;				                      //  94: Gantry_Tilt
  stream >> m_Data.Gantry_Rotation;		                      // 102: Gantry_Rotation
  stream >> m_Data.Bed_Elevation;			                      // 106: Bed_Elevation
  stream >> m_Data.Rot_Source_Speed;	                      // 110: Rot_Source_Speed
  stream >> m_Data.Wobble_Speed;			                      // 112: Wobble_Speed
  stream >> m_Data.Transm_Source_Type;                      // 114: Transm_Source_Type
  stream >> m_Data.Axial_FOV;					                      // 116: Axial_FOV
  stream >> m_Data.Transaxial_FOV;		                      // 120: Transaxial_FOV
  stream >> m_Data.Coin_Samp_Mode;		                      // 124: Coin_Samp_Mode
  stream >> m_Data.Axial_Samp_Mode;		                      // 126: Axial_Samp_Mode
  stream >> m_Data.Calibration_Factor;                      // 128: Calibration_Factor
  stream >> m_Data.Calibration_Units;												// 132: Calibration_Units
  stream >> m_Data.Compression_Code;												// 134: Compression_Code
	stream.readRawBytes(&m_Data.Study_Name[0], 12);						// 136: Study_Name
	stream.readRawBytes(&m_Data.Patient_ID[0], 16);						// 148: Patient_ID
	stream.readRawBytes(&m_Data.Patient_Name[0], 32);					// 164: Patient_Name
	stream.readRawBytes(&m_Data.Patient_Sex[0], 1);						// 196: Patient_Sex
	stream.readRawBytes(&m_Data.Patient_Age[0], 10);					// 197: Patient_Age
	stream.readRawBytes(&m_Data.Patient_Height[0], 10);				// 207: Patient_Height
	stream.readRawBytes(&m_Data.Patient_Weight[0], 10);				// 217: Patient_Weight
	stream.readRawBytes(&m_Data.Patient_Dexterity[0], 1);			// 227: Patient_Dexterity
	stream.readRawBytes(&m_Data.Physician_Name[0], 32);				// 228: Physician_Name
	stream.readRawBytes(&m_Data.Operator_Name[0], 32);				// 260: Operator_Name
	stream.readRawBytes(&m_Data.Study_Description[0], 32);		// 292: Study_Description
  stream >> m_Data.Acquisition_Type;												// 324: Acquisition_Type
  stream >> m_Data.Bed_Type;																// 326: Bed_Type
  stream >> m_Data.Septa_Type;															// 328: Septa_Type
	stream.readRawBytes(&m_Data.Facility_Name[0], 20);				// 330: Facility_Name
  stream >> m_Data.Num_Planes;															// 350: Num_Planes
  stream >> m_Data.Num_Frames;															// 352: Num_Frames
  stream >> m_Data.Num_Gates;																// 354: Num_Gates
  stream >> m_Data.Num_Bed_Pos;															// 356: Num_Bed_Pos
  stream >> m_Data.Init_Bed_Position;												// 358: Init_Bed_Position
	for(int i=0; i < 15; i++)
		stream >> m_Data.Bed_Offset[i];													// 362: Bed_Offset (15)
  stream >> m_Data.Plane_Separation;												// 422: Plane_Separation
  stream >> m_Data.Lwr_Sctr_Thres;													// 426: Lwr_Sctr_Thres
  stream >> m_Data.Lwr_True_Thres;													// 430: Lwr_True_Thres
  stream >> m_Data.Upr_True_Thres;													// 432: Upr_True_Thres
  stream >> m_Data.Collimator;															// 434: Collimator
  stream.readRawBytes(&m_Data.User_Process_Code[0], 10);		// 438: User_Process_Code
  stream >> m_Data.Acquisition_Mode;												// 448: Acquisition_Mode
	
	// some more debug output
#if defined(DEBUG)
	D("ECAT6 Main Header loaded:");
	D("------------------------");
	D("Original_File_Name      : %s",  	 	 	 	    m_Data.Original_File_Name);
	D("SW_Version              : %d",  	 	 	 	    m_Data.SW_Version);
	D("Data_Type               : %d",  	 	 	 	    m_Data.Data_Type);
	D("System_Type             : %d",  	 	 	 	    m_Data.System_Type);
	D("File_Type               : %d",  	 	 	 	    m_Data.File_Type);
	D("Node_ID                 : %s",  	 	 	 	    m_Data.Node_ID);
	D("Scan_Start_Day          : %d",  	 	 	 	    m_Data.Scan_Start_Day);
	D("Scan_Start_Month        : %d",  	 	 	 	    m_Data.Scan_Start_Month);
	D("Scan_Start_Year         : %d",  	 	 	 	    m_Data.Scan_Start_Year);
	D("Scan_Start_Hour         : %d",  	 	 	 	    m_Data.Scan_Start_Hour);
	D("Scan_Start_Minute       : %d",  	 	 	 	    m_Data.Scan_Start_Minute);
	D("Scan_Start_Second       : %d",  	 	 	 	    m_Data.Scan_Start_Second);
	D("Isotope Name            : %s",  	 	 	 	    m_Data.Isotope_Code);
	D("Isotope Halflife        : %f sec",				  m_Data.Isotope_Halflife);
	D("Radiopharmaceutical     : %s",			 	 		  m_Data.Radiopharmaceutical);
	D("Gantry Tilt             : %f°",					  m_Data.Gantry_Tilt);
	D("Gantry Rotation         : %f°",			 	 	  m_Data.Gantry_Rotation);
	D("Bed elevation           : %f cm",		 	 	  m_Data.Bed_Elevation);
	D("Rot_Source_Speed        : %d",  	 	 	 	    m_Data.Rot_Source_Speed);
	D("Wobble_Speed            : %d",  	 	 	 	    m_Data.Wobble_Speed);
	D("Transm_Source_Type      : %d",  	 	 	 	    m_Data.Transm_Source_Type);
	D("Axial_FOV               : %f cm",  			  m_Data.Axial_FOV);
	D("Transaxial_FOV          : %f cm",  			  m_Data.Transaxial_FOV);
	D("Transaxial_Samp_Mode    : %d",							m_Data.Transaxial_Samp_Mode);
	D("Coin_Samp_Mode          : %d",							m_Data.Coin_Samp_Mode);
	D("Axial_Samp_Mode         : %d",							m_Data.Axial_Samp_Mode);
	D("Calibration_Factor      : %f",     			  m_Data.Calibration_Factor);
	D("Calibration_Units       : %d",     			  m_Data.Calibration_Units);
	D("Compression Code        : %d",							m_Data.Compression_Code);
	D("Study_Name              : %s",							m_Data.Study_Name);
	D("Patient ID              : %s",							m_Data.Patient_ID);
	D("Patient Name            : %s",							m_Data.Patient_Name);
	D("Patient Sex             : %c",							m_Data.Patient_Sex[0]);
	D("Patient Age             : %f years",				m_Data.Patient_Age);
	D("Patient Height          : %f cm",					m_Data.Patient_Height);
	D("Patient Weight          : %f kg",					m_Data.Patient_Weight);
	D("Patient Dexterity       : %c",							m_Data.Patient_Dexterity[0]);
	D("Physician Name          : %s",							m_Data.Physician_Name);
	D("Operator Name           : %s",							m_Data.Operator_Name);
	D("Study Description       : %s",							m_Data.Study_Description);
	D("Acquisition Type        : %d",							m_Data.Acquisition_Type);
	D("Bed_Type                : %d",							m_Data.Bed_Type);
	D("Septa_Type              : %d",							m_Data.Septa_Type);
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
	D("Plane separation        : %f cm",					m_Data.Plane_Separation);
	D("Lowest Threshold Scatter: %d KeV",					m_Data.Lwr_Sctr_Thres);
	D("Lower Threshold Trues   : %d KeV",					m_Data.Lwr_True_Thres);
	D("Upper Threshold Trues   : %d KeV",					m_Data.Upr_True_Thres);
	D("Collimator              : %f",							m_Data.Collimator);
	D("User_Process_Code       : %s",							m_Data.User_Process_Code);
	D("Acquisition_Mode        : %d",							m_Data.Acquisition_Mode);
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
		 m_pMedIOData->at(0) == false)
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
	QByteArray buffer(sizeof(struct ECAT6MainHeader));
	QDataStream stream(&buffer, QIODevice::WriteOnly);

	// we read in the MainHeader data with one read
	// operation
	stream.writeRawBytes(&m_Data.Original_File_Name[0], 20);	//   0: Original_File_Name
  stream << m_Data.SW_Version;                              //  20: SW_Version
  stream << m_Data.Data_Type;	                              //  22: Data_Type
  stream << m_Data.System_Type;                             //  24: System_Type
  stream << m_Data.File_Type;                               //  26: File_Type
	stream.writeRawBytes(&m_Data.Node_ID[0], 10);							//  28: Node_ID
  stream << m_Data.Scan_Start_Day;                          //  38: Scan_Start_Day
  stream << m_Data.Scan_Start_Month;                        //  40: Scan_Start_Month
  stream << m_Data.Scan_Start_Year;                         //  42: Scan_Start_Year
  stream << m_Data.Scan_Start_Hour;                         //  44: Scan_Start_Hour
  stream << m_Data.Scan_Start_Minute;                       //  46: Scan_Start_Minute
  stream << m_Data.Scan_Start_Second;                       //  48: Scan_Start_Minute
	stream.writeRawBytes(&m_Data.Isotope_Code[0], 8);					//  50: Isotope_Code
  stream << m_Data.Isotope_Halflife;	                      //  58: Isotope_Halflife
	stream.writeRawBytes(&m_Data.Radiopharmaceutical[0], 32);	//  62: Radiopharmaceutical
  stream << m_Data.Gantry_Tilt;				                      //  94: Gantry_Tilt
  stream << m_Data.Gantry_Rotation;		                      // 102: Gantry_Rotation
  stream << m_Data.Bed_Elevation;			                      // 106: Bed_Elevation
  stream << m_Data.Rot_Source_Speed;	                      // 110: Rot_Source_Speed
  stream << m_Data.Wobble_Speed;			                      // 112: Wobble_Speed
  stream << m_Data.Transm_Source_Type;                      // 114: Transm_Source_Type
  stream << m_Data.Axial_FOV;					                      // 116: Axial_FOV
  stream << m_Data.Transaxial_FOV;		                      // 120: Transaxial_FOV
  stream << m_Data.Coin_Samp_Mode;		                      // 124: Coin_Samp_Mode
  stream << m_Data.Axial_Samp_Mode;		                      // 126: Axial_Samp_Mode
  stream << m_Data.Calibration_Factor;                      // 128: Calibration_Factor
  stream << m_Data.Calibration_Units;												// 132: Calibration_Units
  stream << m_Data.Compression_Code;												// 134: Compression_Code
	stream.writeRawBytes(&m_Data.Study_Name[0], 12);					// 136: Study_Name
	stream.writeRawBytes(&m_Data.Patient_ID[0], 16);					// 148: Patient_ID
	stream.writeRawBytes(&m_Data.Patient_Name[0], 32);				// 164: Patient_Name
	stream.writeRawBytes(&m_Data.Patient_Sex[0], 1);					// 196: Patient_Sex
	stream.writeRawBytes(&m_Data.Patient_Age[0], 10);					// 197: Patient_Age
	stream.writeRawBytes(&m_Data.Patient_Height[0], 10);			// 207: Patient_Height
	stream.writeRawBytes(&m_Data.Patient_Weight[0], 10);			// 217: Patient_Weight
	stream.writeRawBytes(&m_Data.Patient_Dexterity[0], 1);		// 227: Patient_Dexterity
	stream.writeRawBytes(&m_Data.Physician_Name[0], 32);			// 228: Physician_Name
	stream.writeRawBytes(&m_Data.Operator_Name[0], 32);				// 260: Operator_Name
	stream.writeRawBytes(&m_Data.Study_Description[0], 32);		// 292: Study_Description
  stream << m_Data.Acquisition_Type;												// 324: Acquisition_Type
  stream << m_Data.Bed_Type;																// 326: Bed_Type
	stream << m_Data.Septa_Type;															// 328: Septa_Type
	stream.writeRawBytes(&m_Data.Facility_Name[0], 20);				// 330: Facility_Name
	stream << numPlanes;																			// 350: Num_Planes
	stream << numFrames;																			// 352: Num_Frames
	stream << numGates;																				// 354: Num_Gates
	stream << numBedPos;																			// 356: Num_Bed_Pos
	stream << m_Data.Init_Bed_Position;												// 358: Init_Bed_Position
	for(int i=0; i < 15; i++)
		stream << m_Data.Bed_Offset[i];													// 362: Bed_Offset (15)
  stream << m_Data.Plane_Separation;												// 422: Plane_Separation
	stream << m_Data.Lwr_Sctr_Thres;													// 426: Lwr_Sctr_Thres
	stream << m_Data.Lwr_True_Thres;													// 430: Lwr_True_Thres
	stream << m_Data.Upr_True_Thres;													// 432: Upr_True_Thres
	stream << m_Data.Collimator;															// 434: Collimator
	stream.writeRawBytes(&m_Data.User_Process_Code[0], 10);		// 438: User_Process_Code
	stream << m_Data.Acquisition_Mode;												// 448: Acquisition_Mode

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

CMedIOHeader& CECAT6MainHeader::copyData(const CMedIOHeader& src)
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
				// if the source header is also an ECAT6 one we can copy it in whole
				// via a simple memcpy()
				case CECATMainHeader::ECAT6MainHeader:
				{
					memcpy(&(this->m_Data), &(static_cast<const CECAT6MainHeader*>(&src)->m_Data), sizeof(struct ECAT6MainHeader));
				}
				break;

				// if this is an ECAT7 Mainheader we have to take care of the fact that
				// some information is missing in one of the headers.
				case CECATMainHeader::ECAT7MainHeader:
				{
					const CECAT7MainHeader* e7src = static_cast<const CECAT7MainHeader*>(&src);
					setOriginal_File_Name(e7src->original_File_Name());
					setSystem_Type(e7src->system_Type());

					#warning "ECAT7->ECAT6 copy not fully implemented yet!"
				}
				break;
			}
	
			// afterwards we have to make sure sensible data is restored
			m_Data.SW_Version = 60; // This header does conform to the ECAT 6.0 standard
		}

		case CMedIOHeader::ECATSubHeader:
			// copying a sub header into a main header doesn't make much sense, so we
			// do nothing here
		break;

		case CMedIOHeader::ConcordeMicropet:
		{
			#warning "Concorde->ECAT6 copy missing"
		}
		break;

		case CMedIOHeader::Unknown:
			// for an unknown header type we do nothing
		break;
	}
	
	LEAVE();
	return *this;
}
