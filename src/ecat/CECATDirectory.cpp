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

#include "CECATDirectory.h"
#include "CECATDirectoryItem.h"
#include "CECATFile.h"

#include <qdatastream.h>
#include <qintdict.h>
#include <qvaluevector.h>

#include <rtdebug.h>

// the RAW structures used in the ECAT directory blocks
#pragma pack(2)
struct ECAT_DirHead
{
	Q_UINT32 FreeItems;
	Q_UINT32 Next;
	Q_UINT32 Prev;
	Q_UINT32 ItemsToFollow;
};

struct ECAT_DirItem
{
	Q_UINT32 matrixID;				// frame/plane/gate/bed/data encoded matrixID
	Q_UINT32 dataBlock_Start;	// start position of the subHeader
	Q_UINT32 dataBlock_End;		// end position of the last dataBlock
	Q_INT32 matrixStatus;			// status of the item (0=notWrittenYet,1=OK,-1=deleted)
};

struct ECAT_DirList // should be 512 bytes
{
	struct ECAT_DirHead head;
	struct ECAT_DirItem items[31];
};
#pragma pack()

CECATDirectory::CECATDirectory(CECATFile* ecatFile)
	: m_pECATFile(ecatFile)
{
	ENTER();

	// do some debugging to make sure that our structure really
	// have the correct width
	ASSERT(sizeof(struct ECAT_DirHead) == 16);	// should be 16bytes
	ASSERT(sizeof(struct ECAT_DirItem) == 16);	// should be 16bytes
	ASSERT(sizeof(struct ECAT_DirList) == 512);	// a dirlist should always be 512 bytes

  // create a new value vector for carrying all the different file
  // positions this directory list is splitted to.
	m_pFilePositions = new QValueVector<QIODevice::Offset>();
	m_pFilePositions->clear();
	m_pFilePositions->append(ECATBlock2FilePos(ECAT_POS_MAINDIR));

	setAutoDelete(true);

	LEAVE();
}

CECATDirectory::~CECATDirectory()
{
	ENTER();
	delete m_pFilePositions;
	LEAVE();
}

bool CECATDirectory::load(void)
{
	ENTER();
	bool result = true;

	// only go on if the device is readable at all
	if(m_pECATFile->isReadable() == false)
	{
		RETURN(false);
		return false;
	}

	// let us clear our filePositions ValueVector as we are going
	// to read in the whole dir list again
	m_pFilePositions->clear();
	clear();

	// before we load the directory information we set the
	// fileposition according to the normal position of the
	// main directory.
	if(m_pECATFile->at(ECATBlock2FilePos(ECAT_POS_MAINDIR)) == false)
	{
		RETURN(false);
		return false;
	}

	// we process the splitted directory list in a do..while loop
	// and jump from one dirlist to another in the ecat file.
	struct ECAT_DirList dList;
	do
	{
		// add each DirList position to our filePositions value list
		// for later reference
		SHOWVALUE(m_pECATFile->at());
		m_pFilePositions->append(m_pECATFile->at());

		// we use a ByteArray buffer to speed up the endianess decoding
		QByteArray buffer(sizeof(struct ECAT_DirList));
		if(m_pECATFile->readBlock(buffer.data(), sizeof(struct ECAT_DirList)) != 
					sizeof(struct ECAT_DirList))
		{
			E("An error occurred while reading data");

			result = false;
			break;
		}

		// now we generate a QDataStream on our buffer so that we can read
		// out of the buffer instead of the raw file (> speed)
		QDataStream stream(buffer, IO_ReadOnly);
		
		// read out the dirHead first
		stream >> dList.head.FreeItems;
		stream >> dList.head.Next;
		stream >> dList.head.Prev;
		stream >> dList.head.ItemsToFollow;
		
		// output some debug information on the head of the dirList.
		D("DirHead.FreeItems   : %d", dList.head.FreeItems);
		D("DirHead.Next        : %d", dList.head.Next);
		D("DirHead.Prev        : %d", dList.head.Prev);
		D("DirHead.ItemToFollow: %d", dList.head.ItemsToFollow);

		// now we know how many item will follow in the Directory, so we
		// can populate our Directory here.
		unsigned int iItemsInserted = 0;
		for(unsigned int i=0; i < dList.head.ItemsToFollow; i++)
		{
			CECATDirectoryItem* pNewDirItem = new CECATDirectoryItem(m_pECATFile);

			// let us read out our information directly from the stream
			stream >> *pNewDirItem;

			// before we add the item to the directory we check if the
			// values are somehow valid
			if(pNewDirItem->matrixID() > 0)
			{
				// insert the new DirectoryItem in the ItemDictonary
				// with using the MatrixID as the unique key for it and of
				// course we make sure no other item with the same ID exists or
				// we will run into trouble.
				ASSERT(find(pNewDirItem->matrixID()) == 0);
				insert(pNewDirItem->matrixID(), pNewDirItem);

				iItemsInserted++;
			}
			else
			{
				E("an error occurred while loading the dirItem");
				delete pNewDirItem;
				result = false;
				
				break;
			}
		}

		#if defined(DEBUG)
		if(dList.head.ItemsToFollow != iItemsInserted)
		{
			W("ItemsToFollow:%ld != iItemsInserted:%ld", dList.head.ItemsToFollow, iItemsInserted);
		}
		#endif

		m_pECATFile->at(ECATBlock2FilePos(dList.head.Next));
	}
	while(dList.head.Next > ECAT_POS_MAINDIR && result == true);
	
	RETURN(result);
	return result;
}

bool CECATDirectory::save(void) const
{
	ENTER();
	bool result = true;

	// only go on if the device is writeable at all
	if(m_pECATFile->isWritable() == false)
	{
		RETURN(false);
		return false;
	}
	
	// we need to populate the dirList a bit ordered
	// frame->plane->gate->bed->data
	// even if the ECAT standard doesn't define a ordering, but some
	// stupid software seems to require it.
	struct ECAT_DirHead dirHead;

	// before we start we have to link the first dirList to itself
	memset(&dirHead, 0, sizeof(struct ECAT_DirHead)); // clear it first
	dirHead.FreeItems	= 31;
	dirHead.Next			= ECAT_POS_MAINDIR;

	// we use two buffers. one for storing the dirHead of the ECAT and
	// one for the 31 dirItems.
	QByteArray dirItemBuffer(31*sizeof(struct ECAT_DirItem));
	memset(dirItemBuffer.data(), 0, 31*sizeof(struct ECAT_DirItem)); // clear it first
	QDataStream dirItemStream(dirItemBuffer, IO_WriteOnly);

	// now we have to go through our directory and stream all items
	// in 31 chunks as the directory list can only have 31 items plus
	// 1 chunk for the header
	int frame=1;
	int plane=1;
	int gate=1;
	int bed=0;
	int data=0;
	short depth=4; // we have five depth levels (4==data, 3==bed, 2==gate, 1==plane, 0==frame)
	short currentDirPos = ECAT_POS_MAINDIR;
	short nextDirPos;
	unsigned short curDirList = 0;
	do
	{
		Q_UINT32 matrixID = convertToMatrixID(frame, plane, gate, bed, data);
		CECATDirectoryItem* pDirItem = find(matrixID);

		if(pDirItem)
		{
			D("found DirItem: %d/%d/%d/%d/%d", frame, plane, gate, bed, data);

			// stream in the directory Item information
			dirItemStream << *pDirItem;

			// increment the ItemsToFollow value and decrement the 
			// freeItems
			dirHead.ItemsToFollow++;
			dirHead.FreeItems--;
			
			// first we check wheter the dirList is filled up (>=31 items)
			// and then write it out separate first.
			if(dirHead.ItemsToFollow == 31)
			{
				// get the position where the next directorylist will start
				// so we look into our filePositions ValueVector and if there is
				// another entry we use it or we create a new directoryList
				if(m_pFilePositions->count()-1 == curDirList)
				{
					// there a no more dirListEntries in our list
					// so we go and create a new one at the end of the file
					m_pFilePositions->append(m_pECATFile->size());
				}

				// calculate the block position where the next directory list
				// starts
				nextDirPos = FilePos2ECATBlock(m_pECATFile->size());
				dirHead.Next = nextDirPos;

				// now we can write out the whole directory List to the file
				// where we first write out the dirHead and then the 31 dirItems
				QByteArray dirHeadBuffer(sizeof(struct ECAT_DirHead));
				QDataStream dirHeadStream(dirHeadBuffer, IO_WriteOnly);

				dirHeadStream << dirHead.FreeItems;
				dirHeadStream << dirHead.Next;
				dirHeadStream << dirHead.Prev;
				dirHeadStream << dirHead.ItemsToFollow;
					
				// now we can seek to the file position of the DirList
				// in the file.
				if(m_pECATFile->at((*m_pFilePositions)[curDirList]) == false)
				{
					E("Error while seeking to DirList position: %d", (*m_pFilePositions)[curDirList]);
					result = false;
					break;
				}
					
				SHOWVALUE(m_pECATFile->at());

				// write out everything
				if(m_pECATFile->writeBlock(dirHeadBuffer) != sizeof(struct ECAT_DirHead) ||
					 m_pECATFile->writeBlock(dirItemBuffer) != 31*sizeof(struct ECAT_DirItem))
				{
					E("Error while writing DirList");
					result = false;
					break;
				}
				else
					D("DirList #%d successfully written", curDirList);

				// clear the dirHead so that we can immediately reuse it
				memset(&dirHead, 0, sizeof(struct ECAT_DirHead));
				dirHead.FreeItems	= 31;
				dirHead.Next			= ECAT_POS_MAINDIR;
				dirHead.Prev			= currentDirPos;
				currentDirPos			= nextDirPos;

				// clear also the DirItem buffer
				memset(dirItemBuffer.data(), 0, 31*sizeof(struct ECAT_DirItem));
				dirItemStream.device()->at(0);

				// iterate to the next dirList
				curDirList++;
			}
			
			if(depth < 4)				depth++;	
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

	// now we make sure that even not fully filled up directory lists
	// get written out at the end
	if(result == true &&
		 m_pFilePositions->count()-1 == curDirList)
	{
		// now we can write out the whole directory List to the file
		// where we first write out the dirHead and then the 31 dirItems
		QByteArray dirHeadBuffer(sizeof(struct ECAT_DirHead));
		QDataStream dirHeadStream(dirHeadBuffer, IO_WriteOnly);

		dirHeadStream << dirHead.FreeItems;
		dirHeadStream << dirHead.Next;
		dirHeadStream << dirHead.Prev;
		dirHeadStream << dirHead.ItemsToFollow;
			
		// now we can seek to the file position of the DirList
		// in the file.
		if(m_pECATFile->at((*m_pFilePositions)[curDirList]) == false)
		{
			E("Error while seeking to DirList position: %d", (*m_pFilePositions)[curDirList]);
			
			result = false;
		}
		else
		{
			SHOWVALUE(m_pECATFile->at());

			// write out everything
			if(m_pECATFile->writeBlock(dirHeadBuffer) != sizeof(struct ECAT_DirHead) ||
				 m_pECATFile->writeBlock(dirItemBuffer) != 31*sizeof(struct ECAT_DirItem))
			{
				E("Error while writing DirList");
				result = false;
			}
			else
				D("DirList #%d successfully written", curDirList);
		}
	}

	RETURN(result);
	return result;
}

CECATDirectoryItem* CECATDirectory::item(short frame, short plane,
																			 	 short gate, short bed, short data)
{
	// form the MatrixID from the supplied data and
	// process a find in the QIntDict
	Q_UINT32 mID = convertToMatrixID(frame, plane, gate, bed, data);
	D("Generated MatrixID: %08lx", mID);

	return find(mID);
}

CECATDirectoryItem* CECATDirectory::newItem(Q_UINT32 matrixID)
{
	ENTER();

	// create a new DirectoryItem which we put in this Directory
	CECATDirectoryItem* pNewDItem = new CECATDirectoryItem(m_pECATFile, matrixID);

	// now that we generated a new directory item we want to put
	// in our directory immediately, we have to first check at which
	// dataposition it should be placed
	QIODevice::Offset dataOffset = lastDirItemOffset();

	// modify the Start offset of the item now. So if this is the first
	// one we place it directly behind the maindirectory, which should
	// be block 3. If this is not the first one we place it behind the
	// lastDirItemOffset...
	if(dataOffset > 0)
		pNewDItem->setDataBlock_Start(dataOffset+ECAT_BLOCKSIZE);
	else
		pNewDItem->setDataBlock_Start(ECATBlock2FilePos(ECAT_POS_MAINDIR+1));

	// insert the new directory item into the directory now.
	insert(matrixID, pNewDItem);

	// now that we have created a new directory item we have to
	// sync our main header again
	m_pECATFile->reWriteMainHeader();

	RETURN(pNewDItem);
	return pNewDItem;
}

QIODevice::Offset CECATDirectory::lastDirItemOffset(void)
{
	ENTER();
	QIODevice::Offset offset = 0;

	QIntDictIterator<CECATDirectoryItem> it(*this);
	for(;it.current(); ++it)
	{
		if(it.current()->dataBlock_End() > offset)
			offset = it.current()->dataBlock_End();
	}

	RETURN(offset);
	return offset;
}

short CECATDirectory::numFrames(void) const
{
	short framesNum = 0;

	// we iterate through our dictionary looking for the highest available
	// frame number
	QIntDictIterator<CECATDirectoryItem> it(*this);
	for(;it.current(); ++it)
	{
		if(it.current()->frame() > framesNum)
			framesNum = it.current()->frame();
	}

	return framesNum;
}

short CECATDirectory::numPlanes(void) const
{
	short planesNum = 0;

	// we iterate through our dictionary looking for the highest available
	// plane number
	QIntDictIterator<CECATDirectoryItem> it(*this);
	for(;it.current(); ++it)
	{
		if(it.current()->plane() > planesNum)
			planesNum = it.current()->plane();
	}

	return planesNum;
}

short CECATDirectory::numGates(void) const
{
	short gatesNum = 0;

	// we iterate through our dictionary looking for the highest available
	// gates number
	QIntDictIterator<CECATDirectoryItem> it(*this);
	for(;it.current(); ++it)
	{
		if(it.current()->gate() > gatesNum)
			gatesNum = it.current()->gate();
	}

	return gatesNum;
}	

short CECATDirectory::numBedPos(void) const
{
	short bedsNum = 0;

	// we iterate through our dictionary looking for the highest available
	// plane number
	QIntDictIterator<CECATDirectoryItem> it(*this);
	for(;it.current(); ++it)
	{
		if(it.current()->bed() > bedsNum)
			bedsNum = it.current()->bed();
	}

	return bedsNum;
}	


bool CECATDirectory::readSubHeader(CECATSubHeader*& subHeader, short frame, short plane,
																	 short gate, short bed, short data)
{
	// get the directoryItem so that we can query the matrix from it
	CECATDirectoryItem* pDirItem = find(convertToMatrixID(frame, plane, gate, bed, data));

	if(pDirItem)
		return pDirItem->readSubHeader(subHeader);

	return false;
}

bool CECATDirectory::readMatrix(QByteArray*& matrixData, short frame, short plane,
															 short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	// get the directoryItem so that we can query the matrix from it
	CECATDirectoryItem* pDirItem = find(convertToMatrixID(frame, plane, gate, bed, data));

	if(pDirItem)
		result = pDirItem->readMatrix(matrixData);

	RETURN(result);
	return result;
}

bool CECATDirectory::readMatrix(char*& matrixData, unsigned int& len, short frame, short plane,
															  short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	// get the directoryItem so that we can query the matrix from it
	CECATDirectoryItem* pDirItem = find(convertToMatrixID(frame, plane, gate, bed, data));

	if(pDirItem)
		result = pDirItem->readMatrix(matrixData, len);

	RETURN(result);
	return result;
}

bool CECATDirectory::writeSubHeader(const CECATSubHeader& subHeader, short frame,
																		short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	// form the MatrixID from the supplied data and create a new
	// DirectoryItem (or replace an existing one)
	Q_UINT32 mID = convertToMatrixID(frame, plane, gate, bed, data);
	D("Generated MatrixID: %08lx", mID);

	// see if we already have an item with the same matrixID in our
	// dictonary and if so we reuse that one and place a new subheader at
	// this position
	CECATDirectoryItem* pNewDItem = find(mID);
	if(pNewDItem == NULL)
	{
		// create a new DirectoryItem which we put in this Directory
		pNewDItem = newItem(mID);
	}

	// then we make sure the subheader is written to the ECAT file.
	result = pNewDItem->writeSubHeader(subHeader);
	if(result)
		save();
	
	RETURN(result);
	return result;
}

bool CECATDirectory::writeMatrix(const QByteArray& matrixData,
																 short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	// form the MatrixID from the supplied data and create a new
	// DirectoryItem (or replace an existing one)
	Q_UINT32 mID = convertToMatrixID(frame, plane, gate, bed, data);
	D("Generated MatrixID: %08lx", mID);

	// see if we already have an item with the same matrixID in our
	// dictonary and if so we reuse that one and place a new subheader at
	// this position
	CECATDirectoryItem* pNewDItem = find(mID);
	if(pNewDItem == NULL)
	{
		// create a new DirectoryItem which we put in this Directory
		pNewDItem = newItem(mID);
	}

	// then we make sure the subheader is written to the ECAT file.
	result = pNewDItem->writeMatrix(matrixData);
	if(result)
		save();
	
	RETURN(result);
	return result;
}

bool CECATDirectory::writeMatrix(const char* matrixData, unsigned int size,
																 short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	// form the MatrixID from the supplied data and create a new
	// DirectoryItem (or replace an existing one)
	Q_UINT32 mID = convertToMatrixID(frame, plane, gate, bed, data);
	D("Generated MatrixID: %08lx", mID);

	// see if we already have an item with the same matrixID in our
	// dictonary and if so we reuse that one and place a new subheader at
	// this position
	CECATDirectoryItem* pNewDItem = find(mID);
	if(pNewDItem == NULL)
	{
		// create a new DirectoryItem which we put in this Directory
		pNewDItem = newItem(mID);
	}

	// then we make sure the subheader is written to the ECAT file.
	result = pNewDItem->writeMatrix(matrixData, size);
	if(result)
		save();

	RETURN(result);
	return result;
}

bool CECATDirectory::writeMatrix(const QByteArray& matrixData, CECATSubHeader::Data_Type type,
																 short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	// form the MatrixID from the supplied data and create a new
	// DirectoryItem (or replace an existing one)
	Q_UINT32 mID = convertToMatrixID(frame, plane, gate, bed, data);
	D("Generated MatrixID: %08lx", mID);

	// see if we already have an item with the same matrixID in our
	// dictonary and if so we reuse that one and place a new subheader at
	// this position
	CECATDirectoryItem* pNewDItem = find(mID);
	if(pNewDItem == NULL)
	{
		// create a new DirectoryItem which we put in this Directory
		pNewDItem = newItem(mID);
	}

	// then we make sure the subheader is written to the ECAT file.
	result = pNewDItem->writeMatrix(matrixData, type);
	if(result)
		save();
	
	RETURN(result);
	return result;
}

bool CECATDirectory::writeMatrix(const char* matrixData, unsigned int size,
																 CECATSubHeader::Data_Type type,
																 short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	// form the MatrixID from the supplied data and create a new
	// DirectoryItem (or replace an existing one)
	Q_UINT32 mID = convertToMatrixID(frame, plane, gate, bed, data);
	D("Generated MatrixID: %08lx", mID);

	// see if we already have an item with the same matrixID in our
	// dictonary and if so we reuse that one and place a new subheader at
	// this position
	CECATDirectoryItem* pNewDItem = find(mID);
	if(pNewDItem == NULL)
	{
		// create a new DirectoryItem which we put in this Directory
		pNewDItem = newItem(mID);
	}

	// then we make sure the subheader is written to the ECAT file.
	result = pNewDItem->writeMatrix(matrixData, size, type);
	if(result)
		save();

	RETURN(result);
	return result;
}
