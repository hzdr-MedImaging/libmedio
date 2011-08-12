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

#ifndef CPHILIPSSUBHEADERSINOGRAM_H
#define CPHILIPSSUBHEADERSINOGRAM_H

#ifndef __MEDIO_PRIVATE__
#include <CPhilipsSubHeader>
#else
#include <CPhilipsSubHeader.h>
#endif

// forward declarations
class CPhilipsSubHeaderSinogramPrivate;
class CPhilipsFile;
class CPhilipsDirectoryItem;

class CPhilipsSubHeaderSinogram : public CPhilipsSubHeader
{
  public:
    enum Randoms_Type { UndefinedRandomsType = 0,
                        NoneRandoms,
                        Delayed,
                        Singles
                      };

    enum Decay_Type { UndefinedDecayType = 0,
                      NoneDecay,
                      Acqstart,
                      Injadmin
                    };

    enum Laterality_Type { UnknownLateralityType = 0,
                           Right,
                           Left,
                           Unpaired,
                           Both
                         };

    enum Anatomy_Type { UnknownAnatomyType = 0,
                        Abdomen,
                        Abdomen_And_Pelvis,
                        Chest,
                        Chest_And_Abdomen,
                        Chest_Abdomen_And_Pelvis,
                        Entire_Body,
                        Head,
                        Head_And_Neck,
                        Heart,
                        Neck,
                        Neck_And_Chest,
                        Neck_Chest_And_Abdomen,
                        Neck_Chest_Abdomen_And_Pelvis,
                        Pelvis,
                        Leg,
                        Pelvis_And_Lower_Extremeties
                      };

    // constructors
    CPhilipsSubHeaderSinogram(CPhilipsFile* philipsFile = NULL,
                           CPhilipsDirectoryItem* pDirItem = NULL);
    ~CPhilipsSubHeaderSinogram();

    // copy constructur and default assignment operator
    CPhilipsSubHeaderSinogram(const CPhilipsSubHeaderSinogram& src);    
    CPhilipsSubHeaderSinogram& operator=(const CPhilipsSubHeaderSinogram& src);

    // header clear method
    void clear();

    // public methods
    bool load(void);
    bool save(void) const;

    // the number of bytes the data of that header requires on disk
    int rawDataSize() const;
    
    CPhilipsSubHeader::Type subHeaderType(void) const;

    // clone methods
    bool convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader* pHead2 = NULL);
    CMedIOHeader* clone() const;

  private:
    CPhilipsSubHeaderSinogramPrivate* m_pData;
};

#endif // CPHILIPSSUBHEADERSINOGRAM_H
