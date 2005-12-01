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

//! @file CConcordeMainHeader.h
//! @author Hagen Moelle
//! @date 09/10/2005
//! @brief contains definition of the class CConcordeMainHeader

#ifndef CCONCORDEMAINHEADER_H
#define CCONCORDEMAINHEADER_H

#ifndef __MEDIO_PRIVATE__
#include <CMedIOHeader>
#else
#include <CMedIOHeader.h>
#include <CKeyParser.h>
#include <CIntVector.h>
#include <CDate.h>
#endif

// forward declarations
class CConcordeFile;

class CConcordeMainHeader : public CMedIOHeader
{
	public :
	//contructors
		//! @brief constructor
		//! @param File: complete path to file holding header
		CConcordeMainHeader(QString File);

		//! @brief default constructor
		CConcordeMainHeader(CConcordeFile* file = NULL);

	//destructor
		//! @brief destructor
		~CConcordeMainHeader();	
	//members
		enum FileType{UnknownFile = 0, ListMode, Sinogram, Normalization, Attenuation,
				Image, Blank, MuMap = 8, Scatter};
		enum AquisitionMode{UnknownAquisition = 0, BlankAquisition, Emission, Dynamic, Gated,
					ContinuousBedMotion, SinglesTransmission,
					WindowedCoincidenceTransmission,
					NonWindowedCoincidenceTransmission};
		enum BedMotion{Static = 0, Continuous};
		enum SourceType{UnknownSource = 0, Point, Line};
		enum DataType{UnknownDataType = 0, Byte, IntelShort, IntelInt, IntelFloat,
				IEEEFloat, SunShort, SunInt};
		enum DataOrder{ViewMode = 0, SinogramMode};
		enum DeadTimeCorrectionApplied{NoDeadTimeCorrection = 0, Global, CMS};
		enum DecayCorrectionApplied{NoDecayCorrection = 0, 
						DecayCorrection = 1};
		enum NormalizationApplied{NoNormalization = 0, PointSourceInversion, 
						PointSourceComponentBased, 
						CylinderSourceInversion,
						CylinderSourceComponentBased};
		enum AttenuationApplied{NoAttenuation = 0, PointSourceWindowedTX, PointSourceSingles,
					SegmentedPointSourceTX, SegmentedPointSourceSingles,
					Geometry, NonPositronSource, PointSourceNonWindowedTX};
		enum ScatterCorrection{NoScatterCorrection = 0, FitOfEmissionTail, MonteCarlo, 
					DirectCalculation, ModelBased, TXOffWindow};
		enum ArcCorrectionApplied{NoArcCorrection = 0, ArcCorrection = 1};
		enum CalibrationUnits{UnknownCalibrationUnits = 0, nCiPerCC, BqPerCC};
		enum DoseUnits{UnknownDoseUnits = 0, mCi, MBq};
		enum SubjectOrientation{UnknownSubjectOrientation = 0, FeetFirstProne, HeadFirstProne,
					FeetFirstSupine, HeadFirstSupine, FeetFirstRight,
					HeadFirstRight, FeetFirstLeft, HeadFirstLeft};
		enum SubjectLengthUnits{UnknownSubjectLengthUnits = 0, Millimeters, Centimeters, Inches};
		enum SubjectWeightUnits{UnknownSubjectWeightUnits = 0, Grams, Ounces, Kilograms, Pounds};
	//methods
		//! @brief set default values in header
		void clear();
		
		//CMedIOData* fileObject() const;
		bool load();
		bool load(QString File);
		bool save() const;
		CMedIOHeader::Format headerFormat() const;
		//int rtti();
		
		//! @return framesize of a sinogram in bytes
		virtual unsigned int frameSize() = 0;

		virtual CMedIOHeader* clone() const = 0;
		bool convertFrom(const CMedIOHeader* srcMainHeader, const CMedIOHeader* srcSubHeader = NULL);

		
		//! @return framesize of a imagevolume in bytes
		//unsigned int getImageFrameSize();
		
		//accessor methods
		//! @brief access frames starting with frame 1 as first frame -> i = 1
		//! @return frame specific header
		//CHeaderConcordeFrame* frame(int i);
		
		int model(void) const;
		QString institution(void) const;
		QString study(void) const;
		QString fileName(void) const;
		FileType fileType(void) const;
		AquisitionMode acquisitionMode(void) const; 
		BedMotion bedMotion(void) const;
		int totalFrames(void) const;
		
		QString isotope(void) const;
		float isotopeHalfTime(void) const;
		float isotopeBranchingFraction(void) const;
		
		int transaxialCrystalsPerBlock(void) const;
		int axialCrystalsPerBlock(void) const;
		int intrinsicCrystalOffset(void) const;
		int axialBlocks(void) const;
		float axialCrystalPitch(void) const;
		float radius(void) const;
		float radialFov(void) const;
		float srcRadius(void) const;
		float srcCmPerRev(void) const;
		SourceType txSrcType(void) const;
		float transaxialBinSize(void) const;
		float axialPlaneSize(void) const;
		float lld(void) const;
		float uld(void) const;
		
		DataType dataType(void) const;
		DataOrder dataOrder(void) const;
		int span(void) const;
		int ringDifference(void) const;
		int numberOfDimensions(void) const;
		int xDimension(void) const;
		int yDimension(void) const;
		int zDimension(void) const;
		int wDimension(void) const;
		int deltaElements(int i) const;
		
		DeadTimeCorrectionApplied deadtimeCorrectionApplied(void) const;
		DecayCorrectionApplied decayCorrectionApplied(void) const;
		NormalizationApplied normalizationApplied(void) const;
		AttenuationApplied attenuationApplied(void) const;
		ScatterCorrection scatterCorrection(void) const;
		ArcCorrectionApplied arcCorrectionApplied(void) const;
		
		float pixelSize(void) const;

		CalibrationUnits calibrationUnits(void) const; 
		float calibrationFactor(void) const;
		float calibrationBranchingFraction(void) const;
		int numberOfSinglesRates(void) const;
		
		QString investigatorName(void) const;
		QString operatorName(void) const;
		QString studyIdentifier(void) const;
		long scanTime(void) const;
		QString injectedCompound(void) const;
		DoseUnits doseUnits(void) const;
		float dose(void) const;
		long injectionTime(void) const;
		float injectionDecayCorrection(void) const;
		
		
		QString subjectIdentifier(void) const;
		QString subjectGenus(void) const;
		SubjectOrientation subjectOrientation(void) const;
		SubjectLengthUnits subjectLengthUnits(void) const;
		float subjectLength(void) const;
		SubjectWeightUnits subjectWeightUnits(void) const;
		float subjectWeight(void) const;
		QString subjectPhenotype(void) const;
		QString studyModel(void) const;
		
		QString anesthesia(void) const;
		QString analgesia(void) const;
		QString otherDrugs(void) const;
		QString foodAccess(void) const;
		QString waterAccess(void) const;

		QDateTime scanTimeQt(void) const;
		QDateTime injectionTimeQt(void) const;
		QString strSubjectWeightUnits(void) const;
		QString strSubjectLengthUnits(void) const;
		QString strDoseUnits(void) const;

		//mutator methods
		void setModel(const int value);
		void setInstitution(const QString value);
		void setFileName(const QString value);
		void setFileType(const int value);
		void setAcquisitionMode(const int value);
		void setBedMotion(const int value);
		void setNumFrames(const int value);
		
		void setIsotope(const QString value);
		void setIsotopeHalftime(const float value);
		void setIsotopeBranchingFraction(const float value);
		
		void setTransaxialCrystalsPerBlock(const int value);
		void setAxialCrystalsPerBlock(const int value);
		void setIntrinsicCrystalOffset(const int value);
		void setAxialBlocks(const int value);
		void setAxialCrystalPitch(const float value);
		void setRadius(const float value);
		void setRadialFov(const float value);
		void setSrcRadius(const float value);
		void setSrcCmPerRev(const float value);
		void setTxSrcType(const int value);
		void setTransaxialBinSize(const float value);
		void setAxialPlaneSize(const float value);
		void setLld(const float value);
		void setUld(const float value);
		
		void setDataType(const int value);
		void setDataOrder(const int value);
		void setSpan(const int value);
		void setRingDifference(const int value);
		void setNumberOfDimensions(const int value);
		void setXDimension(const int value);
		void setYDimension(const int value);
		void setZDimension(const int value);
		void setWDimension(const int value);
		void setDeltaElements(const int i, const int value);
		
		void setDeadtimeCorrectionApplied(const int value);
		void setDecayCorrectionApplied(const int value);
		void setNormalizationApplied(const int value);
		void setAttenuationApplied(const int value);
		void setScatterCorrection(const int value);
		void setArcCorrectionApplied(const int value);

		void setPixelSize(const float value);
		
		void setCalibrationUnits(const int value);
		void setCalibrationFactor(const float value);
		void setCalibrationBranchingFraction(const float value);
		void setNumberOfSinglesRates(const int value);
		
		void setInvestigatorName(const QString value);
		void setOperatorName(const QString value);
		void setStudyIdentifier(const QString value);
		void setScanTime(const long value);
		void setInjectedCompound(const QString value);
		void setDoseUnits(const int value);
		void setDose(const float value);
		void setInjectionTime(const long value);
		void setInjectionDecayCorrection(const float value);
		
		void setSubjectIdentifier(const QString value);
		void setSubjectGenus(const QString value);
		void setSubjectOrientation(const int value);
		void setSubjectLength_units(const int value);
		void setSubjectLength(const float value);
		void setSubjectWeight_units(const int value);
		void setSubjectWeight(const float value);
		void setSubjectPhenotype(const QString value);
		void setStudyModel(const QString value);
		
		void setAnesthesia(const QString value);
		void setAnalgesia(const QString value);
		void setOtherDrugs(const QString value);
		void setFoodAccess(const QString value);
		void setWaterAccess(const QString value);

	protected:
		bool copyData(const CMedIOHeader* src);	
	
#ifdef __MEDIO_PRIVATE__
	//members
		//QList<CHeaderConcordeFrame*> frames;
		CKeyParser Parser;
		typedef struct //Concorde Micropet Header <--> ECAT7 Header
    		{
			int		model;			//System_Type;
			QString		institution;		//Facility_Name;
			QString		study;			//Study type/description (string)
			QString		file_name;		//Original_File_Name;
			int		file_type;		//File_Type;
			int		acquisition_mode;	//Acquisition_Type;
			int		bed_motion;		//0 static/unknown : 1 continuous motion
			int		total_frames;
			QString		isotope;		//Isotope_Name;
			float		isotope_half_time; 	//Isotope_Halflife;
			float		isotope_branching_fraction;	//Branching_Fraction;
			int		transaxial_crystals_per_block;
			int		axial_crystals_per_block;
			int		intrinsic_crystal_offset;
			int		transaxial_blocks;
			int		axial_blocks;
			float		transaxial_crystal_pitch;
			float		axial_crystal_pitch;
			float		radius;
			float		radial_fov;		//Distance_Scanned Transaxial_FOV;
			float		src_radius;
			float		src_cm_per_rev;
			int		tx_src_type;
			float		transaxial_bin_size;	//Bin_Size; X_Resolution;
			float		axial_plane_size;	//Z_Resolution;
			float		lld;			//Lwr_True_Thres;
			float		uld;			//Upr_True_Thres;
			int		data_type;		//Data_Type;
			int		data_order;		//0 Segment|Axis|View|Ring_Diff
								//1 Segment|View|Axis|Ring_Diff
			int		span;			//Axial_Compression;
			int		ring_difference;	//Ring_Difference;
			int		number_of_dimensions;	//Num_Dimensions;
			int		x_dimension;		//Num_R_Elements;
			int		y_dimension;		//Num_Angles;
			int		z_dimension;
			int		w_dimension;
			CIntVector	delta_elements;	//Num_Z_Elements[64];
			int		deadtime_correction_applied;
			int		decay_correction_applied;
			int		normalization_applied;
			int		attenuation_applied;
			int		scatter_correction;
			int		arc_correction_applied;		//0 none : != 0 true
			float		x_offset;
			float		y_offset;
			float		zoom;
			float		pixel_size;
			int		calibration_units;
			float		calibration_factor;
			float		calibration_branching_fraction;
			int		number_of_singles_rates;
			QString		investigator;
			QString		Operator;
			QString		study_identifier;
			CDate		scan_time;		//Scan_Start_Time;
			QString		injected_compound;
			int		dose_units;
			float		dose;
			CDate		injection_time;
			float		injection_decay_correction;
			QString		subject_identifier;
			QString		subject_genus;
			int		subject_orientation;	//Patient_Orientation;
			int		subject_length_units;
			float		subject_length;
			int		subject_weight_units;
			float		subject_weight;
			QString		subject_phenotype;
			QString		study_model;
			QString		anesthesia;
			QString		analgesia;
			QString		other_drugs;
			QString		food_access;
			QString		water_access;
    		} ConcordeHeader;
		ConcordeHeader m_Data;
	//methods
		bool init();
#endif
};

#endif
