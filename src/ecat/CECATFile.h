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

#ifndef CECATFILE_H
#define CECATFILE_H

#include <qfile.h>
#include <qstring.h>

#include <CMedIOData.h>
#include <CECATDirectory.h>
#include <CECATMainHeader.h>
#include <CECATSubHeader.h>

// forward declarations

class Q_EXPORT CECATFile : public CMedIOData
{
	public:
		enum ECATFormat	{ Undefined=0, ECAT7, ECAT6 };

		// all our methods return specific error messages
		// so we have different defined error messages here
		enum Status { SUCCESS			= 1,	// everything went fine
									ERR_GENERAL = 0,	// a general error occurred
									ERR_IO			= -1	// an input/output error occurred
								};

		CECATFile();
		CECATFile(const QString& filename,
							CECATMainHeader::Type fileType = CECATMainHeader::Unknown);
		~CECATFile();
		
		// static methods for the MedIOData interface
		static bool isOfType(const QString& fileName);
		static CMedIOData* createFromFile(const QString& fileName);

		// runtime type information
		int rtti() const { return CMedIOData::ECAT; }

		bool open(int mode);
		void close(void);

		ECATFormat format(void) const { return m_iECATformat; }
		CECATMainHeader::Type fileType(void);
		bool setFileType(CECATMainHeader::Type fileType);

		// methods to query some information on the subHeader type
		CECATSubHeader::Type subHeaderType(void);
		CECATSubHeader::Type subHeaderType(CECATMainHeader::Type fileType);

		// methods to calculate the real amount
		// of frame/plane/gate numbers carried in the directory.
		short numFrames(void) const { return m_pMainDirectory->numFrames();	}
		short numPlanes(void) const	{ return m_pMainDirectory->numPlanes();	}
		short numGates(void) const	{ return m_pMainDirectory->numGates();	}
		short numBedPos(void) const	{ return m_pMainDirectory->numBedPos();	}

		// interface methods to read out specific data from the ECAT files
		bool readMainHeader(CECATMainHeader*&);
		bool readSubHeader(CECATSubHeader*&,
												 short frame, short plane=1, short gate=1, short bed=0, short data=0);
		bool readMatrix(QByteArray*& matrixData,
											short frame, short plane=1, short gate=1, short bed=0, short data=0);
		bool readMatrix(char*& matrixData, unsigned int& len,
											short frame, short plane=1, short gate=1, short bed=0, short data=0);

		// methods to create new and modify existing entries in this file.
		bool writeMainHeader(CECATMainHeader& mainHeader);
		bool writeSubHeader(const CECATSubHeader& subHeader,
													short frame, short plane=1, short gate=1, short bed=0, short data=0);
		bool writeMatrix(const QByteArray& matrixData,
											 short frame, short plane=1, short gate=1, short bed=0, short data=0);
		bool writeMatrix(const char* matrixData, unsigned int size,
											 short frame, short plane=1, short gate=1, short bed=0, short data=0);
		bool writeMatrix(const QByteArray& matrixData, CECATSubHeader::Data_Type type, 
											 short frame, short plane=1, short gate=1, short bed=0, short data=0);
		bool writeMatrix(const char* matrixData, unsigned int size, CECATSubHeader::Data_Type type,
											 short frame, short plane=1, short gate=1, short bed=0, short data=0);

		// methods to sync specific data with our headers
		void mainHeaderWritten(const CECATMainHeader& mainHeader);
		bool reWriteMainHeader(void);

	private:
		ECATFormat						m_iECATformat;
		CECATMainHeader::Type	m_iMainHeaderType;
		CECATDirectory*				m_pMainDirectory;
		CECATMainHeader*			m_pCachedMainHeader; // for speed reasons we cache the loaded main header
};

#endif // CECATFILE_H
