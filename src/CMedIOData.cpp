//! @file CMedIOData.cpp
//! @brief contains the implementation of the class CMedIOData
//! @author Hagen Moelle
//! @date 11/13/2004

#include "CMedIOData.h"
#include "debug.h"

#include <qstring.h>

//  Class: CMedIOData
//  Constructor: CMedIOData
//!
//! constructs a CMedIOData object
//!
////////////////////////////////////////////////////////////////////////////////
CMedIOData::CMedIOData(QString file)
{
	File = file;
	Header = NULL;
}

//  Class: CMedIOData
//  Destructor:  CMedIOData
//!
//! destructucts a CMedIOData object.
//! 
////////////////////////////////////////////////////////////////////////////////
CMedIOData::~CMedIOData()
{
	ENTER();
	delete Header;
	LEAVE();
}

//  Class: CMedIOData
//  Method: header
//!
//! This method will return the header of the medical data. So all information
//! of the data is accessable.
//!
//! @return header of medical data
////////////////////////////////////////////////////////////////////////////////
CMedIOHeader* CMedIOData::header()
{
	return Header;
}
