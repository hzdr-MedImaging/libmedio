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

//! @file CInterfileFrameHeader.h
//! @brief contains the definition of the class CInterfileFrameHeader

#ifndef CINTERFILEFRAMEHEADER_H
#define CINTERFILEFRAMEHEADER_H

#include <QString>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOHeader>
#include <CMedIOVolume>
#include <CInterfileKeys>
#else
#include <CMedIOHeader.h>
#include <CMedIOVolume.h>
#include <CInterfileKeys.h>
#endif

class CInterfileFrameHeaderPrivate;

//! @class CInterfileFrameHeader
//! @brief the description of one frame of an Interfile file (see CInterfileFile)
//!
//! The keys of the header describing the frame (the image header of the data set
//! of a Siemens reconstruction, otherwise the header of the file) and the values
//! read from them: the matrix, the data type of the stored values, the scaling,
//! the position of the voxels and the timing. CInterfileFile::readMatrix() gives
//! the voxels in the order of the file (x fastest, then y, then the planes) in
//! the byte order of this machine.
//!
//! The format independent attributes (attributes()) are the frame attributes
//! (ActualFrameDuration) and the keys of the frame (the image header of a Siemens
//! data set or the frame group of a dynamic Interfile 3.3 study) that do not
//! describe the layout of the image data, in the section "Interfile".
class CInterfileFrameHeader : public CMedIOHeader
{
  public:
    //! @brief constructor
    //! @param file the file the header belongs to (load() reads it from there)
    //! @param frame the number of the frame (1..n)
    CInterfileFrameHeader(CMedIOData* file = NULL, short frame = 1);
    CInterfileFrameHeader(const CInterfileFrameHeader& src);
    virtual ~CInterfileFrameHeader();

    //! @brief copy of another Interfile frame header (other headers: nothing is copied)
    CMedIOHeader& operator=(const CMedIOHeader& src);
    CInterfileFrameHeader& operator=(const CInterfileFrameHeader& src);

    // CMedIOHeader interface
    void clear();
    //! @brief read the header of frame() from the CInterfileFile it belongs to
    bool load();
    //! @brief Interfile files can be read only: false
    bool save() const;
    CMedIOHeader::Format headerFormat() const;
    using CMedIOHeader::convertFrom;
    //! @brief copy of an Interfile frame header (header1); false for other headers
    bool convertFrom(const CMedIOHeader* header1, const CMedIOHeader* header2 = NULL);
    CMedIOHeader* clone() const;

    //! @brief the number of the frame (1..n)
    short frame() const;
    void setFrame(short frame);

    //! @brief the keys of the header describing the frame
    const CInterfileKeys& keys() const;
    void setKeys(const CInterfileKeys& keys);

    //! @brief the header file and the data file of the frame
    QString headerFileName() const;
    void setHeaderFileName(const QString& fileName);
    QString dataFileName() const;
    void setDataFileName(const QString& fileName);

    //! @brief matrix size: columns, rows, planes
    int dimension(int axis) const;
    void setDimensions(int nx, int ny, int nz);

    //! @brief data type of the stored values
    CMedIOVolume::DataType dataType() const;
    void setDataType(CMedIOVolume::DataType type);

    //! @brief physical value = stored value * slope() + intercept(), in units()
    double slope() const;
    double intercept() const;
    void setScaling(double slope, double intercept);
    //! @brief units of the physical values: "BQML" (Bq/ml) where the units of the
    //!        header are a unit of activity concentration (they are converted into
    //!        Bq/ml by the slope), otherwise as given in the header
    QString units() const;
    void setUnits(const QString& units);

    //! @brief position of the voxels: affine m (3 x 4) transforming voxel (i, j, k)
    //!        of the file into DICOM patient coordinates (LPS, mm)
    void affine(double m[3][4]) const;
    void setAffine(const double m[3][4]);

    //! @brief start of the frame relative to the start of the study and its duration (s)
    double frameStart() const;
    double frameDuration() const;
    void setTiming(double start, double duration);

  private:
    CInterfileFrameHeaderPrivate* m_pData;
};

#endif // CINTERFILEFRAMEHEADER_H
