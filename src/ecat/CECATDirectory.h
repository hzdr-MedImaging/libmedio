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

#include <QHash>
#include <QIODevice>
#include <QVector>

#ifndef __MEDIO_PRIVATE__
#include <CECATDirectoryItem>
#else
#include <CECATDirectoryItem.h>
#endif

// forward declarations
class CECATFile;

class CECATDirectory : protected QHash<int, CECATDirectoryItem*>
{
	public:
		CECATDirectory(CECATFile* ecatFile);
		~CECATDirectory();

		// file i/o routines
		bool load(void);
		bool save(void) const;

		// accessor methods
		bool isEmpty() const;
		unsigned int count() const;
		CECATDirectoryItem* operator[](long num) const;
		CECATDirectoryItem* item(short frame, short plane=1,
														 short gate=1, short bed=0, short data=0);

		// methods to calculate the real amount
		// of frame/plane/gate numbers carried in the directory.
		short numFrames(void) const;
		short numPlanes(void) const;
		short numGates(void) const;
		short numBedPos(void) const;

		// read methods
		bool readSubHeader(CECATSubHeader*& subHeader,
											 short frame, short plane=1, short gate=1, short bed=0, short data=0);
		bool readMatrix(QByteArray*& matrixData,
										short frame, short plane=1, short gate=1, short bed=0, short data=0);
		bool readMatrix(char*& matrixData, unsigned int& len,
										short frame, short plane=1, short gate=1, short bed=0, short data=0);
		bool readMatrix(QByteArray*& matrixData, CECATSubHeader*& subHeader,
										short frame, short plane=1, short gate=1, short bed=0, short data=0);
		bool readMatrix(char*& matrixData, unsigned int& len, CECATSubHeader*& subHeader,
										short frame, short plane=1, short gate=1, short bed=0, short data=0);	
	
		// write methods
		bool writeSubHeader(const CECATSubHeader& subHeader,
												short frame, short plane=1, short gate=1, short bed=0, short data=0);
		bool writeMatrix(const QByteArray& matrix,
										 short frame, short plane=1, short gate=1, short bed=0, short data=0);
		bool writeMatrix(const char* matrix, unsigned int size,
										 short frame, short plane=1, short gate=1, short bed=0, short data=0);
		bool writeMatrix(const QByteArray& matrix, CECATSubHeader::Data_Type type,
										 short frame, short plane=1, short gate=1, short bed=0, short data=0);
		bool writeMatrix(const char* matrix, unsigned int size, CECATSubHeader::Data_Type type,
										 short frame, short plane=1, short gate=1, short bed=0, short data=0);		
		bool writeMatrix(const QByteArray& matrix, const CECATSubHeader& subHeader,
										 short frame, short plane=1, short gate=1, short bed=0, short data=0);
		bool writeMatrix(const char* matrix, unsigned int size, const CECATSubHeader& subHeader,
										 short frame, short plane=1, short gate=1, short bed=0, short data=0);

#ifdef __MEDIO_PRIVATE__
	private:
		CECATFile* m_pECATFile;	// ptr to our associated ECATFile
																				
		QVector<qint64>* m_pFilePositions; // for each DirList we
																			 // could have different file
																			 // positions which we have
																			 // to store

		// some private methods 
		CECATDirectoryItem* newItem(quint32 matrixID);
		qint64 lastDirItemOffset(void) const;
#endif
};

#endif // CECATDIRECTORY_H
