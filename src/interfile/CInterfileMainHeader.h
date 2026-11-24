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

//! @file CInterfileMainHeader.h
//! @brief contains the definition of the class CInterfileMainHeader

#ifndef CINTERFILEMAINHEADER_H
#define CINTERFILEMAINHEADER_H

#include <QString>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOHeader>
#include <CInterfileKeys>
#else
#include <CMedIOHeader.h>
#include <CInterfileKeys.h>
#endif

class CInterfileMainHeaderPrivate;

//! @class CInterfileMainHeader
//! @brief the main header of an Interfile file (see CInterfileFile)
//!
//! The keys of the header file that was opened: the main header of a Siemens
//! reconstruction (.mhdr), otherwise the header of the image data. The format
//! independent attributes (attributes()) are taken from these keys and, for a
//! Siemens main header, from the image header of its first data set (patient,
//! study, radiopharmaceutical, reconstruction); the keys that do not describe the
//! layout of the image data are in the section "Interfile" (repeated keys as array).
//!
//! Interfile files can be read only: save() fails.
class CInterfileMainHeader : public CMedIOHeader
{
  public:
    //! @brief constructor
    //! @param file the file the header belongs to (load() reads it from there)
    CInterfileMainHeader(CMedIOData* file = NULL);
    CInterfileMainHeader(const CInterfileMainHeader& src);
    virtual ~CInterfileMainHeader();

    //! @brief copy of another Interfile main header (other headers: nothing is copied)
    CMedIOHeader& operator=(const CMedIOHeader& src);
    CInterfileMainHeader& operator=(const CInterfileMainHeader& src);

    // CMedIOHeader interface
    void clear();
    //! @brief read the header from the CInterfileFile it belongs to
    bool load();
    //! @brief Interfile files can be read only: false
    bool save() const;
    CMedIOHeader::Format headerFormat() const;
    using CMedIOHeader::convertFrom;
    //! @brief copy of an Interfile main header (header1); false for other headers
    bool convertFrom(const CMedIOHeader* header1, const CMedIOHeader* header2 = NULL);
    CMedIOHeader* clone() const;

    //! @brief the keys of the header
    const CInterfileKeys& keys() const;
    void setKeys(const CInterfileKeys& keys);

    //! @brief the file the keys were read from
    QString headerFileName() const;
    void setHeaderFileName(const QString& fileName);

    //! @brief number of frames of the file
    int numFrames() const;
    void setNumFrames(int frames);

  private:
    CInterfileMainHeaderPrivate* m_pData;
};

#endif // CINTERFILEMAINHEADER_H
