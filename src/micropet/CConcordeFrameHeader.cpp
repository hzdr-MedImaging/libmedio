#include <rtdebug.h>
#include "CMedIOHeader.h"
#include "CConcordeFrameHeader.h"

//  Class: CConcordeFrameHeader
//  Constructor: CConcordeFrameHeader
//!
//! constructs a CConcordeFrameHeader object
//!
//! @param Filename: complete path to file holding header
//! @param frame: specific frame of header which holds information
//!
CConcordeFrameHeader::CConcordeFrameHeader(QString Filename, int frame)
{
	ENTER();
	File = Filename;
	m_Data.frame = frame;
	init();
	if(!this->load())
	{
		D("Something is wrong with the headerfile");
	}
	else
		D("Everything ok");
	LEAVE();
}

CConcordeFrameHeader::CConcordeFrameHeader(CConcordeFile* file, QString Filename, int frame) : CMedIOHeader(NULL)
{
	ENTER();	
	m_pMedIOData = file;
	File = Filename;
	m_Data.frame = frame;
	init();
	if(!this->load())
	{
		D("Something is wrong with the headerfile");
	}
	else
		D("Everything ok");
	LEAVE();
}

//  Class: CConcordeFrameHeader
//  Method: init
//!
//! //initialise all keys which should be searched for in header file
//!
////////////////////////////////////////////////////////////////////////////////
void CConcordeFrameHeader::init()
{
	ENTER();
	Parser.addSeparator(" ");
	Parser.addComment("#");
	int real_frame_in_file = m_Data.frame;
	QString tmp("frame ");
	tmp = tmp + QString::number(real_frame_in_file);
	D("Frame: %s", tmp.toAscii().data());
	Parser.addStartSymbol(tmp);
	Parser.addStopSymbol("end_of_header");
	
	Parser.addKey("frame", &m_Data.frame);
	Parser.addKey("event_type", &m_Data.event_type);
	Parser.addKey("gate", &m_Data.gate);
	Parser.addKey("bed", &m_Data.bed);
	Parser.addKey("bed_offset", &m_Data.bed_offset);
	Parser.addKey("ending_bed_offset", &m_Data.ending_bed_offset);
	Parser.addKey("vertical_bed_offset", &m_Data.vertical_bed_offset);
	Parser.addKey("data_file_pointer", &m_Data.data_file_pointer);
	Parser.addKey("frame_start", &m_Data.frame_start);
	Parser.addKey("frame_duration", &m_Data.frame_duration);
	Parser.addKey("scale_factor", &m_Data.scale_factor);
	Parser.addKey("minimum", &m_Data.minimum);
	Parser.addKey("maximum", &m_Data.maximum);
	Parser.addKey("deadtime_correction", &m_Data.deadtime_correction);
	Parser.addKey("decay_correction", &m_Data.decay_correction);
	Parser.addKey("prompts", &m_Data.prompts);
	Parser.addKey("delays", &m_Data.delays);
	Parser.addKey("trues", &m_Data.trues);
	Parser.addKey("prompts_rate", &m_Data.prompts_rate);
	Parser.addKey("delays_rate", &m_Data.delays_rate);
	//Parser.addKey("singles", m_Data.singles);
	LEAVE();
}

//  Class: CConcordeFrameHeader
//  Method: load
//!
//! loades the headerinformation.
//!
//! @return true on success otherwise false
////////////////////////////////////////////////////////////////////////////////
bool CConcordeFrameHeader::load()
{
	D("Start Parsing File %s for Frame: %d", File.toAscii().data(), m_Data.frame);
	if(Parser.parse(File))
		return true;
	else
		return false;
}

//  Class: CConcordeFrameHeader
//  Method: save
//!
//! Saves the headerinformation.
//!
//! @return true on success otherwise false
////////////////////////////////////////////////////////////////////////////////
bool CConcordeFrameHeader::save() const
{
	return true;
}

CMedIOHeader::Format CConcordeFrameHeader::headerFormat() const
{
	return CMedIOHeader::ConcordeMicroPetFrameHeader;
}

CMedIOHeader& CConcordeFrameHeader::copyData(const CMedIOHeader& src)
{
	ENTER();
	RETURN(this);
	return *this;
}
