/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio
 
  Copyright (C) 2004-2026 hzdr.de and contributors
 
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
    http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

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
class CECATSubHeader;

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
    CMedIOData::Format dataFormat() const;

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
    short maxTilt(void) const;
    short minTilt(void) const;
    short numTilts(void) const;

    // interface methods to read out specific data from the philips files
    bool readMainHeader(CPhilipsMainHeader*& mainHeader);
    bool readSubHeader(CPhilipsSubHeader*& subHeader, short frame=-1, short slice=-1, short tilt=-1);
    bool readMatrix(QByteArray*& matrixData,
                    short frame=-1, short slice=-1, short tilt=-1);
    bool readMatrix(char*& matrixData, unsigned int& len,
                    short frame=-1, short slice=-1, short tilt=-1);
    bool readMatrix(QByteArray*& matrixData, CPhilipsSubHeader*& subHeader,
                    short frame=-1, short slice=-1, short tilt=-1);
    bool readMatrix(char*& matrixData, unsigned int& len, CPhilipsSubHeader*& subHeader,
                    short frame=-1, short slice=-1, short tilt=-1);

    // ECAT7 representation of one frame of a Philips image or Syntegra file,
    // as returned by CECATFile::readMatrix() and CNIFTIFile::readMatrix():
    // voxel data of all slices in ECAT7 voxel order and a
    // CECAT7SubHeaderImage (to be deleted by the caller) describing it like
    // the image sub header of an ECAT7 file: dimensions, voxel sizes, data
    // type, scale factor, value range and frame timing, converted with
    // CECAT7SubHeaderImage::convertFrom(philipsSubHeader, philipsMainHeader)
    // from the sub header of the first slice. The ECAT7 main header is
    // obtained with CECAT7MainHeader::convertFrom(philipsMainHeader,
    // philipsSubHeader) and the sub header of the first slice (readSubHeader()).
    //
    // - frame is the frame number as in the methods above (usually 1 to
    //   numFrames(), like the ECAT7 frame numbers); -1 = default = minFrame().
    // - The slices of the frame are assembled as with slice -1 in the
    //   methods above; the voxel order within a slice already is the ECAT7
    //   voxel order. The voxel data is in the byte order of this machine.
    // - Physical value = stored value x scale factor (calibration factor 1),
    //   the scale factor of a slice is CPhilipsSubHeaderImage::ecat_Scale_Factor()
    //   (1 if the slice has no valid scale factor).
    // - Data types: byte, signed short and float are returned as ByteData,
    //   SunShort and IEEEFloat, unsigned short as SunLong, with the stored
    //   values and the common scale factor of the slices. If the slices of
    //   the frame have different scale factors, the physical values are
    //   returned as IEEEFloat with scale factor 1.
    bool readMatrix(QByteArray*& matrixData, CECATSubHeader*& subHeader, short frame=-1);
    bool readMatrix(char*& matrixData, unsigned int& len, CECATSubHeader*& subHeader, short frame=-1);

    // methods to write out headers/data to the file.
    bool writeMainHeader(CPhilipsMainHeader& mainHeader);
    bool writeSubHeader(const CPhilipsSubHeader& subHeader,
                        short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const QByteArray& matrixData,
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const char* matrixData, unsigned int size,
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const QByteArray& matrixData, const CPhilipsSubHeader& subHeader,
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const char* matrixData, unsigned int size, const CPhilipsSubHeader& subHeader,
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const QByteArray& matrixData, CPhilipsSubHeader::Data_Type type, 
                     short frame=-1, short slice=-1, short tilt=-1);
    bool writeMatrix(const char* matrixData, unsigned int size, CPhilipsSubHeader::Data_Type type,
                     short frame=-1, short slice=-1, short tilt=-1);

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
