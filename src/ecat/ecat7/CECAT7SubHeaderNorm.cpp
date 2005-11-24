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

#include "CECAT7SubHeaderNorm.h"
#include "CECATDirectoryItem.h"
#include "CECATFile.h"

#include <qdatastream.h>

#include <rtdebug.h>

CECAT7SubHeaderNorm::CECAT7SubHeaderNorm(CECATFile* ecatFile,
																				 CECATDirectoryItem* pDirItem)
	: CECATSubHeader(ecatFile, pDirItem)
{
	// then clear the structure
	memset(&m_Data, 0, sizeof(struct ECAT7SubHeader_Norm));
}

CECAT7SubHeaderNorm::CECAT7SubHeaderNorm()
	: CECATSubHeader(NULL)
{
	// then clear the structure
	memset(&m_Data, 0, sizeof(struct ECAT7SubHeader_Norm));
}

bool CECAT7SubHeaderNorm::load(void)
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
	stream >> m_Data.Num_Z_Elements;								//   8: Num_Z_Elements
	stream >> m_Data.Ring_Difference;								//  10: Ring_Difference
	stream >> m_Data.Scale_Factor;									//  12: Scale_Factor
	stream >> m_Data.Norm_Min;											//  16: Norm_Min
	stream >> m_Data.Norm_Max;											//  20: Norm_Max
	stream >> m_Data.FOV_Source_Width;							//  24: FOV_Source_Width
	stream >> m_Data.Norm_Quality_Factor;						//  28: Norm_Quality_Factor
	stream >> m_Data.Norm_Quality_Factor_Code;			//  32: Norm_Quality_Factor_Code
	stream >> m_Data.Storage_Order;									//	34: Storage_Order
	stream >> m_Data.Span;													//  36: Span
	for(int i=0; i < 64; i++)
		stream >> m_Data.Z_Elements[i];								//  38: Z_Elements
	for(int i=0; i < 123; i++)
		stream >> m_Data.CTI_reserved[i];							// 123: CTI_reserved
	for(int i=0; i < 50; i++)
		stream >> m_Data.User_Reserved[i];						// 412: User_Reserved

	// some more debug output
#if defined(DEBUG)
	D("ECAT7 Normalization SubHeader loaded:");
	D("------------------------------------");
	D("Data_Type	               : %d",				m_Data.Data_Type);
	D("Num_Dimensions					   : %d",				m_Data.Num_Dimensions);
	D("Num_R_Elements		         : %d",				m_Data.Num_R_Elements);
	D("Num_Angles                : %d",				m_Data.Num_Angles);
	D("Num_Z_Elements            : %d",				m_Data.Num_Z_Elements);
	D("Ring_Difference           : %d",				m_Data.Ring_Difference);
	D("Scale_Factor              : %f",				m_Data.Scale_Factor);
	D("Norm_Min                  : %f",				m_Data.Norm_Min);
	D("Norm_Max                  : %f",				m_Data.Norm_Max);
	D("FOV_Source_Width          : %f cm",		m_Data.FOV_Source_Width);
	D("Norm_Quality_Factor       : %f",				m_Data.Norm_Quality_Factor);
	D("Norm_Quality_Factor_Code  : %d",				m_Data.Norm_Quality_Factor_Code);
	D("Storage_Order             : %d",				m_Data.Storage_Order);
	D("Span                      : %d",				m_Data.Span);
	for(int i=0; i < 64; i++)
	{
		D("Z_Elements							[%d]: %f", i+1, m_Data.Z_Elements[i]);
	}
#endif

	RETURN(true);
	return true;
}

bool CECAT7SubHeaderNorm::save(void) const
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
	stream << m_Data.Num_Z_Elements;								//   8: Num_Z_Elements
	stream << m_Data.Ring_Difference;								//  10: Ring_Difference
	stream << m_Data.Scale_Factor;									//  12: Scale_Factor
	stream << m_Data.Norm_Min;											//  16: Norm_Min
	stream << m_Data.Norm_Max;											//  20: Norm_Max
	stream << m_Data.FOV_Source_Width;							//  24: FOV_Source_Width
	stream << m_Data.Norm_Quality_Factor;						//  28: Norm_Quality_Factor
	stream << m_Data.Norm_Quality_Factor_Code;			//  32: Norm_Quality_Factor_Code
	stream << m_Data.Storage_Order;									//	34: Storage_Order
	stream << m_Data.Span;													//  36: Span
	for(int i=0; i < 64; i++)
		stream << m_Data.Z_Elements[i];								//  38: Z_Elements
	for(int i=0; i < 123; i++)
		stream << m_Data.CTI_reserved[i];							// 123: CTI_reserved
	for(int i=0; i < 50; i++)
		stream << m_Data.User_Reserved[i];						// 412: User_Reserved

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

int CECAT7SubHeaderNorm::rawDataSize() const
{
	return 1*ECAT_BLOCKSIZE;
}

CECATSubHeader::Type CECAT7SubHeaderNorm::subHeaderType(void) const
{
	return CECATSubHeader::ECAT7_Norm;
}

bool CECAT7SubHeaderNorm::convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader*) 
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
				case CECATSubHeader::ECAT7_Norm:
				{
					memcpy(&(this->m_Data), &(static_cast<const CECAT7SubHeaderNorm*>(pHead1)->m_Data), sizeof(struct ECAT7SubHeader_Norm));
					bResult = true;
				}
				break;

				case CECATSubHeader::Unknown:
					// for an unknown header type we do nothing
				break;
				
				#warning "non Norm copy not complete"
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

CMedIOHeader* CECAT7SubHeaderNorm::clone() const
{
	ENTER();

	CECAT7SubHeaderNorm* pNewHead = new CECAT7SubHeaderNorm();
	pNewHead->convertFrom(this);

	RETURN(pNewHead);
	return pNewHead;
}

// data access methods
CECATSubHeader::Data_Type CECAT7SubHeaderNorm::data_Type(void) const
{
	return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type);	}

short CECAT7SubHeaderNorm::num_Dimensions(void) const
{
	return m_Data.Num_Dimensions;
}

short CECAT7SubHeaderNorm::num_R_Elements(void) const
{
	return m_Data.Num_R_Elements;
}

short CECAT7SubHeaderNorm::num_Angles(void) const
{
	return m_Data.Num_Angles;
}

short CECAT7SubHeaderNorm::num_Z_Elements(void) const
{
	return m_Data.Num_Z_Elements;
}

short CECAT7SubHeaderNorm::ring_Difference(void) const
{
	return m_Data.Ring_Difference;
}

float CECAT7SubHeaderNorm::scale_Factor(void) const
{
	return m_Data.Scale_Factor;
}

float CECAT7SubHeaderNorm::norm_Min(void) const
{
	return m_Data.Norm_Min;
}

float CECAT7SubHeaderNorm::norm_Max(void) const
{
	return m_Data.Norm_Max;
}

float CECAT7SubHeaderNorm::fov_Source_Width(void) const
{
	return m_Data.FOV_Source_Width;
}

float CECAT7SubHeaderNorm::norm_Quality_Factor(void) const
{
	return m_Data.Norm_Quality_Factor;
}

CECAT7SubHeaderNorm::Norm_Qual_Factor_Code CECAT7SubHeaderNorm::norm_Quality_Factor_Code(void) const
{
	return static_cast<CECAT7SubHeaderNorm::Norm_Qual_Factor_Code>(m_Data.Norm_Quality_Factor_Code);
}

short CECAT7SubHeaderNorm::storage_Order(void) const
{
	return m_Data.Storage_Order;
}

short CECAT7SubHeaderNorm::span(void) const
{
	return m_Data.Span;
}

short CECAT7SubHeaderNorm::z_Elements(const short i) const
{
	return m_Data.Z_Elements[i];
}

short CECAT7SubHeaderNorm::cti_Reserved(const short i) const
{
	return m_Data.CTI_reserved[i];
}

short CECAT7SubHeaderNorm::user_Reserved(const short i) const
{
	return m_Data.User_Reserved[i];
}


void CECAT7SubHeaderNorm::setData_Type(const CECATSubHeader::Data_Type dType)
{
	m_Data.Data_Type = static_cast<Q_UINT16>(dType);	}	

void CECAT7SubHeaderNorm::setNum_Dimensions(const short n)
{
	m_Data.Num_Dimensions = n;
}

void CECAT7SubHeaderNorm::setNum_R_Elements(const short n)
{
	m_Data.Num_R_Elements = n;
}

void CECAT7SubHeaderNorm::setNum_Angles(const short n)
{
	m_Data.Num_Angles = n;
}

void CECAT7SubHeaderNorm::setNum_Z_Elements(const short n)
{
	m_Data.Num_Z_Elements = n;
}

void CECAT7SubHeaderNorm::setRing_Difference(const short n)
{
	m_Data.Ring_Difference = n;
}

void CECAT7SubHeaderNorm::SetScale_Factor(const float n)
{
	m_Data.Scale_Factor = n;
}

void CECAT7SubHeaderNorm::setNorm_Min(const float n)
{
	m_Data.Norm_Min = n;
}

void CECAT7SubHeaderNorm::setNorm_Max(const float n)
{
	m_Data.Norm_Max = n;
}

void CECAT7SubHeaderNorm::setFOV_Source_Width(const float n)
{
	m_Data.FOV_Source_Width = n;
}

void CECAT7SubHeaderNorm::setNorm_Quality_Factor(const float n)
{
	m_Data.Norm_Quality_Factor = n;
}

void CECAT7SubHeaderNorm::setNorm_Quality_Factor_Code(const Norm_Qual_Factor_Code n)
{
	m_Data.Norm_Quality_Factor_Code = static_cast<Q_UINT16>(n);
}

void CECAT7SubHeaderNorm::setStorage_Order(const short n)
{
	m_Data.Storage_Order = n;
}

void CECAT7SubHeaderNorm::setSpan(const short n)
{
	m_Data.Span = n;
}

void CECAT7SubHeaderNorm::setZ_Elements(const short i, const short n)
{
	m_Data.Z_Elements[i] = n;
}

void CECAT7SubHeaderNorm::setCTI_Reserved(const short i, const short n)
{
	m_Data.CTI_reserved[i] = n;
}

void CECAT7SubHeaderNorm::setUser_Reserved(const short i, const short n)
{
	m_Data.User_Reserved[i] = n;
}

