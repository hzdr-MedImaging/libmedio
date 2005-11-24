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

#include <qdatastream.h>

#include <CECATSubHeader.h>
#include <CECATDirectoryItem.h>

// forward declarations
class CECATFile;

class Q_EXPORT CECAT7SubHeaderNorm3D : public CECATSubHeader
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

		// the number of bytes the data of that header requires on disk
		int rawDataSize() const;
		
		CECATSubHeader::Type subHeaderType(void) const;
		
		// access methods to get directly data out of
		// the SubHeader
		CECATSubHeader::Data_Type data_Type(void) const;
		short num_R_Elements(void) const;
		short num_Transaxial_Crystals(void) const;
		short num_Crystal_Rings(void) const;
		short crystals_Per_Ring(void) const;
		short num_Geo_Corr_Planes(void) const;
		short uld(void) const;
		short lld(void) const;
		short scatter_Energy(void) const;
		float norm_Quality_Factor(void) const;
		Norm_Qual_Factor_Code norm_Quality_Factor_Code(void) const;
		float ring_DTCor1(const short i) const;
		float ring_DTCor2(const short i) const;
		float crystal_DTCor(const short i) const;
		short span(void) const;
		short max_Ring_Diff(void) const;
		short cti_Reserved(const short i) const;
		short user_Reserved(const short i) const;

		// data mutator methods
		void setData_Type(const CECATSubHeader::Data_Type dType);
		void setNum_R_Elements(const short n);
		void setNum_Transaxial_Crystals(const short n);
		void setNum_Crystal_Rings(const short n);
		void setCrystals_Per_Ring(const short n);
		void setNum_Geo_Corr_Planes(const short n);
		void setULD(const short n);
		void setLLD(const short n);
		void setScatter_Energy(const short n);
		void setNorm_Quality_Factor(const float n);
		void setNorm_Quality_Factor_Code(const Norm_Qual_Factor_Code n);
		void setRing_DTCor1(const short i, const float n);
		void setRing_DTCor2(const short i, const float n);
		void setCrystal_DTCor(const short i, const float n);
		void setSpan(const short n);
		void setMax_Ring_Diff(const short n);
		void setCTI_Reserved(const short i, const short n);
		void setUser_Reserved(const short i, const short n);

	protected:
		// required method to copy relevant data from another MedIOHeader object
		CMedIOHeader& copyData(const CMedIOHeader& src);
		
	private:
		struct ECAT7SubHeader_Norm3D
		{
			Q_UINT16	Data_Type;
			Q_UINT16	Num_R_Elements;
			Q_UINT16	Num_Transaxial_Crystals;
			Q_UINT16	Num_Crystal_Rings;
			Q_UINT16	Crystals_Per_Ring;
			Q_UINT16	Num_Geo_Corr_Planes;
			Q_UINT16	ULD;
			Q_UINT16	LLD;
			Q_UINT16	Scatter_Energy;
			float			Norm_Quality_Factor;
			Q_UINT16	Norm_Quality_Factor_Code;
			float			Ring_DTCor1[32];
			float			Ring_DTCor2[32];
			float			Crystal_DTCor[8];
			Q_UINT16	Span;
			Q_UINT16	Max_Ring_Diff;
			Q_UINT16	CTI_reserved[48];
			Q_UINT16	User_Reserved[50];
		} m_Data;
};

#endif // CECAT7SUBHEADERNORM3D_H
