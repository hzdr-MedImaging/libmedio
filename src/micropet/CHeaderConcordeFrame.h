#ifndef CHEADERCONCORDEFRAME_H
#define CHEADERCONCORDEFRAME_H

#include <string>


class CHeaderConcordeFrame
{
	public :
	//contructors
		CHeaderConcordeFrame();
	//destructor
		~CHeaderConcorde();	
		
	//members
		
	//methods
	
	private :
	//members
		typedef struct //Concorde Micropet Header <--> ECAT7 Header
    		{
			std::string	frame;
			std::string	event_type;
			std::string	gate;
			std::string	bed;
			std::string	bed_offset;
			std::string	ending_bed_offset;
			std::string	vertical_bed_offset;
			std::string	data_file_pointer;
			std::string	frame_start;
			std::string	frame_duration;
			std::string	scale_factor;
			std::string	minimum;
			std::string	maximum;
			std::string	deadtime_correction;
			std::string	decay_correction;
			std::string	prompts;
			std::string	delays;
			std::string	trues;
			std::string	prompts_rate;
			std::string	delays_rate;
			//singles
		}
		ConcordeHeaderFrame;
		ConcordeHeaderFrame m_Data;
	//methods
};
