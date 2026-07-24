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

//! @file CMedIOData.h
//! @brief contains definition of the class CMedIOData

#ifndef CMEDIODATA_H
#define CMEDIODATA_H

#include <QFile>
#include <QString>

// forward declarations
class CMedIOHeader;

//! @class CMedIOData
//! @brief an abstract class which provides an interface for medical data
//! @bug no bugs reported
//! @warning still in development
//!
//! This class is an interface for easier handling of different medical data formats.
//! All data formats which are included in Medical IO Library should use the class
//! as a baseclass.
class CMedIOData : public QFile
{
  public:
    //! @enum enumeration of supported medical data formats
    enum Format
    {  
      Unknown = 0,      //!< unknown data which is not supported
      ConcordeMicropet,  //!< data produced by a concorde micropet scanner  
      ECAT,              //!< ECAT6/7 format
      Philips,           //!< data produced by a philips PET/MR scanner
      PhilipsBin,
      NIFTI              //!< NIfTI format
    };

    //! @enum enumeration carrying special IO error codes which
    //!       can be queried with the errorStatus() method.
    enum errStatus { MERR_Ok = 0,         //!< No Error
                     MERR_Unspecified = 1 //!< Some unspecified error occurred
                   };

    //! @brief constructor
    //! @param file: complete path to file holding medical data
    CMedIOData(const QString& filename);

    //! @brief destructor
    virtual ~CMedIOData();
      
    //! @brief opens the file for a certain operation
    //! @return true if loading is succesful otherwise false
    virtual bool open(QIODevice::OpenModeFlag mode) = 0;
    
    //! @brief saves the data  
    //! @return true if saving is succesful otherwise false
    virtual void close(void) = 0;
    
    //! @brief returns the data format of the mediodata
    virtual CMedIOData::Format dataFormat() const = 0;

    //! @brief run time typeinformation  
    //! @return specific class type at runtime
    int userRtti() const;
 
    //! @brief for querying error status information
    //! @return integer value for the specific error reason
    int errorStatus(void) const;

  protected :
    int            m_iErrStatus;  //!< error status variable
};

#endif
