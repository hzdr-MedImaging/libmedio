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

#ifndef CECAT7SUBHEADERIMAGE_H
#define CECAT7SUBHEADERIMAGE_H

#include <qdatastream.h>

#include "CECATSubHeader.h"

// forward declarations

class Q_EXPORT CECAT7SubHeaderImage : public CECATSubHeader
{
	public:
		// public enumeration types
		enum Filter_Code			{	NoFilter=0,
														Ramp,
														Butterfield,
														Hanning,
														Hamming,
														Parzen,
														Shepp };

		enum Scatter_Type			{	None=0,
														Deconvolution,
														Simulated,
														DualEnergy };

		enum Recon_Type				{	FilteredBackProjection=0,
														ForwardProjection3D,
														Ramp3D,
														Favor3D,
														SSRB,
														MultiSliceRebinning,
														FORE };

		// public BitMask field
		enum Processing_Code	{	NotProcessed				= (0<<0),
														Normalized					= (1<<0),
														MeasuredAttenCorr		= (1<<1),
														CalculatedAttenCorr = (1<<2),
														XSmoothing					= (1<<3),
														YSmoothing					= (1<<4),
														ZSmoothing					= (1<<5),
														Scatter2DCorr				= (1<<6),
														Scatter3DCorr				=	(1<<7),
														ArcCorrection				=	(1<<8),
														DecayCorrection			=	(1<<9),
														OnlineCompression		= (1<<10) };

		// constructor
		CECAT7SubHeaderImage()
		{
			// check that the headsize is 512 bytes long
			ASSERT(sizeof(struct ECAT7SubHeader_Image) == 512);

			// then clear the structure
			memset(&m_Data, 0, sizeof(struct ECAT7SubHeader_Image));
		}

		// public methods
		bool load(QDataStream& stream);
		bool save(QDataStream& stream);
		CECATSubHeader::Type rtti(void)
		{ return CECATSubHeader::ECAT7_Image; }

		// data acess methods
		CECATSubHeader::Data_Type data_Type(void)
		{ return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type);	}
		
		void setData_Type(CECATSubHeader::Data_Type dType)
		{ m_Data.Data_Type = static_cast<Q_UINT16>(dType);	}
		

	private:
		#pragma pack(2)	// set the structure alignment
		struct ECAT7SubHeader_Image
		{
			Q_UINT16	Data_Type;
			Q_UINT16	Num_Dimensions;
			Q_UINT16	X_Dimension;
			Q_UINT16	Y_Dimension;
			Q_UINT16	Z_Dimension;
			float			X_Offset;
			float			Y_Offset;
			float			Z_Offset;
			float			Recon_Zoom;
			float			Scale_Factor;
			Q_UINT16	Image_Min;
			Q_UINT16	Image_Max;
			float			X_Pixel_Size;
			float			Y_Pixel_Size;
			float			Z_Pixel_Size;
			Q_UINT32	Frame_Duration;
			Q_UINT32	Frame_Start_Time;
			Q_UINT16	Filter_Code;
			float			X_Resolution;
			float			Y_Resolution;
			float			Z_Resolution;
			float			Num_R_Elements;
			float			Num_Angles;
			float			Z_Rotation_Angle;
			float			Decay_Corr_Fctr;
			Q_UINT32	Processing_Code;
			Q_UINT32	Gate_Duration;
			Q_UINT32	R_Wave_Offset;
			Q_UINT32	Num_Accepted_Beats;
			float			Filter_Cutoff_Frequency;
			float			Filter_Resolution;
			float			Filter_Ramp_Slope;
			Q_UINT16	Filter_Order;
			float			Filter_Scatter_Fraction;
			float			Filter_Scatter_Slope;
			char			Annotation[40];
			float			MT_1_1;
			float			MT_1_2;
			float			MT_1_3;
			float			MT_2_1;
			float			MT_2_2;
			float			MT_2_3;
			float			MT_3_1;
			float			MT_3_2;
			float			MT_3_3;
			float			RFilter_Cutoff;
			float			RFilter_Resolution;
			Q_UINT16	RFilter_Code;
			Q_UINT16	RFilter_Order;
			float			ZFilter_Cutoff;
			float			ZFilter_Resolution;
			Q_UINT16	ZFilter_Code;
			Q_UINT16	ZFilter_Order;
			float			MT_1_4;
			float			MT_2_4;
			float			MT_3_4;
			Q_UINT16	Scatter_Type;
			Q_UINT16	Recon_Type;
			Q_UINT16	Recon_Views;
			Q_UINT16	CTI_Reserved[87];
			Q_UINT16	User_Reserved[48];
		} m_Data;
		#pragma pack()
};

#endif // CECAT7SUBHEADERIMAGE_H
