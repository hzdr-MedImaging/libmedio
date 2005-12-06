//! @file CConcordeImage.cpp
//! @brief contains the implementation of the class CConcordeImage
//! @author Hagen Moelle
//! @date 11/13/2004

#include "CConcordeImage.h"
#include "CMedIOHeader.h"
#include "CConcordeMainHeaderImage.h"
#include "CMedIOData.h"
#include <rtdebug.h>

#include <qdatastream.h>
#include <qfile.h>

//  Class: CConcordeImage
//  Constructor: CConcordeImage
//!
//! constructs a CConcordeImage object
//!
////////////////////////////////////////////////////////////////////////////////
CConcordeImage::CConcordeImage(const QString& fileName)
	: CConcordeFile(fileName)
{
	ENTER();
	LEAVE();
}

//  Class: CConcordeImage
//  Destructor:  CConcordeImage
//!
//! destructucts a CConcordeImage object.
//! 
////////////////////////////////////////////////////////////////////////////////
CConcordeImage::~CConcordeImage()
{
	ENTER();
	LEAVE();
}

//  Class: CConcordeImage
//  Method: open
//!
//! This method initalises and loads the header of the concorde microPET medical data.
//!
//! @return true if successful otherwise false
////////////////////////////////////////////////////////////////////////////////
bool CConcordeImage::open(int mode)
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
		m_pCachedMainHeader = new CConcordeMainHeaderImage(this);
		D("Loading header information");
		result = m_pCachedMainHeader->load();

		if(result)
			if((result = QFile::open(mode)) == false)
							QFile::close();
	}
	RETURN(result);
	return result;
}

int CConcordeImage::fileType() const
{
	return CConcordeFile::ConcordeMicropet_Image;
}
