/* -*- mode: c++; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 * vim:set ts=2 sw=2 expandtab: *********************************************
 *
 * libmedio - C++ I/O Library for loading/saving medical data formats
 * Copyright (C) 2006-2010 by Jens Langner <Jens.Langner@light-speed.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id$
 *
 **************************************************************************/

#include "CPhilipsDirectory.h"
#include "CPhilipsDirectoryItem.h"
#include "CPhilipsFile.h"

#include <QMap>
#include <QVector>
#include <rtdebug.h>

// some very own defines specifying the boundaries
// of a philips directory list
#define PHILIPS_DIRHEAD_SIZE 16
#define PHILIPS_DIRITEM_SIZE 16
#define PHILIPS_DIRITEM_NUM  31
#define PHILIPS_DIRLIST_SIZE (PHILIPS_DIRHEAD_SIZE+PHILIPS_DIRITEM_NUM*PHILIPS_DIRITEM_SIZE)

// the RAW structures used in the philips directory blocks
struct Philips_DirHead          // should be 16 bytes
{
  quint32 freeItems;
  quint32 nextDirectory;
  quint32 prevDirectory;
  quint32 itemsToFollow;
};

struct Philips_DirItem          // should be 16 bytes
{
  quint32 matrixID;             // bits 0-15 Frame, 16-27 slice, 28-31 tilt (lsb 0)
  quint32 dataBlock_Start;      // start position of the subHeader
  quint32 dataBlock_End;        // end position of the last data block
  quint16 compressionFlag;      // 0x0001: uncompressed, 0x0101: compressed
  quint16 contentFlag;          // 0xFFFE: header, 0xFFFF: removed, 0x0000: unused, 0x0001: used
};

struct Philips_DirList // should be 512 bytes
{
  struct Philips_DirHead head;
  struct Philips_DirItem items[PHILIPS_DIRITEM_NUM];
};

// macro for quickly checking if the frame/plane/gate/bed/data
// matrix parameters are between bounds or not
#define matrixParamsValid(s, f, t) ((s) >= 0 && (s) < 4095 && \
                                    (f) >= 0 && (f) < 100 && \
                                    (t) >= 0 && (t) < 15)

class CPhilipsDirectoryPrivate
{
  public:
    CPhilipsFile* file;            // ptr to our associated philipsFile
    QMap<quint32, CPhilipsDirectoryItem*> dirItems; // value map of dirItems
    QVector<qint64> filePositions; // for each DirList we
                                   // may have different file
                                   // positions which we have
                                   // to store

    // some private methods 
    CPhilipsDirectoryItem* newDirItem(quint32 matrixID);
    qint64 lastDirItemOffset(void) const;
};

CPhilipsDirectory::CPhilipsDirectory(CPhilipsFile* philipsFile)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CPhilipsDirectoryPrivate();  

  // create the int dictonary for the dirItems and set it to autodelete
  m_pData->dirItems.clear();

  // create a new value vector for carrying all the different file
  // positions this directory list is splitted to.
  m_pData->filePositions.clear();
  m_pData->filePositions.append(PhilipsBlock2FilePos(PHILIPS_POS_MAINDIR));

  // save the pihlipsFile pointer
  m_pData->file = philipsFile;

  LEAVE();
}

CPhilipsDirectory::~CPhilipsDirectory()
{
  ENTER();

  // now we iterate through our QHash and make sure we delete all our
  // stored diritems correctly.
  QMapIterator<quint32, CPhilipsDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();
    delete i.value();
  }

  delete m_pData;

  LEAVE();
}

CPhilipsDirectory::CPhilipsDirectory(const CPhilipsDirectory& src)
{
  ENTER();

  // allocate data from our private instance class  
  m_pData = new CPhilipsDirectoryPrivate();

  // copy the instance data
  *this = src;

  // set the file ptr to NULL  
  m_pData->file = NULL;

  LEAVE();
}
  
CPhilipsDirectory& CPhilipsDirectory::operator=(const CPhilipsDirectory& src)
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

bool CPhilipsDirectory::load()
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
  if(m_pData->file->seek(PhilipsBlock2FilePos(PHILIPS_POS_MAINDIR)) == false)
  {
    RETURN(false);
    return false;
  }

  // we process the splitted directory list in a do..while loop
  // and jump from one dirlist to another in the philips file.
  struct Philips_DirList dList;
  do
  {
    // add each DirList position to our filePositions value list
    // for later reference
    SHOWVALUE(m_pData->file->pos());
    m_pData->filePositions.append(m_pData->file->pos());

    // we use a ByteArray buffer to speed up the endianess decoding
    QByteArray buffer(PHILIPS_DIRLIST_SIZE, 0);
    if(m_pData->file->read(buffer.data(), buffer.size()) != PHILIPS_DIRLIST_SIZE) 
    {
      E("An error occurred while reading data");

      result = false;
      break;
    }

    // now we generate a QDataStream on our buffer so that we can read
    // out of the buffer instead of the raw file (> speed)
    QDataStream stream(buffer);

    // read out the dirHead first
    stream >> dList.head.freeItems;
    stream >> dList.head.nextDirectory;
    stream >> dList.head.prevDirectory;
    stream >> dList.head.itemsToFollow;
    
    // output some debug information on the head of the dirList.
    D("DirHead.freeItems   : %d", dList.head.freeItems);
    D("DirHead.next        : %d", dList.head.nextDirectory);
    D("DirHead.prev        : %d", dList.head.prevDirectory);
    D("DirHead.itemToFollow: %d", dList.head.itemsToFollow);

    // now we know how many item will follow in the Directory, so we
    // can populate our Directory here.
    unsigned int iItemsInserted = 0;
    for(unsigned int i=0; i < dList.head.itemsToFollow; i++)
    {
      CPhilipsDirectoryItem* pNewDirItem = new CPhilipsDirectoryItem(m_pData->file);

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
        ASSERT(m_pData->dirItems.contains(pNewDirItem->matrixID()) == false);
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
    if(dList.head.itemsToFollow != iItemsInserted)
    {
      W("ItemsToFollow:%ld != iItemsInserted:%ld", dList.head.itemsToFollow, iItemsInserted);
    }
    #endif

    if(m_pData->file->seek(PhilipsBlock2FilePos(dList.head.nextDirectory)) == false)
    {
      E("couldn't seek to file position %lld.", PhilipsBlock2FilePos(dList.head.nextDirectory));
      result = false;
      break;
    }
  }
  while((dList.head.nextDirectory > PHILIPS_POS_MAINDIR) && (result == true));

  RETURN(result);
  return result;
}

bool CPhilipsDirectory::save() const
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
  // even if the Philips standard doesn't define a ordering, but some
  // stupid software seems to require it.
  struct Philips_DirHead dirHead;

  // before we start we have to link the first dirList to itself
  memset(&dirHead, 0, sizeof(struct Philips_DirHead)); // clear it first
  dirHead.freeItems  = PHILIPS_DIRITEM_NUM;
  dirHead.nextDirectory = PHILIPS_POS_MAINDIR;

  // we use two buffers. one for storing the dirHead of the Philips and
  // one for the 31 dirItems.
  QByteArray dirItemBuffer(PHILIPS_DIRITEM_NUM*PHILIPS_DIRITEM_SIZE, 0);
  QDataStream dirItemStream(&dirItemBuffer, QIODevice::WriteOnly);

  // now we have to go through our directory and stream all items
  // in 31 chunks as the directory list can only have 31 items plus
  // 1 chunk for the header
  unsigned int currentDirPos = PHILIPS_POS_MAINDIR;
  unsigned int processedDirItems = 0;
  unsigned short curDirList = 0;
  quint32 nextDirPos;
  qint64 lastDirListPos = 0;

  // now iterate through our sorted QMap and write out the
  // data sorted as well.
  QMapIterator<quint32, CPhilipsDirectoryItem*> it(m_pData->dirItems);
  while(it.hasNext())
  {
    it.next();
    
    CPhilipsDirectoryItem* pDirItem = it.value();
    if(pDirItem)
    {
      processedDirItems++;

      D("found DirItem: %d/%d/%d", pDirItem->slice(), 
                                   pDirItem->frame(), 
                                   pDirItem->tilt());

      // we fix a problem where it happened that in case the writeSubHeader()/
      // writeMatrix() where used separately, the directory got overwritten
      // but the very next directory item.
      if(lastDirListPos > 0 && lastDirListPos == pDirItem->dataBlock_Start())
        pDirItem->setDataBlock_Start(lastDirListPos+PHILIPS_DIRLIST_SIZE);
      else
        lastDirListPos = 0;
      
      // stream in the directory Item information
      dirItemStream << *pDirItem;

      // increment the itemsToFollow value and decrement the 
      // freeItems
      dirHead.itemsToFollow++;
      dirHead.freeItems--;
      
      // first we check wheter the dirList is filled up (>=31 items)
      // and then write it out separate first.
      if(dirHead.itemsToFollow == PHILIPS_DIRITEM_NUM && 
         processedDirItems < count())
      {
        qint64 appendPos;

        // get the position where the next directorylist will start
        // so we look into our filePositions ValueVector and if there is
        // another entry we use it or we create a new directoryList
        if(m_pData->filePositions.count()-1 == curDirList)
        {      
          appendPos = m_pData->lastDirItemOffset()+PHILIPS_BLOCKSIZE;

          // append the position to our FilePositions for DirLists
          m_pData->filePositions.append(appendPos);
          
          D("appended new DirList #%d @ %lld (%lld)", curDirList+1, FilePos2PhilipsBlock(appendPos), appendPos);

          lastDirListPos = appendPos;
        }
        else
          appendPos = m_pData->filePositions[curDirList+1];

        // calculate the block position where the next directory list
        // starts (the current endsize of the Philips file substracted by one
        // Philips blocksize [512 bytes])
        nextDirPos = FilePos2PhilipsBlock(appendPos);
        dirHead.nextDirectory = nextDirPos;

        // now we can write out the whole directory List to the file
        // where we first write out the dirHead and then the 31 dirItems
        QByteArray dirHeadBuffer(PHILIPS_DIRHEAD_SIZE, 0);
        QDataStream dirHeadStream(&dirHeadBuffer, QIODevice::WriteOnly);

        dirHeadStream << dirHead.freeItems;
        dirHeadStream << dirHead.nextDirectory;
        dirHeadStream << dirHead.prevDirectory;
        dirHeadStream << dirHead.itemsToFollow;
          
        // now we can seek to the file position of the DirList
        // in the file.
        if(m_pData->file->seek(m_pData->filePositions[curDirList]) == false)
        {
          E("Error while seeking to DirList position: %lld", m_pData->filePositions[curDirList]);
          result = false;
          break;
        }
          
        D("DirHead%d.Position     : %d (%d)", curDirList, FilePos2PhilipsBlock(m_pData->file->pos()), m_pData->filePositions[curDirList]);
        D("DirHead%d.freeItems    : %d", curDirList, dirHead.freeItems);
        D("DirHead%d.nextDirectory         : %d", curDirList, dirHead.nextDirectory);
        D("DirHead%d.prevDirectory         : %d", curDirList, dirHead.prevDirectory);
        D("DirHead%d.itemsToFollow: %d", curDirList, dirHead.itemsToFollow);

        // write out everything
        if(m_pData->file->write(dirHeadBuffer) != PHILIPS_DIRHEAD_SIZE ||
           m_pData->file->write(dirItemBuffer) != PHILIPS_DIRITEM_NUM*PHILIPS_DIRITEM_SIZE)
        {
          E("Error while writing DirList");
          result = false;
          break;
        }
        else
          D("DirList #%d successfully written @ %lld (%lld)", curDirList, 
                                                              FilePos2PhilipsBlock(m_pData->file->pos()-PHILIPS_DIRLIST_SIZE),
                                                              m_pData->file->pos()-PHILIPS_DIRLIST_SIZE);

        // clear the dirHead so that we can immediately reuse it
        dirHead.freeItems      = PHILIPS_DIRITEM_NUM;
        dirHead.nextDirectory          = PHILIPS_POS_MAINDIR;
        dirHead.prevDirectory          = currentDirPos;
        dirHead.itemsToFollow = 0;
        currentDirPos          = nextDirPos;

        // clear also the DirItem buffer
        memset(dirItemBuffer.data(), 0, dirItemBuffer.size());
        if(dirItemStream.device()->seek(0) == false)
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
    QByteArray dirHeadBuffer(PHILIPS_DIRHEAD_SIZE, 0);
    QDataStream dirHeadStream(&dirHeadBuffer, QIODevice::WriteOnly);

    dirHeadStream << dirHead.freeItems;
    dirHeadStream << dirHead.nextDirectory;
    dirHeadStream << dirHead.prevDirectory;
    dirHeadStream << dirHead.itemsToFollow;
      
    // now we can seek to the file position of the DirList
    // in the file.
    if(m_pData->file->seek(m_pData->filePositions[curDirList]) == false)
    {
      E("Error while seeking to DirList position: %d", m_pData->filePositions[curDirList]);
      
      result = false;
    }
    else
    {
      D("DirHead%d.Position     : %lld (%lld)", curDirList, FilePos2PhilipsBlock(m_pData->file->pos()), m_pData->filePositions[curDirList]);
      D("DirHead%d.freeItems    : %d", curDirList, dirHead.freeItems);
      D("DirHead%d.nextDirectory         : %d", curDirList, dirHead.nextDirectory);
      D("DirHead%d.prevDirectory         : %d", curDirList, dirHead.prevDirectory);
      D("DirHead%d.itemsToFollow: %d", curDirList, dirHead.itemsToFollow);

      // write out everything
      if(m_pData->file->write(dirHeadBuffer) != PHILIPS_DIRHEAD_SIZE ||
         m_pData->file->write(dirItemBuffer) != PHILIPS_DIRITEM_NUM*PHILIPS_DIRITEM_SIZE)
      {
        E("Error while writing DirList #%d at %lld", curDirList, m_pData->file->pos());
        result = false;
      }
      else
        D("DirList #%d successfully written @ %lld (%lld)", curDirList, 
                                                            FilePos2PhilipsBlock(m_pData->file->pos()-PHILIPS_DIRLIST_SIZE),
                                                            m_pData->file->pos()-PHILIPS_DIRLIST_SIZE);
    }
  }

  RETURN(result);
  return result;
}

bool CPhilipsDirectory::isEmpty() const
{ 
  return m_pData->dirItems.isEmpty();
}

unsigned int CPhilipsDirectory::count() const
{ 
  return m_pData->dirItems.count();
}

CPhilipsDirectoryItem* CPhilipsDirectory::item(short frame, short slice, short tilt)
{
  ENTER();
  CPhilipsDirectoryItem* dirItem = NULL;

  if(frame == -1) frame = minFrame();
  if(slice == -1) slice = minSlice();
  if(tilt == -1) tilt = minTilt();

  if(matrixParamsValid(slice, frame, tilt))
  {
    // form the MatrixID from the supplied data and
    // process a find in the QIntDict
    quint32 mID = convertToMatrixID(slice, frame, tilt);
    D("Generated MatrixID: %08lx", mID);

    dirItem = m_pData->dirItems.value(mID);
  }

  RETURN(dirItem);
  return dirItem;
}

CPhilipsDirectoryItem* CPhilipsDirectoryPrivate::newDirItem(quint32 matrixID)
{
  ENTER();
  CPhilipsDirectoryItem* pNewDItem = new CPhilipsDirectoryItem(file, matrixID);

  // now that we generated a new directory item we want to put
  // in our directory immediately, we have to first check at which
  // dataposition it should be placed
  qint64 dataOffset = lastDirItemOffset();

  // modify the Start offset of the item now. So if this is the first
  // one we place it directly behind the maindirectory, which should
  // be block 3. If this is not the first one we place it behind the
  // lastDirItemOffset...
  if(dataOffset > 0)
    pNewDItem->setDataBlock_Start(dataOffset+PHILIPS_BLOCKSIZE);
  else
    pNewDItem->setDataBlock_Start(PhilipsBlock2FilePos(PHILIPS_POS_MAINDIR+1));

  D("created new diritem with datablock start @ %lld (%lld)", FilePos2PhilipsBlock(pNewDItem->dataBlock_Start()),
                                                              pNewDItem->dataBlock_Start());       

  // insert the new directory item into the directory now.
  dirItems.insert(matrixID, pNewDItem);

  // now that we have created a new directory item we have to
  // sync our main header again
  // we do not sync if the new diritem is an extended main header
  if(pNewDItem->isExtendedHeader() == false)
    file->reWriteMainHeader();

  RETURN(pNewDItem);
  return pNewDItem;
}

qint64 CPhilipsDirectoryPrivate::lastDirItemOffset(void) const
{
  ENTER();
  qint64 offset = 0;

  // we search through our item list and check the for the
  // very last item position
  QMapIterator<quint32, CPhilipsDirectoryItem*> i(dirItems);
  while(i.hasNext())
  {
    i.next();
    if(i.value()->dataBlock_End() > offset)
      offset = i.value()->dataBlock_End();
  }

  // in addition to the diritem position check we have to check
  // the dirlist positions as well.
  for(unsigned int i=0; i < static_cast<unsigned int>(filePositions.count()); i++)
  {
    if(filePositions[i] > offset)
      offset = filePositions[i];
  }

  RETURN(offset);
  return offset;
}

short CPhilipsDirectory::maxFrame() const
{
  ENTER();
  short framesMax = 0;

  // we iterate through our dictionary looking for the highest available
  // frame number
  QMapIterator<quint32, CPhilipsDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();
    // skip extended header
    if(i.value()->isExtendedHeader() == false)
    {
      if(i.value()->frame() > framesMax)
      {
        framesMax = i.value()->frame();
        ASSERT(framesMax > 0 && framesMax <= 100);
      }
    }
  }
  
  RETURN(framesMax);
  return framesMax;
}

short CPhilipsDirectory::minFrame() const
{
  ENTER();
  short framesMin = 101;

  // we iterate through our dictionary looking for the lowest available
  // frame number
  QMapIterator<quint32, CPhilipsDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();
    // skip extended header
    if(i.value()->isExtendedHeader() == false)
    {
      if(i.value()->frame() < framesMin)
      {
        framesMin = i.value()->frame();
        ASSERT(framesMin > 0 && framesMin <= 100);
      }
    }
  }

  // if framesMin is still 101, there are no frames in the map
  // in this case we have to set the min Value to 0
  if(framesMin == 101)
    framesMin = 0;
  
  RETURN(framesMin);
  return framesMin;
}

short CPhilipsDirectory::numFrames() const
{
  ENTER();
  short framesNum = 0;

  // we iterate through our dictionary looking for the highest available
  // frame number
  QMapIterator<quint32, CPhilipsDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();
    // skip extended header
    if(i.value()->isExtendedHeader() == false)
    {
      if(i.value()->frame() > framesNum)
      {
        framesNum = i.value()->frame();
        ASSERT(framesNum > 0 && framesNum <= 100);
      }
    }
  }
  
  RETURN(framesNum);
  return framesNum;
}

short CPhilipsDirectory::maxSlice() const
{
  ENTER();
  short slicesMax = 0;

  // we iterate through our dictionary looking for the highest
  // available slice number
  QMapIterator<quint32, CPhilipsDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();

    // skip extended header
    if(i.value()->isExtendedHeader() == false)
    {
      if(i.value()->slice() > slicesMax)
      {
        slicesMax = i.value()->slice();
        ASSERT(slicesMax > 0 && slicesMax <= 4095);
      }
    }
  }
  
  RETURN(slicesMax);
  return slicesMax;
}

short CPhilipsDirectory::minSlice() const
{
  ENTER();
  short slicesMin = 4096;

  // we iterate through our dictionary looking for the lowest
  // available slice number
  QMapIterator<quint32, CPhilipsDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();

    // skip extended header
    if(i.value()->isExtendedHeader() == false)
    {
      if(i.value()->slice() < slicesMin)
      {
        slicesMin = i.value()->slice();
        ASSERT(slicesMin > 0 && slicesMin <= 4095);
      }
    }
  }

  // if slicesMin is still 4096, there are no slices in the map
  // in this case we have to set the min Value to 0
  if(slicesMin == 4096)
    slicesMin = 0;

  RETURN(slicesMin);
  return slicesMin;
}

short CPhilipsDirectory::numSlices(short frame) const
{
  ENTER();
  short numSlices = 0;

  if(frame == -1)
    frame = minFrame();

  // we iterate through our dictionary looking for the lowest
  // available slice number
  QMapIterator<quint32, CPhilipsDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();

    // skip extended header
    if(i.value()->isExtendedHeader() == false)
    {
      if(i.value()->frame() == frame)
        numSlices++;
    }
  }

  RETURN(numSlices);
  return numSlices;
}

short CPhilipsDirectory::maxTilt() const
{
  ENTER();
  short tiltMax = 0;

  // we iterate through our dictionary looking for the highest
  // available slice number
  QMapIterator<quint32, CPhilipsDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();

    // skip extended header
    if(i.value()->isExtendedHeader() == false)
    {
      if(i.value()->tilt() > tiltMax)
      {
        tiltMax = i.value()->tilt();
        ASSERT(tiltMax > 0 && tiltMax <= 15);
      }
    }
  }
  
  RETURN(tiltMax);
  return tiltMax;
}

short CPhilipsDirectory::minTilt() const
{
  ENTER();
  short tiltMin = 15;

  // we iterate through our dictionary looking for the lowest
  // available slice number
  QMapIterator<quint32, CPhilipsDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();

    // skip extended header
    if(i.value()->isExtendedHeader() == false)
    {
      if(i.value()->tilt() < tiltMin)
      {
        tiltMin = i.value()->tilt();
        ASSERT(tiltMin > 0 && tiltMin <= 15);
      }
    }
  }

  // if slicesMin is still 4096, there are no slices in the map
  // in this case we have to set the min Value to 0
  if(tiltMin == 15)
    tiltMin = 0;

  RETURN(tiltMin);
  return tiltMin;
}


short CPhilipsDirectory::numTilts() const
{
  ENTER();
  short tiltsNum = 0;

  // we iterate through our dictionary looking for the highest available
  // tilt number
  QMapIterator<quint32, CPhilipsDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();

    // skip extended header
    if(i.value()->isExtendedHeader() == false)
    {
      if(i.value()->tilt() > tiltsNum)
      {
        tiltsNum = i.value()->tilt();
        ASSERT(tiltsNum > 0 && tiltsNum <= 15);
      }
    }
  }
  
  RETURN(tiltsNum);
  return tiltsNum;
}

CPhilipsDirectoryItem* CPhilipsDirectory::extendedMainHeaderItem() const
{
  ENTER();
  CPhilipsDirectoryItem* item = NULL;
  
  // we iterate through our dictionary looking for an extendedMainHeader
  QMapIterator<quint32, CPhilipsDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();
    if(i.value()->isExtendedHeader() == true)
    {
      // we found the extendedMainHeader item so lets return it
      item = i.value();
      break;
    }
  }

  // if we still didn't find any extened main header item in the
  // directory we simply create one.
  if(item == NULL)
  {
    item = m_pData->newDirItem(PHILIPS_EXTENDED_HEADER);
    item->setContentFlag(CPhilipsDirectoryItem::Header);
  }
  
  RETURN(item);
  return item;
}

bool CPhilipsDirectory::readSubHeader(CPhilipsSubHeader*& subHeader,
                                      short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;

  if(frame == -1) frame = minFrame();
  if(slice == -1) slice = minSlice();
  if(tilt == -1) tilt = minTilt();

  if(matrixParamsValid(slice, frame, tilt))
  {
    // get the directoryItem so that we can query the matrix from it
    CPhilipsDirectoryItem* pDirItem = m_pData->dirItems.value(convertToMatrixID(slice, frame, tilt));
    SHOWVALUE(pDirItem);
    if(pDirItem)
      result = pDirItem->readSubHeader(subHeader);
  }
  
  RETURN(result);
  return result;
}

bool CPhilipsDirectory::readMatrix(QByteArray*& matrixData,
                                   short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;

  if(frame == -1) frame = minFrame();
  if(slice == -1) slice = minSlice();
  if(tilt == -1) tilt = minTilt();

  if(matrixParamsValid(slice, frame, tilt))
  {
    // get the directoryItem so that we can query the matrix from it
    CPhilipsDirectoryItem* pDirItem = m_pData->dirItems.value(convertToMatrixID(slice, frame, tilt));

    if(pDirItem)
      result = pDirItem->readMatrix(matrixData);
  }

  RETURN(result);
  return result;
}

bool CPhilipsDirectory::readMatrix(char*& matrixData, unsigned int& len,
                                   short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;

  if(frame == -1) frame = minFrame();
  if(slice == -1) slice = minSlice();
  if(tilt == -1) tilt = minTilt();

  if(matrixParamsValid(slice, frame, tilt))
  {
    // get the directoryItem so that we can query the matrix from it
    CPhilipsDirectoryItem* pDirItem = m_pData->dirItems.value(convertToMatrixID(slice, frame, tilt));

    if(pDirItem)
      result = pDirItem->readMatrix(matrixData, len);
  }
  
  RETURN(result);
  return result;
}

bool CPhilipsDirectory::readMatrix(QByteArray*& matrixData, CPhilipsSubHeader*& subHeader,
                                   short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;

  if(frame == -1) frame = minFrame();
  if(slice == -1) slice = minSlice();
  if(tilt == -1) tilt = minTilt();

  if(matrixParamsValid(slice, frame, tilt))
  {
    // get the directoryItem so that we can query the matrix from it
    CPhilipsDirectoryItem* pDirItem = m_pData->dirItems.value(convertToMatrixID(slice, frame, tilt));

    if(pDirItem)
      result = pDirItem->readMatrix(matrixData, subHeader);
  }
  
  RETURN(result);
  return result;
}

bool CPhilipsDirectory::readMatrix(char*& matrixData, unsigned int& len, CPhilipsSubHeader*& subHeader,
                                   short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;
 
  if(frame == -1) frame = minFrame();
  if(slice == -1) slice = minSlice();
  if(tilt == -1) tilt = minTilt();

  if(matrixParamsValid(slice, frame, tilt))
  {
    // get the directoryItem so that we can query the matrix from it
    CPhilipsDirectoryItem* pDirItem = m_pData->dirItems.value(convertToMatrixID(slice, frame, tilt));

    if(pDirItem)
      result = pDirItem->readMatrix(matrixData, len, subHeader);
  }
  
  RETURN(result);
  return result;
}

bool CPhilipsDirectory::writeSubHeader(const CPhilipsSubHeader& subHeader,
                                       short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;

  if(frame == -1) frame = minFrame();
  if(slice == -1) slice = minSlice();
  if(tilt == -1) tilt = minTilt();

  if(matrixParamsValid(slice, frame, tilt))
  {
    quint32 mID = convertToMatrixID(slice, frame, tilt);
    D("Generated MatrixID: %08lx", mID);

    // see if we already have an item with the same matrixID in our
    // dictonary and if so we reuse that one and place a new subheader at
    // this position
    CPhilipsDirectoryItem* pNewDItem = m_pData->dirItems.value(mID);
    if(pNewDItem == NULL)
      pNewDItem = m_pData->newDirItem(mID);

    // then we make sure the subheader is written to the Philips file.
    result = pNewDItem->writeSubHeader(subHeader);
    if(result)
      save();
  }

  RETURN(result);
  return result;
}

bool CPhilipsDirectory::writeMatrix(const QByteArray& matrixData,
                                    short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;

  if(frame == -1) frame = minFrame();
  if(slice == -1) slice = minSlice();
  if(tilt == -1) tilt = minTilt();

  if(matrixParamsValid(slice, frame, tilt))
  {
    // form the MatrixID from the supplied data and create a new
    // DirectoryItem (or replace an existing one)
    quint32 mID = convertToMatrixID(slice, frame, tilt);
    D("Generated MatrixID: %08lx", mID);

    // see if we already have an item with the same matrixID in our
    // dictonary and if so we reuse that one and place a new subheader at
    // this position
    CPhilipsDirectoryItem* pNewDItem = m_pData->dirItems.value(mID);
    if(pNewDItem == NULL)
    {
      // create a new DirectoryItem which we put in this Directory
      pNewDItem = m_pData->newDirItem(mID);
    }

    // then we make sure the subheader is written to the Philips file.
    result = pNewDItem->writeMatrix(matrixData);
    if(result)
      save();
  }
  
  RETURN(result);
  return result;
}

bool CPhilipsDirectory::writeMatrix(const char* matrixData, unsigned int size,
                                    short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;

  if(frame == -1) frame = minFrame();
  if(slice == -1) slice = minSlice();
  if(tilt == -1) tilt = minTilt();

  if(matrixParamsValid(slice, frame, tilt))
  {  
    // form the MatrixID from the supplied data and create a new
    // DirectoryItem (or replace an existing one)
    quint32 mID = convertToMatrixID(slice, frame, tilt);
    D("Generated MatrixID: %08lx", mID);

    // see if we already have an item with the same matrixID in our
    // dictonary and if so we reuse that one and place a new subheader at
    // this position
    CPhilipsDirectoryItem* pNewDItem = m_pData->dirItems.value(mID);
    if(pNewDItem == NULL)
    {
      // create a new DirectoryItem which we put in this Directory
      pNewDItem = m_pData->newDirItem(mID);
    }

    // then we make sure the subheader is written to the Philips file.
    result = pNewDItem->writeMatrix(matrixData, size);
    if(result)
      save();
  }

  RETURN(result);
  return result;
}

bool CPhilipsDirectory::writeMatrix(const QByteArray& matrixData, CPhilipsSubHeader::Data_Type type,
                                    short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;

  if(frame == -1) frame = minFrame();
  if(slice == -1) slice = minSlice();
  if(tilt == -1) tilt = minTilt();

  if(matrixParamsValid(slice, frame, tilt))
  {
    // form the MatrixID from the supplied data and create a new
    // DirectoryItem (or replace an existing one)
    quint32 mID = convertToMatrixID(slice, frame, tilt);
    D("Generated MatrixID: %08lx", mID);

    // see if we already have an item with the same matrixID in our
    // dictonary and if so we reuse that one and place a new subheader at
    // this position
    CPhilipsDirectoryItem* pNewDItem = m_pData->dirItems.value(mID);
    if(pNewDItem == NULL)
    {
      // create a new DirectoryItem which we put in this Directory
      pNewDItem = m_pData->newDirItem(mID);
    }

    // then we make sure the subheader is written to the Philips file.
    result = pNewDItem->writeMatrix(matrixData, type);
    if(result)
      save();
  }
  
  RETURN(result);
  return result;
}

bool CPhilipsDirectory::writeMatrix(const char* matrixData, unsigned int size,
                                    CPhilipsSubHeader::Data_Type type,
                                    short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;

  if(frame == -1) frame = minFrame();
  if(slice == -1) slice = minSlice();
  if(tilt == -1) tilt = minTilt();

  if(matrixParamsValid(slice, frame, tilt))
  {
    // form the MatrixID from the supplied data and create a new
    // DirectoryItem (or replace an existing one)
    quint32 mID = convertToMatrixID(slice, frame, tilt);
    D("Generated MatrixID: %08lx", mID);

    // see if we already have an item with the same matrixID in our
    // dictonary and if so we reuse that one and place a new subheader at
    // this position
    CPhilipsDirectoryItem* pNewDItem = m_pData->dirItems.value(mID);
    if(pNewDItem == NULL)
    {
      // create a new DirectoryItem which we put in this Directory
      pNewDItem = m_pData->newDirItem(mID);
    }

    // then we make sure the subheader is written to the Philips file.
    result = pNewDItem->writeMatrix(matrixData, size, type);
    if(result)
      save();
  }

  RETURN(result);
  return result;
}

bool CPhilipsDirectory::writeMatrix(const QByteArray& matrixData, const CPhilipsSubHeader& subHeader,
                                    short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;

  if(frame == -1) frame = minFrame();
  if(slice == -1) slice = minSlice();
  if(tilt == -1) tilt = minTilt();

  if(matrixParamsValid(slice, frame, tilt))
  {  
    // form the MatrixID from the supplied data and create a new
    // DirectoryItem (or replace an existing one)
    quint32 mID = convertToMatrixID(slice, frame, tilt);
    D("Generated MatrixID: %08lx", mID);

    // see if we already have an item with the same matrixID in our
    // dictonary and if so we reuse that one and place a new subheader at
    // this position
    CPhilipsDirectoryItem* pNewDItem = m_pData->dirItems.value(mID);
    if(pNewDItem == NULL)
    {
      // create a new DirectoryItem which we put in this Directory
      pNewDItem = m_pData->newDirItem(mID);
    }

    // then we make sure the subheader is written to the Philips file.
    result = pNewDItem->writeMatrix(matrixData, subHeader);
    if(result)
      save();
  }
  
  RETURN(result);
  return result;
}

bool CPhilipsDirectory::writeMatrix(const char* matrixData, unsigned int size, const CPhilipsSubHeader& subHeader,
                                    short frame, short slice, short tilt)
{
  ENTER();
  bool result = false;

  if(frame == -1) frame = minFrame();
  if(slice == -1) slice = minSlice();
  if(tilt == -1) tilt = minTilt();

  if(matrixParamsValid(slice, frame, tilt))
  {  
    // form the MatrixID from the supplied data and create a new
    // DirectoryItem (or replace an existing one)
    quint32 mID = convertToMatrixID(slice, frame, tilt);
    D("Generated MatrixID: %08lx", mID);

    // see if we already have an item with the same matrixID in our
    // dictonary and if so we reuse that one and place a new subheader at
    // this position
    CPhilipsDirectoryItem* pNewDItem = m_pData->dirItems.value(mID);
    if(pNewDItem == NULL)
    {
      // create a new DirectoryItem which we put in this Directory
      pNewDItem = m_pData->newDirItem(mID);
    }

    // then we make sure the subheader is written to the Philips file.
    result = pNewDItem->writeMatrix(matrixData, size, subHeader);
    if(result)
      save();
  }

  RETURN(result);
  return result;
}

CPhilipsDirectoryItem* CPhilipsDirectory::operator[](unsigned int num) const
{
  ENTER();
  CPhilipsDirectoryItem* foundItem = NULL;

  // use an QMapIterator to iterate until we got the num'th
  // element in our dictonary and return it
  QMapIterator<quint32, CPhilipsDirectoryItem*> it(m_pData->dirItems);
  for(unsigned int i=0; it.hasNext(); i++)
  {
    it.next();

    if(i == num)
    {
      foundItem = it.value();
      break;
    }
  }
  
  RETURN(foundItem);
  return foundItem;
}
