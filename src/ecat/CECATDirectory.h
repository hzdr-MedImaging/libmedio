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

#ifndef CECATDIRECTORY_H
#define CECATDIRECTORY_H

#include <qintdict.h>
#include <qiodevice.h>
#include <qvaluevector.h>

#include "CECATDirectoryItem.h"

// forward declarations
class CECATFile;

class Q_EXPORT CECATDirectory : protected QIntDict<CECATDirectoryItem>
{
	public:
		CECATDirectory(CECATFile* ecatFile,
									 CECATSubHeader::Type subHeaderType =
											CECATSubHeader::Unknown);
		~CECATDirectory();

		// file i/o routines
		bool load(void);
		bool save(void) const;

		// accessor methods
		CECATDirectoryItem* getItem(short frame, short plane=1,
																short gate=1, short bed=0, short data=0);

		// methods to calculate the real amount
		// of frame/plane/gate numbers carried in the directory.
		short numFrames(void) const;
		short numPlanes(void) const;
		short numGates(void) const;
		short numBedPos(void) const;

		// read methods
		bool readSubHeader(CECATSubHeader*& subHeader, short frame, short plane=1,
											 short gate=1, short bed=0, short data=0);
		bool readMatrix(QByteArray*& matrixData, short frame, short plane=1, short gate=1,
										short bed=0, short data=0);
		bool readMatrix(char*& matrixData, unsigned int& len, short frame, short plane=1,
										short gate=1, short bed=0, short data=0);
	
		// mutabor methods
		bool writeSubHeader(const CECATSubHeader& subHeader, short frame, short plane=1,
												short gate=1, short bed=0, short data=0);
		bool writeMatrix(const QByteArray& matrix, short frame, short plane=1,
										 short gate=1, short bed=0, short data=0);
		bool writeMatrix(const char* matrix, unsigned int size, short frame,
										 short plane=1, short gate=1, short bed=0, short data=0);

	private:
		CECATFile*						m_pECATFile;	// ptr to our associated ECATFile
		CECATSubHeader::Type	m_iItemType;	// this directory manages
																				// items of this SubHeaderType
																				
		QValueVector<QIODevice::Offset>* m_pFilePositions; // for each DirList we
																											 // could have different file
																											 // positions which we have
																											 // to store
};

#endif // CECATDIRECTORY_H
