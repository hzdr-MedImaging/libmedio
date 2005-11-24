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

#include "CECAT7SubHeaderAttenCorr.h"
#include "CECATDirectoryItem.h"
#include "CECATFile.h"

#include <qdatastream.h>

#include <rtdebug.h>

CECAT7SubHeaderAttenCorr::CECAT7SubHeaderAttenCorr(CECATFile* ecatFile,
																									 CECATDirectoryItem* pDirItem)
	: CECATSubHeader(ecatFile, pDirItem)
{
	// then clear the structure
	memset(&m_Data, 0, sizeof(struct ECAT7SubHeader_AttenCorr));
}

CECAT7SubHeaderAttenCorr::CECAT7SubHeaderAttenCorr()
	: CECATSubHeader(NULL)
{
	// then clear the structure
	memset(&m_Data, 0, sizeof(struct ECAT7SubHeader_AttenCorr));
}

bool CECAT7SubHeaderAttenCorr::load(void)
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
	stream >> m_Data.Data_Type;										//   0: Data_Type
	stream >> m_Data.Num_Dimensions;							//   2: Num_Dimensions
	stream >> m_Data.Attenuation_Type;						//   4: Attenuation_Type
	stream >> m_Data.Num_R_Elements;							//   6: Num_R_Elements
	stream >> m_Data.Num_Angles;									//   8: Num_Angles
	stream >> m_Data.Num_Z_Elements;							//  10: Num_Z_Elements
	stream >> m_Data.Ring_Difference;							//  12: Ring_Difference
	stream >> m_Data.X_Resolution;								//  14: X_Resolution
	stream >> m_Data.Y_Resolution;								//  18: Y_Resolution
	stream >> m_Data.Z_Resolution;								//  22: Z_Resolution
	stream >> m_Data.W_Resolution;								//  26: W_Resolution
	stream >> m_Data.Scale_Factor;								//  30: Scale_Factor
	stream >> m_Data.X_Offset;										//  34: X_Offset
	stream >> m_Data.Y_Offset;										//  38: Y_Offset
	stream >> m_Data.X_Radius;										//  42: X_Radius
	stream >> m_Data.Y_Radius;										//  46: Y_Radius
	stream >> m_Data.Tilt_Angle;									//  50: Tilt_Angle
	stream >> m_Data.Attenuation_Coeff;						//  54: Attenuation_Coeff
	stream >> m_Data.Attenuation_Min;							//  58: Attenuation_Min
	stream >> m_Data.Attenuation_Max;							//  62: Attenuation_Max
	stream >> m_Data.Skull_Thickness;							//  66: Skull_Thickness
	stream >> m_Data.Num_Additional_Atten_Coeff;	//  70: Num_Additional_Atten_Coeff
	for(int i=0; i < 8; i++)
		stream >> m_Data.Additional_Atten_Coeff[i];	//  72: Additional_Atten_Coeff (8)
	stream >> m_Data.Edge_Finding_Threshold;			// 104: Edge_Finding_Threshold
	stream >> m_Data.Storage_Order;								// 108: Storage_Order
	stream >> m_Data.Span;												// 110: Span
	for(int i=0; i < 64; i++)
		stream >> m_Data.Z_Elements[i];							// 112: Z_Elements (64)
	for(int i=0; i < 86; i++)
		stream >> m_Data.Unused[i];									// 240: Unused (86)
	for(int i=0; i < 64; i++)
		stream >> m_Data.CTI_Reserved[i];						// 412: CTI_Reserved (64)

	// some more debug output
#if defined(DEBUG)
	D("ECAT7 Attenuation Correction SubHeader loaded:");
	D("---------------------------------------------");
	D("Data_Type	               : %d",				m_Data.Data_Type);
	D("Num_Dimensions					   : %d",				m_Data.Num_Dimensions);
	D("Attenuation_Type          : %d",				m_Data.Attenuation_Type);
	D("Num_R_Elements		         : %d",				m_Data.Num_R_Elements);
	D("Num_Angles                : %d",				m_Data.Num_Angles);
	D("Num_Z_Elements            : %d",				m_Data.Num_Z_Elements);
	D("Ring_Difference           : %d",				m_Data.Ring_Difference);
	D("X_Resolution              : %f cm",		m_Data.X_Resolution);
	D("Y_Resolution              : %f cm",		m_Data.Y_Resolution);
	D("Z_Resolution              : %f cm",		m_Data.Z_Resolution);
	D("W_Resolution              : %f",				m_Data.W_Resolution);
	D("Scale_Factor              : %f",				m_Data.Scale_Factor);
	D("X_Offset				           : %f cm",		m_Data.X_Offset);
	D("Y_Offset			             : %f cm",		m_Data.Y_Offset);
	D("X_Radius                  : %f cm",		m_Data.X_Radius);
	D("Y_Radius                  : %f cm",		m_Data.Y_Radius);
	D("Tilt_Angle                : %f°",			m_Data.Tilt_Angle);
	D("Attenuation_Coeff         : %f cm^-1",	m_Data.Attenuation_Coeff);
	D("Attenuation_Min           : %f",				m_Data.Attenuation_Min);
	D("Attenuation_Max           : %f",				m_Data.Attenuation_Max);
	D("Skull_Thickness           : %f cm",		m_Data.Skull_Thickness);
	D("Num_Additional_Atten_Coeff: %d",				m_Data.Num_Additional_Atten_Coeff);
	for(int i=0; i < m_Data.Num_Additional_Atten_Coeff; i++)
	{
		D("Additional_Atten_Coeff[%d]: %f", i+1, m_Data.Additional_Atten_Coeff[i]);
	}
	D("Edge_Finding_Threshold    : %f",				m_Data.Edge_Finding_Threshold);
	D("Storage_Order						 : %d",				m_Data.Storage_Order);
	D("Span				               : %d",				m_Data.Span);
	for(int i=0; i < 64; i++)
	{
		D("Z_Elements							[%d]: %f", i+1, m_Data.Z_Elements[i]);
	}
#endif

	RETURN(true);
	return true;
}

bool CECAT7SubHeaderAttenCorr::save(void) const
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
	stream << m_Data.Data_Type;										//   0: Data_Type
	stream << m_Data.Num_Dimensions;							//   2: Num_Dimensions
	stream << m_Data.Attenuation_Type;						//   4: Attenuation_Type
	stream << m_Data.Num_R_Elements;							//   6: Num_R_Elements
	stream << m_Data.Num_Angles;									//   8: Num_Angles
	stream << m_Data.Num_Z_Elements;							//  10: Num_Z_Elements
	stream << m_Data.Ring_Difference;							//  12: Ring_Difference
	stream << m_Data.X_Resolution;								//  14: X_Resolution
	stream << m_Data.Y_Resolution;								//  18: Y_Resolution
	stream << m_Data.Z_Resolution;								//  22: Z_Resolution
	stream << m_Data.W_Resolution;								//  26: W_Resolution
	stream << m_Data.Scale_Factor;								//  30: Scale_Factor
	stream << m_Data.X_Offset;										//  34: X_Offset
	stream << m_Data.Y_Offset;										//  38: Y_Offset
	stream << m_Data.X_Radius;										//  42: X_Radius
	stream << m_Data.Y_Radius;										//  46: Y_Radius
	stream << m_Data.Tilt_Angle;									//  50: Tilt_Angle
	stream << m_Data.Attenuation_Coeff;						//  54: Attenuation_Coeff
	stream << m_Data.Attenuation_Min;							//  58: Attenuation_Min
	stream << m_Data.Attenuation_Max;							//  62: Attenuation_Max
	stream << m_Data.Skull_Thickness;							//  66: Skull_Thickness
	stream << m_Data.Num_Additional_Atten_Coeff;	//  70: Num_Additional_Atten_Coeff
	for(int i=0; i < 8; i++)
		stream << m_Data.Additional_Atten_Coeff[i];	//  72: Additional_Atten_Coeff (8)
	stream << m_Data.Edge_Finding_Threshold;			// 104: Edge_Finding_Threshold
	stream << m_Data.Storage_Order;								// 108: Storage_Order
	stream << m_Data.Span;												// 110: Span
	for(int i=0; i < 64; i++)
		stream << m_Data.Z_Elements[i];							// 112: Z_Elements (64)
	for(int i=0; i < 86; i++)
		stream << m_Data.Unused[i];									// 240: Unused (86)
	for(int i=0; i < 64; i++)
		stream << m_Data.CTI_Reserved[i];						// 412: CTI_Reserved (64)		

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

int CECAT7SubHeaderAttenCorr::rawDataSize() const 
{ 
	return 1*ECAT_BLOCKSIZE;
}

CECATSubHeader::Type CECAT7SubHeaderAttenCorr::subHeaderType(void) const
{
	return CECATSubHeader::ECAT7_AttenCorr;
}

bool CECAT7SubHeaderAttenCorr::convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader*) 
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
				case CECATSubHeader::ECAT7_AttenCorr:
				{
					memcpy(&(this->m_Data), &(static_cast<const CECAT7SubHeaderAttenCorr*>(pHead1)->m_Data), sizeof(struct ECAT7SubHeader_AttenCorr));
					bResult = true;
				}
				break;

				case CECATSubHeader::Unknown:
					// for an unknown header type we do nothing
				break;
				
				#warning "non AttenCorr copy not complete"
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

CMedIOHeader* CECAT7SubHeaderAttenCorr::clone() const
{
	ENTER();

	CECAT7SubHeaderAttenCorr* pNewHead = new CECAT7SubHeaderAttenCorr();
	pNewHead->convertFrom(this);

	RETURN(pNewHead);
	return pNewHead;
}

// data access methods
CECATSubHeader::Data_Type CECAT7SubHeaderAttenCorr::data_Type(void) const
{
	return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type);
}

short	CECAT7SubHeaderAttenCorr::num_Dimensions(void) const
{
	return m_Data.Num_Dimensions;
}

CECAT7SubHeaderAttenCorr::Attenuation_Type CECAT7SubHeaderAttenCorr::attenuation_Type(void) const
{
	return static_cast<CECAT7SubHeaderAttenCorr::Attenuation_Type>(m_Data.Attenuation_Type);
}

short CECAT7SubHeaderAttenCorr::num_R_Elements(void) const
{
	return m_Data.Num_R_Elements;
}

short CECAT7SubHeaderAttenCorr::num_Angles(void) const
{
	return m_Data.Num_Angles;
}

short CECAT7SubHeaderAttenCorr::num_Z_Elements(void) const
{
	return m_Data.Num_Z_Elements;
}

short CECAT7SubHeaderAttenCorr::ring_Difference(void) const
{
	return m_Data.Ring_Difference;
}

float CECAT7SubHeaderAttenCorr::x_Resolution(void) const
{
	return m_Data.X_Resolution;
}

float CECAT7SubHeaderAttenCorr::y_Resolution(void) const
{
	return m_Data.Y_Resolution;
}

float	CECAT7SubHeaderAttenCorr::z_Resolution(void) const
{
	return m_Data.Z_Resolution;
}

float CECAT7SubHeaderAttenCorr::w_Resolution(void) const
{
	return m_Data.W_Resolution;
}

float	CECAT7SubHeaderAttenCorr::scale_Factor(void) const
{
	return m_Data.Scale_Factor;
}

float	CECAT7SubHeaderAttenCorr::x_Offset(void) const
{
	return m_Data.X_Offset;
}

float	CECAT7SubHeaderAttenCorr::y_Offset(void) const
{
	return m_Data.Y_Offset;
}

float	CECAT7SubHeaderAttenCorr::x_Radius(void) const
{
	return m_Data.X_Radius;
}

float	CECAT7SubHeaderAttenCorr::y_Radius(void) const
{
	return m_Data.Y_Radius;
}

float	CECAT7SubHeaderAttenCorr::tilt_Angle(void) const
{
	return m_Data.Tilt_Angle;
}

float CECAT7SubHeaderAttenCorr::attenuation_Coeff(void) const
{
	return m_Data.Attenuation_Coeff;
}

float	CECAT7SubHeaderAttenCorr::attenuation_Min(void) const
{
	return m_Data.Attenuation_Min;
}

float	CECAT7SubHeaderAttenCorr::attenuation_Max(void) const
{
	return m_Data.Attenuation_Max;
}

float	CECAT7SubHeaderAttenCorr::skull_Thickness(void) const
{
	return m_Data.Skull_Thickness;
}

short	CECAT7SubHeaderAttenCorr::num_Additional_Atten_Coeff(void) const
{
	return m_Data.Num_Additional_Atten_Coeff;
}

float	CECAT7SubHeaderAttenCorr::additional_Atten_Coeff(int n) const
{
	return m_Data.Additional_Atten_Coeff[n];
}

float	CECAT7SubHeaderAttenCorr::edge_Finding_Threshold(void) const
{
	return m_Data.Edge_Finding_Threshold;
}

short CECAT7SubHeaderAttenCorr::storage_Order(void) const
{
	return m_Data.Storage_Order;
}

short CECAT7SubHeaderAttenCorr::span(void) const
{
	return m_Data.Span;
}

short CECAT7SubHeaderAttenCorr::z_Elements(int n) const
{
	return m_Data.Z_Elements[n];
}

short	CECAT7SubHeaderAttenCorr::unused(int n) const
{
	return m_Data.Unused[n];
}

short CECAT7SubHeaderAttenCorr::cti_Reserved(int n) const
{
	return m_Data.CTI_Reserved[n];
}

void CECAT7SubHeaderAttenCorr::setData_Type(const CECATSubHeader::Data_Type dType)
{
	m_Data.Data_Type = static_cast<Q_UINT16>(dType);
}

void CECAT7SubHeaderAttenCorr::setNum_Dimensions(const short n)
{
	m_Data.Num_Dimensions = n;
}

void CECAT7SubHeaderAttenCorr::setAttenuation_Type(const Attenuation_Type n)
{
	m_Data.Attenuation_Type = static_cast<Q_UINT16>(n);
}

void CECAT7SubHeaderAttenCorr::setNum_R_Elements(const short n)
{
	m_Data.Num_R_Elements = n;
}

void CECAT7SubHeaderAttenCorr::setNum_Angles(const short n)
{
	m_Data.Num_Angles = n;
}

void CECAT7SubHeaderAttenCorr::setNum_Z_Elements(const short n)
{
	m_Data.Num_Z_Elements = n;
}

void CECAT7SubHeaderAttenCorr::setRing_Difference(const short n)
{
	m_Data.Ring_Difference = n;
}

void CECAT7SubHeaderAttenCorr::setX_Resolution(const float n)
{
	m_Data.X_Resolution = n;
}

void CECAT7SubHeaderAttenCorr::setY_Resolution(const float n)
{
	m_Data.Y_Resolution = n;
}

void CECAT7SubHeaderAttenCorr::setZ_Resolution(const float n)
{
	m_Data.Z_Resolution = n;
}

void CECAT7SubHeaderAttenCorr::setW_Resolution(const float n)
{
	m_Data.W_Resolution = n;
}

void CECAT7SubHeaderAttenCorr::setScale_Factor(const float n)
{
	m_Data.Scale_Factor = n;
}

void CECAT7SubHeaderAttenCorr::setX_Offset(const float n)
{
	m_Data.X_Offset = n;
}

void CECAT7SubHeaderAttenCorr::setY_Offset(const float n)
{
	m_Data.Y_Offset = n;
}

void CECAT7SubHeaderAttenCorr::setX_Radius(const float n)
{
	m_Data.X_Radius = n;
}

void CECAT7SubHeaderAttenCorr::setY_Radius(const float n)
{
	m_Data.Y_Radius = n;
}

void CECAT7SubHeaderAttenCorr::setTilt_Angle(const float n)
{
	m_Data.Tilt_Angle = n;
}

void CECAT7SubHeaderAttenCorr::setAttenuation_Coeff(const float n)
{
	m_Data.Attenuation_Coeff = n;
}

void CECAT7SubHeaderAttenCorr::setAttenuation_Min(const float n)
{
	m_Data.Attenuation_Min = n;
}

void CECAT7SubHeaderAttenCorr::setAttenuation_Max(const float n)
{
	m_Data.Attenuation_Max = n;
}

void CECAT7SubHeaderAttenCorr::setSkull_Thickness(const float n)
{
	m_Data.Skull_Thickness = n;
}

void CECAT7SubHeaderAttenCorr::setNum_Additional_Atten_Coeff(const short n)
{
	m_Data.Num_Additional_Atten_Coeff = n;
}

void CECAT7SubHeaderAttenCorr::setAdditional_Atten_Coeff(const short i, const float n)
{
	m_Data.Additional_Atten_Coeff[i] = n;
}

void CECAT7SubHeaderAttenCorr::setEdge_Finding_Threshold(const float n)
{
	m_Data.Edge_Finding_Threshold = n;
}

void CECAT7SubHeaderAttenCorr::setStorage_Order(const short n)
{
	m_Data.Storage_Order = n;
}

void CECAT7SubHeaderAttenCorr::setSpan(const short n)
{
	m_Data.Span = n;
}

void CECAT7SubHeaderAttenCorr::setZ_Elements(const short i, const short n)
{
	m_Data.Z_Elements[i] = n;
}

void CECAT7SubHeaderAttenCorr::setUnused(const short i, const short n)
{
	m_Data.Unused[i] = n;
}

void CECAT7SubHeaderAttenCorr::setCTI_Reserverd(const short i, const short n)
{
	m_Data.CTI_Reserved[i] = n;
}

