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

#ifndef CECAT7SUBHEADERPOLARMAP_H
#define CECAT7SUBHEADERPOLARMAP_H

#include <qdatastream.h>

#include "CECATSubHeader.h"

// forward declarations

class CECAT7SubHeaderPolarMap : public CECATSubHeader
{
	public:
		// public BitMask field
		enum Processing_Code	{	MapType							= (0<<0),
														ThresholdApplied		=	(1<<0),
														SummedMap						=	(1<<1),
														SubstractedMap			=	(1<<2),
														ProductOfTwoMaps		=	(1<<3),
														RatioOfTwoMaps			=	(1<<4),
														Bias								=	(1<<5),
														Multiplier					=	(1<<6),
														Transform						=	(1<<7),
														PMapCalcProtoUsed		=	(1<<8) };

		// constructor
		CECAT7SubHeaderPolarMap()
		{
			// check that the headsize is 512 bytes long
			ASSERT(sizeof(struct ECAT7SubHeader_PolarMap) == 512);

			// then clear the structure
			memset(&m_Data, 0, sizeof(struct ECAT7SubHeader_PolarMap));			
		}
			
		// public methods
		bool load(QDataStream& stream);
		bool save(QDataStream& stream);
		CECATSubHeader::Type rtti(void) { return CECATSubHeader::ECAT7_PolarMap; }

		// data access methods
		CECATSubHeader::Data_Type data_Type(void)	{ return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type);	}
		void setData_Type(CECATSubHeader::Data_Type dType)	{ m_Data.Data_Type = static_cast<Q_UINT16>(dType);	}		

	private:
		#pragma pack(2)	// set the structure alignment
		struct ECAT7SubHeader_PolarMap
		{
			Q_UINT16	Data_Type;
			Q_UINT16	Polar_Map_Type;
			Q_UINT16	Num_Rings;
			Q_UINT16	Sectors_Per_Ring[32];
			float			Ring_Position[32];
			Q_UINT16	Ring_Angle[32];
			Q_UINT16	Start_Angle;
			Q_UINT16	Long_Axis_Left[3];
			Q_UINT16	Long_Axis_Right[3];
			Q_UINT16	Position_Data;
			Q_UINT16	Image_Min;
			Q_UINT16	Image_Max;
			float			Scale_Factor;
			float			Pixel_Size;
			Q_UINT16	Frame_Duration;
			Q_UINT16	Frame_Start_Time;
			Q_UINT16	Processing_Code;
			Q_UINT16	Quant_Units;
			char			Annotation[40];
			Q_UINT16	Gate_Duration;
			Q_UINT16	R_Wave_Offset;
			Q_UINT16	Num_Accepted_Beats;
			char			Polar_Map_Protocol[20];
			char			Database_Name[30];
			Q_UINT16	CTI_reserved[27];
			Q_UINT16	User_Reserved[27];
		} m_Data;
		#pragma pack()
};

#endif // CECAT7SUBHEADERPOLARMAP_H
