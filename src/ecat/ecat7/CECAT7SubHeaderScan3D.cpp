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

int CECAT7SubHeaderScan3D::rawDataSize() const
{
	return 2*ECAT_BLOCKSIZE;
}

CECATSubHeader::Type CECAT7SubHeaderScan3D::subHeaderType(void) const
{
	return CECATSubHeader::ECAT7_Scan3D;
}

// methods to access elements of the SubHeader
CECATSubHeader::Data_Type CECAT7SubHeaderScan3D::data_Type(void)	const
{
	return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type);
}

short CECAT7SubHeaderScan3D::num_Dimensions(void) const						
{
	return m_Data.Num_Dimensions;
}

short CECAT7SubHeaderScan3D::num_R_Elements(void) const						
{
	return m_Data.Num_R_Elements;
}

short CECAT7SubHeaderScan3D::num_Angles(void) const								
{
	return m_Data.Num_Angles;
}

short CECAT7SubHeaderScan3D::corrections_Applied(void) const				
{
	return m_Data.Corrections_Applied;
}

short CECAT7SubHeaderScan3D::num_Z_Elements(const short i) const		
{
	return m_Data.Num_Z_Elements[i];
}

short CECAT7SubHeaderScan3D::ring_Difference(void) const					
{	
	return m_Data.Ring_Difference;
}

CECAT7SubHeaderScan3D::Storage_Order CECAT7SubHeaderScan3D::storage_Order(void) const			
{
	return static_cast<CECAT7SubHeaderScan3D::Storage_Order>(m_Data.Storage_Order);
}

short CECAT7SubHeaderScan3D::axial_Compression(void) const				
{	
	return m_Data.Axial_Compression;
}

float CECAT7SubHeaderScan3D::X_Resolution(void) const						
{
	return m_Data.X_Resolution;
}

float CECAT7SubHeaderScan3D::V_Resolution(void) const						
{	
	return m_Data.V_Resolution;
}

float CECAT7SubHeaderScan3D::Z_Resolution(void) const						
{	
	return m_Data.Z_Resolution;	
}

float CECAT7SubHeaderScan3D::W_Resolution(void) const							
{	
	return m_Data.W_Resolution;	
}

short CECAT7SubHeaderScan3D::gate_Reserved(const short i) const		
{
	return m_Data.Gate_Reserved[i];
}

unsigned int CECAT7SubHeaderScan3D::gate_Duration(void) const		
{
	return m_Data.Gate_Duration;
}

unsigned int CECAT7SubHeaderScan3D::r_Wave_Offset(void) const		
{
	return m_Data.R_Wave_Offset;
}

unsigned int CECAT7SubHeaderScan3D::num_Accepted_Beats(void) const	
{	
	return m_Data.Num_Accepted_Beats;
}

float CECAT7SubHeaderScan3D::scale_Factor(void) const						
{
	return m_Data.Scale_Factor;
}

short CECAT7SubHeaderScan3D::scan_Min(void) const								
{
	return m_Data.Scan_Min;
}

short CECAT7SubHeaderScan3D::scan_Max(void) const									
{
	return m_Data.Scan_Max;
}

unsigned int CECAT7SubHeaderScan3D::prompts(void) const						
{
	return m_Data.Prompts;
}

unsigned int CECAT7SubHeaderScan3D::delayed(void) const					
{
	return m_Data.Delayed;
}

unsigned int CECAT7SubHeaderScan3D::multiples(void) const				
{
	return m_Data.Multiples;
}

unsigned int CECAT7SubHeaderScan3D::net_Trues(void) const				
{
	return m_Data.Net_Trues;
}

float CECAT7SubHeaderScan3D::tot_Avg_Cor(void) const 							
{
	return m_Data.Tot_Avg_Cor;
}

float CECAT7SubHeaderScan3D::tot_Avg_Uncor(void) const						
{
	return m_Data.Tot_Avg_Uncor;
}

unsigned int CECAT7SubHeaderScan3D::total_Coin_Rate(void) const	
{
	return m_Data.Total_Coin_Rate;
}

unsigned int CECAT7SubHeaderScan3D::frame_Start_Time(void) const	
{
	return m_Data.Frame_Start_Time;
}

unsigned int CECAT7SubHeaderScan3D::frame_Duration(void) const		
{
	return m_Data.Frame_Duration;
}

float CECAT7SubHeaderScan3D::deadtime_Correction_Factor(void) const
{
	return m_Data.Deadtime_Correction_Factor;	}

short CECAT7SubHeaderScan3D::cti_Reserved(const short i) const				
{
	return m_Data.CTI_reserved[i];
}

short CECAT7SubHeaderScan3D::user_Reserved(const short i) const		
{
	return m_Data.User_Reserved[i];
}

float CECAT7SubHeaderScan3D::uncor_Singles(const short bucket) const
{	
	return m_Data.Uncor_Singles[bucket];
}


// methods to modify elements of the SubHeader
void CECAT7SubHeaderScan3D::setData_Type(const CECATSubHeader::Data_Type dType)	
{
	m_Data.Data_Type = static_cast<Q_UINT16>(dType);
}

void CECAT7SubHeaderScan3D::setNum_Dimensions(const short dim)						
{
	m_Data.Num_Dimensions = dim;
}

void CECAT7SubHeaderScan3D::setNum_R_Elements(const short elem)						
{
	m_Data.Num_R_Elements = elem;
}

void CECAT7SubHeaderScan3D::setNum_Angles(const short angles)							
{
	m_Data.Num_Angles = angles;
}

void CECAT7SubHeaderScan3D::setCorrections_Applied(const short cor)			
{
	m_Data.Corrections_Applied = cor;
}

void CECAT7SubHeaderScan3D::setNum_Z_Elements(const short i, const short num)	
{	m_Data.Num_Z_Elements[i] = num;
}

void CECAT7SubHeaderScan3D::setRing_Difference(const short diff)					
{	m_Data.Ring_Difference = diff;
}

void CECAT7SubHeaderScan3D::setStorage_Order(const Storage_Order order)	
{
	m_Data.Storage_Order = static_cast<Q_UINT16>(order);
}

void CECAT7SubHeaderScan3D::setAxial_Compression(const short comp)			
{	m_Data.Axial_Compression = comp;
}

void CECAT7SubHeaderScan3D::setX_Resolution(const float res)						
{
	m_Data.X_Resolution = res;
}

void CECAT7SubHeaderScan3D::setV_Resolution(const float res)						
{
	m_Data.V_Resolution = res;
}

void CECAT7SubHeaderScan3D::setZ_Resolution(const float res)						
{
	m_Data.Z_Resolution = res;
}

void CECAT7SubHeaderScan3D::setW_Resolution(const float res)						
{
	m_Data.W_Resolution = res;
}

void CECAT7SubHeaderScan3D::setGate_Reserved(const short i, const short val)	
{
	m_Data.Gate_Reserved[i] = val;
}

void CECAT7SubHeaderScan3D::setGate_Duration(const unsigned int dur)		
{	m_Data.Gate_Duration = dur;	}

void CECAT7SubHeaderScan3D::setR_Wave_Offset(const unsigned int off)		
{
	m_Data.R_Wave_Offset = off;	}

void CECAT7SubHeaderScan3D::setNum_Accepted_Beats(const unsigned int b)		
{
	m_Data.Num_Accepted_Beats = b;
}

void CECAT7SubHeaderScan3D::setScale_Factor(const float factor)					
{
	m_Data.Scale_Factor = factor;
}

void CECAT7SubHeaderScan3D::setScan_Min(const short min)								
{
	m_Data.Scan_Min = min;
}

void CECAT7SubHeaderScan3D::setScan_Max(const short max)								
{
	m_Data.Scan_Max = max;
}

void CECAT7SubHeaderScan3D::setPrompts(const unsigned int n)						
{
	m_Data.Prompts = n;
}

void CECAT7SubHeaderScan3D::setDelayed(const unsigned int n)						
{
	m_Data.Delayed = n;	}

void CECAT7SubHeaderScan3D::setMultiples(const unsigned int n)					
{
	m_Data.Multiples = n;	}

void CECAT7SubHeaderScan3D::setNet_Trues(const unsigned int n)					
{
	m_Data.Net_Trues = n;
}

void CECAT7SubHeaderScan3D::setTot_Avg_Cor(const float value)						
{
	m_Data.Tot_Avg_Cor = value;
}

void CECAT7SubHeaderScan3D::setTot_Avg_Uncor(const float value)					
{
	m_Data.Tot_Avg_Uncor = value;
}

void CECAT7SubHeaderScan3D::setTotal_Coin_Rate(const unsigned int n)		
{
	m_Data.Total_Coin_Rate = n;
}

void CECAT7SubHeaderScan3D::setFrame_Start_Time(const unsigned int t)		
{
	m_Data.Frame_Start_Time = t;
}

void CECAT7SubHeaderScan3D::setFrame_Duration(const unsigned int t)			
{
	m_Data.Frame_Duration = t;
}

void CECAT7SubHeaderScan3D::setDeadtime_Correction_Factor(const float f)
{
	m_Data.Deadtime_Correction_Factor = f;
}

void CECAT7SubHeaderScan3D::setCTI_Reserved(const short i, const short value)		
{
	m_Data.CTI_reserved[i] = value;
}

void CECAT7SubHeaderScan3D::setUser_Reserved(const short i, const short value)
{
	m_Data.User_Reserved[i] = value;
}

void CECAT7SubHeaderScan3D::setUncor_Singles(const short b, const float value)
{ 
	m_Data.Uncor_Singles[b] = value;
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
