//! @file CHeaderConcorde.h
//! @author Hagen Moelle
//! @date 11/13/2004
//! @brief contains definition of the class CHeaderConcorde

#ifndef CHEADERCONCORDE_H
#define CHEADERCONCORDE_H

#include "CMedIOHeader.h"
#include "CConcordeFile.h"
#include "CHeaderConcordeFrame.h"

#include <string>
#include <list>

//! @class CHeaderConcorde
//! @brief class which handles concorde microPET headers
//! @bug no bugs reported
//! @warning still in development
//!
//! This class provides access to all fields of a concorde microPET header.
//! The concorde microPET header is a text header. There are no differences in
//! the header between differt data formats. The only thing to pay attention on
//! is that the names of the keys of the headerfile are changing from version to
//! version. Furthermore the header is seperated into a mainheader which holds all
//! common fields and one subheader for each frame in the data file.
//! 
class CHeaderConcorde : public CMedIOHeader
{
	public :
	//contructors
		////! @brief constructor
		////! @param file: complete path to file holding header
		CHeaderConcorde(QString File);

		//! @brief default constructor
		CHeaderConcorde(CConcordeFile* file = NULL);

	//destructor
		//! @brief destructor
		~CHeaderConcorde();	
	//members
		enum FileType{UnknownFile = 0, ListMode, Sinogram, Normalization, Attenuation,
				Image, Blank, MuMap, Scatter};
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

		bool load();
		bool load(QString File);
		bool save() const;
		CMedIOHeader::Format headerFormat() const;
		int rtti();
		
		//! @return framesize of a sinogram in bytes
		unsigned int getFrameSize();
		
		//! @return framesize of a imagevolume in bytes
		unsigned int getImageFrameSize();
		
		//accessor methods
		//! @brief access frames starting with frame 1 as first frame -> i = 1
		//! @return frame specific header
		CHeaderConcordeFrame* frame(int i);
		
		int model(void) {return m_Data.model;}
		std::string institution(void) {return m_Data.institution;}
		std::string study(void) {return m_Data.study;}
		std::string filename(void) {return m_Data.file_name;}
		int filetype(void) {return m_Data.file_type;}
		int acquisitionmode(void) {return m_Data.acquisition_mode;}
		int bedmotion(void) {return m_Data.bed_motion;}
		int numframes(void) {return m_Data.total_frames;}
		
		std::string isotope(void) {return m_Data.isotope;}
		float isotopehalftime(void) {return m_Data.isotope_half_time;}
		float isotopebranchingfraction(void) {return m_Data.isotope_branching_fraction;}
		
		int transaxialcrystalsperblock(void) {return m_Data.transaxial_crystals_per_block;}
		int axialcrystalsperblock(void) {return m_Data.axial_crystals_per_block;}
		int intrinsiccrystaloffset(void) {return m_Data.intrinsic_crystal_offset;}
		int axialblocks(void) {return m_Data.axial_blocks;}
		float axialcrystalpitch(void) {return m_Data.axial_crystal_pitch;}
		float radius(void) {return m_Data.radius;}
		float radialfov(void) {return m_Data.radial_fov;}
		float srcradius(void) {return m_Data.src_radius;}
		float srccmperrev(void) {return m_Data.src_cm_per_rev;}
		int txsrctype(void) {return m_Data.tx_src_type;}
		float transaxialbinsize(void) {return m_Data.transaxial_bin_size;}
		float axialplanesize(void) {return m_Data.axial_plane_size;}
		float lld(void) {return m_Data.lld;}
		float uld(void) {return m_Data.uld;}
		
		int datatype(void) {return m_Data.data_type;}
		int dataorder(void){return m_Data.data_order;}
		int span(void) {return m_Data.span;}
		int ringdifference(void) {return m_Data.ring_difference;}		
		int numberofdimensions(void) {return m_Data.number_of_dimensions;}
		int xdimension(void) {return m_Data.x_dimension;}
		int ydimension(void) {return m_Data.y_dimension;}
		int zdimension(void) {return m_Data.z_dimension;}
		int wdimension(void) {return m_Data.w_dimension;}
		int deltaelements(int i) {return m_Data.delta_elements[i];}
		
		int deadtimecorrectionapplied(void) {return m_Data.deadtime_correction_applied;}
		int decaycorrectionapplied(void) {return m_Data.decay_correction_applied;}
		int normalizationapplied(void) {return m_Data.normalization_applied;}
		int attenuationapplied(void) {return m_Data.attenuation_applied;}
		int scattercorrection(void) {return m_Data.scatter_correction;}
		int arccorrection(void) {return m_Data.arc_correction_applied;}
		
		float pixelsize(void) {return m_Data.pixel_size;}

		float calibrationfactor(void) {return m_Data.calibration_factor;}
		float calibrationbranchingfraction(void) {return m_Data.calibration_branching_fraction;}
		int numberofsinglesrates(void) {return m_Data.number_of_singles_rates;}
		
		std::string investigator(void) {return m_Data.investigator;}
		std::string Operator(void) {return m_Data.Operator;}
		std::string studyidentifier(void) {return m_Data.study_identifier;}
		long scantime(void) {return m_Data.scan_time;}
		std::string injectedcompound(void) {return m_Data.injected_compound;}
		int doseunits(void) {return m_Data.dose_units;}
		float dose(void) {return m_Data.dose;}
		long injectiontime(void) {return m_Data.injection_time;}
		float injectiondecaycorrection(void) {return m_Data.injection_decay_correction;}
		
		
		std::string subjectidentifier(void) {return m_Data.subject_identifier;}
		std::string subjectgenus(void) {return m_Data.subject_genus;}
		int subjectorientation(void) {return m_Data.subject_orientation;}
		int subjectlengthunits(void) {return m_Data.subject_length_units;}
		float subjectlength(void) {return m_Data.subject_length;}
		int subjectweightunits(void) {return m_Data.subject_weight_units;}
		float subjectweight(void) {return m_Data.subject_weight;}
		std::string subjectphenotype(void) {return m_Data.subject_phenotype;}
		std::string studymodel(void) {return m_Data.study_model;}
		
		std::string anesthesia(void) {return m_Data.anesthesia;}
		std::string analgesia(void) {return m_Data.analgesia;}
		std::string otherdrugs(void) {return m_Data.other_drugs;}
		std::string foodaccess(void) {return m_Data.food_access;}
		std::string wateraccess(void) {return m_Data.water_access;}
		
		//mutator methods
		void setModel(int value) { m_Data.model = value;}
		void setInstitution(std::string value) { m_Data.institution = value;}
		void setFileName(std::string value) { m_Data.file_name = value;}
		void setFileType(int value) { m_Data.file_type = value;}
		void setAcquisitionMode(int value) { m_Data.acquisition_mode = value;}
		void setBedMotion(int value) { m_Data.bed_motion = value;}
		void setNumFrames(int value) { m_Data.total_frames = value;}
		
		void setIsotope(std::string value) { m_Data.isotope = value;}
		void setIsotopeHalftime(float value) { m_Data.isotope_half_time = value;}
		void setIsotopeBranchingFraction(float value) { m_Data.isotope_branching_fraction = value;}
		
		void setTransaxialCrystalsPerBlock(int value) {m_Data.transaxial_crystals_per_block = value;}
		void setAxialCrystalsPerBlock(int value) {m_Data.axial_crystals_per_block = value;}
		void setIntrinsicCrystalOffset(int value) {m_Data.intrinsic_crystal_offset = value;}
		void setAxialBlocks(int value) {m_Data.axial_blocks = value;}
		void setAxialCrystalPitch(float value) {m_Data.axial_crystal_pitch = value;}
		void setRadius(float value) {m_Data.radius = value;}
		void setRadialFov(float value) {m_Data.radial_fov = value;}
		void setSrcRadius(float value) {m_Data.src_radius = value;}
		void setSrcCmPerRev(float value) {m_Data.src_cm_per_rev = value;}
		void setTxSrcType(int value) {m_Data.tx_src_type = value;}
		void setTransaxialBinSize(float value) {m_Data.transaxial_bin_size = value;}
		void setAxialPlaneSize(float value) {m_Data.axial_plane_size = value;}
		void setLld(float value) {m_Data.lld = value;}
		void setUld(float value) {m_Data.uld = value;}
		
		void setDataType(int value) { m_Data.data_type = value;}
		void setDataOrder(int value){ m_Data.data_order = value;}
		void setSpan(int value) { m_Data.span = value;}
		void setRingDifference(int value) { m_Data.ring_difference = value;}
		void setNumberOfDimensions(int value) { m_Data.number_of_dimensions = value;}
		void setXDimension(int value) { m_Data.x_dimension = value;}
		void setYDimension(int value) { m_Data.y_dimension = value;}
		void setZDimension(int value) { m_Data.z_dimension = value;}
		void setWDimension(int value) { m_Data.w_dimension = value;}
		void setDeltaElements(int i, int value) { m_Data.delta_elements[i] = value;}
		
		void setDeadtimeCorrectionApplied(int value) { m_Data.deadtime_correction_applied = value;}
		void setDecayCorrectionApplied(int value) { m_Data.decay_correction_applied = value;}
		void setNormalizationApplied(int value) { m_Data.normalization_applied = value;}
		void setAttenuationApplied(int value) { m_Data.attenuation_applied = value;}
		void setScatterCorrection(int value) { m_Data.scatter_correction = value;}
		void setArcCorrectionApplied(int value) { m_Data.arc_correction_applied = value;}

		void setPixelSize(float value) {m_Data.pixel_size = value;}
		
		void setCalibrationFactor(float value) { m_Data.calibration_factor = value;}
		void setCalibrationBranchingFraction(float value) { m_Data.calibration_branching_fraction = value;}
		void setNumberOfSinglesRates(int value) { m_Data.number_of_singles_rates = value;}
		
		void setInvestigator(std::string value) { m_Data.investigator = value;}
		void setOperator(std::string value) { m_Data.Operator = value;}
		void setStudyIdentifier(std::string value) { m_Data.study_identifier = value;}
		void setScanTime(long value) { m_Data.scan_time = value;}
		void setInjectedCompound(std::string value) { m_Data.injected_compound = value;}
		void setDoseUnits(int value) { m_Data.dose_units = value;}
		void setDose(float value) { m_Data.dose = value;}
		void setInjectionTime(long value) { m_Data.injection_time = value;}
		void setInjectionDecayCorrection(float value) { m_Data.injection_decay_correction = value;}
		
		void setSubjectIdentifier(std::string value) { m_Data.subject_identifier = value;}
		void setSubjectGenus(std::string value) { m_Data.subject_genus = value;}
		void setSubjectOrientation(int value) { m_Data.subject_orientation = value;}
		void setSubjectLength_units(int value) { m_Data.subject_length_units = value;}
		void setSubjectLength(float value) { m_Data.subject_length = value;}
		void setSubjectWeight_units(int value) { m_Data.subject_weight_units = value;}
		void setSubjectWeight(float value) { m_Data.subject_weight = value;}
		void setSubjectPhenotype(std::string value) { m_Data.subject_phenotype = value;}
		void setStudyModel(std::string value) { m_Data.study_model = value;}
		
		void setAnesthesia(std::string value) { m_Data.anesthesia = value;}
		void setAnalgesia(std::string value) { m_Data.analgesia = value;}
		void setOtherDrugs(std::string value) { m_Data.other_drugs = value;}
		void setFoodAccess(std::string value) { m_Data.food_access = value;}
		void setWaterAccess(std::string value) { m_Data.water_access = value;}

	protected:
		CMedIOHeader& copyData(const CMedIOHeader& src);	
	
	private :
	//members
		std::list<std::string> literals;
		std::list<CHeaderConcordeFrame*> frames;
		typedef struct //Concorde Micropet Header <--> ECAT7 Header
    		{
			int		model;			//System_Type;
			std::string	institution;		//Facility_Name;
			std::string	study;			//Study type/description (string)
			std::string	file_name;		//Original_File_Name;
			int		file_type;		//File_Type;
			int		acquisition_mode;	//Acquisition_Type;
			int		bed_motion;		//0 static/unknown : 1 continuous motion
			int		total_frames;
			std::string	isotope;		//Isotope_Name;
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
			int		delta_elements[11];	//Num_Z_Elements[64];
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
			std::string	investigator;
			std::string	Operator;
			std::string	study_identifier;
			long		scan_time;		//Scan_Start_Time;
			std::string	injected_compound;
			int		dose_units;
			float		dose;
			long		injection_time;
			float		injection_decay_correction;
			std::string	subject_identifier;
			std::string	subject_genus;
			int		subject_orientation;	//Patient_Orientation;
			int		subject_length_units;
			float		subject_length;
			int		subject_weight_units;
			float		subject_weight;
			std::string	subject_phenotype;
			std::string	study_model;
			std::string	anesthesia;
			std::string	analgesia;
			std::string	other_drugs;
			std::string	food_access;
			std::string	water_access;
    		} ConcordeHeader;
		ConcordeHeader m_Data;
	//methods
		bool init();
};

#endif
