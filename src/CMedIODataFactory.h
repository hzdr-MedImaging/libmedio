#ifndef CMEDIODATAFACTORY_H
#define CMEDIODATAFACTORY_H

#include "CMedIOData.h"

class CMedIODataFactory
{
	public :
	//contructors
		
	//destructor
		
		
	//members
		
		
	//methods
		//tries to identify format and type of data (sinogram, ...)
		//of the given file
		static CMedIOData* makeMedIOData(QString file);
		static CMedIOData* makeMedIOData(CMedIOData::Format format, QString file);
		
	private :
	//members
		
	//methods
};

#endif
