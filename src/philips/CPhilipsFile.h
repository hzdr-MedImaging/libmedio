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
#include <CPhilipsSubHeader>
#else
#include <CMedIOData.h>
#include <CPhilipsMainHeader.h>
#include <CPhilipsSubHeader.h>
#endif

// forward declarations
class CPhilipsFilePrivate;
class CPhilipsDirectory;

class CPhilipsFile : public CMedIOData
{
  public:
    // constructors
    CPhilipsFile(const QString& filename,
                 CPhilipsMainHeader::File_Type fileType = CPhilipsMainHeader::Unknown);
    CPhilipsFile(CPhilipsMainHeader::File_Type fileType = CPhilipsMainHeader::Unknown);
    ~CPhilipsFile();

    // static methods for the MedIOData Interface
    static bool isOfType(const QString& filename);
    static CMedIOData* createFromFile(const QString& fileName);

    // runtime type information
    int rtti() const;

    // the file open/close methods
    bool open(QIODevice::OpenModeFlag mode);
    void close(void);

    CPhilipsMainHeader::File_Type fileType();
    CPhilipsSubHeader::Type subHeaderType();

    // methods to calculate the real amount
    // of frame/plane/gate numbers carried in the directory.
    short maxFrame(void) const;
    short minFrame(void) const;
    short numFrames(void) const;
    short maxSlice(void) const;
    short minSlice(void) const;
    short numSlices(short frame=-1) const;
    short numTilts(void) const;

    // interface methods to read out specific data from the philips files
    bool readMainHeader(CPhilipsMainHeader*& mainHeader);
    bool readSubHeader(CPhilipsSubHeader*& subHeader, short frame=-1, short slice=-1, short tilt=0);
    bool readMatrix(QByteArray*& matrixData,
                    short frame=-1, short slice=-1, short tilt=0);
    bool readMatrix(char*& matrixData, unsigned int& len,
                    short frame=-1, short slice=-1, short tilt=0);
    bool readMatrix(QByteArray*& matrixData, CPhilipsSubHeader*& subHeader,
                    short frame=-1, short slice=-1, short tilt=0);
    bool readMatrix(char*& matrixData, unsigned int& len, CPhilipsSubHeader*& subHeader,
                    short frame=-1, short slice=-1, short tilt=0);

    // methods to write out headers/data to the file.
    bool writeMainHeader(CPhilipsMainHeader& mainHeader);
    bool writeSubHeader(const CPhilipsSubHeader& subHeader,
                        short frame=-1, short slice=-1, short tilt=0);
    bool writeMatrix(const QByteArray& matrixData,
                     short frame=-1, short slice=-1, short tilt=0);
    bool writeMatrix(const char* matrixData, unsigned int size,
                     short frame=-1, short slice=-1, short tilt=0);
    bool writeMatrix(const QByteArray& matrixData, const CPhilipsSubHeader& subHeader,
                     short frame=-1, short slice=-1, short tilt=0);
    bool writeMatrix(const char* matrixData, unsigned int size, const CPhilipsSubHeader& subHeader,
                     short frame=-1, short slice=-1, short tilt=0);
    bool writeMatrix(const QByteArray& matrixData, CPhilipsSubHeader::Data_Type type, 
                     short frame=-1, short slice=-1, short tilt=0);
    bool writeMatrix(const char* matrixData, unsigned int size, CPhilipsSubHeader::Data_Type type,
                     short frame=-1, short slice=-1, short tilt=0);

    // more advanced methods to access the directory list of a philips file
    CPhilipsDirectory* directory() const;

    // methods to generate some objects just for convienence
    CPhilipsMainHeader* createEmptyMainHeader();
    CPhilipsSubHeader* createEmptySubHeader();

    // internal methods to sync specific data with our headers
    void mainHeaderWritten(const CPhilipsMainHeader& mainHeader);
    bool reWriteMainHeader();

  private:
    CPhilipsFilePrivate* m_pData;
};

#endif // CPHILIPSFILE_H
