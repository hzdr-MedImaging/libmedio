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

//! @file CConcordeFile.h
//! @brief contains definition of the class CConcordeFile

#ifndef CCONCORDEFILE_H
#define CCONCORDEFILE_H

#include <QString>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOData>
#else
#include <CMedIOData.h>
#endif

// forward declarations
class CConcordeMainHeader;
class CConcordeFrameHeader;
class CConcordeFilePrivate;

//! @class CConcordeFile
//! @brief class to handle all file produced by a concorde microPET scanner
//! @bug no bugs reported
//! @warning still in development
//!
//! This abstract class is a base class for all specific concorde file formats.
//! It handles basic filetype checking.
//! 
class CConcordeFile : public CMedIOData
{
  public :
    enum FileType 
    { 
      Unknown = 0,
      ConcordeMicropet_Image, 
      ConcordeMicropet_Sinogram
    };

    //! @brief constructor
    //! @param filename: complete path to file holding concorde microPET medical data
    CConcordeFile(const QString& filename = QString());

    //! @brief destructor
    virtual ~CConcordeFile();

    // copy constructur and default assignment operator
    CConcordeFile(const CConcordeFile& src);    
    CConcordeFile& operator=(const CConcordeFile& src);
    
    // static methods for the MedIOData interface
    static bool isOfType(const QString& fileName);
    static CMedIOData* createFromFile(const QString& fileName);

    //! @brief loads a given file 
    //! @return true if loading is succesful otherwise false
    //bool open(QIODevice::OpenModeFlag mode);

    //! @brief saves the data  
    //! @return true if saving is succesful otherwise false
    void close();
    
    //! @brief reads the mainheader of the Concorde microPET file
    //! @param mainHeader: Pointer to CHeaderConcorde object where mainheader should be stored to
    //! @return true if reading is successful otherwise false
    bool readMainHeader(CConcordeMainHeader*& mainHeader);

    //! @brief reads the subheader of the Concorde microPET file
    //! @param subHeader: Pointer to CHeaderConcordeFrame object where subheader should be stored to
    //! @param frame: specify the frame - frame starts with 1 to number of frames in study
    //! @return true if reading is successful otherwise false
    bool readSubHeader(CConcordeFrameHeader*& subHeader, int frame);

    //! @brief run time typeinformation  
    //! @return specific class type at runtime
    CMedIOData::Format dataFormat() const;

    virtual CConcordeFile::FileType fileType() const = 0;

    bool readMatrix(QByteArray*& matrixData, short frame);
    bool readMatrix(QByteArray*& matrixData, CConcordeFrameHeader*& subHeader, short frame);
    bool readMatrix(char*& matrixData, unsigned int& length, short frame);
    bool readMatrix(char*& matrixData, unsigned int& length, CConcordeFrameHeader*& subHeader, short frame);

  protected :
    CConcordeMainHeader* m_pCachedMainHeader;

  private:
    CConcordeFilePrivate* m_pData;
};

#endif
