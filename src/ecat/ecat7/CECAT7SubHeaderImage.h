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

		// constructors
		CECAT7SubHeaderImage();

		// public methods
		bool load(QDataStream& stream);
		bool save(QDataStream& stream);
		CECATSubHeader::Type rtti(void)
		{ return CECATSubHeader::ECAT7_Image; }

		// data acess methods
		CECATSubHeader::Data_Type data_Type(void)
		{ return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type);	}
		
		// methods to modify elements of the SubHeader
		void setData_Type(CECATSubHeader::Data_Type dType)
		{ m_Data.Data_Type = static_cast<Q_UINT16>(dType);	}
		
		void setNum_Dimensions(short dim)						
		{ m_Data.Num_Dimensions = dim; }
		
		void setX_Dimension(short dim)						
		{ m_Data.X_Dimension = dim; }
		
		void setY_Dimension(short dim)						
		{ m_Data.Y_Dimension = dim; }
		
		void setZ_Dimension(short dim)						
		{ m_Data.Z_Dimension = dim; }
		
		void setX_Offset(float off)						
		{ m_Data.X_Offset = off; }
		
		void setY_Offset(float off)						
		{ m_Data.Y_Offset = off; }
		
		void setZ_Offset(float off)						
		{ m_Data.Z_Offset = off; }
		
		void setRecon_Zoom(float zoom)						
		{ m_Data.Recon_Zoom = zoom; }
		
		void setScale_Factor(float scale)						
		{ m_Data.Scale_Factor = scale; }
		
		void setImage_Min(short min)						
		{ m_Data.Image_Min = min; }
		
		void setImage_Max(short max)						
		{ m_Data.Image_Max = max; }
		
		void setX_Pixel_Size(float size)						
		{ m_Data.X_Pixel_Size = size; }
		
		void setY_Pixel_Size(float size)						
		{ m_Data.Y_Pixel_Size = size; }
		
		void setZ_Pixel_Size(float size)						
		{ m_Data.Z_Pixel_Size = size; }
		
		void setFrame_Duration(unsigned int dur)		
		{	m_Data.Frame_Duration = dur;	}
		
		void setFrame_Start_Time(unsigned int time)		
		{	m_Data.Frame_Start_Time = time;	}
		
		void setFilter_Code(short code)						
		{ m_Data.Filter_Code = code; }
		
		void setX_Resolution(float res)						
		{ m_Data.X_Resolution = res; }
		
		void setY_Resolution(float res)						
		{ m_Data.Y_Resolution = res; }
		
		void setZ_Resolution(float res)						
		{ m_Data.Z_Resolution = res; }
		
		void setNum_R_Elements(float elem)						
		{ m_Data.Num_R_Elements = elem; }
		
		void setNum_Angles(float angles)							
		{ m_Data.Num_Angles = angles; }
		
		void setZ_Rotation_Angle(float angle)						
		{ m_Data.Z_Rotation_Angle = angle; }
		
		void setDecay_Corr_Fctr(float factor)							
		{ m_Data.Decay_Corr_Fctr = factor; }
		
		void setProcessing_Code(unsigned int code)		
		{	m_Data.Processing_Code = code;	}
		
		void setGate_Duration(unsigned int dur)		
		{	m_Data.Gate_Duration = dur;	}
		
		void setR_Wave_Offset(unsigned int off)		
		{	m_Data.R_Wave_Offset = off;	}
		
		void setNum_Accepted_Beats(unsigned int beats)		
		{	m_Data.Num_Accepted_Beats = beats;	}
		
		void setFilter_Cutoff_Frequency(float freq)						
		{ m_Data.Filter_Cutoff_Frequency = freq; }
		
		void setFilter_Resolution(float res)						
		{ m_Data.Filter_Resolution = res; }
		
		void setFilter_Ramp_Slope(float slope)						
		{ m_Data.Filter_Ramp_Slope = slope; }
		
		void setFilter_Order(short order)						
		{ m_Data.Filter_Order = order; }
		
		void setFilter_Scatter_Fraction(float frac)						
		{ m_Data.Filter_Scatter_Fraction = frac; }
		
		void setFilter_Scatter_Slope(float slope)						
		{ m_Data.Filter_Scatter_Slope = slope; }
		
		void setAnnotation(const char* str)													
		{ strncpy(m_Data.Annotation, str, 40); }
		
		void setMT_1_1(float value)						
		{ m_Data.MT_1_1 = value; }
		
		void setMT_1_2(float value)						
		{ m_Data.MT_1_2 = value; }
		
		void setMT_1_3(float value)						
		{ m_Data.MT_1_3 = value; }
		
		void setMT_2_1(float value)						
		{ m_Data.MT_2_1 = value; }
		
		void setMT_2_2(float value)						
		{ m_Data.MT_2_2 = value; }
		
		void setMT_2_3(float value)						
		{ m_Data.MT_2_3 = value; }
		
		void setMT_3_1(float value)						
		{ m_Data.MT_3_1 = value; }
		
		void setMT_3_2(float value)						
		{ m_Data.MT_3_2 = value; }
		
		void setMT_3_3(float value)						
		{ m_Data.MT_3_3 = value; }
		
		void setRFilter_Cutoff(float cutoff)						
		{ m_Data.RFilter_Cutoff = cutoff; }
		
		void setRFilter_Resolution(float res)						
		{ m_Data.RFilter_Resolution = res; }
		
		void setZFilter_Code(short code)						
		{ m_Data.ZFilter_Code = code; }
		
		void setRFilter_Order(short order)						
		{ m_Data.RFilter_Order = order; }
		
		void setMT_1_4(float value)						
		{ m_Data.MT_1_4 = value; }
		
		void setMT_2_4(float value)						
		{ m_Data.MT_2_4 = value; }
		
		void setMT_3_4(float value)						
		{ m_Data.MT_3_4 = value; }
		
		void setScatter_Type(short type)						
		{ m_Data.Scatter_Type = type; }
		
		void setRecon_Type(short type)						
		{ m_Data.Recon_Type = type; }
		
		void setRecon_Views(short views)						
		{ m_Data.Recon_Views = views; }
		
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
			Q_UINT16	User_Reserved[49];
		} m_Data;
		#pragma pack()
};

#endif // CECAT7SUBHEADERIMAGE_H
