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

#ifndef CPHILIPSLISTVIEWHEADER_H
#define CPHILIPSLISTVIEWHEADER_H

#include <QString>

// forward declarations
class CPhilipsListviewHeaderPrivate;
class CPhilipsFile;

class CPhilipsListviewHeader
{
  public:
    enum File_Format
    {
      Unknown = 0,
      LOR4 = 14,
      LOR8 = 15,
      Xtal = 16
    };

    explicit CPhilipsListviewHeader(CPhilipsFile* file);
    ~CPhilipsListviewHeader();

    // header clear method
    void clear();

    // file i/o methods
    bool load();
    bool save() const;

    int numberOfEventsPerBuffer() const;
    QString magicNumber() const;
    File_Format listModeFormat() const;
    bool singlesMode() const;
    bool timestampsStoredInAUX() const;
    short mainHeaderSize() const;
    short subHeaderSize() const;
    QString comment() const;

    void setNumberOfEventsPerBuffer(const int n);
    void setMagicNumber(const QString& magicNumber);
    void setFileFormat(const File_Format format);
    void setSinglesMode(const bool flag);
    void setTimestampsStoredInAUX(const bool flag);
    void setMainHeaderSize(const short size);
    void setSubHeaderSize(const short size);
    void setComment(const QString& comment);

  private:
    CPhilipsListviewHeaderPrivate* m_pData;
};

#endif // CPHILIPSLISTVIEWHEADER_H
