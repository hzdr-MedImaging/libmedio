//! @file CMedIODataFactory.cpp
//! @brief contains the implementation of the class CMedIODataFactory
//! @author Hagen Moelle
//! @date 11/13/2004

#include "CMedIODataFactory.h"

#include "CECATFile.h"
#include "CMedIOData.h"
#include "CConcordeFile.h"
#include "CConcordeSinogram.h"
#include "CConcordeImage.h"

//  Class: CMedIODataFactory
//  method: makeMedIOData(QString)
//!
//! This method tries to identify the medical data format of the given file.
//! So it allows an easy handling of files with the medio library.
//! 
//! @param file: filename where medical data is located
//! @return CMedIOData object if format is known otherwise NULL
////////////////////////////////////////////////////////////////////////////////
CMedIOData* CMedIODataFactory::makeMedIOData(QString file)
{
	//test all available formats
	//return fitting format
	//return NULL if unknown format
	
	//test if its a concorde format -> return format_type
	if(CConcordeFile::isoftype(file) == CConcordeFile::ConcordeMicropet_Sinogram)
		return new CConcordeSinogram(file);
	else if(CConcordeFile::isoftype(file) == CConcordeFile::ConcordeMicropet_Image)
		return new CConcordeImage(file);
	else if(CECATFile::isoftype(file) != CECATFile::Undefined)
	{
		CECATFile* tmp = new CECATFile(file);
		tmp->load();
		return tmp;
	}
	else
		return NULL;
}

//  Class: CMedIODataFactory
//  method: makeMedIOData(CMedIOData::Format,QString)
//!
//! This method verifies the medical data format of the given file before
//! constructing the CMedIOData object.
//! So it allows an easy handling of files with the medio library.
//! 
//! @param format: fileformat of file
//! @param file: filename where medical data is located
//! @return CMedIOData object if verification is true otherwise NULL
////////////////////////////////////////////////////////////////////////////////
CMedIOData* CMedIODataFactory::makeMedIOData(CMedIOData::Format format, QString file)
{
	/*
	if(format == CMedIOData::ConcordeMicropet_Sinogram)
	{
		return new CConcordeSinogram(file);
	}
	else
		return NULL;
	*/
	return NULL;
}
