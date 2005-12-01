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

#ifndef __MEDIO_PRIVATE__
#include <CECATSubHeader>
#else
#include <CECATSubHeader.h>
#endif

// forward declarations
class CECATFile;
class CECATDirectoryItem;

class Q_EXPORT CECAT7SubHeaderAttenCorr : public CECATSubHeader
{
	public:
		// public available enumeration types
		enum Attenuation_Type { Atten_None=0,
														Atten_Meas,
														Atten_Calc };

		// constructors
		CECAT7SubHeaderAttenCorr(CECATFile* ecatFile,
														 CECATDirectoryItem* pDirItem = NULL);

		void clear();

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
		short	num_Dimensions(void) const;
		Attenuation_Type attenuation_Type(void) const;
		short num_R_Elements(void) const;
		short num_Angles(void) const;
		short num_Z_Elements(void) const;
		short ring_Difference(void) const;
		float x_Resolution(void) const;
		float y_Resolution(void) const;
		float	z_Resolution(void) const;
		float w_Resolution(void) const;
		float	scale_Factor(void) const;
		float	x_Offset(void) const;
		float	y_Offset(void) const;
		float	x_Radius(void) const;
		float	y_Radius(void) const;
		float	tilt_Angle(void) const;
		float attenuation_Coeff(void) const;
		float	attenuation_Min(void) const;
		float	attenuation_Max(void) const;
		float	skull_Thickness(void) const;
		short	num_Additional_Atten_Coeff(void) const;
		float	additional_Atten_Coeff(int n) const;
		float	edge_Finding_Threshold(void) const;
		short storage_Order(void) const;
		short span(void) const;
		short z_Elements(int n) const;
		short	unused(int n) const;
		short cti_Reserved(int n) const;
		
		// data mutator methods
		void setData_Type(const CECATSubHeader::Data_Type dType);
		void setNum_Dimensions(const short n);
		void setAttenuation_Type(const Attenuation_Type n);
		void setNum_R_Elements(const short n);
		void setNum_Angles(const short n);
		void setNum_Z_Elements(const short n);
		void setRing_Difference(const short n);
		void setX_Resolution(const float n);
		void setY_Resolution(const float n);
		void setZ_Resolution(const float n);
		void setW_Resolution(const float n);
		void setScale_Factor(const float n);
		void setX_Offset(const float n);
		void setY_Offset(const float n);
		void setX_Radius(const float n);
		void setY_Radius(const float n);
		void setTilt_Angle(const float n);
		void setAttenuation_Coeff(const float n);
		void setAttenuation_Min(const float n);
		void setAttenuation_Max(const float n);
		void setSkull_Thickness(const float n);
		void setNum_Additional_Atten_Coeff(const short n);
		void setAdditional_Atten_Coeff(const short i, const float n);
		void setEdge_Finding_Threshold(const float n);
		void setStorage_Order(const short n);
		void setSpan(const short n);
		void setZ_Elements(const short i, const short n);
		void setUnused(const short i, const short n);
		void setCTI_Reserverd(const short i, const short n);
		
	protected:
		// constructors
		CECAT7SubHeaderAttenCorr();

#ifdef __MEDIO_PRIVATE__
	private:
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
#endif
};

#endif // CECAT7SUBHEADERATTENCORR_H
