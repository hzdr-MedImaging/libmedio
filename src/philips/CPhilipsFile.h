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

#ifndef CPHILIPSFILE_H
#define CPHILIPSFILE_H

#include <QFile>
#include <QString>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOData>
#include <CPhilipsMainHeader>
//#include <CPhilipsSubHeader>
#else
#include <CMedIOData.h>
#include <CPhilipsMainHeader.h>
//#include <CPhilipsSubHeader.h>
#endif

// forward declarations
class CPhilipsFilePrivate;

class CPhilipsFile : public CMedIOData
{
  public:
    // constructors
    CPhilipsFile(const QString& filename);
    ~CPhilipsFile();

    // runtime type information
    int rtti() const;

    // the file open/close methods
    bool open(QIODevice::OpenModeFlag mode);
    void close(void);

    // interface methods to read out specific data from the ECAT files
    bool readMainHeader(CPhilipsMainHeader*& mainHeader);

  private:
    CPhilipsFilePrivate* m_pData;
};

#endif // CPHILIPSFILE_H
