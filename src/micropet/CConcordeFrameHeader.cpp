/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004 by Jens Langner <Jens.Langner@light-speed.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

#include "CMedIOHeader.h"
#include "CConcordeFrameHeader.h"
#include "CConcordeFile.h"
#include "CKeyParser.h"

#include <rtdebug.h>

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CConcordeFrameHeaderPrivate
{
	public:
		void init();

		QString		 fileName;
		CKeyParser parser;
		struct HeaderData
		{
			int			frame;
			int			event_type;
			int			gate;
			int			bed;
			float		bed_offset;
			float		ending_bed_offset;
			float		vertical_bed_offset;
			QString	data_file_pointer;
			float		frame_start;
			float		frame_duration;
			float		scale_factor;
			float		minimum;
			float		maximum;
			float		deadtime_correction;
			float		decay_correction;
			QString	prompts;
			QString	delays;
			QString	trues;
			int			prompts_rate;
			int			delays_rate;
			float		singles[168];
			float		rawsingles[168];
		} header;
};

//  Class: CConcordeFrameHeader
//  Constructor: CConcordeFrameHeader
//!
//! constructs a CConcordeFrameHeader object
//!
//! @param Filename: complete path to file holding header
//! @param frame: specific frame of header which holds information
//!
CConcordeFrameHeader::CConcordeFrameHeader(const QString& filename, int frame)
	: CMedIOHeader(NULL)
{
	ENTER();

	// allocate data from our private instance class
	m_pData = new CConcordeFrameHeaderPrivate();
	m_pData->fileName = filename;
	m_pData->header.frame = frame;
	m_pData->init();

	clear();
	
	if(!load())
	{
		D("Something is wrong with the headerfile");
	}
	else
		D("Everything ok");
	
	LEAVE();
}

CConcordeFrameHeader::CConcordeFrameHeader(CConcordeFile* file, int frame) 
	: CMedIOHeader(file)
{
	ENTER();	

	// allocate data from our private instance class
	m_pData = new CConcordeFrameHeaderPrivate();
	m_pData->fileName = file->fileName() + ".hdr";
	m_pData->header.frame = frame;
	m_pData->init();

	clear();
	
	if(!load())
	{
		D("Something is wrong with the headerfile");
	}
	else
		D("Everything ok");
	
	LEAVE();
}

CConcordeFrameHeader::~CConcordeFrameHeader()
{
	ENTER();

	delete m_pData;

	LEAVE();
}

CConcordeFrameHeader::CConcordeFrameHeader(const CConcordeFrameHeader& src)
	: CMedIOHeader(src)
{
	ENTER();

	// allocate data from our private instance class
	m_pData = new CConcordeFrameHeaderPrivate(*(src.m_pData));

	LEAVE();
}

CMedIOHeader& CConcordeFrameHeader::operator=(const CMedIOHeader& src)
{
	ENTER();

	this->convertFrom(&src);

	LEAVE();
	return *this;
}

void CConcordeFrameHeader::clear()
{
	m_pData->header.frame = 0;
	m_pData->header.event_type = 0;
	m_pData->header.gate = 0;
	m_pData->header.bed = 0;
	m_pData->header.bed_offset = 0.0F;
	m_pData->header.ending_bed_offset = 0.0F;
	m_pData->header.vertical_bed_offset = 0.0F;
	m_pData->header.data_file_pointer = QString();
	m_pData->header.frame_start = 0.0F;
	m_pData->header.frame_duration = 0.0F;
	m_pData->header.scale_factor = 0.0F;
	m_pData->header.minimum = 0.0F;
	m_pData->header.maximum = 0.0F;
	m_pData->header.deadtime_correction = 0.0F;
	m_pData->header.decay_correction = 0.0F;
	m_pData->header.prompts = QString();
	m_pData->header.delays = QString();
	m_pData->header.trues = QString();
	m_pData->header.prompts_rate = 0;
	m_pData->header.delays_rate = 0;
	memset(m_pData->header.singles, 0, 168*sizeof(float));
	memset(m_pData->header.rawsingles, 0, 168*sizeof(float));
}

//  Class: CConcordeFrameHeader
//  Method: init
//!
//! //initialise all keys which should be searched for in header file
//!
////////////////////////////////////////////////////////////////////////////////
void CConcordeFrameHeaderPrivate::init()
{
	ENTER();
	parser.addSeparator(" ");
	parser.addComment("#");
	int real_frame_in_file = header.frame;
	QString tmp("frame ");
	tmp = tmp + QString::number(real_frame_in_file);
	D("Frame: %s", tmp.toAscii().data());
	parser.addStartSymbol(tmp);
	parser.addStopSymbol("end_of_header");
	
	parser.addKey("frame", &header.frame);
	parser.addKey("event_type", &header.event_type);
	parser.addKey("gate", &header.gate);
	parser.addKey("bed", &header.bed);
	parser.addKey("bed_offset", &header.bed_offset);
	parser.addKey("ending_bed_offset", &header.ending_bed_offset);
	parser.addKey("vertical_bed_offset", &header.vertical_bed_offset);
	parser.addKey("data_file_pointer", &header.data_file_pointer);
	parser.addKey("frame_start", &header.frame_start);
	parser.addKey("frame_duration", &header.frame_duration);
	parser.addKey("scale_factor", &header.scale_factor);
	parser.addKey("minimum", &header.minimum);
	parser.addKey("maximum", &header.maximum);
	parser.addKey("deadtime_correction", &header.deadtime_correction);
	parser.addKey("decay_correction", &header.decay_correction);
	parser.addKey("prompts", &header.prompts);
	parser.addKey("delays", &header.delays);
	parser.addKey("trues", &header.trues);
	parser.addKey("prompts_rate", &header.prompts_rate);
	parser.addKey("delays_rate", &header.delays_rate);

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
	D("Start Parsing File %s for Frame: %d", m_pData->fileName.toAscii().data(), m_pData->header.frame);
	if(m_pData->parser.parse(m_pData->fileName))
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
	return m_pData->header.frame;
}

int CConcordeFrameHeader::eventType(void) const 
{
	return m_pData->header.event_type;
}

int CConcordeFrameHeader::gate(void) const 
{
	return m_pData->header.gate;
}

int CConcordeFrameHeader::bed(void) const 
{
	return m_pData->header.bed;
}

float CConcordeFrameHeader::bedOffset(void) const 
{
	return m_pData->header.bed_offset;
}

float CConcordeFrameHeader::endingBedOffset(void) const 
{
	return m_pData->header.ending_bed_offset;
}

float CConcordeFrameHeader::verticalBedOffset(void) const 
{
	return m_pData->header.vertical_bed_offset;
}

QString CConcordeFrameHeader::dataFilePointer(void) const 
{
	return m_pData->header.data_file_pointer;
}

float CConcordeFrameHeader::frameStart(void) const 
{
	return m_pData->header.frame_start;
}

float CConcordeFrameHeader::frameDuration(void) const 
{
	return m_pData->header.frame_duration;
}

float CConcordeFrameHeader::scaleFactor(void) const 
{
	return m_pData->header.scale_factor;
}

float CConcordeFrameHeader::minimum(void) const 
{
	return m_pData->header.minimum;
}

float CConcordeFrameHeader::maximum(void) const 
{
	return m_pData->header.maximum;
}

float CConcordeFrameHeader::deadTimeCorrection(void) const 
{
	return m_pData->header.deadtime_correction;
}

float CConcordeFrameHeader::decayCorrection(void) const 
{
	return m_pData->header.decay_correction;
}

QString CConcordeFrameHeader::prompts(void) const 
{
	return m_pData->header.prompts;
}

QString CConcordeFrameHeader::delays(void) const 
{
	return m_pData->header.delays;
}

QString CConcordeFrameHeader::trues(void) const
{
	return m_pData->header.trues;
}

int CConcordeFrameHeader::promptsRate(void) const 
{
	return m_pData->header.prompts_rate;
}

int CConcordeFrameHeader::delaysRate(void) const 
{
	return m_pData->header.delays_rate;
}

float CConcordeFrameHeader::single(int i) const 
{
	return m_pData->header.singles[i];
}

float CConcordeFrameHeader::rawSingle(int i) const 
{
	return m_pData->header.rawsingles[i];
}

//mutator methods
void CConcordeFrameHeader::setFrame(const int value)
{ 
	m_pData->header.frame = value;
}

void CConcordeFrameHeader::setEventType(const int value) 
{ 
	m_pData->header.event_type = value;
}

void CConcordeFrameHeader::setGate(const int value) 
{ 
	m_pData->header.gate = value;
}

void CConcordeFrameHeader::setBed(const int value) 
{ 
	m_pData->header.bed = value;
}

void CConcordeFrameHeader::setBedOffset(const float value) 
{ 
	m_pData->header.bed_offset = value;
}

void CConcordeFrameHeader::setEndingBedOffset(const float value) 
{ 
	m_pData->header.ending_bed_offset = value;
}

void CConcordeFrameHeader::setVerticalBedOffset(const float value) 
{ 
	m_pData->header.vertical_bed_offset = value;
}

void CConcordeFrameHeader::setDataFilePointer(const QString value) 
{ 
	m_pData->header.data_file_pointer = value;
}

void CConcordeFrameHeader::setFrameStart(const float value) 
{ 
	m_pData->header.frame_start = value;
}

void CConcordeFrameHeader::setFrameDuration(const float value)
{
	 m_pData->header.frame_duration = value;
}

void CConcordeFrameHeader::setScaleFactor(const float value)
{
	m_pData->header.scale_factor = value;
}

void CConcordeFrameHeader::setMinimum(const float value)
{
	m_pData->header.minimum = value;
}

void CConcordeFrameHeader::setMaximum(const float value)
{
	m_pData->header.maximum = value;
}

void CConcordeFrameHeader::setDeadtimeCorrection(const float value)
{
	m_pData->header.deadtime_correction = value;
}

void CConcordeFrameHeader::setDecayCorrection(const float value)
{
	m_pData->header.decay_correction = value;
}

void CConcordeFrameHeader::setPrompts(const QString value)
{
	m_pData->header.prompts = value;
}

void CConcordeFrameHeader::setDelays(const QString value)
{
	m_pData->header.delays = value;
}

void CConcordeFrameHeader::setTrues(const QString value)
{
	m_pData->header.trues = value;
}

void CConcordeFrameHeader::setPromptsRate(const int value)
{
	m_pData->header.prompts_rate = value;
}

void CConcordeFrameHeader::setDelaysRate(const int value)
{
	m_pData->header.delays_rate = value;
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
	bool bResult = false;
	if(srcMainHeader)
		bResult = copyData(srcMainHeader);
	return bResult;
}

bool CConcordeFrameHeader::copyData(const CMedIOHeader* src)
{
	bool bResult = false;
	ENTER();
	if(src)
	{
		switch(src->headerFormat())
		{
			case CMedIOHeader::ConcordeMicroPetFrameHeader:
			{
				m_pData->header.frame = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.frame;
				m_pData->header.event_type = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.event_type;
				m_pData->header.gate = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.gate;
				m_pData->header.bed = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.bed;
				m_pData->header.bed_offset = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.bed_offset;
				m_pData->header.ending_bed_offset = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.ending_bed_offset;
				m_pData->header.vertical_bed_offset = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.vertical_bed_offset;
				m_pData->header.data_file_pointer = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.data_file_pointer;
				m_pData->header.frame_start = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.frame_start;
				m_pData->header.frame_duration = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.frame_duration;
				m_pData->header.scale_factor = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.scale_factor;
				m_pData->header.minimum = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.minimum;
				m_pData->header.maximum = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.maximum;
				m_pData->header.deadtime_correction = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.deadtime_correction;
				m_pData->header.decay_correction = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.decay_correction;
				m_pData->header.prompts = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.prompts;
				m_pData->header.delays = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.delays;
				m_pData->header.trues = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.trues;
				m_pData->header.prompts_rate = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.prompts_rate;
				m_pData->header.delays_rate = static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.delays_rate;
				memcpy(m_pData->header.singles, static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.singles, 168*sizeof(float));
				memcpy(m_pData->header.rawsingles, static_cast<const CConcordeFrameHeader*>(src)->m_pData->header.rawsingles, 168*sizeof(float));
				bResult = true;
			}
			break;
			default:
			{
				E("File format not supported yet");
				bResult = false;
			}
			break;
		}
	}
	RETURN(bResult);
	return bResult;
}
