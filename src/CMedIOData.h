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

//! @file CMedIOData.h
//! @author Hagen Moelle
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
      PhilipsBin
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
