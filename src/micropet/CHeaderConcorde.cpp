#include "CHeaderConcorde.h"
#include "debug.h"

#include <fstream>
#include <string>
#include <list>
#include <iostream>

using namespace std;

CHeaderConcorde::CHeaderConcorde()
{
	init();
}

CHeaderConcorde::~CHeaderConcorde()
{
	ENTER();
	LEAVE();
}

CMedIOHeader::Format CHeaderConcorde::rtti()
{
	return CMedIOHeader::ConcordeMicropet;
}

bool CHeaderConcorde::save()
{
	return true;
}

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

CHeaderConcorde::CHeaderConcorde(string File) : CMedIOHeader(File)
{ 
	init();
	if(!this->load())
	{
		D("Something is wrong with the headerfile");
	}
	else
		D("Everything ok");
}

bool CHeaderConcorde::load()
{
	//unset function should be used !!!
	string File = CMedIOHeader::getFile();
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
				/*
				if(strcmp("end_of_header", ptr_tok) == 0)
				{
					cout << "reached end of header" << endl; 
					break;
				}
				*/
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
					
					if(strcmp(ptr_tok, "model") == 0) 		m_Data.model = rest;
					if(strcmp(ptr_tok, "institution") == 0)		m_Data.institution = rest;
					if(strcmp(ptr_tok, "study") == 0) 		m_Data.study = rest;
					if(strcmp(ptr_tok, "file_name") == 0) 		m_Data.file_name = rest;
					if(strcmp(ptr_tok, "file_type") == 0) 		m_Data.file_type = rest;
					if(strcmp(ptr_tok, "acquisition_mode") == 0) 	m_Data.acquisition_mode = rest;
					if(strcmp(ptr_tok, "bed_motion") == 0) 		m_Data.bed_motion = rest;
					if(strcmp(ptr_tok, "total_frames") == 0) 	m_Data.total_frames = rest;
					
					if(strcmp(ptr_tok, "isotope") == 0) 		m_Data.isotope = rest;
					if(strcmp(ptr_tok, "isotope_half_time") == 0) 	m_Data.isotope_half_time = rest;
					if(strcmp(ptr_tok, "isotope_branching_fraction") == 0)	m_Data.isotope_branching_fraction = rest;
					
					if(strcmp(ptr_tok, "transaxial_crystals_per_block") == 0)	m_Data.transaxial_crystals_per_block = rest;
					if(strcmp(ptr_tok, "axial_crystals_per_block") == 0)	m_Data.axial_crystals_per_block = rest;
					if(strcmp(ptr_tok, "intrinsic_crystal_offset")	== 0)	m_Data.intrinsic_crystal_offset = rest;
					if(strcmp(ptr_tok, "transaxial_blocks") == 0) 	m_Data.transaxial_blocks = rest;
					if(strcmp(ptr_tok, "axial_blocks") == 0) m_Data.axial_blocks = rest;
					if(strcmp(ptr_tok, "axial_crystal_pitch") == 0) m_Data.axial_crystal_pitch = rest;
					if(strcmp(ptr_tok, "radius") == 0)	m_Data.radius = rest;
					if(strcmp(ptr_tok, "radial_fov") == 0)	m_Data.radial_fov = rest;
					if(strcmp(ptr_tok, "src_radius") == 0)	m_Data.src_radius = rest;
					if(strcmp(ptr_tok, "src_cm_per_rev") == 0) m_Data.src_cm_per_rev = rest;
					if(strcmp(ptr_tok, "tx_src_type") == 0)	m_Data.tx_src_type = rest;
					if(strcmp(ptr_tok, "transaxial_bin_size") == 0)	m_Data.transaxial_bin_size = rest;
					if(strcmp(ptr_tok, "axial_plane_size") == 0)	m_Data.axial_plane_size = rest;
					if(strcmp(ptr_tok, "lld") == 0)			m_Data.lld = rest;
					if(strcmp(ptr_tok, "uld") == 0) 		m_Data.uld = rest;
					
					if(strcmp(ptr_tok, "data_order") == 0) 		m_Data.data_order = rest;
					if(strcmp(ptr_tok, "data_type") == 0)		m_Data.data_type = rest;
					if(strcmp(ptr_tok, "span") == 0) 		m_Data.span = rest;
					if(strcmp(ptr_tok, "ring_difference") == 0) 	m_Data.ring_difference = rest;
					if(strcmp(ptr_tok, "number_of_dimensions") == 0)	m_Data.number_of_dimensions = rest;
					if(strcmp(ptr_tok, "x_dimension") == 0) 		m_Data.x_dimension = rest;
					if(strcmp(ptr_tok, "y_dimension") == 0)		m_Data.y_dimension = rest;
					if(strcmp(ptr_tok, "z_dimension") == 0)		m_Data.z_dimension = rest;
					if(strcmp(ptr_tok, "w_dimension") == 0)		m_Data.w_dimension = rest;
					if(strcmp(ptr_tok, "delta_elements") == 0)
					{ 	
						char *element = NULL;
						element = strtok(rest, " ");
						char *count;
						count = strtok(NULL, "\0");
						m_Data.delta_elements[atoi(element)] = count;
					}
					
					if(strcmp(ptr_tok, "deadtime_correction_applied") == 0)		m_Data.deadtime_correction_applied = rest;
					if(strcmp(ptr_tok, "decay_correction_applied") == 0)		m_Data.decay_correction_applied = rest;
					if(strcmp(ptr_tok, "normalization_applied") == 0)		m_Data.normalization_applied = rest;
					if(strcmp(ptr_tok, "attenuation_applied") == 0)		m_Data.attenuation_applied = rest;
					if(strcmp(ptr_tok, "scatter_correction") == 0)		m_Data.scatter_correction = rest;
					if(strcmp(ptr_tok, "arc_correction") == 0)		m_Data.arc_correction = rest;
					
					if(strcmp(ptr_tok, "calibration_factor") == 0)		m_Data.calibration_factor = rest;
					if(strcmp(ptr_tok, "calibration_branching_fraction") == 0)		m_Data.calibration_branching_fraction = rest;
					if(strcmp(ptr_tok, "number_of_singles_rates") == 0)		m_Data.number_of_singles_rates = rest;
					
					if(strcmp(ptr_tok, "investigator") == 0)		m_Data.investigator = rest;
					if(strcmp(ptr_tok, "Operator") == 0)		m_Data.Operator = rest;
					if(strcmp(ptr_tok, "study_identifier") == 0)		m_Data.study_identifier = rest;
					if(strcmp(ptr_tok, "scan_time") == 0)		m_Data.scan_time = rest;
					if(strcmp(ptr_tok, "injected_compound") == 0)		m_Data.injected_compound = rest;
					if(strcmp(ptr_tok, "dose_units") == 0)		m_Data.dose_units = rest;
					if(strcmp(ptr_tok, "dose") == 0)		m_Data.dose = rest;
					if(strcmp(ptr_tok, "injection_time") == 0)		m_Data.injection_time = rest;
					if(strcmp(ptr_tok, "injection_decay_correction") == 0)		m_Data.injection_decay_correction = rest;
					
					if(strcmp(ptr_tok, "subject_identifier") == 0)		m_Data.subject_identifier = rest;
					if(strcmp(ptr_tok, "subject_genus") == 0)		m_Data.subject_genus = rest;
					if(strcmp(ptr_tok, "subject_orientation") == 0)		m_Data.subject_orientation = rest;
					if(strcmp(ptr_tok, "subject_length_units") == 0)		m_Data.subject_length_units = rest;
					if(strcmp(ptr_tok, "subject_length") == 0)		m_Data.subject_length = rest;
					if(strcmp(ptr_tok, "subject_weight_units") == 0)		m_Data.subject_weight = rest;
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
		D("Parsing is ok!");
	}
	else
	{
		D("Headerfile %s nicht gefunden !", File.c_str());
		return false;
	}
	return true;
}

unsigned int CHeaderConcorde::getFrameSize()
{	
	int framesize = 0;
	int typesize = 0;
	//calculate the frame size: 
	//frame_size = number_of_sinograms*x_dimension*y_dimension*data_type_size
	//number_of_dimensions = sum(sinograms) over all delta_elements
	if(m_Data.data_type == "2" | m_Data.data_type == "6")
	{
		typesize = 2;
	}
	else 
		typesize = 4;
	
	if(m_Data.data_type == "0")
	{
		typesize = 1;
	}
	if(m_Data.data_type == "1")
	{
		typesize = 1;
	}
	
	for(int i = 0; i < 11; i++) framesize += atoi(m_Data.delta_elements[i].c_str());
	framesize = framesize*atoi(m_Data.x_dimension.c_str())*atoi(m_Data.y_dimension.c_str())*typesize;
	return framesize;
}

unsigned int CHeaderConcorde::getImageFrameSize()
{	
	int framesize = 0;
	int typesize = 0;
	//calculate the imageframe size: 
	//framesize = x_dimension*y_dimension*z_dimension*data_type_size
	if(m_Data.data_type == "2" | m_Data.data_type == "6")
	{
		typesize = 2;
	}
	else 
		typesize = 4;
	
	if(m_Data.data_type == "0")
	{
		typesize = 1;
	}
	if(m_Data.data_type == "1")
	{
		typesize = 1;
	}
	
	framesize = atoi(m_Data.x_dimension.c_str())*atoi(m_Data.y_dimension.c_str())*atoi(m_Data.z_dimension.c_str())*typesize;
	return framesize;
}
