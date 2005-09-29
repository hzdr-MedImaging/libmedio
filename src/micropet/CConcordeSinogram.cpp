//! @file CConcordeSinogram.cpp
//! @brief contains the implementation of the class CConcordeSinogram
//! @author Hagen Moelle
//! @date 11/13/2004

#include "CConcordeSinogram.h"

#include "CMedIOHeader.h"
#include "CHeaderConcorde.h"
#include "CMedIOData.h"
#include <rtdebug.h>

#include <qdatastream.h>
#include <qfile.h>

#define READ_SIZE 8192

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
//  Method: getMatrix
//!
//! This method loads the data of a sinogram from file to memory and constructs
//! a QByteArray which holds the data. The user is responsible for destruction
//! of the object.
//!
//! @return data of specific frame in sinogram or NULL on error
////////////////////////////////////////////////////////////////////////////////
QByteArray* CConcordeSinogram::getMatrix(short frame)
{
	ENTER();
	//check if header is from Concorde
	if(m_pHeader->headerFormat() == CMedIOHeader::ConcordeMicropet)
	{
		CHeaderConcorde* head = (CHeaderConcorde*)m_pHeader;
		bool result = true;
		//create new filepointer to datafile
		QFile pfile(fileName());
		unsigned int framesize = head->getFrameSize();
		QByteArray* data_bytes = new QByteArray(framesize, 0);
		char * data = data_bytes->data();

		//check on open file success
		if(pfile.open(QIODevice::ReadOnly))
		{
			//check if desired frame exists
			int totalframes = head->numframes();
			if(frame > totalframes)
			{
				//throw exception if frame out of range
				result = false;
			}
			
			//set readpointer to appropriate frame
			pfile.seek((frame-1)*framesize);
			
			//create new datastream
			QDataStream stream(&pfile);
		
			//get byte order from header and set it in datastream
			switch(head->datatype())
			{
				case CHeaderConcorde::UnknownDataType:
				{
					W("No or an unknown data type");
					stream.readRawData(data, framesize);	
				}
				break;
				case CHeaderConcorde::Byte:
				{
					// we use the RawBytes() method here.
					stream.readRawData(data, framesize);
				}
				break;
				// IntelShort is a little endian short value, so we
				// need to set the stream to little endian, read the data via the
				// QDataStream operators to ensure correct byte swapping
				case CHeaderConcorde::IntelShort:
				{
					QByteArray bufArray(READ_SIZE, 0);
					quint16* ptr = (quint16*)data;
					for(unsigned int read=0; read < framesize;)
					{
						unsigned int toRead = framesize-read >= READ_SIZE ? READ_SIZE : framesize-read;
						unsigned int curRead = pfile.read(bufArray.data(), toRead);
						
						if(curRead != toRead)
						{
							result = false;
							break;
						}
						
						// check if the curRead value is divide able through our data type 
						ASSERT(curRead % sizeof(quint16) == 0);
						
						QDataStream bufStream(bufArray);
						bufStream.setByteOrder(QDataStream::LittleEndian);
						for(unsigned int i=0; i < curRead; i+=sizeof(quint16))
						{
							bufStream >> *ptr;
							++ptr;
						}
						bufStream.setByteOrder(QDataStream::BigEndian);

						// increase our read counter
						read += curRead;
					}	
				}
				break;
				// IntelInt is a little endian 4byte integer value, so we
				// need to set the stream to little endian, read the data via the
				// QDataStream operators to ensure correct byte swapping
				case CHeaderConcorde::IntelInt:
				{
					D("DataType is : little endian integer");
					D("FrameSize : %d", framesize);
					QByteArray bufArray(READ_SIZE, 0);
					quint32* ptr = (quint32*)data;
					for(unsigned int read=0; read < framesize;)
					{
						unsigned int toRead = framesize-read >= READ_SIZE ? READ_SIZE : framesize-read;
						unsigned int curRead = pfile.read(bufArray.data(), toRead);
						
						if(curRead != toRead)
						{
							result = false;
							break;
						}
						
						// check if the curRead value is divide able through our data type 
						ASSERT(curRead % sizeof(quint32) == 0);
						
						QDataStream bufStream(bufArray);
						bufStream.setByteOrder(QDataStream::LittleEndian);
						for(unsigned int i=0; i < curRead; i+=sizeof(quint32))
						{
							bufStream >> *ptr;
							++ptr;
						}
						bufStream.setByteOrder(QDataStream::BigEndian);

						// increase our read counter
						read += curRead;
					}
				}
				break;
				// IntelFloat is a little endian float value, so we
				// need to set the stream to little endian, read the data via the
				// QDataStream operators to ensure correct byte swapping			
				case CHeaderConcorde::IntelFloat:
				{
					D("DataType is : little endian float");
					D("FrameSize : %d", framesize);
					QByteArray bufArray(READ_SIZE, 0);
					float* ptr = (float*)data;
					for(unsigned int read=0; read < framesize;)
					{
						unsigned int toRead = framesize-read >= READ_SIZE ? READ_SIZE : framesize-read;
						unsigned int curRead = pfile.read(bufArray.data(), toRead);
						
						if(curRead != toRead)
						{
							result = false;
							break;
						}
						
						// check if the curRead value is divide able through our data type 
						ASSERT(curRead % sizeof(float) == 0);
						
						QDataStream bufStream(bufArray);
						bufStream.setByteOrder(QDataStream::LittleEndian);
						for(unsigned int i=0; i < curRead; i+=sizeof(float))
						{
							bufStream >> *ptr;
							++ptr;
						}
						bufStream.setByteOrder(QDataStream::BigEndian);

						// increase our read counter
						read += curRead;
					}
				}
				break;
				// IEEEFloat is defined to be a big endian float value. As the QDataStream
				// is per default big endian, we don't have to set it to another byte order
				// and just use the QDataStream operators to ensure correct byte swapping
				case CHeaderConcorde::IEEEFloat:
				{
					QByteArray bufArray(READ_SIZE, 0);
					float* ptr = (float*)data;
					for(unsigned int read=0; read < framesize;)
					{
						unsigned int toRead = framesize-read >= READ_SIZE ? READ_SIZE : framesize-read;
						unsigned int curRead = pfile.read(bufArray.data(), toRead);
						
						if(curRead != toRead)
						{
							result = false;
							break;
						}
						
						// check if the curRead value is divide able through our data type 
						ASSERT(curRead % sizeof(float) == 0);
						
						QDataStream bufStream(bufArray);
						for(unsigned int i=0; i < curRead; i+=sizeof(float))
						{
							bufStream >> *ptr;
							++ptr;
						}

						// increase our read counter
						read += curRead;
					}
				}
				break;
				// SunShort is defined to be a big endian short value. As the QDataStream
				// is per default big endian, we don't have to set it to another byte order
				// and just use the QDataStream operators to ensure correct byte swapping
				case CHeaderConcorde::SunShort:
				{
					QByteArray bufArray(READ_SIZE, 0);
					quint16* ptr = (quint16*)data;
					for(unsigned int read=0; read < framesize;)
					{
						unsigned int toRead = framesize-read >= READ_SIZE ? READ_SIZE : framesize-read;
						unsigned int curRead = pfile.read(bufArray.data(), toRead);
						
						if(curRead != toRead)
						{
							result = false;
							break;
						}
						
						// check if the curRead value is divide able through our data type 
						ASSERT(curRead % sizeof(quint16) == 0);
						
						QDataStream bufStream(bufArray);
						for(unsigned int i=0; i < curRead; i+=sizeof(quint16))
						{
							bufStream >> *ptr;
							++ptr;
						}

						// increase our read counter
						read += curRead;
					}
				}
				break;
				// SunInt is defined to be a big endian 4byte integer value. As the QDataStream
				// is per default big endian, we don't have to set it to another byte order
				// and just use the QDataStream operators to ensure correct byte swapping
				case CHeaderConcorde::SunInt:
				{
					QByteArray bufArray(READ_SIZE, 0);
					quint32* ptr = (quint32*)data;
					for(unsigned int read=0; read < framesize;)
					{
						unsigned int toRead = framesize-read >= READ_SIZE ? READ_SIZE : framesize-read;
						unsigned int curRead = pfile.read(bufArray.data(), toRead);
						
						if(curRead != toRead)
						{
							result = false;
							break;
						}
						
						// check if the curRead value is divide able through our data type 
						ASSERT(curRead % sizeof(quint32) == 0);
						
						QDataStream bufStream(bufArray);
						for(unsigned int i=0; i < curRead; i+=sizeof(quint32))
						{
							bufStream >> *ptr;
							++ptr;
						}

						// increase our read counter
						read += curRead;
					}
				}
				break;
			}
			//close file
			pfile.close();
			if(result)
			{
				return data_bytes;
			}
			else
			{
				delete data_bytes;
				return NULL;
			}
		}
		else
			return NULL;
	}
	else 
		return NULL;
}
