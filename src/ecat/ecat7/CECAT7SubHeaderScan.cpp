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

#include "CECAT7SubHeaderScan.h"
#include "CECATDirectoryItem.h"
#include "CECATFile.h"

#include <qdatastream.h>

#include <rtdebug.h>

CECAT7SubHeaderScan::CECAT7SubHeaderScan(CECATFile* ecatFile,
																				 CECATDirectoryItem* pDirItem)
	: CECATSubHeader(ecatFile, pDirItem)
{
	// check that the headsize is 1024 bytes long
	ASSERT(sizeof(struct ECAT7SubHeader_Scan) == ECAT7_HEADERSIZE_SCAN);

	// then clear the structure
	memset(&m_Data, 0, sizeof(struct ECAT7SubHeader_Scan));			
}

CECAT7SubHeaderScan::CECAT7SubHeaderScan(const CECAT7SubHeaderScan& sh)
	: CECATSubHeader(sh)
{
	// check that the headsize is 1024 bytes long
	ASSERT(sizeof(struct ECAT7SubHeader_Scan) == ECAT7_HEADERSIZE_SCAN);

	// then copy the structure
	memcpy(&m_Data, &sh.m_Data, sizeof(struct ECAT7SubHeader_Scan));			
}

bool CECAT7SubHeaderScan::load(void)
{
	ENTER();

	// check if the stream is readable and if we can seek to the
  // expected position of the subheader
	if(m_pMedIOData->isReadable() == false ||
		 m_pDirItem->dataBlock_Start() == 0 ||
	   m_pMedIOData->at(m_pDirItem->dataBlock_Start()) == false)
	{
		RETURN(false);
		return false;
	}

	// we use a ByteArray buffer to speed up the endianess
	// decoding
	QByteArray buffer(sizeof(struct ECAT7SubHeader_Scan));
	if(m_pMedIOData->readBlock(buffer.data(), sizeof(struct ECAT7SubHeader_Scan)) 
			!= sizeof(struct ECAT7SubHeader_Scan))
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
	stream >> m_Data.Data_Type;											//   0: Data_Type
	stream >> m_Data.Num_Dimensions;								//   2: Num_Dimensions
	stream >> m_Data.Num_R_Elements;								//   4: Num_R_Elements
	stream >> m_Data.Num_Angles;										//   6: Num_Angles
	stream >> m_Data.Corrections_Applied;						//   8: Corrections_Applied
	stream >> m_Data.Num_Z_Elements;								//  10: Num_Z_Elements
	stream >> m_Data.Ring_Difference;								//  12: Ring_Difference
	stream >> m_Data.X_Resolution;									//  14: X_Resolution
	stream >> m_Data.Y_Resolution;									//  18: Y_Resolution
	stream >> m_Data.Z_Resolution;									//  22: Z_Resolution
	stream >> m_Data.W_Resolution;									//  26: W_Resolution
	for(int i=0; i < 6; i++)
		stream >> m_Data.Gate_Reserved[i];						//  30: Gate_Reserved
	stream >> m_Data.Gate_Duration;									//  42: Gate_Duration
	stream >> m_Data.R_Wave_Offset;									//  46: R_Wave_Offset
	stream >> m_Data.Num_Accepted_Beats;						//  50: Num_Accepted_Beats
	stream >> m_Data.Scale_Factor;									//  54: Scale_Factor
	stream >> m_Data.Scan_Min;											//  54: Scan_Min
	stream >> m_Data.Scan_Max;											//  60: Scan_Max
	stream >> m_Data.Prompts;												//  62: Prompts
	stream >> m_Data.Delayed;												//  66: Delayed
	stream >> m_Data.Multiples;											//  70: Multiples
	stream >> m_Data.Net_Trues;											//  74: Net_Trues
	for(int i=0; i < 16; i++)
		stream >> m_Data.Cor_Singles[i];							//  78: Cor_Singles
	for(int i=0; i < 16; i++)
		stream >> m_Data.Uncor_Singles[i];						// 142: Uncor_Singles
	stream >> m_Data.Tot_Avg_Cor;										// 206: Tot_Avg_Cor
	stream >> m_Data.Tot_Avg_Uncor;									// 210: Tot_Avg_Uncor
	stream >> m_Data.Total_Coin_Rate;								// 214: Total_Coin_Rate
	stream >> m_Data.Frame_Start_Time;							// 218: Frame_Start_Time
	stream >> m_Data.Frame_Duration;								// 222: Frame_Duration
	stream >> m_Data.Deadtime_Correction_Factor;		// 226: Deadtime_Correction_Factor
	for(int i=0; i < 8; i++)
		stream >> m_Data.Physical_Planes[i];					// 230: Physical_Planes
	for(int i=0; i < 83; i++)
		stream >> m_Data.CTI_reserved[i];							// 246: CTI_reserved
	for(int i=0; i < 50; i++)
		stream >> m_Data.User_Reserved[i];						// 412: User_Reserved	

	// some more debug output
#if defined(DEBUG)
	D("ECAT7 Scan SubHeader loaded:");
	D("---------------------------");
	D("Data_Type	               : %d",				m_Data.Data_Type);
	D("Num_Dimensions					   : %d",				m_Data.Num_Dimensions);
	D("Num_R_Elements		         : %d",				m_Data.Num_R_Elements);
	D("Num_Angles                : %d",				m_Data.Num_Angles);
	D("Corrections_Applied       : %d",				m_Data.Corrections_Applied);
	D("Num_Z_Elements            : %d",				m_Data.Num_Z_Elements);
	D("Ring_Difference           : %d",				m_Data.Ring_Difference);
	D("X_Resolution              : %f cm",		m_Data.X_Resolution);
	D("Y_Resolution              : %f cm",		m_Data.Y_Resolution);
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
	for(int i=0; i < 16; i++)
	{
		D("Cor_Singles           [%02d]: %f", i+1, m_Data.Cor_Singles[i]);
	}
	for(int i=0; i < 16; i++)
	{
		D("Uncor_Singles         [%02d]: %f", i+1, m_Data.Uncor_Singles[i]);
	}
	D("Tot_Avg_Cor               : %f",				m_Data.Tot_Avg_Cor);
	D("Tot_Avg_Uncor             : %f",				m_Data.Tot_Avg_Uncor);
	D("Total_Coin_Rate           : %d",				m_Data.Total_Coin_Rate);
	D("Frame_Start_Time          : %d msec",	m_Data.Frame_Start_Time);
	D("Frame_Duration            : %d msec",	m_Data.Frame_Duration);
	D("Deadtime_Correction_Factor: %f",				m_Data.Deadtime_Correction_Factor);
	for(int i=0; i < 8; i++)
	{
	D("Physical_Planes        [%d]: %d", i+1, m_Data.Physical_Planes[i]);
	}
#endif

	RETURN(true);
	return true;
}

bool CECAT7SubHeaderScan::save(void) const
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
	QByteArray buffer(sizeof(struct ECAT7SubHeader_Scan));
	QDataStream stream(buffer, IO_WriteOnly);

	// lets read in each single data element of our
	// data structure to maintain the correct endianess of the
	// data
	stream << m_Data.Data_Type;											//   0: Data_Type
	stream << m_Data.Num_Dimensions;								//   2: Num_Dimensions
	stream << m_Data.Num_R_Elements;								//   4: Num_R_Elements
	stream << m_Data.Num_Angles;										//   6: Num_Angles
	stream << m_Data.Corrections_Applied;						//   8: Corrections_Applied
	stream << m_Data.Num_Z_Elements;								//  10: Num_Z_Elements
	stream << m_Data.Ring_Difference;								//  12: Ring_Difference
	stream << m_Data.X_Resolution;									//  14: X_Resolution
	stream << m_Data.Y_Resolution;									//  18: Y_Resolution
	stream << m_Data.Z_Resolution;									//  22: Z_Resolution
	stream << m_Data.W_Resolution;									//  26: W_Resolution
	for(int i=0; i < 6; i++)
		stream << m_Data.Gate_Reserved[i];						//  30: Gate_Reserved
	stream << m_Data.Gate_Duration;									//  42: Gate_Duration
	stream << m_Data.R_Wave_Offset;									//  46: R_Wave_Offset
	stream << m_Data.Num_Accepted_Beats;						//  50: Num_Accepted_Beats
	stream << m_Data.Scale_Factor;									//  54: Scale_Factor
	stream << m_Data.Scan_Min;											//  54: Scan_Min
	stream << m_Data.Scan_Max;											//  60: Scan_Max
	stream << m_Data.Prompts;												//  62: Prompts
	stream << m_Data.Delayed;												//  66: Delayed
	stream << m_Data.Multiples;											//  70: Multiples
	stream << m_Data.Net_Trues;											//  74: Net_Trues
	for(int i=0; i < 16; i++)
		stream << m_Data.Cor_Singles[i];							//  78: Cor_Singles
	for(int i=0; i < 16; i++)
		stream << m_Data.Uncor_Singles[i];						// 142: Uncor_Singles
	stream << m_Data.Tot_Avg_Cor;										// 206: Tot_Avg_Cor
	stream << m_Data.Tot_Avg_Uncor;									// 210: Tot_Avg_Uncor
	stream << m_Data.Total_Coin_Rate;								// 214: Total_Coin_Rate
	stream << m_Data.Frame_Start_Time;							// 218: Frame_Start_Time
	stream << m_Data.Frame_Duration;								// 222: Frame_Duration
	stream << m_Data.Deadtime_Correction_Factor;		// 226: Deadtime_Correction_Factor
	for(int i=0; i < 8; i++)
		stream << m_Data.Physical_Planes[i];					// 230: Physical_Planes
	for(int i=0; i < 83; i++)
		stream << m_Data.CTI_reserved[i];							// 246: CTI_reserved
	for(int i=0; i < 50; i++)
		stream << m_Data.User_Reserved[i];						// 412: User_Reserved	
	
	// now write out to our outStream
	bool result = false;
	if(m_pMedIOData->writeBlock(buffer) != -1)
		result = true;

	RETURN(result);
	return result;
}
