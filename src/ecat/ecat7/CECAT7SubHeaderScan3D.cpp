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

#include "CECAT7SubHeaderScan3D.h"
#include "CECATFile.h"

#include <qdatastream.h>

#include "debug.h"

bool CECAT7SubHeaderScan3D::load(QDataStream& stream)
{
	// check if the stream is readable or not.
	if(stream.device()->isReadable() == false)
		return false;

	// lets read in each single data element of our
	// data structure to maintain the correct endianess of the
	// data
	stream >> m_Data.Data_Type;											//   0: Data_Type
	stream >> m_Data.Num_Dimensions;								//   2: Num_Dimensions
	stream >> m_Data.Num_R_Elements;								//   4: Num_R_Elements
	stream >> m_Data.Num_Angles;										//   6: Num_Angles
	stream >> m_Data.Corrections_Applied;						//   8: Corrections_Applied
	for(int i=0; i < 64; i++)
		stream >> m_Data.Num_Z_Elements[i];						//  10: Num_Z_Elements
	stream >> m_Data.Ring_Difference;								// 138: Ring_Difference
	stream >> m_Data.Storage_Order;									// 140: Storage_Order
	stream >> m_Data.Axial_Compression;							// 142: Axial_Compression
	stream >> m_Data.X_Resolution;									// 144: X_Resolution
	stream >> m_Data.V_Resolution;									// 148: V_Resolution
	stream >> m_Data.Z_Resolution;									// 152: Z_Resolution
	stream >> m_Data.W_Resolution;									// 156: W_Resolution
	for(int i=0; i < 6; i++)
		stream >> m_Data.Gate_Reserved[i];						// 160: Gate_Reserved
	stream >> m_Data.Gate_Duration;									// 172: Gate_Duration
	stream >> m_Data.R_Wave_Offset;									// 176: R_Wave_Offset
	stream >> m_Data.Num_Accepted_Beats;						// 180: Num_Accepted_Beats
	stream >> m_Data.Scale_Factor;									// 184: Scale_Factor
	stream >> m_Data.Scan_Min;											// 188: Scan_Min
	stream >> m_Data.Scan_Max;											// 190: Scan_Max
	stream >> m_Data.Prompts;												// 192: Prompts
	stream >> m_Data.Delayed;												// 196: Delayed
	stream >> m_Data.Multiples;											// 200: Multiples
	stream >> m_Data.Net_Trues;											// 204: Net_Trues
	stream >> m_Data.Tot_Avg_Cor;										// 208: Tot_Avg_Cor
	stream >> m_Data.Tot_Avg_Uncor;									// 212: Tot_Avg_Uncor
	stream >> m_Data.Total_Coin_Rate;								// 216: Total_Coin_Rate
	stream >> m_Data.Frame_Start_Time;							// 220: Frame_Start_Time
	stream >> m_Data.Frame_Duration;								// 224: Frame_Duration
	stream >> m_Data.Deadtime_Correction_Factor;		// 228: Deadtime_Correction_Factor
	for(int i=0; i < 90; i++)
		stream >> m_Data.CTI_reserved[i];							// 232: CTI_reserved
	for(int i=0; i < 50; i++)
		stream >> m_Data.User_Reserved[i];						// 412: User_Reserved	
	for(int i=0; i < 128; i++)
		stream >> m_Data.Uncor_Singles[i];						// 512: Uncor_Singles
		
	// some more debug output
#if defined(DEBUG)
	D("ECAT7 Scan3D SubHeader loaded:");
	D("-----------------------------");
	D("Data_Type	               : %d",				m_Data.Data_Type);
	D("Num_Dimensions            : %d",				m_Data.Num_Dimensions);
	D("Num_R_Elements            : %d",				m_Data.Num_R_Elements);
	D("Num_Angles                : %d",				m_Data.Num_Angles);
	D("Corrections_Applied       : %d",				m_Data.Corrections_Applied);
	for(int i=0; i < 64; i++)
	{
		D("Num_Z_Elements        [%02d]: %d", i+1, m_Data.Num_Z_Elements[i]);
	}
	D("Storage_Order             : %d",				m_Data.Storage_Order);
	D("Axial_Compression         : %d",				m_Data.Axial_Compression);
	D("X_Resolution              : %f cm",		m_Data.X_Resolution);
	D("V_Resolution              : %f rad",		m_Data.V_Resolution);
	D("Z_Resolution              : %f cm",		m_Data.Z_Resolution);
	D("W_Resolution              : %f",				m_Data.W_Resolution);
	for(int i=0; i < 6; i++)
	{
		D("Gate_Reserved          [%d]: %d", i+1, m_Data.Gate_Reserved[i]);
	}
	D("Gate_Duration             : %d msec",	m_Data.Gate_Duration);
	D("R_Wave_Offset             : %d",				m_Data.R_Wave_Offset);
	D("Num_Accepted_Beats        : %d",				m_Data.Num_Accepted_Beats);
	D("Scale_Factor              : %f",				m_Data.Scale_Factor);
	D("Scan_Min                  : %d",				m_Data.Scan_Min);
	D("Scan_Max                  : %d",				m_Data.Scan_Max);
	D("Prompts                   : %d",				m_Data.Prompts);
	D("Delayed                   : %d",				m_Data.Delayed);
	D("Multiples                 : %d",				m_Data.Multiples);
	D("Net_Trues                 : %d",				m_Data.Net_Trues);
	D("Tot_Avg_Cor               : %f",				m_Data.Tot_Avg_Cor);
	D("Tot_Avg_Uncor             : %f",				m_Data.Tot_Avg_Uncor);
	D("Total_Coin_Rate           : %d",				m_Data.Total_Coin_Rate);
	D("Frame_Start_Time          : %d msec",	m_Data.Frame_Start_Time);
	D("Frame_Duration            : %d msec",	m_Data.Frame_Duration);
	D("Deadtime_Correction_Factor: %f",				m_Data.Deadtime_Correction_Factor);
	for(int i=0; i < 8; i++)
	{
		D("Uncor_Singles         [%03d]: %f", i+1, m_Data.Uncor_Singles[i]);
	}
#endif

	return true;
}

bool CECAT7SubHeaderScan3D::save(QDataStream& stream)
{
	// check if this stream is writeable or not
	if(stream.device()->isWritable() == false)
		return false;
	
	// lets write out each single data element of our
	// data structure to maintain the correct endianess of the
	// data
	stream << m_Data.Data_Type;											//   0: Data_Type
	stream << m_Data.Num_Dimensions;								//   2: Num_Dimensions
	stream << m_Data.Num_R_Elements;								//   4: Num_R_Elements
	stream << m_Data.Num_Angles;										//   6: Num_Angles
	stream << m_Data.Corrections_Applied;						//   8: Corrections_Applied
	for(int i=0; i < 64; i++)
		stream << m_Data.Num_Z_Elements[i];						//  10: Num_Z_Elements
	stream << m_Data.Ring_Difference;								// 138: Ring_Difference
	stream << m_Data.Storage_Order;									// 140: Storage_Order
	stream << m_Data.Axial_Compression;							// 142: Axial_Compression
	stream << m_Data.X_Resolution;									// 144: X_Resolution
	stream << m_Data.V_Resolution;									// 148: V_Resolution
	stream << m_Data.Z_Resolution;									// 152: Z_Resolution
	stream << m_Data.W_Resolution;									// 156: W_Resolution
	for(int i=0; i < 6; i++)
		stream << m_Data.Gate_Reserved[i];						// 160: Gate_Reserved
	stream << m_Data.Gate_Duration;									// 172: Gate_Duration
	stream << m_Data.R_Wave_Offset;									// 176: R_Wave_Offset
	stream << m_Data.Num_Accepted_Beats;						// 180: Num_Accepted_Beats
	stream << m_Data.Scale_Factor;									// 184: Scale_Factor
	stream << m_Data.Scan_Min;											// 188: Scan_Min
	stream << m_Data.Scan_Max;											// 190: Scan_Max
	stream << m_Data.Prompts;												// 192: Prompts
	stream << m_Data.Delayed;												// 196: Delayed
	stream << m_Data.Multiples;											// 200: Multiples
	stream << m_Data.Net_Trues;											// 204: Net_Trues
	stream << m_Data.Tot_Avg_Cor;										// 208: Tot_Avg_Cor
	stream << m_Data.Tot_Avg_Uncor;									// 212: Tot_Avg_Uncor
	stream << m_Data.Total_Coin_Rate;								// 216: Total_Coin_Rate
	stream << m_Data.Frame_Start_Time;							// 220: Frame_Start_Time
	stream << m_Data.Frame_Duration;								// 224: Frame_Duration
	stream << m_Data.Deadtime_Correction_Factor;		// 228: Deadtime_Correction_Factor
	for(int i=0; i < 90; i++)
		stream << m_Data.CTI_reserved[i];							// 232: CTI_reserved
	for(int i=0; i < 50; i++)
		stream << m_Data.User_Reserved[i];						// 412: User_Reserved	
	for(int i=0; i < 128; i++)
		stream << m_Data.Uncor_Singles[i];						// 512: Uncor_Singles
	
	return true;
}
