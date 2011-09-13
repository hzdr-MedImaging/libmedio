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

#ifndef CPHILIPSDIRECTORYITEM_H
#define CPHILIPSDIRECTORYITEM_H

#include <QDataStream>

#ifndef __MEDIO_PRIVATE__
#include <CPhilipsSubHeader>
#include <CPhilipsExtendedMainHeader>
#else
#include <CPhilipsSubHeader.h>
#include <CPhilipsExtendedMainHeader.h>
// some useful defines on the Philips file format
// especially for PhilipsBlock calculation as Philips Files
// are seperated in 512 byte blocks and positions within
// the directorylist are refered in this format starting
// from 1.
#define PHILIPS_BLOCKSIZE        512
#define PHILIPS_POS_MAINHEADER   1    // the MainHeader is always at block 1
#define PHILIPS_POS_MAINDIR      2    // the MainDir is always at block 2
#define PhilipsBlock2FilePos(v)  (((v)-1)*PHILIPS_BLOCKSIZE)
#define FilePos2PhilipsBlock(v)  ((v)/PHILIPS_BLOCKSIZE+1)
#define PHILIPS_EXTENDED_HEADER 0x7FFFFFFF // matrixID for the extended header
#endif

// special macros to convert the MatrixID to their respect
// slice/frame and tilt representation
#define matrixID2Frame(x)  ( (x) & 0x0000FFFF)
#define matrixID2Slice(x)  (((x) & 0x0FFF0000) >> 16)
#define matrixID2Tilt(x)   (((x) & 0xF0000000) >> 28)

#define frame2MatrixID(x)  ( (x) & 0x0000FFFF)
#define slice2MatrixID(x)  (((x) & 0x00000FFF) << 16)
#define tilt2MatrixID(x)   (((x) & 0x0000000F) << 28)

#define convertToMatrixID(s, f, t) (slice2MatrixID(s) | \
                                    frame2MatrixID(f) | \
                                    tilt2MatrixID(t) )
// forward declarations
class CPhilipsDirectoryItemPrivate;
class CPhilipsFile;

class CPhilipsDirectoryItem
{
  public:
    enum CompressionFlag { Uncompressed = 0,
                           Compressed = 1
                         };

    enum ContentFlag { Used = 1,
                       Unused = 0,
                       Removed = -1,
                       Header = -2
                     };

    // constructors
    CPhilipsDirectoryItem(CPhilipsFile* file,
                          quint32 matrixID = 0);
    ~CPhilipsDirectoryItem();

    // copy constructur and default assignment operator
    CPhilipsDirectoryItem(const CPhilipsDirectoryItem& src);    
    CPhilipsDirectoryItem& operator=(const CPhilipsDirectoryItem& src);      

    // accessor methods
    quint32 matrixID() const;
    bool isExtendedHeader() const;
    qint64 dataBlock_Start() const;
    qint64 dataBlock_End() const;
    CompressionFlag compressionFlag() const;
    ContentFlag contentFlag() const;
    short frame() const;
    short slice() const;
    short tilt() const;

    // // mutator methods
    // void setDataBlock_Start(const qint64 offset);
    // void setDataBlock_End(const qint64 offset);
    // void setDataBlock_Status(const AccessStatus status);
    // void setFrame(const short f);
    // void setPlane(const short p);
    // void setGate(const short g);
    // void setBed(const short b);
    // void setData(const short d);

    // read i/o methods
    bool readExtendedMainHeader(CPhilipsExtendedMainHeader*& extendedMainHeader);
    bool readSubHeader(CPhilipsSubHeader*& subHeader);
    bool readMatrix(QByteArray*& data);
    bool readMatrix(char*& data, unsigned int& len);
    bool readMatrix(QByteArray*& data, CPhilipsSubHeader*& subHeader);
    bool readMatrix(char*& data, unsigned int& len, CPhilipsSubHeader*& subHeader);

    // // write i/o methods
    // bool writeSubHeader(const CPhilipsSubHeader& subHeader);
    // bool writeMatrix(const QByteArray& data);
    // bool writeMatrix(const char* data, unsigned int len);
    // bool writeMatrix(const QByteArray& data, CPhilipsSubHeader::Data_Type type);
    // bool writeMatrix(const char* data, unsigned int len, CPhilipsSubHeader::Data_Type type);
    // bool writeMatrix(const QByteArray& data, const CPhilipsSubHeader& subHeader);
    // bool writeMatrix(const char* data, unsigned int len, const CPhilipsSubHeader& subHeader);

    // our QDataStream operators
    friend QDataStream& operator<<(QDataStream& stream, const CPhilipsDirectoryItem& item);
    friend QDataStream& operator>>(QDataStream& stream, CPhilipsDirectoryItem& item);

    // // internal methods to sync specific data with our headers
    // void subHeaderWritten(const CPhilipsSubHeader& subHeader);

  private:
    CPhilipsDirectoryItemPrivate* m_pData;
};

#endif // CPHILIPSDIRECTORYITEM_H
