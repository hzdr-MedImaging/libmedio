//! @file CMedIOData.h
//! @author Hagen Moelle
//! @date 11/13/2004
//! @brief contains definition of the class CMedIOData

#ifndef CMEDIODATA_H
#define CMEDIODATA_H

#include "CMedIOHeader.h"

#include <qstring.h>

//! @class CMedIOData
//! @brief an abstract class which provides an interface for medical data
//! @bug no bugs reported
//! @warning still in development
//!
//! This class is an interface for easier handling of different medical data formats.
//! All data formats which are included in Medical IO Library should use the class
//! as a baseclass.
class CMedIOData
{
	public :
	//contructors
		//! @brief constructor
		//! @param file: complete path to file holding medical data
		CMedIOData(QString file);
	//destructor
		//! @brief destructor
		virtual ~CMedIOData();
		
	//members
		//! @enum enumeration of supported medical data
		enum Format {	Unknown = 0,		//!< data which is not supported
				ConcordeMicropet,	//!< data produced by a concorde micropet scanner	
				ECAT,			//!< ECAT format
				};
		
	//methods
		//! @brief loads a given file 
		//! @return true if loading is succesful otherwise false
		virtual bool load() = 0;
		
		//! @brief saves the data  
		//! @return true if saving is succesful otherwise false
		virtual bool save() = 0;
		
		//! @brief run time typeinformation  
		//! @return specific class type at runtime
		virtual int rtti() = 0;
		
		//virtual int isoftype(QString file) = 0;
		
		//! @brief access to associated header of medical data
		//! @return header of medical data
		CMedIOHeader* header();
		
		//! @brief access to filename
		//! @return filename of medical data file
		QString file() {return File;}

	protected :
		CMedIOHeader* Header;	//!< associated header of medical data
		QString File;		//!< filename of medical data file

	private :
	//members
		
	//methods
};

#endif
