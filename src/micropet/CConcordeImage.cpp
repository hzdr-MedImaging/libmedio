//! @file CConcordeImage.cpp
//! @brief contains the implementation of the class CConcordeImage
//! @author Hagen Moelle
//! @date 11/13/2004

#include "CConcordeImage.h"
#include "CMedIOHeader.h"
#include "CHeaderConcorde.h"
#include "CMedIOData.h"

#include <QDataStream>
#include <QFile>

#include <rtdebug.h>

//  Class: CConcordeImage
//  Constructor: CConcordeImage
//!
//! constructs a CConcordeImage object
//!
////////////////////////////////////////////////////////////////////////////////
CConcordeImage::CConcordeImage(QString file):CConcordeFile(file)
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
//  Method: getMatrix
//!
//! This method loads the data of a imagevolume from file to memory and constructs
//! a QByteArray which holds the data. The user is responsible for destruction
//! of the object.
//!
//! @return data of specific frame in imagevolume or NULL on error
////////////////////////////////////////////////////////////////////////////////
QByteArray* CConcordeImage::getMatrix(short frame, short plane, 
			short gate, short bed, short data)
{
	
	ENTER();
	//check if header is from Concorde
	if(Header->rtti() == CMedIOHeader::ConcordeMicropet)
	{
		CHeaderConcorde* head = (CHeaderConcorde*)Header;
		
		//create new filepointer to datafile
		QFile pfile(file());
		unsigned int framesize = head->getImageFrameSize();
		char * data = new char[framesize];

		//check on open file success
		if(pfile.open(QIODevice::ReadOnly))
		{
			//check if desired frame exists
			int totalframes = head->numframes();
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
			if(head->datatype() == 0)
			{
				W("No or an unknown data type");
				stream.readRawBytes(data, framesize);	
			}

			if(head->datatype() == 1)
			{
				// we use the RawBytes() method here.
				stream.readRawBytes(data, framesize);
			}

			// 2 is a little endian short value, so we
			// need to set the stream to little endian, read the data via the
			// QDataStream operators to ensure correct byte swapping
			if(head->datatype() == 2)
			{
				stream.setByteOrder(QDataStream::LittleEndian);
				quint16* ptr = (quint16*)data;
				for(unsigned int i=0; i < framesize; i+=sizeof(quint16))
				{
					stream >> *ptr;
					++ptr;
				}
				stream.setByteOrder(QDataStream::BigEndian);
			}

			// 3 is a little endian 4byte integer value, so we
			// need to set the stream to little endian, read the data via the
			// QDataStream operators to ensure correct byte swapping
			if(head->datatype() == 3)
			{
				D("DataType is : little endian integer");
				D("FrameSize : %d", framesize);
				stream.setByteOrder(QDataStream::LittleEndian);
				quint32* ptr = (quint32*)data;
				for(unsigned int i=0; i < framesize; i+=sizeof(quint32))
				{
					stream >> *ptr;
					++ptr;
				}
				stream.setByteOrder(QDataStream::BigEndian);
			}
			
			// 4 is a little endian float value, so we
			// need to set the stream to little endian, read the data via the
			// QDataStream operators to ensure correct byte swapping			
			if(head->datatype() == 4)
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
			if(head->datatype() == 5)
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
			if(head->datatype() == 6)
			{
				quint16* ptr = (quint16*)data;
				for(unsigned int i=0; i < framesize; i+=sizeof(quint16))
				{
					stream >> *ptr;
					++ptr;
				}
			}

			// 7 is defined to be a big endian 4byte integer value. As the QDataStream
			// is per default big endian, we don't have to set it to another byte order
			// and just use the QDataStream operators to ensure correct byte swapping
			if(head->datatype() == 7)
			{
				quint32* ptr = (quint32*)data;
				for(unsigned int i=0; i < framesize; i+=sizeof(quint32))
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
