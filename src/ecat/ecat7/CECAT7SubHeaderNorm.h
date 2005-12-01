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

#ifndef CECAT7SUBHEADERNORM_H
#define CECAT7SUBHEADERNORM_H

#include <qdatastream.h>

#ifndef __MEDIO_PRIVATE__
#include <CECATSubHeader>
#else
#include <CECATSubHeader.h>
#endif

// forward declarations
class CECATFile;
class CECATDirectoryItem;

class Q_EXPORT CECAT7SubHeaderNorm : public CECATSubHeader
{
	public:
		// public enumeration types
		enum Norm_Qual_Factor_Code	{	TBD };

		// constructors
		CECAT7SubHeaderNorm(CECATFile* ecatFile,
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
		short num_Dimensions(void) const;
		short num_R_Elements(void) const;
		short num_Angles(void) const;
		short num_Z_Elements(void) const;
		short ring_Difference(void) const;
		float scale_Factor(void) const;
		float norm_Min(void) const;
		float norm_Max(void) const;
		float fov_Source_Width(void) const;
		float norm_Quality_Factor(void) const;
		Norm_Qual_Factor_Code norm_Quality_Factor_Code(void) const;
		short storage_Order(void) const;
		short span(void) const;
		short z_Elements(const short i) const;
		short cti_Reserved(const short i) const;
		short user_Reserved(const short i) const;
		
		// data mutator methods
		void setData_Type(const CECATSubHeader::Data_Type dType);
		void setNum_Dimensions(const short n);
		void setNum_R_Elements(const short n);
		void setNum_Angles(const short n);
		void setNum_Z_Elements(const short n);
		void setRing_Difference(const short n);
		void SetScale_Factor(const float n);
		void setNorm_Min(const float n);
		void setNorm_Max(const float n);
		void setFOV_Source_Width(const float n);
		void setNorm_Quality_Factor(const float n);
		void setNorm_Quality_Factor_Code(const Norm_Qual_Factor_Code n);
		void setStorage_Order(const short n);
		void setSpan(const short n);
		void setZ_Elements(const short i, const short n);
		void setCTI_Reserved(const short i, const short n);
		void setUser_Reserved(const short i, const short n);

	protected:
		// constructors
		CECAT7SubHeaderNorm();

#ifdef __MEDIO_PRIVATE__
	private:
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
#endif
};

#endif // CECAT7SUBHEADERNORM_H
