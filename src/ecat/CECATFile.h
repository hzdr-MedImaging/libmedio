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

// some useful defines on the ECAT file format
// especially for ECATBlock calculation as ECAT Files
// are seperated in 512 byte blocks and positions within
// the directorylist are refered in this format starting
// from 1.
#define ECAT_BLOCKSIZE				512
#define ECAT_POS_MAINHEADER		1		// the MainHeader is always at block 1
#define ECAT_POS_MAINDIR			2		// the MainDir is always at block 2
#define ECATBlock2FilePos(v)	(((v)-1)*ECAT_BLOCKSIZE)
#define FilePos2ECATBlock(v)	((v)/ECAT_BLOCKSIZE+1)

#include "CECATMainHeader.h"
#include "CECATSubHeader.h"

// forward declarations
class CECATDirectory;

class Q_EXPORT CECATFile : public QFile
{
	public:
		enum ECATFormat	{ Undefined=0, ECAT7, ECAT6 };

		CECATFile()
			: QFile(), m_ECATformat(Undefined), m_pMainHeader(NULL),
				m_pMainDirectory(NULL)
		{ }

		CECATFile(const QString& filename,
							CECATMainHeader::Type fileType = CECATMainHeader::Unknown);
		~CECATFile();

		bool load(void);
		bool save(void);

		ECATFormat format(void) { return m_ECATformat; }
		CECATMainHeader::Type fileType(void);
		CECATSubHeader::Type	subHeaderType(void);
		CECATSubHeader::Type	subHeaderType(CECATMainHeader::Type fileType);
		void setFileType(CECATMainHeader::Type fileType);

		// interface methods for directly obtaining information from
		// the mainheader
		CECATMainHeader* mainHeader(void) { return m_pMainHeader;									}
		short	numPlanes(void)							{ return m_pMainHeader->num_Planes();		}
		short numFrames(void)							{	return m_pMainHeader->num_Frames();		}
		short numGates(void)							{	return m_pMainHeader->num_Gates();		}
		short numBedPos(void)							{ return m_pMainHeader->num_Bed_Pos();	}

		// interface methods to get out a specific Matrix or
		// subheader from the ECATFile
		CECATSubHeader* getSubHeader(short frame=1, short plane=1, short gate=1,
																 short bed=0, short data=0);
		QByteArray* getMatrix(short frame=1, short plane=1, short gate=1,
												  short bed=0, short data=0);
		void* getMatrixData(short frame=1, short plane=1, short gate=1,
												short bed=0, short data=0);

		// methods to create new and modify existing entries in this file.
		CECATSubHeader* newEntry(short frame=1, short plane=1, short gate=1,
														 short bed=0, short data=0);
		bool setMatrix(QByteArray* matrix, short frame=1, short plane=1,
									 short gate=1, short bed=0, short data=0);
		bool setMatrixData(void* matrix, short frame=1, short plane=1,
											 short gate=1, short bed=0, short data=0);

	private:
		ECATFormat				m_ECATformat;

		// components of the ECAT file
		CECATMainHeader*	m_pMainHeader;
		CECATDirectory*		m_pMainDirectory;
};

#endif // CECATFILE_H
