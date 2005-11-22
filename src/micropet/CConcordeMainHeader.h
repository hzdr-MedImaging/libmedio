//! @file CConcordeMainHeader.h
//! @author Hagen Moelle
//! @date 09/10/2005
//! @brief contains definition of the class CConcordeMainHeader

#ifndef CCONCORDEMAINHEADER_H
#define CCONCORDEMAINHEADER_H

#include "CMedIOHeader.h"
#include "CKeyParser.h"
#include "CIntVector.h"
#include "CConcordeFile.h"
#include "CDate.h"

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
		enum CalibrationUnits{UnknownCalibrationUnits = 0, nCi, Bq};
		enum DoseUnits{UnknownDoseUnits = 0, mCi, MBq};
		enum SubjectOrientation{UnknownSubjectOrientation = 0, FeetFirstProne, HeadFirstProne,
					FeetFirstSupine, HeadFirstSupine, FeetFirstRight,
					HeadFirstRight, FeetFirstLeft, HeadFirstLeft};
		enum SubjectLengthUnits{UnknownSubjectLengthUnits = 0, Millimeters, Centimeters, Inches};
		enum SubjectWeightUnits{UnknownSubjectWeightUnits = 0, Grams, Ounces, Kilograms, Pounds};
	//methods
		//! @brief set default values in header
		void setDefaults();
		
		CMedIOData* fileObject() {return m_pMedIOData;} 
		bool load();
		bool load(QString File);
		bool save() const;
		CMedIOHeader::Format headerFormat() const;
		//int rtti();
		
		//! @return framesize of a sinogram in bytes
		virtual unsigned int frameSize() = 0;
		
		//! @return framesize of a imagevolume in bytes
		//unsigned int getImageFrameSize();
		
		//accessor methods
		//! @brief access frames starting with frame 1 as first frame -> i = 1
		//! @return frame specific header
		//CHeaderConcordeFrame* frame(int i);
		
		int model(void) const {return m_Data.model;}
		QString institution(void) const {return m_Data.institution;}
		QString study(void) const {return m_Data.study;}
		QString fileName(void) const {return m_Data.file_name;}
		int fileType(void) const {return m_Data.file_type;}
		int acquisitionMode(void) const {return m_Data.acquisition_mode;}
		int bedMotion(void) const {return m_Data.bed_motion;}
		int totalFrames(void) const {return m_Data.total_frames;}
		
		QString isotope(void) const {return m_Data.isotope;}
		float isotopeHalfTime(void) const {return m_Data.isotope_half_time;}
		float isotopeBranchingFraction(void) const {return m_Data.isotope_branching_fraction;}
		
		int transaxialCrystalsPerBlock(void) const {return m_Data.transaxial_crystals_per_block;}
		int axialCrystalsPerBlock(void) const {return m_Data.axial_crystals_per_block;}
		int intrinsicCrystalOffset(void) const {return m_Data.intrinsic_crystal_offset;}
		int axialBlocks(void) const {return m_Data.axial_blocks;}
		float axialCrystalPitch(void) const {return m_Data.axial_crystal_pitch;}
		float radius(void) const {return m_Data.radius;}
		float radialFov(void) const {return m_Data.radial_fov;}
		float srcRadius(void) const {return m_Data.src_radius;}
		float srcCmPerRev(void) const {return m_Data.src_cm_per_rev;}
		int txSrcType(void) const {return m_Data.tx_src_type;}
		float transaxialBinSize(void) const {return m_Data.transaxial_bin_size;}
		float axialPlaneSize(void) const {return m_Data.axial_plane_size;}
		float lld(void) const {return m_Data.lld;}
		float uld(void) const {return m_Data.uld;}
		
		int dataType(void) const {return m_Data.data_type;}
		int dataOrder(void) const {return m_Data.data_order;}
		int span(void) const {return m_Data.span;}
		int ringDifference(void) const {return m_Data.ring_difference;}		
		int numberOfDimensions(void) const {return m_Data.number_of_dimensions;}
		int xDimension(void) const {return m_Data.x_dimension;}
		int yDimension(void) const {return m_Data.y_dimension;}
		int zDimension(void) const {return m_Data.z_dimension;}
		int wDimension(void) const {return m_Data.w_dimension;}
		int deltaElements(int i) const {return m_Data.delta_elements[i];}
		
		int deadtimeCorrectionApplied(void) const {return m_Data.deadtime_correction_applied;}
		int decayCorrectionApplied(void) const {return m_Data.decay_correction_applied;}
		int normalizationApplied(void) const {return m_Data.normalization_applied;}
		int attenuationApplied(void) const {return m_Data.attenuation_applied;}
		int scatterCorrection(void) const {return m_Data.scatter_correction;}
		int arcCorrectionApplied(void) const {return m_Data.arc_correction_applied;}
		
		float pixelSize(void) const {return m_Data.pixel_size;}

		float calibrationFactor(void) const {return m_Data.calibration_factor;}
		float calibrationBranchingFraction(void) const {return m_Data.calibration_branching_fraction;}
		int numberOfSinglesRates(void) const {return m_Data.number_of_singles_rates;}
		
		QString investigatorName(void) const {return m_Data.investigator;}
		QString operatorName(void) const {return m_Data.Operator;}
		QString studyIdentifier(void) const {return m_Data.study_identifier;}
		long scanTime(void) const {return m_Data.scan_time.toTime_t();}
		QString injectedCompound(void) const {return m_Data.injected_compound;}
		int doseUnits(void) const {return m_Data.dose_units;}
		float dose(void) const {return m_Data.dose;}
		long injectionTime(void) const {return m_Data.injection_time.toTime_t();}
		float injectionDecayCorrection(void) const {return m_Data.injection_decay_correction;}
		
		
		QString subjectIdentifier(void) const {return m_Data.subject_identifier;}
		QString subjectGenus(void) const {return m_Data.subject_genus;}
		int subjectOrientation(void) const {return m_Data.subject_orientation;}
		int subjectLengthUnits(void) const {return m_Data.subject_length_units;}
		float subjectLength(void) const {return m_Data.subject_length;}
		int subjectWeightUnits(void) const {return m_Data.subject_weight_units;}
		float subjectWeight(void) const {return m_Data.subject_weight;}
		QString subjectPhenotype(void) const {return m_Data.subject_phenotype;}
		QString studyModel(void) const {return m_Data.study_model;}
		
		QString anesthesia(void) const {return m_Data.anesthesia;}
		QString analgesia(void) const {return m_Data.analgesia;}
		QString otherDrugs(void) const {return m_Data.other_drugs;}
		QString foodAccess(void) const {return m_Data.food_access;}
		QString waterAccess(void) const {return m_Data.water_access;}

		QDateTime scanTimeQt(void) const {return static_cast<QDateTime>(m_Data.scan_time);}
		QDateTime injectionTimeQt(void) const {return static_cast<QDateTime>(m_Data.injection_time);}
		QString strSubjectWeightUnits(void) const
		{
			QString tmp; 
			switch(m_Data.subject_weight_units)
			{
				case Grams: tmp = "g"; break;
				case Ounces: tmp = "oz"; break;
				case Kilograms: tmp = "kg"; break;
				case Pounds: tmp = "pounds"; break;
				default: tmp = "unknown units"; break;
			}
			return tmp;
		}
		QString strSubjectLengthUnits(void) const
		{
			QString tmp; 
			switch(m_Data.subject_length_units)
			{
				case Millimeters: tmp = "mm"; break;
				case Centimeters: tmp = "cm"; break;
				case Inches: tmp = "in"; break;
				default: tmp = "unknown units"; break;
			}
			return tmp;
		}
		QString strDoseUnits(void) const
		{
			QString tmp; 
			switch(m_Data.dose_units)
			{
				case mCi: tmp = "mCi"; break;
				case MBq: tmp = "MBq"; break;
				default: tmp = "unknown units"; break;
			}
			return tmp;
		}

		//mutator methods
		void setModel(const int value) { m_Data.model = value;}
		void setInstitution(const QString value) { m_Data.institution = value;}
		void setFileName(const QString value) { m_Data.file_name = value;}
		void setFileType(const int value) { m_Data.file_type = value;}
		void setAcquisitionMode(const int value) { m_Data.acquisition_mode = value;}
		void setBedMotion(const int value) { m_Data.bed_motion = value;}
		void setNumFrames(const int value) { m_Data.total_frames = value;}
		
		void setIsotope(const QString value) { m_Data.isotope = value;}
		void setIsotopeHalftime(const float value) { m_Data.isotope_half_time = value;}
		void setIsotopeBranchingFraction(const float value) { m_Data.isotope_branching_fraction = value;}
		
		void setTransaxialCrystalsPerBlock(const int value) {m_Data.transaxial_crystals_per_block = value;}
		void setAxialCrystalsPerBlock(const int value) {m_Data.axial_crystals_per_block = value;}
		void setIntrinsicCrystalOffset(const int value) {m_Data.intrinsic_crystal_offset = value;}
		void setAxialBlocks(const int value) {m_Data.axial_blocks = value;}
		void setAxialCrystalPitch(const float value) {m_Data.axial_crystal_pitch = value;}
		void setRadius(const float value) {m_Data.radius = value;}
		void setRadialFov(const float value) {m_Data.radial_fov = value;}
		void setSrcRadius(const float value) {m_Data.src_radius = value;}
		void setSrcCmPerRev(const float value) {m_Data.src_cm_per_rev = value;}
		void setTxSrcType(const int value) {m_Data.tx_src_type = value;}
		void setTransaxialBinSize(const float value) {m_Data.transaxial_bin_size = value;}
		void setAxialPlaneSize(const float value) {m_Data.axial_plane_size = value;}
		void setLld(const float value) {m_Data.lld = value;}
		void setUld(const float value) {m_Data.uld = value;}
		
		void setDataType(const int value) { m_Data.data_type = value;}
		void setDataOrder(const int value){ m_Data.data_order = value;}
		void setSpan(const int value) { m_Data.span = value;}
		void setRingDifference(const int value) { m_Data.ring_difference = value;}
		void setNumberOfDimensions(const int value) { m_Data.number_of_dimensions = value;}
		void setXDimension(const int value) { m_Data.x_dimension = value;}
		void setYDimension(const int value) { m_Data.y_dimension = value;}
		void setZDimension(const int value) { m_Data.z_dimension = value;}
		void setWDimension(const int value) { m_Data.w_dimension = value;}
		void setDeltaElements(const int i, const int value) { m_Data.delta_elements[i] = value;}
		
		void setDeadtimeCorrectionApplied(const int value) { m_Data.deadtime_correction_applied = value;}
		void setDecayCorrectionApplied(const int value) { m_Data.decay_correction_applied = value;}
		void setNormalizationApplied(const int value) { m_Data.normalization_applied = value;}
		void setAttenuationApplied(const int value) { m_Data.attenuation_applied = value;}
		void setScatterCorrection(const int value) { m_Data.scatter_correction = value;}
		void setArcCorrectionApplied(const int value) { m_Data.arc_correction_applied = value;}

		void setPixelSize(const float value) {m_Data.pixel_size = value;}
		
		void setCalibrationFactor(const float value) { m_Data.calibration_factor = value;}
		void setCalibrationBranchingFraction(const float value) { m_Data.calibration_branching_fraction = value;}
		void setNumberOfSinglesRates(const int value) { m_Data.number_of_singles_rates = value;}
		
		void setInvestigatorName(const QString value) { m_Data.investigator = value;}
		void setOperatorName(const QString value) { m_Data.Operator = value;}
		void setStudyIdentifier(const QString value) { m_Data.study_identifier = value;}
		void setScanTime(const long value) { m_Data.scan_time.setTime_t(value);}
		void setInjectedCompound(const QString value) { m_Data.injected_compound = value;}
		void setDoseUnits(const int value) { m_Data.dose_units = value;}
		void setDose(const float value) { m_Data.dose = value;}
		void setInjectionTime(const long value) { m_Data.injection_time.setTime_t(value);}
		void setInjectionDecayCorrection(const float value) { m_Data.injection_decay_correction = value;}
		
		void setSubjectIdentifier(const QString value) { m_Data.subject_identifier = value;}
		void setSubjectGenus(const QString value) { m_Data.subject_genus = value;}
		void setSubjectOrientation(const int value) { m_Data.subject_orientation = value;}
		void setSubjectLength_units(const int value) { m_Data.subject_length_units = value;}
		void setSubjectLength(const float value) { m_Data.subject_length = value;}
		void setSubjectWeight_units(const int value) { m_Data.subject_weight_units = value;}
		void setSubjectWeight(const float value) { m_Data.subject_weight = value;}
		void setSubjectPhenotype(const QString value) { m_Data.subject_phenotype = value;}
		void setStudyModel(const QString value) { m_Data.study_model = value;}
		
		void setAnesthesia(const QString value) { m_Data.anesthesia = value;}
		void setAnalgesia(const QString value) { m_Data.analgesia = value;}
		void setOtherDrugs(const QString value) { m_Data.other_drugs = value;}
		void setFoodAccess(const QString value) { m_Data.food_access = value;}
		void setWaterAccess(const QString value) { m_Data.water_access = value;}

	protected:
		CMedIOHeader& copyData(const CMedIOHeader& src);	
	
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
};

#endif
