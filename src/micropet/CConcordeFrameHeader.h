//! @file CConcordeFrameHeader.h
//! @author Hagen Moelle
//! @date 11/13/2004
//! @brief contains definition of the class CConcordeFrameHeader

#ifndef CCONCORDEFRAMEHEADER_H
#define CCONCORDEFRAMEHEADER_H

#include <qstring.h>
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
		const CMedIOData* fileObject() const {return m_pMedIOData;}
		bool load();
		bool save() const;
		CMedIOHeader::Format headerFormat() const;

		//accessor methods
		int frame(void) const {return m_Data.frame;}
		int eventType(void) const {return m_Data.event_type;}
		int gate(void) const {return m_Data.gate;}
		int bed(void) const {return m_Data.bed;}
		float bedOffset(void) const {return m_Data.bed_offset;}
		float endingBedOffset(void) const {return m_Data.ending_bed_offset;}
		float verticalBedOffset(void) const {return m_Data.vertical_bed_offset;}
		QString dataFilePointer(void) const {return m_Data.data_file_pointer;}
		float frameStart(void) const {return m_Data.frame_start;}
		float frameDuration(void) const {return m_Data.frame_duration;}
		float scaleFactor(void) const {return m_Data.scale_factor;}
		float minimum(void) const {return m_Data.minimum;}
		float maximum(void) const {return m_Data.maximum;}
		float deadTimeCorrection(void) const {return m_Data.deadtime_correction;}
		float decayCorrection(void) const {return m_Data.decay_correction;}
		QString prompts(void) const {return m_Data.prompts;}
		QString delays(void) const {return m_Data.delays;}
		QString trues(void) const {return m_Data.trues;}
		int promptsRate(void) const {return m_Data.prompts_rate;}
		int delaysRate(void) const {return m_Data.delays_rate;}
		float single(int i) const {return m_Data.singles[i];}
		float rawSingle(int i) const {return m_Data.rawsingles[i];}
		
		//mutator methods
		void setFrame(const int value) { m_Data.frame = value;}
		void setEventType(const int value) { m_Data.event_type = value;}
		void setGate(const int value) { m_Data.gate = value;}
		void setBed(const int value) { m_Data.bed = value;}
		void setBedOffset(const float value) { m_Data.bed_offset = value;}
		void setEndingBedOffset(const float value) { m_Data.ending_bed_offset = value;}
		void setVerticalBedOffset(const float value) { m_Data.vertical_bed_offset = value;}
		void setDataFilePointer(const QString value) { m_Data.data_file_pointer = value;}
		void setFrameStart(const float value) { m_Data.frame_start = value;}
		void setFrameDuration(const float value) { m_Data.frame_duration = value;}
		void setScaleFactor(const float value) {m_Data.scale_factor = value;}
		void setMinimum(const float value) {m_Data.minimum = value;}
		void setMaximum(const float value) {m_Data.maximum = value;}
		void setDeadtimeCorrection(const float value) {m_Data.deadtime_correction = value;}
		void setDecayCorrection(const float value) {m_Data.decay_correction = value;}
		void setPrompts(const QString value) {m_Data.prompts = value;}
		void setDelays(const QString value) {m_Data.delays = value;}
		void setTrues(const QString value) {m_Data.trues = value;}
		void setPromptsRate(const int value) {m_Data.prompts_rate = value;}
		void setDelaysRate(const int value) {m_Data.delays_rate = value;}

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
