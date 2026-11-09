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

#include <QJsonObject>
#include <QJsonValue>
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
class CMedIOHeader {
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
      PhilipsListviewHeader,        //!< listview header of philips scanners
      NIFTIMainHeader               //!< main header of NIfTI1/2 data format
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

    //---------------------------------------------------------------------------------------
    // Format independent metadata (see CMedIOMetadata.h): a JSON object with
    //
    // - the metadata keys (DICOM keywords, e.g. "PatientWeight" for the DICOM
    //   attribute (0010,1030)) with the values of this header, converted into
    //   the units of DICOM,
    // - the native fields of the format in a section named after the format,
    //   e.g. "ECAT7": {"Patient_Name": ..., ...} (without the fields describing
    //   the size, data type, scaling, geometry or layout of the image data),
    // - metadata the format cannot store: keys of other formats, unknown keys
    //   and values the format cannot represent. They are kept in the header
    //   object (also by copies), carried along by conversions between the
    //   formats (CMedIOData::readMainHeader(header, format), readMatrix(...))
    //   and stored by formats that can store any metadata (NIfTI: JSON header
    //   extension); the other formats leave them out when saving.
    //
    // Example (main header of an ECAT7 file):
    //   { "PatientWeight": 8.63, "RadionuclideTotalDose": 60780000,
    //     "AcquisitionDateTime": "2026-06-03T17:04:56Z", ...,
    //     "ECAT7": { "Patient_Weight": 8.63, "Dosage": 60780000, ... } }

    //! @brief metadata of the header (see above)
    QJsonObject metadata() const;

    //! @brief change metadata
    //! @param changes keys and values to change: metadata keys (keyword or tag
    //!        string "(0010,1030)"), native fields in the section of the format,
    //!        any other key; null removes a key (clears the field)
    //! @return false (and nothing changed) if a value is invalid for its key or
    //!         a native field does not exist or does not fit
    //!
    //! Only values that differ from metadata() are applied, so that the object
    //! returned by metadata() can be changed and given back as a whole; if a
    //! metadata key and the native field storing it are both changed, the
    //! metadata key wins.
    bool updateMetadata(const QJsonObject& changes);

    //! @brief value of a metadata key: keyword ("PatientWeight") or tag
    //!        string ("(0010,1030)"); undefined if not set
    QJsonValue metadataValue(const QString& keywordOrTag) const;

    //! @brief value of a metadata key given by its tag, e.g. 0x00101030 = (0010,1030)
    QJsonValue metadataValue(quint32 tag) const;

    //! @brief set (or with null remove) the value of a metadata key
    bool setMetadataValue(const QString& keywordOrTag, const QJsonValue& value);
    bool setMetadataValue(quint32 tag, const QJsonValue& value);

  protected:
    //! @return true if header is a main header
    bool isMainHeader(void) const;

    //! @brief copy the metadata kept in the header object (not stored in the
    //!        fields of the format), for the assignment operators
    void copyMetadata(const CMedIOHeader& src);

  private:
    CMedIOHeaderPrivate* m_pData; //!< private data object carrying all data
};

#endif
