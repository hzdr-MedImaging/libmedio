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
	literals.push_back("file_name");
	literals.push_back("model");
	literals.push_back("file_type");
	literals.push_back("scan_time");
	literals.push_back("isotope");
	literals.push_back("isotope_half_time");
	//literals.push_back("vertical_bed_offset");
	literals.push_back("radial_fov");
	//literals.push_back("event_type");
	literals.push_back("subject_weight");
	literals.push_back("acquisition_mode");
	literals.push_back("subject_orientation");
	literals.push_back("institution");
	literals.push_back("total_frames");
	literals.push_back("lld");
	literals.push_back("uld");
	literals.push_back("transaxial_bin_size");
	literals.push_back("isotope_branching_fraction");
	literals.push_back("data_type");
	literals.push_back("number_of_dimensions");
	literals.push_back("x_dimension");
	literals.push_back("y_dimension");
	literals.push_back("z_dimension");
	literals.push_back("delta_elements");
	literals.push_back("ring_difference");
	literals.push_back("data_order");
	literals.push_back("span");
	literals.push_back("axial_plane_size");
	literals.push_back("bed_motion");
	literals.push_back("arc_correction_applied");
	literals.push_back("scale_factor");
	
	return true;
}

CHeaderConcorde::CHeaderConcorde(string File) : CMedIOHeader(File)
{ 
	init();
	if(!this->load())
	{
		cout<<"Something is wrong with the headerfile" << endl;
	}
	else
		cout << "Everything ok" <<endl;
}

bool CHeaderConcorde::load()
{
	//unset function should be used !!!
	string File = CMedIOHeader::getFile();
	cout << "Start Parsing File " << File << endl;
	ifstream pFile(File.c_str());
	if(pFile.good())
	{
		//cout << "1.if CConcordeMicropetHeader::load" << endl;
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
					
					if(strcmp(ptr_tok, "file_name") == 0) 		m_Data.file_name = rest;
					if(strcmp(ptr_tok, "model") == 0) 		m_Data.model = rest;
					if(strcmp(ptr_tok, "file_type") == 0) 		m_Data.file_type = rest;
					if(strcmp(ptr_tok, "scan_time")	== 0)		m_Data.scan_time = rest;
					
					if(strcmp(ptr_tok, "isotope") == 0) 		m_Data.isotope = rest;
					if(strcmp(ptr_tok, "isotope_half_time") == 0) 	m_Data.isotope_half_time = rest;
					if(strcmp(ptr_tok, "radial_fov") == 0)		m_Data.radial_fov = rest;
					if(strcmp(ptr_tok, "subject_weight") == 0) 	m_Data.subject_weight = rest;
					if(strcmp(ptr_tok, "acquisition_mode") == 0) 	m_Data.acquisition_mode = rest;
					if(strcmp(ptr_tok, "subject_orientation") == 0)	m_Data.subject_orientation = rest;
					if(strcmp(ptr_tok, "institution") == 0)		m_Data.institution = rest;
					
					if(strcmp(ptr_tok, "total_frames") == 0) 	m_Data.total_frames = rest;
					if(strcmp(ptr_tok, "lld") == 0)			m_Data.lld = rest;
					if(strcmp(ptr_tok, "uld") == 0) 		m_Data.uld = rest;
					if(strcmp(ptr_tok, "transaxial_bin_size") == 0)	m_Data.transaxial_bin_size = rest;
					if(strcmp(ptr_tok, "isotope_branching_fraction") == 0)	m_Data.isotope_branching_fraction = rest;
					if(strcmp(ptr_tok, "data_type") == 0)		m_Data.data_type = rest;
					if(strcmp(ptr_tok, "number_of_dimensions") == 0)	m_Data.number_of_dimensions = rest;
					if(strcmp(ptr_tok, "x_dimension") == 0) 		m_Data.x_dimension = rest;
					if(strcmp(ptr_tok, "y_dimension") == 0)		m_Data.y_dimension = rest;
					if(strcmp(ptr_tok, "z_dimension") == 0)		m_Data.z_dimension = rest;
					if(strcmp(ptr_tok, "delta_elements") == 0)
					{ 	
						char *element = NULL;
						element = strtok(rest, " ");
						char *count;
						count = strtok(NULL, "\0");
						m_Data.delta_elements[atoi(element)] = count;
					}
					if(strcmp(ptr_tok, "ring_difference") == 0) 	m_Data.ring_difference = rest;
					if(strcmp(ptr_tok, "span") == 0) 		m_Data.span = rest;
					if(strcmp(ptr_tok, "axial_plane_size") == 0) 	m_Data.axial_plane_size = rest;
					if(strcmp(ptr_tok, "bed_motion") == 0) 		m_Data.bed_motion = rest;
					if(strcmp(ptr_tok, "arc_correction_applied") == 0) 	m_Data.arc_correction = rest;
					if(strcmp(ptr_tok, "data_order") == 0) 		m_Data.data_order = rest;
					if(strcmp(ptr_tok, "scale_factor") == 0) 	m_Data.scale_factor = rest;
				}
			}
			
			
		}
		pFile.close();
		cout << "Parsing is ok!" <<endl;
	}
	else
	{
		cout << "Headerfile" << File << "nicht gefunden !" <<endl;
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
	if(getDataType() == "2" | getDataType() == "6")
	{
		typesize = 2;
	}
	else 
		typesize = 4;
	
	if(getDataType() == "0")
	{
		typesize = 1;
	}
	if(getDataType() == "1")
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
	if(getDataType() == "2" | getDataType() == "6")
	{
		typesize = 2;
	}
	else 
		typesize = 4;
	
	if(getDataType() == "0")
	{
		typesize = 1;
	}
	if(getDataType() == "1")
	{
		typesize = 1;
	}
	
	framesize = atoi(m_Data.x_dimension.c_str())*atoi(m_Data.y_dimension.c_str())*atoi(m_Data.z_dimension.c_str())*typesize;
	return framesize;
}
