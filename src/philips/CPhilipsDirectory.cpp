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
#define PHILIPS_DIRITEM_NUM 31
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

bool CPhilipsDirectory::isEmpty() const
{ 
  return m_pData->dirItems.isEmpty();
}

unsigned int CPhilipsDirectory::count() const
{ 
  return m_pData->dirItems.count();
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
  short framesMin = 100;

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
    if(i.value()->frame() > framesNum)
    {
      framesNum = i.value()->frame();
      ASSERT(framesNum > 0 && framesNum <= 100);
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
  short slicesMin = 4095;

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
  
  RETURN(slicesMin);
  return slicesMin;
}

short CPhilipsDirectory::numSlices(void) const
{
  short slicesNum = 0;

  // we iterate through our dictionary looking for the highest available
  // slice number
  QMapIterator<quint32, CPhilipsDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();
    if(i.value()->slice() > slicesNum)
    {
      slicesNum = i.value()->slice();
      ASSERT(slicesNum > 0 && slicesNum <= 256);
    }
  }
  
  return slicesNum;
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

bool CPhilipsDirectory::readExtendedMainHeader(CPhilipsExtendedMainHeader*& extendedMainHeader)
{
  ENTER();
  bool result = false;
  
  // we iterate through our dictionary looking for an extendedMainHeader
  QMapIterator<quint32, CPhilipsDirectoryItem*> i(m_pData->dirItems);
  while(i.hasNext())
  {
    i.next();
    if(i.value()->isExtendedHeader() == true)
    {
      // get the directoryItem so that we can query the extendedMainHeaader from it
      CPhilipsDirectoryItem* pDirItem = i.value();
      SHOWVALUE(pDirItem);
      if(pDirItem)
        result = pDirItem->readExtendedMainHeader(extendedMainHeader);

      break;
    }
  }
  
  RETURN(result);
  return result;
}

bool CPhilipsDirectory::readSubHeader(CPhilipsSubHeader*& subHeader, short slice, short frame,
                                      short tilt)
{
  ENTER();
  bool result = false;
  D("IN directory readSubHeader");
  
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
                                   short slice, short frame, short tilt)
{
  ENTER();
  bool result = false;

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
                                   short slice, short frame, short tilt)
{
  ENTER();
  bool result = false;

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
                                   short slice, short frame, short tilt)
{
  ENTER();
  bool result = false;

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
                                   short slice, short frame, short tilt)
{
  ENTER();
  bool result = false;

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
