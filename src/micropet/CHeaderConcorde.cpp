//! @file CHeaderConcorde.cpp
//! @brief contains the implementation of the class CHeaderConcorde
//! @author Hagen Moelle
//! @date 11/13/2004

#include "CHeaderConcorde.h"
#include "debug.h"
#include "CHeaderConcordeFrame.h"
#include "CMedIOHeader.h"

#include <fstream>
#include <string>
#include <list>
#include <iostream>
#include <qdatetime.h>
#include <qstring.h>

using namespace std;

//  Class: CHeaderConcorde
//  Constructor: CHeaderConcorde
//!
//! constructs a CHeaderConcorde object
//!
//! @param file: complete path to file holding header
////////////////////////////////////////////////////////////////////////////////
CHeaderConcorde::CHeaderConcorde(string File) : CMedIOHeader(File)
{ 
	init();
	setDefaults();
	if(!this->load())
	{
		D("Something is wrong with the headerfile");
	}
	else
		D("Everything ok");
}

//  Class: CHeaderConcorde
//  Constructor: CHeaderConcorde
//!
//! constructs a CHeaderConcorde object
//!
////////////////////////////////////////////////////////////////////////////////
CHeaderConcorde::CHeaderConcorde()
{
	//initialise all keys which should be searched for in
	//header file
	init();
}

//  Class: CHeaderConcorde
//  Destructor:  CHeaderConcorde
//!
//! destructucts a CHeaderConcorde object.
//! 
////////////////////////////////////////////////////////////////////////////////
CHeaderConcorde::~CHeaderConcorde()
{
	ENTER();
	LEAVE();
}

//  Class: CHeaderConcorde
//  Method: setDefaults
//!
//! This methods sets some default values in the header. That minimizes 
//! the occurance of errors due to undefined values.
//!
////////////////////////////////////////////////////////////////////////////////
void CHeaderConcorde::setDefaults()
{
	m_Data.model = 0;
	m_Data.institution = "FZ Rossendorf"; 
	m_Data.file_type = 0;
	m_Data.acquisition_mode = 0;
	m_Data.bed_motion = 0;
	m_Data.total_frames = 0;
	m_Data.transaxial_crystals_per_block = 8;
	m_Data.axial_crystals_per_block = 8;
	m_Data.intrinsic_crystal_offset = 4;
	m_Data.transaxial_blocks = 42;
	m_Data.axial_blocks = 4;
	m_Data.transaxial_crystal_pitch = 0.2444F;
	m_Data.axial_crystal_pitch = 0.2423F;
	m_Data.radius = 13.0673F;
	m_Data.radial_fov = 12.067F;
	m_Data.src_radius = 11.2F;
	m_Data.src_cm_per_rev = 0.05F;
	m_Data.tx_src_type = 0;
	m_Data.transaxial_bin_size = 0.1222F;
	m_Data.axial_plane_size = 0.12115F;
	m_Data.lld = 350.0F;
	m_Data.uld = 750.0F;
	m_Data.data_type = 0;
	m_Data.data_order = 1;

	m_Data.span = 3;
	m_Data.ring_difference = 31;
	m_Data.number_of_dimensions = 4;
	m_Data.x_dimension = 192;
	m_Data.y_dimension = 168;
	m_Data.z_dimension = 63;
	m_Data.w_dimension = 11;

	m_Data.deadtime_correction_applied = 0;
	m_Data.decay_correction_applied = 0;
	m_Data.normalization_applied = 0;
	m_Data.attenuation_applied = 0;
	m_Data.scatter_correction = 0;
	m_Data.arc_correction_applied = 0;
	m_Data.x_offset = 0.0F;
	m_Data.y_offset = 0.0F;
	m_Data.zoom = 1.0F;
	m_Data.pixel_size = 0.0F;
	m_Data.calibration_units = 0;
	m_Data.calibration_factor = 0.0F;
	m_Data.calibration_branching_fraction = 1.0F;
	m_Data.number_of_singles_rates = 168;
	m_Data.dose_units = 0;
	m_Data.subject_orientation = 0;
	m_Data.subject_length_units = 0;
	m_Data.subject_weight_units = 0;
}

//  Class: CHeaderConcorde
//  Method: load
//!
//! loades the headerinformation.
//!
//! @return true on success otherwise false
////////////////////////////////////////////////////////////////////////////////
bool CHeaderConcorde::load()
{
	//unset function should be used !!!
	string File = file();
	D("Start Parsing File %s", File.c_str());
	ifstream pFile(File.c_str());
	if(pFile.good())
	{
		// read all literals from headerfile skipping comments starting with #
		// compare the literals with recommended list of literals
		// if true delete literal from list and set literal in ECAT7 header
		for(;pFile.eof()!=true;)
		{
			//there is a problem with eof() when file ends with \n (0x10)
			//so we have to check for the keyword end_of_header
			//another problem is that there are multiple end_of_header
			//1+number_of_frames
			
			char line[1024];
			pFile.getline(line,1024);
			
			
			//cout << line << endl;
			if(strlen(line) == 0) 
				continue;
				
			if(line[0]!='#')
			{
				char *ptr_tok = NULL;
				ptr_tok = strtok(line, " ");
				//cout << ptr_tok << endl;
				//cout << ptr_tok << " ";
				//check for end_of_header
				//if true break for loop
				if((int)line[strlen(line)-1] == 13)
					line[strlen(line)-1] = 0;
				if(strcmp("end_of_header", ptr_tok) == 0)
				{
					D("reached end of header"); 
					break;
				}
				
				//ptr_tok = strtok(NULL, "\0");
				// compare literal tokens[0] with list
				list<string>::iterator nums_iter;
				bool literal_exists = false;
				
				for(nums_iter = literals.begin(); nums_iter != literals.end(); nums_iter++)
				{	
					if(strcmp(nums_iter->c_str(), ptr_tok) == 0)
					{
						literal_exists = true;
						//cout << nums_iter->c_str() << endl;
						break;	
					}
					
				}
				
				if(literal_exists) 
				{
					//cout << "3.if CConcordeMicropetHeader::load" << endl;
					char *rest = NULL;
					rest = strtok(NULL, "\0");
					if((int)rest[strlen(rest)-1] == 13)
					{
						D("Warning: Header is Windowsfile");
						rest[strlen(rest)-1] = 0;
					}
					if(strcmp(ptr_tok, "model") == 0) 		m_Data.model = atoi(rest);
					if(strcmp(ptr_tok, "institution") == 0)		m_Data.institution = rest;
					if(strcmp(ptr_tok, "study") == 0) 		m_Data.study = rest;
					if(strcmp(ptr_tok, "file_name") == 0) 		m_Data.file_name = rest;
					if(strcmp(ptr_tok, "file_type") == 0) 		m_Data.file_type = atoi(rest);
					if(strcmp(ptr_tok, "acquisition_mode") == 0) 	m_Data.acquisition_mode = atoi(rest);
					if(strcmp(ptr_tok, "bed_motion") == 0) 		m_Data.bed_motion = atoi(rest);
					if(strcmp(ptr_tok, "total_frames") == 0) 	m_Data.total_frames = atoi(rest);
					
					if(strcmp(ptr_tok, "isotope") == 0) 		m_Data.isotope = rest;
					if(strcmp(ptr_tok, "isotope_half_time") == 0) 	m_Data.isotope_half_time = atof(rest);
					if(strcmp(ptr_tok, "isotope_branching_fraction") == 0)	m_Data.isotope_branching_fraction = atof(rest);
					
					if(strcmp(ptr_tok, "transaxial_crystals_per_block") == 0)	m_Data.transaxial_crystals_per_block = atoi(rest);
					if(strcmp(ptr_tok, "axial_crystals_per_block") == 0)	m_Data.axial_crystals_per_block = atoi(rest);
					if(strcmp(ptr_tok, "intrinsic_crystal_offset")	== 0)	m_Data.intrinsic_crystal_offset = atoi(rest);
					if(strcmp(ptr_tok, "transaxial_blocks") == 0) 	m_Data.transaxial_blocks = atoi(rest);
					if(strcmp(ptr_tok, "axial_blocks") == 0) m_Data.axial_blocks = atoi(rest);
					if(strcmp(ptr_tok, "axial_crystal_pitch") == 0) m_Data.axial_crystal_pitch = atof(rest);
					if(strcmp(ptr_tok, "radius") == 0)	m_Data.radius = atof(rest);
					if(strcmp(ptr_tok, "radial_fov") == 0)	m_Data.radial_fov = atof(rest);
					if(strcmp(ptr_tok, "src_radius") == 0)	m_Data.src_radius = atof(rest);
					if(strcmp(ptr_tok, "src_cm_per_rev") == 0) m_Data.src_cm_per_rev = atof(rest);
					if(strcmp(ptr_tok, "tx_src_type") == 0)	m_Data.tx_src_type = atoi(rest);
					if(strcmp(ptr_tok, "transaxial_bin_size") == 0)	m_Data.transaxial_bin_size = atof(rest);
					if(strcmp(ptr_tok, "axial_plane_size") == 0)	m_Data.axial_plane_size = atof(rest);
					if(strcmp(ptr_tok, "lld") == 0)			m_Data.lld = atof(rest);
					if(strcmp(ptr_tok, "uld") == 0) 		m_Data.uld = atof(rest);
					
					if(strcmp(ptr_tok, "data_order") == 0) 		m_Data.data_order = atoi(rest);
					if(strcmp(ptr_tok, "data_type") == 0)		m_Data.data_type = atoi(rest);
					if(strcmp(ptr_tok, "span") == 0) 		m_Data.span = atoi(rest);
					if(strcmp(ptr_tok, "ring_difference") == 0) 	m_Data.ring_difference = atoi(rest);
					if(strcmp(ptr_tok, "number_of_dimensions") == 0)	m_Data.number_of_dimensions = atoi(rest);
					if(strcmp(ptr_tok, "x_dimension") == 0) 		m_Data.x_dimension = atoi(rest);
					if(strcmp(ptr_tok, "y_dimension") == 0)		m_Data.y_dimension = atoi(rest);
					if(strcmp(ptr_tok, "z_dimension") == 0)		m_Data.z_dimension = atoi(rest);
					if(strcmp(ptr_tok, "w_dimension") == 0)		m_Data.w_dimension = atoi(rest);
					if(strcmp(ptr_tok, "delta_elements") == 0)
					{ 	
						char *element = NULL;
						element = strtok(rest, " ");
						char *count;
						count = strtok(NULL, "\0");
						m_Data.delta_elements[atoi(element)] = atoi(count);
					}
					
					if(strcmp(ptr_tok, "deadtime_correction_applied") == 0)		m_Data.deadtime_correction_applied = atoi(rest);
					if(strcmp(ptr_tok, "decay_correction_applied") == 0)		m_Data.decay_correction_applied = atoi(rest);
					if(strcmp(ptr_tok, "normalization_applied") == 0)		m_Data.normalization_applied = atoi(rest);
					if(strcmp(ptr_tok, "attenuation_applied") == 0)		m_Data.attenuation_applied = atoi(rest);
					if(strcmp(ptr_tok, "scatter_correction") == 0)		m_Data.scatter_correction = atoi(rest);
					if(strcmp(ptr_tok, "arc_correction") == 0)		m_Data.arc_correction_applied = atoi(rest);
					
					if(strcmp(ptr_tok, "calibration_factor") == 0)		m_Data.calibration_factor = atof(rest);
					if(strcmp(ptr_tok, "calibration_branching_fraction") == 0)		m_Data.calibration_branching_fraction = atof(rest);
					if(strcmp(ptr_tok, "number_of_singles_rates") == 0)		m_Data.number_of_singles_rates = atoi(rest);
					
					if(strcmp(ptr_tok, "investigator") == 0)		m_Data.investigator = rest;
					if(strcmp(ptr_tok, "Operator") == 0)		m_Data.Operator = rest;
					if(strcmp(ptr_tok, "study_identifier") == 0)		m_Data.study_identifier = rest;
					if(strcmp(ptr_tok, "scan_time") == 0)		
					{	
						QString str(rest);
						QDateTime dt = QDateTime::fromString(str);
						m_Data.scan_time = (long)dt.toTime_t();
						D("ScanTime %s in seconds %ld", rest, m_Data.scan_time);
					}
					if(strcmp(ptr_tok, "injected_compound") == 0)		m_Data.injected_compound = rest;
					if(strcmp(ptr_tok, "dose_units") == 0)		m_Data.dose_units = atoi(rest);
					if(strcmp(ptr_tok, "dose") == 0)		m_Data.dose = atof(rest);
					if(strcmp(ptr_tok, "injection_time") == 0)
					{
						QString str(rest);
						QDateTime dt = QDateTime::fromString(str);
						m_Data.injection_time = (long)dt.toTime_t();
						D("InjectionTime %s in seconds %ld", rest, m_Data.injection_time);
					}
					if(strcmp(ptr_tok, "injection_decay_correction") == 0)		m_Data.injection_decay_correction = atof(rest);
					
					if(strcmp(ptr_tok, "subject_identifier") == 0)		m_Data.subject_identifier = rest;
					if(strcmp(ptr_tok, "subject_genus") == 0)		m_Data.subject_genus = rest;
					if(strcmp(ptr_tok, "subject_orientation") == 0)		m_Data.subject_orientation = atoi(rest);
					if(strcmp(ptr_tok, "subject_length_units") == 0)		m_Data.subject_length_units = atoi(rest);
					if(strcmp(ptr_tok, "subject_length") == 0)		m_Data.subject_length = atof(rest);
					if(strcmp(ptr_tok, "subject_weight_units") == 0)		m_Data.subject_weight = atof(rest);
					if(strcmp(ptr_tok, "subject_phenotype") == 0)		m_Data.subject_phenotype = rest;
					if(strcmp(ptr_tok, "study_model") == 0)		m_Data.study_model = rest;
					
					if(strcmp(ptr_tok, "anesthesia") == 0)		m_Data.anesthesia = rest;
					if(strcmp(ptr_tok, "analgesia") == 0)		m_Data.analgesia = rest;
					if(strcmp(ptr_tok, "other_drugs") == 0)		m_Data.other_drugs = rest;
					if(strcmp(ptr_tok, "food_access") == 0)		m_Data.food_access = rest;
					if(strcmp(ptr_tok, "water_access") == 0)		m_Data.water_access = rest;
				}
			}
			
			
		}
		pFile.close();
		D("Parsing of MainHeader is ok!");
		//now parse same file for frames
		
		for(int i = 0; i < m_Data.total_frames; i++)
		{
			CHeaderConcordeFrame* frame = new CHeaderConcordeFrame(File, i);
			frames.push_back(frame);
		}
		
	}
	else
	{
		D("Headerfile %s nicht gefunden !", File.c_str());
		return false;
	}
	return true;
}

//  Class: CHeaderConcorde
//  Method: save
//!
//! Saves the headerinformation.
//!
//! @return true on success otherwise false
////////////////////////////////////////////////////////////////////////////////
bool CHeaderConcorde::save()
{
	return true;
}

int CHeaderConcorde::rtti()
{
	return CMedIOHeader::ConcordeMicropet;
}

//  Class: CHeaderConcorde
//  Method: getFrameSize
//!
//! @return framesize of a sinogram in bytes
////////////////////////////////////////////////////////////////////////////////
unsigned int CHeaderConcorde::getFrameSize()
{	
	int framesize = 0;
	int typesize = 0;
	//calculate the frame size: 
	//frame_size = number_of_sinograms*x_dimension*y_dimension*data_type_size
	//number_of_dimensions = sum(sinograms) over all delta_elements
	if(m_Data.data_type == 2 | m_Data.data_type == 6)
	{
		typesize = 2;
	}
	else 
		typesize = 4;
	
	if(m_Data.data_type == 0)
	{
		typesize = 1;
	}
	if(m_Data.data_type == 1)
	{
		typesize = 1;
	}
	
	for(int i = 0; i < 11; i++) framesize += m_Data.delta_elements[i];
	framesize = framesize*m_Data.x_dimension*m_Data.y_dimension*typesize;
	return framesize;
}

//  Class: CHeaderConcorde
//  Method: getImageFrameSize
//!
//! @return framesize of a imagevolume in bytes
////////////////////////////////////////////////////////////////////////////////
unsigned int CHeaderConcorde::getImageFrameSize()
{	
	int framesize = 0;
	int typesize = 0;
	//calculate the imageframe size: 
	//framesize = x_dimension*y_dimension*z_dimension*data_type_size
	if(m_Data.data_type == 2 | m_Data.data_type == 6)
	{
		typesize = 2;
	}
	else 
		typesize = 4;
	
	if(m_Data.data_type == 0)
	{
		typesize = 1;
	}
	if(m_Data.data_type == 1)
	{
		typesize = 1;
	}
	
	framesize = m_Data.x_dimension*m_Data.y_dimension*m_Data.z_dimension*typesize;
	return framesize;
}

//  Class: CHeaderConcorde
//  Method: getImageFrameSize
//!
//! access frames starting with frame 1 as first frame -> i = 1
//!
//! @return frame specific header
////////////////////////////////////////////////////////////////////////////////
CHeaderConcordeFrame* CHeaderConcorde::frame(int i)
{
	list<CHeaderConcordeFrame*>::iterator nums_iter;
	int k = 1;
	for(nums_iter = frames.begin(); k < i; nums_iter++,k++);
	return *nums_iter;
}

//  Class: CHeaderConcorde
//  Method: init
//!
//! //initialise all keys which should be searched for in header file
//!
//! @return true on success otherwise false
////////////////////////////////////////////////////////////////////////////////
bool CHeaderConcorde::init()
{
	literals.push_back("model");
	literals.push_back("institution");
	literals.push_back("study");
	literals.push_back("file_name");
	literals.push_back("file_type");
	literals.push_back("acquisition_mode");
	literals.push_back("bed_motion");
	literals.push_back("total_frames");
	
	literals.push_back("isotope");
	literals.push_back("isotope_half_time");
	literals.push_back("isotope_branching_fraction");
	
	literals.push_back("transaxial_crystals_per_block");
	literals.push_back("axial_crystals_per_block");
	literals.push_back("intrinsic_crystal_offset");
	literals.push_back("axial_blocks");
	literals.push_back("axial_crystal_pitch");
	literals.push_back("radius");
	literals.push_back("radial_fov");
	literals.push_back("src_radius");
	literals.push_back("src_cm_per_rev");
	literals.push_back("tx_src_type");
	literals.push_back("transaxial_bin_size");
	literals.push_back("axial_plane_size");
	literals.push_back("lld");
	literals.push_back("uld");
	
	literals.push_back("data_type");
	literals.push_back("data_order");
	literals.push_back("span");
	literals.push_back("ring_difference");
	literals.push_back("number_of_dimensions");
	literals.push_back("x_dimension");
	literals.push_back("y_dimension");
	literals.push_back("z_dimension");
	literals.push_back("w_dimension");
	literals.push_back("delta_elements");	
	
	literals.push_back("deadtime_correction_applied");
	literals.push_back("decay_correction_applied");
	literals.push_back("normalization_applied");
	literals.push_back("attenuation_applied");
	literals.push_back("scatter_correction");
	literals.push_back("arc_correction");
	
	literals.push_back("calibration_factor");
	literals.push_back("calibration_branching_fraction");
	literals.push_back("number_of_singles_rates");
	
	literals.push_back("investigator");
	literals.push_back("operator");
	literals.push_back("study_identifier");
	literals.push_back("scan_time");
	literals.push_back("injected_compound");
	literals.push_back("dose_units");
	literals.push_back("dose");
	literals.push_back("injection_time");
	literals.push_back("injection_decay_correction");
	
	literals.push_back("subject_identifier");
	literals.push_back("subject_genus");
	literals.push_back("subject_orientation");
	literals.push_back("subject_length_units");
	literals.push_back("subject_length");
	literals.push_back("subject_weight_units");
	literals.push_back("subject_weight");
	literals.push_back("subject_phenotype");
	literals.push_back("study_model");
	
	literals.push_back("anesthesia");
	literals.push_back("analgesia");
	literals.push_back("other_drugs");
	literals.push_back("food_access");
	literals.push_back("water_access");
			
	return true;
}
