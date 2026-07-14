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

#ifndef CPHILIPSLISTVIEWHEADER_H
#define CPHILIPSLISTVIEWHEADER_H

#include <QString>

// forward declarations
class CPhilipsListviewHeaderPrivate;
class CPhilipsFile;

class CPhilipsListviewHeader
{
  public:
    enum File_Format
    {
      Unknown = 0,
      LOR4 = 14,
      LOR8 = 15,
      Xtal = 16
    };

    explicit CPhilipsListviewHeader(CPhilipsFile* file);
    ~CPhilipsListviewHeader();

    // header clear method
    void clear();

    // file i/o methods
    bool load();
    bool save() const;

    int numberOfEventsPerBuffer() const;
    QString magicNumber() const;
    File_Format listModeFormat() const;
    bool singlesMode() const;
    bool timestampsStoredInAUX() const;
    short mainHeaderSize() const;
    short subHeaderSize() const;
    QString comment() const;

    void setNumberOfEventsPerBuffer(const int n);
    void setMagicNumber(const QString& magicNumber);
    void setFileFormat(const File_Format format);
    void setSinglesMode(const bool flag);
    void setTimestampsStoredInAUX(const bool flag);
    void setMainHeaderSize(const short size);
    void setSubHeaderSize(const short size);
    void setComment(const QString& comment);

  private:
    CPhilipsListviewHeaderPrivate* m_pData;
};

#endif // CPHILIPSLISTVIEWHEADER_H
