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

#ifndef CPHILIPSSUBHEADER_H
#define CPHILIPSSUBHEADER_H

#ifndef __MEDIO_PRIVATE__
#include <CMedIOHeader>
#else
#include <CMedIOHeader.h>
#endif

// forward declarations
class CPhilipsDirectoryItem;

class CPhilipsSubHeader : public CMedIOHeader
{
  public:
    enum Type { Unknown = 0,
                Image,
                Sinogram
              };

    enum Data_Type { ByteData = 1,      // 1: char
                     SignedShort = 2,   // 2: short
                     UnsignedShort = 3, // 2: unsigned short
                     Float = 4          // 4: float
                   };

    // constructor
    CPhilipsSubHeader(CMedIOData* philipsFile,
                   CPhilipsDirectoryItem* dItem = NULL);
    
    // copy constructur and default assignment operator
    CPhilipsSubHeader(const CPhilipsSubHeader& src);    
    CPhilipsSubHeader& operator=(const CPhilipsSubHeader& src);

    // header clear method
    virtual void clear() = 0;

    // our virtual load/save I/O routines
    virtual bool load(void) = 0;
    virtual bool save(void) const = 0;

    // size information in bytes of specific raw subheader structure
    virtual int rawDataSize() const = 0;

    // runtime type information methods
    CMedIOHeader::Format headerFormat() const;
    virtual CPhilipsSubHeader::Type subHeaderType(void) const = 0;

    // internal method to set the directory Item of a subHeader
    void setDirectoryItem(CPhilipsDirectoryItem* dItem);

  protected:
    CPhilipsDirectoryItem* m_pDirItem; // the directory item to which this
                                       // subHeader belongs
};

#endif // CPHILIPSSUBHEADER_H
