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

#include <CECATSubHeader.h>

// forward declarations
class CECATFile;

class Q_EXPORT CECAT7SubHeaderPolarMap : public CECATSubHeader
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

		// constructors
		CECAT7SubHeaderPolarMap(CECATFile* ecatFile,
														CECATDirectoryItem* pDirItem = NULL);
		CECAT7SubHeaderPolarMap(const CECAT7SubHeaderPolarMap& sh);
			
		// public methods
		bool load(void);
		bool save(void) const;

		CECATSubHeader::Type subHeaderType(void) const
		{ return CECATSubHeader::ECAT7_PolarMap; }
		
		int size() const 
		{	return ECAT7_HEADERSIZE_POLARMAP; }

		// data access methods
		CECATSubHeader::Data_Type data_Type(void) const
		{ return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type); }

		short polar_Map_Type(void) const
		{ return m_Data.Polar_Map_Type; }

		short num_Rings(void) const
		{ return m_Data.Num_Rings; }

		short sectors_Per_Ring(const short i) const
		{ return m_Data.Sectors_Per_Ring[i]; }

		float ring_Position(const short i) const
		{ return m_Data.Ring_Position[i]; }

		short ring_Angle(const short i) const
		{ return m_Data.Ring_Angle[i]; }

		short start_Angle(void) const
		{ return m_Data.Start_Angle; }

		short long_Axis_Left(const short i) const
		{ return m_Data.Long_Axis_Left[i]; }

		short long_Axis_Right(const short i) const
		{ return m_Data.Long_Axis_Right[i]; }

		short position_Data(void) const
		{ return m_Data.Position_Data; }

		short image_Min(void) const
		{ return m_Data.Image_Min; }

		short image_Max(void) const
		{ return m_Data.Image_Max; }

		float scale_Factor(void) const
		{ return m_Data.Scale_Factor; }

		float pixel_Size(void) const
		{ return m_Data.Pixel_Size; }

		unsigned int frame_Duration(void) const
		{ return m_Data.Frame_Duration; }

		unsigned int frame_Start_Time(void) const
		{ return m_Data.Frame_Start_Time; }

		unsigned int processing_Code(void) const
		{ return m_Data.Processing_Code; }

		short quant_Units(void) const
		{ return m_Data.Quant_Units; }

		const char* annotation(void) const
		{ return m_Data.Annotation; }

		unsigned int gate_Duration(void) const
		{ return m_Data.Gate_Duration; }

		unsigned int r_Wave_Offset(void) const
		{ return m_Data.R_Wave_Offset; }

		unsigned int num_Accepted_Beats(void) const
		{ return m_Data.Num_Accepted_Beats; }

		const char* polar_Map_Protocol(void) const
		{ return m_Data.Polar_Map_Protocol; }

		const char* database_Name(void) const
		{ return m_Data.Database_Name; }

		short cti_Reserved(const short i)
		{ return m_Data.CTI_reserved[i]; }

		short user_Reserved(const short i)
		{ return m_Data.User_Reserved[i]; }
	
		
		void setData_Type(const CECATSubHeader::Data_Type dType)
		{ m_Data.Data_Type = static_cast<Q_UINT16>(dType);	}		

		void setPolar_Map_Type(const short n)
		{ m_Data.Polar_Map_Type = n; }

		void setNum_Rings(const short n)
		{ m_Data.Num_Rings = n; }

		void setSectors_Per_Ring(const short i, const short n)
		{ m_Data.Sectors_Per_Ring[i] = n; }

		void setRing_Position(const short i, const float n)
		{ m_Data.Ring_Position[i] = n; }

		void setRing_Angle(const short i, const short n)
		{ m_Data.Ring_Angle[i] = n; }

		void setStart_Angle(const short n)
		{ m_Data.Start_Angle = n; }

		void setLong_Axis_Left(const short i, const short n)
		{ m_Data.Long_Axis_Left[i] = n; }

		void setLong_Axis_Right(const short i, const short n)
		{ m_Data.Long_Axis_Right[i] = n; }

		void setPosition_Data(const short n)
		{ m_Data.Position_Data = n; }

		void setImage_Min(const short n)
		{ m_Data.Image_Min = n; }

		void setImage_Max(const short n)
		{ m_Data.Image_Max = n; }

		void setScale_Factor(const float n)
		{ m_Data.Scale_Factor = n; }

		void setPixel_Size(const float n)
		{ m_Data.Pixel_Size = n; }

		void setFrame_Duration(const unsigned int n)
		{ m_Data.Frame_Duration = n; }

		void setFrame_Start_Time(const unsigned int n)
		{ m_Data.Frame_Start_Time = n; }

		void setProcessing_Code(const unsigned int n)
		{ m_Data.Processing_Code = n; }

		void setQuant_Units(const short n)
		{ m_Data.Quant_Units = n; }

		void setAnnotation(const char* str)
		{ strncpy(m_Data.Annotation, str, 40); }

		void setGate_Duration(const unsigned int n)
		{ m_Data.Gate_Duration = n; }

		void setR_Wave_Offset(const unsigned int n)
		{ m_Data.R_Wave_Offset = n; }

		void setNum_Accepted_Beats(const unsigned int n)
		{ m_Data.Num_Accepted_Beats = n; }

		void setPolar_Map_Protocol(const char* str)
		{ strncpy(m_Data.Polar_Map_Protocol, str, 20); }

		void setDatabase_Name(const char* str)
		{ strncpy(m_Data.Database_Name, str, 30); }

		void setCTI_Reserved(const short i, const short n)
		{ m_Data.CTI_reserved[i] = n; }

		void setUser_Reserved(const short i, const short n)
		{ m_Data.User_Reserved[i] = n; }


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
			Q_UINT32	Frame_Duration;
			Q_UINT32	Frame_Start_Time;
			Q_UINT16	Processing_Code;
			Q_UINT16	Quant_Units;
			char			Annotation[40];
			Q_UINT32	Gate_Duration;
			Q_UINT32	R_Wave_Offset;
			Q_UINT32	Num_Accepted_Beats;
			char			Polar_Map_Protocol[20];
			char			Database_Name[30];
			Q_UINT16	CTI_reserved[27];
			Q_UINT16	User_Reserved[27];
		} m_Data;
		#pragma pack()
};

#endif // CECAT7SUBHEADERPOLARMAP_H
