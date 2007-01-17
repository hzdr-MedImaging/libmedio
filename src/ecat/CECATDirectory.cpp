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

#include "CECATDirectory.h"
#include "CECATDirectoryItem.h"
#include "CECATFile.h"

#include <qdatastream.h>
#include <qmap.h>
#include <qvaluevector.h>

#include <rtdebug.h>

// some very own defines specifying the boundaries
// of a ecat directory list
#define ECAT_DIRHEAD_SIZE 16
#define ECAT_DIRITEM_SIZE 16
#define ECAT_DIRITEM_NUM  31
#define ECAT_DIRLIST_SIZE (ECAT_DIRHEAD_SIZE+ECAT_DIRITEM_NUM*ECAT_DIRITEM_SIZE)

// the RAW structures used in the ECAT directory blocks
struct ECAT_DirHead // should be 16 bytes
{
	Q_UINT32 FreeItems;
	Q_UINT32 Next;
	Q_UINT32 Prev;
	Q_UINT32 ItemsToFollow;
};

struct ECAT_DirItem // should be 16 bytes
{
	Q_UINT32 matrixID;				// frame/plane/gate/bed/data encoded matrixID
	Q_UINT32 dataBlock_Start;	// start position of the subHeader
	Q_UINT32 dataBlock_End;		// end position of the last dataBlock
	Q_INT32 matrixStatus;			// status of the item (0=notWrittenYet,1=OK,-1=deleted)
};

struct ECAT_DirList // should be 512 bytes
{
	struct ECAT_DirHead head;
	struct ECAT_DirItem items[ECAT_DIRITEM_NUM];
};

// macro for quickly checking if the frame/plane/gate/bed/data
// matrix parameters are between bounds or not
#define matrixParamsValid(f, p, g, b, d) ((f) > 0 && (f) < 256 && \
												 								  (p) > 0 && (f) < 256 && \
																					(g) > 0 && (f) < 256 && \
																					(b) >= 0 && (b) < 256 && \
																					(d) >= 0 && (d) < 256)

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CECATDirectoryPrivate
{
	public:
		CECATFile* file;	// ptr to our associated ECATFile
		QMap<Q_UINT32, CECATDirectoryItem*> dirItems;		// value map of dirItems
		QValueVector<QIODevice::Offset>  filePositions; // for each DirList we
																										// could have different file
																										// positions which we have
																										// to store

		// some private methods 
		CECATDirectoryItem* newDirItem(Q_UINT32 matrixID);
		QIODevice::Offset lastDirItemOffset(void) const;
};

CECATDirectory::CECATDirectory(CECATFile* ecatFile)
{
	ENTER();

	// allocate data from our private instance class
	m_pData = new CECATDirectoryPrivate();	

	// create the int dictonary for the dirItems and set it to autodelete
	m_pData->dirItems.clear();

  // create a new value vector for carrying all the different file
  // positions this directory list is splitted to.
	m_pData->filePositions.clear();
	m_pData->filePositions.append(ECATBlock2FilePos(ECAT_POS_MAINDIR));

	// save the ecatFile pointer
	m_pData->file = ecatFile;

	LEAVE();
}

CECATDirectory::~CECATDirectory()
{
	ENTER();

	// now we iterate through our QHash and make sure we delete all our
	// stored diritems correctly.
  QMap<Q_UINT32, CECATDirectoryItem*>::Iterator it;
	for(it=m_pData->dirItems.begin(); it != m_pData->dirItems.end(); ++it)
		delete it.data();

	delete m_pData;

	LEAVE();
}

CECATDirectory::CECATDirectory(const CECATDirectory& src)
{
	ENTER();

	// allocate data from our private instance class	
	m_pData = new CECATDirectoryPrivate();

	// copy the instance data
	*this = src;

	// set the file ptr to NULL	
	m_pData->file = NULL;

	LEAVE();
}
	
CECATDirectory& CECATDirectory::operator=(const CECATDirectory& src)
{
	ENTER();

	if(m_pData != src.m_pData)
	{
		m_pData->dirItems = src.m_pData->dirItems;
		m_pData->filePositions = src.m_pData->filePositions;
	}

	LEAVE();
	return *this;
}

bool CECATDirectory::load(void)
{
	ENTER();
	bool result = true;

	// only go on if the device is readable at all
	if(m_pData->file == NULL ||
		 m_pData->file->isReadable() == false)
	{
		RETURN(false);
		return false;
	}

	// let us clear our filePositions ValueVector as we are going
	// to read in the whole dir list again
	m_pData->filePositions.clear();
	m_pData->dirItems.clear();

	// before we load the directory information we set the
	// fileposition according to the normal position of the
	// main directory.
	if(m_pData->file->at(ECATBlock2FilePos(ECAT_POS_MAINDIR)) == false)
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
		SHOWVALUE(m_pData->file->at());
		m_pData->filePositions.append(m_pData->file->at());

		// we use a ByteArray buffer to speed up the endianess decoding
		QByteArray buffer(ECAT_DIRLIST_SIZE);
		if(m_pData->file->readBlock(buffer.data(), buffer.size()) != ECAT_DIRLIST_SIZE)
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
			CECATDirectoryItem* pNewDirItem = new CECATDirectoryItem(m_pData->file);

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
				ASSERT(m_pData->dirItems.find(pNewDirItem->matrixID()) == 0);
				m_pData->dirItems.insert(pNewDirItem->matrixID(), pNewDirItem);

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

		if(m_pData->file->at(ECATBlock2FilePos(dList.head.Next)) == false)
		{
			E("couldn't seek to file position %lld.", ECATBlock2FilePos(dList.head.Next));
			result = false;
			break;
		}
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
	if(m_pData->file->isWritable() == false)
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
	QByteArray dirItemBuffer(ECAT_DIRITEM_NUM*ECAT_DIRITEM_SIZE);
	memset(dirItemBuffer.data(), 0, dirItemBuffer.size()); // clear it first
	QDataStream dirItemStream(dirItemBuffer, IO_WriteOnly);

	// now we have to go through our directory and stream all items
	// in 31 chunks as the directory list can only have 31 items plus
	// 1 chunk for the header
	unsigned int currentDirPos = ECAT_POS_MAINDIR;
	unsigned int processedDirItems = 0;
	unsigned short curDirList = 0;
	Q_UINT32 nextDirPos;
	QIODevice::Offset lastDirListPos = 0;

	// now iterate through our sorted QMap and write out the
	// data sorted as well.
  QMap<Q_UINT32, CECATDirectoryItem*>::Iterator it;
	for(it=m_pData->dirItems.begin(); it != m_pData->dirItems.end(); ++it)
	{
		CECATDirectoryItem* pDirItem = it.data();
		if(pDirItem)
		{
			processedDirItems++;

			D("found DirItem: %d/%d/%d/%d/%d", pDirItem->frame(), 
																				 pDirItem->plane(), 
																				 pDirItem->gate(), 
																				 pDirItem->bed(), 
																				 pDirItem->data());

			// we fix a problem where it happened that in case the writeSubHeader()/
			// writeMatrix() where used separately, the directory got overwritten
			// but the very next directory item.
			if(lastDirListPos > 0 && lastDirListPos == pDirItem->dataBlock_Start())
				pDirItem->setDataBlock_Start(lastDirListPos+ECAT_DIRLIST_SIZE);
			else
				lastDirListPos = 0;
			
			// stream in the directory Item information
			dirItemStream << *pDirItem;

			// increment the ItemsToFollow value and decrement the 
			// freeItems
			dirHead.ItemsToFollow++;
			dirHead.FreeItems--;
			
			// first we check wheter the dirList is filled up (>=31 items)
			// and then write it out separate first.
			if(dirHead.ItemsToFollow == ECAT_DIRITEM_NUM && 
				 processedDirItems < count())
			{
				QIODevice::Offset appendPos;

				// get the position where the next directorylist will start
				// so we look into our filePositions ValueVector and if there is
				// another entry we use it or we create a new directoryList
				if(m_pData->filePositions.count()-1 == curDirList)
				{			
					appendPos = m_pData->lastDirItemOffset()+ECAT_BLOCKSIZE;

					// append the position to our FilePositions for DirLists
					m_pData->filePositions.append(appendPos);
					
					D("appended new DirList #%d @ %lld (%lld)", curDirList+1, FilePos2ECATBlock(appendPos), appendPos);

					lastDirListPos = appendPos;
				}
				else
					appendPos = m_pData->filePositions[curDirList+1];

				// calculate the block position where the next directory list
				// starts (the current endsize of the ECAT file substracted by one
				// ECAT blocksize [512 bytes])
				nextDirPos = FilePos2ECATBlock(appendPos);
				dirHead.Next = nextDirPos;

				// now we can write out the whole directory List to the file
				// where we first write out the dirHead and then the 31 dirItems
				QByteArray dirHeadBuffer(ECAT_DIRHEAD_SIZE);
				QDataStream dirHeadStream(dirHeadBuffer, IO_WriteOnly);

				dirHeadStream << dirHead.FreeItems;
				dirHeadStream << dirHead.Next;
				dirHeadStream << dirHead.Prev;
				dirHeadStream << dirHead.ItemsToFollow;
					
				// now we can seek to the file position of the DirList
				// in the file.
				if(m_pData->file->at(m_pData->filePositions[curDirList]) == false)
				{
					E("Error while seeking to DirList position: %lld", m_pData->filePositions[curDirList]);
					result = false;
					break;
				}
					
				D("DirHead%d.Position     : %d (%d)", curDirList, FilePos2ECATBlock(m_pData->file->at()), m_pData->filePositions[curDirList]);
				D("DirHead%d.FreeItems    : %d", curDirList, dirHead.FreeItems);
				D("DirHead%d.Next         : %d", curDirList, dirHead.Next);
				D("DirHead%d.Prev         : %d", curDirList, dirHead.Prev);
				D("DirHead%d.ItemsToFollow: %d", curDirList, dirHead.ItemsToFollow);

				// write out everything
				if(m_pData->file->writeBlock(dirHeadBuffer) != ECAT_DIRHEAD_SIZE ||
					 m_pData->file->writeBlock(dirItemBuffer) != ECAT_DIRITEM_NUM*ECAT_DIRITEM_SIZE)
				{
					E("Error while writing DirList");
					result = false;
					break;
				}
				else
					D("DirList #%d successfully written @ %lld (%lld)", curDirList, 
																															FilePos2ECATBlock(m_pData->file->at()-ECAT_DIRLIST_SIZE),
																															m_pData->file->at()-ECAT_DIRLIST_SIZE);

				// clear the dirHead so that we can immediately reuse it
				dirHead.FreeItems			= ECAT_DIRITEM_NUM;
				dirHead.Next					= ECAT_POS_MAINDIR;
				dirHead.Prev					= currentDirPos;
				dirHead.ItemsToFollow = 0;
				currentDirPos					= nextDirPos;

				// clear also the DirItem buffer
				memset(dirItemBuffer.data(), 0, dirItemBuffer.size());
				if(dirItemStream.device()->at(0) == false)
				{
					E("couldn't seek to dirItemStream position #0");
					result = false;
					break;
				}

				// iterate to the next dirList
				curDirList++;
			}
		}
	}

	// now we make sure that even not fully filled up directory lists
	// get written out at the end
	if(result == true &&
		 m_pData->filePositions.count()-1 == curDirList)
	{
		// now we can write out the whole directory List to the file
		// where we first write out the dirHead and then the 31 dirItems
		QByteArray dirHeadBuffer(ECAT_DIRHEAD_SIZE);
		QDataStream dirHeadStream(dirHeadBuffer, IO_WriteOnly);

		dirHeadStream << dirHead.FreeItems;
		dirHeadStream << dirHead.Next;
		dirHeadStream << dirHead.Prev;
		dirHeadStream << dirHead.ItemsToFollow;
			
		// now we can seek to the file position of the DirList
		// in the file.
		if(m_pData->file->at(m_pData->filePositions[curDirList]) == false)
		{
			E("Error while seeking to DirList position: %d", m_pData->filePositions[curDirList]);
			
			result = false;
		}
		else
		{
			D("DirHead%d.Position     : %lld (%lld)", curDirList, FilePos2ECATBlock(m_pData->file->at()), m_pData->filePositions[curDirList]);
			D("DirHead%d.FreeItems    : %d", curDirList, dirHead.FreeItems);
			D("DirHead%d.Next         : %d", curDirList, dirHead.Next);
			D("DirHead%d.Prev         : %d", curDirList, dirHead.Prev);
			D("DirHead%d.ItemsToFollow: %d", curDirList, dirHead.ItemsToFollow);

			// write out everything
			if(m_pData->file->writeBlock(dirHeadBuffer) != ECAT_DIRHEAD_SIZE ||
				 m_pData->file->writeBlock(dirItemBuffer) != ECAT_DIRITEM_NUM*ECAT_DIRITEM_SIZE)
			{
				E("Error while writing DirList #%d at %lld", curDirList, m_pData->file->at());
				result = false;
			}
			else
				D("DirList #%d successfully written @ %lld (%lld)", curDirList, 
																														FilePos2ECATBlock(m_pData->file->at()-ECAT_DIRLIST_SIZE),
																														m_pData->file->at()-ECAT_DIRLIST_SIZE);
		}
	}

	RETURN(result);
	return result;
}

bool CECATDirectory::isEmpty() const
{ 
	return m_pData->dirItems.isEmpty();
}

unsigned int CECATDirectory::count() const
{ 
	return m_pData->dirItems.count();
}


CECATDirectoryItem* CECATDirectory::item(short frame, short plane,
																			 	 short gate, short bed, short data)
{
	ENTER();
	CECATDirectoryItem* dirItem = NULL;

	if(matrixParamsValid(frame, plane, gate, bed, data))
	{
		// form the MatrixID from the supplied data and
		// process a find in the QIntDict
		Q_UINT32 mID = convertToMatrixID(frame, plane, gate, bed, data);
		D("Generated MatrixID: %08lx", mID);

		QMap<Q_UINT32, CECATDirectoryItem*>::Iterator it = m_pData->dirItems.find(mID);
		if(it != m_pData->dirItems.end())
			dirItem = it.data();
	}

	RETURN(dirItem);
	return dirItem;
}

CECATDirectoryItem* CECATDirectoryPrivate::newDirItem(Q_UINT32 matrixID)
{
	ENTER();

	// create a new DirectoryItem which we put in this Directory
	CECATDirectoryItem* pNewDItem = new CECATDirectoryItem(file, matrixID);

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

	D("created new diritem with datablock start @ %lld (%lld)", FilePos2ECATBlock(pNewDItem->dataBlock_Start()),
																															pNewDItem->dataBlock_Start());			 

	// insert the new directory item into the directory now.
	dirItems.insert(matrixID, pNewDItem);

	// now that we have created a new directory item we have to
	// sync our main header again
	file->reWriteMainHeader();

	RETURN(pNewDItem);
	return pNewDItem;
}

QIODevice::Offset CECATDirectoryPrivate::lastDirItemOffset(void) const
{
	ENTER();
	QIODevice::Offset offset = 0;

	// we search through our item list and check the for the
	// very last item position
  QMap<Q_UINT32, CECATDirectoryItem*>::ConstIterator it;
	for(it=dirItems.begin(); it != dirItems.end(); ++it)
	{
		CECATDirectoryItem* dItem = it.data();

		if(dItem->dataBlock_End() > offset)
			offset = dItem->dataBlock_End();
	}

	// in addition to the diritem position check we have to check
	// the dirlist positions as well.
	for(unsigned int i=0; i < filePositions.count(); i++)
	{
		if(filePositions[i] > offset)
			offset = filePositions[i];
	}

	RETURN(offset);
	return offset;
}

short CECATDirectory::numFrames(void) const
{
	ENTER();
	short framesNum = 0;

	// we iterate through our dictionary looking for the highest available
	// frame number
  QMap<Q_UINT32, CECATDirectoryItem*>::ConstIterator it;
	for(it=m_pData->dirItems.begin(); it != m_pData->dirItems.end(); ++it)
	{
		CECATDirectoryItem* dItem = it.data();
		
		if(dItem->frame() > framesNum)
		{
			framesNum = dItem->frame();
			ASSERT(framesNum > 0 && framesNum <= 256);
		}
	}

	RETURN(framesNum);
	return framesNum;
}

short CECATDirectory::numPlanes(void) const
{
	short planesNum = 0;

	// we iterate through our dictionary looking for the highest available
	// plane number
  QMap<Q_UINT32, CECATDirectoryItem*>::ConstIterator it;
	for(it=m_pData->dirItems.begin(); it != m_pData->dirItems.end(); ++it)
	{
		CECATDirectoryItem* dItem = it.data();

		if(dItem->plane() > planesNum)
		{
			planesNum = dItem->plane();
			ASSERT(planesNum > 0 && planesNum <= 256);
		}
	}

	return planesNum;
}

short CECATDirectory::numGates(void) const
{
	short gatesNum = 0;

	// we iterate through our dictionary looking for the highest available
	// gates number
  QMap<Q_UINT32, CECATDirectoryItem*>::ConstIterator it;
	for(it=m_pData->dirItems.begin(); it != m_pData->dirItems.end(); ++it)
	{
		CECATDirectoryItem* dItem = it.data();

		if(dItem->gate() > gatesNum)
		{
			gatesNum = dItem->gate();
			ASSERT(gatesNum > 0 && gatesNum <= 256);
		}
	}

	return gatesNum;
}	

short CECATDirectory::numBedPos(void) const
{
	short bedsNum = 0;

	// we iterate through our dictionary looking for the highest available
	// plane number
  QMap<Q_UINT32, CECATDirectoryItem*>::ConstIterator it;
	for(it=m_pData->dirItems.begin(); it != m_pData->dirItems.end(); ++it)
	{
		CECATDirectoryItem* dItem = it.data();

		if(dItem->bed() > bedsNum)
		{
			bedsNum = dItem->bed();
			ASSERT(bedsNum >= 0 && bedsNum < 256);
		}
	}

	return bedsNum;
}	


bool CECATDirectory::readSubHeader(CECATSubHeader*& subHeader, short frame, short plane,
																	 short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	if(matrixParamsValid(frame, plane, gate, bed, data))
	{
		// get the directoryItem so that we can query the matrix from it
		QMap<Q_UINT32, CECATDirectoryItem*>::Iterator it = m_pData->dirItems.find(convertToMatrixID(frame, plane, gate, bed, data));

		if(it != m_pData->dirItems.end())
			result = it.data()->readSubHeader(subHeader);
	}
	
	RETURN(result);
	return result;
}

bool CECATDirectory::readMatrix(QByteArray*& matrixData, short frame, short plane,
															 short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	if(matrixParamsValid(frame, plane, gate, bed, data))
	{
		// get the directoryItem so that we can query the matrix from it
		QMap<Q_UINT32, CECATDirectoryItem*>::Iterator it = m_pData->dirItems.find(convertToMatrixID(frame, plane, gate, bed, data));

		if(it != m_pData->dirItems.end())
			result = it.data()->readMatrix(matrixData);
	}

	RETURN(result);
	return result;
}

bool CECATDirectory::readMatrix(char*& matrixData, unsigned int& len, short frame, short plane,
															  short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	if(matrixParamsValid(frame, plane, gate, bed, data))
	{
		// get the directoryItem so that we can query the matrix from it
		QMap<Q_UINT32, CECATDirectoryItem*>::Iterator it = m_pData->dirItems.find(convertToMatrixID(frame, plane, gate, bed, data));

		if(it != m_pData->dirItems.end())
			result = it.data()->readMatrix(matrixData, len);
	}
	
	RETURN(result);
	return result;
}

bool CECATDirectory::readMatrix(QByteArray*& matrixData, CECATSubHeader*& subHeader,
																short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	if(matrixParamsValid(frame, plane, gate, bed, data))
	{
		// get the directoryItem so that we can query the matrix from it
		QMap<Q_UINT32, CECATDirectoryItem*>::Iterator it = m_pData->dirItems.find(convertToMatrixID(frame, plane, gate, bed, data));

		if(it != m_pData->dirItems.end())
			result = it.data()->readMatrix(matrixData, subHeader);
	}
	
	RETURN(result);
	return result;
}

bool CECATDirectory::readMatrix(char*& matrixData, unsigned int& len, CECATSubHeader*& subHeader,
																short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	if(matrixParamsValid(frame, plane, gate, bed, data))
	{
		// get the directoryItem so that we can query the matrix from it
		QMap<Q_UINT32, CECATDirectoryItem*>::Iterator it = m_pData->dirItems.find(convertToMatrixID(frame, plane, gate, bed, data));

		if(it != m_pData->dirItems.end())
			result = it.data()->readMatrix(matrixData, len, subHeader);
	}
	
	RETURN(result);
	return result;
}

bool CECATDirectory::writeSubHeader(const CECATSubHeader& subHeader, short frame,
																		short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	if(matrixParamsValid(frame, plane, gate, bed, data))
	{	
		// form the MatrixID from the supplied data and create a new
		// DirectoryItem (or replace an existing one)
		Q_UINT32 mID = convertToMatrixID(frame, plane, gate, bed, data);
		D("Generated MatrixID: %08lx", mID);

		// see if we already have an item with the same matrixID in our
		// dictonary and if so we reuse that one and place a new subheader at
		// this position
		CECATDirectoryItem* pNewDItem;
		
		// get the directoryItem so that we can query the matrix from it
		QMap<Q_UINT32, CECATDirectoryItem*>::Iterator it = m_pData->dirItems.find(mID);
		if(it != m_pData->dirItems.end())
			pNewDItem = it.data();
		else
		{
			// create a new DirectoryItem which we put in this Directory
			pNewDItem = m_pData->newDirItem(mID);
		}

		// then we make sure the subheader is written to the ECAT file.
		result = pNewDItem->writeSubHeader(subHeader);
		if(result)
			save();
	}

	RETURN(result);
	return result;
}

bool CECATDirectory::writeMatrix(const QByteArray& matrixData,
																 short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	if(matrixParamsValid(frame, plane, gate, bed, data))
	{
		// form the MatrixID from the supplied data and create a new
		// DirectoryItem (or replace an existing one)
		Q_UINT32 mID = convertToMatrixID(frame, plane, gate, bed, data);
		D("Generated MatrixID: %08lx", mID);

		// see if we already have an item with the same matrixID in our
		// dictonary and if so we reuse that one and place a new subheader at
		// this position
		CECATDirectoryItem* pNewDItem;
		
		// get the directoryItem so that we can query the matrix from it
		QMap<Q_UINT32, CECATDirectoryItem*>::Iterator it = m_pData->dirItems.find(mID);
		if(it != m_pData->dirItems.end())
			pNewDItem = it.data();
		else
		{
			// create a new DirectoryItem which we put in this Directory
			pNewDItem = m_pData->newDirItem(mID);
		}

		// then we make sure the subheader is written to the ECAT file.
		result = pNewDItem->writeMatrix(matrixData);
		if(result)
			save();
	}
	
	RETURN(result);
	return result;
}

bool CECATDirectory::writeMatrix(const char* matrixData, unsigned int size,
																 short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	if(matrixParamsValid(frame, plane, gate, bed, data))
	{	
		// form the MatrixID from the supplied data and create a new
		// DirectoryItem (or replace an existing one)
		Q_UINT32 mID = convertToMatrixID(frame, plane, gate, bed, data);
		D("Generated MatrixID: %08lx", mID);

		// see if we already have an item with the same matrixID in our
		// dictonary and if so we reuse that one and place a new subheader at
		// this position
		CECATDirectoryItem* pNewDItem;
		
		// get the directoryItem so that we can query the matrix from it
		QMap<Q_UINT32, CECATDirectoryItem*>::Iterator it = m_pData->dirItems.find(mID);
		if(it != m_pData->dirItems.end())
			pNewDItem = it.data();
		else
		{
			// create a new DirectoryItem which we put in this Directory
			pNewDItem = m_pData->newDirItem(mID);
		}

		// then we make sure the subheader is written to the ECAT file.
		result = pNewDItem->writeMatrix(matrixData, size);
		if(result)
			save();
	}

	RETURN(result);
	return result;
}

bool CECATDirectory::writeMatrix(const QByteArray& matrixData, CECATSubHeader::Data_Type type,
																 short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	if(matrixParamsValid(frame, plane, gate, bed, data))
	{
		// form the MatrixID from the supplied data and create a new
		// DirectoryItem (or replace an existing one)
		Q_UINT32 mID = convertToMatrixID(frame, plane, gate, bed, data);
		D("Generated MatrixID: %08lx", mID);

		// see if we already have an item with the same matrixID in our
		// dictonary and if so we reuse that one and place a new subheader at
		// this position
		CECATDirectoryItem* pNewDItem;
		
		// get the directoryItem so that we can query the matrix from it
		QMap<Q_UINT32, CECATDirectoryItem*>::Iterator it = m_pData->dirItems.find(mID);
		if(it != m_pData->dirItems.end())
			pNewDItem = it.data();
		else
		{
			// create a new DirectoryItem which we put in this Directory
			pNewDItem = m_pData->newDirItem(mID);
		}

		// then we make sure the subheader is written to the ECAT file.
		result = pNewDItem->writeMatrix(matrixData, type);
		if(result)
			save();
	}
	
	RETURN(result);
	return result;
}

bool CECATDirectory::writeMatrix(const char* matrixData, unsigned int size,
																 CECATSubHeader::Data_Type type,
																 short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	if(matrixParamsValid(frame, plane, gate, bed, data))
	{
		// form the MatrixID from the supplied data and create a new
		// DirectoryItem (or replace an existing one)
		Q_UINT32 mID = convertToMatrixID(frame, plane, gate, bed, data);
		D("Generated MatrixID: %08lx", mID);

		// see if we already have an item with the same matrixID in our
		// dictonary and if so we reuse that one and place a new subheader at
		// this position
		CECATDirectoryItem* pNewDItem;
		
		// get the directoryItem so that we can query the matrix from it
		QMap<Q_UINT32, CECATDirectoryItem*>::Iterator it = m_pData->dirItems.find(mID);
		if(it != m_pData->dirItems.end())
			pNewDItem = it.data();
		else
		{
			// create a new DirectoryItem which we put in this Directory
			pNewDItem = m_pData->newDirItem(mID);
		}

		// then we make sure the subheader is written to the ECAT file.
		result = pNewDItem->writeMatrix(matrixData, size, type);
		if(result)
			save();
	}

	RETURN(result);
	return result;
}

bool CECATDirectory::writeMatrix(const QByteArray& matrixData, const CECATSubHeader& subHeader,
																 short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	if(matrixParamsValid(frame, plane, gate, bed, data))
	{	
		// form the MatrixID from the supplied data and create a new
		// DirectoryItem (or replace an existing one)
		Q_UINT32 mID = convertToMatrixID(frame, plane, gate, bed, data);
		D("Generated MatrixID: %08lx", mID);

		// see if we already have an item with the same matrixID in our
		// dictonary and if so we reuse that one and place a new subheader at
		// this position
		CECATDirectoryItem* pNewDItem;
		
		// get the directoryItem so that we can query the matrix from it
		QMap<Q_UINT32, CECATDirectoryItem*>::Iterator it = m_pData->dirItems.find(mID);
		if(it != m_pData->dirItems.end())
			pNewDItem = it.data();
		else
		{
			// create a new DirectoryItem which we put in this Directory
			pNewDItem = m_pData->newDirItem(mID);
		}

		// then we make sure the subheader is written to the ECAT file.
		result = pNewDItem->writeMatrix(matrixData, subHeader);
		if(result)
			save();
	}
	
	RETURN(result);
	return result;
}

bool CECATDirectory::writeMatrix(const char* matrixData, unsigned int size, const CECATSubHeader& subHeader,
																 short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	if(matrixParamsValid(frame, plane, gate, bed, data))
	{	
		// form the MatrixID from the supplied data and create a new
		// DirectoryItem (or replace an existing one)
		Q_UINT32 mID = convertToMatrixID(frame, plane, gate, bed, data);
		D("Generated MatrixID: %08lx", mID);

		// see if we already have an item with the same matrixID in our
		// dictonary and if so we reuse that one and place a new subheader at
		// this position
		CECATDirectoryItem* pNewDItem;
		
		// get the directoryItem so that we can query the matrix from it
		QMap<Q_UINT32, CECATDirectoryItem*>::Iterator it = m_pData->dirItems.find(mID);
		if(it != m_pData->dirItems.end())
			pNewDItem = it.data();
		else
		{
			// create a new DirectoryItem which we put in this Directory
			pNewDItem = m_pData->newDirItem(mID);
		}

		// then we make sure the subheader is written to the ECAT file.
		result = pNewDItem->writeMatrix(matrixData, size, subHeader);
		if(result)
			save();
	}

	RETURN(result);
	return result;
}

CECATDirectoryItem* CECATDirectory::operator[](unsigned int num) const
{
	ENTER();

	// use an QMap iterator to iterate until we got the num'th
	// element in our dictonary and return it
	QMap<Q_UINT32, CECATDirectoryItem*>::ConstIterator it = m_pData->dirItems.begin();
	for(unsigned int i=0; i < num && it != m_pData->dirItems.end(); i++)
		++it;

	CECATDirectoryItem* dItem;
	if(it != m_pData->dirItems.end())
		dItem = it.data();
	else
		dItem = NULL;

	RETURN(dItem);
	return dItem;
}
