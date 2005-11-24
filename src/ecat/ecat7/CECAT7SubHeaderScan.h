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
#include <CECATDirectoryItem.h>

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
		
		// public methods
		bool load(void);
		bool save(void) const;

		// the number of bytes the data of that header requires on disk
		int rawDataSize() const;
		
		CECATSubHeader::Type subHeaderType(void) const;

		// clone methods
		bool convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader* pHead2 = NULL);
		CMedIOHeader* clone() const;
		
		// data access methods
		CECATSubHeader::Data_Type data_Type(void) const;
		short num_Dimensions(void) const;
		short num_R_Elements(void) const;
		short num_Angles(void) const;
		short corrections_Applied(void) const;
		short num_Z_Elements(void) const;
		short ring_Difference(void) const;
		float x_Resolution(void) const;
		float y_Resolution(void) const;
		float z_Resolution(void) const;
		float w_Resolution(void) const;
		short gate_Reserved(const short i) const;
		unsigned int gate_Duration(void) const;
		unsigned int r_Wave_Offset(void) const;
		unsigned int num_Accepted_Beats(void) const;
		float scale_Factor(void) const;
		short scan_Min(void) const;
		short scan_Max(void) const;
		unsigned int prompts(void) const;
		unsigned int delayed(void) const;
		unsigned int multiples(void) const;
		unsigned int net_Trues(void) const;
		float cor_Singles(const short i) const;
		float uncor_Singles(const short i) const;
		float tot_Avg_Cor(void) const;
		float tot_Avg_Uncor(void) const;
		unsigned int total_Coin_Rate(void) const;
		unsigned int frame_Start_Time(void) const;
		unsigned int frame_Duration(void) const;
		float deadtime_Correction_Factor(void) const;
		short physical_Planes(const short i) const;
		short cti_Reserved(const short i) const;
		short user_Reserved(const short i) const;

		// data mutator methods
		void setData_Type(const CECATSubHeader::Data_Type dType);
		void setNum_Dimensions(const short n);
		void setNum_R_Elements(const short n);
		void setNum_Angles(const short n);
		void setCorrections_Applied(const short n);
		void setNum_Z_Elements(const short n);
		void setRing_Difference(const short n);
		void setX_Resolution(const float n);
		void setY_Resolution(const float n);
		void setZ_Resolution(const float n);
		void setW_Resolution(const float n);
		void setGate_Reserved(const short i, const short n);
		void setGate_Duration(const unsigned int n);
		void setR_Wave_Offset(const unsigned int n);
		void setNum_Accepted_Beats(const unsigned int n);
		void setScale_Factor(const float n);
		void setScan_Min(const short n);
		void setScan_Max(const short n);
		void setPrompts(const unsigned int n);
		void setDelayed(const unsigned int n);
		void setMultiples(const unsigned int n);
		void setNet_Trues(const unsigned int n);
		void setCor_Singles(const short i, const float n);
		void setUncor_Singles(const short i, const float n);
		void setTot_Avg_Cor(const float n);
		void setTot_Avg_Uncor(const float n);
		void setTotal_Coin_Rate(const unsigned int n);
		void setFrame_Start_Time(const unsigned int n);
		void setFrame_Duration(const unsigned int n);
		void setDeadtime_Correction_Factor(const float n);
		void setPhysical_Planes(const short i, const short n);
		void setCTI_Reserved(const short i, const short n);
		void setUser_Reserved(const short i, const short n);

	protected:
		// constructors
		CECAT7SubHeaderScan();
		// required method to copy relevant data from another MedIOHeader object
		CMedIOHeader& copyData(const CMedIOHeader& src);		

	private:
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
};

#endif // CECAT7SUBHEADERSCAN_H
