//! @file CMedIOHeader.cpp
//! @brief contains the implementation of the class CMedIOHeader
//! @author Hagen Moelle
//! @date 11/13/2004

#include "CMedIOHeader.h"
#include "debug.h"

//  Class: CMedIOHeader
//  Constructor: CMedIOHeader
//!
//! constructs a CMedIOHeader object
//!
//! @param file: filename of headerfile
////////////////////////////////////////////////////////////////////////////////
CMedIOHeader::CMedIOHeader(QString file)
{
	File = file;
}


//  Class: CMedIOHeader
//  Destructor:  CMedIOHeader
//!
//! destructucts a CMedIOHeader object.
//! 
////////////////////////////////////////////////////////////////////////////////
CMedIOHeader::~CMedIOHeader()
{
	ENTER();
	LEAVE();
}
