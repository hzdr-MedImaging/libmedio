#include "CMedIOData.h"
#include "CConcordeFile.h"
#include "CConcordeSinogram.h"
#include "CConcordeImage.h"
#include "CMedIODataFactory.h"

CMedIOData* CMedIODataFactory::makeMedIOData(QString file)
{
	//test all available formats
	//return fitting format
	//return NULL if unknown format
	
	//test if its a concorde format -> return format_type
	if(CConcordeFile::isoftype(file) == CMedIOData::ConcordeMicropet_Sinogram)
	{
		return new CConcordeSinogram(file);
	}
	else if(CConcordeFile::isoftype(file) == CMedIOData::ConcordeMicropet_Image)
		return new CConcordeImage(file);
	else
		return NULL;
}

CMedIOData* CMedIODataFactory::makeMedIOData(CMedIOData::Format format, QString file)
{
	if(format == CMedIOData::ConcordeMicropet_Sinogram)
	{
		return new CConcordeSinogram(file);
	}
	else
		return NULL;
}
