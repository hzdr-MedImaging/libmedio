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
		CConcordeFrameHeader(CConcordeFile* file, int frame);

	//destructor
		//! @brief destructor
		~CConcordeFrameHeader(){};	
		
	//members
		enum EventType{UnknownEventType = 0, Singles, Prompt, Delay, Trues};
	//methods
		CMedIOData* fileObject() const;
		bool load();
		bool save() const;
		CMedIOHeader::Format headerFormat() const;
		CMedIOHeader* clone() const;
		bool convertFrom(const CMedIOHeader* srcMainHeader, const CMedIOHeader* srcSubHeader = NULL);

		//accessor methods
		int frame(void) const;
		int eventType(void) const;
		int gate(void) const;
		int bed(void) const;
		float bedOffset(void) const;
		float endingBedOffset(void) const;
		float verticalBedOffset(void) const;
		QString dataFilePointer(void) const;
		float frameStart(void) const;
		float frameDuration(void) const;
		float scaleFactor(void) const;
		float minimum(void) const;
		float maximum(void) const;
		float deadTimeCorrection(void) const;
		float decayCorrection(void) const;
		QString prompts(void) const;
		QString delays(void) const;
		QString trues(void) const;
		int promptsRate(void) const;
		int delaysRate(void) const;
		float single(int i) const;
		float rawSingle(int i) const;
		
		//mutator methods
		void setFrame(const int value);
		void setEventType(const int value);
		void setGate(const int value);
		void setBed(const int value);
		void setBedOffset(const float value);
		void setEndingBedOffset(const float value);
		void setVerticalBedOffset(const float value);
		void setDataFilePointer(const QString value);
		void setFrameStart(const float value);
		void setFrameDuration(const float value);
		void setScaleFactor(const float value);
		void setMinimum(const float value);
		void setMaximum(const float value);
		void setDeadtimeCorrection(const float value);
		void setDecayCorrection(const float value);
		void setPrompts(const QString value);
		void setDelays(const QString value);
		void setTrues(const QString value);
		void setPromptsRate(const int value);
		void setDelaysRate(const int value);

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
