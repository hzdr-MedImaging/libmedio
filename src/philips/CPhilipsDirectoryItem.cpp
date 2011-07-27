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

#include "CPhilipsDirectoryItem.h"
#include "CPhilipsFile.h"

//#include "CPhilipsSubHeader.h"

#include <QDataStream>
#include <rtdebug.h>

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CPhilipsDirectoryItemPrivate
{
  public:
    // void cacheSubHeader(const CPhilipsSubHeader& subHeader);
    
    CPhilipsDirectoryItem* dirItem; // pointer to the directory Item
    CPhilipsFile* file;             // pointer to the PhilipsFile
    // CPhilipsSubHeader* cachedSubHeader;  // pointer to a cached SubHeader object

    // META information about the directory Item
    short frame;                // information normally
    short slice;                // encoded in
    short tilt;                 // the MatrixID
    qint64 dataBlock_Start;     // also start of SubHeader
    qint64 dataBlock_End;       // end of raw Data area

    enum CPhilipsDirectoryItem::CompressionFlag compressionFlag;
    enum CPhilipsDirectoryItem::ContentFlag contentFlag;
};

CPhilipsDirectoryItem::CPhilipsDirectoryItem(CPhilipsFile* pFile,
                                             quint32 matrixID) 
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CPhilipsDirectoryItemPrivate();
  m_pData->dirItem = this;
  m_pData->file = pFile;
  // m_pData->cachedSubHeader = NULL;
  m_pData->dataBlock_Start = 0;
  m_pData->dataBlock_End = 0;
  m_pData->compressionFlag = Uncompressed;
  m_pData->contentFlag = Unused;

  if(matrixID != 0)
  {
    m_pData->frame = matrixID2Frame(matrixID);
    m_pData->slice = matrixID2Slice(matrixID);
    m_pData->tilt  = matrixID2Tilt(matrixID);

    D("created new DirItem (%d/%d/%d)", m_pData->frame, m_pData->slice, m_pData->tilt);
  }
  else
  {
    m_pData->frame = -1;
    m_pData->slice = -1;
    m_pData->tilt = -1;
  }    
  
  LEAVE();
}

CPhilipsDirectoryItem::~CPhilipsDirectoryItem()
{
  ENTER();

  // delete m_pData->cachedSubHeader;
  delete m_pData;

  LEAVE();
}

quint32 CPhilipsDirectoryItem::matrixID() const
{ 
  return convertToMatrixID(m_pData->frame, 
                           m_pData->slice, 
                           m_pData->tilt);
}

bool CPhilipsDirectoryItem::isExtendedHeader() const
{
  return ((m_pData->contentFlag == Header) &&
          (matrixID() == PHILIPS_EXTENDED_HEADER));
}

qint64 CPhilipsDirectoryItem::dataBlock_Start() const
{ 
  return m_pData->dataBlock_Start;
}

qint64 CPhilipsDirectoryItem::dataBlock_End() const
{ 
  return m_pData->dataBlock_End;
}

CPhilipsDirectoryItem::CompressionFlag CPhilipsDirectoryItem::compressionFlag() const
{ 
  return m_pData->compressionFlag; 
}

CPhilipsDirectoryItem::ContentFlag CPhilipsDirectoryItem::contentFlag() const
{ 
  return m_pData->contentFlag; 
}

short CPhilipsDirectoryItem::frame() const
{ 
  return m_pData->frame;
}

short CPhilipsDirectoryItem::slice() const
{ 
  return m_pData->slice;
}

short CPhilipsDirectoryItem::tilt() const
{ 
  return m_pData->tilt;
}

QDataStream& operator>>(QDataStream& stream, CPhilipsDirectoryItem& dItem)
{
  ENTER();

  // first read and convert the matrixID
  quint32 matrixID;
  stream >> matrixID;
  dItem.m_pData->frame = matrixID2Frame(matrixID);
  dItem.m_pData->slice = matrixID2Slice(matrixID);
  dItem.m_pData->tilt  = matrixID2Tilt(matrixID);

  // then we read out the rest
  quint32 dataBlock_Start;
  stream >> dataBlock_Start;
  dItem.m_pData->dataBlock_Start = PhilipsBlock2FilePos(dataBlock_Start);

  quint32 dataBlock_End;
  stream >> dataBlock_End;
  dItem.m_pData->dataBlock_End = PhilipsBlock2FilePos(dataBlock_End);

  qint16 compressionFlag;
  stream >> compressionFlag;
  dItem.m_pData->compressionFlag = static_cast<CPhilipsDirectoryItem::CompressionFlag>(compressionFlag);

  qint16 contentFlag;
  stream >> contentFlag;
  dItem.m_pData->contentFlag = static_cast<CPhilipsDirectoryItem::ContentFlag>(contentFlag);

  // output some debug information.
  #if defined(DEBUG)
  if(dItem.isExtendedHeader())
  {
    D("DItem.Matrix_ID       : %08x (extended header)", matrixID);
    D("DItem.DataBlock_Start : %d (%ld)", dItem.m_pData->dataBlock_Start, FilePos2PhilipsBlock(dItem.m_pData->dataBlock_Start));
    D("DItem.DataBlock_End   : %d (%ld)", dItem.m_pData->dataBlock_End, FilePos2PhilipsBlock(dItem.m_pData->dataBlock_End));
  }
  else
  {
    D("DItem.Matrix_ID       : %08x (%d/%d/%d)", matrixID,
                                                  dItem.m_pData->frame,
                                                  dItem.m_pData->slice,
                                                  dItem.m_pData->tilt);

    D("DItem.DataBlock_Start : %d (%ld)", dItem.m_pData->dataBlock_Start, FilePos2PhilipsBlock(dItem.m_pData->dataBlock_Start));
    D("DItem.DataBlock_End   : %d (%ld)", dItem.m_pData->dataBlock_End, FilePos2PhilipsBlock(dItem.m_pData->dataBlock_End));
    D("DItem.compressionFlag : %04x", dItem.m_pData->compressionFlag);
    D("DItem.contentFlag     : %04x", dItem.m_pData->contentFlag);
  }
  #endif

  LEAVE();
  return stream;
}
