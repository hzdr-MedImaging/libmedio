/* vim:set ts=2 sw=2 expandtab: ********************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2006-2010 by Jens Langner <Jens.Langner@light-speed.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

#ifndef CECATFILE_H
#define CECATFILE_H

#include <QFile>
#include <QString>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOData>
#include <CECATMainHeader>
#include <CECATSubHeader>
#else
#include <CMedIOData.h>
#include <CECATMainHeader.h>
#include <CECATSubHeader.h>
#endif

// forward declarations
class CECATFilePrivate;
class CECATDirectory;

class CECATFile : public CMedIOData
{
  public:
    enum ECATFormat  { Undefined=0, ECAT7, ECAT6 };

    // constructors
    CECATFile(const QString& filename,
              CECATMainHeader::Type fileType = CECATMainHeader::Unknown);
    ~CECATFile();
    
    // static methods for the MedIOData interface
    static bool isOfType(const QString& fileName);
    static CMedIOData* createFromFile(const QString& fileName);

    // runtime type information
    int rtti() const;
  
    // the file open/close methods
    bool open(QIODevice::OpenModeFlag mode);
    void close(void);

    ECATFormat format(void) const;
    CECATMainHeader::Type fileType(void);
    bool setFileType(CECATMainHeader::Type fileType);

    // methods to query some information on the subHeader type
    CECATSubHeader::Type subHeaderType(void);
    CECATSubHeader::Type subHeaderType(CECATMainHeader::Type fileType);

    // methods to calculate the real amount
    // of frame/plane/gate numbers carried in the directory.
    short numFrames(void) const;
    short numPlanes(void) const;
    short numGates(void) const;
    short numBedPos(void) const;

    // interface methods to read out specific data from the ECAT files
    bool readMainHeader(CECATMainHeader*& mainHeader);
    bool readSubHeader(CECATSubHeader*& subHeader,
                       short frame, short plane=1, short gate=1, short bed=0, short data=0);
    bool readMatrix(QByteArray*& matrixData,
                    short frame, short plane=1, short gate=1, short bed=0, short data=0);
    bool readMatrix(char*& matrixData, unsigned int& len,
                    short frame, short plane=1, short gate=1, short bed=0, short data=0);
    bool readMatrix(QByteArray*& matrixData, CECATSubHeader*& subHeader,
                    short frame, short plane=1, short gate=1, short bed=0, short data=0);
    bool readMatrix(char*& matrixData, unsigned int& len, CECATSubHeader*& subHeader,
                    short frame, short plane=1, short gate=1, short bed=0, short data=0);

    // methods to write out headers/data to the file.
    bool writeMainHeader(CECATMainHeader& mainHeader);
    bool writeSubHeader(const CECATSubHeader& subHeader,
                        short frame, short plane=1, short gate=1, short bed=0, short data=0);
    bool writeMatrix(const QByteArray& matrixData,
                     short frame, short plane=1, short gate=1, short bed=0, short data=0);
    bool writeMatrix(const char* matrixData, unsigned int size,
                     short frame, short plane=1, short gate=1, short bed=0, short data=0);
    bool writeMatrix(const QByteArray& matrixData, const CECATSubHeader& subHeader,
                     short frame, short plane=1, short gate=1, short bed=0, short data=0);
    bool writeMatrix(const char* matrixData, unsigned int size, const CECATSubHeader& subHeader,
                     short frame, short plane=1, short gate=1, short bed=0, short data=0);    
    bool writeMatrix(const QByteArray& matrixData, CECATSubHeader::Data_Type type, 
                     short frame, short plane=1, short gate=1, short bed=0, short data=0);
    bool writeMatrix(const char* matrixData, unsigned int size, CECATSubHeader::Data_Type type,
                     short frame, short plane=1, short gate=1, short bed=0, short data=0);

    // methods to generate some objects just for convienence
    CECATMainHeader* createEmptyMainHeader(void);
    CECATSubHeader*  createEmptySubHeader(void);

    // more advanced methods to access the directory list of an ECAT file
    CECATDirectory* directory(void) const;

    // internal methods to sync specific data with our headers
    void mainHeaderWritten(const CECATMainHeader& mainHeader);
    bool reWriteMainHeader(void);

  private:
    CECATFilePrivate*  m_pData;
};

#endif // CECATFILE_H
