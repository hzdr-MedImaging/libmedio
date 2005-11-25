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

#ifndef CECAT6MAINHEADER_H
#define CECAT6MAINHEADER_H

#include <CECATMainHeader.h>
#include <CECATDirectoryItem.h>
#include <CMedIOHeader.h>

#include <QDataStream>
#include <QTextStream>

// forward declarations
class CECATFile;

class CECAT6MainHeader : public CECATMainHeader
{
	public:
		enum File_Type { Unknown=0, // FIXME: Others are missing for the moment
									 };
		
		CECAT6MainHeader(CECATFile* ecatFile = NULL,
										 CECATMainHeader::Type fileType = 
												CECATMainHeader::Unknown);

		void clear();

		// public methods
		bool load(void);
		bool save(void) const;

		// the number of bytes the data of that header requires on disk
		int rawDataSize() const;

		// runtime type information methods
		int rtti() const;

		// clone methods
		bool convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader* pHead2 = NULL);
		CMedIOHeader* clone() const;

		// accessor Methods
		const char* original_File_Name(void) const;
		short sw_Version(void) const;
		short system_Type(void) const;
		File_Type file_Type(void)	const;
		short num_Planes(void) const;
		short num_Frames(void) const;
		short num_Gates(void) const;
		short num_Bed_Pos(void) const;

		// mutator methods
		void setOriginal_File_Name(const char* name);
		void setSW_Version(const short ver);
		void setSystem_Type(const short type);
		void setFileType(const File_Type fType);
		void setNum_Planes(const short num);
		void setNum_Frames(const short num);
		void setNum_Gates(const short num);
		void setNum_Bed_Pos(const short num);

	private:
		struct ECAT6MainHeader
		{
			char		Original_File_Name[20];
			quint16	SW_Version;
			quint16	Data_Type;
      quint16	System_Type;
			quint16	File_Type;
			char		Node_ID[10];
			quint16	Scan_Start_Day;
      quint16	Scan_Start_Month;
      quint16	Scan_Start_Year;
			quint16	Scan_Start_Hour;
			quint16	Scan_Start_Minute;
			quint16	Scan_Start_Second;
      char		Isotope_Code[8];
      float		Isotope_Halflife;
      char		Radiopharmaceutical[32];
      float		Gantry_Tilt;
			float		Gantry_Rotation;
			float		Bed_Elevation;
      quint16	Rot_Source_Speed;
			quint16	Wobble_Speed;
      quint16	Transm_Source_Type;
      float		Axial_FOV;
			float		Transaxial_FOV;
      quint16	Transaxial_Samp_Mode;
			quint16	Coin_Samp_Mode;
      quint16	Axial_Samp_Mode;
      float		Calibration_Factor;
      quint16	Calibration_Units;
			quint16	Compression_Code;
      char		Study_Name[12];
			char		Patient_ID[16];
			char		Patient_Name[32];
      char		Patient_Sex[1];
      char		Patient_Age[10];
      char		Patient_Height[10];
      char		Patient_Weight[10];
      char		Patient_Dexterity[1];
      char		Physician_Name[32];
      char		Operator_Name[32];
      char		Study_Description[32];
      quint16	Acquisition_Type;
			quint16	Bed_Type;
			quint16	Septa_Type;
      char		Facility_Name[20];
      quint16	Num_Planes;
      quint16	Num_Frames;
      quint16	Num_Gates;
      quint16	Num_Bed_Pos;
      float		Init_Bed_Position;
			float		Bed_Offset[15];
      float		Plane_Separation;
      quint16	Lwr_Sctr_Thres;
      quint16	Lwr_True_Thres;
      quint16	Upr_True_Thres;
      float		Collimator;
      char		User_Process_Code[10];
      quint16	Acquisition_Mode;
			quint16	CTI_Reserved[33];
		} m_Data;
};

#endif // CECAT6MAINHEADER_H
