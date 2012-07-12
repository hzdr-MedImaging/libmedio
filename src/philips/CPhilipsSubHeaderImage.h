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

#ifndef CPHILIPSSUBHEADERIMAGE_H
#define CPHILIPSSUBHEADERIMAGE_H

#ifndef __MEDIO_PRIVATE__
#include <CPhilipsSubHeader>
#else
#include <CPhilipsSubHeader.h>
#endif

// forward declarations
class CPhilipsFile;
class CPhilipsDirectoryItem;
class CPhilipsMainHeader;
class CPhilipsExtendedMainHeader;

class CPhilipsSubHeaderImage : public CPhilipsSubHeader
{
  public:
    // constructors
    CPhilipsSubHeaderImage(CPhilipsFile* philipsFile = NULL,
                           CPhilipsDirectoryItem* pDirItem = NULL);

    // copy constructur and default assignment operator
    CPhilipsSubHeaderImage(const CPhilipsSubHeaderImage& src);    
  
    // return the subHeader type
    CPhilipsSubHeader::Type subHeaderType(void) const;

    // clone methods
    CMedIOHeader* clone() const;

    // data access methods
    float suvscl() const;
    float imgscl() const;
    short imgmin() const;
    short imgmax() const;
    float imgsum() const;
    float scale_Factor(bool& ok) const;

    // methods to modify elements of the SubHeader  
    void setSuvscl(const float suvscl);
    void setImgscl(const float imgscl);
    void setImgmin(const short imgmin);
    void setImgmax(const short imgmax);
    void setImgsum(const float imgsum);
};

#endif // CPHILIPSSUBHEADERIMAGE_H
