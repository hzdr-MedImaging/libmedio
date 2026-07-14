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

//! @file CConcordeSinogram.h
//! @date 11/13/2004
//! @brief contains definition of the class CConcordeSinogram

#ifndef CCONCORDESINOGRAM_H
#define CCONCORDESINOGRAM_H

#include <QString>

#ifndef __MEDIO_PRIVATE__
#include <CConcordeFile>
#else
#include <CConcordeFile.h>
#endif

//! @class CConcordeSinogram
//! @brief class which handles Concorde microPET sinograms
//! @bug no bugs reported
//! @warning still in development
//!
//! This class provides access to all kind of microPET sinograms.
//! 
class CConcordeSinogram : public CConcordeFile
{
  public :
    //! @brief constructor
    //! @param file: complete path to file holding concorde sinogram
    CConcordeSinogram(const QString& file);
    
    //! @brief destructor
    ~CConcordeSinogram();
    
    //! @brief run time typeinformation  
    //! @return specific class type at runtime
    CConcordeFile::FileType fileType() const;

    //! @brief loads a given file
    //! @return true if loading is succesful otherwise false
    bool open(QIODevice::OpenModeFlag mode);
};

#endif
