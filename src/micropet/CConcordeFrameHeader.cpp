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

CConcordeFrameHeader::CConcordeFrameHeader(CConcordeFile* file, int frame) : CMedIOHeader(NULL)
{
	ENTER();	
	m_pMedIOData = file;
	File = file->fileName() + ".hdr";
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

CMedIOData* CConcordeFrameHeader::fileObject() const
{
	return m_pMedIOData;
}

void CConcordeFrameHeader::clear()
{
	memset(&m_Data, 0, sizeof(ConcordeHeaderFrame));
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
	D("Frame: %s", tmp.ascii());
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
	D("Start Parsing File %s for Frame: %d", File.ascii(), m_Data.frame);
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

//accessor methods
int CConcordeFrameHeader::frame(void) const 
{
	return m_Data.frame;
}

int CConcordeFrameHeader::eventType(void) const 
{
	return m_Data.event_type;
}

int CConcordeFrameHeader::gate(void) const 
{
	return m_Data.gate;
}

int CConcordeFrameHeader::bed(void) const 
{
	return m_Data.bed;
}

float CConcordeFrameHeader::bedOffset(void) const 
{
	return m_Data.bed_offset;
}

float CConcordeFrameHeader::endingBedOffset(void) const 
{
	return m_Data.ending_bed_offset;
}

float CConcordeFrameHeader::verticalBedOffset(void) const 
{
	return m_Data.vertical_bed_offset;
}

QString CConcordeFrameHeader::dataFilePointer(void) const 
{
	return m_Data.data_file_pointer;
}

float CConcordeFrameHeader::frameStart(void) const 
{
	return m_Data.frame_start;
}

float CConcordeFrameHeader::frameDuration(void) const 
{
	return m_Data.frame_duration;
}

float CConcordeFrameHeader::scaleFactor(void) const 
{
	return m_Data.scale_factor;
}

float CConcordeFrameHeader::minimum(void) const 
{
	return m_Data.minimum;
}

float CConcordeFrameHeader::maximum(void) const 
{
	return m_Data.maximum;
}

float CConcordeFrameHeader::deadTimeCorrection(void) const 
{
	return m_Data.deadtime_correction;
}

float CConcordeFrameHeader::decayCorrection(void) const 
{
	return m_Data.decay_correction;
}

QString CConcordeFrameHeader::prompts(void) const 
{
	return m_Data.prompts;
}

QString CConcordeFrameHeader::delays(void) const 
{
	return m_Data.delays;
}

QString CConcordeFrameHeader::trues(void) const
{
	return m_Data.trues;
}

int CConcordeFrameHeader::promptsRate(void) const 
{
	return m_Data.prompts_rate;
}

int CConcordeFrameHeader::delaysRate(void) const 
{
	return m_Data.delays_rate;
}

float CConcordeFrameHeader::single(int i) const 
{
	return m_Data.singles[i];
}

float CConcordeFrameHeader::rawSingle(int i) const 
{
	return m_Data.rawsingles[i];
}

//mutator methods
void CConcordeFrameHeader::setFrame(const int value)
{ 
	m_Data.frame = value;
}

void CConcordeFrameHeader::setEventType(const int value) 
{ 
	m_Data.event_type = value;
}

void CConcordeFrameHeader::setGate(const int value) 
{ 
	m_Data.gate = value;
}

void CConcordeFrameHeader::setBed(const int value) 
{ 
	m_Data.bed = value;
}

void CConcordeFrameHeader::setBedOffset(const float value) 
{ 
	m_Data.bed_offset = value;
}

void CConcordeFrameHeader::setEndingBedOffset(const float value) 
{ 
	m_Data.ending_bed_offset = value;
}

void CConcordeFrameHeader::setVerticalBedOffset(const float value) 
{ 
	m_Data.vertical_bed_offset = value;
}

void CConcordeFrameHeader::setDataFilePointer(const QString value) 
{ 
	m_Data.data_file_pointer = value;
}

void CConcordeFrameHeader::setFrameStart(const float value) 
{ 
	m_Data.frame_start = value;
}

void CConcordeFrameHeader::setFrameDuration(const float value)
{
	 m_Data.frame_duration = value;
}

void CConcordeFrameHeader::setScaleFactor(const float value)
{
	m_Data.scale_factor = value;
}

void CConcordeFrameHeader::setMinimum(const float value)
{
	m_Data.minimum = value;
}

void CConcordeFrameHeader::setMaximum(const float value)
{
	m_Data.maximum = value;
}

void CConcordeFrameHeader::setDeadtimeCorrection(const float value)
{
	m_Data.deadtime_correction = value;
}

void CConcordeFrameHeader::setDecayCorrection(const float value)
{
	m_Data.decay_correction = value;
}

void CConcordeFrameHeader::setPrompts(const QString value)
{
	m_Data.prompts = value;
}

void CConcordeFrameHeader::setDelays(const QString value)
{
	m_Data.delays = value;
}

void CConcordeFrameHeader::setTrues(const QString value)
{
	m_Data.trues = value;
}

void CConcordeFrameHeader::setPromptsRate(const int value)
{
	m_Data.prompts_rate = value;
}

void CConcordeFrameHeader::setDelaysRate(const int value)
{
	m_Data.delays_rate = value;
}

CMedIOHeader* CConcordeFrameHeader::clone() const
{
	CConcordeFrameHeader* pTmp = new CConcordeFrameHeader(NULL, frame());
	if(pTmp->convertFrom(this))
		return pTmp;
	else
		return NULL;
}

bool CConcordeFrameHeader::convertFrom(const CMedIOHeader* srcMainHeader, const CMedIOHeader*)
{
	copyData(*srcMainHeader);
	return true;
}

CMedIOHeader& CConcordeFrameHeader::copyData(const CMedIOHeader&)
{
	ENTER();
	RETURN(this);
	return *this;
}
