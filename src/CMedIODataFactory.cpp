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

//! @file CMedIODataFactory.cpp
//! @brief contains the implementation of the class CMedIODataFactory

#include "CMedIODataFactory.h"

#include "CECATFile.h"
#include "CMedIOData.h"
#include "CConcordeFile.h"
#include "CConcordeSinogram.h"
#include "CConcordeImage.h"
#include "CPhilipsFile.h"
#include "CNIFTIFile.h"
#include "CInterfileFile.h"

#include <QFileInfo>

#include <rtdebug.h>

CMedIOData::Format CMedIODataFactory::identify(const QString& fileName)
{
  ENTER();

  CMedIOData::Format result;

  // Interfile before Philips and NIfTI: Interfile uses their file name extensions
  // (.img, .hdr) and is identified by its content
  if(CECATFile::isOfType(fileName))
    result = CMedIOData::ECAT;
  else if(CInterfileFile::isOfType(fileName))
    result = CMedIOData::Interfile;
  else if(CPhilipsFile::isOfType(fileName))
    result = CMedIOData::Philips;
  else if(CNIFTIFile::isOfType(fileName))
    result = CMedIOData::NIFTI;
  else if(CConcordeFile::isOfType(fileName))
    result = CMedIOData::ConcordeMicropet;
  else
    result = CMedIOData::Unknown;  

  RETURN(result);
  return result;
}

//  Class: CMedIODataFactory
//  method: create
//!
//! This method tries to identify the medical data format of the given file.
//! So it allows an easy handling of files with the medio library.
//! 
//! @param file: filename where medical data is located
//! @return CMedIOData object if format is known otherwise NULL
////////////////////////////////////////////////////////////////////////////////
CMedIOData* CMedIODataFactory::create(const QString& fileName) {
  ENTER();
  CMedIOData* mData;

  // here we check for our different main data formats 
  if((mData = CECATFile::createFromFile(fileName)))
    D("ECAT format found");
  else if((mData = CConcordeFile::createFromFile(fileName)))
    D("Concorde microPET format found");
  else if((mData = CInterfileFile::createFromFile(fileName)))
    D("Interfile file found");
  else if((mData = CPhilipsFile::createFromFile(fileName)))
    D("Philips file found");
  else if((mData = CNIFTIFile::createFromFile(fileName)))
  {
    D("NIfTI file found");
  }
  else
  {
    E("Can not identify file format");
    mData = NULL;
  }
  
  RETURN(mData);
  return mData;
}

CMedIOData::Format CMedIODataFactory::formatOfFileName(const QString& fileName)
{
  ENTER();

  const QString name = QFileInfo(fileName).fileName().toLower();
  CMedIOData::Format result = CMedIOData::Unknown;

  if(name.endsWith(".v"))
    result = CMedIOData::ECAT;
  else if(name.endsWith(".nii") || name.endsWith(".nii.gz") || name.endsWith(".hdr") || name.endsWith(".hdr.gz"))
    result = CMedIOData::NIFTI;
  else if(name.endsWith(".img") || name.endsWith(".syn"))
    result = CMedIOData::Philips;
  else if(name.endsWith(".hv"))
    result = CMedIOData::Interfile;

  RETURN(result);
  return result;
}

CMedIOData* CMedIODataFactory::createForWriting(const QString& fileName, CMedIOData::Format format)
{
  ENTER();

  if(format == CMedIOData::Unknown)
    format = formatOfFileName(fileName);

  CMedIOData* result = NULL;
  switch(format)
  {
    case CMedIOData::ECAT:
      result = new CECATFile(fileName, CECATMainHeader::ECAT7_Volume16);
    break;

    case CMedIOData::NIFTI:
      result = new CNIFTIFile(fileName, CNIFTIMainHeader::NIFTI1);
    break;

    case CMedIOData::Philips:
      result = new CPhilipsFile(fileName, fileName.endsWith(".syn", Qt::CaseInsensitive) ? CPhilipsMainHeader::Syntegra
                                                                                          : CPhilipsMainHeader::Image);
    break;

    case CMedIOData::Interfile:
      result = new CInterfileFile(fileName);
    break;

    default:
      E("files of data format %d cannot be written", static_cast<int>(format));
    break;
  }

  RETURN(result);
  return result;
}

const char* CMedIODataFactory::formatName(CMedIOData::Format format)
{
  switch(format)
  {
    case CMedIOData::ECAT:             return "ecat";
    case CMedIOData::ConcordeMicropet: return "micropet";
    case CMedIOData::Philips:          return "philips";
    case CMedIOData::PhilipsBin:       return "philipsbin";
    case CMedIOData::NIFTI:            return "nifti";
    case CMedIOData::Interfile:        return "interfile";
    case CMedIOData::Unknown:          break;
  }
  return "unknown";
}

bool CMedIODataFactory::formatFromName(const QString& name, CMedIOData::Format& format)
{
  ENTER();
  bool result = true;

  if(name.startsWith('e', Qt::CaseInsensitive))
    format = CMedIOData::ECAT;
  else if(name.startsWith('u', Qt::CaseInsensitive))
    format = CMedIOData::Unknown;
  else if(name.startsWith('c', Qt::CaseInsensitive) || name.startsWith('m', Qt::CaseInsensitive))
    format = CMedIOData::ConcordeMicropet;
  else if(name.startsWith("philipsbin", Qt::CaseInsensitive))
    format = CMedIOData::PhilipsBin;
  else if(name.startsWith('p', Qt::CaseInsensitive))
    format = CMedIOData::Philips;
  else if(name.startsWith('n', Qt::CaseInsensitive))
    format = CMedIOData::NIFTI;
  else if(name.startsWith('i', Qt::CaseInsensitive))
    format = CMedIOData::Interfile;
  else
    result = false;

  RETURN(result);
  return result;
}
