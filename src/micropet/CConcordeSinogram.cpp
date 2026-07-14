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

//! @file CConcordeSinogram.cpp
//! @brief contains the implementation of the class CConcordeSinogram
//! @date 11/13/2004

#include "CConcordeSinogram.h"
#include "CConcordeMainHeaderSinogram.h"
#include "CMedIOHeader.h"
#include "CMedIOData.h"

#include <rtdebug.h>
#include <QFile>

//  Class: CConcordeSinogram
//  Constructor: CConcordeSinogram
//!
//! constructs a CConcordeSinogram object
//!
////////////////////////////////////////////////////////////////////////////////
CConcordeSinogram::CConcordeSinogram(const QString& fileName)
  : CConcordeFile(fileName)
{
  ENTER();
  LEAVE();
}

//  Class: CConcordeSinogram
//  Destructor:  CConcordeSinogram
//!
//! destructucts a CConcordeSinogram object.
//! 
////////////////////////////////////////////////////////////////////////////////
CConcordeSinogram::~CConcordeSinogram()
{
  ENTER();
  LEAVE();
}

//  Class: CConcordeSinogram
//  Method: open
//!
//! This method initalises and loads the header of the concorde microPET medical data.
//!
//! @return true if successful otherwise false
////////////////////////////////////////////////////////////////////////////////
bool CConcordeSinogram::open(QIODevice::OpenModeFlag mode)
{
  ENTER();
  bool result = false;

  if(isOpen())
  {
    W("File is already opened");
    result = false;
  }
  else
  {
    //initalise and load header
    D("Creating headerobject");
    m_pCachedMainHeader = new CConcordeMainHeaderSinogram(this);
    D("Loading header information");
    result = m_pCachedMainHeader->load();

    if(result)
      if((result = QFile::open(mode)) == false)
        QFile::close();
  }
  RETURN(result);
  return result;
}

CConcordeFile::FileType CConcordeSinogram::fileType() const
{ 
  return CConcordeFile::ConcordeMicropet_Sinogram;
}
