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

//! @file CMedIOHeaderDictionary.h
//! @brief contains the definition of the class CMedIOHeaderDictionary

#ifndef CMEDIOHEADERDICTIONARY_H
#define CMEDIOHEADERDICTIONARY_H

#include <QJsonValue>
#include <QList>
#include <QString>

//! @class CMedIOHeaderDictionary
//! @brief dictionary of the format independent header attributes of libmedio
//!
//! Every header (CMedIOHeader::attributes()) describes its attributes as JSON
//! object. Its keys are the keywords of the DICOM data dictionary (PS3.6),
//! e.g. "PatientWeight" for the DICOM attribute (0010,1030). A key has the
//! meaning and the unit of its DICOM attribute (PS3.3), but a JSON value:
//!
//! | DICOM VR             | JSON value                                         |
//! |----------------------|----------------------------------------------------|
//! | PN, LO, SH, UI, CS,  | string                                             |
//! | AS                   | (PatientAge "045Y")                                |
//! | DS, IS               | number                                             |
//! | DA                   | string "YYYY-MM-DD"                                |
//! | TM                   | string "hh:mm:ss[.ffffff]"                         |
//! | DT                   | string "YYYY-MM-DDThh:mm:ss[.fff][Z or +hh:mm]"    |
//! |                      | (ISO 8601; without zone if the format has none)    |
//! | SQ (code sequence)   | object {"CodeValue", "CodingSchemeDesignator",     |
//! |                      |         "CodingSchemeVersion", "CodeMeaning"}      |
//! | value multiplicity   | array of strings (e.g. CorrectedImage)             |
//! | 1-n                  |                                                    |
//!
//! Attributes that DICOM stores in a sequence item (e.g. RadionuclideTotalDose
//! (0018,1074) in the RadiopharmaceuticalInformationSequence (0054,0016)) are
//! stored without the sequence; entry() gives the tag of the sequence.
//!
//! Besides the keys of this dictionary, the attributes of a header hold:
//! - the native fields of its format in a section named after the format
//!   ("ECAT7", "Philips", "microPET", "NIfTI"), see CMedIOHeader::attributes(),
//! - the attributes of other formats and keys unknown to libmedio, which are
//!   carried along until a format can store them (NIfTI),
//! - "Frames" (FramesKey): the frame attributes of all frames (NIfTI main header).
//!
//! DICOM tags can be given as number (0x00101030) or as string "(0010,1030)";
//! parseTag() also accepts "0010,1030" and "00101030".
class CMedIOHeaderDictionary
{
  public:
    //! @enum Scope header type a key belongs to
    enum Scope { MainHeader = 0,   //!< file or series (main header)
                 FrameHeader       //!< frame (sub header)
               };

    //! @enum ValueType JSON value of a key (see the table above)
    enum ValueType { String = 0,   //!< string (PN, LO, SH, UI, CS, AS)
                     Number,       //!< number (DS, IS)
                     Date,         //!< "YYYY-MM-DD" (DA)
                     Time,         //!< "hh:mm:ss[.ffffff]" (TM)
                     DateTime,     //!< ISO 8601 date and time (DT)
                     Strings,      //!< array of strings (value multiplicity 1-n)
                     Code          //!< code sequence item (object)
                   };

    //! @struct Entry
    //! @brief an entry of the dictionary: an attribute with its DICOM keyword, tag, VR and unit
    struct Entry {
      const char* keyword;     //!< DICOM keyword, e.g. "PatientWeight"
      quint32     tag;         //!< DICOM tag, e.g. 0x00101030 = (0010,1030)
      const char* vr;          //!< DICOM value representation, e.g. "DS"
      const char* vm;          //!< DICOM value multiplicity, "1" or "1-n"
      quint32     sequence;    //!< tag of the sequence holding the attribute, 0 if none
      Scope       scope;       //!< main header or frame
      ValueType   type;        //!< JSON value
      const char* unit;        //!< unit of the value ("" if none), e.g. "kg"
    };

    //! @brief entry of a DICOM keyword ("PatientWeight") or tag ("(0010,1030)")
    //! @return NULL if the dictionary has no entry for it
    static const Entry* entry(const QString& keywordOrTag);

    //! @brief entry of a DICOM tag, e.g. 0x00101030 = (0010,1030)
    static const Entry* entry(quint32 tag);

    //! @brief all entries of the dictionary
    static QList<const Entry*> entries();

    //! @brief key of a keyword or tag in the attributes of a header: the keyword of a known tag
    //!        ("(0010,1030)" -> "PatientWeight"), a known keyword unchanged,
    //!        the tag string of an unknown tag ("00291010" -> "(0029,1010)"),
    //!        any other key unchanged
    static QString normalizedKey(const QString& keywordOrTag);

    //! @brief DICOM tag as string: 0x00101030 -> "(0010,1030)"
    static QString tagString(quint32 tag);

    //! @brief DICOM tag of a string "(0010,1030)", "0010,1030" or "00101030"
    //! @return false if text is not a tag
    static bool parseTag(const QString& text, quint32& tag);

    //! @brief tag of a keyword or tag string, 0 if unknown
    static quint32 tag(const QString& keywordOrTag);

    //! @brief true if value is a valid JSON value of the entry (see the table above)
    static bool isValid(const Entry& entry, const QJsonValue& value);

    //! @brief key of the frame attributes of all frames in a main header ("Frames")
    static const char* const FramesKey;
};

#endif // CMEDIOHEADERDICTIONARY_H
