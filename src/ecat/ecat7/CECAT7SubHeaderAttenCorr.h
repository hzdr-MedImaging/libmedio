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

#ifndef CECAT7SUBHEADERATTENCORR_H
#define CECAT7SUBHEADERATTENCORR_H

#include <qdatastream.h>

#include <CECATSubHeader.h>

// forward declarations
class CECATFile;

class Q_EXPORT CECAT7SubHeaderAttenCorr : public CECATSubHeader
{
	public:
		// public available enumeration types
		enum Attenuation_Type { Atten_None=0,
														Atten_Meas,
														Atten_Calc };

		// constructors
		CECAT7SubHeaderAttenCorr(CECATFile* ecatFile,
														 CECATDirectoryItem* pDirItem);
		CECAT7SubHeaderAttenCorr(const CECAT7SubHeaderAttenCorr& sh);

		// public methods
		bool load(void);
		bool save(void) const;

		CECATSubHeader::Type subHeaderType(void) const
		{ return CECATSubHeader::ECAT7_AttenCorr; }
		
		int size() const 
		{ return ECAT7_HEADERSIZE_ATTENCORR; }

		// data access methods
		CECATSubHeader::Data_Type data_Type(void) const
		{ return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type); }

		short	num_Dimensions(void) const
		{ return m_Data.Num_Dimensions; }

		Attenuation_Type attenuation_Type(void) const
		{ return static_cast<Attenuation_Type>(m_Data.Attenuation_Type); }

		short num_R_Elements(void) const
		{ return m_Data.Num_R_Elements; }

		short num_Angles(void) const
		{ return m_Data.Num_Angles; }

		short num_Z_Elements(void) const
		{ return m_Data.Num_Z_Elements; }

		short ring_Difference(void) const
		{ return m_Data.Ring_Difference; }

		float x_Resolution(void) const
		{ return m_Data.X_Resolution; }

		float y_Resolution(void) const
		{ return m_Data.Y_Resolution; }

		float	z_Resolution(void) const
		{ return m_Data.Z_Resolution; }

		float w_Resolution(void) const
		{ return m_Data.W_Resolution; }

		float	scale_Factor(void) const
		{ return m_Data.Scale_Factor; }

		float	x_Offset(void) const
		{ return m_Data.X_Offset; }

		float	y_Offset(void) const
		{ return m_Data.Y_Offset; }

		float	x_Radius(void) const
		{ return m_Data.X_Radius; }

		float	y_Radius(void) const
		{ return m_Data.Y_Radius; }

		float	tilt_Angle(void) const
		{ return m_Data.Tilt_Angle; }

		float attenuation_Coeff(void) const
		{ return m_Data.Attenuation_Coeff; }

		float	attenuation_Min(void) const
		{ return m_Data.Attenuation_Min; }

		float	attenuation_Max(void) const
		{ return m_Data.Attenuation_Max; }

		float	skull_Thickness(void) const
		{ return m_Data.Skull_Thickness; }

		short	num_Additional_Atten_Coeff(void) const
		{ return m_Data.Num_Additional_Atten_Coeff; }

		float	additional_Atten_Coeff(int n) const
		{ return m_Data.Additional_Atten_Coeff[n]; }

		float	edge_Finding_Threshold(void) const
		{ return m_Data.Edge_Finding_Threshold; }

		short storage_Order(void) const
		{ return m_Data.Storage_Order; }

		short span(void) const
		{ return m_Data.Span; }

		short z_Elements(int n) const
		{ return m_Data.Z_Elements[n]; }

		short	unused(int n) const
		{ return m_Data.Unused[n]; }

		short cti_Reserved(int n) const
		{ return m_Data.CTI_Reserved[n]; }		
		
		void setData_Type(const CECATSubHeader::Data_Type dType)
		{ m_Data.Data_Type = static_cast<Q_UINT16>(dType); }

		void setNum_Dimensions(const short n)
		{ m_Data.Num_Dimensions = n; }

		void setAttenuation_Type(const Attenuation_Type n)
		{ m_Data.Attenuation_Type = static_cast<Q_UINT16>(n); }

		void setNum_R_Elements(const short n)
		{ m_Data.Num_R_Elements = n; }

		void setNum_Angles(const short n)
		{ m_Data.Num_Angles = n; }

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

		void setScale_Factor(const float n)
		{ m_Data.Scale_Factor = n; }

		void setX_Offset(const float n)
		{ m_Data.X_Offset = n; }

		void setY_Offset(const float n)
		{ m_Data.Y_Offset = n; }

		void setX_Radius(const float n)
		{ m_Data.X_Radius = n; }

		void setY_Radius(const float n)
		{ m_Data.Y_Radius = n; }

		void setTilt_Angle(const float n)
		{ m_Data.Tilt_Angle = n; }

		void setAttenuation_Coeff(const float n)
		{ m_Data.Attenuation_Coeff = n; }

		void setAttenuation_Min(const float n)
		{ m_Data.Attenuation_Min = n; }

		void setAttenuation_Max(const float n)
		{ m_Data.Attenuation_Max = n; }

		void setSkull_Thickness(const float n)
		{ m_Data.Skull_Thickness = n; }

		void setNum_Additional_Atten_Coeff(const short n)
		{ m_Data.Num_Additional_Atten_Coeff = n; }

		void setAdditional_Atten_Coeff(const short i, const float n)
		{ m_Data.Additional_Atten_Coeff[i] = n; }

		void setEdge_Finding_Threshold(const float n)
		{ m_Data.Edge_Finding_Threshold = n; }

		void setStorage_Order(const short n)
		{ m_Data.Storage_Order = n; }

		void setSpan(const short n)
		{ m_Data.Span = n; }

		void setZ_Elements(const short i, const short n)
		{ m_Data.Z_Elements[i] = n; }

		void setUnused(const short i, const short n)
		{ m_Data.Unused[i] = n; }

		void setCTI_Reserverd(const short i, const short n)
		{ m_Data.CTI_Reserved[i] = n; }

	private:
		#pragma pack(2)	// set the structure alignment
		struct ECAT7SubHeader_AttenCorr
		{
			Q_UINT16	Data_Type;
			Q_UINT16	Num_Dimensions;
			Q_UINT16	Attenuation_Type;
			Q_UINT16	Num_R_Elements;
			Q_UINT16	Num_Angles;
			Q_UINT16	Num_Z_Elements;
			Q_UINT16	Ring_Difference;
			float			X_Resolution;
			float			Y_Resolution;
			float			Z_Resolution;
			float			W_Resolution;
			float			Scale_Factor;
			float			X_Offset;
			float			Y_Offset;
			float			X_Radius;
			float			Y_Radius;
			float			Tilt_Angle;
			float			Attenuation_Coeff;
			float			Attenuation_Min;
			float			Attenuation_Max;
			float			Skull_Thickness;
			Q_UINT16	Num_Additional_Atten_Coeff;
			float			Additional_Atten_Coeff[8];
			float			Edge_Finding_Threshold;
			Q_UINT16	Storage_Order;
			Q_UINT16	Span;
			Q_UINT16	Z_Elements[64];
			Q_UINT16	Unused[86];
			Q_UINT16	CTI_Reserved[50];
		} m_Data;
		#pragma pack()
};

#endif // CECAT7SUBHEADERATTENCORR_H
