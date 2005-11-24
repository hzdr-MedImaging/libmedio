/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 20042-2005 by Jens Langner <Jens.Langner@light-speed.de>

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
#include <CECATDirectoryItem.h>

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
		short polar_Map_Type(void) const;
		short num_Rings(void) const;
		short sectors_Per_Ring(const short i) const;
		float ring_Position(const short i) const;
		short ring_Angle(const short i) const;
		short start_Angle(void) const;
		short long_Axis_Left(const short i) const;
		short long_Axis_Right(const short i) const;
		short position_Data(void) const;
		short image_Min(void) const;
		short image_Max(void) const;
		float scale_Factor(void) const;
		float pixel_Size(void) const;
		unsigned int frame_Duration(void) const;
		unsigned int frame_Start_Time(void) const;
		unsigned int processing_Code(void) const;
		short quant_Units(void) const;
		const char* annotation(void) const;
		unsigned int gate_Duration(void) const;
		unsigned int r_Wave_Offset(void) const;
		unsigned int num_Accepted_Beats(void) const;
		const char* polar_Map_Protocol(void) const;
		const char* database_Name(void) const;
		short cti_Reserved(const short i) const;
		short user_Reserved(const short i) const;
		
		// data mutator methods
		void setData_Type(const CECATSubHeader::Data_Type dType);
		void setPolar_Map_Type(const short n);
		void setNum_Rings(const short n);
		void setSectors_Per_Ring(const short i, const short n);
		void setRing_Position(const short i, const float n);
		void setRing_Angle(const short i, const short n);
		void setStart_Angle(const short n);
		void setLong_Axis_Left(const short i, const short n);
		void setLong_Axis_Right(const short i, const short n);
		void setPosition_Data(const short n);
		void setImage_Min(const short n);
		void setImage_Max(const short n);
		void setScale_Factor(const float n);
		void setPixel_Size(const float n);
		void setFrame_Duration(const unsigned int n);
		void setFrame_Start_Time(const unsigned int n);
		void setProcessing_Code(const unsigned int n);
		void setQuant_Units(const short n);
		void setAnnotation(const char* str);
		void setGate_Duration(const unsigned int n);
		void setR_Wave_Offset(const unsigned int n);
		void setNum_Accepted_Beats(const unsigned int n);
		void setPolar_Map_Protocol(const char* str);
		void setDatabase_Name(const char* str);
		void setCTI_Reserved(const short i, const short n);
		void setUser_Reserved(const short i, const short n);

	protected:
		// constructors
		CECAT7SubHeaderPolarMap();

	private:
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
};

#endif // CECAT7SUBHEADERPOLARMAP_H
