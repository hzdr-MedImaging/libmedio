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

#include "CECATDirectory.h"
#include "CECATDirectoryItem.h"
#include "CECATFile.h"

#include <qdatastream.h>
#include <qintdict.h>

#include "debug.h"

CECATDirectory::CECATDirectory(CECATSubHeader::Type subHeaderType)
	: m_ItemType(subHeaderType)
{
	// do some debugging to make sure that our structure really
	// have the correct width
	ASSERT(sizeof(struct DirHead) == 16);		// should be 16bytes
	ASSERT(sizeof(struct DirItem) == 16);		// should be 16bytes
	ASSERT(sizeof(struct DirList) == 512);	// a dirlist should always be 512 bytes

	m_ItemDict.setAutoDelete(true);
}

bool CECATDirectory::load(CECATFile* pFile)
{
	bool result = true;

	// open the file read-only now
	pFile->open(IO_ReadOnly);

	// before we load the directory information we set the
	// fileposition accordingly to the normal position of the
	// main directory.
	pFile->at(ECATBlock2FilePos(ECAT_POS_MAINDIR));

	// let us create some anonymous structs so that we can 
	// read out the data from the ECAT file
	struct DirHead head;

	// first we generate a datastream and
	// read out the Directory Head so that we know
	// how many items will follow
	QDataStream stream(pFile);

	// set the ItemType to the new SubHeaderType we are going
	// to load now, just in case someone wants to create more of
	// those items.
	m_ItemType = pFile->subHeaderType();

	// we process the splitted directory list in a do..while loop
	// and jump from one dirlist to another in the ecat file.
	do
	{
		// read out each item of the DirHead with the stream
		// operations of QT so that it is portable between different
		// endianess.
		stream >> head.FreeItems;
		stream >> head.Next;
		stream >> head.Prev;
		stream >> head.ItemsToFollow;
		
		// output some debug information.
		D("DirHead.FreeItems: %d", head.FreeItems);
		D("DirHead.Next			: %d", head.Next);
		D("DirHead.Prev			: %d", head.Prev);
		D("DirHead.ItemToFol: %d", head.ItemsToFollow);

		// now we know how many item will follow in the Directory, so we
		// can populate our Directory here.
		int iItemsInserted = 0;
		for(unsigned int i=0; i < head.ItemsToFollow; i++)
		{
			CECATDirectoryItem* pNewDirItem = new CECATDirectoryItem(m_ItemType);

			// let us read out our information directly from the
			// stream
			pNewDirItem->loadFromFile(pFile, pFile->at());

			// before we add the item to the directory we check if the
			// values are somehow valid
			if(pNewDirItem->getMatrixID() > 0)
			{
				// insert the new DirectoryItem in the ItemDictonary
				// with using the MatrixID as the unique key for it and of
				// course we make sure no other item with the same ID exists or
				// we will run into trouble.
				ASSERT(m_ItemDict.find(pNewDirItem->getMatrixID()) == 0);
				m_ItemDict.insert(pNewDirItem->getMatrixID(), pNewDirItem);

				iItemsInserted++;
			}
			else
			{
				delete pNewDirItem;
				break;
			}
		}

		#if defined(DEBUG)
		if(head.ItemsToFollow != m_ItemDict.count())
		{
			W("ItemsToFollow:%ld != iItemsInserted:%ld", head.ItemsToFollow, iItemsInserted);
		}
		#endif

		pFile->at(ECATBlock2FilePos(head.Next));
	}
	while(head.Next > ECAT_POS_MAINDIR);
	
	// and last, but not least close the file now
	pFile->close();

	return result;
}

bool CECATDirectory::save(CECATFile* pFile)
{
	// open the file for writing
	pFile->open(IO_ReadWrite);
	QDataStream stream(pFile);

	// now we have to go through our directory and stream all items
	// in 31 chunks as the directory list can only have 31 items plus
	// 1 chunk for the header
	int frame=1;
	int plane=1;
	int gate=1;
	int bed=0;
	int data=0;

	// we need to populate the dirList a bit ordered
	// frame->plane->gate->bed->data
	// even if the ECAT standard doesn't define a ordering, but some
	// stupid software seems to require it.
	struct DirList dirList;

	// before we start we have to relink the first dirList we produce
	// to itself
	memset(&dirList, 0, sizeof(struct DirList)); // clear it first
	dirList.head.FreeItems	= 31;
	dirList.head.Next				= ECAT_POS_MAINDIR;

	// so before we process our DirItem list we write out this DirList
	// to the end of the file
	pFile->at(ECATBlock2FilePos(ECAT_POS_MAINDIR));

	// to maintain correct endianess, we write out each DirList item
	// seperatly
	stream << dirList;

	short depth=4; // we have five depth levels (4==data, 3==bed, 2==gate, 1==plane, 0==frame)
	short currentDirPos = ECAT_POS_MAINDIR;
	short nextDirPos;
	do
	{
		CECATDirectoryItem* pDirItem =
			m_ItemDict.find(convertToMatrixID(frame, plane, gate, bed, data));

		if(pDirItem)
		{
			// DO THE JOB !!!
			D("do: %d/%d/%d/%d/%d", frame, plane, gate, bed, data);
			// first we check wheter the dirList is filled up (>31 items)
			// and then write it out separate first.
			if(dirList.head.ItemsToFollow >= 31)
			{
				// get the position where the next directorylist will start
				nextDirPos = FilePos2ECATBlock(pFile->at());
				dirList.head.Next = nextDirPos;

				// now that we have build the new one, we can write out the old one
				pFile->at(ECATBlock2FilePos(currentDirPos));

				// to maintain correct endianess, we write out each DirList item
				// seperatly
				stream << dirList;
				
				// clear the dirList so that we can immediately reuse it
				memset(&dirList, 0, sizeof(struct DirList));
				dirList.head.FreeItems	= 31;
				dirList.head.Next				= ECAT_POS_MAINDIR;
				dirList.head.Prev				= currentDirPos;
				currentDirPos						= nextDirPos;

				// now that we have build the new one, we can write it out before
				// we add more matrix data
				pFile->at(ECATBlock2FilePos(nextDirPos));

				// to maintain correct endianess, we write out each DirList item
				// seperatly
				stream << dirList;
			}

			// set the dirList entry for the pDirItem so that we
			// can save it later on
			short curItem = dirList.head.ItemsToFollow;
			dirList.items[curItem].matrixID = convertToMatrixID(frame, plane, gate, bed, data);
			dirList.items[curItem].dataBlock_Start = FilePos2ECATBlock(pFile->at());

			// now write out the data to the file
			pDirItem->saveToFile(pFile);
						
			// now recall the end position of the datablockl
			dirList.items[curItem].dataBlock_End = FilePos2ECATBlock(pFile->at())-1;
			dirList.items[curItem].matrixStatus = CECATDirectoryItem::Finished;

			// increment the ItemsToFollow value and decrement the 
			// freeItems
			dirList.head.ItemsToFollow++;
			dirList.head.FreeItems--;
					
			if(depth < 4) depth++;	
			if(depth == 4)			data++;
			else if(depth == 3) bed++;
			else if(depth == 2) gate++;
			else if(depth == 1) plane++;
			else if(depth == 0) frame++;			
		}
		else
		{
			if(depth == 0)			break;
			else if(depth == 1) { frame++;	plane=1; }
			else if(depth == 2) { plane++;	gate=1;	 }
			else if(depth == 3) { gate++;		bed=0;	 }
			else if(depth == 4) { bed++;		data=0;	 }
			
			depth--;
		}

	}
	while(1);

	// write out the last DirectoryList
	pFile->at(ECATBlock2FilePos(currentDirPos));

	// to maintain correct endianess, we write out each DirList item
	// seperatly
	stream << dirList;

	pFile->close();

	return true;
}

CECATDirectoryItem* CECATDirectory::getItem(short frame, short plane,
																						short gate, short bed, short data)
{
	// form the MatrixID from the supplied data and
	// process a find in the QIntDict
	unsigned int mID = convertToMatrixID(frame, plane, gate, bed, data);
	D("Generated MatrixID: %08lx", mID);

	return m_ItemDict.find(mID);
}

CECATSubHeader* CECATDirectory::getSubHeader(short frame, short plane,
																						 short gate, short bed, short data)
{
	// get the directoryItem so that we can query the matrix from it
	CECATDirectoryItem* pDirItem = getItem(frame, plane, gate, bed, data);

	if(pDirItem)
	{
		return pDirItem->getSubHeader();
	}

	return NULL;
}

QByteArray* CECATDirectory::getMatrix(short frame, short plane,
																			short gate, short bed, short data)
{
	// get the directoryItem so that we can query the matrix from it
	CECATDirectoryItem* pDirItem = getItem(frame, plane, gate, bed, data);

	if(pDirItem)
	{
		return pDirItem->getMatrix();
	}

	return NULL;
}

void* CECATDirectory::getMatrixData(short frame, short plane,
																		short gate, short bed, short data)
{
	QByteArray* matrixData = getMatrix(frame, plane, gate, bed, data);

	if(matrixData)
		return matrixData->data();
		
	return NULL;
}

CECATSubHeader* CECATDirectory::newEntry(short frame, short plane,
																				 short gate, short bed, short data)
{
	ENTER();

	// form the MatrixID from the supplied data and create a new
	// DirectoryItem (replace a existing one)
	unsigned int mID = convertToMatrixID(frame, plane, gate, bed, data);
	D("Generated MatrixID: %08lx", mID);

	// create a new DirectoryItem which we put in this Directory
	CECATDirectoryItem* pNewDItem = new CECATDirectoryItem(m_ItemType);

	#if defined(DEBUG)
	if(m_ItemDict.find(mID))
	{
		W("Item with mID: %08lx already exists in Dir..replacing..\n", mID);
	}
	#endif

	m_ItemDict.replace(mID, pNewDItem);

	CECATSubHeader* result = pNewDItem->getSubHeader();

	RETURN(result);
	return result;
}

bool CECATDirectory::setMatrix(QByteArray* matrix, short frame, short plane,
															 short gate, short bed, short data)
{
	ENTER();

	// get the directoryItem so that we can query the matrix from it
	CECATDirectoryItem* pDirItem = getItem(frame, plane, gate, bed, data);

	if(pDirItem)
	{
		pDirItem->setMatrix(matrix);

		RETURN(true);
		return true;
	}

	RETURN(false);
	return false;
}

bool CECATDirectory::setMatrixData(void* matrix, unsigned int size, short frame,
																	 short plane, short gate, short bed, short data)
{
	// get the directoryItem so that we can query the matrix from it
	CECATDirectoryItem* pDirItem = getItem(frame, plane, gate, bed, data);

	if(pDirItem)
	{
		pDirItem->setMatrixData(matrix, size);
		return true;
	}
	
	return false;
}

//  Operator:    <<
//! 
//! Stream operator overloading that allows to write out all ECATDirectory
//! relevant data to a stream, taking care for the correct endianess throughout
//! the whole process.
//! 
//! @param       stream reference to the QDataStream to write to.
//! @param       dirList reference to the directory list to write out
//! @return      the reference to the initial stream
////////////////////////////////////////////////////////////////////////////////
QDataStream& operator<<(QDataStream& stream, const struct CECATDirectory::DirList& dirList)
{
	// lets write out the directory list stepwise as we have to take care
	// of the correct endianess.
	stream << dirList.head.FreeItems			<<
						dirList.head.Next						<<
						dirList.head.Prev						<<
						dirList.head.ItemsToFollow;

	// loop over 31 diritems and write them out to the stream
	for(short i=0; i < 31; i++)
	{
		stream << dirList.items[i].matrixID					<<
							dirList.items[i].dataBlock_Start	<<
							dirList.items[i].dataBlock_End		<<
							dirList.items[i].matrixStatus;
	}

	return stream;
}
