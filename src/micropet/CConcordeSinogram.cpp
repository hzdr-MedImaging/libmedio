/* -*- mode: c++; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 * vim:set ts=2 sw=2 expandtab: *********************************************
 *
 * libmedio - C++ I/O Library for loading/saving medical data formats
 * Copyright (C) 2006-2010 by Jens Langner <Jens.Langner@light-speed.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id$
 *
 **************************************************************************/

//! @file CConcordeSinogram.cpp
//! @brief contains the implementation of the class CConcordeSinogram
//! @author Hagen Moelle
//! @date 11/13/2004

#include "CConcordeSinogram.h"
#include "CConcordeMainHeaderSinogram.h"
#include "CMedIOHeader.h"
#include "CMedIOData.h"

#include <rtdebug.h>
#include <QDataStream>
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

int CConcordeSinogram::fileType() const
{ 
  return CConcordeFile::ConcordeMicropet_Sinogram;
}
