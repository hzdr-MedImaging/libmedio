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

#ifndef CECAT6MAINHEADER_H
#define CECAT6MAINHEADER_H

#include <QDataStream>
#include <QTextStream>

#ifndef __MEDIO_PRIVATE__
#include <CECATMainHeader>
#else
#include <CECATMainHeader.h>
#endif

// forward declarations
class CECAT6MainHeaderPrivate;
class CECATFile;
class CMedIOHeader;

class CECAT6MainHeader : public CECATMainHeader
{
  public:
    enum File_Type { Unknown=0, // FIXME: Others are missing for the moment
                   };

    // constructors
    CECAT6MainHeader(CECATFile* ecatFile = NULL,
                     CECATMainHeader::Type fileType = CECATMainHeader::Unknown);
    ~CECAT6MainHeader();

    // copy constructur and default assignment operator
    CECAT6MainHeader(const CECAT6MainHeader& src);    
    CECAT6MainHeader& operator=(const CECAT6MainHeader& src);
    
    // header clear method
    void clear();

    // public methods
    bool load(void);
    bool save(void) const;

    // the number of bytes the data of that header requires on disk
    int rawDataSize() const;

    // runtime type information methods
    int rtti() const;

    // clone methods
    bool convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader* pHead2 = NULL);
    CMedIOHeader* clone() const;

    // accessor Methods
    const char* original_File_Name(void) const;
    short sw_Version(void) const;
    short system_Type(void) const;
    File_Type file_Type(void)  const;
    short num_Planes(void) const;
    short num_Frames(void) const;
    short num_Gates(void) const;
    short num_Bed_Pos(void) const;

    // mutator methods
    void setOriginal_File_Name(const char* name);
    void setSW_Version(const short ver);
    void setSystem_Type(const short type);
    void setFileType(const File_Type fType);
    void setNum_Planes(const short num);
    void setNum_Frames(const short num);
    void setNum_Gates(const short num);
    void setNum_Bed_Pos(const short num);

  private:
    CECAT6MainHeaderPrivate* m_pData;
};

#endif // CECAT6MAINHEADER_H
