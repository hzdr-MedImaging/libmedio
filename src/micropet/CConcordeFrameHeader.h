//! @file CConcordeFrameHeader.h
//! @author Hagen Moelle
//! @date 11/13/2004
//! @brief contains definition of the class CConcordeFrameHeader

#ifndef CCONCORDEFRAMEHEADER_H
#define CCONCORDEFRAMEHEADER_H

#include <QString>
#include "CConcordeFile.h"
#include "CKeyParser.h"

class CConcordeFile;

//! @class CConcordeFrameHeader
//! @brief class which holds specific frame information of concorde microPET headers
//! @bug no bugs reported
//! @warning still in development
//!
//! This class allows easy access and setting of specific frame information in concorde
//! microPET headers.
//! 
class CConcordeFrameHeader : public CMedIOHeader
{
	public :
	//contructors
		//! @brief constructor
		//! @param Filename: complete path to file holding header
		//! @param frame: specific frame of header which holds information
		CConcordeFrameHeader(QString Filename, int frame);
		CConcordeFrameHeader(CConcordeFile* file, QString Filename, int frame);

	//destructor
		//! @brief destructor
		~CConcordeFrameHeader(){};	
		
	//members
		enum EventType{UnknownEventType = 0, Singles, Prompt, Delay, Trues};
	//methods
		CMedIOData* fileObject() {return m_pMedIOData;}
		bool load();
		bool save() const;
		CMedIOHeader::Format headerFormat() const;

		//accessor methods
		int frame(void) {return m_Data.frame;}
		int eventType(void) {return m_Data.event_type;}
		int gate(void) {return m_Data.gate;}
		int bed(void) {return m_Data.bed;}
		float bedOffset(void) {return m_Data.bed_offset;}
		float endingBedOffset(void) {return m_Data.ending_bed_offset;}
		float verticalBedOffset(void) {return m_Data.vertical_bed_offset;}
		QString dataFilePointer(void) {return m_Data.data_file_pointer;}
		float frameStart(void) {return m_Data.frame_start;}
		float frameDuration(void) {return m_Data.frame_duration;}
		float scaleFactor(void) {return m_Data.scale_factor;}
		float minimum(void) {return m_Data.minimum;}
		float maximum(void) {return m_Data.maximum;}
		float deadTimeCorrection(void) {return m_Data.deadtime_correction;}
		float decayCorrection(void) {return m_Data.decay_correction;}
		QString prompts(void) {return m_Data.prompts;}
		QString delays(void) {return m_Data.delays;}
		QString trues(void) {return m_Data.trues;}
		int promptsRate(void) {return m_Data.prompts_rate;}
		int delaysRate(void) {return m_Data.delays_rate;}
		float single(int i) {return m_Data.singles[i];}
		float rawSingle(int i) {return m_Data.rawsingles[i];}
		
		//mutator methods
		void setFrame(int value) { m_Data.frame = value;}
		void setEventType(int value) { m_Data.event_type = value;}
		void setGate(int value) { m_Data.gate = value;}
		void setBed(int value) { m_Data.bed = value;}
		void setBedOffset(float value) { m_Data.bed_offset = value;}
		void setEndingBedOffset(float value) { m_Data.ending_bed_offset = value;}
		void setVerticalBedOffset(float value) { m_Data.vertical_bed_offset = value;}
		void setDataFilePointer(QString value) { m_Data.data_file_pointer = value;}
		void setFrameStart(float value) { m_Data.frame_start = value;}
		void setFrameDuration(float value) { m_Data.frame_duration = value;}
		void setScaleFactor(float value) {m_Data.scale_factor = value;}
		void setMinimum(float value) {m_Data.minimum = value;}
		void setMaximum(float value) {m_Data.maximum = value;}
		void setDeadtimeCorrection(float value) {m_Data.deadtime_correction = value;}
		void setDecayCorrection(float value) {m_Data.decay_correction = value;}
		void setPrompts(QString value) {m_Data.prompts = value;}
		void setDelays(QString value) {m_Data.delays = value;}
		void setTrues(QString value) {m_Data.trues = value;}
		void setPromptsRate(int value) {m_Data.prompts_rate = value;}
		void setDelaysRate(int value) {m_Data.delays_rate = value;}

	protected:
		CMedIOHeader& copyData(const CMedIOHeader& src);	
	
	private :
	//members
		CKeyParser Parser;
		typedef struct //Concorde Micropet Header <--> ECAT7 Header
    		{
			int		frame;
			int		event_type;
			int		gate;
			int		bed;
			float		bed_offset;
			float		ending_bed_offset;
			float		vertical_bed_offset;
			QString		data_file_pointer;
			float		frame_start;
			float		frame_duration;
			float		scale_factor;
			float		minimum;
			float		maximum;
			float		deadtime_correction;
			float		decay_correction;
			QString		prompts;
			QString		delays;
			QString		trues;
			int		prompts_rate;
			int		delays_rate;
			float		singles[168];
			float		rawsingles[168];
		}
		ConcordeHeaderFrame;
		ConcordeHeaderFrame m_Data;
		QString File;
	//methods
		void init();
};

#endif
