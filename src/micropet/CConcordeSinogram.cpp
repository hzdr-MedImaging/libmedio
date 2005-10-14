//! @file CConcordeSinogram.cpp
//! @brief contains the implementation of the class CConcordeSinogram
//! @author Hagen Moelle
//! @date 11/13/2004

#include "CConcordeSinogram.h"
#include "CMedIOHeader.h"
#include "CConcordeMainHeaderSinogram.h"
#include "CMedIOData.h"

#include <rtdebug.h>
#include <qdatastream.h>
#include <qfile.h>

//  Class: CConcordeSinogram
//  Constructor: CConcordeSinogram
//!
//! constructs a CConcordeSinogram object
//!
////////////////////////////////////////////////////////////////////////////////
CConcordeSinogram::CConcordeSinogram(QString file):CConcordeFile(file)
{
	ENTER();
	LEAVE();
}

//  Class: CConcordeSinogram
//  Destructor:  CConcordeSinogram
//!
//! destructucts a CConcordeSinogram object.
//! 
////////////////////////////////////////////////////////////////////////////////
CConcordeSinogram::~CConcordeSinogram()
{
	ENTER();
	LEAVE();
}

//  Class: CConcordeSinogram
//  Method: open
//!
//! This method initalises and loads the header of the concorde microPET medical data.
//!
//! @return true if successful otherwise false
////////////////////////////////////////////////////////////////////////////////
bool CConcordeSinogram::open(int mode)
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
                m_pCachedMainHeader = new CConcordeMainHeaderSinogram(this);
                D("Loading header information");
		result = m_pCachedMainHeader->load();

                if(result)
                        if((result = QFile::open(mode)) == false)
                                QFile::close();
        }
        RETURN(result);
        return result;
}
