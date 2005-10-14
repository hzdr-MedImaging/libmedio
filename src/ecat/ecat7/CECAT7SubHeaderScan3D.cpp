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
#include "CECATDirectoryItem.h"
#include "CECATFile.h"

#include <qdatastream.h>

#include <rtdebug.h>

CECAT7SubHeaderScan3D::CECAT7SubHeaderScan3D(CECATFile* ecatFile,
																						 CECATDirectoryItem* pDirItem)
	: CECATSubHeader(ecatFile, pDirItem)
{
	// then clear the structure
	memset(&m_Data, 0, sizeof(struct ECAT7SubHeader_Scan3D));
		
	// put in some default values which are the most common ones
	m_Data.Data_Type									= static_cast<Q_UINT16>(CECATSubHeader::SunShort);
	m_Data.Num_Dimensions							= 4;
	m_Data.Num_R_Elements							= 288;
	m_Data.Num_Angles									= 144;	// with mash=2
	m_Data.Ring_Difference						= 22;
	m_Data.Axial_Compression					= 9;		// SPAN of 9
	m_Data.Scale_Factor								= 1.0;
	m_Data.Scan_Min										= -1;
	m_Data.Scan_Max										= -1;
	m_Data.Deadtime_Correction_Factor = 1.0;
}

CECAT7SubHeaderScan3D::CECAT7SubHeaderScan3D(const CECAT7SubHeaderScan3D& sh)
	: CECATSubHeader(sh)
{
	// then copy the structure
	memcpy(&m_Data, &sh.m_Data, sizeof(struct ECAT7SubHeader_Scan3D));
}

bool CECAT7SubHeaderScan3D::load(void)
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

	RETURN(true);
	return true;
}

bool CECAT7SubHeaderScan3D::save(void) const
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

CMedIOHeader& CECAT7SubHeaderScan3D::copyData(const CMedIOHeader& src)
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
				case CECATSubHeader::ECAT7_Scan3D:
				{
					memcpy(&(this->m_Data), &(static_cast<const CECAT7SubHeaderScan3D*>(&src)->m_Data), sizeof(struct ECAT7SubHeader_Scan3D));
				}
				break;

				case CECATSubHeader::Unknown:
					// for an unknown header type we do nothing
				break;
				
				#warning "non Scan3D copy not complete"
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
