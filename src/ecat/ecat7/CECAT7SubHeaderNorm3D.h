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

#ifndef CECAT7SUBHEADERNORM3D_H
#define CECAT7SUBHEADERNORM3D_H

#include <qdatastream.h>

#include "CECATSubHeader.h"

#include <cstdlib>

// forward declarations

class Q_EXPORT CECAT7SubHeaderNorm3D : public CECATSubHeader
{
	public:
		// public enumeration types
		enum Norm_Qual_Factor_Code	{	TBD };

		// constructors
		CECAT7SubHeaderNorm3D();

		// public methods
		bool load(QDataStream& stream);
		bool save(QDataStream& stream);
		CECATSubHeader::Type rtti(void) { return CECATSubHeader::ECAT7_Norm3D; }

		// access methods to get directly data out of
		// the SubHeader
		CECATSubHeader::Data_Type data_Type(void)
		{ return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type);	}
		
		void setData_Type(CECATSubHeader::Data_Type dType)
		{ m_Data.Data_Type = static_cast<Q_UINT16>(dType);	}			
		
		float* RING_DTCOR1(void)
		{ float* array = new float[32]; memcpy(array, m_Data.Ring_DTCor1, sizeof(m_Data.Ring_DTCor1));			return array; }
		
		float* RING_DTCOR2(void)
		{ float* array = new float[32]; memcpy(array, m_Data.Ring_DTCor2, sizeof(m_Data.Ring_DTCor2));			return array; }
		
		float* CRYSTAL_DTCOR(void)
		{ float* array = new float[8];	memcpy(array, m_Data.Crystal_DTCor, sizeof(m_Data.Crystal_DTCor));	return array; }

	private:
		#pragma pack(2)	// set the structure alignment
		struct ECAT7SubHeader_Norm3D
		{
			Q_UINT16	Data_Type;
			Q_UINT16	Num_R_Elements;
			Q_UINT16	Num_Transaxial_Crystals;
			Q_UINT16	Num_Crystal_Rings;
			Q_UINT16	Crystals_Per_Ring;
			Q_UINT16	Num_Geo_Corr_Planes;
			Q_UINT16	ULD;
			Q_UINT16	LLD;
			Q_UINT16	Scatter_Energy;
			float			Norm_Quality_Factor;
			Q_UINT16	Norm_Quality_Factor_Code;
			float			Ring_DTCor1[32];
			float			Ring_DTCor2[32];
			float			Crystal_DTCor[8];
			Q_UINT16	Span;
			Q_UINT16	Max_Ring_Diff;
			Q_UINT16	CTI_reserved[48];
			Q_UINT16	User_Reserved[50];
		} m_Data;
		#pragma pack()
};

#endif // CECAT7SUBHEADERNORM3D_H
