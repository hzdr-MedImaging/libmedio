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

#include <QString>

#include "CPhilipsListviewHeader.h"
#include "CPhilipsFile.h"

#include <rtdebug.h>
#include "bswap.h"

class CPhilipsListviewHeaderPrivate
{
  public:
    CPhilipsListviewHeaderPrivate(CPhilipsFile *file)
      : listModeFile(file)
    {}

    #define LISTVIEWHEADER_START 0
    #define LISTVIEWHEADER_SIZE 512
    #pragma pack(1)
    struct HeaderData
    {
        qint32 numberOfEventsPerBuffer; // 000: number of events per buffer
        char magicNumber[4];            // 004: magic number
                                        //       "LOR4" for compact acquisition format
                                        //       "LOR8" for research acquisition foramt
                                        //       "Xtal" for research crystal acquisition format
        qint32 fileFormat;              // 008: type of listview file format
        qint8 singlesMode;              // 012: singles mode
        qint8 timestampsStoredInAUX;    // 013: flag to indicate if timestamps are stored in the AUX data
        qint16 mainHeaderSize;          // 014: main header size
        qint16 subHeaderSize;           // 016: sub header size
        char comment[494];              // 018: comment
    } header;
    #pragma pack()

    CPhilipsFile* listModeFile;
};

CPhilipsListviewHeader::CPhilipsListviewHeader(CPhilipsFile *file)
{
  ENTER();
  m_pData = new CPhilipsListviewHeaderPrivate(file);
  LEAVE();
}

CPhilipsListviewHeader::~CPhilipsListviewHeader()
{
  ENTER();
  delete m_pData;
  LEAVE();
}

void CPhilipsListviewHeader::clear()
{
  ENTER();
  // clear the listview header stucture
  memset(&m_pData->header, 0, sizeof(CPhilipsListviewHeaderPrivate::HeaderData));
  LEAVE();
}

bool CPhilipsListviewHeader::load()
{
  ENTER();

  // only go on if the device is readable at all
  if(m_pData->listModeFile == NULL ||
     m_pData->listModeFile->isReadable() == false ||
     m_pData->listModeFile->seek(LISTVIEWHEADER_START) == false)
  {
    RETURN(false);
    return false;
  }

  // read the listview header in one read
  ASSERT(sizeof(m_pData->header) == LISTVIEWHEADER_SIZE);
  if(m_pData->listModeFile->read(reinterpret_cast<char*>(&m_pData->header), sizeof(m_pData->header)) != LISTVIEWHEADER_SIZE)
  {
    RETURN(false);
    return false;
  }

  // now that we have streamed in all data in one run we
  // have to take care of correct endianness in the non-char
  // entries in the header structure in case this is a little endian
  // machine
  if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
  {
    BSWAP_32(m_pData->header.numberOfEventsPerBuffer);
    BSWAP_32(m_pData->header.fileFormat);
    BSWAP_16(m_pData->header.mainHeaderSize);
    BSWAP_16(m_pData->header.subHeaderSize);
  }

#if defined(DEBUG)
  D("philips listview header:");
  D("-----------------------");
  D("numberOfEventsPerBuffer: %d", numberOfEventsPerBuffer());
  D("magicNumber            : %s", qPrintable(magicNumber()));
  D("fileFormat             : %d", listModeFormat());
  D("singlesMode            : %d", singlesMode());
  D("timestampsStoredInAUX  : %d", timestampsStoredInAUX());
  D("mainHeaderSize         : %d", mainHeaderSize());
  D("subHeaderSize          : %d", subHeaderSize());
  D("comment                : %s", qPrintable(comment()));
#endif
  RETURN(true);
  return true;
}

bool CPhilipsListviewHeader::save() const
{
  ENTER();
  // nothing yet
  RETURN(false);
  return false;
}

int CPhilipsListviewHeader::numberOfEventsPerBuffer() const
{
  return m_pData->header.numberOfEventsPerBuffer;
}

QString CPhilipsListviewHeader::magicNumber() const
{
  return QString::fromLatin1(m_pData->header.magicNumber, sizeof(m_pData->header.magicNumber));
}

CPhilipsListviewHeader::File_Format CPhilipsListviewHeader::listModeFormat() const
{
  return static_cast<File_Format>(m_pData->header.fileFormat);
}

bool CPhilipsListviewHeader::singlesMode() const
{
  return (m_pData->header.singlesMode == 1);
}

bool CPhilipsListviewHeader::timestampsStoredInAUX() const
{
  return (m_pData->header.timestampsStoredInAUX == 1);
}

short CPhilipsListviewHeader::mainHeaderSize() const
{
  return m_pData->header.mainHeaderSize;
}

short CPhilipsListviewHeader::subHeaderSize() const
{
  return m_pData->header.subHeaderSize;
}

QString CPhilipsListviewHeader::comment() const
{
  return QString::fromLatin1(m_pData->header.comment, sizeof(m_pData->header.comment));
}

void CPhilipsListviewHeader::setNumberOfEventsPerBuffer(const int n)
{
  m_pData->header.numberOfEventsPerBuffer = n;
}

void CPhilipsListviewHeader::setMagicNumber(const QString &magicNumber)
{
  strncpy(m_pData->header.magicNumber, magicNumber.toLatin1().constData(), sizeof(m_pData->header.magicNumber)-1);
}

void CPhilipsListviewHeader::setFileFormat(const CPhilipsListviewHeader::File_Format format)
{
  m_pData->header.fileFormat = format;
}

void CPhilipsListviewHeader::setSinglesMode(const bool flag)
{
  m_pData->header.singlesMode = flag ? 1 : 0;
}

void CPhilipsListviewHeader::setTimestampsStoredInAUX(const bool flag)
{
  m_pData->header.timestampsStoredInAUX = flag ? 1 : 0;
}

void CPhilipsListviewHeader::setMainHeaderSize(const short size)
{
  m_pData->header.mainHeaderSize = size;
}

void CPhilipsListviewHeader::setSubHeaderSize(const short size)
{
  m_pData->header.subHeaderSize = size;
}

void CPhilipsListviewHeader::setComment(const QString &comment)
{
  strncpy(m_pData->header.comment, comment.toLatin1().constData(), sizeof(m_pData->header.comment)-1);
}
