/* vim:set ts=2 nowrap: ****************************************************

 LMMC - ListMode Movement Correction
 Copyright (C) 2003 by Jens Langner <Jens.Langner@light-speed.de>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

#ifndef CECATDIRECTORY_H
#define CECATDIRECTORY_H

#include <qintdict.h>

#include "CECATDirectoryItem.h"

// forward declarations
class CECATFile;

class CECATDirectory
{
	public:
		CECATDirectory(CECATSubHeader::Type subHeaderType = CECATSubHeader::Unknown);

		// file i/o routines
		bool load(CECATFile* pFile);
		bool save(CECATFile* pFile);

		// accessor methods
		CECATDirectoryItem* getItem(short frame=1, short plane=1, short gate=1, short bed=0, short data=0);
		CECATSubHeader* getSubHeader(short frame=1, short plane=1, short gate=1, short bed=0, short data=0);
		QByteArray* getMatrix(short frame=1, short plane=1, short gate=1, short bed=0, short data=0);
		void* getMatrixData(short frame=1, short plane=1, short gate=1, short bed=0, short data=0);
	
		// mutabor methods
		CECATSubHeader* newEntry(short frame=1, short plane=1, short gate=1, short bed=0, short data=0);
		bool setMatrix(QByteArray* matrix, short frame=1, short plane=1, short gate=1, short bed=0, short data=0);
		bool setMatrixData(void* matrix, unsigned int size, short frame=1, short plane=1, short gate=1, short bed=0, short data=0);

	private:
		// private data
		QIntDict<CECATDirectoryItem>	m_ItemDict;			// storage for our directory items
		CECATSubHeader::Type					m_ItemType;			// this directory manages items of this SubHeaderType

		// for convience in load/saving the binary
		// data we specify some helper structures
		// here which we use within the loading and
		// saving routines.
		#pragma pack(2)
		struct DirHead
		{
			Q_UINT32 FreeItems;
			Q_UINT32 Next;
			Q_UINT32 Prev;
			Q_UINT32 ItemsToFollow;
		};

		struct DirItem
		{
			Q_UINT32 matrixID;
			Q_UINT32 dataBlock_Start;
			Q_UINT32 dataBlock_End;
			Q_UINT32 matrixStatus;
		};

		struct DirList // should be 512 bytes
		{
			struct DirHead head;
			struct DirItem items[31];
		};
		#pragma pack()

		// private methods
		friend QDataStream& operator<<(QDataStream& stream, const struct DirList& dirList);	
};

#endif // CECATDIRECTORY_H
