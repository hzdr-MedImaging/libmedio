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

//! @file CInterfileKeys.h
//! @brief the "key := value" lines of an Interfile header
//!
//! An Interfile header (Interfile 3.3, Todd-Pokropek et al., Nucl Med Commun 13
//! (1992) 673-699, and the PET extensions of STIR and the scanner vendors) is ASCII
//! text with one "key := value" pair per line, starting with "!INTERFILE :=" and
//! ending with "!END OF INTERFILE :=":
//!
//!   !INTERFILE :=
//!   !imaging modality := nucmed
//!   ; a comment
//!   !matrix size [1] := 128
//!   scaling factor (mm/pixel) [1] := 2.0
//!   %vendor key := value
//!   !END OF INTERFILE :=
//!
//! - '!' marks a key required by the standard, '%' a key of a vendor (Siemens),
//!   ';' starts a comment line.
//! - Keys are compared without regard to the letter case, '_' and white space
//!   (normalizedName(): "Image_Duration(Sec)" is "image duration (sec)").
//! - Indices are given as "[i]" after the key, also several ("[f][g]").
//! - Lists are written as "{a, b, c}" (toList()).
//! - The headers of Siemens have no "!END OF INTERFILE": the header then ends with
//!   the file (a file holding header and data needs it).
//!
//! CInterfileKeys keeps all lines in their order, also repeated keys (e.g. the
//! keys of every frame group of a dynamic study), unknown keys and comments, so
//! that a header can be written again as it was read (toText()).

#ifndef CINTERFILEKEYS_H
#define CINTERFILEKEYS_H

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

//! @brief one line of an Interfile header
struct CInterfileKey
{
  QString text;        //!< the key as written (with '!'/'%' and indices), or the comment line
  QString name;        //!< the normalized name without indices (see CInterfileKeys::normalizedName())
  QList<int> indices;  //!< the indices given after the key ("[1][2]": 1, 2)
  QString value;       //!< the value without leading and trailing white space
  bool required;       //!< '!' before the key
  bool vendor;         //!< '%' before the key
  bool comment;        //!< a comment line (only text is set)

  CInterfileKey() : required(false), vendor(false), comment(false) {}
};

//! @brief the ordered "key := value" lines of an Interfile header
class CInterfileKeys
{
  public:
    //! @brief the normalized name of a key: in lower case, without '!', '%' and
    //!        indices, '_' and tabs as spaces, single spaces, one space before '('
    //! @param key the key as written, e.g. "!Matrix_Size [1]"
    //! @param indices if given, receives the indices ("[1][2]": 1, 2)
    //! @param required if given, receives whether '!' is given
    //! @param vendor if given, receives whether '%' is given
    //! @return e.g. "matrix size"
    static QString normalizedName(const QString& key, QList<int>* indices = 0,
                                  bool* required = 0, bool* vendor = 0);

    //! @brief parse a header
    //! @param text the header, it may be followed by anything (e.g. the image data
    //!        of a file holding header and data), which is not looked at
    //! @param error if given, receives the reason of a failure
    //! @param headerSize if given, receives the number of bytes up to and including
    //!        the line "!END OF INTERFILE :="
    //! @return false if the text does not start with "!INTERFILE" (after empty and
    //!         comment lines) or holds binary data before "!END OF INTERFILE"; without
    //!         that line the header ends with the text (Siemens), headerSize is its size
    bool parse(const QByteArray& text, QString* error = 0, qint64* headerSize = 0);

    //! @brief read the header of a file (header file or file holding header and data)
    //! @return false if the file cannot be read or has no Interfile header (see parse())
    bool read(const QString& fileName, QString* error = 0, qint64* headerSize = 0);

    //! @brief whether a file starts with an Interfile header ("!INTERFILE")
    static bool isInterfile(const QString& fileName);

    //! @brief the header as text: the lines in their order, "key := value" with the
    //!        keys as written, lines ending with CR LF (as Interfile 3.3 asks for)
    QByteArray toText() const;

    //! @brief remove all lines
    void clear();

    //! @brief number of lines (keys and comments)
    int count() const;

    //! @brief line i (0 <= i < count())
    const CInterfileKey& at(int i) const;

    //! @brief the index of the first line from position from on with the key name
    //!        (compared normalized) and the indices; -1 if there is none
    //! @param indices the indices the line has to have; an empty list matches only
    //!        lines without indices
    int indexOf(const QString& name, const QList<int>& indices = QList<int>(), int from = 0) const;

    //! @brief whether a line with the key name and the indices exists
    bool contains(const QString& name, const QList<int>& indices = QList<int>()) const;

    //! @brief the value of the first line with the key name and the indices
    //! @return defaultValue if there is no such line
    QString value(const QString& name, const QList<int>& indices = QList<int>(),
                  const QString& defaultValue = QString()) const;

    //! @brief append a line "key := value"
    //! @param key the key as it is to be written, e.g. "!matrix size [1]"
    void append(const QString& key, const QString& value);

    //! @brief append a comment line (written as "; text")
    void appendComment(const QString& text);

    //! @brief set the value of the first line with the key (its normalized name
    //!        and indices), or append a line "key := value" if there is none
    void setValue(const QString& key, const QString& value);

    //! @brief remove all lines with the key name and the indices
    //! @return the number of removed lines
    int remove(const QString& name, const QList<int>& indices = QList<int>());

    //! @brief the integer of a value (white space allowed around it)
    static bool toInt(const QString& value, qint64* result);

    //! @brief the floating point number of a value (C locale, white space allowed)
    static bool toDouble(const QString& value, double* result);

    //! @brief the elements of a list value "{a, b, c}" (without white space around
    //!        them), or the value itself as single element if it is no list
    static QStringList toList(const QString& value);

  private:
    QList<CInterfileKey> m_keys;
};

#endif // CINTERFILEKEYS_H
