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

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CECAT7SubHeaderScanPrivate
{
	public:
		struct HeaderData
		{
			Q_UINT16	Data_Type;
			Q_UINT16	Num_Dimensions;
			Q_UINT16	Num_R_Elements;
			Q_UINT16	Num_Angles;
			Q_UINT16	Corrections_Applied;
			Q_UINT16	Num_Z_Elements;
			Q_UINT16	Ring_Difference;
			float			X_Resolution;
			float			Y_Resolution;
			float			Z_Resolution;
			float			W_Resolution;
			Q_UINT16	Gate_Reserved[6];
			Q_UINT32	Gate_Duration;
			Q_UINT32	R_Wave_Offset;
			Q_UINT32	Num_Accepted_Beats;
			float			Scale_Factor;
			Q_UINT16	Scan_Min;
			Q_UINT16	Scan_Max;
			Q_UINT32	Prompts;
			Q_UINT32	Delayed;
			Q_UINT32	Multiples;
			Q_UINT32	Net_Trues;
			float			Cor_Singles[16];
			float			Uncor_Singles[16];
			float			Tot_Avg_Cor;
			float			Tot_Avg_Uncor;
			Q_UINT32	Total_Coin_Rate;
			Q_UINT32	Frame_Start_Time;
			Q_UINT32	Frame_Duration;
			float			Deadtime_Correction_Factor;
			Q_UINT16	Physical_Planes[8];
			Q_UINT16	CTI_reserved[83];
			Q_UINT16	User_Reserved[50];
		} header;
};

CECAT7SubHeaderScan::CECAT7SubHeaderScan(CECATFile* ecatFile,
																				 CECATDirectoryItem* pDirItem)
	: CECATSubHeader(ecatFile, pDirItem)
{
	ENTER();

	// allocate data from our private instance class
	m_pData = new CECAT7SubHeaderScanPrivate();
	
	clear();

	LEAVE();
}

CECAT7SubHeaderScan::~CECAT7SubHeaderScan()
{
	ENTER();

	delete m_pData;

	LEAVE();
}

CECAT7SubHeaderScan::CECAT7SubHeaderScan(const CECAT7SubHeaderScan& src)
	: CECATSubHeader(src)
{
	ENTER();

	// allocate data from our private instance class
	m_pData = new CECAT7SubHeaderScanPrivate(*(src.m_pData));

	LEAVE();
}

CECAT7SubHeaderScan& CECAT7SubHeaderScan::operator=(const CECAT7SubHeaderScan& src)
{
	ENTER();

	if(m_pData != src.m_pData)
	{
		memcpy(&m_pData->header, 
					 &src.m_pData->header, 
					 sizeof(struct CECAT7SubHeaderScanPrivate::HeaderData));
	}

	LEAVE();
	return *this;
}

void CECAT7SubHeaderScan::clear()
{
	ENTER();

	// then clear the structure
	memset(&m_pData->header, 0, sizeof(struct CECAT7SubHeaderScanPrivate::HeaderData));			

	setData_Type(CECATSubHeader::UnknownDataType);

	LEAVE();
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
	stream >> m_pData->header.Data_Type;											//   0: Data_Type
	stream >> m_pData->header.Num_Dimensions;								//   2: Num_Dimensions
	stream >> m_pData->header.Num_R_Elements;								//   4: Num_R_Elements
	stream >> m_pData->header.Num_Angles;										//   6: Num_Angles
	stream >> m_pData->header.Corrections_Applied;						//   8: Corrections_Applied
	stream >> m_pData->header.Num_Z_Elements;								//  10: Num_Z_Elements
	stream >> m_pData->header.Ring_Difference;								//  12: Ring_Difference
	stream >> m_pData->header.X_Resolution;									//  14: X_Resolution
	stream >> m_pData->header.Y_Resolution;									//  18: Y_Resolution
	stream >> m_pData->header.Z_Resolution;									//  22: Z_Resolution
	stream >> m_pData->header.W_Resolution;									//  26: W_Resolution
	for(int i=0; i < 6; i++)
		stream >> m_pData->header.Gate_Reserved[i];						//  30: Gate_Reserved
	stream >> m_pData->header.Gate_Duration;									//  42: Gate_Duration
	stream >> m_pData->header.R_Wave_Offset;									//  46: R_Wave_Offset
	stream >> m_pData->header.Num_Accepted_Beats;						//  50: Num_Accepted_Beats
	stream >> m_pData->header.Scale_Factor;									//  54: Scale_Factor
	stream >> m_pData->header.Scan_Min;											//  54: Scan_Min
	stream >> m_pData->header.Scan_Max;											//  60: Scan_Max
	stream >> m_pData->header.Prompts;												//  62: Prompts
	stream >> m_pData->header.Delayed;												//  66: Delayed
	stream >> m_pData->header.Multiples;											//  70: Multiples
	stream >> m_pData->header.Net_Trues;											//  74: Net_Trues
	for(int i=0; i < 16; i++)
		stream >> m_pData->header.Cor_Singles[i];							//  78: Cor_Singles
	for(int i=0; i < 16; i++)
		stream >> m_pData->header.Uncor_Singles[i];						// 142: Uncor_Singles
	stream >> m_pData->header.Tot_Avg_Cor;										// 206: Tot_Avg_Cor
	stream >> m_pData->header.Tot_Avg_Uncor;									// 210: Tot_Avg_Uncor
	stream >> m_pData->header.Total_Coin_Rate;								// 214: Total_Coin_Rate
	stream >> m_pData->header.Frame_Start_Time;							// 218: Frame_Start_Time
	stream >> m_pData->header.Frame_Duration;								// 222: Frame_Duration
	stream >> m_pData->header.Deadtime_Correction_Factor;		// 226: Deadtime_Correction_Factor
	for(int i=0; i < 8; i++)
		stream >> m_pData->header.Physical_Planes[i];					// 230: Physical_Planes
	for(int i=0; i < 83; i++)
		stream >> m_pData->header.CTI_reserved[i];							// 246: CTI_reserved
	for(int i=0; i < 50; i++)
		stream >> m_pData->header.User_Reserved[i];						// 412: User_Reserved	

	// some more debug output
#if defined(DEBUG)
	D("ECAT7 Scan SubHeader loaded:");
	D("---------------------------");
	D("Data_Type	               : %d",				m_pData->header.Data_Type);
	D("Num_Dimensions					   : %d",				m_pData->header.Num_Dimensions);
	D("Num_R_Elements		         : %d",				m_pData->header.Num_R_Elements);
	D("Num_Angles                : %d",				m_pData->header.Num_Angles);
	D("Corrections_Applied       : %d",				m_pData->header.Corrections_Applied);
	D("Num_Z_Elements            : %d",				m_pData->header.Num_Z_Elements);
	D("Ring_Difference           : %d",				m_pData->header.Ring_Difference);
	D("X_Resolution              : %f cm",		m_pData->header.X_Resolution);
	D("Y_Resolution              : %f cm",		m_pData->header.Y_Resolution);
	D("Z_Resolution              : %f cm",		m_pData->header.Z_Resolution);
	D("W_Resolution              : %f",				m_pData->header.W_Resolution);
	for(int i=0; i < 6; i++)
	{
		D("Gate_Reserved          [%d]: %d", i+1, m_pData->header.Gate_Reserved[i]);
	}
	D("Gate_Duration             : %d msec",	m_pData->header.Gate_Duration);
	D("R_Wave_Offset             : %d",				m_pData->header.R_Wave_Offset);
	D("Num_Accepted_Beats        : %d",				m_pData->header.Num_Accepted_Beats);
	D("Scale_Factor              : %f",				m_pData->header.Scale_Factor);
	D("Scan_Min                  : %d",				m_pData->header.Scan_Min);
	D("Scan_Max                  : %d",				m_pData->header.Scan_Max);
	D("Prompts                   : %d",				m_pData->header.Prompts);
	D("Delayed                   : %d",				m_pData->header.Delayed);
	D("Multiples                 : %d",				m_pData->header.Multiples);
	D("Net_Trues                 : %d",				m_pData->header.Net_Trues);
	for(int i=0; i < 16; i++)
	{
		D("Cor_Singles           [%02d]: %f", i+1, m_pData->header.Cor_Singles[i]);
	}
	for(int i=0; i < 16; i++)
	{
		D("Uncor_Singles         [%02d]: %f", i+1, m_pData->header.Uncor_Singles[i]);
	}
	D("Tot_Avg_Cor               : %f",				m_pData->header.Tot_Avg_Cor);
	D("Tot_Avg_Uncor             : %f",				m_pData->header.Tot_Avg_Uncor);
	D("Total_Coin_Rate           : %d",				m_pData->header.Total_Coin_Rate);
	D("Frame_Start_Time          : %d msec",	m_pData->header.Frame_Start_Time);
	D("Frame_Duration            : %d msec",	m_pData->header.Frame_Duration);
	D("Deadtime_Correction_Factor: %f",				m_pData->header.Deadtime_Correction_Factor);
	for(int i=0; i < 8; i++)
	{
	D("Physical_Planes        [%d]: %d", i+1, m_pData->header.Physical_Planes[i]);
	}
#endif

	RETURN(true);
	return true;
}

bool CECAT7SubHeaderScan::save(void) const
{
	ENTER();

	// check if this stream is writeable or not
	if(m_pMedIOData == NULL || m_pMedIOData->isWritable() == false ||
		 m_pDirItem == NULL || m_pDirItem->dataBlock_Start() == 0 ||
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
	stream << m_pData->header.Data_Type;											//   0: Data_Type
	stream << m_pData->header.Num_Dimensions;								//   2: Num_Dimensions
	stream << m_pData->header.Num_R_Elements;								//   4: Num_R_Elements
	stream << m_pData->header.Num_Angles;										//   6: Num_Angles
	stream << m_pData->header.Corrections_Applied;						//   8: Corrections_Applied
	stream << m_pData->header.Num_Z_Elements;								//  10: Num_Z_Elements
	stream << m_pData->header.Ring_Difference;								//  12: Ring_Difference
	stream << m_pData->header.X_Resolution;									//  14: X_Resolution
	stream << m_pData->header.Y_Resolution;									//  18: Y_Resolution
	stream << m_pData->header.Z_Resolution;									//  22: Z_Resolution
	stream << m_pData->header.W_Resolution;									//  26: W_Resolution
	for(int i=0; i < 6; i++)
		stream << m_pData->header.Gate_Reserved[i];						//  30: Gate_Reserved
	stream << m_pData->header.Gate_Duration;									//  42: Gate_Duration
	stream << m_pData->header.R_Wave_Offset;									//  46: R_Wave_Offset
	stream << m_pData->header.Num_Accepted_Beats;						//  50: Num_Accepted_Beats
	stream << m_pData->header.Scale_Factor;									//  54: Scale_Factor
	stream << m_pData->header.Scan_Min;											//  54: Scan_Min
	stream << m_pData->header.Scan_Max;											//  60: Scan_Max
	stream << m_pData->header.Prompts;												//  62: Prompts
	stream << m_pData->header.Delayed;												//  66: Delayed
	stream << m_pData->header.Multiples;											//  70: Multiples
	stream << m_pData->header.Net_Trues;											//  74: Net_Trues
	for(int i=0; i < 16; i++)
		stream << m_pData->header.Cor_Singles[i];							//  78: Cor_Singles
	for(int i=0; i < 16; i++)
		stream << m_pData->header.Uncor_Singles[i];						// 142: Uncor_Singles
	stream << m_pData->header.Tot_Avg_Cor;										// 206: Tot_Avg_Cor
	stream << m_pData->header.Tot_Avg_Uncor;									// 210: Tot_Avg_Uncor
	stream << m_pData->header.Total_Coin_Rate;								// 214: Total_Coin_Rate
	stream << m_pData->header.Frame_Start_Time;							// 218: Frame_Start_Time
	stream << m_pData->header.Frame_Duration;								// 222: Frame_Duration
	stream << m_pData->header.Deadtime_Correction_Factor;		// 226: Deadtime_Correction_Factor
	for(int i=0; i < 8; i++)
		stream << m_pData->header.Physical_Planes[i];					// 230: Physical_Planes
	for(int i=0; i < 83; i++)
		stream << m_pData->header.CTI_reserved[i];							// 246: CTI_reserved
	for(int i=0; i < 50; i++)
		stream << m_pData->header.User_Reserved[i];						// 412: User_Reserved	
	
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

int CECAT7SubHeaderScan::rawDataSize() const
{
	return 1*ECAT_BLOCKSIZE;
}

CECATSubHeader::Type CECAT7SubHeaderScan::subHeaderType(void) const
{
	return CECATSubHeader::ECAT7_Scan;
}

bool CECAT7SubHeaderScan::convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader*) 
{
	ENTER();

	bool bResult = false;
	// depending on the MedIOHeader format we do have to 
	// distinguish between our copy operations.
	switch(pHead1->headerFormat())
	{
		case CMedIOHeader::ECATSubHeader:
		{
			const CECATSubHeader* eSubHeader = static_cast<const CECATSubHeader*>(pHead1);

			// depending on the source type we have to copy either every data or just 
			// some data of the src header
			switch(eSubHeader->subHeaderType())
			{
				// if the source header is also an ECAT7 one we can copy it in whole
				// via a simple memcpy()
				case CECATSubHeader::ECAT7_Scan:
				{
					// we use the assignment operator which will do the convertation
					// for us.
					*this = *static_cast<const CECAT7SubHeaderScan*>(pHead1);
					
					bResult = true;
				}
				break;

				case CECATSubHeader::Unknown:
					// for an unknown header type we do nothing
				break;
				
				#warning "non Scan copy not complete"
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

	RETURN(bResult);
	return bResult;
}

CMedIOHeader* CECAT7SubHeaderScan::clone() const
{
	ENTER();

	CECAT7SubHeaderScan* pNewHead = new CECAT7SubHeaderScan();
	pNewHead->convertFrom(this);

	RETURN(pNewHead);
	return pNewHead;
}

// data access methods
CECATSubHeader::Data_Type CECAT7SubHeaderScan::data_Type(void) const
{
	return static_cast<CECATSubHeader::Data_Type>(m_pData->header.Data_Type);	}

short CECAT7SubHeaderScan::num_Dimensions(void) const
{
	return m_pData->header.Num_Dimensions;
}

short CECAT7SubHeaderScan::num_R_Elements(void) const
{
	return m_pData->header.Num_R_Elements;
}

short CECAT7SubHeaderScan::num_Angles(void) const
{
	return m_pData->header.Num_Angles;
}

short CECAT7SubHeaderScan::corrections_Applied(void) const
{
	return m_pData->header.Corrections_Applied;
}

short CECAT7SubHeaderScan::num_Z_Elements(void) const
{
	return m_pData->header.Num_Z_Elements;
}

short CECAT7SubHeaderScan::ring_Difference(void) const
{
	return m_pData->header.Ring_Difference;
}

float CECAT7SubHeaderScan::x_Resolution(void) const
{
	return m_pData->header.X_Resolution;
}

float CECAT7SubHeaderScan::y_Resolution(void) const
{
	return m_pData->header.Y_Resolution;
}

float CECAT7SubHeaderScan::z_Resolution(void) const
{
	return m_pData->header.Z_Resolution;
}

float CECAT7SubHeaderScan::w_Resolution(void) const
{
	return m_pData->header.W_Resolution;
}

short CECAT7SubHeaderScan::gate_Reserved(const short i) const
{
	return m_pData->header.Gate_Reserved[i];
}

unsigned int CECAT7SubHeaderScan::gate_Duration(void) const
{
	return m_pData->header.Gate_Duration;
}

unsigned int CECAT7SubHeaderScan::r_Wave_Offset(void) const
{
	return m_pData->header.R_Wave_Offset;
}

unsigned int CECAT7SubHeaderScan::num_Accepted_Beats(void) const
{
	return m_pData->header.Num_Accepted_Beats;
}

float CECAT7SubHeaderScan::scale_Factor(void) const
{
	return m_pData->header.Scale_Factor;
}

short CECAT7SubHeaderScan::scan_Min(void) const
{
	return m_pData->header.Scan_Min;
}

short CECAT7SubHeaderScan::scan_Max(void) const
{
	return m_pData->header.Scan_Max;
}

unsigned int CECAT7SubHeaderScan::prompts(void) const
{
	return m_pData->header.Prompts;
}

unsigned int CECAT7SubHeaderScan::delayed(void) const
{
	return m_pData->header.Delayed;
}

unsigned int CECAT7SubHeaderScan::multiples(void) const
{
	return m_pData->header.Multiples;
}

unsigned int CECAT7SubHeaderScan::net_Trues(void) const
{
	return m_pData->header.Net_Trues;
}

float CECAT7SubHeaderScan::cor_Singles(const short i) const
{
	return m_pData->header.Cor_Singles[i];
}

float CECAT7SubHeaderScan::uncor_Singles(const short i) const
{
	return m_pData->header.Uncor_Singles[i];
}

float CECAT7SubHeaderScan::tot_Avg_Cor(void) const
{
	return m_pData->header.Tot_Avg_Cor;
}

float CECAT7SubHeaderScan::tot_Avg_Uncor(void) const
{
	return m_pData->header.Tot_Avg_Uncor;
}

unsigned int CECAT7SubHeaderScan::total_Coin_Rate(void) const
{
	return m_pData->header.Total_Coin_Rate;
}

unsigned int CECAT7SubHeaderScan::frame_Start_Time(void) const
{
	return m_pData->header.Frame_Start_Time;
}

unsigned int CECAT7SubHeaderScan::frame_Duration(void) const
{
	return m_pData->header.Frame_Duration;
}

float CECAT7SubHeaderScan::deadtime_Correction_Factor(void) const
{
	return m_pData->header.Deadtime_Correction_Factor;
}

short CECAT7SubHeaderScan::physical_Planes(const short i) const
{
	return m_pData->header.Physical_Planes[i];
}

short CECAT7SubHeaderScan::cti_Reserved(const short i) const
{
	return m_pData->header.CTI_reserved[i];
}

short CECAT7SubHeaderScan::user_Reserved(const short i) const
{
	return m_pData->header.User_Reserved[i];
}


void CECAT7SubHeaderScan::setData_Type(const CECATSubHeader::Data_Type dType)
{
	m_pData->header.Data_Type = static_cast<Q_UINT16>(dType);	}	

void CECAT7SubHeaderScan::setNum_Dimensions(const short n)
{
	m_pData->header.Num_Dimensions = n;
}

void CECAT7SubHeaderScan::setNum_R_Elements(const short n)
{
	m_pData->header.Num_R_Elements = n;
}

void CECAT7SubHeaderScan::setNum_Angles(const short n)
{
	m_pData->header.Num_Angles = n;
}

void CECAT7SubHeaderScan::setCorrections_Applied(const short n)
{
	m_pData->header.Corrections_Applied = n;
}

void CECAT7SubHeaderScan::setNum_Z_Elements(const short n)
{
	m_pData->header.Num_Z_Elements = n;
}

void CECAT7SubHeaderScan::setRing_Difference(const short n)
{
	m_pData->header.Ring_Difference = n;
}

void CECAT7SubHeaderScan::setX_Resolution(const float n)
{
	m_pData->header.X_Resolution = n;
}

void CECAT7SubHeaderScan::setY_Resolution(const float n)
{
	m_pData->header.Y_Resolution = n;
}

void CECAT7SubHeaderScan::setZ_Resolution(const float n)
{
	m_pData->header.Z_Resolution = n;
}

void CECAT7SubHeaderScan::setW_Resolution(const float n)
{
	m_pData->header.W_Resolution = n;
}

void CECAT7SubHeaderScan::setGate_Reserved(const short i, const short n)
{
	m_pData->header.Gate_Reserved[i] = n;
}

void CECAT7SubHeaderScan::setGate_Duration(const unsigned int n)
{
	m_pData->header.Gate_Duration = n;
}

void CECAT7SubHeaderScan::setR_Wave_Offset(const unsigned int n)
{
	m_pData->header.R_Wave_Offset = n;
}

void CECAT7SubHeaderScan::setNum_Accepted_Beats(const unsigned int n)
{
	m_pData->header.Num_Accepted_Beats = n;
}

void CECAT7SubHeaderScan::setScale_Factor(const float n)
{
	m_pData->header.Scale_Factor = n;
}

void CECAT7SubHeaderScan::setScan_Min(const short n)
{
	m_pData->header.Scan_Min = n;
}

void CECAT7SubHeaderScan::setScan_Max(const short n)
{
	m_pData->header.Scan_Max = n;
}

void CECAT7SubHeaderScan::setPrompts(const unsigned int n)
{
	m_pData->header.Prompts = n;
}

void CECAT7SubHeaderScan::setDelayed(const unsigned int n)
{
	m_pData->header.Delayed = n;
}

void CECAT7SubHeaderScan::setMultiples(const unsigned int n)
{
	m_pData->header.Multiples = n;
}

void CECAT7SubHeaderScan::setNet_Trues(const unsigned int n)
{
	m_pData->header.Net_Trues = n;
}

void CECAT7SubHeaderScan::setCor_Singles(const short i, const float n)
{
	m_pData->header.Cor_Singles[i] = n;
}

void CECAT7SubHeaderScan::setUncor_Singles(const short i, const float n)
{
	m_pData->header.Uncor_Singles[i] = n;
}

void CECAT7SubHeaderScan::setTot_Avg_Cor(const float n)
{
	m_pData->header.Tot_Avg_Cor = n;
}

void CECAT7SubHeaderScan::setTot_Avg_Uncor(const float n)
{
	m_pData->header.Tot_Avg_Uncor = n;
}

void CECAT7SubHeaderScan::setTotal_Coin_Rate(const unsigned int n)
{
	m_pData->header.Total_Coin_Rate = n;
}

void CECAT7SubHeaderScan::setFrame_Start_Time(const unsigned int n)
{
	m_pData->header.Frame_Start_Time = n;
}

void CECAT7SubHeaderScan::setFrame_Duration(const unsigned int n)
{
	m_pData->header.Frame_Duration = n;
}

void CECAT7SubHeaderScan::setDeadtime_Correction_Factor(const float n)
{
	m_pData->header.Deadtime_Correction_Factor = n;
}

void CECAT7SubHeaderScan::setPhysical_Planes(const short i, const short n)
{
	m_pData->header.Physical_Planes[i] = n;
}

void CECAT7SubHeaderScan::setCTI_Reserved(const short i, const short n)
{
	m_pData->header.CTI_reserved[i] = n;
}

void CECAT7SubHeaderScan::setUser_Reserved(const short i, const short n)
{
	m_pData->header.User_Reserved[i] = n;
}

