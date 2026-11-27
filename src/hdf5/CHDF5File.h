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

//! @file CHDF5File.h
//! @brief contains the definition of the class CHDF5File

#ifndef CHDF5FILE_H
#define CHDF5FILE_H

#include <QString>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOData>
#else
#include <CMedIOData.h>
#endif

class CHDF5FilePrivate;

//! @class CHDF5File
//! @brief files of the Hierarchical Data Format version 5 (HDF5)
//!
//! HDF5 files are identified by their signature. Their content cannot be read
//! or written yet: open() fails and openError() gives the reason.
//!
//! HDF5 support is an optional feature of the build (CMake option
//! MEDIO_WITH_HDF5), as it needs the HDF5 library. The class and the format
//! CMedIOData::HDF5 exist in every build, so that the interface of libmedio does
//! not depend on it: hasSupport() tells whether this build has HDF5 support,
//! and HDF5 files are identified in every build.
class CHDF5File : public CMedIOData
{
  public:
    CHDF5File(const QString& fileName = QString());
    virtual ~CHDF5File();

    //! @brief true if the file is an HDF5 file: it has the HDF5 signature at its
    //!        start or, after a user block, at offset 512, 1024, 2048, ...
    //!        (the HDF5 library is not needed for this)
    static bool isOfType(const QString& fileName);
    //! @brief a new CHDF5File (not opened) if isOfType(), otherwise NULL
    static CMedIOData* createFromFile(const QString& fileName);

    //! @brief true if this build of libmedio has HDF5 support
    static bool hasSupport();
    //! @brief version of the HDF5 library used, e.g. "1.10.10" (empty without HDF5 support)
    static QString libraryVersion();

    //! @brief not implemented yet: always fails, openError() gives the reason
    bool open(QIODevice::OpenModeFlag mode);
    void close();

    CMedIOData::Format dataFormat() const;

    //! @brief the reason why the last open() failed (empty if it did not)
    QString openError() const;

  private:
    CHDF5File(const CHDF5File&);
    CHDF5File& operator=(const CHDF5File&);

    CHDF5FilePrivate* m_pData;
};

#endif // CHDF5FILE_H
