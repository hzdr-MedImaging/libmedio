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

#ifndef CECAT7SUBHEADERSCAN_H
#define CECAT7SUBHEADERSCAN_H

#include <qdatastream.h>

#include "CECATSubHeader.h"

// forward declarations

class Q_EXPORT CECAT7SubHeaderScan : public CECATSubHeader
{
	public:
		// public enumeration types
		enum Corrections_Applied	{	Norm		= (0<<0),
																Atten		=	(1<<0),
																Smooth	=	(1<<1) };

		// constructors
		CECAT7SubHeaderScan();
		
		// public methods
		bool load(QDataStream& stream);
		bool save(QDataStream& stream);
		CECATSubHeader::Type rtti(void)
		{ return CECATSubHeader::ECAT7_Scan; }

		// data access methods
		CECATSubHeader::Data_Type data_Type(void)
		{ return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type);	}
		
		void setData_Type(CECATSubHeader::Data_Type dType)
		{ m_Data.Data_Type = static_cast<Q_UINT16>(dType);	}	
		

	private:
		#pragma pack(2)	// set the structure alignment
		struct ECAT7SubHeader_Scan
		{
			Q_UINT16	Data_Type;
			Q_UINT16	Num_Dimensions;
			Q_UINT16	Num_R_Elements;
			Q_UINT16	Num_Angles;
			Q_UINT16	Corrections_Applied;
			Q_UINT16	Num_Z_Elements;
			Q_UINT16	Ring_Difference;
			float			X_Resolution;
			float			Y_Resolution;
			float			Z_Resolution;
			float			W_Resolution;
			Q_UINT16	Gate_Reserved[6];
			Q_UINT32	Gate_Duration;
			Q_UINT32	R_Wave_Offset;
			Q_UINT32	Num_Accepted_Beats;
			float			Scale_Factor;
			Q_UINT16	Scan_Min;
			Q_UINT16	Scan_Max;
			Q_UINT32	Prompts;
			Q_UINT32	Delayed;
			Q_UINT32	Multiples;
			Q_UINT32	Net_Trues;
			float			Cor_Singles[16];
			float			Uncor_Singles[16];
			float			Tot_Avg_Cor;
			float			Tot_Avg_Uncor;
			Q_UINT32	Total_Coin_Rate;
			Q_UINT32	Frame_Start_Time;
			Q_UINT32	Frame_Duration;
			float			Deadtime_Correction_Factor;
			Q_UINT16	Physical_Planes[8];
			Q_UINT16	CTI_reserved[83];
			Q_UINT16	User_Reserved[50];
		} m_Data;
		#pragma pack()
};

#endif // CECAT7SUBHEADERSCAN_H
