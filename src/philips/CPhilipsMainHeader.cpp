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

#include <QDataStream>

#include "CPhilipsMainHeader.h"
#include "CPhilipsFile.h"
#include "CMedIOHeader.h"
#include "CMedIOData.h"

#include <rtdebug.h>

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CPhilipsMainHeaderPrivate
{
  public:
    // MainHeader structure (should be 512 bytes)
    struct HeaderData
    {
      qint16 File_Format;       // file format (mainheader) version number
      qint16 Scanner_Geometry;  // encoding of the scanner geometry
      qint16 Hardware_Config;   // encoding of hardware used in acquisition
      qint16 Edit_Flag;         // 1 to indicat that the user has modified mainheader
      qint16 File_Type;         // file type
    } header;
};

CPhilipsMainHeader::CPhilipsMainHeader(CPhilipsFile* file,
                                       CPhilipsMainHeader::File_Type fileType)
  : CMedIOHeader(file)
{
  ENTER();

  m_pData = new CPhilipsMainHeaderPrivate();

  // this constructor creates a empty EPhilipsMainHeader
  // with prefilled data that is always the same for a philips main header
  clear();

  setFile_Type(fileType);

  LEAVE();
}

CPhilipsMainHeader::CPhilipsMainHeader(const CPhilipsMainHeader& src)
  : CMedIOHeader(src)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CPhilipsMainHeaderPrivate(*(src.m_pData));

  LEAVE();
}

CPhilipsMainHeader& CPhilipsMainHeader::operator=(const CPhilipsMainHeader& src)
{
  ENTER();

  if(m_pData != src.m_pData)
  {
    memcpy(&m_pData->header, 
           &src.m_pData->header, 
           sizeof(struct CPhilipsMainHeaderPrivate::HeaderData));
  }

  LEAVE();
  return *this;
}
CPhilipsMainHeader::~CPhilipsMainHeader()
{
  ENTER();

  delete m_pData;

  LEAVE();
}

void CPhilipsMainHeader::clear()
{
  ENTER();
  // clear our MainHeader structure first
  memset(&m_pData->header, 0, sizeof(struct CPhilipsMainHeaderPrivate::HeaderData));

  // set some default values for the new Main Header

  LEAVE();
}

bool CPhilipsMainHeader::load()
{
  ENTER();

  // only go on if the device is readable at all
  if(m_pMedIOData == NULL ||
     m_pMedIOData->isReadable() == false ||
     m_pMedIOData->seek(0) == false)
  {
    RETURN(false);
    return false;
  }

  // we use a ByteArray buffer to speed up the endianess
  // decoding
  QByteArray buffer(rawDataSize(), 0);
  if(m_pMedIOData->read(buffer.data(), buffer.size()) != rawDataSize())
  {
    RETURN(false);
    return false;
  }

  // now we generate a QDataStream on our buffer so that we can read
  // out of the buffer instead of the raw file (> speed)
  QDataStream stream(buffer);

  // we have to set the QDataStream version to the Qt4.5 version
  // because with Qt4.6 the floating point precision changed and
  // otherwise causes our streaming to fail
  stream.setVersion(QDataStream::Qt_4_5);

  // we now read out the header information stepwise
  stream.skipRawData(6);                      // 0: skip the first 6 bytes
  stream >> m_pData->header.File_Format;      // 6: File_Format
  stream >> m_pData->header.Scanner_Geometry; // 8: Scanner_Geometry
  stream >> m_pData->header.Hardware_Config;  // 10: Hardware_Config
  stream.skipRawData(4);                      // 12: skip the next 4 bytes
  stream >> m_pData->header.Edit_Flag;        // 16: Edit_Flag

#if defined(DEBUG)
  D("philips Main Header loaded:");
  D("--------------------------");
  D("File_Format:              : %d", m_pData->header.File_Format);
  D("Scanner_Geometry          : %d", m_pData->header.Scanner_Geometry);
  D("Hardware_Config           : %d", m_pData->header.Hardware_Config);
  D("Edit_Flag                 : %d", m_pData->header.Edit_Flag);
#endif

  RETURN(true);
  return true;
}

bool CPhilipsMainHeader::save(void) const
{
  ENTER();
#warning TODO: implement CPhilipsMainHEader::save()
  RETURN(false);
  return false;
}

void CPhilipsMainHeader::setFile_Type(const File_Type fType)
{
  ENTER();
  m_pData->header.File_Type = fType;
  LEAVE();
}

int CPhilipsMainHeader::rawDataSize() const
{
  return PHILIPS_BLOCKSIZE;
}

CMedIOHeader::Format CPhilipsMainHeader::headerFormat() const
{ 
  return CMedIOHeader::ECATMainHeader;
}

bool CPhilipsMainHeader::convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader* pHead2) 
{
  ENTER();
  bool bResult = false;
#warning TODO: implement CPhilipsMainHeader::convertFrom()
  RETURN(false);
  return false;
}

CMedIOHeader* CPhilipsMainHeader::clone() const
{
  ENTER();

  CPhilipsMainHeader* pNewHeader = new CPhilipsMainHeader(*this);

  RETURN(pNewHeader);
  return pNewHeader;
}

short CPhilipsMainHeader::file_Format() const
{
  return m_pData->header.File_Format;
}

short CPhilipsMainHeader::scanner_Geometry() const
{
  return m_pData->header.Scanner_Geometry;
}

short CPhilipsMainHeader::hardware_Config() const
{
  return m_pData->header.Hardware_Config;
}

short CPhilipsMainHeader::edit_Flag() const
{
  return m_pData->header.Edit_Flag;
}

short CPhilipsMainHeader::file_Type() const
{
  return m_pData->header.File_Type;
}


void CPhilipsMainHeader::setFile_Format(const short format)
{
  m_pData->header.File_Format = format;
}

void CPhilipsMainHeader::setScanner_Geometry(const short geometry)
{
  m_pData->header.Scanner_Geometry = geometry;
}

void CPhilipsMainHeader::setHardware_Config(const short conifg)
{
  m_pData->header.Hardware_Config = conifg;
}

void CPhilipsMainHeader::setEdit_Flag(const short eFlag)
{
  m_pData->header.Edit_Flag = eFlag;
}

void CPhilipsMainHeader::setFile_Type(const short fType)
{
  m_pData->header.File_Type = fType;
}
