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

#ifndef CECAT7SUBHEADERSCAN_H
#define CECAT7SUBHEADERSCAN_H

#include <qdatastream.h>

#include <CECATSubHeader.h>

// forward declarations
class CECATFile;

class Q_EXPORT CECAT7SubHeaderScan : public CECATSubHeader
{
	public:
		// public bitmask enumeration types
		enum Corrections_Applied	{	Norm		= (0<<0),
																Atten		=	(1<<0),
																Smooth	=	(1<<1) };

		// constructors
		CECAT7SubHeaderScan(CECATFile* ecatFile,
												CECATDirectoryItem* pDirItem = NULL);
		CECAT7SubHeaderScan(const CECAT7SubHeaderScan& sh);
		
		// public methods
		bool load(void);
		bool save(void) const;

		CECATSubHeader::Type subHeaderType(void) const
		{ return CECATSubHeader::ECAT7_Scan; }
		
		int size() const 
		{	return ECAT7_HEADERSIZE_SCAN; }

		// some required operator overloading methods to copy a header
		CMedIOHeader& operator=(const CMedIOHeader& src);			
		
		// data access methods
		CECATSubHeader::Data_Type data_Type(void) const
		{ return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type);	}

		short num_Dimensions(void) const
		{ return m_Data.Num_Dimensions; }

		short num_R_Elements(void) const
		{ return m_Data.Num_R_Elements; }

		short num_Angles(void) const
		{ return m_Data.Num_Angles; }

		short corrections_Applied(void) const
		{ return m_Data.Corrections_Applied; }

		short num_Z_Elements(void) const
		{ return m_Data.Num_Z_Elements; }

		short ring_Difference(void) const
		{ return m_Data.Ring_Difference; }

		float x_Resolution(void) const
		{ return m_Data.X_Resolution; }

		float y_Resolution(void) const
		{ return m_Data.Y_Resolution; }

		float z_Resolution(void) const
		{ return m_Data.Z_Resolution; }

		float w_Resolution(void) const
		{ return m_Data.W_Resolution; }

		short gate_Reserved(const short i) const
		{ return m_Data.Gate_Reserved[i]; }

		unsigned int gate_Duration(void) const
		{ return m_Data.Gate_Duration; }

		unsigned int r_Wave_Offset(void) const
		{ return m_Data.R_Wave_Offset; }

		unsigned int num_Accepted_Beats(void) const
		{ return m_Data.Num_Accepted_Beats; }

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

		float cor_Singles(const short i) const
		{ return m_Data.Cor_Singles[i]; }

		float uncor_Singles(const short i) const
		{ return m_Data.Uncor_Singles[i]; }

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
		{ return m_Data.Deadtime_Correction_Factor; }

		short physical_Planes(const short i) const
		{ return m_Data.Physical_Planes[i]; }

		short cti_Reserved(const short i) const
		{ return m_Data.CTI_reserved[i]; }

		short user_Reserved(const short i) const
		{ return m_Data.User_Reserved[i]; }
		

		void setData_Type(const CECATSubHeader::Data_Type dType)
		{ m_Data.Data_Type = static_cast<Q_UINT16>(dType);	}	

		void setNum_Dimensions(const short n)
		{ m_Data.Num_Dimensions = n; }

		void setNum_R_Elements(const short n)
		{ m_Data.Num_R_Elements = n; }

		void setNum_Angles(const short n)
		{ m_Data.Num_Angles = n; }

		void setCorrections_Applied(const short n)
		{ m_Data.Corrections_Applied = n; }

		void setNum_Z_Elements(const short n)
		{ m_Data.Num_Z_Elements = n; }

		void setRing_Difference(const short n)
		{ m_Data.Ring_Difference = n; }

		void setX_Resolution(const float n)
		{ m_Data.X_Resolution = n; }

		void setY_Resolution(const float n)
		{ m_Data.Y_Resolution = n; }

		void setZ_Resolution(const float n)
		{ m_Data.Z_Resolution = n; }

		void setW_Resolution(const float n)
		{ m_Data.W_Resolution = n; }

		void setGate_Reserved(const short i, const short n)
		{ m_Data.Gate_Reserved[i] = n; }

		void setGate_Duration(const unsigned int n)
		{ m_Data.Gate_Duration = n; }

		void setR_Wave_Offset(const unsigned int n)
		{ m_Data.R_Wave_Offset = n; }

		void setNum_Accepted_Beats(const unsigned int n)
		{ m_Data.Num_Accepted_Beats = n; }

		void setScale_Factor(const float n)
		{ m_Data.Scale_Factor = n; }

		void setScan_Min(const short n)
		{ m_Data.Scan_Min = n; }

		void setScan_Max(const short n)
		{ m_Data.Scan_Max = n; }

		void setPrompts(const unsigned int n)
		{ m_Data.Prompts = n; }

		void setDelayed(const unsigned int n)
		{ m_Data.Delayed = n; }

		void setMultiples(const unsigned int n)
		{ m_Data.Multiples = n; }

		void setNet_Trues(const unsigned int n)
		{ m_Data.Net_Trues = n; }

		void setCor_Singles(const short i, const float n)
		{ m_Data.Cor_Singles[i] = n; }

		void setUncor_Singles(const short i, const float n)
		{ m_Data.Uncor_Singles[i] = n; }

		void setTot_Avg_Cor(const float n)
		{ m_Data.Tot_Avg_Cor = n; }

		void setTot_Avg_Uncor(const float n)
		{ m_Data.Tot_Avg_Uncor = n; }

		void setTotal_Coin_Rate(const unsigned int n)
		{ m_Data.Total_Coin_Rate = n; }

		void setFrame_Start_Time(const unsigned int n)
		{ m_Data.Frame_Start_Time = n; }

		void setFrame_Duration(const unsigned int n)
		{ m_Data.Frame_Duration = n; }

		void setDeadtime_Correction_Factor(const float n)
		{ m_Data.Deadtime_Correction_Factor = n; }

		void setPhysical_Planes(const short i, const short n)
		{ m_Data.Physical_Planes[i] = n; }

		void setCTI_Reserved(const short i, const short n)
		{ m_Data.CTI_reserved[i] = n; }

		void setUser_Reserved(const short i, const short n)
		{ m_Data.User_Reserved[i] = n; }

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
