/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio
 
  Copyright (C) 2004-2026 hzdr.de and contributors
 
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
    http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "CECATDirectory.h"
#include "CECATDirectoryItem.h"
#include "CECATFile.h"

#include <QMap>
#include <QVector>

#include <rtdebug.h>

#include "bswap.h"

// some very own defines specifying the boundaries
// of a ecat directory list
#define ECAT_DIRHEAD_SIZE 16
#define ECAT_DIRITEM_SIZE 16
#define ECAT_DIRITEM_NUM  31
#define ECAT_DIRLIST_SIZE (ECAT_DIRHEAD_SIZE+ECAT_DIRITEM_NUM*ECAT_DIRITEM_SIZE)

// the RAW structures used in the ECAT directory blocks
#pragma pack(1)
struct ECAT_DirHead // should be 16 bytes
{
  quint32 FreeItems;
  quint32 Next;
  quint32 Prev;
  quint32 ItemsToFollow;
};

struct ECAT_DirItem // should be 16 bytes
{
  quint32 matrixID;         // frame/plane/gate/bed/data encoded matrixID
  quint32 dataBlock_Start;  // start position of the subHeader
  quint32 dataBlock_End;    // end position of the last dataBlock
  qint32 matrixStatus;      // status of the item (0=notWrittenYet,1=OK,-1=deleted)
};

struct ECAT_DirList // should be 512 bytes
{
  struct ECAT_DirHead head;
  struct ECAT_DirItem items[ECAT_DIRITEM_NUM];
};
#pragma pack()

// defines for checking if parameters are in bounds
#define MIN_FRAME   0
#define MAX_FRAME   512
#define MIN_PLANE   1
#define MAX_PLANE   1024
#define MIN_GATE    1
#define MAX_GATE    64
#define MIN_BEDPOS  0
#define MAX_BEDPOS  16
#define MIN_DATA    0
#define MAX_DATA    8

// macro for quickly checking if the frame/plane/gate/bed/data
// matrix parameters are between bounds or not
#define matrixParamsValid(f, p, g, b, d) ((f) >= MIN_FRAME  && (f) < MAX_FRAME  && \
                                          (p) >= MIN_PLANE  && (p) < MAX_PLANE  && \
                                          (g) >= MIN_GATE   && (g) < MAX_GATE   && \
                                          (b) >= MIN_BEDPOS && (b) < MAX_BEDPOS && \
                                          (d) >= MIN_DATA   && (d) < MAX_DATA)

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CECATDirectoryPrivate
{
  public:
    CECATFile* file; // ptr to our associated ECATFile
    QMap<quint32, CECATDirectoryItem*> dirItems; // value map of dirItems
    QVector<qint64> filePositions; // for each DirList we
                                   // may have different file
                                   // positions which we have
                                   // to store

    // some private methods 
    CECATDirectoryItem* newDirItem(quint32 matrixID);
    qint64 lastDirItemOffset(void) const;
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
  QMapIterator<quint32, CECATDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();
    delete i.value();
  }

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
  if(m_pData->file->seek(ECATBlock2FilePos(ECAT_POS_MAINDIR)) == false)
  {
    RETURN(false);
    return false;
  }

  // check that we seeked correctly
  ASSERT(m_pData->file->pos() == ECATBlock2FilePos(ECAT_POS_MAINDIR));

  // we process the splitted directory list in a do..while loop
  // and jump from one dirlist to another in the ecat file.
  struct ECAT_DirList dList;
  do
  {
    // add each DirList position to our filePositions value list
    // for later reference
    SHOWVALUE(m_pData->file->pos());
    m_pData->filePositions.append(m_pData->file->pos());

    // read in the whole dirlist at once
    ASSERT(sizeof(dList) == ECAT_DIRLIST_SIZE);
    if(m_pData->file->read(reinterpret_cast<char*>(&dList), sizeof(dList)) != ECAT_DIRLIST_SIZE) 
    {
      E("An error occurred while reading data");

      result = false;
      break;
    }

    // now we swap the data fields of the dirlist header in case this is
    // not a big endian machine
    if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
    {
      BSWAP_32(dList.head.FreeItems);
      BSWAP_32(dList.head.Next);
      BSWAP_32(dList.head.Prev);
      BSWAP_32(dList.head.ItemsToFollow);
    }

    // output some debug information on the head of the dirList.
    D("DirHead.FreeItems   : %d", dList.head.FreeItems);
    D("DirHead.Next        : %d", dList.head.Next);
    D("DirHead.Prev        : %d", dList.head.Prev);
    D("DirHead.ItemToFollow: %d", dList.head.ItemsToFollow);

    // now we know how many item will follow in the Directory, so we
    // can populate our Directory here.
    quint32 iItemsInserted = 0;
    for(quint32 i=0; i < dList.head.ItemsToFollow; i++)
    {
      CECATDirectoryItem* pNewDirItem = new CECATDirectoryItem(m_pData->file);

      // lets byteswap the data of thir dir item before
      // assigning it to the dir item object
      if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
      {
        BSWAP_32(dList.items[i].matrixID);
        BSWAP_32(dList.items[i].dataBlock_Start);
        BSWAP_32(dList.items[i].dataBlock_End);
        BSWAP_32(dList.items[i].matrixStatus);
      } 

      // set all attributes of the directory item
      pNewDirItem->setMatrixID(dList.items[i].matrixID);
      pNewDirItem->setDataBlock_Start(ECATBlock2FilePos(dList.items[i].dataBlock_Start));
      pNewDirItem->setDataBlock_End(ECATBlock2FilePos(dList.items[i].dataBlock_End));
      pNewDirItem->setDataBlock_Status(static_cast<CECATDirectoryItem::AccessStatus>(dList.items[i].matrixStatus));

      // output some debug information.
      D("DItem.Matrix_ID       : %08x (%d/%d/%d/%d/%d)", pNewDirItem->matrixID(),
                                                         pNewDirItem->frame(),
                                                         pNewDirItem->plane(),
                                                         pNewDirItem->gate(),
                                                         pNewDirItem->bed(),
                                                         pNewDirItem->data());

      D("DItem.DataBlock_Start : %ld (%ld)", pNewDirItem->dataBlock_Start(), FilePos2ECATBlock(pNewDirItem->dataBlock_Start()));
      D("DItem.DataBlock_End   : %ld (%ld)", pNewDirItem->dataBlock_End(), FilePos2ECATBlock(pNewDirItem->dataBlock_End()));
      D("DItem.DataBlock_Status: %d", pNewDirItem->dataBlock_Status());

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
    if(dList.head.ItemsToFollow != iItemsInserted)
      W("ItemsToFollow:%ld != iItemsInserted:%ld", dList.head.ItemsToFollow, iItemsInserted);
    #endif

    if(m_pData->file->seek(ECATBlock2FilePos(dList.head.Next)) == false)
    {
      E("couldn't seek to file position %lld.", ECATBlock2FilePos(dList.head.Next));
      result = false;
      break;
    }

    // check that we seeked correctly
    ASSERT(m_pData->file->pos() == ECATBlock2FilePos(dList.head.Next));
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
  if(m_pData->file == NULL ||
     m_pData->file->isWritable() == false)
  {
    RETURN(false);
    return false;
  }
  
  // we need to populate the dirList a bit ordered
  // frame->plane->gate->bed->data
  // even if the ECAT standard doesn't define a ordering, but some
  // stupid software seems to require it.
  struct ECAT_DirList dirList;

  // before we start we have to link the first dirList to itself
  memset(&dirList, 0, sizeof(dirList)); // clear it first
  dirList.head.FreeItems = ECAT_DIRITEM_NUM;
  dirList.head.Next      = ECAT_POS_MAINDIR;

  // now we have to go through our directory and stream all items
  // in 31 chunks as the directory list can only have 31 items plus
  // 1 chunk for the header
  quint32 currentDirPos = ECAT_POS_MAINDIR;
  quint32 processedDirItems = 0;
  quint16 curDirList = 0;
  quint32 nextDirPos = currentDirPos;
  qint64 lastDirListPos = 0;

  // now iterate through our sorted QMap and write out the
  // data sorted as well.
  QMapIterator<quint32, CECATDirectoryItem*> it(m_pData->dirItems);
  while(it.hasNext())
  {
    it.next();
    
    CECATDirectoryItem* pDirItem = it.value();
    if(pDirItem)
    {
      quint32 i = dirList.head.ItemsToFollow;
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
      
      // populate the diritem
      dirList.items[i].matrixID = pDirItem->matrixID();
      dirList.items[i].dataBlock_Start = FilePos2ECATBlock(pDirItem->dataBlock_Start());
      dirList.items[i].dataBlock_End = FilePos2ECATBlock(pDirItem->dataBlock_End());
      dirList.items[i].matrixStatus = static_cast<qint32>(pDirItem->dataBlock_Status());

      D("DItem.Matrix_ID       : %08x (%d/%d/%d/%d/%d)", pDirItem->matrixID(),
                                                         pDirItem->frame(),
                                                         pDirItem->plane(),
                                                         pDirItem->gate(),
                                                         pDirItem->bed(),
                                                         pDirItem->data());

      D("DItem.DataBlock_Start : %lld (%lld)", FilePos2ECATBlock(pDirItem->dataBlock_Start()), pDirItem->dataBlock_Start());
      D("DItem.DataBlock_End   : %lld (%lld)", FilePos2ECATBlock(pDirItem->dataBlock_End()), pDirItem->dataBlock_End());
      D("DItem.DataBlock_Status: %d", pDirItem->dataBlock_Status());

      // now we need to byteswap the data if this is no big endian machine
      if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
      {
        BSWAP_32(dirList.items[i].matrixID);
        BSWAP_32(dirList.items[i].dataBlock_Start);
        BSWAP_32(dirList.items[i].dataBlock_End);
        BSWAP_32(dirList.items[i].matrixStatus);
      }

      // increment the ItemsToFollow value and decrement the 
      // freeItems
      dirList.head.ItemsToFollow++;
      dirList.head.FreeItems--;
      
      // first we check wheter the dirList is filled up (>=31 items)
      // and then write it out separate first.
      if((dirList.head.ItemsToFollow == ECAT_DIRITEM_NUM && processedDirItems < count()) ||
         it.hasNext() == false)
      {
        // check if were are here because there is no
        // futher directory item to be added
        if(it.hasNext() == true)
        {
          qint64 appendPos;

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
          dirList.head.Next = nextDirPos;
        }
        else
          dirList.head.Next = ECAT_POS_MAINDIR;

        D("DirHead%d.Position     : %d (%d)", curDirList, FilePos2ECATBlock(m_pData->file->pos()), m_pData->filePositions[curDirList]);
        D("DirHead%d.FreeItems    : %d", curDirList, dirList.head.FreeItems);
        D("DirHead%d.Next         : %d", curDirList, dirList.head.Next);
        D("DirHead%d.Prev         : %d", curDirList, dirList.head.Prev);
        D("DirHead%d.ItemsToFollow: %d", curDirList, dirList.head.ItemsToFollow);

        // lets byteswap the directory header as we have finished it now
        if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
        {
          BSWAP_32(dirList.head.FreeItems);
          BSWAP_32(dirList.head.Next);
          BSWAP_32(dirList.head.Prev);
          BSWAP_32(dirList.head.ItemsToFollow);
        }
        
        // now we can seek to the file position of the DirList
        // in the file.
        if(m_pData->file->seek(m_pData->filePositions[curDirList]) == false)
        {
          E("Error while seeking to DirList position: %lld", m_pData->filePositions[curDirList]);
          result = false;
          break;
        }
          
        // check that we seeked correctly
        ASSERT(m_pData->file->pos() == m_pData->filePositions[curDirList]);

        // write out the whole dirlist in one write() operation
        ASSERT(sizeof(dirList) == ECAT_DIRLIST_SIZE);
        if(m_pData->file->write(reinterpret_cast<char*>(&dirList), sizeof(dirList)) != ECAT_DIRLIST_SIZE)
        {
          E("Error while writing DirList");
          result = false;
          break;
        }
        else
          D("DirList #%d successfully written @ %lld (%lld)", curDirList, 
                                                              FilePos2ECATBlock(m_pData->file->pos()-ECAT_DIRLIST_SIZE),
                                                              m_pData->file->pos()-ECAT_DIRLIST_SIZE);

        // clear the dirList so that we can immediately reuse it
        memset(&dirList, 0, sizeof(dirList)); // clear it first
        dirList.head.FreeItems     = ECAT_DIRITEM_NUM;
        dirList.head.Next          = ECAT_POS_MAINDIR;
        dirList.head.Prev          = currentDirPos;
        dirList.head.ItemsToFollow = 0;

        // lets point the currentDirPos to the nextDirPos now that
        // we added a new dirList
        currentDirPos = nextDirPos;

        // iterate to the next dirList
        curDirList++;
      }
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

  if(frame == -1) frame = minFrame();
  if(plane == -1) plane = minPlane();
  if(gate == -1)  gate = minGate();
  if(bed == -1)   bed = minBedPos();
  if(data == -1)  data = minData();

  if(matrixParamsValid(frame, plane, gate, bed, data))
  {
    // form the MatrixID from the supplied data and
    // process a find in the QIntDict
    quint32 mID = convertToMatrixID(frame, plane, gate, bed, data);
    D("Generated MatrixID: %08x", mID);

    dirItem = m_pData->dirItems.value(mID);
  }

  RETURN(dirItem);
  return dirItem;
}

CECATDirectoryItem* CECATDirectoryPrivate::newDirItem(quint32 matrixID)
{
  ENTER();

  // create a new DirectoryItem which we put in this Directory
  CECATDirectoryItem* pNewDItem = new CECATDirectoryItem(file, matrixID);

  // now that we generated a new directory item we want to put
  // in our directory immediately, we have to first check at which
  // dataposition it should be placed
  qint64 dataOffset = lastDirItemOffset();

  D("num dir lists available: %d", filePositions.count());
  D("num items in directory: %d", dirItems.count());
  
  // now we check wheter we have enough directory items available for the
  // amount of dirItems we want to store and if not we add an additional
  // file position for locking this one.
  if((static_cast<float>(dirItems.count()+1) / ECAT_DIRITEM_NUM) > filePositions.count())
  {
    dataOffset += ECAT_BLOCKSIZE;

    // append the position to our FilePositions for DirLists
    filePositions.append(dataOffset);
          
    D("appended new DirList #%d @ %lld (%lld)", filePositions.count(), FilePos2ECATBlock(dataOffset), dataOffset);
  }

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

qint64 CECATDirectoryPrivate::lastDirItemOffset(void) const
{
  ENTER();
  qint64 offset = 0;

  // we search through our item list and check the for the
  // very last item position
  QMapIterator<quint32, CECATDirectoryItem*> i(dirItems);
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

short CECATDirectory::maxFrame() const
{
  ENTER();
  short framesMax = 0;

  // we iterate through our dictionary looking for the highest available
  // frame number
  QMapIterator<quint32, CECATDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();

    if(i.value()->frame() > framesMax)
    {
      framesMax = i.value()->frame();
      ASSERT(framesMax >= MIN_FRAME && framesMax < MAX_FRAME);
    }
  }

  RETURN(framesMax);
  return framesMax;
}

short CECATDirectory::minFrame() const
{
  ENTER();
  short framesMin = SHRT_MAX;

  // we iterate through our dictionary looking for the highest available
  // frame number
  QMapIterator<quint32, CECATDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();

    if(i.value()->frame() < framesMin)
    {
      framesMin = i.value()->frame();
      ASSERT(framesMin >= MIN_FRAME && framesMin < MAX_FRAME);
    }
  }

  // if framesMin is still SHRT_MAX there are no frames in the map
  // in this case we have to set the min value to 0
  if(framesMin == SHRT_MAX)
    framesMin = MIN_FRAME;

  RETURN(framesMin);
  return framesMin;
}

short CECATDirectory::numFrames(void) const
{
  ENTER();
  short framesNum = 0;

  // we use maxFrame() as the max frame number defines
  // more or less how many frames the file has
  framesNum = maxFrame();
  
  RETURN(framesNum);
  return framesNum;
}

short CECATDirectory::maxPlane() const
{
  ENTER();
  short planeMax = 0;

  // we iterate through our dictionary looking for the highest available
  // plane number
  QMapIterator<quint32, CECATDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();

    if(i.value()->plane() > planeMax)
    { 
      planeMax = i.value()->plane();
      ASSERT(planeMax >= MIN_PLANE && planeMax < MAX_PLANE);
    } 
  } 
  
  RETURN(planeMax);
  return planeMax;
} 

short CECATDirectory::minPlane() const
{
  ENTER();
  short planeMin = SHRT_MAX;

  // we iterate through our dictionary looking for the lowest available
  // plane number
  QMapIterator<quint32, CECATDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();

    if(i.value()->plane() < planeMin)
    { 
      planeMin = i.value()->plane();
      ASSERT(planeMin >= MIN_PLANE && planeMin < MAX_PLANE);
    } 
  } 
 
  // if planeMin is still SHRT_MAX there are no planes in the map
  // in this case we have to set the min value to 0
  if(planeMin == SHRT_MAX)
    planeMin = MIN_PLANE;
 
  RETURN(planeMin);
  return planeMin;
}

short CECATDirectory::numPlanes(void) const
{
  ENTER();
  short planesNum = 0;

  // we use maxPlane() as the max plane number defines
  // more or less how many frames the file has
  planesNum = maxPlane();

  RETURN(planesNum);
  return planesNum;
}

short CECATDirectory::maxGate() const
{
  ENTER();
  short gateMax = 0;

  // we iterate through our dictionary looking for the highest available
  // gate number
  QMapIterator<quint32, CECATDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();

    if(i.value()->gate() > gateMax)
    {
      gateMax = i.value()->gate();
      ASSERT(gateMax >= MIN_GATE && gateMax < MAX_GATE);
    }
  }

  RETURN(gateMax);
  return gateMax;
}

short CECATDirectory::minGate() const
{
  ENTER();
  short gateMin = SHRT_MAX;

  // we iterate through our dictionary looking for the lowest available
  // gate number
  QMapIterator<quint32, CECATDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();

    if(i.value()->gate() > gateMin)
    {
      gateMin = i.value()->gate();
      ASSERT(gateMin >= MIN_GATE && gateMin < MAX_GATE);
    }
  }

  // if gateMin is still SHRT_MAX there are no gates in the map
  // in this case we have to set the min value to 0
  if(gateMin == SHRT_MAX)
    gateMin = MIN_GATE;

  RETURN(gateMin);
  return gateMin;
}

short CECATDirectory::numGates(void) const
{
  ENTER();
  short gatesNum = 0;

  // we use maxGate() as the max plane number defines
  // more or less how many frames the file has
  gatesNum = maxGate();

  RETURN(gatesNum);
  return gatesNum;
}  

short CECATDirectory::maxBedPos() const
{
  ENTER();
  short bedPosMax = 0;

  // we iterate through our dictionary looking for the highest available
  // bedpos number
  QMapIterator<quint32, CECATDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();

    if(i.value()->bed() > bedPosMax)
    {
      bedPosMax = i.value()->bed();
      ASSERT(bedPosMax >= MIN_BEDPOS && bedPosMax < MAX_BEDPOS);
    }
  }

  RETURN(bedPosMax);
  return bedPosMax;
}

short CECATDirectory::minBedPos() const
{
  ENTER();
  short bedPosMin = SHRT_MAX;

  // we iterate through our dictionary looking for the lowest available
  // gate number
  QMapIterator<quint32, CECATDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();

    if(i.value()->bed() > bedPosMin)
    {
      bedPosMin = i.value()->bed();
      ASSERT(bedPosMin >= MIN_BEDPOS && bedPosMin < MAX_BEDPOS);
    }
  }

  // if bedPosMin is still SHRT_MAX there are no bedpos in the map
  // in this case we have to set the min value to 0
  if(bedPosMin == SHRT_MAX)
    bedPosMin = MIN_BEDPOS;

  RETURN(bedPosMin);
  return bedPosMin;
}

short CECATDirectory::numBedPos(void) const
{
  ENTER();
  short bedsNum = 0;

  // we use maxBedPos() as the max bedpos number defines
  // more or less how many bedpos the file has
  bedsNum = maxBedPos();

  RETURN(bedsNum);
  return bedsNum;
}  

short CECATDirectory::maxData() const
{
  ENTER();
  short dataMax = 0;

  // we iterate through our dictionary looking for the highest available
  // bedpos number
  QMapIterator<quint32, CECATDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();

    if(i.value()->data() > dataMax)
    {
      dataMax = i.value()->data();
      ASSERT(dataMax >= MIN_DATA && dataMax < MAX_DATA);
    }
  }

  RETURN(dataMax);
  return dataMax;
}

short CECATDirectory::minData() const
{
  ENTER();
  short dataMin = SHRT_MAX;

  // we iterate through our dictionary looking for the lowest available
  // gate number
  QMapIterator<quint32, CECATDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();

    if(i.value()->data() > dataMin)
    {
      dataMin = i.value()->data();
      ASSERT(dataMin >= MIN_DATA && dataMin < MAX_DATA);
    }
  }

  // if dataMin is still SHRT_MAX there are no data entries in the map
  // in this case we have to set the min value to 0
  if(dataMin == SHRT_MAX)
    dataMin = MIN_DATA;

  RETURN(dataMin);
  return dataMin;
}

short CECATDirectory::numData(void) const
{
  ENTER();
  short dataNum = 0;

  // we use maxData() as the max data number defines
  // more or less how many data entries the file has
  dataNum = maxData();

  RETURN(dataNum);
  return dataNum;
}

bool CECATDirectory::readSubHeader(CECATSubHeader*& subHeader, short frame, short plane,
                                   short gate, short bed, short data)
{
  ENTER();
  bool result = false;

  if(frame == -1) frame = minFrame();
  if(plane == -1) plane = minPlane();
  if(gate == -1)  gate = minGate();
  if(bed == -1)   bed = minBedPos();
  if(data == -1)  data = minData();

  if(matrixParamsValid(frame, plane, gate, bed, data))
  {
    // get the directoryItem so that we can query the matrix from it
    CECATDirectoryItem* pDirItem = m_pData->dirItems.value(convertToMatrixID(frame, plane, gate, bed, data));

    if(pDirItem)
      result = pDirItem->readSubHeader(subHeader);
  }
  
  RETURN(result);
  return result;
}

bool CECATDirectory::readMatrix(QByteArray*& matrixData, short frame, short plane,
                               short gate, short bed, short data)
{
  ENTER();
  bool result = false;

  if(frame == -1) frame = minFrame();
  if(plane == -1) plane = minPlane();
  if(gate == -1)  gate = minGate();
  if(bed == -1)   bed = minBedPos();
  if(data == -1)  data = minData();

  if(matrixParamsValid(frame, plane, gate, bed, data))
  {
    // get the directoryItem so that we can query the matrix from it
    CECATDirectoryItem* pDirItem = m_pData->dirItems.value(convertToMatrixID(frame, plane, gate, bed, data));

    if(pDirItem)
      result = pDirItem->readMatrix(matrixData);
  }

  RETURN(result);
  return result;
}

bool CECATDirectory::readMatrix(char*& matrixData, unsigned int& len, short frame, short plane,
                                short gate, short bed, short data)
{
  ENTER();
  bool result = false;

  if(frame == -1) frame = minFrame();
  if(plane == -1) plane = minPlane();
  if(gate == -1)  gate = minGate();
  if(bed == -1)   bed = minBedPos();
  if(data == -1)  data = minData();

  if(matrixParamsValid(frame, plane, gate, bed, data))
  {
    // get the directoryItem so that we can query the matrix from it
    CECATDirectoryItem* pDirItem = m_pData->dirItems.value(convertToMatrixID(frame, plane, gate, bed, data));

    if(pDirItem)
      result = pDirItem->readMatrix(matrixData, len);
  }
  
  RETURN(result);
  return result;
}

bool CECATDirectory::readMatrix(QByteArray*& matrixData, CECATSubHeader*& subHeader,
                                short frame, short plane, short gate, short bed, short data)
{
  ENTER();
  bool result = false;

  if(frame == -1) frame = minFrame();
  if(plane == -1) plane = minPlane();
  if(gate == -1)  gate = minGate();
  if(bed == -1)   bed = minBedPos();
  if(data == -1)  data = minData();

  if(matrixParamsValid(frame, plane, gate, bed, data))
  {
    // get the directoryItem so that we can query the matrix from it
    CECATDirectoryItem* pDirItem = m_pData->dirItems.value(convertToMatrixID(frame, plane, gate, bed, data));

    if(pDirItem)
      result = pDirItem->readMatrix(matrixData, subHeader);
  }
  
  RETURN(result);
  return result;
}

bool CECATDirectory::readMatrix(char*& matrixData, unsigned int& len, CECATSubHeader*& subHeader,
                                short frame, short plane, short gate, short bed, short data)
{
  ENTER();
  bool result = false;

  if(frame == -1) frame = minFrame();
  if(plane == -1) plane = minPlane();
  if(gate == -1)  gate = minGate();
  if(bed == -1)   bed = minBedPos();
  if(data == -1)  data = minData();

  if(matrixParamsValid(frame, plane, gate, bed, data))
  {
    // get the directoryItem so that we can query the matrix from it
    CECATDirectoryItem* pDirItem = m_pData->dirItems.value(convertToMatrixID(frame, plane, gate, bed, data));

    if(pDirItem)
      result = pDirItem->readMatrix(matrixData, len, subHeader);
  }
  
  RETURN(result);
  return result;
}

bool CECATDirectory::writeSubHeader(const CECATSubHeader& subHeader, short frame,
                                    short plane, short gate, short bed, short data)
{
  ENTER();
  bool result = false;

  if(frame == -1) frame = minFrame();
  if(plane == -1) plane = minPlane();
  if(gate == -1)  gate = minGate();
  if(bed == -1)   bed = minBedPos();
  if(data == -1)  data = minData();

  if(matrixParamsValid(frame, plane, gate, bed, data))
  {  
    // form the MatrixID from the supplied data and create a new
    // DirectoryItem (or replace an existing one)
    quint32 mID = convertToMatrixID(frame, plane, gate, bed, data);
    D("Generated MatrixID: %08x", mID);

    // see if we already have an item with the same matrixID in our
    // dictonary and if so we reuse that one and place a new subheader at
    // this position
    CECATDirectoryItem* pNewDItem = m_pData->dirItems.value(mID);
    if(pNewDItem == NULL)
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

  if(frame == -1) frame = minFrame();
  if(plane == -1) plane = minPlane();
  if(gate == -1)  gate = minGate();
  if(bed == -1)   bed = minBedPos();
  if(data == -1)  data = minData();

  if(matrixParamsValid(frame, plane, gate, bed, data))
  {
    // form the MatrixID from the supplied data and create a new
    // DirectoryItem (or replace an existing one)
    quint32 mID = convertToMatrixID(frame, plane, gate, bed, data);
    D("Generated MatrixID: %08x", mID);

    // see if we already have an item with the same matrixID in our
    // dictonary and if so we reuse that one and place a new subheader at
    // this position
    CECATDirectoryItem* pNewDItem = m_pData->dirItems.value(mID);
    if(pNewDItem == NULL)
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

  if(frame == -1) frame = minFrame();
  if(plane == -1) plane = minPlane();
  if(gate == -1)  gate = minGate();
  if(bed == -1)   bed = minBedPos();
  if(data == -1)  data = minData();

  if(matrixParamsValid(frame, plane, gate, bed, data))
  {  
    // form the MatrixID from the supplied data and create a new
    // DirectoryItem (or replace an existing one)
    quint32 mID = convertToMatrixID(frame, plane, gate, bed, data);
    D("Generated MatrixID: %08x", mID);

    // see if we already have an item with the same matrixID in our
    // dictonary and if so we reuse that one and place a new subheader at
    // this position
    CECATDirectoryItem* pNewDItem = m_pData->dirItems.value(mID);
    if(pNewDItem == NULL)
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

  if(frame == -1) frame = minFrame();
  if(plane == -1) plane = minPlane();
  if(gate == -1)  gate = minGate();
  if(bed == -1)   bed = minBedPos();
  if(data == -1)  data = minData();

  if(matrixParamsValid(frame, plane, gate, bed, data))
  {
    // form the MatrixID from the supplied data and create a new
    // DirectoryItem (or replace an existing one)
    quint32 mID = convertToMatrixID(frame, plane, gate, bed, data);
    D("Generated MatrixID: %08x", mID);

    // see if we already have an item with the same matrixID in our
    // dictonary and if so we reuse that one and place a new subheader at
    // this position
    CECATDirectoryItem* pNewDItem = m_pData->dirItems.value(mID);
    if(pNewDItem == NULL)
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

  if(frame == -1) frame = minFrame();
  if(plane == -1) plane = minPlane();
  if(gate == -1)  gate = minGate();
  if(bed == -1)   bed = minBedPos();
  if(data == -1)  data = minData();

  if(matrixParamsValid(frame, plane, gate, bed, data))
  {
    // form the MatrixID from the supplied data and create a new
    // DirectoryItem (or replace an existing one)
    quint32 mID = convertToMatrixID(frame, plane, gate, bed, data);
    D("Generated MatrixID: %08x", mID);

    // see if we already have an item with the same matrixID in our
    // dictonary and if so we reuse that one and place a new subheader at
    // this position
    CECATDirectoryItem* pNewDItem = m_pData->dirItems.value(mID);
    if(pNewDItem == NULL)
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

  if(frame == -1) frame = minFrame();
  if(plane == -1) plane = minPlane();
  if(gate == -1)  gate = minGate();
  if(bed == -1)   bed = minBedPos();
  if(data == -1)  data = minData();

  if(matrixParamsValid(frame, plane, gate, bed, data))
  {  
    // form the MatrixID from the supplied data and create a new
    // DirectoryItem (or replace an existing one)
    quint32 mID = convertToMatrixID(frame, plane, gate, bed, data);
    D("Generated MatrixID: %08x", mID);

    // see if we already have an item with the same matrixID in our
    // dictonary and if so we reuse that one and place a new subheader at
    // this position
    CECATDirectoryItem* pNewDItem = m_pData->dirItems.value(mID);
    if(pNewDItem == NULL)
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

  if(frame == -1) frame = minFrame();
  if(plane == -1) plane = minPlane();
  if(gate == -1)  gate = minGate();
  if(bed == -1)   bed = minBedPos();
  if(data == -1)  data = minData();

  if(matrixParamsValid(frame, plane, gate, bed, data))
  {  
    // form the MatrixID from the supplied data and create a new
    // DirectoryItem (or replace an existing one)
    quint32 mID = convertToMatrixID(frame, plane, gate, bed, data);
    D("Generated MatrixID: %08x", mID);

    // see if we already have an item with the same matrixID in our
    // dictonary and if so we reuse that one and place a new subheader at
    // this position
    CECATDirectoryItem* pNewDItem = m_pData->dirItems.value(mID);
    if(pNewDItem == NULL)
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
  CECATDirectoryItem* foundItem = NULL;

  // use an QMapIterator to iterate until we got the num'th
  // element in our dictonary and return it
  QMapIterator<quint32, CECATDirectoryItem*> it(m_pData->dirItems);
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
