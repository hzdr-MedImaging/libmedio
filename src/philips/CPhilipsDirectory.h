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

#ifndef CPHILIPSDIRECTORY_H
#define CPHILIPSDIRECTORY_H

#ifndef __MEDIO_PRIVATE__
#include <CPhilipsDirectoryItem>
#else
#include <CPhilipsDirectoryItem.h>
#endif

// forward declarations
class CPhilipsDirectoryPrivate;
class CPhilipsFile;

class CPhilipsDirectory
{
  public:
    // constructors    
    CPhilipsDirectory(CPhilipsFile* philipsFile);
    ~CPhilipsDirectory();

    // copy constructur and default assignment operator
    CPhilipsDirectory(const CPhilipsDirectory& src);    
    CPhilipsDirectory& operator=(const CPhilipsDirectory& src);  

    // file i/o routines
    bool load();
    bool save() const;

    // accessor methods
    bool isEmpty() const;
    unsigned int count() const;
    CPhilipsDirectoryItem* operator[](unsigned int num) const;
    CPhilipsDirectoryItem* item(short frame=-1, short slice=-1, short tilt=-1);

    // accessor method to get extended main header item
    CPhilipsDirectoryItem* extendedMainHeaderItem() const;

    // methods to calculate the real amount
    // of frame/slice/tilt numbers carried in the directory.
    short maxFrame(void) const;
    short minFrame(void) const;
    short numFrames(void) const;
    short maxSlice(void) const;
    short minSlice(void) const;
    short numSlices(short frame=-1) const;
    short maxTilt(void) const;
    short minTilt(void) const;
    short numTilts(void) const;

    // read methods
    bool readSubHeader(CPhilipsSubHeader*& subHeader,
                       short frame=-1, short slice=-1, short tilt=-1);
    bool readMatrix(QByteArray*& matrixData,
                    short frame=-1, short slice=-1, short tilt=-1);
    bool readMatrix(char*& matrixData, unsigned int& len,
                    short frame=-1, short slice=-1, short tilt=-1);
    bool readMatrix(QByteArray*& matrixData, CPhilipsSubHeader*& subHeader,
                    short frame=-1, short slice=-1, short tilt=-1);
    bool readMatrix(char*& matrixData, unsigned int& len, CPhilipsSubHeader*& subHeader,
                    short frame=-1, short slice=-1, short tilt=-1);

    // write methods
    bool writeSubHeader(const CPhilipsSubHeader& subHeader,
                        short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const QByteArray& matrix,
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const char* matrix, unsigned int size,
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const QByteArray& matrix, CPhilipsSubHeader::Data_Type type,
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const char* matrix, unsigned int size, CPhilipsSubHeader::Data_Type type,
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const QByteArray& matrix, const CPhilipsSubHeader& subHeader,
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const char* matrix, unsigned int size, const CPhilipsSubHeader& subHeader,
                     short frame=-1, short slice=-1, short tilt=-1);

  private:
    CPhilipsDirectoryPrivate* m_pData;
};

#endif // CPHILIPSDIRECTORY_H
