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

//! @file CConcordeFile.cpp
//! @brief contains the implementation of the class CConcordeFile
//! @author Hagen Moelle

#include "CConcordeFile.h"
#include "CHeaderConcorde.h"
#include "CMedIOData.h"
#include "debug.h"

//  Class: CConcordeFile
//  Constructor: CConcordeFile
//!
//! constructs a CConcordeFile object
//!
////////////////////////////////////////////////////////////////////////////////
CConcordeFile::CConcordeFile(const QString& fileName)
	: CMedIOData(fileName)
{
	ENTER();
	LEAVE();
}

//  Class: CConcordeFile
//  Destructor:  CConcordeFile
//!
//! destructucts a CConcordeFile object.
//! 
////////////////////////////////////////////////////////////////////////////////
CConcordeFile::~CConcordeFile()
{
	ENTER();
	LEAVE();
}

//  Class: CConcordeFile
//  Method: load
//!
//! This method initalises and loads the header of the concorde microPET medical data.
//!
//! @return true if successful otherwise false
////////////////////////////////////////////////////////////////////////////////
bool CConcordeFile::load()
{
	//initalise and load header
	Header = new CHeaderConcorde(File + ".hdr"); 
	return true;
}

//  Class: CConcordeFile
//  Method: save
//!
//! This method saves the data
//!
//! @return true if successful otherwise false
////////////////////////////////////////////////////////////////////////////////
bool CConcordeFile::save()
{
	return true;
}

//  Class: CConcordeFile
//  Method: isoftype
//!
//! This method tries to identify the format of the given file.
//! 
//! @return type of file or unknown if unknown or error
////////////////////////////////////////////////////////////////////////////////
int CConcordeFile::isoftype(QString file)
{
	//try to initialise the header 
	//check if it is a CConcordeFile
	//return the specific type
	//return CData::Unknwon - if it is not a concorde type 

        // try to get Headerinfo on Sinogramfile
        QString hfile(file+".hdr");
        
	CHeaderConcorde head(file+".hdr");

        if(head.model() == 2000)
        {
                D("file is from concorde");
		//file type = 2 -> Sinogram
		//file type = 3 -> Normalisation
		//file type = 4 -> Attenuation (transmission)
		//file type = 5 -> Image
		//file type = 8 -> Mu map ( also image )
		// since attenuationfile/Normalisation is a sinogram we could define it as one 
		if((head.filetype() == 2 || head.filetype() == 3)
			|| head.filetype() == 4)
		{
			D("file is a sinogram");
			return CConcordeFile::ConcordeMicropet_Sinogram;
		}
		else if(head.filetype() == 5 || head.filetype() == 8)
		{
			D("file is a image");
			return CConcordeFile::ConcordeMicropet_Image;
		}
		else
			return CConcordeFile::Unknown;
        }
        else
        {
		D("File is not from Concorde");
		return CMedIOData::Unknown;
	}
}
