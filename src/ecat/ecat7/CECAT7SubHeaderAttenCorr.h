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

#ifndef CECAT7SUBHEADERATTENCORR_H
#define CECAT7SUBHEADERATTENCORR_H

#include <qdatastream.h>

#include "CECATSubHeader.h"

class CECAT7SubHeaderAttenCorr : public CECATSubHeader
{
	public:
		// public available enumeration types
		enum Attenuation_Type { Atten_None=0,
														Atten_Meas,
														Atten_Calc };

		// constructor
		CECAT7SubHeaderAttenCorr()
		{
			// check that the headsize is 512 bytes long
			ASSERT(sizeof(struct ECAT7SubHeader_AttenCorr) == 512);

			// then clear the structure
			memset(&m_Data, 0, sizeof(struct ECAT7SubHeader_AttenCorr));
		}

		// public methods
		bool load(QDataStream& stream);
		bool save(QDataStream& stream);
		CECATSubHeader::Type rtti(void) { return CECATSubHeader::ECAT7_AttenCorr; }

		// data access methods
		CECATSubHeader::Data_Type data_Type(void)	{ return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type);	}
		void setData_Type(CECATSubHeader::Data_Type dType)	{ m_Data.Data_Type = static_cast<Q_UINT16>(dType);	}

	private:
		#pragma pack(2)	// set the structure alignment
		struct ECAT7SubHeader_AttenCorr
		{
			Q_UINT16	Data_Type;
			Q_UINT16	Num_Dimensions;
			Q_UINT16	Attenuation_Type;
			Q_UINT16	Num_R_Elements;
			Q_UINT16	Num_Angles;
			Q_UINT16	Num_Z_Elements;
			Q_UINT16	Ring_Difference;
			float			X_Resolution;
			float			Y_Resolution;
			float			Z_Resolution;
			float			W_Resolution;
			float			Scale_Factor;
			float			X_Offset;
			float			Y_Offset;
			float			X_Radius;
			float			Y_Radius;
			float			Tilt_Angle;
			float			Attenuation_Coeff;
			float			Attenuation_Min;
			float			Attenuation_Max;
			float			Skull_Thickness;
			Q_UINT16	Num_Additional_Atten_Coeff;
			float			Additional_Atten_Coeff[8];
			float			Edge_Finding_Threshold;
			Q_UINT16	Storage_Order;
			Q_UINT16	Span;
			Q_UINT16	Z_Elements[64];
			Q_UINT16	Unused[86];
			Q_UINT16	CTI_Reserved[50];
		} m_Data;
		#pragma pack()
};

#endif // CECAT7SUBHEADERATTENCORR_H
