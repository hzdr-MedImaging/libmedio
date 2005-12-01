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

#ifndef __MEDIO_PRIVATE__
#include <CECATSubHeader>
#else
#include <CECATSubHeader.h>
#endif

// forward declarations
class CECATFile;
class CECATDirectoryItem;

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

		void clear();

		// constructors
		CECAT7SubHeaderScan3D(CECATFile* ecatFile,
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
		
		// methods to access elements of the SubHeader
		CECATSubHeader::Data_Type data_Type(void)	const;
		short num_Dimensions(void) const;
		short num_R_Elements(void) const;
		short num_Angles(void) const;
		short corrections_Applied(void) const;
		short num_Z_Elements(const short i) const;
		short ring_Difference(void) const;
		Storage_Order storage_Order(void) const;
		short axial_Compression(void) const;
		float X_Resolution(void) const;
		float V_Resolution(void) const;
		float Z_Resolution(void) const;
		float W_Resolution(void) const;
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
		float tot_Avg_Cor(void) const;
		float tot_Avg_Uncor(void) const;
		unsigned int total_Coin_Rate(void) const;
		unsigned int frame_Start_Time(void) const;
		unsigned int frame_Duration(void) const;
		float deadtime_Correction_Factor(void) const;
		short cti_Reserved(const short i) const;
		short user_Reserved(const short i) const;
		float uncor_Singles(const short bucket) const;

		// methods to modify elements of the SubHeader
		void setData_Type(const CECATSubHeader::Data_Type dType);
		void setNum_Dimensions(const short dim);
		void setNum_R_Elements(const short elem);
		void setNum_Angles(const short angles);
		void setCorrections_Applied(const short cor);
		void setNum_Z_Elements(const short i, const short num);
		void setRing_Difference(const short diff);
		void setStorage_Order(const Storage_Order order);
		void setAxial_Compression(const short comp);
		void setX_Resolution(const float res);
		void setV_Resolution(const float res);
		void setZ_Resolution(const float res);
		void setW_Resolution(const float res);
		void setGate_Reserved(const short i, const short val);
		void setGate_Duration(const unsigned int dur);
		void setR_Wave_Offset(const unsigned int off);
		void setNum_Accepted_Beats(const unsigned int b);
		void setScale_Factor(const float factor);
		void setScan_Min(const short min);
		void setScan_Max(const short max);
		void setPrompts(const unsigned int n);
		void setDelayed(const unsigned int n);
		void setMultiples(const unsigned int n);
		void setNet_Trues(const unsigned int n);
		void setTot_Avg_Cor(const float value);
		void setTot_Avg_Uncor(const float value);
		void setTotal_Coin_Rate(const unsigned int n);
		void setFrame_Start_Time(const unsigned int t);
		void setFrame_Duration(const unsigned int t);
		void setDeadtime_Correction_Factor(const float f);
		void setCTI_Reserved(const short i, const short value);
		void setUser_Reserved(const short i, const short value);
		void setUncor_Singles(const short b, const float value);

	protected:
		// constructors
		CECAT7SubHeaderScan3D();

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
