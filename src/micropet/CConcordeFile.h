//! @file CConcordeFile.h
//! @author Hagen Moelle
//! @date 11/13/2004
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
	//contructors
		//! @brief constructor
		//! @param file: complete path to file holding concorde microPET medical data
		CConcordeFile(QString file);

	//destructor
		//! @brief destructor
		virtual ~CConcordeFile();

	//members	
		enum FileType { Unknown = 0, ConcordeMicropet_Image, ConcordeMicropet_Sinogram};

	//methods
		//! @brief loads a given file 
		//! @return true if loading is succesful otherwise false
		bool load();

		//! @brief saves the data  
		//! @return true if saving is succesful otherwise false
		bool save();

		//! @brief run time typeinformation  
		//! @return specific class type at runtime
		int rtti() {return CMedIOData::ConcordeMicropet;}

		virtual int fileType() = 0;

		//! @brief checks format of file  
		//! @return specific format or Unknown if unknown or error
		static int isoftype(QString file);
		
	private :
	//members
		
	//methods
};

#endif
