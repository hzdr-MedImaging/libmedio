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

//! @file CMedIODataFactory.h
//! @brief contains definition of the class CMedIODataFactory

#ifndef CMEDIODATAFACTORY_H
#define CMEDIODATAFACTORY_H

#ifndef __MEDIO_PRIVATE__
#include <CMedIOData>
#else
#include <CMedIOData.h>
#endif

#include <QString>

//! @class CMedIODataFactory
//! @brief factory class allowing construction of CMedIOData objects 
//! @bug no bugs reported
//! @warning still in development
//!
//! This class is a factory which allows easy construction of different 
//! CMedIOData objects. All kinds of medical data which are implemented
//! can be instancieted by this class.
class CMedIODataFactory
{
  public:
    //! @brief identifies fileformat and returns the type of it
    static CMedIOData::Format identify(const QString& fileName);

    //! @brief identifies fileformat and constructs CMedIOData object
    //! @param file: filename where medical data is located
    //! @return CMedIOData object if format is known otherwise NULL
    static CMedIOData* create(const QString& fileName);

    //-------------------------------------------------------------------------------------
    // New files of the writable formats (ECAT7, NIfTI, Philips, Interfile)

    //! @brief format of a new file, taken from the extension of its name (any letter case):
    //!        ".v" ECAT, ".nii", ".nii.gz", ".hdr" and ".hdr.gz" NIFTI, ".img" and ".syn" Philips,
    //!        ".hv" Interfile
    //! @return Unknown for other names; identify() determines the format of an existing
    //!         file from its content
    static CMedIOData::Format formatOfFileName(const QString& fileName);

    //! @brief constructs a CMedIOData object to write a new file
    //! @param fileName name of the file
    //! @param format ECAT: ECAT7 image volumes (CECATMainHeader::ECAT7_Volume16), NIFTI:
    //!        NIfTI-1, as single file or header/image pair depending on the name (see
    //!        CNIFTIFile), Philips: an image file, a Syntegra file for names ending with
    //!        ".syn", Interfile: the header with this name and the data file <name>.v (as STIR,
    //!        see CInterfileFile); Unknown: the format of formatOfFileName()
    //! @return the new object (not opened, to be deleted by the caller), NULL if the format
    //!         is unknown or cannot be written
    //!
    //! Check with CMedIOData::fileNames() that none of the files exists before the file is
    //! opened for writing, write the main header with CMedIOData::writeMainHeader() and the
    //! frames with CMedIOData::writeVolume(), and remove the files with
    //! CMedIOData::removeFiles() if writing fails.
    static CMedIOData* createForWriting(const QString& fileName,
                                        CMedIOData::Format format = CMedIOData::Unknown);

    //-------------------------------------------------------------------------------------
    // Names of the formats, as used by the language bindings (Python, R)

    //! @brief name of a format: "unknown", "ecat", "micropet", "philips", "philipsbin",
    //!        "nifti" or "interfile" (a string literal)
    static const char* formatName(CMedIOData::Format format);

    //! @brief format of a name, in any letter case: names starting with "e" are ECAT, with
    //!        "n" NIFTI, with "philipsbin" PhilipsBin, with other "p" Philips, with "c" or "m"
    //!        ConcordeMicropet, with "i" Interfile and with "u" Unknown, so that the names of
    //!        formatName() and abbreviations like "NIfTI" or "micropet" are accepted
    //! @return false (format unchanged) for other names
    static bool formatFromName(const QString& name, CMedIOData::Format& format);
};

#endif
