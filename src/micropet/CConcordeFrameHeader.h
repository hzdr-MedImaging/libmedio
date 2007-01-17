/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004-2007 by Jens Langner <Jens.Langner@light-speed.de>

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

//! @file CConcordeFrameHeader.h
//! @author Hagen Moelle
//! @date 11/13/2004
//! @brief contains definition of the class CConcordeFrameHeader

#ifndef CCONCORDEFRAMEHEADER_H
#define CCONCORDEFRAMEHEADER_H

#include <qstring.h>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOHeader>
#else
#include <CMedIOHeader.h>
#endif

// forward declarations
class CConcordeFrameHeaderPrivate;
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
		//! @brief constructor
		//! @param Filename: complete path to file holding header
		//! @param frame: specific frame of header which holds information
		CConcordeFrameHeader(const QString& filename, int frame);
		CConcordeFrameHeader(CConcordeFile* file, int frame);

		//! @brief destructor
		~CConcordeFrameHeader();

		// copy constructur and default assignment operator
		CConcordeFrameHeader(const CConcordeFrameHeader& src);		
		CMedIOHeader& operator=(const CMedIOHeader& src);
		
		// header clear method
		void clear();

		enum EventType{UnknownEventType = 0, Singles, Prompt, Delay, Trues};
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
		bool copyData(const CMedIOHeader* src);	

	private:
		CConcordeFrameHeaderPrivate* m_pData;
};

#endif
