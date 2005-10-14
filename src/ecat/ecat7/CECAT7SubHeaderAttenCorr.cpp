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

CECAT7SubHeaderAttenCorr::CECAT7SubHeaderAttenCorr(const CECAT7SubHeaderAttenCorr& sh)
	: CECATSubHeader(sh)
{
	// then clear the structure
	memcpy(&m_Data, &sh.m_Data, sizeof(struct ECAT7SubHeader_AttenCorr));
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

CMedIOHeader& CECAT7SubHeaderAttenCorr::copyData(const CMedIOHeader& src)
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
				case CECATSubHeader::ECAT7_AttenCorr:
				{
					memcpy(&(this->m_Data), &(static_cast<const CECAT7SubHeaderAttenCorr*>(&src)->m_Data), sizeof(struct ECAT7SubHeader_AttenCorr));
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

	LEAVE();
	return *this;
}
