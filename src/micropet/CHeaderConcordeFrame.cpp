#include <iostream>
#include <fstream>

#include "debug.h"
#include "CMedIOHeader.h"
#include "CHeaderConcordeFrame.h"

using namespace std;

//  Class: CHeaderConcordeFrame
//  Constructor: CHeaderConcordeFrame
//!
//! constructs a CHeaderConcordeFrame object
//!
//! @param Filename: complete path to file holding header
//! @param frame: specific frame of header which holds information
//!
CHeaderConcordeFrame::CHeaderConcordeFrame(string Filename, int frame)
{
	File = Filename;
	init();
	m_Data.frame = frame;
	if(!this->load())
	{
		D("Something is wrong with the headerfile");
	}
	else
		D("Everything ok");
}

//  Class: CHeaderConcordeFrame
//  Method: init
//!
//! //initialise all keys which should be searched for in header file
//!
////////////////////////////////////////////////////////////////////////////////
void CHeaderConcordeFrame::init()
{
	literals.push_back("frame");
	literals.push_back("event_type");
	literals.push_back("gate");
	literals.push_back("bed");
	literals.push_back("bed_offset");
	literals.push_back("ending_bed_offset");
	literals.push_back("vertical_bed_offset");
	literals.push_back("data_file_pointer");
	literals.push_back("frame_start");
	literals.push_back("frame_duration");
	literals.push_back("scale_factor");
	literals.push_back("minimum");
	literals.push_back("maximum");
	literals.push_back("deadtime_correction");
	literals.push_back("decay_correction");
	literals.push_back("prompts");
	literals.push_back("delays");
	literals.push_back("trues");
	literals.push_back("prompts_rate");
	literals.push_back("delays_rate");
}

//  Class: CHeaderConcordeFrame
//  Method: load
//!
//! loades the headerinformation.
//!
//! @return true on success otherwise false
////////////////////////////////////////////////////////////////////////////////
bool CHeaderConcordeFrame::load()
{
	D("Start Parsing File %s for Frame: %d", File.c_str(), m_Data.frame);
	bool foundframe = false;
	ifstream pFile(File.c_str());
	if(pFile.good())
	{
		for(;pFile.eof()!=true;)
		{
			//search for specific frame entry in file
			//read all literals till end_of_header of frame
			char line[1024];
			pFile.getline(line,1024);
			
			if(strlen(line) == 0) 
				continue;
			if(line[0]!='#')
			{
				if((int)line[strlen(line)-1] == 13)
					line[strlen(line)-1] = 0;
				char *ptr_tok = NULL;
				ptr_tok = strtok(line, " ");
				//cout << ptr_tok << endl;
				//cout << ptr_tok << " ";
				//check for end_of_header
				//if true break for loop
				
				if(foundframe && strcmp("end_of_header", ptr_tok) == 0)
				{
					D("reached end of header"); 
					break;
				}
				
				if(strcmp(ptr_tok, "frame") == 0)
				{
					char *rest = NULL;
					rest = strtok(NULL, "\0");
					if(atoi(rest) == m_Data.frame)
					{
						foundframe = true;
						D("Frame %d found", m_Data.frame);
						continue;
					}
				}
				
				if(foundframe)
				{
					char *rest = NULL;
					rest = strtok(NULL, "\0");
					if((int)rest[strlen(rest)-1] == 13)
					{
						D("Warning: Header is Windowsfile");
						rest[strlen(rest)-1] = 0;
					}
					//D("%s found : value %s", ptr_tok, rest);
					if(strcmp(ptr_tok, "event_type") == 0) 		m_Data.event_type = atoi(rest);
					if(strcmp(ptr_tok, "gate") == 0)		m_Data.gate = atoi(rest);
					if(strcmp(ptr_tok, "bed") == 0) 		m_Data.bed = atoi(rest);
					if(strcmp(ptr_tok, "bed_offset") == 0) 		m_Data.bed_offset = atof(rest);
					if(strcmp(ptr_tok, "ending_bed_offset") == 0) 	m_Data.ending_bed_offset = atof(rest);
					if(strcmp(ptr_tok, "vertical_bed_offset") == 0) m_Data.vertical_bed_offset = atof(rest);
					if(strcmp(ptr_tok, "data_file_pointer") == 0) 	m_Data.data_file_pointer = rest;
					if(strcmp(ptr_tok, "frame_start") == 0) 	m_Data.frame_start = atof(rest);
					
					if(strcmp(ptr_tok, "frame_duration") == 0) 	m_Data.frame_duration = atof(rest);
					if(strcmp(ptr_tok, "scale_factor") == 0) 	m_Data.scale_factor = atof(rest);
					if(strcmp(ptr_tok, "minimum") == 0)	m_Data.minimum = atof(rest);
					
					if(strcmp(ptr_tok, "maximum") == 0)	m_Data.maximum = atof(rest);
					if(strcmp(ptr_tok, "deadtime_correction") == 0)	m_Data.deadtime_correction = atof(rest);
					if(strcmp(ptr_tok, "decay_correction")	== 0)	m_Data.decay_correction = atof(rest);
					
					if(strcmp(ptr_tok, "prompts") == 0)	m_Data.prompts = rest;
					if(strcmp(ptr_tok, "delays") == 0)	m_Data.delays = rest;
					if(strcmp(ptr_tok, "trues")	== 0)	m_Data.trues = rest;
					if(strcmp(ptr_tok, "prompts_rate") == 0)	m_Data.prompts_rate = atoi(rest);
					if(strcmp(ptr_tok, "delays_rate") == 0)	m_Data.delays_rate = atoi(rest);
				}	
			}
		}
		pFile.close();		
	}
	return true;
}
