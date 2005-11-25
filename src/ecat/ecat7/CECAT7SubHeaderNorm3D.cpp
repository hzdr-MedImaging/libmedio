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

#include <QDataStream>

#include <rtdebug.h>

CECAT7SubHeaderNorm3D::CECAT7SubHeaderNorm3D(CECATFile* ecatFile,
																						 CECATDirectoryItem* pDirItem)
	: CECATSubHeader(ecatFile, pDirItem)
{
	clear();
}

CECAT7SubHeaderNorm3D::CECAT7SubHeaderNorm3D()
	: CECATSubHeader(NULL)
{
	clear();
}

void CECAT7SubHeaderNorm3D::clear()
{
	ENTER();

	// then clear the structure
	memset(&m_Data, 0, sizeof(struct ECAT7SubHeader_Norm3D));

	setData_Type(CECATSubHeader::UnknownDataType);
	setNorm_Quality_Factor_Code(TBD);

	LEAVE();
}

bool CECAT7SubHeaderNorm3D::load(void)
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
		 m_pMedIOData->seek(m_pDirItem->dataBlock_Start()) == false)
	{
		RETURN(false);
		return false;
	}

	SHOWVALUE(m_pMedIOData->pos());

	// we write to a buffer first and write out later directly to the file
	QByteArray buffer(rawDataSize(), 0);
	QDataStream stream(&buffer, QIODevice::WriteOnly);
	
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
	if(m_pMedIOData->write(buffer) != -1)
	{
		m_pDirItem->subHeaderWritten(*this);
		result = true;
	}

	RETURN(result);
	return result;
}

int CECAT7SubHeaderNorm3D::rawDataSize() const
{
	return 1*ECAT_BLOCKSIZE;
}

CECATSubHeader::Type CECAT7SubHeaderNorm3D::subHeaderType(void) const
{
	return CECATSubHeader::ECAT7_Norm3D;
}

bool CECAT7SubHeaderNorm3D::convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader*) 
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
				case CECATSubHeader::ECAT7_Norm3D:
				{
					memcpy(&(this->m_Data), &(static_cast<const CECAT7SubHeaderNorm3D*>(pHead1)->m_Data), sizeof(struct ECAT7SubHeader_Norm3D));
					bResult = true;
				}
				break;

				case CECATSubHeader::Unknown:
					// for an unknown header type we do nothing
				break;
				
				#warning "non Norm3D copy not complete"
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

CMedIOHeader* CECAT7SubHeaderNorm3D::clone() const
{
	ENTER();

	CECAT7SubHeaderNorm3D* pNewHead = new CECAT7SubHeaderNorm3D();
	pNewHead->convertFrom(this);

	RETURN(pNewHead);
	return pNewHead;
}

// access methods to get directly data out of
// the SubHeader
CECATSubHeader::Data_Type CECAT7SubHeaderNorm3D::data_Type(void) const
{
	return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type);	}

short CECAT7SubHeaderNorm3D::num_R_Elements(void) const
{
	return m_Data.Num_R_Elements;
}

short CECAT7SubHeaderNorm3D::num_Transaxial_Crystals(void) const
{
	return m_Data.Num_Transaxial_Crystals;
}

short CECAT7SubHeaderNorm3D::num_Crystal_Rings(void) const
{
	return m_Data.Num_Crystal_Rings;
}

short CECAT7SubHeaderNorm3D::crystals_Per_Ring(void) const
{
	return m_Data.Crystals_Per_Ring;
}

short CECAT7SubHeaderNorm3D::num_Geo_Corr_Planes(void) const
{
	return m_Data.Num_Geo_Corr_Planes;
}

short CECAT7SubHeaderNorm3D::uld(void) const
{
	return m_Data.ULD;
}

short CECAT7SubHeaderNorm3D::lld(void) const
{
	return m_Data.LLD;
}

short CECAT7SubHeaderNorm3D::scatter_Energy(void) const
{
	return m_Data.Scatter_Energy;
}

float CECAT7SubHeaderNorm3D::norm_Quality_Factor(void) const
{
	return m_Data.Norm_Quality_Factor;
}

CECAT7SubHeaderNorm3D::Norm_Qual_Factor_Code CECAT7SubHeaderNorm3D::norm_Quality_Factor_Code(void) const
{
	return static_cast<CECAT7SubHeaderNorm3D::Norm_Qual_Factor_Code>(m_Data.Norm_Quality_Factor_Code);
}

float CECAT7SubHeaderNorm3D::ring_DTCor1(const short i) const
{
	return m_Data.Ring_DTCor1[i];
}

float CECAT7SubHeaderNorm3D::ring_DTCor2(const short i) const
{
	return m_Data.Ring_DTCor2[i];
}

float CECAT7SubHeaderNorm3D::crystal_DTCor(const short i) const
{
	return m_Data.Crystal_DTCor[i];
}

short CECAT7SubHeaderNorm3D::span(void) const
{
	return m_Data.Span;
}

short CECAT7SubHeaderNorm3D::max_Ring_Diff(void) const
{
	return m_Data.Max_Ring_Diff;
}

short CECAT7SubHeaderNorm3D::cti_Reserved(const short i) const
{
	return m_Data.CTI_reserved[i];
}

short CECAT7SubHeaderNorm3D::user_Reserved(const short i) const
{
	return m_Data.User_Reserved[i];
}


void CECAT7SubHeaderNorm3D::setData_Type(const CECATSubHeader::Data_Type dType)
{
	m_Data.Data_Type = static_cast<quint16>(dType);
}			

void CECAT7SubHeaderNorm3D::setNum_R_Elements(const short n)
{
	m_Data.Num_R_Elements = n;
}

void CECAT7SubHeaderNorm3D::setNum_Transaxial_Crystals(const short n)
{
	m_Data.Num_Transaxial_Crystals = n;
}

void CECAT7SubHeaderNorm3D::setNum_Crystal_Rings(const short n)
{
	m_Data.Num_Crystal_Rings = n;
}

void CECAT7SubHeaderNorm3D::setCrystals_Per_Ring(const short n)
{
	m_Data.Crystals_Per_Ring = n;
}

void CECAT7SubHeaderNorm3D::setNum_Geo_Corr_Planes(const short n)
{
	m_Data.Num_Geo_Corr_Planes = n;
}

void CECAT7SubHeaderNorm3D::setULD(const short n)
{
	m_Data.ULD = n;
}

void CECAT7SubHeaderNorm3D::setLLD(const short n)
{
	m_Data.LLD = n;
}

void CECAT7SubHeaderNorm3D::setScatter_Energy(const short n)
{
	m_Data.Scatter_Energy = n;
}

void CECAT7SubHeaderNorm3D::setNorm_Quality_Factor(const float n)
{
	m_Data.Norm_Quality_Factor = n;
}

void CECAT7SubHeaderNorm3D::setNorm_Quality_Factor_Code(const Norm_Qual_Factor_Code n)
{
	m_Data.Norm_Quality_Factor_Code = static_cast<quint16>(n);
}

void CECAT7SubHeaderNorm3D::setRing_DTCor1(const short i, const float n)
{
	m_Data.Ring_DTCor1[i] = n;
}

void CECAT7SubHeaderNorm3D::setRing_DTCor2(const short i, const float n)
{
	m_Data.Ring_DTCor2[i] = n;
}

void CECAT7SubHeaderNorm3D::setCrystal_DTCor(const short i, const float n)
{
	m_Data.Crystal_DTCor[i] = n;
}

void CECAT7SubHeaderNorm3D::setSpan(const short n)
{
	m_Data.Span = n;
}

void CECAT7SubHeaderNorm3D::setMax_Ring_Diff(const short n)
{
	m_Data.Max_Ring_Diff = n;
}

void CECAT7SubHeaderNorm3D::setCTI_Reserved(const short i, const short n)
{
	m_Data.CTI_reserved[i] = n;
}

void CECAT7SubHeaderNorm3D::setUser_Reserved(const short i, const short n)
{
	m_Data.User_Reserved[i] = n;
}

