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

#ifndef CECAT7SUBHEADERSCAN3D_H
#define CECAT7SUBHEADERSCAN3D_H

#include <qdatastream.h>

#include "CECATSubHeader.h"

// forward declarations

class Q_EXPORT CECAT7SubHeaderScan3D : public CECATSubHeader
{
	public:
		// public enumeration types
		enum Corrections_Applied	{	Norm		= (0<<0),
																Atten		=	(1<<0),
																Smooth	=	(1<<1)
															};

		enum Storage_Order { ViewMode=0,	// ViewMode		(r0zd)
												 VolumeMode		// VolumeMode	(rz0d)
											 };

		// constructors
		CECAT7SubHeaderScan3D();
			
		// public methods
		bool load(QDataStream& stream);
		bool save(QDataStream& stream);
		CECATSubHeader::Type rtti(void)	
		{ return CECATSubHeader::ECAT7_Scan3D;			}

		// methods to access elements of the SubHeader
		CECATSubHeader::Data_Type data_Type(void)	
		{ return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type);	}
		
		short num_Dimensions()						
		{ return m_Data.Num_Dimensions;	}
		
		short num_R_Elements()						
		{ return m_Data.Num_R_Elements; }
		
		short num_Angles()								
		{	return m_Data.Num_Angles; }
		
		short corrections_Applied()				
		{ return m_Data.Corrections_Applied; }
		
		short num_Z_Elements(short i)		
		{ return m_Data.Num_Z_Elements[i]; }
		
		short ring_Difference()					
		{	return m_Data.Ring_Difference; }
		
		Storage_Order storage_Order()			
		{ return static_cast<Storage_Order>(m_Data.Storage_Order); }
		
		short axial_Compression()				
		{	return m_Data.Axial_Compression; }
		
		float X_Resolution()						
		{	return m_Data.X_Resolution;	}
		
		float V_Resolution()						
		{	return m_Data.V_Resolution;	}
		
		float Z_Resolution()						
		{	return m_Data.Z_Resolution;	}
		
		float W_Resolution()							
		{	return m_Data.W_Resolution;	}
		
		short gate_Reserved(short i)		
		{ return m_Data.Gate_Reserved[i]; }
		
		unsigned int gate_Duration()		
		{ return m_Data.Gate_Duration; }
		
		unsigned int r_Wave_Offset()		
		{ return m_Data.R_Wave_Offset; }
		
		unsigned int num_Accepted_Beats()	
		{	return m_Data.Num_Accepted_Beats; }
		
		float scale_Factor()						
		{ return m_Data.Scale_Factor; }
		
		short scan_Min()								
		{ return m_Data.Scan_Min; }
		
		short scan_Max()									
		{ return m_Data.Scan_Max; }
		
		unsigned int prompts()						
		{ return m_Data.Prompts; }
		
		unsigned int delayed()					
		{ return m_Data.Delayed; }
		
		unsigned int multiples()				
		{ return m_Data.Multiples; }
		
		unsigned int net_Trues()				
		{ return m_Data.Net_Trues; }
		
		float tot_Avg_Cor()							
		{ return m_Data.Tot_Avg_Cor; }
		
		float tot_Avg_Uncor()						
		{ return m_Data.Tot_Avg_Uncor; }
		
		unsigned int total_Coin_Rate()	
		{ return m_Data.Total_Coin_Rate; }
		
		unsigned int frame_Start_Time()	
		{ return m_Data.Frame_Start_Time; }
		
		unsigned int frame_Duration()		
		{ return m_Data.Frame_Duration; }
		
		float deadtime_Correction_Factor()
		{ return m_Data.Deadtime_Correction_Factor;	}
		
		short cti_Reserved(short i)				
		{ return m_Data.CTI_reserved[i]; }
		
		short user_Reserved(short i)		
		{ return m_Data.User_Reserved[i]; }
		
		float uncor_Singles(short bucket)
		{	return m_Data.Uncor_Singles[bucket]; }


		// methods to modify elements of the SubHeader
		void setData_Type(CECATSubHeader::Data_Type dType)	
		{ m_Data.Data_Type = static_cast<Q_UINT16>(dType); }	
		
		void setNum_Dimensions(short dim)						
		{ m_Data.Num_Dimensions = dim; }
		
		void setNum_R_Elements(short elem)						
		{ m_Data.Num_R_Elements = elem; }
		
		void setNum_Angles(short angles)							
		{ m_Data.Num_Angles = angles; }
		
		void setCorrections_Applied(short cor)			
		{ m_Data.Corrections_Applied = cor; }
		
		void setNum_Z_Elements(short i, short num)	
		{	m_Data.Num_Z_Elements[i] = num; }
		
		void setRing_Difference(short diff)					
		{	m_Data.Ring_Difference = diff; }
		
		void setStorage_Order(Storage_Order order)	
		{ m_Data.Storage_Order = static_cast<Q_UINT16>(order); }
		
		void setAxial_Compression(short comp)			
		{	m_Data.Axial_Compression = comp; }
		
		void setX_Resolution(float res)						
		{ m_Data.X_Resolution = res; }
		
		void setV_Resolution(float res)						
		{ m_Data.V_Resolution = res; }
		
		void setZ_Resolution(float res)						
		{ m_Data.Z_Resolution = res; }
		
		void setW_Resolution(float res)						
		{ m_Data.W_Resolution = res; }
		
		void setGate_Reserved(short i, short val)	
		{ m_Data.Gate_Reserved[i] = val; }
		
		void setGate_Duration(unsigned int dur)		
		{	m_Data.Gate_Duration = dur;	}
		
		void setR_Wave_Offset(unsigned int off)		
		{ m_Data.R_Wave_Offset = off;	}
		
		void setNum_Accepted_Beats(unsigned int b)		
		{ m_Data.Num_Accepted_Beats = b; }
		
		void setScale_Factor(float factor)					
		{ m_Data.Scale_Factor = factor; }
		
		void setScan_Min(short min)								
		{ m_Data.Scan_Min = min; }
		
		void setScan_Max(short max)								
		{ m_Data.Scan_Max = max; }
		
		void setPrompts(unsigned int n)						
		{ m_Data.Prompts = n; }
		
		void setDelayed(unsigned int n)						
		{ m_Data.Delayed = n;	}
		
		void setMultiples(unsigned int n)					
		{ m_Data.Multiples = n;	}
		
		void setNet_Trues(unsigned int n)					
		{ m_Data.Net_Trues = n; }
		
		void setTot_Avg_Cor(float value)						
		{ m_Data.Tot_Avg_Cor = value; }
		
		void setTot_Avg_Uncor(float value)					
		{ m_Data.Tot_Avg_Uncor = value; }
		
		void setTotal_Coin_Rate(unsigned int n)		
		{ m_Data.Total_Coin_Rate = n; }
		
		void setFrame_Start_Time(unsigned int t)		
		{ m_Data.Frame_Start_Time = t; }
		
		void setFrame_Duration(unsigned int t)			
		{ m_Data.Frame_Duration = t; }
		
		void setDeadtime_Correction_Factor(float f)
		{ m_Data.Deadtime_Correction_Factor = f; }
		
		void setCTI_Reserved(short i, short value)		
		{ m_Data.CTI_reserved[i] = value; }
		
		void setUser_Reserved(short i, short value)
		{ m_Data.User_Reserved[i] = value; }
		
		void setUncor_Singles(short b, float value)
		{ m_Data.Uncor_Singles[b] = value; }

	private:
		#pragma pack(2)	// set the structure alignment
		struct ECAT7SubHeader_Scan3D
		{
			Q_UINT16	Data_Type;
			Q_UINT16	Num_Dimensions;
			Q_UINT16	Num_R_Elements;
			Q_UINT16	Num_Angles;
			Q_UINT16	Corrections_Applied;
			Q_UINT16	Num_Z_Elements[64];
			Q_UINT16	Ring_Difference;
			Q_UINT16	Storage_Order;
			Q_UINT16	Axial_Compression;
			float			X_Resolution;
			float			V_Resolution;
			float			Z_Resolution;
			float			W_Resolution;
			Q_UINT16	Gate_Reserved[6];
			Q_UINT32	Gate_Duration;
			Q_UINT32	R_Wave_Offset;
			Q_UINT32	Num_Accepted_Beats;
			float			Scale_Factor;
			Q_INT16		Scan_Min;
			Q_INT16		Scan_Max;
			Q_UINT32	Prompts;
			Q_UINT32	Delayed;
			Q_UINT32	Multiples;
			Q_UINT32	Net_Trues;
			float			Tot_Avg_Cor;
			float			Tot_Avg_Uncor;
			Q_UINT32	Total_Coin_Rate;
			Q_UINT32	Frame_Start_Time;
			Q_UINT32	Frame_Duration;
			float			Deadtime_Correction_Factor;
			Q_UINT16	CTI_reserved[90];
			Q_UINT16	User_Reserved[50];
			float			Uncor_Singles[128];
		} m_Data;
		#pragma pack()
};

#endif // CECAT7SUBHEADERNORM3D_H
