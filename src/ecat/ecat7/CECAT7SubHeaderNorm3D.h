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

#ifndef CECAT7SUBHEADERNORM3D_H
#define CECAT7SUBHEADERNORM3D_H

#include <QDataStream>

#include <CECATSubHeader.h>

#include <cstdlib>

// forward declarations
class CECATFile;

class CECAT7SubHeaderNorm3D : public CECATSubHeader
{
	public:
		// public enumeration types
		enum Norm_Qual_Factor_Code { TBD };

		// constructors
		CECAT7SubHeaderNorm3D(CECATFile* ecatFile,
													CECATDirectoryItem* pDirItem = NULL);
		CECAT7SubHeaderNorm3D(const CECAT7SubHeaderNorm3D& sh);

		// public methods
		bool load(void);
		bool save(void) const;

		CECATSubHeader::Type subHeaderType(void) const
		{ return CECATSubHeader::ECAT7_Norm3D; }
		
		int size() const
		{	return ECAT7_HEADERSIZE_NORM3D; }
	
		// access methods to get directly data out of
		// the SubHeader
		CECATSubHeader::Data_Type data_Type(void) const
		{ return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type);	}

		short num_R_Elements(void) const
		{ return m_Data.Num_R_Elements; }

		short num_Transaxial_Crystals(void) const
		{ return m_Data.Num_Transaxial_Crystals; }

		short num_Crystal_Rings(void) const
		{ return m_Data.Num_Crystal_Rings; }

		short crystals_Per_Ring(void) const
		{ return m_Data.Crystals_Per_Ring; }

		short num_Geo_Corr_Planes(void) const
		{ return m_Data.Num_Geo_Corr_Planes; }

		short uld(void) const
		{ return m_Data.ULD; }

		short lld(void) const
		{ return m_Data.LLD; }

		short scatter_Energy(void) const
		{ return m_Data.Scatter_Energy; }

		float norm_Quality_Factor(void) const
		{ return m_Data.Norm_Quality_Factor; }

		Norm_Qual_Factor_Code norm_Quality_Factor_Code(void) const
		{ return static_cast<Norm_Qual_Factor_Code>(m_Data.Norm_Quality_Factor_Code); }

		float ring_DTCor1(const short i) const
		{ return m_Data.Ring_DTCor1[i]; }

		float ring_DTCor2(const short i) const
		{ return m_Data.Ring_DTCor2[i]; }

		float crystal_DTCor(const short i) const
		{ return m_Data.Crystal_DTCor[i]; }

		short span(void) const
		{ return m_Data.Span; }

		short max_Ring_Diff(void) const
		{ return m_Data.Max_Ring_Diff; }

		short cti_Reserved(const short i)
		{ return m_Data.CTI_reserved[i]; }

		short user_Reserved(const short i)
		{ return m_Data.User_Reserved[i]; }


		void setData_Type(const CECATSubHeader::Data_Type dType)
		{ m_Data.Data_Type = static_cast<quint16>(dType);	}			
		
		void setNum_R_Elements(const short n)
		{ m_Data.Num_R_Elements = n; }

		void setNum_Transaxial_Crystals(const short n)
		{ m_Data.Num_Transaxial_Crystals = n; }

		void setNum_Crystal_Rings(const short n)
		{ m_Data.Num_Crystal_Rings = n; }

		void setCrystals_Per_Ring(const short n)
		{ m_Data.Crystals_Per_Ring = n; }

		void setNum_Geo_Corr_Planes(const short n)
		{ m_Data.Num_Geo_Corr_Planes = n; }

		void setULD(const short n)
		{ m_Data.ULD = n; }

		void setLLD(const short n)
		{ m_Data.LLD = n; }

		void setScatter_Energy(const short n)
		{ m_Data.Scatter_Energy = n; }

		void setNorm_Quality_Factor(const float n)
		{ m_Data.Norm_Quality_Factor = n; }

		void setNorm_Quality_Factor_Code(const Norm_Qual_Factor_Code n)
		{ m_Data.Norm_Quality_Factor_Code = static_cast<quint16>(n); }

		void setRing_DTCor1(const short i, const float n)
		{ m_Data.Ring_DTCor1[i] = n; }

		void setRing_DTCor2(const short i, const float n)
		{ m_Data.Ring_DTCor2[i] = n; }

		void setCrystal_DTCor(const short i, const float n)
		{ m_Data.Crystal_DTCor[i] = n; }

		void setSpan(const short n)
		{ m_Data.Span = n; }

		void setMax_Ring_Diff(const short n)
		{ m_Data.Max_Ring_Diff = n; }

		void setCTI_Reserved(const short i, const short n)
		{ m_Data.CTI_reserved[i] = n; }

		void setUser_Reserved(const short i, const short n)
		{ m_Data.User_Reserved[i] = n; }

	protected:
		// required method to copy relevant data from another MedIOHeader object
		CMedIOHeader& copyData(const CMedIOHeader& src);
		
	private:
		#pragma pack(2)	// set the structure alignment
		struct ECAT7SubHeader_Norm3D
		{
			quint16	Data_Type;
			quint16	Num_R_Elements;
			quint16	Num_Transaxial_Crystals;
			quint16	Num_Crystal_Rings;
			quint16	Crystals_Per_Ring;
			quint16	Num_Geo_Corr_Planes;
			quint16	ULD;
			quint16	LLD;
			quint16	Scatter_Energy;
			float		Norm_Quality_Factor;
			quint16	Norm_Quality_Factor_Code;
			float		Ring_DTCor1[32];
			float		Ring_DTCor2[32];
			float		Crystal_DTCor[8];
			quint16	Span;
			quint16	Max_Ring_Diff;
			quint16	CTI_reserved[48];
			quint16	User_Reserved[50];
		} m_Data;
		#pragma pack()
};

#endif // CECAT7SUBHEADERNORM3D_H
