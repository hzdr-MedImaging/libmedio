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

#ifndef CECAT7MAINHEADER_H
#define CECAT7MAINHEADER_H

#include "CECATMainHeader.h"

#include <qdatastream.h>

// forward declarations
class CECATFile;

class Q_EXPORT CECAT7MainHeader : public CECATMainHeader
{
	public:
		// possible ECAT7 file types
		enum File_Type { Unknown=0, Sinogram, Image16, AttenuationCorr,
										 Normalization, PolarMap, Volume8, Volume16,
										 Projection8, Projection16, Image8, Sinogram3D_16,
										 Sinogram3D_8, Normalization_3D, Sinogram3D_Float
									 };

		enum Transm_Source_Type				{ SRC_NONE=0, SRC_RRING, SRC_RING, SRC_ROD,
																		SRC_RROD };
		enum Angular_Compression			{ No_Mash=0, Mash2, Mash4 };
		enum Coin_Samp_Mode						{	NetTrues=0, PromptsAndDelayed,
																		PromptsDelayedMultiples=3 };
		enum Axial_Samp_Mode					{	Normal=0, X2, X3 };
		enum Calibration_Units				{	Uncalibrated=0, Calibrated };
		enum Calibration_Units_Label	{	Blood_Flow=0, LMRGLU };
		enum Compression_Code					{	Comp_None=0 };
		enum Patient_Sex							{ Sex_Male='M', Sex_Female='F', Sex_Unknown='U' };
		enum Patient_Dexterity				{ Dext_RT='R', Dext_LF='L', Dext_Unknown='U' };
		enum Acquisition_Type					{	Undefined=0, Blank, Transmission,
																		StaticEmission, DynamicEmission,
																		GatedEmission, TransmissionRectilinear,
																		EmissionRectilinear };
		enum Acquisition_Mode					{	AcqNormal=0, Windowed, WindowedAndNonWindowed,
																		DualEnergy, UpperEnergy, EmissionTransmission };
		enum Septa_State							{ Extended=0, Retracted };
				
		// constructors
		CECAT7MainHeader(const CECAT7MainHeader& mh);
		CECAT7MainHeader(CECATMainHeader::Type fileType =
										 CECATMainHeader::Unknown);

		// file i/o Methods
		bool load(CECATFile* pFile);
		bool load(QTextStream& stream);
		bool save(CECATFile *pFile);
		bool save(QTextStream& stream);

		// methods to copy data
		void setStudyData(const CECAT7MainHeader& mh);

		// accessor Methods
		const char* magic_Number(void) const
		{ return m_Data.Magic_Number;	}
		
		const char* original_File_Name(void) const
		{ return m_Data.Original_File_Name;	}
		
		short sw_Version(void) const
		{ return m_Data.SW_Version;	}

		short system_Type(void) const
		{ return m_Data.System_Type; }

		File_Type file_Type(void)	const
		{ return static_cast<File_Type>(m_Data.File_Type); }
		
		const char* serial_Number(void) const
		{ return m_Data.Serial_Number; }

		unsigned int scan_Start_Time(void) const
		{ return m_Data.Scan_Start_Time; }
		
		const char* isotope_Name(void) const
		{ return m_Data.Isotope_Name;	}
		
		float isotope_Halflife(void) const
		{ return m_Data.Isotope_Halflife; }
		
		const char* radiopharmaceutical(void) const
		{ return m_Data.Radiopharmaceutical; }
		
		float gantry_Tilt(void) const
		{ return m_Data.Gantry_Tilt; }
		
		float gantry_Rotation(void) const
		{ return m_Data.Gantry_Rotation; }
		
		float bed_Elevation(void) const
		{ return m_Data.Bed_Elevation; }
		
		float intrinsic_Tilt(void) const
		{ return m_Data.Intrinsic_Tilt; }

		short wobble_Speed(void) const
		{ return m_Data.Wobble_Speed; }
		
		Transm_Source_Type transm_Source_Type(void) const
		{ return static_cast<Transm_Source_Type>(m_Data.Transm_Source_Type); }
		
		float distance_Scanned(void) const													
		{ return m_Data.Distance_Scanned;	}
		
		float transaxial_FOV(void) const														
		{ return m_Data.Transaxial_FOV;	}
		
		Angular_Compression angular_Compression(void) const					
		{ return static_cast<Angular_Compression>(m_Data.Angular_Compression); }
		
		Coin_Samp_Mode coin_Samp_Mode(void) const										
		{ return static_cast<Coin_Samp_Mode>(m_Data.Coin_Samp_Mode); }
		
		Axial_Samp_Mode axial_Samp_Mode(void) const									
		{ return static_cast<Axial_Samp_Mode>(m_Data.Axial_Samp_Mode); }
		
		float calibration_Factor(void) const												
		{ return m_Data.Calibration_Factor;	}
		
		Calibration_Units calibration_Units(void) const							
		{ return static_cast<Calibration_Units>(m_Data.Calibration_Units); }
		
		Calibration_Units_Label calibration_Units_Label(void) const	
		{ return static_cast<Calibration_Units_Label>(m_Data.Calibration_Units_Label); }
		
		Compression_Code compression_Code(void) const							
		{ return static_cast<Compression_Code>(m_Data.Compression_Code); }
		
		const char* study_Type(void) const												
		{ return m_Data.Study_Type; }
		
		const char* patient_ID(void) const													
		{ return m_Data.Patient_ID;	}
		
		const char* patient_Name(void) const												
		{ return m_Data.Patient_Name;	}
		
		Patient_Sex patient_Sex(void) const													
		{ return static_cast<Patient_Sex>(m_Data.Patient_Sex[0]);	}
		
		Patient_Dexterity patient_Dexterity(void) const							
		{ return static_cast<Patient_Dexterity>(m_Data.Patient_Dexterity[0]);	}
		
		float patient_Age(void) const																
		{ return m_Data.Patient_Age; }
		
		float patient_Height(void) const														
		{ return m_Data.Patient_Height;	}
		
		float patient_Weight(void) const														
		{ return m_Data.Patient_Weight;	}
		
		unsigned int patient_Birth_Date(void) const								
		{ return m_Data.Patient_Birth_Date;	}
		
		const char* physician_Name(void) const										
		{ return m_Data.Physician_Name;	}
		
		const char* operator_Name(void) const												
		{ return m_Data.Operator_Name; }
		
		const char* study_Description(void) const										
		{ return m_Data.Study_Description; }
		
		Acquisition_Type acquisition_Type(void) const							
		{ return static_cast<Acquisition_Type>(m_Data.Acquisition_Type); }
		
		short patient_Orientation(void) const											
		{ return m_Data.Patient_Orientation; }
		
		const char* facility_Name(void) const											
		{ return m_Data.Facility_Name; }
		
		short num_Planes(void) const															
		{ return m_Data.Num_Planes; }
		
		short num_Frames(void) const															
		{ return m_Data.Num_Frames; }
		
		short num_Gates(void) const																
		{ return m_Data.Num_Gates; }
		
		short num_Bed_Pos(void) const															
		{ return m_Data.Num_Bed_Pos; }
		
		float init_Bed_Position(void) const												
		{ return m_Data.Init_Bed_Position; }
		
		float bed_Offset(const short i) const											
		{ return m_Data.Bed_Offset[i]; }
		
		float plane_Separation(void) const												
		{ return m_Data.Plane_Separation; }
		
		short lwr_Sctr_Thres(void) const													
		{ return m_Data.Lwr_Sctr_Thres; }
		
		short lwr_True_Thres(void) const													
		{ return m_Data.Lwr_True_Thres;	}
		
		short upr_True_Thres(void) const													
		{ return m_Data.Upr_True_Thres;	}
		
		const char* user_Process_Code(void) const									
		{ return m_Data.User_Process_Code; }
		
		Acquisition_Mode acquisition_Mode(void) const							
		{ return static_cast<Acquisition_Mode>(m_Data.Acquisition_Mode); }
		
		float bin_Size(void) const																
		{ return m_Data.Bin_Size;	 }
		
		float branching_Fraction(void) const											
		{ return m_Data.Branching_Fraction; }
		
		unsigned int dose_Start_Time(void) const									
		{ return m_Data.Dose_Start_Time; }
		
		float dosage(void) const																	
		{ return m_Data.Dosage; }
		
		float well_Counter_Corr_Factor(void) const								
		{ return m_Data.Well_Counter_Corr_Factor;	}
		
		const char* data_Units(void) const												
		{ return m_Data.Data_Units; }
		
		Septa_State septa_State(void) const												
		{ return static_cast<Septa_State>(m_Data.Septa_State); }
		
		short cti_Reserved(const short i) const										
		{ return m_Data.CTI_Reserved[i]; }

		
		// mutator methods
		void setMagic_Number(const char* mn)
		{ strncpy(m_Data.Magic_Number, mn, 14); }
		
		void setOriginal_File_Name(const char* name)
		{ strncpy(m_Data.Original_File_Name, name, 32);	}
		
		void setSW_Version(const short ver)
		{ m_Data.SW_Version = ver; }
		
		void setSystem_Type(const short type)
		{ m_Data.System_Type = type; }
		
		void setFileType(CECATMainHeader::Type fType);
		void setFileType(const File_Type fType)
		{ m_Data.File_Type = fType;	updateMagicNumber(); }
		
		void setSerial_Number(const char* num)
		{ strncpy(m_Data.Serial_Number, num, 10); }
		
		void setScan_Start_Time(const unsigned int time)
		{ m_Data.Scan_Start_Time = time; }
		
		void setIsotope_Name(const char* name)
		{ strncpy(m_Data.Isotope_Name, name, 8); }
		
		void setIsotope_Halflife(const float hlf)															
		{ m_Data.Isotope_Halflife = hlf; }
		
		void setRadiopharmaceutical(const char* str)													
		{ strncpy(m_Data.Radiopharmaceutical, str, 32); }
		
		void setGantry_Tilt(const float tilt)																	
		{ m_Data.Gantry_Tilt = tilt; }
		
		void setGantry_Rotation(const float rot)															
		{ m_Data.Gantry_Rotation = rot;	}
		
		void setBed_Elevation(const float elev)																
		{ m_Data.Bed_Elevation = elev; }
		
		void setIntrinsic_Tilt(const float tilt)															
		{ m_Data.Intrinsic_Tilt = tilt;	}
		
		void setWobble_Speed(const short speed)															
		{ m_Data.Wobble_Speed = speed; }
		
		void setTransm_Source_Type(const Transm_Source_Type type)						
		{ m_Data.Transm_Source_Type = type; }
		
		void setDistance_Scanned(const float dist)													
		{ m_Data.Distance_Scanned = dist;	}
		
		void setTransaxial_FOV(const float fov)																
		{ m_Data.Transaxial_FOV = fov; }
		
		void setAngular_Compression(const Angular_Compression comp)						
		{ m_Data.Angular_Compression = comp; }
		
		void setCoin_Samp_Mode(const Coin_Samp_Mode mode)
		{ m_Data.Coin_Samp_Mode = mode;	}
		
		void setAxial_Samp_Mode(const Axial_Samp_Mode mode)										
		{ m_Data.Axial_Samp_Mode = mode; }
		
		void setCalibration_Factor(const float factor)												
		{ m_Data.Calibration_Factor = factor; }
		
		void setCalibration_Units(const Calibration_Units unit)								
		{ m_Data.Calibration_Units = unit;	}
		
		void setCalibration_Units_Label(const Calibration_Units_Label label)
		{ m_Data.Calibration_Units_Label = label;	}
		
		void setCompression_Code(const Compression_Code code)	
		{ m_Data.Compression_Code = code;	}
		
		void setStudy_Type(const char* type)																	
		{ strncpy(m_Data.Study_Type, type, 12);	}
		
		void setPatient_ID(const char* id)																	
		{ strncpy(m_Data.Patient_ID, id, 16);	}
		
		void setPatient_Name(const char* name)
		{ strncpy(m_Data.Patient_Name, name, 32);	}
		
		void setPatient_Sex(const Patient_Sex sex)														
		{ m_Data.Patient_Sex[0] = sex; }
		
		void setPatient_Dexterity(const Patient_Dexterity dex)
		{ m_Data.Patient_Dexterity[0] = dex; }
		
		void setPatient_Age(const float age)																
		{ m_Data.Patient_Age = age; }
		
		void setPatient_Height(const float height)														
		{ m_Data.Patient_Height = height;	}
		
		void setPatient_Weight(const float weight)													
		{ m_Data.Patient_Weight = weight;	}
		
		void setPatient_Birth_Date(const unsigned int date)										
		{ m_Data.Patient_Birth_Date = date;	}
		
		void setPhysician_Name(const char* name)															
		{ strncpy(m_Data.Physician_Name, name, 32);	}
		
		void setOperator_Name(const char* name)															
		{ strncpy(m_Data.Operator_Name, name, 32); }
		
		void setStudy_Description(const char* descr)												
		{ strncpy(m_Data.Study_Description, descr, 32);	}
		
		void setAcquisition_Type(const Acquisition_Type type)								
		{ m_Data.Acquisition_Type = type; }
		
		void setPatient_Orientation(const short orient)											
		{ m_Data.Patient_Orientation = orient; }
		
		void setFacility_Name(const char* name)															
		{ strncpy(m_Data.Facility_Name, name, 20); }
		
		void setNum_Planes(const short num)																	
		{ m_Data.Num_Planes = num; }
		
		void setNum_Frames(const short num)																	
		{ m_Data.Num_Frames = num; }
		
		void setNum_Gates(const short num)																	
		{ m_Data.Num_Gates = num; }
		
		void setNum_Bed_Pos(const short num)																
		{ m_Data.Num_Bed_Pos = num; }
		
		void setInit_Bed_Position(const float pos)													
		{ m_Data.Init_Bed_Position = pos; }
		
		void setBed_Offset(const short i, const float offset)								
		{ m_Data.Bed_Offset[i] = offset; }
		
		void setPlane_Separation(const float sep)														
		{ m_Data.Plane_Separation = sep; }
		
		void setLwr_Sctr_Thres(const short thres)														
		{ m_Data.Lwr_Sctr_Thres = thres; }
		
		void setLwr_True_Thres(const short thres)														
		{ m_Data.Lwr_True_Thres = thres; }
		
		void setUpr_True_Thres(const short thres)														
		{ m_Data.Upr_True_Thres = thres; }
		
		void setUser_Process_Code(const char* code)													
		{ strncpy(m_Data.User_Process_Code, code, 10); }
		
		void setAcquisition_Mode(const Acquisition_Mode mode)								
		{ m_Data.Acquisition_Mode = mode; }
		
		void setBin_Size(const float size)																	
		{ m_Data.Bin_Size = size; }
		
		void setBranching_Fraction(const float frac)												
		{ m_Data.Branching_Fraction = frac; }
		
		void setDose_Start_Time(const unsigned int time)										
		{ m_Data.Dose_Start_Time = time; }
		
		void setDosage(const float dosage)																	
		{ m_Data.Dosage = dosage; }
		
		void setWell_Counter_Corr_Factor(const float fac)										
		{ m_Data.Well_Counter_Corr_Factor = fac; }
		
		void setData_Units(const char* units)																
		{ strncpy(m_Data.Data_Units, units, 32); }
		
		void setSepta_State(const Septa_State state)												
		{ m_Data.Septa_State = state; }
		
		void setCTI_Reserved(const short i, const short val)								
		{ m_Data.CTI_Reserved[i] = val; }

	protected:
		void updateMagicNumber(void);

	private:
		#pragma pack(2)	// set the structure alignment
		// MainHeader structure (should be 512bytes)
		struct ECAT7MainHeader
		{
			char					Magic_Number[14];
			char					Original_File_Name[32];
			Q_UINT16			SW_Version;
			Q_UINT16			System_Type;
			Q_UINT16			File_Type;
			char					Serial_Number[10];
			Q_UINT32			Scan_Start_Time;
			char					Isotope_Name[8];
			float					Isotope_Halflife;
			char					Radiopharmaceutical[32];
      float					Gantry_Tilt;
      float					Gantry_Rotation;
      float					Bed_Elevation;
      float					Intrinsic_Tilt;
      Q_UINT16			Wobble_Speed;
      Q_UINT16			Transm_Source_Type;
			float					Distance_Scanned;
      float					Transaxial_FOV;
      Q_UINT16			Angular_Compression;
      Q_UINT16			Coin_Samp_Mode;
      Q_UINT16			Axial_Samp_Mode;
      float					Calibration_Factor;
      Q_UINT16			Calibration_Units;
      Q_UINT16			Calibration_Units_Label;
      Q_UINT16			Compression_Code;
      char					Study_Type[12];
			char					Patient_ID[16];
      char					Patient_Name[32];
      char					Patient_Sex[1];
      char					Patient_Dexterity[1];
      float					Patient_Age;
      float					Patient_Height;
      float					Patient_Weight;
      Q_UINT32			Patient_Birth_Date;
      char					Physician_Name[32];
      char					Operator_Name[32];
      char					Study_Description[32];
      Q_UINT16			Acquisition_Type;
      Q_UINT16			Patient_Orientation;
			char					Facility_Name[20];
      Q_UINT16			Num_Planes;
      Q_UINT16			Num_Frames;
      Q_UINT16			Num_Gates;
      Q_UINT16			Num_Bed_Pos;
      float					Init_Bed_Position;
      float					Bed_Offset[15];
      float					Plane_Separation;
      Q_UINT16			Lwr_Sctr_Thres;
      Q_UINT16			Lwr_True_Thres;
      Q_UINT16			Upr_True_Thres;
      char					User_Process_Code[10];
      Q_UINT16			Acquisition_Mode;
      float					Bin_Size;
      float					Branching_Fraction;
      Q_UINT32			Dose_Start_Time;
			float					Dosage;
      float					Well_Counter_Corr_Factor;
      char					Data_Units[32];
      Q_UINT16			Septa_State;
			Q_UINT16			CTI_Reserved[6];
		} m_Data;
		#pragma pack()
};

#endif // CECAT7MAINHEADER_H
