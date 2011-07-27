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

#ifndef CPHILIPSMAINHEADER_H
#define CPHILIPSMAINHEADER_H

#include <QTextStream>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOHeader>
#else
#include <CMedIOHeader.h>
#endif

// forward declarations
class CPhilipsMainHeaderPrivate;
class CPhilipsFile;

class CPhilipsMainHeader : public CMedIOHeader
{
  public:
    // possible Philips file types
    enum File_Type {Unknown = 0, Sinogram, Image, VID, OTHER, Syntegra, Listmode, Rawmode};

    // constructors
    CPhilipsMainHeader(CPhilipsFile* file,
                       CPhilipsMainHeader::File_Type = CPhilipsMainHeader::Unknown);
    ~CPhilipsMainHeader();

    // copy constructor and default assignment operator
    CPhilipsMainHeader(const CPhilipsMainHeader& src);
    CPhilipsMainHeader& operator=(const CPhilipsMainHeader& src);

    // header clear method
    void clear();

    // file i/o methods
    bool load();
    bool save() const;

    // the number of bytes the data of that header requires on disk
    int rawDataSize() const;

    void setFile_Type(const File_Type fType);

    // runtime type information methods
    int rtti() const;
    CMedIOHeader::Format headerFormat() const;

    // clone methods
    virtual bool convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader* pHead2 = NULL);
    virtual CMedIOHeader* clone() const;

    bool isPETMR() const;

    short file_Format() const;
    short scanner_Geometry() const;
    short hardware_Config() const;
    short edit_Flag() const;
    short file_Type() const;
    short num_Frames() const;
    short num_Slices() const;
    short num_Tilts() const;

    void setFile_Format(const short format);
    void setScanner_Geometry(const short geometry);
    void setHardware_Config(const short conifg);
    void setEdit_Flag(const short eFlag);
    void setFile_Type(const short fType);

  private:
    CPhilipsMainHeaderPrivate* m_pData;
};

#endif // CPHILIPSMAINHEADER_H
