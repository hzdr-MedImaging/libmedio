#include "CConcordeSinogram.h"
#include "CMedIOHeader.h"
#include "CHeaderConcorde.h"
#include "CMedIOData.h"
#include "debug.h"

#include <qdatastream.h>
#include <qfile.h>

CConcordeSinogram::CConcordeSinogram(QString file):CConcordeFile(file)
{
	ENTER();
	LEAVE();
}

CConcordeSinogram::~CConcordeSinogram()
{
	ENTER();
	LEAVE();
}

QByteArray* CConcordeSinogram::get(short frame, short plane, 
			short gate, short bed, short data)
{
	ENTER();
	//check if header is from Concorde
	if(CMedIOData::getHeader()->rtti() == CMedIOHeader::ConcordeMicropet)
	{
		CHeaderConcorde* head = (CHeaderConcorde*)CMedIOData::getHeader();
		
		//create new filepointer to datafile
		QFile pfile(CMedIOData::getFile());
		
		//check on open file success
		if(pfile.open(IO_ReadOnly))
		{
			//check if desired frame exists
			int totalframes = atoi(head->getTotalFrames().c_str());
			if(frame > totalframes)
			{
				//throw exception if frame out of range
				return NULL;
			}
			
			//set readpointer to appropriate frame
			pfile.at((frame-1)*head->getFrameSize());
			
			//create new datastream
			QDataStream data_stream(&pfile);
		
			//get byte order from header and set it in datastream
			if(head->getDataType() == "0")
			{
				return NULL;
			}
			
			if((head->getDataType()=="2" |
				head->getDataType()=="3") |
				head->getDataType()=="4")
			{
				data_stream.setByteOrder(QDataStream::LittleEndian);
			}
			else
			{
				data_stream.setByteOrder(QDataStream::BigEndian);
			}
		
			char * data = new char[head->getFrameSize()];
			data_stream.readRawBytes(data,head->getFrameSize());
			
			//close file
			pfile.close();
			
			QByteArray* data_bytes = new QByteArray();
			data_bytes->setRawData(data,head->getFrameSize());
		
			return data_bytes;
		}
		else
			return NULL;
	}
	else 
		return NULL;
}

CMedIOData::Format CConcordeSinogram::rtti()
{
	return CMedIOData::ConcordeMicropet_Sinogram;
}
