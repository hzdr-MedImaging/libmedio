//! @file CHeaderConcordeFrame.h
//! @author Hagen Moelle
//! @date 11/13/2004
//! @brief contains definition of the class CHeaderConcordeFrame

#ifndef CHEADERCONCORDEFRAME_H
#define CHEADERCONCORDEFRAME_H

#include <string>
#include <list>

//! @class CHeaderConcordeFrame
//! @brief class which holds specific frame information of concorde microPET headers
//! @bug no bugs reported
//! @warning still in development
//!
//! This class allows easy access and setting of specific frame information in concorde
//! microPET headers.
//! 
class CHeaderConcordeFrame
{
	public :
	//contructors
		//! @brief constructor
		//! @param Filename: complete path to file holding header
		//! @param frame: specific frame of header which holds information
		CHeaderConcordeFrame(std::string Filename, int frame);

	//destructor
		//! @brief destructor
		~CHeaderConcordeFrame();	
		
	//members
		enum EventType{UnknownEventType = 0, Singles, Prompt, Delay, Trues};
	//methods
		//accessor methods
		int frame(void) {return m_Data.frame;}
		int eventtype(void) {return m_Data.event_type;}
		int gate(void) {return m_Data.gate;}
		int bed(void) {return m_Data.bed;}
		float bedoffset(void) {return m_Data.bed_offset;}
		float endingbedoffset(void) {return m_Data.ending_bed_offset;}
		float verticalbedoffset(void) {return m_Data.vertical_bed_offset;}
		std::string datafilepointer(void) {return m_Data.data_file_pointer;}
		float framestart(void) {return m_Data.frame_start;}
		float frameduration(void) {return m_Data.frame_duration;}
		float scalefactor(void) {return m_Data.scale_factor;}
		float minimum(void) {return m_Data.minimum;}
		float maximum(void) {return m_Data.maximum;}
		float deadtimecorrection(void) {return m_Data.deadtime_correction;}
		float decaycorrection(void) {return m_Data.decay_correction;}
		std::string prompts(void) {return m_Data.prompts;}
		std::string delays(void) {return m_Data.delays;}
		std::string trues(void) {return m_Data.trues;}
		int promptsrate(void) {return m_Data.prompts_rate;}
		int delaysrate(void) {return m_Data.delays_rate;}
		
		//mutator methods
		void setFrame(int value) { m_Data.frame = value;}
		void setEventType(int value) { m_Data.event_type = value;}
		void setGate(int value) { m_Data.gate = value;}
		void setBed(int value) { m_Data.bed = value;}
		void setBedOffset(float value) { m_Data.bed_offset = value;}
		void setEndingBedOffset(float value) { m_Data.ending_bed_offset = value;}
		void setVerticalBedOffset(float value) { m_Data.vertical_bed_offset = value;}
		void setDataFilePointer(std::string value) { m_Data.data_file_pointer = value;}
		void setFrameStart(float value) { m_Data.frame_start = value;}
		void setFrameDuration(float value) { m_Data.frame_duration = value;}
		void setScaleFactor(float value) {m_Data.scale_factor = value;}
		void setMinimum(float value) {m_Data.minimum = value;}
		void setMaximum(float value) {m_Data.maximum = value;}
		void setDeadtimeCorrection(float value) {m_Data.deadtime_correction = value;}
		void setDecayCorrection(float value) {m_Data.decay_correction = value;}
		void setPrompts(std::string value) {m_Data.prompts = value;}
		void setDelays(std::string value) {m_Data.delays = value;}
		void setTrues(std::string value) {m_Data.trues = value;}
		void setPromptsRate(int value) {m_Data.prompts_rate = value;}
		void setDelaysRate(int value) {m_Data.delays_rate = value;}
		
	private :
	//members
		typedef struct //Concorde Micropet Header <--> ECAT7 Header
    		{
			int		frame;
			int		event_type;
			int		gate;
			int		bed;
			float		bed_offset;
			float		ending_bed_offset;
			float		vertical_bed_offset;
			std::string	data_file_pointer;
			float		frame_start;
			float		frame_duration;
			float		scale_factor;
			float		minimum;
			float		maximum;
			float		deadtime_correction;
			float		decay_correction;
			std::string	prompts;
			std::string	delays;
			std::string	trues;
			int		prompts_rate;
			int		delays_rate;
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
