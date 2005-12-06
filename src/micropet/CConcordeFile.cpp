/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004 by Jens Langner <Jens.Langner@light-speed.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

//! @file CConcordeFile.cpp
//! @brief contains the implementation of the class CConcordeFile
//! @author Hagen Moelle

#include "CConcordeFile.h"
#include "CConcordeSinogram.h"
#include "CConcordeImage.h"
#include "CConcordeMainHeaderSinogram.h"
#include "CConcordeMainHeaderImage.h"
#include "CConcordeFrameHeader.h"
#include "CMedIOData.h"

#include <rtdebug.h>
#include <qdatastream.h>

#define READ_SIZE 8192

//  Class: CConcordeFile
//  Constructor: CConcordeFile
//!
//! constructs a CConcordeFile object
//!
////////////////////////////////////////////////////////////////////////////////
CConcordeFile::CConcordeFile(const QString& fileName)
	: CMedIOData(fileName),
		m_pCachedMainHeader(NULL)
{
	ENTER();
	LEAVE();
}

//  Class: CConcordeFile
//  Destructor:  CConcordeFile
//!
//! destructucts a CConcordeFile object.
//! 
////////////////////////////////////////////////////////////////////////////////
CConcordeFile::~CConcordeFile()
{
	ENTER();

	delete m_pCachedMainHeader;
	
	LEAVE();
}

CConcordeFile::CConcordeFile(const CConcordeFile& src)
	: CMedIOData(src.fileName())
{
	ENTER();

	// just set the cachedMainHeader pointer to NULL
	m_pCachedMainHeader = NULL;

	LEAVE();
}
	
CConcordeFile& CConcordeFile::operator=(const CConcordeFile&)
{
	ENTER();

	// do nothing

	LEAVE();
	return *this;
}

int CConcordeFile::rtti() const 
{ 
	return CMedIOData::ConcordeMicropet;
}

//  Class: CConcordeFile
//  Method: save
//!
//! This method saves the data
//!
//! @return true if successful otherwise false
////////////////////////////////////////////////////////////////////////////////
void CConcordeFile::close()
{
	ENTER();
	m_pCachedMainHeader->save();
	delete m_pCachedMainHeader;
	m_pCachedMainHeader = NULL;
	QFile::close();
	LEAVE();
	return;
}

bool CConcordeFile::readMainHeader(CConcordeMainHeader*& mainHeader)
{
	ENTER();
	bool result = false;
	if(!isOpen())
	{
		W("Can not read mainheader if file is closed");
		result = false;
		mainHeader = NULL;
	}
	else
	{
		D("Trying to read main header"); 
		//delete mainHeader;
		switch(m_pCachedMainHeader->fileType())
		{
			case CConcordeMainHeader::Sinogram:
			case CConcordeMainHeader::Normalization:
			case CConcordeMainHeader::Attenuation:
			{
				D("Setting new sinogram");
				//#warning "filename is wrong but not used"
				mainHeader = new CConcordeMainHeaderSinogram(this);//(fileName() + ".hdr");
				result = true;
			}
			break;
			case CConcordeMainHeader::Image:
			case CConcordeMainHeader::MuMap:
			{
				//#warning "filename is wrong but not used"
				D("Setting new image");
				mainHeader = new CConcordeMainHeaderImage(this);//(fileName() + ".hdr");
				result = true;
			}
			break;
			default:
			{
				E("File type is not yet supported");
				result = false;
			}
			break;
		}
		if(result)
				*mainHeader = *m_pCachedMainHeader;
	}
	RETURN(result);
	return result;
}

bool CConcordeFile::readSubHeader(CConcordeFrameHeader*& subHeader, int frame)
{
	ENTER();
	bool result = false;
	if(!isOpen())
	{
		W("Can not read mainheader if file is closed");
		result = false;
		subHeader = NULL;
	}
	else
	{
		//TODO: check if frame is in between 1 and number of frames in study
		W("TODO: check if frame is in between 1 and number of frames in study");
		subHeader = new CConcordeFrameHeader(this, frame-1);
		if(subHeader)
			result = true;
	}
	RETURN(result);
	return result;
}

//  Class: CConcordeFile
//  Method: isoftype
//!
//! This method tries to identify the format of the given file.
//! 
//! @return type of file or unknown if unknown or error
////////////////////////////////////////////////////////////////////////////////
int CConcordeFile::isoftype(QString file)
{
	//try to initialise the header 
	//check if it is a CConcordeFile
	//return the specific type
	//return CData::Unknwon - if it is not a concorde type 
	ENTER();
	D("Check if file is a specific concorde file");
        // try to get Headerinfo on Sinogramfile
        QString hfile(file+".hdr");
        
	CConcordeMainHeaderSinogram head(file+".hdr");
	
	int result = 0;
        
	if(head.model() == 2000)
        {
                D("File is from concorde");
		//file type = 2 -> Sinogram
		//file type = 3 -> Normalisation
		//file type = 4 -> Attenuation (transmission)
		//file type = 5 -> Image
		//file type = 8 -> Mu map ( also image )
		// since attenuationfile/Normalisation is a sinogram we could define it as one 
		switch(head.fileType())
		{
			case CConcordeMainHeader::Sinogram:
			case CConcordeMainHeader::Normalization:
			case CConcordeMainHeader::Attenuation:
			{
				D("File is a sinogram");
				result = CConcordeFile::ConcordeMicropet_Sinogram;
			}
			break;
			case CConcordeMainHeader::Image:
			case CConcordeMainHeader::MuMap:
			{
				D("File is an image");
				result = CConcordeFile::ConcordeMicropet_Image;
			}
			break;
			default:
				result = CConcordeFile::Unknown;
		}
        }
        else
        {
		D("File is not from Concorde");
		result = CMedIOData::Unknown;
	}
	RETURN(result);
	return result;
}

CMedIOData* CConcordeFile::createFromFile(const QString& fileName)
{
	ENTER();
	CMedIOData* data = NULL;	
	switch(CConcordeFile::isoftype(fileName))
	{
		case CConcordeFile::ConcordeMicropet_Sinogram:
		{
			D("Creating concorde microPET sinogram");
			data = new CConcordeSinogram(fileName);
			if(data == NULL) 
				D("An error occured when creating sinogram");
			else
				D("Done with creating sinogram");
		}
		break;
		case CConcordeFile::ConcordeMicropet_Image:
		{
			D("Creating concorde microPET image");
			data = new CConcordeImage(fileName);
			if(data == NULL) 
				D("An error occured when creating image");
			else
				D("Done with creating image");
		}
		break;
		default:
		data = NULL;
	}

	RETURN(data);
	return data;
}

bool CConcordeFile::readMatrix(QByteArray*& matrixData, short frame)
{
		ENTER();
		bool result = false;
		matrixData = NULL;
		if(isOpen())
		{
				char* pTmp = NULL;
				unsigned int iFrameSize = 0;
				if(readMatrix(pTmp, iFrameSize, frame))
				{
						result = true;
						matrixData = new QByteArray();
						matrixData->setRawData(pTmp, iFrameSize);
				}
		}
		RETURN(result);
		return result;
}

bool CConcordeFile::readMatrix(QByteArray*& matrixData, CConcordeFrameHeader*& subHeader, short frame)
{
		ENTER();
		bool result = false;
		matrixData = NULL;
		if(isOpen())
				if(readSubHeader(subHeader, frame))
				{
						char* pTmp = matrixData->data();
						unsigned int iFrameSize = 0;
						if(readMatrix(pTmp, iFrameSize, frame))
						{
								result = true;
								matrixData = new QByteArray();
								matrixData->setRawData(pTmp, iFrameSize);
						}
				}
		RETURN(result);
		return result;
}

bool CConcordeFile::readMatrix(char*& matrixData, unsigned int& length, CConcordeFrameHeader*& subHeader, short frame)
{
		ENTER();
		bool result = false;
		if(isOpen())
				if(readSubHeader(subHeader, frame))
						if(readMatrix(matrixData, length, frame))
								result = true;
		RETURN(result);
		return result;
}

//  Class: CConcordeFile
//  Method: readMatrix
//!
//! This method loads the data of a sinogram from file to memory and constructs
//! a QByteArray which holds the data. The user is responsible for destruction
//! of the object.
//!
//! @return data of specific frame in sinogram or NULL on error
////////////////////////////////////////////////////////////////////////////////
bool CConcordeFile::readMatrix(char*& matrixData, unsigned int& length, short frame)
{
	ENTER();
	bool result = true;

	//clean up
	//if(matrixData)
	//		delete matrixData;
	//matrixData = NULL;
	
	//check if file is open
	if(!isOpen())
	{
		result = false;
	}
	else
	{
		//check if header is from Concorde
		//if(m_pCachedMainHeader->headerFormat() == CMedIOHeader::ConcordeMicropet)
		{
			
			CConcordeMainHeader* head = m_pCachedMainHeader;
			 
			unsigned int framesize = m_pCachedMainHeader->frameSize();
			length = framesize;

			//matrixData = new QByteArray(framesize, 0);
			matrixData = new char[framesize];//matrixData->data();
	
			
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
				at((frame-1)*framesize);
				
				//get byte order from header and set it in datastream
				switch(head->dataType())
				{
					case CConcordeMainHeader::UnknownDataType:
					{
						W("No or an unknown data type");
						QFile::readBlock(matrixData, framesize);	
					}
					break;
					case CConcordeMainHeader::Byte:
					{
						// we use the RawBytes() method here.
						QFile::readBlock(matrixData, framesize);
					}
					break;
					// IntelShort is a little endian short value, so we
					// need to set the stream to little endian, read the data via the
					// QDataStream operators to ensure correct byte swapping
					case CConcordeMainHeader::IntelShort:
					{
						QByteArray bufArray(READ_SIZE);
						Q_UINT16* ptr = (Q_UINT16*)matrixData;
						for(unsigned int read=0; read < framesize;)
						{
							unsigned int toRead = framesize-read >= READ_SIZE ? READ_SIZE : framesize-read;
							unsigned int curRead = QFile::readBlock(bufArray.data(), toRead);
							
							if(curRead != toRead)
							{
								result = false;
								break;
							}
							
							// check if the curRead value is divide able through our data type 
							ASSERT(curRead % sizeof(Q_UINT16) == 0);
							
							QDataStream bufStream(bufArray, IO_ReadOnly);
							bufStream.setByteOrder(QDataStream::LittleEndian);
							for(unsigned int i=0; i < curRead; i+=sizeof(Q_UINT16))
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
					case CConcordeMainHeader::IntelInt:
					{
						D("DataType is : little endian integer");
						D("FrameSize : %d", framesize);
						QByteArray bufArray(READ_SIZE);
						Q_UINT32* ptr = (Q_UINT32*)matrixData;
						for(unsigned int read=0; read < framesize;)
						{
							unsigned int toRead = framesize-read >= READ_SIZE ? READ_SIZE : framesize-read;
							unsigned int curRead = QFile::readBlock(bufArray.data(), toRead);
							
							if(curRead != toRead)
							{
								result = false;
								break;
							}
							
							// check if the curRead value is divide able through our data type 
							ASSERT(curRead % sizeof(Q_UINT32) == 0);
							
							QDataStream bufStream(bufArray, IO_ReadOnly);
							bufStream.setByteOrder(QDataStream::LittleEndian);
							for(unsigned int i=0; i < curRead; i+=sizeof(Q_UINT32))
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
					case CConcordeMainHeader::IntelFloat:
					{
						D("DataType is : little endian float");
						D("FrameSize : %d", framesize);
						QByteArray bufArray(READ_SIZE);
						float* ptr = (float*)matrixData;
						for(unsigned int read=0; read < framesize;)
						{
							unsigned int toRead = framesize-read >= READ_SIZE ? READ_SIZE : framesize-read;
							unsigned int curRead = QFile::readBlock(bufArray.data(), toRead);
							
							if(curRead != toRead)
							{
								result = false;
								break;
							}
							
							// check if the curRead value is divide able through our data type 
							ASSERT(curRead % sizeof(float) == 0);
							
							QDataStream bufStream(bufArray, IO_ReadOnly);
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
					case CConcordeMainHeader::IEEEFloat:
					{
						QByteArray bufArray(READ_SIZE);
						float* ptr = (float*)matrixData;
						for(unsigned int read=0; read < framesize;)
						{
							unsigned int toRead = framesize-read >= READ_SIZE ? READ_SIZE : framesize-read;
							unsigned int curRead = QFile::readBlock(bufArray.data(), toRead);
							
							if(curRead != toRead)
							{
								result = false;
								break;
							}
							
							// check if the curRead value is divide able through our data type 
							ASSERT(curRead % sizeof(float) == 0);
							
							QDataStream bufStream(bufArray, IO_ReadOnly);
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
					case CConcordeMainHeader::SunShort:
					{
						QByteArray bufArray(READ_SIZE);
						Q_UINT16* ptr = (Q_UINT16*)matrixData;
						for(unsigned int read=0; read < framesize;)
						{
							unsigned int toRead = framesize-read >= READ_SIZE ? READ_SIZE : framesize-read;
							unsigned int curRead = QFile::readBlock(bufArray.data(), toRead);
							
							if(curRead != toRead)
							{
								result = false;
								break;
							}
							
							// check if the curRead value is divide able through our data type 
							ASSERT(curRead % sizeof(Q_UINT16) == 0);
							
							QDataStream bufStream(bufArray, IO_ReadOnly);
							for(unsigned int i=0; i < curRead; i+=sizeof(Q_UINT16))
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
					case CConcordeMainHeader::SunInt:
					{
						QByteArray bufArray(READ_SIZE);
						Q_UINT32* ptr = (Q_UINT32*)matrixData;
						for(unsigned int read=0; read < framesize;)
						{
							unsigned int toRead = framesize-read >= READ_SIZE ? READ_SIZE : framesize-read;
							unsigned int curRead = QFile::readBlock(bufArray.data(), toRead);
							
							if(curRead != toRead)
							{
								result = false;
								break;
							}
							
							// check if the curRead value is divide able through our data type 
							ASSERT(curRead % sizeof(Q_UINT32) == 0);
							
							QDataStream bufStream(bufArray, IO_ReadOnly);
							for(unsigned int i=0; i < curRead; i+=sizeof(Q_UINT32))
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
				matrixData = NULL;
			}
			//else
			//{
			//	matrixData = new QByteArray();
			//	matrixData->setRawData(data, framesize);
			//}
		}
	}
	RETURN(result);
	return result;
}
