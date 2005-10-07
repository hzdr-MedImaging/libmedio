//! @file CConcordeImage.cpp
//! @brief contains the implementation of the class CConcordeImage
//! @author Hagen Moelle
//! @date 11/13/2004

#include "CConcordeImage.h"
#include "CMedIOHeader.h"
#include "CHeaderConcorde.h"
#include "CMedIOData.h"
#include <rtdebug.h>

#include <qdatastream.h>
#include <qfile.h>

#define READ_SIZE 8192

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
bool CConcordeImage::readMatrix(QByteArray*& matrixData, short frame)
{
	ENTER();
	bool result = true;

	//clean up
	if(matrixData)
			delete matrixData;
	matrixData = NULL;
	
	//check if file is open
	if(!isOpen())
	{
		result = false;
	}
	else
	{
		//check if header is from Concorde
		if(m_pCachedMainHeader->headerFormat() == CMedIOHeader::ConcordeMicropet)
		{
			CHeaderConcorde* head = m_pCachedMainHeader;
			//create new filepointer to datafile
			
			unsigned int framesize = head->getImageFrameSize();
			
			matrixData = new QByteArray(framesize, 0);
			char * data = matrixData->data();
	
			
			//check if desired frame exists
			int totalframes = head->totalFrames();
			if(frame > totalframes)
			{
				//throw exception if frame out of range
				result = false;
			}
			else
			{
				
				//set readpointer to appropriate frame
				seek((frame-1)*framesize);
				
				//get byte order from header and set it in datastream
				switch(head->dataType())
				{
					case CHeaderConcorde::UnknownDataType:
					{
						W("No or an unknown data type");
						QFile::read(data, framesize);	
					}
					break;
					case CHeaderConcorde::Byte:
					{
						// we use the RawBytes() method here.
						QFile::read(data, framesize);
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
							unsigned int curRead = QFile::read(bufArray.data(), toRead);
							
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
							unsigned int curRead = QFile::read(bufArray.data(), toRead);
							
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
							unsigned int curRead = QFile::read(bufArray.data(), toRead);
							
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
							unsigned int curRead = QFile::read(bufArray.data(), toRead);
							
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
							unsigned int curRead = QFile::read(bufArray.data(), toRead);
							
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
							unsigned int curRead = QFile::read(bufArray.data(), toRead);
							
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
			}
			if(!result)
			{
				delete matrixData;
			}
		}
	}
	RETURN(result);
	return result;
}
