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

#include <QDataStream>

#include <rtdebug.h>

CECAT7SubHeaderNorm::CECAT7SubHeaderNorm(CECATFile* ecatFile,
																				 CECATDirectoryItem* pDirItem)
	: CECATSubHeader(ecatFile, pDirItem)
{
	// then clear the structure
	memset(&m_Data, 0, sizeof(struct ECAT7SubHeader_Norm));
}

CECAT7SubHeaderNorm::CECAT7SubHeaderNorm(const CECAT7SubHeaderNorm& sh)
	: CECATSubHeader(sh)
{
	// then copy the structure
	memcpy(&m_Data, &sh.m_Data, sizeof(struct ECAT7SubHeader_Norm));
}

bool CECAT7SubHeaderNorm::load(void)
{
	ENTER();

	// check if the stream is readable or not and
	// set our MedIOData to the correct file position so that we can
	// read the subheader	
	if(m_pMedIOData->isReadable() == false ||
		 m_pDirItem->dataBlock_Start() == 0 ||
		 m_pMedIOData->seek(m_pDirItem->dataBlock_Start()) == false)
	{
		RETURN(false);
		return false;
	}
	
	// we use a ByteArray buffer to speed up the endianess
	// decoding
	QByteArray buffer(rawDataSize(), 0);
	if(m_pMedIOData->read(buffer.data(), buffer.size()) != rawDataSize())
	{
		RETURN(false);
		return false;
	}

	// now we generate a QDataStream on our buffer so that we can read
	// out of the buffer instead of the raw file (> speed)
	QDataStream stream(buffer);

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
		 m_pMedIOData->seek(m_pDirItem->dataBlock_Start()) == false)
	{
		RETURN(false);
		return false;
	}

	SHOWVALUE(m_pMedIOData->pos());

	// we write to a buffer first and write out later directly to the file
	QByteArray buffer(rawDataSize(), 0);
	QDataStream stream(&buffer, QIODevice::WriteOnly);

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
	if(m_pMedIOData->write(buffer) != -1)
	{
		m_pDirItem->subHeaderWritten(*this);
		result = true;
	}

	RETURN(result);
	return result;
}

CMedIOHeader& CECAT7SubHeaderNorm::copyData(const CMedIOHeader& src)
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
				case CECATSubHeader::ECAT7_Norm:
				{
					memcpy(&(this->m_Data), &(static_cast<const CECAT7SubHeaderNorm*>(&src)->m_Data), sizeof(struct ECAT7SubHeader_Norm));
				}
				break;

				case CECATSubHeader::Unknown:
					// for an unknown header type we do nothing
				break;
				
				#warning "non Norm copy not complete"
			}
		}

		case CMedIOHeader::ECATMainHeader:
			// copying a main header into a sub header doesn't make much sense, so we
			// do nothing here
		break;

		case CMedIOHeader::ConcordeMicropet:
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
