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

#include "CECAT7SubHeaderNorm3D.h"
#include "CECATDirectoryItem.h"
#include "CECATFile.h"

#include <qdatastream.h>

#include <rtdebug.h>

CECAT7SubHeaderNorm3D::CECAT7SubHeaderNorm3D(CECATFile* ecatFile,
																						 CECATDirectoryItem* pDirItem)
	: CECATSubHeader(ecatFile, pDirItem)
{
	// check that the headsize is 512 bytes long
	ASSERT(sizeof(struct ECAT7SubHeader_Norm3D) == ECAT7_HEADERSIZE_NORM3D);

	// then clear the structure
	memset(&m_Data, 0, sizeof(struct ECAT7SubHeader_Norm3D));
}

CECAT7SubHeaderNorm3D::CECAT7SubHeaderNorm3D(const CECAT7SubHeaderNorm3D& sh)
	: CECATSubHeader(sh)
{
	// check that the headsize is 512 bytes long
	ASSERT(sizeof(struct ECAT7SubHeader_Norm3D) == ECAT7_HEADERSIZE_NORM3D);

	// then copy the structure
	memcpy(&m_Data, &sh.m_Data, sizeof(struct ECAT7SubHeader_Norm3D));
}

bool CECAT7SubHeaderNorm3D::load(void)
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
	QByteArray buffer(sizeof(struct ECAT7SubHeader_Norm3D));
	if(m_pMedIOData->readBlock(buffer.data(), sizeof(struct ECAT7SubHeader_Norm3D)) 
			!= sizeof(struct ECAT7SubHeader_Norm3D))
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
	stream >> m_Data.Num_R_Elements;								//   2: Num_R_Elements
	stream >> m_Data.Num_Transaxial_Crystals;				//   4: Num_Transaxial_Crystals
	stream >> m_Data.Num_Crystal_Rings;							//   6: Num_Crystal_Rings
	stream >> m_Data.Crystals_Per_Ring;							//   8: Crystals_Per_Ring
	stream >> m_Data.Num_Geo_Corr_Planes;						//  10: Num_Geo_Corr_Planes
	stream >> m_Data.ULD;														//  12: ULD
	stream >> m_Data.LLD;														//  14: LLD
	stream >> m_Data.Scatter_Energy;								//  16: Scatter_Energy
	stream >> m_Data.Norm_Quality_Factor;						//  18: Norm_Quality_Factor
	stream >> m_Data.Norm_Quality_Factor_Code;			//  22: Norm_Quality_Factor
	for(int i=0; i < 32; i++)
		stream >> m_Data.Ring_DTCor1[i];							//  24: Ring_DTCor1 (32)
	for(int i=0; i < 32; i++)
		stream >> m_Data.Ring_DTCor2[i];							// 152: Ring_DTCor2 (32)
	for(int i=0; i < 8; i++)
		stream >> m_Data.Crystal_DTCor[i];						// 280: Crystal_DTCor (8)
	stream >> m_Data.Span;													// 312: Span
	stream >> m_Data.Max_Ring_Diff;									// 314: Max_Ring_Diff
	for(int i=0; i < 48; i++)
		stream >> m_Data.CTI_reserved[i];							// 316: CTI_reserved
	for(int i=0; i < 50; i++)
		stream >> m_Data.User_Reserved[i];						// 412: User_Reserved

	// some more debug output
#if defined(DEBUG)
	D("ECAT7 Normalization3D SubHeader loaded:");
	D("------------------------------------");
	D("Data_Type                 : %d",				m_Data.Data_Type);
	D("Num_R_Elements            : %d",				m_Data.Num_R_Elements);
	D("Num_Transaxial_Crystals   : %d",				m_Data.Num_Transaxial_Crystals);
	D("Num_Crystal_Rings         : %d",				m_Data.Num_Crystal_Rings);
	D("Crystals_Per_Ring         : %d",				m_Data.Crystals_Per_Ring);
	D("Num_Geo_Corr_Planes       : %d",				m_Data.Num_Geo_Corr_Planes);
	D("ULD                       : %d",				m_Data.ULD);
	D("LLD                       : %d",				m_Data.LLD);
	D("Scatter_Energy            : %d",				m_Data.Scatter_Energy);
	D("Norm_Quality_Factor       : %g",				m_Data.Norm_Quality_Factor);
	D("Norm_Quality_Factor_Code  : %d",				m_Data.Norm_Quality_Factor_Code);
	for(int i=0; i < 32; i++)
	{
		D("Ring_DTCOR1           [%02d]: %g", i+1, m_Data.Ring_DTCor1[i]);
	}
	for(int i=0; i < 32; i++)
	{
		D("Ring_DTCOR2           [%02d]: %g", i+1, m_Data.Ring_DTCor2[i]);
	}
	for(int i=0; i < 8; i++)
	{
		D("Crystal_DTCor         [%02d]: %g", i+1, m_Data.Crystal_DTCor[i]);
	}
	D("Span                      : %d",				m_Data.Span);
	D("Max_Ring_Diff             : %d",				m_Data.Max_Ring_Diff);
#endif

	RETURN(true);
	return true;
}

bool CECAT7SubHeaderNorm3D::save(void) const
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
	QByteArray buffer(sizeof(struct ECAT7SubHeader_Norm3D));
	QDataStream stream(buffer, IO_WriteOnly);
	
	// lets read in each single data element of our
	// data structure to maintain the correct endianess of the
	// data
	stream << m_Data.Data_Type;											//   0: Data_Type
	stream << m_Data.Num_R_Elements;								//   2: Num_R_Elements
	stream << m_Data.Num_Transaxial_Crystals;				//   4: Num_Transaxial_Crystals
	stream << m_Data.Num_Crystal_Rings;							//   6: Num_Crystal_Rings
	stream << m_Data.Crystals_Per_Ring;							//   8: Crystals_Per_Ring
	stream << m_Data.Num_Geo_Corr_Planes;						//  10: Num_Geo_Corr_Planes
	stream << m_Data.ULD;														//  12: ULD
	stream << m_Data.LLD;														//  14: LLD
	stream << m_Data.Scatter_Energy;								//  16: Scatter_Energy
	stream << m_Data.Norm_Quality_Factor;						//  18: Norm_Quality_Factor
	stream << m_Data.Norm_Quality_Factor_Code;			//  22: Norm_Quality_Factor
	for(int i=0; i < 32; i++)
		stream << m_Data.Ring_DTCor1[i];							//  24: Ring_DTCor1 (32)
	for(int i=0; i < 32; i++)
		stream << m_Data.Ring_DTCor2[i];							// 152: Ring_DTCor2 (32)
	for(int i=0; i < 8; i++)
		stream << m_Data.Crystal_DTCor[i];						// 280: Crystal_DTCor (8)
	stream << m_Data.Span;													// 312: Span
	stream << m_Data.Max_Ring_Diff;									// 314: Max_Ring_Diff
	for(int i=0; i < 48; i++)
		stream << m_Data.CTI_reserved[i];							// 316: CTI_reserved
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

CMedIOHeader& CECAT7SubHeaderNorm3D::operator=(const CMedIOHeader& src)
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
				case CECATSubHeader::ECAT7_Norm3D:
				{
					memcpy(&(this->m_Data), &(static_cast<const CECAT7SubHeaderNorm3D*>(&src)->m_Data), sizeof(struct ECAT7SubHeader_Norm3D));
				}
				break;

				case CECATSubHeader::Unknown:
					// for an unknown header type we do nothing
				break;
				
				#warning "non Norm3D copy not complete"
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
