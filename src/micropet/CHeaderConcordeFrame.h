#ifndef CHEADERCONCORDEFRAME_H
#define CHEADERCONCORDEFRAME_H

#include <string>
#include <list>

class CHeaderConcordeFrame
{
	public :
	//contructors
		CHeaderConcordeFrame(std::string Filename, std::string frame);
	//destructor
		~CHeaderConcordeFrame();	
		
	//members
		
	//methods
		//accessor methods
		std::string frame(void) {return m_Data.frame;}
		std::string eventtype(void) {return m_Data.event_type;}
		std::string gate(void) {return m_Data.gate;}
		std::string bed(void) {return m_Data.bed;}
		std::string bedoffset(void) {return m_Data.bed_offset;}
		std::string endingbedoffset(void) {return m_Data.ending_bed_offset;}
		std::string verticalbedoffset(void) {return m_Data.vertical_bed_offset;}
		std::string datafilepointer(void) {return m_Data.data_file_pointer;}
		std::string framestart(void) {return m_Data.frame_start;}
		std::string frameduration(void) {return m_Data.frame_duration;}
		std::string scalefactor(void) {return m_Data.scale_factor;}
		std::string minimum(void) {return m_Data.minimum;}
		std::string maximum(void) {return m_Data.maximum;}
		std::string deadtimecorrection(void) {return m_Data.deadtime_correction;}
		std::string decaycorrection(void) {return m_Data.decay_correction;}
		std::string prompts(void) {return m_Data.prompts;}
		std::string delays(void) {return m_Data.delays;}
		std::string trues(void) {return m_Data.trues;}
		std::string promptsrate(void) {return m_Data.prompts_rate;}
		std::string delaysrate(void) {return m_Data.delays_rate;}
		
		//mutator methods
		void setFrame(std::string value) { m_Data.frame = value;}
		void setEventType(std::string value) { m_Data.event_type = value;}
		void setGate(std::string value) { m_Data.gate = value;}
		void setBed(std::string value) { m_Data.bed = value;}
		void setBedOffset(std::string value) { m_Data.bed_offset = value;}
		void setEndingBedOffset(std::string value) { m_Data.ending_bed_offset = value;}
		void setVerticalBedOffset(std::string value) { m_Data.vertical_bed_offset = value;}
		void setDataFilePointer(std::string value) { m_Data.data_file_pointer = value;}
		void setFrameStart(std::string value) { m_Data.frame_start = value;}
		void setFrameDuration(std::string value) { m_Data.frame_duration = value;}
		void setScaleFactor(std::string value) {m_Data.scale_factor = value;}
		void setMinimum(std::string value) {m_Data.minimum = value;}
		void setMaximum(std::string value) {m_Data.maximum = value;}
		void setDeadtimeCorrection(std::string value) {m_Data.deadtime_correction = value;}
		void setDecayCorrection(std::string value) {m_Data.decay_correction = value;}
		void setPrompts(std::string value) {m_Data.prompts = value;}
		void setDelays(std::string value) {m_Data.delays = value;}
		void setTrues(std::string value) {m_Data.trues = value;}
		void setPromptsRate(std::string value) {m_Data.prompts_rate = value;}
		void setDelaysRate(std::string value) {m_Data.delays_rate = value;}
		
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
		std::string File;
		std::list<std::string> literals;
	//methods
		bool load();
		void init();
};

#endif
