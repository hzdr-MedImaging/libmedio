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

//! @file CConcordeSinogram.h
//! @author Hagen Moelle
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
