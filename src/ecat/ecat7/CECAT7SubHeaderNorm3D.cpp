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
#include "CECATFile.h"

#include <qdatastream.h>

#include "debug.h"

CECAT7SubHeaderNorm3D::CECAT7SubHeaderNorm3D()
{
	// check that the headsize is 512 bytes long
	ASSERT(sizeof(struct ECAT7SubHeader_Norm3D) == 512);

	// then clear the structure
	memset(&m_Data, 0, sizeof(struct ECAT7SubHeader_Norm3D));
}

bool CECAT7SubHeaderNorm3D::load(QDataStream& stream)
{
	// check if the stream is readable or not.
	if(stream.device()->isReadable() == false)
		return false;

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

	return true;
}

bool CECAT7SubHeaderNorm3D::save(QDataStream& stream)
{
	// check if this stream is writeable or not
	if(stream.device()->isWritable() == false)
		return false;
	
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
	
	return true;
}
