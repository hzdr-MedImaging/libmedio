#include "CConcordeImage.h"
#include "CMedIOHeader.h"
#include "CHeaderConcorde.h"
#include "CMedIOData.h"
#include "debug.h"

#include <qdatastream.h>
#include <qfile.h>

CConcordeImage::CConcordeImage(QString file):CConcordeFile(file)
{
	ENTER();
	LEAVE();
}

CConcordeImage::~CConcordeImage()
{
	ENTER();
	LEAVE();
}

QByteArray* CConcordeImage::get(short frame, short plane, 
			short gate, short bed, short data)
{
	
	ENTER();
	//check if header is from Concorde
	if(CMedIOData::getHeader()->rtti() == CMedIOHeader::ConcordeMicropet)
	{
		CHeaderConcorde* head = (CHeaderConcorde*)CMedIOData::getHeader();
		
		//create new filepointer to datafile
		QFile pfile(CMedIOData::getFile());
		unsigned int framesize = head->getImageFrameSize();
		char * data = new char[framesize];

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
			pfile.at((frame-1)*framesize);
			
			//create new datastream
			QDataStream stream(&pfile);
		
			//get byte order from header and set it in datastream
			if(head->getDataType() == "0")
			{
				W("No or an unknown data type");
				stream.readRawBytes(data, framesize);	
			}

			if(head->getDataType() == "1")
			{
				// we use the RawBytes() method here.
				stream.readRawBytes(data, framesize);
			}

			// 2 is a little endian short value, so we
			// need to set the stream to little endian, read the data via the
			// QDataStream operators to ensure correct byte swapping
			if(head->getDataType() == "2")
			{
				stream.setByteOrder(QDataStream::LittleEndian);
				Q_UINT16* ptr = (Q_UINT16*)data;
				for(unsigned int i=0; i < framesize; i+=sizeof(Q_UINT16))
				{
					stream >> *ptr;
					++ptr;
				}
				stream.setByteOrder(QDataStream::BigEndian);
			}

			// 3 is a little endian 4byte integer value, so we
			// need to set the stream to little endian, read the data via the
			// QDataStream operators to ensure correct byte swapping
			if(head->getDataType() == "3")
			{
				D("DataType is : little endian integer");
				D("FrameSize : %d", framesize);
				stream.setByteOrder(QDataStream::LittleEndian);
				Q_UINT32* ptr = (Q_UINT32*)data;
				for(unsigned int i=0; i < framesize; i+=sizeof(Q_UINT32))
				{
					stream >> *ptr;
					++ptr;
				}
				stream.setByteOrder(QDataStream::BigEndian);
			}
			
			// 4 is a little endian float value, so we
			// need to set the stream to little endian, read the data via the
			// QDataStream operators to ensure correct byte swapping			
			if(head->getDataType() == "4")
			{
				D("DataType is : little endian float");
				D("FrameSize : %d", framesize);
				stream.setByteOrder(QDataStream::LittleEndian);
				float* ptr = (float*)data;
				for(unsigned int i=0; i < framesize; i+=sizeof(float))
				{
					stream >> *ptr;
					++ptr;
				}
				stream.setByteOrder(QDataStream::BigEndian);
			}

			// 5 is defined to be a big endian float value. As the QDataStream
			// is per default big endian, we don't have to set it to another byte order
			// and just use the QDataStream operators to ensure correct byte swapping
			if(head->getDataType() == "5")
			{
				float* ptr = (float*)data;
				for(unsigned int i=0; i < framesize; i+=sizeof(float))
				{
					stream >> *ptr;
					++ptr;
				}
			}

			// 6 is defined to be a big endian short value. As the QDataStream
			// is per default big endian, we don't have to set it to another byte order
			// and just use the QDataStream operators to ensure correct byte swapping
			if(head->getDataType() == "6")
			{
				Q_UINT16* ptr = (Q_UINT16*)data;
				for(unsigned int i=0; i < framesize; i+=sizeof(Q_UINT16))
				{
					stream >> *ptr;
					++ptr;
				}
			}

			// 7 is defined to be a big endian 4byte integer value. As the QDataStream
			// is per default big endian, we don't have to set it to another byte order
			// and just use the QDataStream operators to ensure correct byte swapping
			if(head->getDataType() == "7")
			{
				Q_UINT32* ptr = (Q_UINT32*)data;
				for(unsigned int i=0; i < framesize; i+=sizeof(Q_UINT32))
				{
					stream >> *ptr;
					++ptr;
				}
			}
			
			//close file
			pfile.close();
			
			QByteArray* data_bytes = new QByteArray();
			data_bytes->setRawData(data, framesize);
		
			return data_bytes;
		}
		else
			return NULL;
	}
	else 
		return NULL;
}

CMedIOData::Format CConcordeImage::rtti()
{
	return CMedIOData::ConcordeMicropet_Image;
}
