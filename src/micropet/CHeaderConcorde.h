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
		int getFrameSize();
		//accessor methods
		std::string getFileName(void) {return m_Data.file_name;}
		std::string getModel(void) {return m_Data.model;}
		std::string getFileType(void) {return m_Data.file_type;}
		std::string getScanTime(void) {return m_Data.scan_time;}
		std::string getIsotope(void) {return m_Data.isotope;}
		std::string getIsotopeHalftime(void) {return m_Data.isotope_half_time;}
		std::string getVerticalBedOffset(void) {return m_Data.vertical_bed_offset;}
		std::string getRadialFOV(void) {return m_Data.radial_fov;}
		std::string getEventType(void) {return m_Data.event_type;}
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
		
	private :
	//members
		std::list<std::string> literals;
		typedef struct //Concorde Micropet Header <--> ECAT7 Header
    		{
			std::string	file_name;		//Original_File_Name;
			std::string	model;			//System_Type;
			std::string	file_type;		//File_Type;
			std::string	scan_time;		//Scan_Start_Time;
			std::string	isotope;		//Isotope_Name;
			std::string	isotope_half_time; 	//Isotope_Halflife;
			std::string	vertical_bed_offset;	//Bed_Elevation;
			std::string	radial_fov;		//Distance_Scanned Transaxial_FOV;
			std::string	event_type;		//Coin_Samp_Mode;
			std::string	subject_weight;		//Patient_Weight;
			std::string	acquisition_mode;	//Acquisition_Type;
			std::string	subject_orientation;	//Patient_Orientation;
			std::string	institution;		//Facility_Name;
			//Q_UINT16      Num_Planes;
			std::string	lld;			//Lwr_True_Thres;
			std::string	uld;			//Upr_True_Thres;
			std::string	transaxial_bin_size;	//Bin_Size; X_Resolution;
			std::string	isotope_branching_fraction;	//Branching_Fraction;
			std::string	data_type;		//Data_Type;
			std::string	number_of_dimensions;	//Num_Dimensions;
			std::string	x_dimension;		//Num_R_Elements;
			std::string	y_dimension;		//Num_Angles;
			std::string	delta_elements[11];	//Num_Z_Elements[64];
			std::string	ring_difference;	//Ring_Difference;
			std::string	span;			//Axial_Compression;
			//string	V_Resolution;
			std::string	axial_plane_size;	//Z_Resolution;
			std::string	bed_motion;		//0 static/unknown : 1 continuous motion
			std::string	arc_correction;		//0 none : != 0 true
			std::string	total_frames;
			std::string	data_order;		//0 Segment|Axis|View|Ring_Diff
								//1 Segment|View|Axis|Ring_Diff
    		} ConcordeHeader;
		ConcordeHeader m_Data;
	//methods
	
};

#endif
