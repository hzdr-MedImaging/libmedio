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

//! @file CHDF5MainHeader.h
//! @brief contains the definition of the class CHDF5MainHeader

#ifndef CHDF5MAINHEADER_H
#define CHDF5MAINHEADER_H

#include <QString>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOHeader>
#else
#include <CMedIOHeader.h>
#endif

class CHDF5MainHeaderPrivate;

//! @class CHDF5MainHeader
//! @brief the main header of an HDF5 image file (see CHDF5File)
//!
//! The format independent attributes (attributes()) are the attributes of the
//! group /header of the file (DICOM keywords) and the keys of its JSON object
//! /header/json that have no typed attribute; the attributes of the single
//! volumes are those of CHDF5FrameHeader.
class CHDF5MainHeader : public CMedIOHeader
{
  public:
    //! @brief constructor
    //! @param file the file the header belongs to (load() reads it from there)
    CHDF5MainHeader(CMedIOData* file = NULL);
    CHDF5MainHeader(const CHDF5MainHeader& src);
    virtual ~CHDF5MainHeader();

    //! @brief copy of another HDF5 main header (other headers: nothing is copied)
    CMedIOHeader& operator=(const CMedIOHeader& src);
    CHDF5MainHeader& operator=(const CHDF5MainHeader& src);

    // CMedIOHeader interface
    void clear();
    //! @brief read the header from the CHDF5File it belongs to
    bool load();
    //! @brief HDF5 files cannot be written yet: false
    bool save() const;
    CMedIOHeader::Format headerFormat() const;
    using CMedIOHeader::convertFrom;
    //! @brief copy of an HDF5 main header (header1); false for other headers
    bool convertFrom(const CMedIOHeader* header1, const CMedIOHeader* header2 = NULL);
    CMedIOHeader* clone() const;

    //! @brief version of the layout of the file, e.g. "0.1"
    QString formatVersion() const;
    void setFormatVersion(const QString& version);

    //! @brief number of volumes of the file (all frames, gates and bed positions)
    int numVolumes() const;
    void setNumVolumes(int volumes);

    //! @brief number of frames of the first gate and bed position (CMedIOData::frameNumbers())
    int numFrames() const;
    void setNumFrames(int frames);

  private:
    CHDF5MainHeaderPrivate* m_pData;
};

#endif // CHDF5MAINHEADER_H
