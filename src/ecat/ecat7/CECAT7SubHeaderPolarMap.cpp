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
#include "CECATFile.h"

#include <qdatastream.h>

#include "debug.h"

CECAT7SubHeaderPolarMap::CECAT7SubHeaderPolarMap()
{
	// check that the headsize is 512 bytes long
	ASSERT(sizeof(struct ECAT7SubHeader_PolarMap) == 512);

	// then clear the structure
	memset(&m_Data, 0, sizeof(struct ECAT7SubHeader_PolarMap));			
}

bool CECAT7SubHeaderPolarMap::load(QDataStream& stream)
{
	// check if the stream is readable or not.
	if(stream.device()->isReadable() == false)
		return false;

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

	return true;
}

bool CECAT7SubHeaderPolarMap::save(QDataStream& stream)
{
	// check if this stream is writeable or not
	if(stream.device()->isWritable() == false)
		return false;
	
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
	
	return true;
}
