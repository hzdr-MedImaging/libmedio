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

#ifndef CECAT7SUBHEADERNORM_H
#define CECAT7SUBHEADERNORM_H

#include <qdatastream.h>

#include "CECATSubHeader.h"

// forward declarations
class CECATFile;

class Q_EXPORT CECAT7SubHeaderNorm : public CECATSubHeader
{
	public:
		// public enumeration types
		enum Norm_Qual_Factor_Code	{	TBD };

		// constructors
		CECAT7SubHeaderNorm(CECATFile* ecatFile,
												CECATDirectoryItem* pDirItem);
		CECAT7SubHeaderNorm(const CECAT7SubHeaderNorm& sh);
		
		// public methods													
		bool load(void);
		bool save(void) const;
		
		CECATSubHeader::Type subHeaderType(void) const
		{ return CECATSubHeader::ECAT7_Norm; }
		
		int size() const
		{	return ECAT7_HEADERSIZE_NORM; }

		// data access methods
		CECATSubHeader::Data_Type data_Type(void) const
		{ return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type);	}

		short num_Dimensions(void) const
		{ return m_Data.Num_Dimensions; }

		short num_R_Elements(void) const
		{ return m_Data.Num_R_Elements; }

		short num_Angles(void) const
		{ return m_Data.Num_Angles; }

		short num_Z_Elements(void) const
		{ return m_Data.Num_Z_Elements; }

		short ring_Difference(void) const
		{ return m_Data.Ring_Difference; }

		float scale_Factor(void) const
		{ return m_Data.Scale_Factor; }

		float norm_Min(void) const
		{ return m_Data.Norm_Min; }

		float norm_Max(void) const
		{ return m_Data.Norm_Max; }

		float fov_Source_Width(void) const
		{ return m_Data.FOV_Source_Width; }

		float norm_Quality_Factor(void) const
		{ return m_Data.Norm_Quality_Factor; }

		Norm_Qual_Factor_Code norm_Quality_Factor_Code(void) const
		{ return static_cast<Norm_Qual_Factor_Code>(m_Data.Norm_Quality_Factor_Code); }

		short storage_Order(void) const
		{ return m_Data.Storage_Order; }

		short span(void) const
		{ return m_Data.Span; }

		short z_Elements(const short i)
		{ return m_Data.Z_Elements[i]; }

		short cti_Reserved(const short i)
		{ return m_Data.CTI_reserved[i]; }

		short user_Reserved(const short i)
		{ return m_Data.User_Reserved[i]; }

		
		void setData_Type(const CECATSubHeader::Data_Type dType)
		{ m_Data.Data_Type = static_cast<Q_UINT16>(dType);	}	

		void setNum_Dimensions(const short n)
		{ m_Data.Num_Dimensions = n; }

		void setNum_R_Elements(const short n)
		{ m_Data.Num_R_Elements = n; }

		void setNum_Angles(const short n)
		{ m_Data.Num_Angles = n; }

		void setNum_Z_Elements(const short n)
		{ m_Data.Num_Z_Elements = n; }

		void setRing_Difference(const short n)
		{ m_Data.Ring_Difference = n; }

		void SetScale_Factor(const float n)
		{ m_Data.Scale_Factor = n; }

		void setNorm_Min(const float n)
		{ m_Data.Norm_Min = n; }

		void setNorm_Max(const float n)
		{ m_Data.Norm_Max = n; }

		void setFOV_Source_Width(const float n)
		{ m_Data.FOV_Source_Width = n; }

		void setNorm_Quality_Factor(const float n)
		{ m_Data.Norm_Quality_Factor = n; }

		void setNorm_Quality_Factor_Code(const Norm_Qual_Factor_Code n)
		{ m_Data.Norm_Quality_Factor_Code = static_cast<Q_UINT16>(n); }

		void setStorage_Order(const short n)
		{ m_Data.Storage_Order = n; }

		void setSpan(const short n)
		{ m_Data.Span = n; }

		void setZ_Elements(const short i, const short n)
		{ m_Data.Z_Elements[i] = n; }

		void setCTI_Reserved(const short i, const short n)
		{ m_Data.CTI_reserved[i] = n; }

		void setUser_Reserved(const short i, const short n)
		{ m_Data.User_Reserved[i] = n; }

	private:
		#pragma pack(2)	// set the structure alignment
		struct ECAT7SubHeader_Norm
		{
			Q_UINT16	Data_Type;
			Q_UINT16	Num_Dimensions;
			Q_UINT16	Num_R_Elements;
			Q_UINT16	Num_Angles;
			Q_UINT16	Num_Z_Elements;
			Q_UINT16	Ring_Difference;
			float			Scale_Factor;
			float			Norm_Min;
			float			Norm_Max;
			float			FOV_Source_Width;
			float			Norm_Quality_Factor;
			Q_UINT16	Norm_Quality_Factor_Code;
			Q_UINT16	Storage_Order;
			Q_UINT16	Span;
			Q_UINT16	Z_Elements[64];
			Q_UINT16	CTI_reserved[123];
			Q_UINT16	User_Reserved[50];
		} m_Data;
		#pragma pack()
};

#endif // CECAT7SUBHEADERNORM_H
