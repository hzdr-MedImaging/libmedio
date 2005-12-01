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

//! @file CConcordeImage.h
//! @author Hagen Moelle
//! @date 11/13/2004
//! @brief contains definition of the class CConcordeImage

#ifndef CCONCORDEIMAGE_H
#define CCONCORDEIMAGE_H

#include <qstring.h>
#include <qcstring.h>

#ifndef __MEDIO_PRIVATE__
#include <CConcordeFile>
#else
#include <CConcordeFile.h>
#endif

//! @class CConcordeImage
//! @brief class which handles Concorde microPET imagevolumes
//! @bug no bugs reported
//! @warning still in development
//!
//! This class provides access to all kind of microPET imagevolumes.
//! 
class CConcordeImage : public CConcordeFile
{
	public :
	//contructors
		//! @brief constructor
		//! @param file: complete path to file holding concorde imagevolume
		CConcordeImage(QString file);
		
	//destructor
		//! @brief destructor
		~CConcordeImage();
		
	//members
		
	//methods
		//! @brief access to data of an imagevolume 
		//! @return array with data of specific frame in imagevolume or NULL on error
		//QByteArray* getMatrix(short frame, short plane, 
		//	short gate, short bed, short data);
		
		//! @brief run time typeinformation  
		//! @return specific class type at runtime
		int fileType() {return CConcordeFile::ConcordeMicropet_Image;}

		//! @brief loads a given file
                //! @return true if loading is succesful otherwise false
                bool open(int mode);

#ifdef __MEDIO_PRIVATE__
	private :
	//members
		
	//methods
#endif
};

#endif
