/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004-2005 by Jens Langner <Jens.Langner@light-speed.de>

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

#include <QDataStream>
#include <QHash>
#include <QVector>

#include <rtdebug.h>

#define ECAT_DIRHEAD_SIZE 16
#define ECAT_DIRITEM_SIZE 16
#define ECAT_DIRITEM_NUM  31
#define ECAT_DIRLIST_SIZE ECAT_DIRHEAD_SIZE+ECAT_DIRITEM_NUM*ECAT_DIRITEM_SIZE

// the RAW structures used in the ECAT directory blocks
struct ECAT_DirHead // should be 16 bytes
{
	quint32 FreeItems;
	quint32 Next;
	quint32 Prev;
	quint32 ItemsToFollow;
};

struct ECAT_DirItem // should be 16 bytes
{
	quint32 matrixID;					// frame/plane/gate/bed/data encoded matrixID
	quint32 dataBlock_Start;	// start position of the subHeader
	quint32 dataBlock_End;		// end position of the last dataBlock
	qint32 matrixStatus;			// status of the item (0=notWrittenYet,1=OK,-1=deleted)
};

struct ECAT_DirList // should be 512 bytes
{
	struct ECAT_DirHead head;
	struct ECAT_DirItem items[ECAT_DIRITEM_NUM];
};

CECATDirectory::CECATDirectory(CECATFile* ecatFile)
	: m_pECATFile(ecatFile)
{
	ENTER();

  // create a new value vector for carrying all the different file
  // positions this directory list is splitted to.
	m_pFilePositions = new QVector<qint64>();
	m_pFilePositions->clear();
	m_pFilePositions->append(ECATBlock2FilePos(ECAT_POS_MAINDIR));

	LEAVE();
}

CECATDirectory::~CECATDirectory()
{
	ENTER();

	// delete the file position QVector
	delete m_pFilePositions;

	// now we iterate through our QHash and make sure we delete all our
	// stored diritems correctly.
  QHashIterator<int, CECATDirectoryItem*> i(*this);
	while(i.hasNext())
	{
		i.next();
		delete i.value();
	}

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
	if(m_pECATFile->seek(ECATBlock2FilePos(ECAT_POS_MAINDIR)) == false)
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
		SHOWVALUE(m_pECATFile->pos());
		m_pFilePositions->append(m_pECATFile->pos());

		// we use a ByteArray buffer to speed up the endianess decoding
		QByteArray buffer(ECAT_DIRLIST_SIZE, 0);
		if(m_pECATFile->read(buffer.data(), buffer.size()) != ECAT_DIRLIST_SIZE) 
		{
			E("An error occurred while reading data");

			result = false;
			break;
		}

		// now we generate a QDataStream on our buffer so that we can read
		// out of the buffer instead of the raw file (> speed)
		QDataStream stream(buffer);
		
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
				ASSERT(contains(pNewDirItem->matrixID()) == false);
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

		m_pECATFile->seek(ECATBlock2FilePos(dList.head.Next));
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
	dirHead.FreeItems	= ECAT_DIRITEM_NUM;
	dirHead.Next			= ECAT_POS_MAINDIR;

	// we use two buffers. one for storing the dirHead of the ECAT and
	// one for the 31 dirItems.
	QByteArray dirItemBuffer(ECAT_DIRITEM_NUM*ECAT_DIRITEM_SIZE, 0);
	QDataStream dirItemStream(&dirItemBuffer, QIODevice::WriteOnly);

	// now we have to go through our directory and stream all items
	// in 31 chunks as the directory list can only have 31 items plus
	// 1 chunk for the header
	int frame=1;
	int plane=1;
	int gate=1;
	int bed=0;
	int data=0;
	short depth=4; // we have five depth levels (4==data, 3==bed, 2==gate, 1==plane, 0==frame)
	unsigned int currentDirPos = ECAT_POS_MAINDIR;
	unsigned int nextDirPos;
	int foundDirItems = 0;
	unsigned short curDirList = 0;
	do
	{
		quint32 matrixID = convertToMatrixID(frame, plane, gate, bed, data);
		CECATDirectoryItem* pDirItem = value(matrixID);

		if(pDirItem)
		{
			D("found DirItem: %d/%d/%d/%d/%d", frame, plane, gate, bed, data);
			foundDirItems++;

			// stream in the directory Item information
			dirItemStream << *pDirItem;

			// increment the ItemsToFollow value and decrement the 
			// freeItems
			dirHead.ItemsToFollow++;
			dirHead.FreeItems--;
			
			// first we check wheter the dirList is filled up (>=31 items)
			// and then write it out separate first.
			if(dirHead.ItemsToFollow == ECAT_DIRITEM_NUM && 
				 foundDirItems < count())
			{
				qint64 appendPos;

				// get the position where the next directorylist will start
				// so we look into our filePositions ValueVector and if there is
				// another entry we use it or we create a new directoryList
				if(m_pFilePositions->count()-1 == curDirList)
				{			
					appendPos = lastDirItemOffset()+ECAT_BLOCKSIZE;

					// append the position to our FilePositions for DirLists
					m_pFilePositions->append(appendPos);
								
					E("appended new DirList #%d @ %d (%d)", curDirList+1, appendPos, FilePos2ECATBlock(appendPos));
				}
				else
					appendPos = (*m_pFilePositions)[curDirList+1];

				// calculate the block position where the next directory list
				// starts (the current endsize of the ECAT file substracted by one
				// ECAT blocksize [512 bytes])
				nextDirPos = FilePos2ECATBlock(appendPos);
				dirHead.Next = nextDirPos;

				// now we can write out the whole directory List to the file
				// where we first write out the dirHead and then the 31 dirItems
				QByteArray dirHeadBuffer(ECAT_DIRHEAD_SIZE, 0);
				QDataStream dirHeadStream(&dirHeadBuffer, QIODevice::WriteOnly);

				dirHeadStream << dirHead.FreeItems;
				dirHeadStream << dirHead.Next;
				dirHeadStream << dirHead.Prev;
				dirHeadStream << dirHead.ItemsToFollow;
					
				// now we can seek to the file position of the DirList
				// in the file.
				if(m_pECATFile->seek((*m_pFilePositions)[curDirList]) == false)
				{
					E("Error while seeking to DirList position: %d", (*m_pFilePositions)[curDirList]);
					result = false;
					break;
				}
					
				D("DirHead%d.Position     : %d (%d)", curDirList, FilePos2ECATBlock(m_pECATFile->pos()), (*m_pFilePositions)[curDirList]);
				D("DirHead%d.FreeItems    : %d", curDirList, dirHead.FreeItems);
				D("DirHead%d.Next         : %d", curDirList, dirHead.Next);
				D("DirHead%d.Prev         : %d", curDirList, dirHead.Prev);
				D("DirHead%d.ItemsToFollow: %d", curDirList, dirHead.ItemsToFollow);

				// write out everything
				if(m_pECATFile->write(dirHeadBuffer) != ECAT_DIRHEAD_SIZE ||
					 m_pECATFile->write(dirItemBuffer) != ECAT_DIRITEM_NUM*ECAT_DIRITEM_SIZE)
				{
					E("Error while writing DirList");
					result = false;
					break;
				}
				else
					D("DirList #%d successfully written @ %d", curDirList, m_pECATFile->pos()-ECAT_DIRLIST_SIZE);

				// clear the dirHead so that we can immediately reuse it
				dirHead.FreeItems			= ECAT_DIRITEM_NUM;
				dirHead.Next					= ECAT_POS_MAINDIR;
				dirHead.Prev					= currentDirPos;
				dirHead.ItemsToFollow = 0;
				currentDirPos					= nextDirPos;

				// clear also the DirItem buffer
				memset(dirItemBuffer.data(), 0, dirItemBuffer.size());
				dirItemStream.device()->seek(0);

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
			if(depth == 0 || foundDirItems == count())
				break;
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
		QByteArray dirHeadBuffer(ECAT_DIRHEAD_SIZE, 0);
		QDataStream dirHeadStream(&dirHeadBuffer, QIODevice::WriteOnly);

		dirHeadStream << dirHead.FreeItems;
		dirHeadStream << dirHead.Next;
		dirHeadStream << dirHead.Prev;
		dirHeadStream << dirHead.ItemsToFollow;
			
		// now we can seek to the file position of the DirList
		// in the file.
		if(m_pECATFile->seek((*m_pFilePositions)[curDirList]) == false)
		{
			E("Error while seeking to DirList position: %d", (*m_pFilePositions)[curDirList]);
			
			result = false;
		}
		else
		{
			D("DirHead%d.Position     : %d (%d)", curDirList, FilePos2ECATBlock(m_pECATFile->pos()), (*m_pFilePositions)[curDirList]);
			D("DirHead%d.FreeItems    : %d", curDirList, dirHead.FreeItems);
			D("DirHead%d.Next         : %d", curDirList, dirHead.Next);
			D("DirHead%d.Prev         : %d", curDirList, dirHead.Prev);
			D("DirHead%d.ItemsToFollow: %d", curDirList, dirHead.ItemsToFollow);
			
			// write out everything
			if(m_pECATFile->write(dirHeadBuffer) != ECAT_DIRHEAD_SIZE ||
				 m_pECATFile->write(dirItemBuffer) != ECAT_DIRITEM_NUM*ECAT_DIRITEM_SIZE)
			{
				E("Error while writing DirList #%d at %ld", curDirList, m_pECATFile->pos());
				result = false;
			}
			else
				D("DirList #%d successfully written @ %d", curDirList, m_pECATFile->pos()-ECAT_DIRLIST_SIZE);
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
	quint32 mID = convertToMatrixID(frame, plane, gate, bed, data);
	D("Generated MatrixID: %08lx", mID);

	return value(mID);
}

CECATDirectoryItem* CECATDirectory::newItem(quint32 matrixID)
{
	ENTER();

	// create a new DirectoryItem which we put in this Directory
	CECATDirectoryItem* pNewDItem = new CECATDirectoryItem(m_pECATFile, matrixID);

	// now that we generated a new directory item we want to put
	// in our directory immediately, we have to first check at which
	// dataposition it should be placed
	qlonglong dataOffset = lastDirItemOffset();

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

qint64 CECATDirectory::lastDirItemOffset(void) const
{
	ENTER();
	qint64 offset = 0;

	// we search through our item list and check the for the
	// very last item position
  QHashIterator<int, CECATDirectoryItem*> i(*this);
	while(i.hasNext())
	{
		i.next();
		if(i.value()->dataBlock_End() > offset)
			offset = i.value()->dataBlock_End();
	}

	// in addition to the diritem position check we have to check
	// the dirlist positions as well.
	for(unsigned int i=0; i < static_cast<unsigned int>(m_pFilePositions->count()); i++)
	{
		if((*m_pFilePositions)[i] > offset)
			offset = (*m_pFilePositions)[i];
	}

	RETURN(offset);
	return offset;
}

short CECATDirectory::numFrames(void) const
{
	short framesNum = 0;

	// we iterate through our dictionary looking for the highest available
	// frame number
  QHashIterator<int, CECATDirectoryItem*> i(*this);
	while(i.hasNext())
	{
		i.next();
		if(i.value()->frame() > framesNum)
			framesNum = i.value()->frame();
	}
	
	return framesNum;
}

short CECATDirectory::numPlanes(void) const
{
	short planesNum = 0;

	// we iterate through our dictionary looking for the highest available
	// plane number
  QHashIterator<int, CECATDirectoryItem*> i(*this);
	while(i.hasNext())
	{
		i.next();
		if(i.value()->plane() > planesNum)
			planesNum = i.value()->plane();
	}
	
	return planesNum;
}

short CECATDirectory::numGates(void) const
{
	short gatesNum = 0;

	// we iterate through our dictionary looking for the highest available
	// gates number
  QHashIterator<int, CECATDirectoryItem*> i(*this);
	while(i.hasNext())
	{
		i.next();
		if(i.value()->gate() > gatesNum)
			gatesNum = i.value()->gate();
	}

	return gatesNum;
}	

short CECATDirectory::numBedPos(void) const
{
	short bedsNum = 0;

	// we iterate through our dictionary looking for the highest available
	// plane number
  QHashIterator<int, CECATDirectoryItem*> i(*this);
	while(i.hasNext())
	{
		i.next();
		if(i.value()->bed() > bedsNum)
			bedsNum = i.value()->bed();
	}

	return bedsNum;
}	


bool CECATDirectory::readSubHeader(CECATSubHeader*& subHeader, short frame, short plane,
																	 short gate, short bed, short data)
{
	// get the directoryItem so that we can query the matrix from it
	CECATDirectoryItem* pDirItem = value(convertToMatrixID(frame, plane, gate, bed, data));

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
	CECATDirectoryItem* pDirItem = value(convertToMatrixID(frame, plane, gate, bed, data));

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
	CECATDirectoryItem* pDirItem = value(convertToMatrixID(frame, plane, gate, bed, data));

	if(pDirItem)
		result = pDirItem->readMatrix(matrixData, len);

	RETURN(result);
	return result;
}

bool CECATDirectory::readMatrix(QByteArray*& matrixData, CECATSubHeader*& subHeader,
																short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	// get the directoryItem so that we can query the matrix from it
	CECATDirectoryItem* pDirItem = value(convertToMatrixID(frame, plane, gate, bed, data));

	if(pDirItem)
		result = pDirItem->readMatrix(matrixData, subHeader);

	RETURN(result);
	return result;
}

bool CECATDirectory::readMatrix(char*& matrixData, unsigned int& len, CECATSubHeader*& subHeader,
																short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	// get the directoryItem so that we can query the matrix from it
	CECATDirectoryItem* pDirItem = value(convertToMatrixID(frame, plane, gate, bed, data));

	if(pDirItem)
		result = pDirItem->readMatrix(matrixData, len, subHeader);

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
	quint32 mID = convertToMatrixID(frame, plane, gate, bed, data);
	D("Generated MatrixID: %08lx", mID);

	// see if we already have an item with the same matrixID in our
	// dictonary and if so we reuse that one and place a new subheader at
	// this position
	CECATDirectoryItem* pNewDItem = value(mID);
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
	quint32 mID = convertToMatrixID(frame, plane, gate, bed, data);
	D("Generated MatrixID: %08lx", mID);

	// see if we already have an item with the same matrixID in our
	// dictonary and if so we reuse that one and place a new subheader at
	// this position
	CECATDirectoryItem* pNewDItem = value(mID);
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
	quint32 mID = convertToMatrixID(frame, plane, gate, bed, data);
	D("Generated MatrixID: %08lx", mID);

	// see if we already have an item with the same matrixID in our
	// dictonary and if so we reuse that one and place a new subheader at
	// this position
	CECATDirectoryItem* pNewDItem = value(mID);
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
	quint32 mID = convertToMatrixID(frame, plane, gate, bed, data);
	D("Generated MatrixID: %08lx", mID);

	// see if we already have an item with the same matrixID in our
	// dictonary and if so we reuse that one and place a new subheader at
	// this position
	CECATDirectoryItem* pNewDItem = value(mID);
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
	quint32 mID = convertToMatrixID(frame, plane, gate, bed, data);
	D("Generated MatrixID: %08lx", mID);

	// see if we already have an item with the same matrixID in our
	// dictonary and if so we reuse that one and place a new subheader at
	// this position
	CECATDirectoryItem* pNewDItem = value(mID);
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

bool CECATDirectory::writeMatrix(const QByteArray& matrixData, const CECATSubHeader& subHeader,
																 short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	// form the MatrixID from the supplied data and create a new
	// DirectoryItem (or replace an existing one)
	quint32 mID = convertToMatrixID(frame, plane, gate, bed, data);
	D("Generated MatrixID: %08lx", mID);

	// see if we already have an item with the same matrixID in our
	// dictonary and if so we reuse that one and place a new subheader at
	// this position
	CECATDirectoryItem* pNewDItem = value(mID);
	if(pNewDItem == NULL)
	{
		// create a new DirectoryItem which we put in this Directory
		pNewDItem = newItem(mID);
	}

	// then we make sure the subheader is written to the ECAT file.
	result = pNewDItem->writeMatrix(matrixData, subHeader);
	if(result)
		save();
	
	RETURN(result);
	return result;
}

bool CECATDirectory::writeMatrix(const char* matrixData, unsigned int size, const CECATSubHeader& subHeader,
																 short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	// form the MatrixID from the supplied data and create a new
	// DirectoryItem (or replace an existing one)
	quint32 mID = convertToMatrixID(frame, plane, gate, bed, data);
	D("Generated MatrixID: %08lx", mID);

	// see if we already have an item with the same matrixID in our
	// dictonary and if so we reuse that one and place a new subheader at
	// this position
	CECATDirectoryItem* pNewDItem = value(mID);
	if(pNewDItem == NULL)
	{
		// create a new DirectoryItem which we put in this Directory
		pNewDItem = newItem(mID);
	}

	// then we make sure the subheader is written to the ECAT file.
	result = pNewDItem->writeMatrix(matrixData, size, subHeader);
	if(result)
		save();

	RETURN(result);
	return result;
}

CECATDirectoryItem* CECATDirectory::operator[](long num) const
{
	ENTER();
	CECATDirectoryItem* foundItem = NULL;

	// use an QHashIterator to iterate until we got the num'th
	// element in our dictonary and return it
  QHashIterator<int, CECATDirectoryItem*> it(*this);
	for(long i=0; i < num && it.hasNext(); i++)
	{
		it.next();
		foundItem = it.value();
	}
	
	RETURN(foundItem);
	return foundItem;
}
