#ifndef CHEADERCONCORDE_H
#define CHEADERCONCORDE_H

#include "CMedIOHeader.h"

#include <string>
#include <list>

class CHeaderConcorde : public CMedIOHeader
{
	public :
	//contructors
		CHeaderConcorde(std::string File);
		CHeaderConcorde();
	//destructor
		~CHeaderConcorde();	
		
	//members
		
	//methods
		bool load();
		bool save();
		CMedIOHeader::Format rtti();
		bool init();
		unsigned int getFrameSize();
		unsigned int getImageFrameSize();
		//accessor methods
		std::string model(void) {return m_Data.model;}
		std::string institution(void) {return m_Data.institution;}
		std::string study(void) {return m_Data.study;}
		std::string filename(void) {return m_Data.file_name;}
		std::string filetype(void) {return m_Data.file_type;}
		std::string acquisitionmode(void) {return m_Data.acquisition_mode;}
		std::string bedmotion(void) {return m_Data.bed_motion;}
		std::string totalframes(void) {return m_Data.total_frames;}
		
		std::string isotope(void) {return m_Data.isotope;}
		std::string isotopehalftime(void) {return m_Data.isotope_half_time;}
		std::string isotopebranchingfraction(void) {return m_Data.isotope_branching_fraction;}
		
		std::string transaxialcrystalsperblock(void) {return m_Data.transaxial_crystals_per_block;}
		std::string axialcrystalsperblock(void) {return m_Data.axial_crystals_per_block;}
		std::string intrinsiccrystaloffset(void) {return m_Data.intrinsic_crystal_offset;}
		std::string axialblocks(void) {return m_Data.axial_blocks;}
		std::string axialcrystalpitch(void) {return m_Data.axial_crystal_pitch;}
		std::string radius(void) {return m_Data.radius;}
		std::string radialfov(void) {return m_Data.radial_fov;}
		std::string srcradius(void) {return m_Data.src_radius;}
		std::string srccmperrev(void) {return m_Data.src_cm_per_rev;}
		std::string txsrctype(void) {return m_Data.tx_src_type;}
		std::string transaxialbinsize(void) {return m_Data.transaxial_bin_size;}
		std::string axialplanesize(void) {return m_Data.axial_plane_size;}
		std::string lld(void) {return m_Data.lld;}
		std::string uld(void) {return m_Data.uld;}
		
		std::string datatype(void) {return m_Data.data_type;}
		std::string dataorder(void){return m_Data.data_order;}
		std::string span(void) {return m_Data.span;}
		std::string ringdifference(void) {return m_Data.ring_difference;}		
		std::string numberofdimensions(void) {return m_Data.number_of_dimensions;}
		std::string xdimension(void) {return m_Data.x_dimension;}
		std::string ydimension(void) {return m_Data.y_dimension;}
		std::string zdimension(void) {return m_Data.z_dimension;}
		std::string wdimension(void) {return m_Data.w_dimension;}
		std::string deltaelements(int i) {return m_Data.delta_elements[i];}
		
		std::string deadtimecorrectionapplied(void) {return m_Data.deadtime_correction_applied;}
		std::string decaycorrectionapplied(void) {return m_Data.decay_correction_applied;}
		std::string normalizationapplied(void) {return m_Data.normalization_applied;}
		std::string attenuationapplied(void) {return m_Data.attenuation_applied;}
		std::string scattercorrection(void) {return m_Data.scatter_correction;}
		std::string arccorrection(void) {return m_Data.arc_correction;}
		
		std::string calibrationfactor(void) {return m_Data.calibration_factor;}
		std::string calibrationbranchingfraction(void) {return m_Data.calibration_branching_fraction;}
		std::string numberofsinglesrates(void) {return m_Data.number_of_singles_rates;}
		
		std::string investigator(void) {return m_Data.investigator;}
		std::string Operator(void) {return m_Data.Operator;}
		std::string studyidentifier(void) {return m_Data.study_identifier;}
		std::string scantime(void) {return m_Data.scan_time;}
		std::string injectedcompound(void) {return m_Data.injected_compound;}
		std::string doseunits(void) {return m_Data.dose_units;}
		std::string dose(void) {return m_Data.dose;}
		std::string injectiontime(void) {return m_Data.injection_time;}
		std::string injectiondecaycorrection(void) {return m_Data.injection_decay_correction;}
		
		
		std::string subjectidentifier(void) {return m_Data.subject_identifier;}
		std::string subjectgenus(void) {return m_Data.subject_genus;}
		std::string subjectorientation(void) {return m_Data.subject_orientation;}
		std::string subjectlengthunits(void) {return m_Data.subject_length_units;}
		std::string subjectlength(void) {return m_Data.subject_length;}
		std::string subjectweightunits(void) {return m_Data.subject_weight_units;}
		std::string subjectweight(void) {return m_Data.subject_weight;}
		std::string subjectphenotype(void) {return m_Data.subject_phenotype;}
		std::string studymodel(void) {return m_Data.study_model;}
		
		std::string anesthesia(void) {return m_Data.anesthesia;}
		std::string analgesia(void) {return m_Data.analgesia;}
		std::string otherdrugs(void) {return m_Data.other_drugs;}
		std::string foodaccess(void) {return m_Data.food_access;}
		std::string wateraccess(void) {return m_Data.water_access;}
		
		//mutator methods
		void setModel(std::string value) { m_Data.model = value;}
		void setInstitution(std::string value) { m_Data.institution = value;}
		void setFileName(std::string value) { m_Data.file_name = value;}
		void setFileType(std::string value) { m_Data.file_type = value;}
		void setAcquisitionMode(std::string value) { m_Data.acquisition_mode = value;}
		void setBedMotion(std::string value) { m_Data.bed_motion = value;}
		void setTotalFrames(std::string value) { m_Data.total_frames = value;}
		
		void setIsotope(std::string value) { m_Data.isotope = value;}
		void setIsotopeHalftime(std::string value) { m_Data.isotope_half_time = value;}
		void setIsotopeBranchingFraction(std::string value) { m_Data.isotope_branching_fraction = value;}
		
		void setTransaxialCrystalsPerBlock(std::string value) {m_Data.transaxial_crystals_per_block = value;}
		void setAxialCrystalsPerBlock(std::string value) {m_Data.axial_crystals_per_block = value;}
		void setIntrinsicCrystalOffset(std::string value) {m_Data.intrinsic_crystal_offset = value;}
		void setAxialBlocks(std::string value) {m_Data.axial_blocks = value;}
		void setAxialCrystalPitch(std::string value) {m_Data.axial_crystal_pitch = value;}
		void setRadius(std::string value) {m_Data.radius = value;}
		void setRadialFov(std::string value) {m_Data.radial_fov = value;}
		void setSrcRadius(std::string value) {m_Data.src_radius = value;}
		void setSrcCmPerRev(std::string value) {m_Data.src_cm_per_rev = value;}
		void setTxSrcType(std::string value) {m_Data.tx_src_type = value;}
		void setTransaxialBinSize(std::string value) {m_Data.transaxial_bin_size = value;}
		void setAxialPlaneSize(std::string value) {m_Data.axial_plane_size = value;}
		void setLld(std::string value) {m_Data.lld = value;}
		void setUld(std::string value) {m_Data.uld = value;}
		
		void setDataType(std::string value) { m_Data.data_type = value;}
		void setDataOrder(std::string value){ m_Data.data_order = value;}
		void setSpan(std::string value) { m_Data.span = value;}
		void setRingDifference(std::string value) { m_Data.ring_difference = value;}
		void setNumberOfDimensions(std::string value) { m_Data.number_of_dimensions = value;}
		void setXDimension(std::string value) { m_Data.x_dimension = value;}
		void setYDimension(std::string value) { m_Data.y_dimension = value;}
		void setZDimension(std::string value) { m_Data.z_dimension = value;}
		void setWDimension(std::string value) { m_Data.w_dimension = value;}
		void setDeltaElements(int i, std::string value) { m_Data.delta_elements[i] = value;}
		
		void setDeadtimeCorrectionApplied(std::string value) { m_Data.deadtime_correction_applied = value;}
		void setDecayCorrectionApplied(std::string value) { m_Data.decay_correction_applied = value;}
		void setNormalizationApplied(std::string value) { m_Data.normalization_applied = value;}
		void setAttenuationApplied(std::string value) { m_Data.attenuation_applied = value;}
		void setScatterCorrection(std::string value) { m_Data.scatter_correction = value;}
		void setArcCorrection(std::string value) { m_Data.arc_correction = value;}
		
		void setCalibrationFactor(std::string value) { m_Data.calibration_factor = value;}
		void setCalibrationBranchingFraction(std::string value) { m_Data.calibration_branching_fraction = value;}
		void setNumberOfSinglesRates(std::string value) { m_Data.number_of_singles_rates = value;}
		
		void setInvestigator(std::string value) { m_Data.investigator = value;}
		void setOperator(std::string value) { m_Data.Operator = value;}
		void setStudyIdentifier(std::string value) { m_Data.study_identifier = value;}
		void setScanTime(std::string value) { m_Data.scan_time = value;}
		void setInjectedCompound(std::string value) { m_Data.injected_compound = value;}
		void setDoseUnits(std::string value) { m_Data.dose_units = value;}
		void setDose(std::string value) { m_Data.dose = value;}
		void setInjectionTime(std::string value) { m_Data.injection_time = value;}
		void setInjectionDecayCorrection(std::string value) { m_Data.injection_decay_correction = value;}
		
		void setSubjectIdentifier(std::string value) { m_Data.subject_identifier = value;}
		void setSubjectGenus(std::string value) { m_Data.subject_genus = value;}
		void setSubjectOrientation(std::string value) { m_Data.subject_orientation = value;}
		void setSubjectLength_units(std::string value) { m_Data.subject_length_units = value;}
		void setSubjectLength(std::string value) { m_Data.subject_length = value;}
		void setSubjectWeight_units(std::string value) { m_Data.subject_weight_units = value;}
		void setSubjectWeight(std::string value) { m_Data.subject_weight = value;}
		void setSubjectPhenotype(std::string value) { m_Data.subject_phenotype = value;}
		void setStudyModel(std::string value) { m_Data.study_model = value;}
		
		void setAnesthesia(std::string value) { m_Data.anesthesia = value;}
		void setAnalgesia(std::string value) { m_Data.analgesia = value;}
		void setOtherDrugs(std::string value) { m_Data.other_drugs = value;}
		void setFoodAccess(std::string value) { m_Data.food_access = value;}
		void setWaterAccess(std::string value) { m_Data.water_access = value;}
		
	private :
	//members
		std::list<std::string> literals;
		typedef struct //Concorde Micropet Header <--> ECAT7 Header
    		{
			std::string	model;			//System_Type;
			std::string	institution;		//Facility_Name;
			std::string	study;			//Study type/description (string)
			std::string	file_name;		//Original_File_Name;
			std::string	file_type;		//File_Type;
			std::string	acquisition_mode;	//Acquisition_Type;
			std::string	bed_motion;		//0 static/unknown : 1 continuous motion
			std::string	total_frames;
			std::string	isotope;		//Isotope_Name;
			std::string	isotope_half_time; 	//Isotope_Halflife;
			std::string	isotope_branching_fraction;	//Branching_Fraction;
			std::string	transaxial_crystals_per_block;
			std::string	axial_crystals_per_block;
			std::string	intrinsic_crystal_offset;
			std::string	transaxial_blocks;
			std::string	axial_blocks;
			std::string	axial_crystal_pitch;
			std::string	radius;
			std::string	radial_fov;		//Distance_Scanned Transaxial_FOV;
			std::string	src_radius;
			std::string	src_cm_per_rev;
			std::string	tx_src_type;
			std::string	transaxial_bin_size;	//Bin_Size; X_Resolution;
			std::string	axial_plane_size;	//Z_Resolution;
			std::string	lld;			//Lwr_True_Thres;
			std::string	uld;			//Upr_True_Thres;
			std::string	data_type;		//Data_Type;
			std::string	data_order;		//0 Segment|Axis|View|Ring_Diff
								//1 Segment|View|Axis|Ring_Diff
			std::string	span;			//Axial_Compression;
			std::string	ring_difference;	//Ring_Difference;
			std::string	number_of_dimensions;	//Num_Dimensions;
			std::string	x_dimension;		//Num_R_Elements;
			std::string	y_dimension;		//Num_Angles;
			std::string	z_dimension;
			std::string	w_dimension;
			std::string	delta_elements[11];	//Num_Z_Elements[64];
			std::string	deadtime_correction_applied;
			std::string	decay_correction_applied;
			std::string	normalization_applied;
			std::string	attenuation_applied;
			std::string	scatter_correction;
			std::string	arc_correction;		//0 none : != 0 true
			std::string	calibration_factor;
			std::string	calibration_branching_fraction;
			std::string	number_of_singles_rates;
			std::string	investigator;
			std::string	Operator;
			std::string	study_identifier;
			std::string	scan_time;		//Scan_Start_Time;
			std::string	injected_compound;
			std::string	dose_units;
			std::string	dose;
			std::string	injection_time;
			std::string	injection_decay_correction;
			std::string	subject_identifier;
			std::string	subject_genus;
			std::string	subject_orientation;	//Patient_Orientation;
			std::string	subject_length_units;
			std::string	subject_length;
			std::string	subject_weight_units;
			std::string	subject_weight;
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
	
};

#endif
