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
		
		CECAT6MainHeader(const CECAT6MainHeader& mh);
		CECAT6MainHeader(CECATFile* ecatFile = NULL,
										 CECATMainHeader::Type fileType = 
												CECATMainHeader::Unknown);

		// public methods
		bool load(void);
		bool save(void) const;

		// runtime type information methods
		int rtti() const
		{ return CECATMainHeader::ECAT6MainHeader; }
		
		// accessor Methods
		const char* original_File_Name(void) const
		{ return m_Data.Original_File_Name;	}

		short sw_Version(void) const
		{ return m_Data.SW_Version;	}

		short system_Type(void) const
		{ return m_Data.System_Type; }

		File_Type file_Type(void)	const
		{ return static_cast<File_Type>(m_Data.File_Type); }
		
		short num_Planes(void) const	{ return m_Data.Num_Planes;		}
		short num_Frames(void) const	{ return m_Data.Num_Frames;		}
		short num_Gates(void) const		{	return m_Data.Num_Gates;		}
		short num_Bed_Pos(void) const	{ return m_Data.Num_Bed_Pos;	}

		// mutator methods
		void setOriginal_File_Name(const char* name)
		{ strncpy(m_Data.Original_File_Name, name, 20);	}
		
		void setSW_Version(const short ver)
		{ m_Data.SW_Version = ver; }
		
		void setSystem_Type(const short type)
		{ m_Data.System_Type = type; }
		
		void setFileType(const File_Type fType)
		{ m_Data.File_Type = fType;	} 

		void setNum_Planes(short num)		{ m_Data.Num_Planes = num;		}
		void setNum_Frames(short num)		{ m_Data.Num_Frames = num;		}
		void setNum_Gates(short num)		{ m_Data.Num_Gates = num;			}
		void setNum_Bed_Pos(short num)	{ m_Data.Num_Bed_Pos = num;		}

	protected:
		// required method to copy relevant data from another MedIOHeader object
		CMedIOHeader& copyData(const CMedIOHeader& src);		

	private:
		#pragma pack(2)
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
		#pragma pack()
};

#endif // CECAT6MAINHEADER_H
