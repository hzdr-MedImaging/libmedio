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

//! @file CMedIOHeader.h
//! @brief contains definition of the class CMedIOHeader

#ifndef CMEDIOHEADER_H
#define CMEDIOHEADER_H

#include <QString>

// forward declarations
class CMedIOData;
class CMedIOHeaderPrivate;

//! @class CMedIOHeader
//! @brief an abstract class which provides an interface for header information of medical data
//! @bug no bugs reported
//! @warning still in development
//!
//! This class is an interface for easier handling of different medical data headers.
//! All headers of data formats which are included in Medical IO Library should use the class
//! as a baseclass.
class CMedIOHeader
{
  public:
    enum Format 
    {  
      Unknown = 0,
      ConcordeMicroPetMainHeader,   //!< main header of concorde microPET scanner
      ConcordeMicroPetFrameHeader,  //!< frame header of concorde microPET scanner
      ECATMainHeader,               //!< main header of ECAT6/7 PET scanners
      ECATSubHeader,                //!< sub header of ECAT6/7 PET scanners
      PhilipsMainHeader,            //!< main header of philips scanners
      PhilipsSubHeader,             //!< sub header of philips scanners
      PhilipsListviewHeader         //!< listview header of philips scanners
    };

    // constructors
    CMedIOHeader();
    CMedIOHeader(const CMedIOHeader& src);
    CMedIOHeader(CMedIOData* data);
    virtual ~CMedIOHeader();

    //! @brief default assignment operator which enforces that m_pMedIOData
    //!        is NOT copied as well.
    CMedIOHeader& operator=(const CMedIOHeader& src);
    
    //! @brief set the header to a defined state
    virtual void clear() = 0;

    //! @brief loads a given file 
    //! @return true if loading is succesful otherwise false
    virtual bool load() = 0;
    
    //! @brief saves the data  
    //! @return true if saving is succesful otherwise false
    virtual bool save() const = 0;

    //! @brief run time typeinformation  
    //! @return specific header format at runtime
    virtual CMedIOHeader::Format headerFormat() const = 0;

    //! @brief for conversion based on an input file
    virtual bool convertFrom(CMedIOData* pData);

    //! @brief try to convert the given medio header in this header type
    //! @return true for success; false for failure
    virtual bool convertFrom(const CMedIOHeader* header1, const CMedIOHeader* header2=NULL) = 0;

    //! @brief user defined rtti
    //! @return user defined rtti if defined else -1
    virtual int userRtti() const;
    
    //! @brief create a new medio header of the same type as pSrc and
    //!         copy all meta information
    //! @return the new created CMedIOHeader object
    virtual CMedIOHeader* clone() const = 0;

    //! @brief allow to query the mediodata object to which this header
    //!        belongs
    virtual CMedIOData* medIOData(void) const;

    //! @brief provid a facility to set the MedIOData object
    //!        which is in fact used by some subHeader implementation
    virtual void setMedIOData(CMedIOData* data);

  protected:
    //! @return true if header is a main header
    bool isMainHeader(void) const;

  private:
    CMedIOHeaderPrivate* m_pData; //!< private data object carrying all data
};

#endif
