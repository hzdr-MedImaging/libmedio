#include "CConcordeSinogram.h"
#include "CMedIOHeader.h"
#include "CHeaderConcorde.h"
#include "CMedIOData.h"
#include "../include/debug.h"

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
	if(CMedIOData::getHeader()->rtti() == CMedIOHeader::ConcordeMicropet)
	{
		CHeaderConcorde* head = (CHeaderConcorde*)CMedIOData::getHeader();
		
		QFile pfile(CMedIOData::getFile());
		if(pfile.open(IO_ReadOnly))
		{
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
			data_bytes->setRawData(data,head->getFrameSize()/2);
		
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
