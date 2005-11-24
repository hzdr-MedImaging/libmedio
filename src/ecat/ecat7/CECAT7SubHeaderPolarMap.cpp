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

#include "CECAT7SubHeaderPolarMap.h"
#include "CECATDirectoryItem.h"
#include "CECATFile.h"

#include <qdatastream.h>

#include <rtdebug.h>

CECAT7SubHeaderPolarMap::CECAT7SubHeaderPolarMap(CECATFile* ecatFile,
																								 CECATDirectoryItem* pDirItem)
	: CECATSubHeader(ecatFile, pDirItem)
{
	// then clear the structure
	memset(&m_Data, 0, sizeof(struct ECAT7SubHeader_PolarMap));			
}

CECAT7SubHeaderPolarMap::CECAT7SubHeaderPolarMap(const CECAT7SubHeaderPolarMap& sh)
	: CECATSubHeader(sh)
{
	// then copy the structure
	memcpy(&m_Data, &sh.m_Data, sizeof(struct ECAT7SubHeader_PolarMap));			
}

bool CECAT7SubHeaderPolarMap::load(void)
{
	ENTER();

	// check if the stream is readable or not and
	// set our MedIOData to the correct file position so that we can
	// read the subheader
	if(m_pMedIOData->isReadable() == false ||
		 m_pDirItem->dataBlock_Start() == 0 ||
		 m_pMedIOData->at(m_pDirItem->dataBlock_Start()) == false)
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

	// lets read in each single data element of our
	// data structure to maintain the correct endianess of the
	// data
	stream >> m_Data.Data_Type;															//   0: Data_Type
	stream >> m_Data.Polar_Map_Type;												//   2: Polar_Map_Type
	stream >> m_Data.Num_Rings;															//   4: Num_Rings
	for(int i=0; i < 32; i++)
		stream >> m_Data.Sectors_Per_Ring[i];									//   6: Sectors_Per_Ring (32)
	for(int i=0; i < 32; i++)
		stream >> m_Data.Ring_Position[i];										//  70: Ring_Position
	for(int i=0; i < 32; i++)
		stream >> m_Data.Ring_Angle[i];												// 198: Ring_Angle
	stream >> m_Data.Start_Angle;														// 262: Start_Angle
	for(int i=0; i < 3; i++)
		stream >> m_Data.Long_Axis_Left[i];										// 264: Long_Axis_Left
	for(int i=0; i < 3; i++)
		stream >> m_Data.Long_Axis_Right[i];									// 270: Long_Axis_Right
	stream >> m_Data.Position_Data;													// 276: Position_Data
	stream >> m_Data.Image_Min;															// 278: Image_Min
	stream >> m_Data.Image_Max;															// 280: Image_Max
	stream >> m_Data.Scale_Factor;													// 282: Scale_Factor
	stream >> m_Data.Pixel_Size;														// 286: Pixel_Size
	stream >> m_Data.Frame_Duration;												// 290: Frame_Duration
	stream >> m_Data.Frame_Start_Time;											// 294: Frame_Start_Time
	stream >> m_Data.Processing_Code;												// 298: Processing_Code
	stream >> m_Data.Quant_Units;														// 300: Quant_Units
	stream.readRawBytes(&m_Data.Annotation[0], 40);					// 302: Annotation
	stream >> m_Data.Gate_Duration;													// 342: Gate_Duration
	stream >> m_Data.R_Wave_Offset;													// 346: R_Wave_Offset
	stream >> m_Data.Num_Accepted_Beats;										// 350: Num_Accepted_Beats
	stream.readRawBytes(&m_Data.Polar_Map_Protocol[0], 20);	// 354: Polar_Map_Protocol
	stream.readRawBytes(&m_Data.Database_Name[0], 30);			// 374: Database_Name
	for(int i=0; i < 27; i++)
		stream >> m_Data.CTI_reserved[i];											// 404: CTI_reserved
	for(int i=0; i < 27; i++)
		stream >> m_Data.User_Reserved[i];										// 464: User_Reserved

	// some more debug output
#if defined(DEBUG)
	D("ECAT7 PolarMap SubHeader loaded:");
	D("-------------------------------");
	D("Data_Type	               : %d",				m_Data.Data_Type);
	D("Polar_Map_Type            : %d",				m_Data.Polar_Map_Type);
	D("Num_Rings                 : %d",				m_Data.Num_Rings);
	for(int i=0; i < 32; i++)
	{
		D("Sectors_Per_Ring      [%02d]: %d", i+1, m_Data.Sectors_Per_Ring[i]);
	}
	for(int i=0; i < 32; i++)
	{
		D("Ring_Position         [%02d]: %f", i+1, m_Data.Ring_Position[i]);
	}
	for(int i=0; i < 32; i++)
	{
		D("Ring_Angle            [%02d]: %d", i+1, m_Data.Ring_Angle[i]);
	}
	D("Start_Angle               : %d",				m_Data.Start_Angle);
	for(int i=0; i < 3; i++)
	{
		D("Long_Axis_Left         [%d]: %d", i+1, m_Data.Long_Axis_Left[i]);
	}
	for(int i=0; i < 3; i++)
	{
		D("Long_Axis_Right        [%d]: %d", i+1, m_Data.Long_Axis_Right[i]);
	}
	D("Position_Data             : %d",				m_Data.Position_Data);
	D("Image_Min                 : %d",				m_Data.Image_Min);
	D("Image_Max                 : %d",				m_Data.Image_Max);
	D("Scale_Factor              : %f",				m_Data.Scale_Factor);
	D("Pixel_Size                : %f",				m_Data.Pixel_Size);
	D("Frame_Duration            : %d",				m_Data.Frame_Duration);
	D("Frame_Start_Time          : %d",				m_Data.Frame_Start_Time);
	D("Processing_Code           : %d",				m_Data.Processing_Code);
	D("Quant_Units               : %d",				m_Data.Quant_Units);
	D("Annotation                : %s",				m_Data.Annotation);
	D("Gate_Duration             : %d msec",	m_Data.Gate_Duration);
	D("R_Wave_Offset             : %d msec",	m_Data.R_Wave_Offset);
	D("Num_Accepted_Beats        : %d",				m_Data.Num_Accepted_Beats);
	D("Polar_Map_Protocol        : %s",				m_Data.Polar_Map_Protocol);
	D("Database_Name             : %s",				m_Data.Database_Name);
#endif

	RETURN(true);
	return true;
}

bool CECAT7SubHeaderPolarMap::save(void) const
{
	ENTER();

	// check if this stream is writeable or not
	if(m_pMedIOData->isWritable() == false ||
		 m_pDirItem->dataBlock_Start() == 0 ||
		 m_pMedIOData->at(m_pDirItem->dataBlock_Start()) == false)
	{
		RETURN(false);
		return false;
	}

	SHOWVALUE(m_pMedIOData->at());
	
	// we write to a buffer first and write out later directly to the file
	QByteArray buffer(rawDataSize());
	QDataStream stream(buffer, IO_WriteOnly);
	
	// lets read in each single data element of our
	// data structure to maintain the correct endianess of the
	// data
	stream << m_Data.Data_Type;															//   0: Data_Type
	stream << m_Data.Polar_Map_Type;												//   2: Polar_Map_Type
	stream << m_Data.Num_Rings;															//   4: Num_Rings
	for(int i=0; i < 32; i++)
		stream << m_Data.Sectors_Per_Ring[i];									//   6: Sectors_Per_Ring (32)
	for(int i=0; i < 32; i++)
		stream << m_Data.Ring_Position[i];										//  70: Ring_Position
	for(int i=0; i < 32; i++)
		stream << m_Data.Ring_Angle[i];												// 198: Ring_Angle
	stream << m_Data.Start_Angle;														// 262: Start_Angle
	for(int i=0; i < 3; i++)
		stream << m_Data.Long_Axis_Left[i];										// 264: Long_Axis_Left
	for(int i=0; i < 3; i++)
		stream << m_Data.Long_Axis_Right[i];									// 270: Long_Axis_Right
	stream << m_Data.Position_Data;													// 276: Position_Data
	stream << m_Data.Image_Min;															// 278: Image_Min
	stream << m_Data.Image_Max;															// 280: Image_Max
	stream << m_Data.Scale_Factor;													// 282: Scale_Factor
	stream << m_Data.Pixel_Size;														// 286: Pixel_Size
	stream << m_Data.Frame_Duration;												// 290: Frame_Duration
	stream << m_Data.Frame_Start_Time;											// 294: Frame_Start_Time
	stream << m_Data.Processing_Code;												// 298: Processing_Code
	stream << m_Data.Quant_Units;														// 300: Quant_Units
	stream.writeRawBytes(&m_Data.Annotation[0], 40);				// 302: Annotation
	stream << m_Data.Gate_Duration;													// 342: Gate_Duration
	stream << m_Data.R_Wave_Offset;													// 346: R_Wave_Offset
	stream << m_Data.Num_Accepted_Beats;										// 350: Num_Accepted_Beats
	stream.writeRawBytes(&m_Data.Polar_Map_Protocol[0], 20);// 354: Polar_Map_Protocol
	stream.writeRawBytes(&m_Data.Database_Name[0], 30);			// 374: Database_Name
	for(int i=0; i < 27; i++)
		stream << m_Data.CTI_reserved[i];											// 404: CTI_reserved
	for(int i=0; i < 27; i++)
		stream << m_Data.User_Reserved[i];										// 464: User_Reserved

	// now write out to our outStream
	bool result = false;
	if(m_pMedIOData->writeBlock(buffer) != -1)
	{
		m_pDirItem->subHeaderWritten(*this);
		result = true;
	}

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

// data access methods
CECATSubHeader::Data_Type CECAT7SubHeaderPolarMap::data_Type(void) const
{
	return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type);
}

short CECAT7SubHeaderPolarMap::polar_Map_Type(void) const
{
	return m_Data.Polar_Map_Type;
}

short CECAT7SubHeaderPolarMap::num_Rings(void) const
{
	return m_Data.Num_Rings;
}

short CECAT7SubHeaderPolarMap::sectors_Per_Ring(const short i) const
{
	return m_Data.Sectors_Per_Ring[i];
}

float CECAT7SubHeaderPolarMap::ring_Position(const short i) const
{
	return m_Data.Ring_Position[i];
}

short CECAT7SubHeaderPolarMap::ring_Angle(const short i) const
{
	return m_Data.Ring_Angle[i];
}

short CECAT7SubHeaderPolarMap::start_Angle(void) const
{
	return m_Data.Start_Angle;
}

short CECAT7SubHeaderPolarMap::long_Axis_Left(const short i) const
{
	return m_Data.Long_Axis_Left[i];
}

short CECAT7SubHeaderPolarMap::long_Axis_Right(const short i) const
{
	return m_Data.Long_Axis_Right[i];
}

short CECAT7SubHeaderPolarMap::position_Data(void) const
{
	return m_Data.Position_Data;
}

short CECAT7SubHeaderPolarMap::image_Min(void) const
{
	return m_Data.Image_Min;
}

short CECAT7SubHeaderPolarMap::image_Max(void) const
{
	return m_Data.Image_Max;
}

float CECAT7SubHeaderPolarMap::scale_Factor(void) const
{
	return m_Data.Scale_Factor;
}

float CECAT7SubHeaderPolarMap::pixel_Size(void) const
{
	return m_Data.Pixel_Size;
}

unsigned int CECAT7SubHeaderPolarMap::frame_Duration(void) const
{
	return m_Data.Frame_Duration;
}

unsigned int CECAT7SubHeaderPolarMap::frame_Start_Time(void) const
{
	return m_Data.Frame_Start_Time;
}

unsigned int CECAT7SubHeaderPolarMap::processing_Code(void) const
{
	return m_Data.Processing_Code;
}

short CECAT7SubHeaderPolarMap::quant_Units(void) const
{
	return m_Data.Quant_Units;
}

const char* CECAT7SubHeaderPolarMap::annotation(void) const
{
	return m_Data.Annotation;
}

unsigned int CECAT7SubHeaderPolarMap::gate_Duration(void) const
{
	return m_Data.Gate_Duration;
}

unsigned int CECAT7SubHeaderPolarMap::r_Wave_Offset(void) const
{
	return m_Data.R_Wave_Offset;
}

unsigned int CECAT7SubHeaderPolarMap::num_Accepted_Beats(void) const
{
	return m_Data.Num_Accepted_Beats;
}

const char* CECAT7SubHeaderPolarMap::polar_Map_Protocol(void) const
{
	return m_Data.Polar_Map_Protocol;
}

const char* CECAT7SubHeaderPolarMap::database_Name(void) const
{
	return m_Data.Database_Name;
}

short CECAT7SubHeaderPolarMap::cti_Reserved(const short i) const
{
	return m_Data.CTI_reserved[i];
}

short CECAT7SubHeaderPolarMap::user_Reserved(const short i) const
{
	return m_Data.User_Reserved[i];
}


void CECAT7SubHeaderPolarMap::setData_Type(const CECATSubHeader::Data_Type dType)
{
	m_Data.Data_Type = static_cast<Q_UINT16>(dType);	}		

void CECAT7SubHeaderPolarMap::setPolar_Map_Type(const short n)
{
	m_Data.Polar_Map_Type = n;
}

void CECAT7SubHeaderPolarMap::setNum_Rings(const short n)
{
	m_Data.Num_Rings = n;
}

void CECAT7SubHeaderPolarMap::setSectors_Per_Ring(const short i, const short n)
{
	m_Data.Sectors_Per_Ring[i] = n;
}

void CECAT7SubHeaderPolarMap::setRing_Position(const short i, const float n)
{
	m_Data.Ring_Position[i] = n;
}

void CECAT7SubHeaderPolarMap::setRing_Angle(const short i, const short n)
{
	m_Data.Ring_Angle[i] = n;
}

void CECAT7SubHeaderPolarMap::setStart_Angle(const short n)
{
	m_Data.Start_Angle = n;
}

void CECAT7SubHeaderPolarMap::setLong_Axis_Left(const short i, const short n)
{
	m_Data.Long_Axis_Left[i] = n;
}

void CECAT7SubHeaderPolarMap::setLong_Axis_Right(const short i, const short n)
{
	m_Data.Long_Axis_Right[i] = n;
}

void CECAT7SubHeaderPolarMap::setPosition_Data(const short n)
{
	m_Data.Position_Data = n;
}

void CECAT7SubHeaderPolarMap::setImage_Min(const short n)
{
	m_Data.Image_Min = n;
}

void CECAT7SubHeaderPolarMap::setImage_Max(const short n)
{
	m_Data.Image_Max = n;
}

void CECAT7SubHeaderPolarMap::setScale_Factor(const float n)
{
	m_Data.Scale_Factor = n;
}

void CECAT7SubHeaderPolarMap::setPixel_Size(const float n)
{
	m_Data.Pixel_Size = n;
}

void CECAT7SubHeaderPolarMap::setFrame_Duration(const unsigned int n)
{
	m_Data.Frame_Duration = n;
}

void CECAT7SubHeaderPolarMap::setFrame_Start_Time(const unsigned int n)
{
	m_Data.Frame_Start_Time = n;
}

void CECAT7SubHeaderPolarMap::setProcessing_Code(const unsigned int n)
{
	m_Data.Processing_Code = n;
}

void CECAT7SubHeaderPolarMap::setQuant_Units(const short n)
{
	m_Data.Quant_Units = n;
}

void CECAT7SubHeaderPolarMap::setAnnotation(const char* str)
{
	strncpy(m_Data.Annotation, str, 40);
}

void CECAT7SubHeaderPolarMap::setGate_Duration(const unsigned int n)
{
	m_Data.Gate_Duration = n;
}

void CECAT7SubHeaderPolarMap::setR_Wave_Offset(const unsigned int n)
{
	m_Data.R_Wave_Offset = n;
}

void CECAT7SubHeaderPolarMap::setNum_Accepted_Beats(const unsigned int n)
{
	m_Data.Num_Accepted_Beats = n;
}

void CECAT7SubHeaderPolarMap::setPolar_Map_Protocol(const char* str)
{
	strncpy(m_Data.Polar_Map_Protocol, str, 20);
}

void CECAT7SubHeaderPolarMap::setDatabase_Name(const char* str)
{
	strncpy(m_Data.Database_Name, str, 30);
}

void CECAT7SubHeaderPolarMap::setCTI_Reserved(const short i, const short n)
{
	m_Data.CTI_reserved[i] = n;
}

void CECAT7SubHeaderPolarMap::setUser_Reserved(const short i, const short n)
{
	m_Data.User_Reserved[i] = n;
}

CMedIOHeader& CECAT7SubHeaderPolarMap::copyData(const CMedIOHeader& src)
{
	ENTER();

	// depending on the MedIOHeader format we do have to 
	// distinguish between our copy operations.
	switch(src.headerFormat())
	{
		case CMedIOHeader::ECATSubHeader:
		{
			const CECATSubHeader* eSubHeader = static_cast<const CECATSubHeader*>(&src);

			// depending on the source type we have to copy either every data or just 
			// some data of the src header
			switch(eSubHeader->subHeaderType())
			{
				// if the source header is also an ECAT7 one we can copy it in whole
				// via a simple memcpy()
				case CECATSubHeader::ECAT7_PolarMap:
				{
					memcpy(&(this->m_Data), &(static_cast<const CECAT7SubHeaderPolarMap*>(&src)->m_Data), sizeof(struct ECAT7SubHeader_PolarMap));
				}
				break;

				case CECATSubHeader::Unknown:
					// for an unknown header type we do nothing
				break;
				
				#warning "non PolarMap copy not complete"
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

	LEAVE();
	return *this;
}
