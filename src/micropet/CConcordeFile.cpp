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
	setHeader(new CHeaderConcorde(file() + ".hdr")); 
	
	
	return true;
}

bool CConcordeFile::save()
{
	return true;
}

int CConcordeFile::isoftype(QString file)
{
	//try to initialise the header 
	//check if it is a CConcordeFile
	//return the specific type
	//return CData::Unknwon - if it is not a concorde type 

        // try to get Headerinfo on Sinogramfile
        QString hfile(file+".hdr");
        
	CHeaderConcorde head(file+".hdr");

        if(head.model() == 2000)
        {
                D("File is from Concorde");
		//file type = 2 -> Sinogram
		//file type = 3 -> Normalisation
		//file type = 4 -> Attenuation (transmission)
		//file type = 5 -> Image
		//file type = 8 -> Mu map ( also image )
		// since attenuationfile/Normalisation is a sinogram we could define it as one 
		if((head.filetype() == 2 || head.filetype() == 3)
			|| head.filetype() == 4)
			return CMedIOData::ConcordeMicropet_Sinogram;
		else if(head.filetype() == 5 || head.filetype() == 8)
			return CMedIOData::ConcordeMicropet_Image;
		else
			return CMedIOData::Unknown;
        }
        else
        {
		D("File is not from Concorde");
		return CMedIOData::Unknown;
	}
}
