/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio
 
  Copyright (C) 2004-2025 hzdr.de and contributors
 
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

#ifndef CECAT6MAINHEADER_H
#define CECAT6MAINHEADER_H

#include <QDataStream>
#include <QTextStream>

#ifndef __MEDIO_PRIVATE__
#include <CECATMainHeader>
#else
#include <CECATMainHeader.h>
#endif

// forward declarations
class CECAT6MainHeaderPrivate;
class CECATFile;
class CMedIOHeader;

class CECAT6MainHeader : public CECATMainHeader
{
  public:
    enum File_Type { Unknown=0 }; // FIXME: Others are missing for the moment

    // constructors
    CECAT6MainHeader(CECATFile* ecatFile = NULL,
                     CECATMainHeader::Type fileType = CECATMainHeader::Unknown);
    ~CECAT6MainHeader();

    // copy constructur and default assignment operator
    CECAT6MainHeader(const CECAT6MainHeader& src);    
    CECAT6MainHeader& operator=(const CECAT6MainHeader& src);
    
    // header clear method
    void clear();

    // public methods
    bool load(void);
    bool save(void) const;

    // the number of bytes the data of that header requires on disk
    int rawDataSize() const;

    // runtime type information methods
    CECATMainHeader::HeaderType mainHeaderType() const;

    // clone methods
    CMedIOHeader* clone() const;

    // conversion methods
    bool convertFrom(const CMedIOHeader* mainHeader, const CMedIOHeader* subHeader=NULL);

    // accessor Methods
    const char* original_File_Name(void) const;
    short sw_Version(void) const;
    short system_Type(void) const;
    File_Type file_Type(void)  const;
    short num_Planes(void) const;
    short num_Frames(void) const;
    short num_Gates(void) const;
    short num_Bed_Pos(void) const;

    // mutator methods
    void setOriginal_File_Name(const char* name);
    void setSW_Version(const short ver);
    void setSystem_Type(const short type);
    void setFileType(const File_Type fType);
    void setNum_Planes(const short num);
    void setNum_Frames(const short num);
    void setNum_Gates(const short num);
    void setNum_Bed_Pos(const short num);

  private:
    CECAT6MainHeaderPrivate* m_pData;
};

#endif // CECAT6MAINHEADER_H
