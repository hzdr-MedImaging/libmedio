#include "CConcordeFile.h"
#include "CHeaderConcorde.h"
#include "CMedIOData.h"
#include "debug.h"

#include <iostream>

using namespace std;

CConcordeFile::CConcordeFile(QString file):CMedIOData(file)
{
	//simply call load 
	//in future load method maybe deleted
	load();
}

CConcordeFile::~CConcordeFile()
{
	ENTER();
	LEAVE();
}

bool CConcordeFile::load()
{
	//initalise and load header
	CMedIOData::setHeader(new CHeaderConcorde(CMedIOData::getFile() + ".hdr")); 
	
	
	return true;
}

bool CConcordeFile::save()
{
	return true;
}

CMedIOData::Format CConcordeFile::isoftype(QString file)
{
	//try to initialise the header 
	//check if it is a CConcordeFile
	//return the specific type
	//return CData::Unknwon - if it is not a concorde type 

        // try to get Headerinfo on Sinogramfile
        QString hfile(file+".hdr");
        
	CHeaderConcorde head(file+".hdr");

        if(head.getModel() == "2000")
        {
                cout << "File is from Concorde" << endl;
		//file type = 2 -> Sinogram|Normalisation
		//file type = 3 -> Attenuationfile
		// since attenuationfile is a sinogram we could define it as one 
		if(head.getFileType() == "2")
			return CMedIOData::ConcordeMicropet_Sinogram;
		else 
			return CMedIOData::Unknown;
        }
        else
        {
		cout << "File is not from Concorde" << endl;
		return CMedIOData::Unknown;
	}
}
