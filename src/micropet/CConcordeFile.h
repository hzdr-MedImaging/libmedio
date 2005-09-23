/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004 by Jens Langner <Jens.Langner@light-speed.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

//! @file CConcordeFile.h
//! @author Hagen Moelle
//! @brief contains definition of the class CConcordeFile

#ifndef CCONCORDEFILE_H
#define CCONCORDEFILE_H

#include "CMedIOData.h"

#include <qstring.h>

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
		enum FileType { Unknown = 0,
										ConcordeMicropet_Image, 
										ConcordeMicropet_Sinogram
									};

		//! @brief constructor
		//! @param file: complete path to file holding concorde microPET medical data
		CConcordeFile(const QString& filename);

		//! @brief destructor
		virtual ~CConcordeFile();

		// static methods for the MedIOData interface
		static bool isOfType(const QString& fileName);
		static CMedIOData* createFromFile(const QString& fileName);

		//! @brief loads a given file 
		//! @return true if loading is succesful otherwise false
		bool open(int mode);

		//! @brief saves the data  
		//! @return true if saving is succesful otherwise false
		void close();

		//! @brief run time typeinformation  
		//! @return specific class type at runtime
		int rtti() const { return CMedIOData::ConcordeMicropet; }

		virtual int fileType() = 0;

		//! @brief checks format of file  
		//! @return specific format or Unknown if unknown or error
		static int isoftype(QString file);
		
	private :
	//members
		
	//methods
};

#endif
