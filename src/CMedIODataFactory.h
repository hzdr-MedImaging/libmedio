//! @file CMedIODataFactory.h
//! @author Hagen Moelle
//! @date 11/13/2004
//! @brief contains definition of the class CMedIODataFactory

#ifndef CMEDIODATAFACTORY_H
#define CMEDIODATAFACTORY_H

#include "CMedIOData.h"

#include <qstring.h>

//! @class CMedIODataFactory
//! @brief factory class allowing construction of CMedIOData objects 
//! @bug no bugs reported
//! @warning still in development
//!
//! This class is a factory which allows easy construction of different 
//! CMedIOData objects. All kinds of medical data which are implemented
//! can be instancieted by this class.
class CMedIODataFactory
{
	public :
	//contructors
		
	//destructor
				
	//members
				
	//methods
		//! @brief identifies fileformat and constructs CMedIOData object
		//! @param file: filename where medical data is located
		//! @return CMedIOData object if format is known otherwise NULL
		static CMedIOData* makeMedIOData(QString file);
		//! @brief verifies fileformat and constructs CMedIOData object
		//! @param format: fileformat of file
		//! @param file: filename where medical data is located
		//! @return CMedIOData object if verification is true otherwise NULL
		static CMedIOData* makeMedIOData(CMedIOData::Format format, QString file);
		
	private :
	//members
		
	//methods
};

#endif
