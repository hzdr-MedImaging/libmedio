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

#ifndef CECAT7SUBHEADERNORM_H
#define CECAT7SUBHEADERNORM_H

#include <qdatastream.h>

#include "CECATSubHeader.h"

// forward declarations

class Q_EXPORT CECAT7SubHeaderNorm : public CECATSubHeader
{
	public:
		// public enumeration types
		enum Norm_Qual_Factor_Code	{	TBD };

		// constructors
		CECAT7SubHeaderNorm();
		
		// public methods													
		bool load(QDataStream& stream);
		bool save(QDataStream& stream);
		CECATSubHeader::Type rtti(void)
		{ return CECATSubHeader::ECAT7_Norm; }

		// data access methods
		CECATSubHeader::Data_Type data_Type(void)
		{ return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type);	}
		
		void setData_Type(CECATSubHeader::Data_Type dType)
		{ m_Data.Data_Type = static_cast<Q_UINT16>(dType);	}	

	private:
		#pragma pack(2)	// set the structure alignment
		struct ECAT7SubHeader_Norm
		{
			Q_UINT16	Data_Type;
			Q_UINT16	Num_Dimensions;
			Q_UINT16	Num_R_Elements;
			Q_UINT16	Num_Angles;
			Q_UINT16	Num_Z_Elements;
			Q_UINT16	Ring_Difference;
			float			Scale_Factor;
			float			Norm_Min;
			float			Norm_Max;
			float			FOV_Source_Width;
			float			Norm_Quality_Factor;
			Q_UINT16	Norm_Quality_Factor_Code;
			Q_UINT16	Storage_Order;
			Q_UINT16	Span;
			Q_UINT16	Z_Elements[64];
			Q_UINT16	CTI_reserved[123];
			Q_UINT16	User_Reserved[50];
		} m_Data;
		#pragma pack()
};

#endif // CECAT7SUBHEADERNORM_H
