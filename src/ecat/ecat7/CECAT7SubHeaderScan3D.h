/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004-2005 by Jens Langner <Jens.Langner@light-speed.de>

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

#include <CECATSubHeader.h>
#include <CECATDirectoryItem.h>

// forward declarations
class CECATFile;

class Q_EXPORT CECAT7SubHeaderScan3D : public CECATSubHeader
{
	public:
		// public enumeration types
		enum Storage_Order { ViewMode=0,	// ViewMode		(r0zd)
												 VolumeMode		// VolumeMode	(rz0d)
											 };

		// Bitmask enumeration
		enum Corrections_Applied	{	Norm		= (0<<0),
																Atten		=	(1<<0),
																Smooth	=	(1<<1)
															};

		// constructors
		CECAT7SubHeaderScan3D(CECATFile* ecatFile,
													CECATDirectoryItem* pDirItem = NULL);
		CECAT7SubHeaderScan3D(const CECAT7SubHeaderScan3D& sh);
			
		// public methods
		bool load(void);
		bool save(void) const;
		
		// the number of bytes the data of that header requires on disk
		int rawDataSize() const { return 2*ECAT_BLOCKSIZE; }
		
		CECATSubHeader::Type subHeaderType(void) const
		{ return CECATSubHeader::ECAT7_Scan3D; }
		
		// methods to access elements of the SubHeader
		CECATSubHeader::Data_Type data_Type(void)	const
		{ return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type);	}
		
		short num_Dimensions(void) const						
		{ return m_Data.Num_Dimensions;	}
		
		short num_R_Elements(void) const						
		{ return m_Data.Num_R_Elements; }
		
		short num_Angles(void) const								
		{	return m_Data.Num_Angles; }
		
		short corrections_Applied(void) const				
		{ return m_Data.Corrections_Applied; }
		
		short num_Z_Elements(const short i) const		
		{ return m_Data.Num_Z_Elements[i]; }
		
		short ring_Difference(void) const					
		{	return m_Data.Ring_Difference; }
		
		Storage_Order storage_Order(void) const			
		{ return static_cast<Storage_Order>(m_Data.Storage_Order); }
		
		short axial_Compression(void) const				
		{	return m_Data.Axial_Compression; }
		
		float X_Resolution(void) const						
		{	return m_Data.X_Resolution;	}
		
		float V_Resolution(void) const						
		{	return m_Data.V_Resolution;	}
		
		float Z_Resolution(void) const						
		{	return m_Data.Z_Resolution;	}
		
		float W_Resolution(void) const							
		{	return m_Data.W_Resolution;	}
		
		short gate_Reserved(const short i) const		
		{ return m_Data.Gate_Reserved[i]; }
		
		unsigned int gate_Duration(void) const		
		{ return m_Data.Gate_Duration; }
		
		unsigned int r_Wave_Offset(void) const		
		{ return m_Data.R_Wave_Offset; }
		
		unsigned int num_Accepted_Beats(void) const	
		{	return m_Data.Num_Accepted_Beats; }
		
		float scale_Factor(void) const						
		{ return m_Data.Scale_Factor; }
		
		short scan_Min(void) const								
		{ return m_Data.Scan_Min; }
		
		short scan_Max(void) const									
		{ return m_Data.Scan_Max; }
		
		unsigned int prompts(void) const						
		{ return m_Data.Prompts; }
		
		unsigned int delayed(void) const					
		{ return m_Data.Delayed; }
		
		unsigned int multiples(void) const				
		{ return m_Data.Multiples; }
		
		unsigned int net_Trues(void) const				
		{ return m_Data.Net_Trues; }
		
		float tot_Avg_Cor(void) const 							
		{ return m_Data.Tot_Avg_Cor; }
		
		float tot_Avg_Uncor(void) const						
		{ return m_Data.Tot_Avg_Uncor; }
		
		unsigned int total_Coin_Rate(void) const	
		{ return m_Data.Total_Coin_Rate; }
		
		unsigned int frame_Start_Time(void) const	
		{ return m_Data.Frame_Start_Time; }
		
		unsigned int frame_Duration(void) const		
		{ return m_Data.Frame_Duration; }
		
		float deadtime_Correction_Factor(void) const
		{ return m_Data.Deadtime_Correction_Factor;	}
		
		short cti_Reserved(const short i) const				
		{ return m_Data.CTI_reserved[i]; }
		
		short user_Reserved(const short i) const		
		{ return m_Data.User_Reserved[i]; }
		
		float uncor_Singles(const short bucket) const
		{	return m_Data.Uncor_Singles[bucket]; }


		// methods to modify elements of the SubHeader
		void setData_Type(const CECATSubHeader::Data_Type dType)	
		{ m_Data.Data_Type = static_cast<Q_UINT16>(dType); }	
		
		void setNum_Dimensions(const short dim)						
		{ m_Data.Num_Dimensions = dim; }
		
		void setNum_R_Elements(const short elem)						
		{ m_Data.Num_R_Elements = elem; }
		
		void setNum_Angles(const short angles)							
		{ m_Data.Num_Angles = angles; }
		
		void setCorrections_Applied(const short cor)			
		{ m_Data.Corrections_Applied = cor; }
		
		void setNum_Z_Elements(const short i, const short num)	
		{	m_Data.Num_Z_Elements[i] = num; }
		
		void setRing_Difference(const short diff)					
		{	m_Data.Ring_Difference = diff; }
		
		void setStorage_Order(const Storage_Order order)	
		{ m_Data.Storage_Order = static_cast<Q_UINT16>(order); }
		
		void setAxial_Compression(const short comp)			
		{	m_Data.Axial_Compression = comp; }
		
		void setX_Resolution(const float res)						
		{ m_Data.X_Resolution = res; }
		
		void setV_Resolution(const float res)						
		{ m_Data.V_Resolution = res; }
		
		void setZ_Resolution(const float res)						
		{ m_Data.Z_Resolution = res; }
		
		void setW_Resolution(const float res)						
		{ m_Data.W_Resolution = res; }
		
		void setGate_Reserved(const short i, const short val)	
		{ m_Data.Gate_Reserved[i] = val; }
		
		void setGate_Duration(const unsigned int dur)		
		{	m_Data.Gate_Duration = dur;	}
		
		void setR_Wave_Offset(const unsigned int off)		
		{ m_Data.R_Wave_Offset = off;	}
		
		void setNum_Accepted_Beats(const unsigned int b)		
		{ m_Data.Num_Accepted_Beats = b; }
		
		void setScale_Factor(const float factor)					
		{ m_Data.Scale_Factor = factor; }
		
		void setScan_Min(const short min)								
		{ m_Data.Scan_Min = min; }
		
		void setScan_Max(const short max)								
		{ m_Data.Scan_Max = max; }
		
		void setPrompts(const unsigned int n)						
		{ m_Data.Prompts = n; }
		
		void setDelayed(const unsigned int n)						
		{ m_Data.Delayed = n;	}
		
		void setMultiples(const unsigned int n)					
		{ m_Data.Multiples = n;	}
		
		void setNet_Trues(const unsigned int n)					
		{ m_Data.Net_Trues = n; }
		
		void setTot_Avg_Cor(const float value)						
		{ m_Data.Tot_Avg_Cor = value; }
		
		void setTot_Avg_Uncor(const float value)					
		{ m_Data.Tot_Avg_Uncor = value; }
		
		void setTotal_Coin_Rate(const unsigned int n)		
		{ m_Data.Total_Coin_Rate = n; }
		
		void setFrame_Start_Time(const unsigned int t)		
		{ m_Data.Frame_Start_Time = t; }
		
		void setFrame_Duration(const unsigned int t)			
		{ m_Data.Frame_Duration = t; }
		
		void setDeadtime_Correction_Factor(const float f)
		{ m_Data.Deadtime_Correction_Factor = f; }
		
		void setCTI_Reserved(const short i, const short value)		
		{ m_Data.CTI_reserved[i] = value; }
		
		void setUser_Reserved(const short i, const short value)
		{ m_Data.User_Reserved[i] = value; }
		
		void setUncor_Singles(const short b, const float value)
		{ m_Data.Uncor_Singles[b] = value; }

	protected:
		// required method to copy relevant data from another MedIOHeader object
		CMedIOHeader& copyData(const CMedIOHeader& src);

	private:
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
};

#endif // CECAT7SUBHEADERNORM3D_H
