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
		std::string getFileName(void) {return m_Data.file_name;}
		std::string getModel(void) {return m_Data.model;}
		std::string getFileType(void) {return m_Data.file_type;}
		std::string getScanTime(void) {return m_Data.scan_time;}
		std::string getIsotope(void) {return m_Data.isotope;}
		std::string getIsotopeHalftime(void) {return m_Data.isotope_half_time;}
		std::string getRadialFOV(void) {return m_Data.radial_fov;}
		std::string getSubjectWeight(void) {return m_Data.subject_weight;}
		std::string getAcquisitionMode(void) {return m_Data.acquisition_mode;}
		std::string getSubjectorientation(void) {return m_Data.subject_orientation;}
		std::string getInstitution(void) {return m_Data.institution;}
		std::string getLLD(void) {return m_Data.lld;}
		std::string getULD(void) {return m_Data.uld;}
		std::string getTransaxialBinSize(void) {return m_Data.transaxial_bin_size;}
		std::string getIsotopeBranchingFraction(void) {return m_Data.isotope_branching_fraction;}
		std::string getNumberOfDimensions(void) {return m_Data.number_of_dimensions;}
		std::string getXDimension(void) {return m_Data.x_dimension;}
		std::string getYDimension(void) {return m_Data.y_dimension;}
		std::string getDeltaElements(int i) {return m_Data.delta_elements[i];}
		std::string getRingDifference(void) {return m_Data.ring_difference;}
		std::string getSpan(void) {return m_Data.span;}
		std::string getAxialPlaneSize(void) {return m_Data.axial_plane_size;}
		std::string getBedMotion(void) {return m_Data.bed_motion;}
		std::string getArcCorrection(void) {return m_Data.arc_correction;}
		std::string getTotalFrames(void) {return m_Data.total_frames;}
		std::string getDataOrder(void){return m_Data.data_order;}
		std::string getDataType(void) {return m_Data.data_type;}
		std::string getScaleFactor(void) {return m_Data.scale_factor;}
		
		//mutator methods
		void setFileName(std::string FileName) { m_Data.file_name = FileName;}
		void setModel(std::string Model) { m_Data.model = Model;}
		void setFileType(std::string FileType) { m_Data.file_type = FileType;}
		void setScanTime(std::string ScanTime) { m_Data.scan_time = ScanTime;}
		void setIsotope(std::string Isotope) { m_Data.isotope = Isotope;}
		void setIsotopeHalftime(std::string IsotopeHalftime) { m_Data.isotope_half_time = IsotopeHalftime;}
		void setRadialFOV(std::string RadialFOV) { m_Data.radial_fov = RadialFOV;}
		void setSubjectWeight(std::string SubjectWeight) { m_Data.subject_weight = SubjectWeight;}
		void setAcquisitionMode(std::string AcquisitionMode) { m_Data.acquisition_mode = AcquisitionMode;}
		void setSubjectorientation(std::string Subjectorientation) { m_Data.subject_orientation = Subjectorientation;}
		void setInstitution(std::string Institution) { m_Data.institution = Institution;}
		void setLLD(std::string LLD) { m_Data.lld = LLD;}
		void setULD(std::string ULD) { m_Data.uld = ULD;}
		void setTransaxialBinSize(std::string TransaxialBinSize) { m_Data.transaxial_bin_size = TransaxialBinSize;}
		void setIsotopeBranchingFraction(std::string IsotopeBranchingFraction) { m_Data.isotope_branching_fraction = IsotopeBranchingFraction;}
		void setNumberOfDimensions(std::string NumberOfDimensions) { m_Data.number_of_dimensions = NumberOfDimensions;}
		void setXDimension(std::string XDimension) { m_Data.x_dimension = XDimension;}
		void setYDimension(std::string YDimension) { m_Data.y_dimension = YDimension;}
		void setDeltaElements(int i, std::string NumSinograms) { m_Data.delta_elements[i] = NumSinograms;}
		void setRingDifference(std::string RingDifference) { m_Data.ring_difference = RingDifference;}
		void setSpan(std::string Span) { m_Data.span = Span;}
		void setAxialPlaneSize(std::string AxialPlaneSize) { m_Data.axial_plane_size = AxialPlaneSize;}
		void setBedMotion(std::string BedMotion) { m_Data.bed_motion = BedMotion;}
		void setArcCorrection(std::string ArcCorrection) { m_Data.arc_correction = ArcCorrection;}
		void setTotalFrames(std::string TotalFrames) { m_Data.total_frames = TotalFrames;}
		void setDataOrder(std::string DataOrder){ m_Data.data_order = DataOrder;}
		void setDataType(std::string DataType) { m_Data.data_type = DataType;}
		
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
			std::string	scale_factor;
    		} ConcordeHeader;
		ConcordeHeader m_Data;
	//methods
	
};

#endif
