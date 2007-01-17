/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004-2007 by Jens Langner <Jens.Langner@light-speed.de>

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

#include "CECATDirectoryItem.h"
#include "CECATFile.h"

#include "CECAT7SubHeaderAttenCorr.h"
#include "CECAT7SubHeaderImage.h"
#include "CECAT7SubHeaderNorm.h"
#include "CECAT7SubHeaderNorm3D.h"
#include "CECAT7SubHeaderPolarMap.h"
#include "CECAT7SubHeaderScan.h"
#include "CECAT7SubHeaderScan3D.h"

#include <QDataStream>

#include <rtdebug.h>

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CECATDirectoryItemPrivate
{
	public:
		void cacheSubHeader(const CECATSubHeader& subHeader);
		
		CECATDirectoryItem*		dirItem;					// pointer to the directory Item
		CECATFile*						file;							// pointer to the ECATFile
		CECATSubHeader*				cachedSubHeader;	// pointer to a cached SubHeader object

		// META information about the directory Item
		short									frame;						// information normally
		short									plane;						// encoded in
		short									gate;							// the MatrixID
		short									bed;
		short									data;
		qint64								dataBlock_Start;	// also start of SubHeader
		qint64								dataBlock_End;		// end of raw Data area

		enum CECATDirectoryItem::AccessStatus			status;
};

CECATDirectoryItem::CECATDirectoryItem(CECATFile* pFile,
																			 quint32 matrixID) 
{
	ENTER();

	// allocate data from our private instance class
	m_pData = new CECATDirectoryItemPrivate();
	m_pData->dirItem = this;
	m_pData->file = pFile;
	m_pData->cachedSubHeader = NULL;
	m_pData->dataBlock_Start = 0;
	m_pData->dataBlock_End = 0;
	m_pData->status = NotYetWritten;

	if(matrixID != 0)
	{
		m_pData->frame = matrixID2Frame(matrixID);
		m_pData->plane = matrixID2Plane(matrixID);
		m_pData->gate  = matrixID2Gate(matrixID);
		m_pData->bed   = matrixID2Bed(matrixID);
		m_pData->data  = matrixID2Data(matrixID);

		D("created new DirItem (%d/%d/%d/%d/%d)", m_pData->frame, m_pData->plane, m_pData->gate, m_pData->bed, m_pData->data);
	}
	else
	{
		m_pData->frame = -1;
		m_pData->plane = -1;
		m_pData->gate = -1;
		m_pData->bed = -1;
		m_pData->data = -1;
	}		
	
	LEAVE();
}

CECATDirectoryItem::~CECATDirectoryItem()
{
	ENTER();

	delete m_pData->cachedSubHeader;
	delete m_pData;

	LEAVE();
}

CECATDirectoryItem::CECATDirectoryItem(const CECATDirectoryItem& src)
{
	ENTER();

	m_pData = new CECATDirectoryItemPrivate();

	// copy the instance data
	*this = src;

	// set the file ptr to NULL	
	m_pData->dirItem = this;
	m_pData->file = NULL;
	m_pData->cachedSubHeader = NULL;
	
	LEAVE();
}

CECATDirectoryItem& CECATDirectoryItem::operator=(const CECATDirectoryItem& src)
{
	ENTER();

	if(m_pData != src.m_pData)
	{
		m_pData->frame						= src.m_pData->frame;
		m_pData->plane						= src.m_pData->plane;
		m_pData->gate							= src.m_pData->gate;
		m_pData->bed							= src.m_pData->bed;
		m_pData->data							= src.m_pData->data;
		m_pData->dataBlock_Start	= src.m_pData->dataBlock_Start;
		m_pData->dataBlock_End		= src.m_pData->dataBlock_End;
		m_pData->status						= src.m_pData->status;
	}

	LEAVE();
	return *this;
}

quint32 CECATDirectoryItem::matrixID(void) const
{ 
	return convertToMatrixID(m_pData->frame, 
													 m_pData->plane, 
													 m_pData->gate, 
													 m_pData->bed, 
													 m_pData->data);
}

qint64 CECATDirectoryItem::dataBlock_Start(void) const
{ 
	return m_pData->dataBlock_Start;
}

qint64 CECATDirectoryItem::dataBlock_End(void) const
{ 
	return m_pData->dataBlock_End;
}

CECATDirectoryItem::AccessStatus CECATDirectoryItem::dataBlock_Status(void) const
{ 
	return m_pData->status; 
}

short CECATDirectoryItem::frame(void) const
{ 
	return m_pData->frame;
}

short CECATDirectoryItem::plane(void) const
{ 
	return m_pData->plane;
}

short CECATDirectoryItem::gate(void) const
{ 
	return m_pData->gate;
}

short CECATDirectoryItem::bed(void) const
{ 
	return m_pData->bed;
}

short CECATDirectoryItem::data(void) const
{	
	return m_pData->data;
}

// mutator methods
void CECATDirectoryItem::setDataBlock_Start(const qint64 offset)
{ 
	m_pData->dataBlock_Start = offset;
}

void CECATDirectoryItem::setDataBlock_End(const qint64 offset)
{ 
	m_pData->dataBlock_End = offset;
}

void CECATDirectoryItem::setDataBlock_Status(const AccessStatus status)
{ 
	m_pData->status = status;
}

void CECATDirectoryItem::setFrame(const short f)
{ 
	m_pData->frame = f;
}

void CECATDirectoryItem::setPlane(const short p)
{ 
	m_pData->plane = p;
}

void CECATDirectoryItem::setGate(const short g)
{ 
	m_pData->gate = g;
}

void CECATDirectoryItem::setBed(const short b)
{ 
	m_pData->bed = b;
}

void CECATDirectoryItem::setData(const short d)
{	
	m_pData->data = d;
}		

bool CECATDirectoryItem::readSubHeader(CECATSubHeader*& subHeader)
{
	ENTER();
	bool result = false;	

	if(m_pData->file && m_pData->file->isReadable())
	{
		// check if we have a cached sub header ready already so that
		// we can take that one instead of loading the sub header once
		// more from scratch
		if(m_pData->cachedSubHeader)
		{
			switch(m_pData->file->subHeaderType())
			{
				case CECATSubHeader::ECAT7_AttenCorr: 
					subHeader = new CECAT7SubHeaderAttenCorr(*static_cast<CECAT7SubHeaderAttenCorr*>(m_pData->cachedSubHeader));
					result = true;
				break;

				case CECATSubHeader::ECAT7_Image:	
					subHeader = new CECAT7SubHeaderImage(*static_cast<CECAT7SubHeaderImage*>(m_pData->cachedSubHeader));
					result = true;
				break;
		
				case CECATSubHeader::ECAT7_Norm:
					subHeader = new CECAT7SubHeaderNorm(*static_cast<CECAT7SubHeaderNorm*>(m_pData->cachedSubHeader));
					result = true;
				break;
				
				case CECATSubHeader::ECAT7_Norm3D:
					subHeader = new CECAT7SubHeaderNorm3D(*static_cast<CECAT7SubHeaderNorm3D*>(m_pData->cachedSubHeader));
					result = true;
				break;
				
				case CECATSubHeader::ECAT7_PolarMap:
					subHeader = new CECAT7SubHeaderPolarMap(*static_cast<CECAT7SubHeaderPolarMap*>(m_pData->cachedSubHeader));
					result = true;
				break;
				
				case CECATSubHeader::ECAT7_Scan:
					subHeader = new CECAT7SubHeaderScan(*static_cast<CECAT7SubHeaderScan*>(m_pData->cachedSubHeader));
					result = true;
				break;
				
				case CECATSubHeader::ECAT7_Scan3D:
					subHeader = new CECAT7SubHeaderScan3D(*static_cast<CECAT7SubHeaderScan3D*>(m_pData->cachedSubHeader));
					result = true;
				break;

				default:
					E("ECAT type isn't specified or not supported yet.");
			}

			// set the mediodata object for the newly created subheader
			subHeader->setMedIOData(m_pData->file);
			subHeader->setDirectoryItem(this);
		}
		else
		{
			// lets prepare the SubHeader depending on the type of the
			// ECATFile
			switch(m_pData->file->subHeaderType())
			{
				case CECATSubHeader::ECAT7_AttenCorr: 
					subHeader = new CECAT7SubHeaderAttenCorr(m_pData->file, this);
				break;

				case CECATSubHeader::ECAT7_Image:	
					subHeader = new CECAT7SubHeaderImage(m_pData->file, this);
				break;
		
				case CECATSubHeader::ECAT7_Norm:
					subHeader = new CECAT7SubHeaderNorm(m_pData->file, this);
				break;
				
				case CECATSubHeader::ECAT7_Norm3D:
					subHeader = new CECAT7SubHeaderNorm3D(m_pData->file, this);
				break;
				
				case CECATSubHeader::ECAT7_PolarMap:
					subHeader = new CECAT7SubHeaderPolarMap(m_pData->file, this);
				break;
				
				case CECATSubHeader::ECAT7_Scan:
					subHeader = new CECAT7SubHeaderScan(m_pData->file, this);
				break;
				
				case CECATSubHeader::ECAT7_Scan3D:
					subHeader = new CECAT7SubHeaderScan3D(m_pData->file, this);
				break;

				default:
					E("ECAT type isn't specified or not supported yet.");
			}

			if(subHeader)
			{
				if(subHeader->load() == false)
				{
					delete subHeader;
					subHeader = NULL;
				}
				else
					result = true;
			}			
		}
	}

	RETURN(result);
	return result;
}

bool CECATDirectoryItem::readMatrix(QByteArray*& matrixData)
{
	ENTER();
	bool result = false;
	char* data = NULL;
	unsigned int dataLen = 0;

	// we use our method operating on a raw char pointer
	if(readMatrix(data, dataLen) && dataLen > 0)
	{
		matrixData = new QByteArray(data, dataLen);
		result = true;
	}

	RETURN(result);
	return result;
}

bool CECATDirectoryItem::readMatrix(char*& matrixData, unsigned int& matrixSize)
{
	ENTER();
	bool result = true;
		
	// check if the file associated with this item is readable or
	// not
	if(m_pData->file && m_pData->file->isReadable() == false)
	{
		RETURN(false);
		return false;
	}
	
	// we load the MatrixData out of the ECATFile manually but only
	// if the access status of this item is set to "Finished"
	if(m_pData->status == Finished)
	{
		// before we can read in the raw data we need to read in
		// the Subheader so that we know the data type of our raw
		// data
		CECATSubHeader* subHeader = NULL;
		if(readSubHeader(subHeader) == false || subHeader == NULL)
		{
			RETURN(false);
			return false;
		}

		// set the ECATFile handle to the correct position of
		// the matrix start. This should normally be
		// Matrix_SubHeader_StartPosition+1
		if(m_pData->file->seek(m_pData->dataBlock_Start+subHeader->rawDataSize()) == false)
		{
			RETURN(false);
			return false;
		}

		SHOWVALUE(m_pData->file->pos());

		// then we allocate some memory for loading the matrixdata
		// this should be:
		matrixSize = m_pData->dataBlock_End-m_pData->dataBlock_Start-subHeader->rawDataSize()+ECAT_BLOCKSIZE;
		matrixData = new char[matrixSize];

		// then we process the matrix data that is associated with
		// this directoryitem.
		// here we have to care about the correct endianess, so that
		// we convert BIG->LITTLE or vise versa depending on the
		// system we are running.
		switch(subHeader->data_Type())
		{
			case CECATSubHeader::UnknownDataType:
			{
				W("No or an unknown data type was set for the matrix data of dirItem %08lx",
						convertToMatrixID(m_pData->frame, m_pData->plane, m_pData->gate, m_pData->bed, m_pData->data));

				if(m_pData->file->read(matrixData, matrixSize) > 0)
					result = true;
			}
			break;

			// write out Byte data. (1 byte)
			case CECATSubHeader::ByteData:
			{
				if(m_pData->file->read(matrixData, matrixSize) > 0)
					result = true;
			}
			break;

			// VAX_Ix2 is a little endian short value, so we
			// need to set the stream to little endian, read the data via the
			// QDataStream operators to ensure correct byte swapping
			case CECATSubHeader::VAX_Ix2:
			{
				// the endianess conversion takes quite some time, so
				// what we do is to us a temporarly buffer to which we read
				// some data from our fileStream and read out to our QByteArray
				QByteArray bufArray(8192, 0); // read 8KB chunks
				quint16* ptr = (quint16*)matrixData;
				for(unsigned int read=0; read < matrixSize;)
				{
					unsigned int toRead = matrixSize-read >= 8192 ? 8192 : matrixSize-read;
					unsigned int curRead = m_pData->file->read(bufArray.data(), toRead);
					if(curRead != toRead)
					{
						result = false;
						break;
					}

					// check if the curRead value is divide able through our data type 
					ASSERT(curRead % sizeof(quint16) == 0);

					// now that we have our chunk we use a bufferStream to stream
					// out the values from it for making sure our data is correctly
					// converted regarding to little/big endianess
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

			// VAX_Ix4 is a little endian long value, so we
			// need to set the stream to little endian, read the data via the
			// QDataStream operators to ensure correct byte swapping
			case CECATSubHeader::VAX_Ix4:
			{
				// the endianess conversion takes quite some time, so
				// what we do is to us a temporarly buffer to which we read
				// some data from our fileStream and read out to our QByteArray
				QByteArray bufArray(8192, 0); // read 8KB chunks
				quint32* ptr = (quint32*)matrixData;
				for(unsigned int read=0; read < matrixSize;)
				{
					unsigned int toRead = matrixSize-read >= 8192 ? 8192 : matrixSize-read;
					unsigned int curRead = m_pData->file->read(bufArray.data(), toRead);
					if(curRead != toRead)
					{
						result = false;
						break;
					}

					// check if the curRead value is divide able through our data type 
					ASSERT(curRead % sizeof(quint32) == 0);

					// now that we have our chunk we use a bufferStream to stream
					// out the values from it for making sure our data is correctly
					// converted regarding to little/big endianess
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
			
			// VAX_Rx4 is a little endian float value, so we
			// need to set the stream to little endian, read the data via the
			// QDataStream operators to ensure correct byte swapping			
			case CECATSubHeader::VAX_Rx4:
			{
				// the endianess conversion takes quite some time, so
				// what we do is to us a temporarly buffer to which we read
				// some data from our fileStream and read out to our QByteArray
				QByteArray bufArray(8192, 0); // read 8KB chunks
				float* ptr = (float*)matrixData;
				for(unsigned int read=0; read < matrixSize;)
				{
					unsigned int toRead = matrixSize-read >= 8192 ? 8192 : matrixSize-read;
					unsigned int curRead = m_pData->file->read(bufArray.data(), toRead);
					if(curRead != toRead)
					{
						result = false;
						break;
					}

					// check if the curRead value is divide able through our data type 
					ASSERT(curRead % sizeof(float) == 0);

					// now that we have our chunk we use a bufferStream to stream
					// out the values from it for making sure our data is correctly
					// converted regarding to little/big endianess
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
			case CECATSubHeader::IEEEFloat:
			{
				// the endianess conversion takes quite some time, so
				// what we do is to us a temporarly buffer to which we read
				// some data from our fileStream and read out to our QByteArray
				QByteArray bufArray(8192, 0); // read 8KB chunks
				float* ptr = (float*)matrixData;
				for(unsigned int read=0; read < matrixSize;)
				{
					unsigned int toRead = matrixSize-read >= 8192 ? 8192 : matrixSize-read;
					unsigned int curRead = m_pData->file->read(bufArray.data(), toRead);
					if(curRead != toRead)
					{
						result = false;
						break;
					}

					// check if the curRead value is divide able through our data type 
					ASSERT(curRead % sizeof(float) == 0);

					// now that we have our chunk we use a bufferStream to stream
					// out the values from it for making sure our data is correctly
					// converted regarding to little/big endianess
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
			case CECATSubHeader::SunShort:
			{
				// the endianess conversion takes quite some time, so
				// what we do is to us a temporarly buffer to which we read
				// some data from our fileStream and read out to our QByteArray
				QByteArray bufArray(8192, 0); // read 8KB chunks
				quint16* ptr = (quint16*)matrixData;
				for(unsigned int read=0; read < matrixSize;)
				{
					unsigned int toRead = matrixSize-read >= 8192 ? 8192 : matrixSize-read;
					unsigned int curRead = m_pData->file->read(bufArray.data(), toRead);
					if(curRead != toRead)
					{
						result = false;
						break;
					}

					// check if the curRead value is divide able through our data type 
					ASSERT(curRead % sizeof(quint16) == 0);

					// now that we have our chunk we use a bufferStream to stream
					// out the values from it for making sure our data is correctly
					// converted regarding to little/big endianess
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

			// SunLong is defined to be a big endian long value. As the QDataStream
			// is per default big endian, we don't have to set it to another byte order
			// and just use the QDataStream operators to ensure correct byte swapping
			case CECATSubHeader::SunLong:
			{
				// the endianess conversion takes quite some time, so
				// what we do is to us a temporarly buffer to which we read
				// some data from our fileStream and read out to our QByteArray
				QByteArray bufArray(8192, 0); // read 8KB chunks
				quint32* ptr = (quint32*)matrixData;
				for(unsigned int read=0; read < matrixSize;)
				{
					unsigned int toRead = matrixSize-read >= 8192 ? 8192 : matrixSize-read;
					unsigned int curRead = m_pData->file->read(bufArray.data(), toRead);
					if(curRead != toRead)
					{
						result = false;
						break;
					}
					
					// check if the curRead value is divide able through our data type 
					ASSERT(curRead % sizeof(quint32) == 0);

					// now that we have our chunk we use a bufferStream to stream
					// out the values from it for making sure our data is correctly
					// converted regarding to little/big endianess
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

		// now we delete the subHeader we loaded temporarly
		delete subHeader;
	}
	else
		result = false;

	RETURN(result);
	return result;
}

bool CECATDirectoryItem::readMatrix(QByteArray*& matrixData, CECATSubHeader*& subHeader)
{
	ENTER();
	bool result = false;
	char* data = NULL;
	unsigned int dataLen = 0;

	// we use our method operating on a raw char pointer
	if(readMatrix(data, dataLen, subHeader) && dataLen > 0)
	{
		matrixData = new QByteArray(data, dataLen);
		result = true;
	}

	RETURN(result);
	return result;
}

bool CECATDirectoryItem::readMatrix(char*& data, unsigned int& len, CECATSubHeader*& subHeader)
{
	ENTER();
	bool result = false;

	// read out the subHeader first
	if(readSubHeader(subHeader))
	{
		// we use our method operating on a raw char pointer
		if(readMatrix(data, len) && len > 0)
			result = true;
	}

	RETURN(result);
	return result;
}

bool CECATDirectoryItem::writeSubHeader(const CECATSubHeader& subHeader)
{
	ENTER();
	bool result = false;

	// make sure the subHeader is going to be cached
	m_pData->cacheSubHeader(subHeader);

	// now issue a write operation upon the cached subheader
	result = m_pData->cachedSubHeader->save();

	RETURN(result);
	return result;
}

void CECATDirectoryItemPrivate::cacheSubHeader(const CECATSubHeader& subHeader)
{
	ENTER();

	// before we can save the subHeader we have to generate the cached copy
	// and make sure it belongs to this particular directory Item.
	switch(file->subHeaderType())
	{
		case CECATSubHeader::ECAT7_AttenCorr: 
		{
			if(cachedSubHeader)
				*static_cast<CECAT7SubHeaderAttenCorr*>(cachedSubHeader) = *static_cast<const CECAT7SubHeaderAttenCorr*>(&subHeader);
			else
				cachedSubHeader = new CECAT7SubHeaderAttenCorr(*static_cast<const CECAT7SubHeaderAttenCorr*>(&subHeader));
		}
		break;

		case CECATSubHeader::ECAT7_Image:	
		{
			if(cachedSubHeader)
				*static_cast<CECAT7SubHeaderImage*>(cachedSubHeader) = *static_cast<const CECAT7SubHeaderImage*>(&subHeader);
			else
				cachedSubHeader = new CECAT7SubHeaderImage(*static_cast<const CECAT7SubHeaderImage*>(&subHeader));
		}
		break;
	
		case CECATSubHeader::ECAT7_Norm:
		{
			if(cachedSubHeader)
				*static_cast<CECAT7SubHeaderNorm*>(cachedSubHeader) = *static_cast<const CECAT7SubHeaderNorm*>(&subHeader);
			else
				cachedSubHeader = new CECAT7SubHeaderNorm(*static_cast<const CECAT7SubHeaderNorm*>(&subHeader));
		}
		break;
			
		case CECATSubHeader::ECAT7_Norm3D:
		{
			if(cachedSubHeader)
				*static_cast<CECAT7SubHeaderNorm3D*>(cachedSubHeader) = *static_cast<const CECAT7SubHeaderNorm3D*>(&subHeader);
			else
				cachedSubHeader = new CECAT7SubHeaderNorm3D(*static_cast<const CECAT7SubHeaderNorm3D*>(&subHeader));
		}
		break;
			
		case CECATSubHeader::ECAT7_PolarMap:
		{
			if(cachedSubHeader)
				*static_cast<CECAT7SubHeaderPolarMap*>(cachedSubHeader) = *static_cast<const CECAT7SubHeaderPolarMap*>(&subHeader);
			else		
				cachedSubHeader = new CECAT7SubHeaderPolarMap(*static_cast<const CECAT7SubHeaderPolarMap*>(&subHeader));
		}
		break;
			
		case CECATSubHeader::ECAT7_Scan:
		{
			if(cachedSubHeader)
				*static_cast<CECAT7SubHeaderScan*>(cachedSubHeader) = *static_cast<const CECAT7SubHeaderScan*>(&subHeader);
			else		
				cachedSubHeader = new CECAT7SubHeaderScan(*static_cast<const CECAT7SubHeaderScan*>(&subHeader));
		}
		break;
			
		case CECATSubHeader::ECAT7_Scan3D:
		{
			if(cachedSubHeader)
				*static_cast<CECAT7SubHeaderScan3D*>(cachedSubHeader) = *static_cast<const CECAT7SubHeaderScan3D*>(&subHeader);
			else		
				cachedSubHeader = new CECAT7SubHeaderScan3D(*static_cast<const CECAT7SubHeaderScan3D*>(&subHeader));
		}
		break;

		default:
			E("ECAT type isn't specified or not supported yet.");
	}
	
	if(cachedSubHeader)
	{
		cachedSubHeader->setMedIOData(file);
		cachedSubHeader->setDirectoryItem(dirItem);
	}
	
	LEAVE();
}

void CECATDirectoryItem::subHeaderWritten(const CECATSubHeader& subHeader)
{
	ENTER();

	// if the subHeaderWritten is the same as our cached subheader we don't
	// have to copy it again and can break out immediately
	if(&subHeader != m_pData->cachedSubHeader);
	{
		// otherwise we make sure the written subHeader is cached
		m_pData->cacheSubHeader(subHeader);
	}

	LEAVE();
}

bool CECATDirectoryItem::writeMatrix(const QByteArray& matrixData)
{
	ENTER();
  bool result;

	result = writeMatrix(matrixData.data(), matrixData.size());

	RETURN(result);
	return result;
}

bool CECATDirectoryItem::writeMatrix(const QByteArray& matrixData,
																		 CECATSubHeader::Data_Type dataType)
{
	ENTER();
  bool result;

	result = writeMatrix(matrixData.data(), matrixData.size(), dataType);

	RETURN(result);
	return result;
}

bool CECATDirectoryItem::writeMatrix(const char* matrixData, unsigned int matrixSize)
{
	ENTER();
	bool result;

	result = writeMatrix(matrixData, matrixSize, CECATSubHeader::UnknownDataType); 

	RETURN(result);
	return result;
}

bool CECATDirectoryItem::writeMatrix(const QByteArray& matrixData, const CECATSubHeader& subHeader)
{
	ENTER();
  bool result;

	result = writeMatrix(matrixData.data(), matrixData.size(), subHeader);

	RETURN(result);
	return result;
}

bool CECATDirectoryItem::writeMatrix(const char* matrixData, unsigned int matrixSize,
																		 CECATSubHeader::Data_Type dataType)
{
	ENTER();
	bool result = true;

	// check if the file associated with this item is writeable at all
	if(m_pData->file == NULL ||
		 m_pData->file->isWritable() == false)
	{
		RETURN(false);
		return false;
	}

	// before we can write out the raw data we need to read in
	// the Subheader so that we know the data type of our raw
	// data but before we actually read it out we check the status
	// of the directory item and eventually generate a new subHeader if
	// there isn't already one.
	CECATSubHeader* subHeader = NULL;

	SHOWVALUE(m_pData->status);
	SHOWVALUE(m_pData->dataBlock_Start);
	SHOWVALUE(m_pData->dataBlock_End);
	SHOWPOINTER(m_pData->cachedSubHeader);

	// try to read the subheader first
	if(dataType == CECATSubHeader::UnknownDataType ||
		 m_pData->status != NotYetWritten ||
		 m_pData->dataBlock_End != 0)
	{
		readSubHeader(subHeader);
	}

	SHOWPOINTER(subHeader);
	
	if(subHeader == NULL &&
		 m_pData->status == NotYetWritten &&
		 m_pData->dataBlock_End == 0)
	{
		SHOWVALUE(m_pData->file->subHeaderType());

		switch(m_pData->file->subHeaderType())
		{
			case CECATSubHeader::ECAT7_AttenCorr: 
				subHeader = new CECAT7SubHeaderAttenCorr(m_pData->file, this);
			break;

			case CECATSubHeader::ECAT7_Image:	
				subHeader = new CECAT7SubHeaderImage(m_pData->file, this);
			break;
		
			case CECATSubHeader::ECAT7_Norm:
				subHeader = new CECAT7SubHeaderNorm(m_pData->file, this);
			break;
				
			case CECATSubHeader::ECAT7_Norm3D:
				subHeader = new CECAT7SubHeaderNorm3D(m_pData->file, this);
			break;
				
			case CECATSubHeader::ECAT7_PolarMap:
				subHeader = new CECAT7SubHeaderPolarMap(m_pData->file, this);
			break;
				
			case CECATSubHeader::ECAT7_Scan:
				subHeader = new CECAT7SubHeaderScan(m_pData->file, this);
			break;
				
			case CECATSubHeader::ECAT7_Scan3D:
				subHeader = new CECAT7SubHeaderScan3D(m_pData->file, this);
			break;

			default:
				E("ECAT type isn't specified or not supported yet.");
		}
	}

	if(subHeader == NULL)
		result = false;
	else
	{
		if(subHeader->data_Type() != dataType &&
			 dataType != CECATSubHeader::UnknownDataType)
		{
			subHeader->setData_Type(dataType);
		}

		result = writeMatrix(matrixData, matrixSize, *subHeader);
	}

	// make sure to delete the new subHeader
	delete subHeader;
	
	RETURN(result);
	return result;
}

bool CECATDirectoryItem::writeMatrix(const char* matrixData, unsigned int matrixSize,
																		 const CECATSubHeader& subHeader)
{
	ENTER();
	bool result = true;
	
	// check if the file associated with this item is writeable at all
	if(m_pData->file == NULL || m_pData->file->isWritable() == false)
	{
		RETURN(false);
		return false;
	}

	// now that we have the proper subHeader here we make sure to write it to
	// out file prior to saving the matrixData
	if(writeSubHeader(subHeader) == false)
	{
		RETURN(false);
		return false;
	}

	// set the ECATFile handle to the correct position of
	// the matrix start. This should normally be
	// Matrix_SubHeader_StartPosition+subHeaderSize
	if(m_pData->file->seek(m_pData->dataBlock_Start+subHeader.rawDataSize()) == false)
	{
		RETURN(false);
		return false;
	}

	SHOWVALUE(m_pData->file->pos());

	// then we process the matrix data that is associated with
	// this directoryitem.
	// here we have to care about the correct endianess, so that
	// we convert BIG->LITTLE or vise versa depending on the
	// system we are running.
	switch(subHeader.data_Type())
	{
		case CECATSubHeader::UnknownDataType:
		{
			E("No or an unknown data type was set for the matrix data of dirItem %08lx",
					convertToMatrixID(m_pData->frame, m_pData->plane, m_pData->gate, m_pData->bed, m_pData->data));
			
			result = false;
		}
		break;

		// write out Byte data. (1 byte)
		case CECATSubHeader::ByteData:
		{
			if(m_pData->file->write(matrixData, matrixSize) == (qint64)matrixSize)
				result = true;
		}
		break;

		// VAX_Ix2 is a little endian short value, so we
		// need to set the stream to little endian, read the data via the
		// QDataStream operators to ensure correct byte swapping
		case CECATSubHeader::VAX_Ix2:
		{
			// the endianess conversion takes quite some time, so
			// what we do is to us a temporarly buffer to which we read
			// some data from our fileStream and read out to our QByteArray
			QByteArray bufArray(8192, 0); // write in 8KB chunks
			quint16* ptr = (quint16*)matrixData;
			for(unsigned int written=0; written < matrixSize;)
			{
				unsigned int toWrite = matrixSize-written >= 8192 ? 8192 : matrixSize-written;

				// check if the curRead value is divide able through our data type 
				ASSERT(toWrite % sizeof(quint16) == 0);

				// now that we have our chunk we use a bufferStream to stream
				// in the values to it for making sure our data is correctly
				// converted regarding to little/big endianess
				QDataStream bufStream(&bufArray, QIODevice::WriteOnly);
				bufStream.setByteOrder(QDataStream::LittleEndian);
				for(unsigned int i=0; i < toWrite; i+=sizeof(quint16))
				{
					bufStream << *ptr;
					++ptr;
				}
				bufStream.setByteOrder(QDataStream::BigEndian);

				// write out the data from our buffer to the file
				if(m_pData->file->write(bufArray.data(), toWrite) != (qint64)toWrite)
				{
					result = false;
					break;
				}
				
				// increase our read counter
				written += toWrite;
			}
		}
		break;

		// VAX_Ix4 is a little endian long value, so we
		// need to set the stream to little endian, read the data via the
		// QDataStream operators to ensure correct byte swapping
		case CECATSubHeader::VAX_Ix4:
		{
			// the endianess conversion takes quite some time, so
			// what we do is to us a temporarly buffer to which we read
			// some data from our fileStream and read out to our QByteArray
			QByteArray bufArray(8192, 0); // write in 8KB chunks
			quint32* ptr = (quint32*)matrixData;
			for(unsigned int written=0; written < matrixSize;)
			{
				unsigned int toWrite = matrixSize-written >= 8192 ? 8192 : matrixSize-written;

				// check if the curRead value is divide able through our data type 
				ASSERT(toWrite % sizeof(quint32) == 0);

				// now that we have our chunk we use a bufferStream to stream
				// in the values to it for making sure our data is correctly
				// converted regarding to little/big endianess
				QDataStream bufStream(&bufArray, QIODevice::WriteOnly);
				bufStream.setByteOrder(QDataStream::LittleEndian);
				for(unsigned int i=0; i < toWrite; i+=sizeof(quint32))
				{
					bufStream << *ptr;
					++ptr;
				}
				bufStream.setByteOrder(QDataStream::BigEndian);

				// write out the data from our buffer to the file
				if(m_pData->file->write(bufArray.data(), toWrite) != (qint64)toWrite)
				{
					result = false;
					break;
				}
				
				// increase our read counter
				written += toWrite;
			}
		}
		break;
		
		// VAX_Rx4 is a little endian float value, so we
		// need to set the stream to little endian, read the data via the
		// QDataStream operators to ensure correct byte swapping			
		case CECATSubHeader::VAX_Rx4:
		{
			// the endianess conversion takes quite some time, so
			// what we do is to us a temporarly buffer to which we read
			// some data from our fileStream and read out to our QByteArray
			QByteArray bufArray(8192, 0); // write in 8KB chunks
			float* ptr = (float*)matrixData;
			for(unsigned int written=0; written < matrixSize;)
			{
				unsigned int toWrite = matrixSize-written >= 8192 ? 8192 : matrixSize-written;

				// check if the curRead value is divide able through our data type 
				ASSERT(toWrite % sizeof(float) == 0);

				// now that we have our chunk we use a bufferStream to stream
				// in the values to it for making sure our data is correctly
				// converted regarding to little/big endianess
				QDataStream bufStream(&bufArray, QIODevice::WriteOnly);
				bufStream.setByteOrder(QDataStream::LittleEndian);
				for(unsigned int i=0; i < toWrite; i+=sizeof(float))
				{
					bufStream << *ptr;
					++ptr;
				}
				bufStream.setByteOrder(QDataStream::BigEndian);

				// write out the data from our buffer to the file
				if(m_pData->file->write(bufArray.data(), toWrite) != (qint64)toWrite)
				{
					result = false;
					break;
				}
				
				// increase our read counter
				written += toWrite;
			}
		}
		break;

		// IEEEFloat is defined to be a big endian float value. As the QDataStream
		// is per default big endian, we don't have to set it to another byte order
		// and just use the QDataStream operators to ensure correct byte swapping
		case CECATSubHeader::IEEEFloat:
		{
			// the endianess conversion takes quite some time, so
			// what we do is to us a temporarly buffer to which we read
			// some data from our fileStream and read out to our QByteArray
			QByteArray bufArray(8192, 0); // write in 8KB chunks
			float* ptr = (float*)matrixData;
			for(unsigned int written=0; written < matrixSize;)
			{
				unsigned int toWrite = matrixSize-written >= 8192 ? 8192 : matrixSize-written;

				// check if the curRead value is divide able through our data type 
				ASSERT(toWrite % sizeof(float) == 0);

				// now that we have our chunk we use a bufferStream to stream
				// in the values to it for making sure our data is correctly
				// converted regarding to little/big endianess
				QDataStream bufStream(&bufArray, QIODevice::WriteOnly);
				for(unsigned int i=0; i < toWrite; i+=sizeof(float))
				{
					bufStream << *ptr;
					++ptr;
				}

				// write out the data from our buffer to the file
				if(m_pData->file->write(bufArray.data(), toWrite) != (qint64)toWrite)
				{
					result = false;
					break;
				}
				
				// increase our read counter
				written += toWrite;
			}
		}
		break;

		// SunShort is defined to be a big endian short value. As the QDataStream
		// is per default big endian, we don't have to set it to another byte order
		// and just use the QDataStream operators to ensure correct byte swapping
		case CECATSubHeader::SunShort:
		{
			// the endianess conversion takes quite some time, so
			// what we do is to us a temporarly buffer to which we read
			// some data from our fileStream and read out to our QByteArray
			QByteArray bufArray(8192, 0); // write in 8KB chunks
			quint16* ptr = (quint16*)matrixData;
			for(unsigned int written=0; written < matrixSize;)
			{
				unsigned int toWrite = matrixSize-written >= 8192 ? 8192 : matrixSize-written;

				// check if the curRead value is divide able through our data type 
				ASSERT(toWrite % sizeof(quint16) == 0);

				// now that we have our chunk we use a bufferStream to stream
				// in the values to it for making sure our data is correctly
				// converted regarding to little/big endianess
				QDataStream bufStream(&bufArray, QIODevice::WriteOnly);
				for(unsigned int i=0; i < toWrite; i+=sizeof(quint16))
				{
					bufStream << *ptr;
					++ptr;
				}

				// write out the data from our buffer to the file
				if(m_pData->file->write(bufArray.data(), toWrite) != (qint64)toWrite)
				{
					result = false;
					break;
				}
				
				// increase our read counter
				written += toWrite;
			}

		}
		break;

		// SunLong is defined to be a big endian long value. As the QDataStream
		// is per default big endian, we don't have to set it to another byte order
		// and just use the QDataStream operators to ensure correct byte swapping
		case CECATSubHeader::SunLong:
		{
			// the endianess conversion takes quite some time, so
			// what we do is to us a temporarly buffer to which we read
			// some data from our fileStream and read out to our QByteArray
			QByteArray bufArray(8192, 0); // write in 8KB chunks
			quint32* ptr = (quint32*)matrixData;
			for(unsigned int written=0; written < matrixSize;)
			{
				unsigned int toWrite = matrixSize-written >= 8192 ? 8192 : matrixSize-written;

				// check if the curRead value is divide able through our data type 
				ASSERT(toWrite % sizeof(quint32) == 0);

				// now that we have our chunk we use a bufferStream to stream
				// in the values to it for making sure our data is correctly
				// converted regarding to little/big endianess
				QDataStream bufStream(&bufArray, QIODevice::WriteOnly);
				for(unsigned int i=0; i < toWrite; i+=sizeof(quint32))
				{
					bufStream << *ptr;
					++ptr;
				}

				// write out the data from our buffer to the file
				if(m_pData->file->write(bufArray.data(), toWrite) != (qint64)toWrite)
				{
					result = false;
					break;
				}
				
				// increase our read counter
				written += toWrite;
			}

		}
		break;
	}		

	if(result)
	{
		// check if the data we have written is divideable through the ECAT_BLOCKSIZE
		// or we have to add some more NULL bytes until we have 512 byte aligned data
		// because the ECAT standard defines ECAT files to be always dividable through
		// 512 byte blocks.
		if((matrixSize % ECAT_BLOCKSIZE) > 0)
		{
			unsigned int fillLen = ECAT_BLOCKSIZE - (matrixSize % ECAT_BLOCKSIZE);
			char fillData[fillLen];

			memset(fillData, 0, fillLen*sizeof(char));

			if(m_pData->file->write(fillData, fillLen) != (qint64)fillLen)
			{
				result = false;
				E("Error occurred while trying to write %d NULL bytes.", fillLen);
			}
			else
			{
				matrixSize += fillLen;
				W("matrixsize %% ECAT_BLOCKSIZE != 0. added %d NULL bytes", fillLen);
			}
		}

		if(result)
		{
			// at the end of our operation we calculate the new EndPosition
			m_pData->dataBlock_End = m_pData->dataBlock_Start+subHeader.rawDataSize()+matrixSize-ECAT_BLOCKSIZE;
			m_pData->status = CECATDirectoryItem::Finished;
		}
	}

	RETURN(result);
	return result;
}

QDataStream& operator<<(QDataStream& stream, const CECATDirectoryItem& dItem)
{
	ENTER();

	// first convert and write out the matrixID
	quint32 matrixID = convertToMatrixID(dItem.m_pData->frame,
																			 dItem.m_pData->plane,
																			 dItem.m_pData->gate,
																			 dItem.m_pData->bed,
																			 dItem.m_pData->data);
	stream << matrixID;
	
	// then we read out the rest
	quint32 dataBlock_Start = FilePos2ECATBlock(dItem.m_pData->dataBlock_Start);
	stream << dataBlock_Start;

	quint32 dataBlock_End = FilePos2ECATBlock(dItem.m_pData->dataBlock_End);
	stream << dataBlock_End;
	
	qint32 matrixStatus = static_cast<qint32>(dItem.m_pData->status);
	stream << matrixStatus;

	D("DItem.Matrix_ID       : %08lx (%d/%d/%d/%d/%d)", matrixID,
																											dItem.m_pData->frame,
																											dItem.m_pData->plane,
																											dItem.m_pData->gate,
																											dItem.m_pData->bed,
																											dItem.m_pData->data);

	D("DItem.DataBlock_Start : %lld (%lld)", FilePos2ECATBlock(dItem.m_pData->dataBlock_Start), dItem.m_pData->dataBlock_Start);
	D("DItem.DataBlock_End   : %lld (%lld)", FilePos2ECATBlock(dItem.m_pData->dataBlock_End), dItem.m_pData->dataBlock_End);
	D("DItem.DataBlock_Status: %d", dItem.m_pData->status);
	
	LEAVE();
	return stream;
}

QDataStream& operator>>(QDataStream& stream, CECATDirectoryItem& dItem)
{
	ENTER();

	// first read and convert the matrixID
	quint32 matrixID;
	stream >> matrixID;
	dItem.m_pData->frame = matrixID2Frame(matrixID);
	dItem.m_pData->plane = matrixID2Plane(matrixID);
	dItem.m_pData->gate  = matrixID2Gate(matrixID);
	dItem.m_pData->bed   = matrixID2Bed(matrixID);
	dItem.m_pData->data  = matrixID2Data(matrixID);

	// then we read out the rest
	quint32 dataBlock_Start;
	stream >> dataBlock_Start;
	dItem.m_pData->dataBlock_Start = ECATBlock2FilePos(dataBlock_Start);

	quint32 dataBlock_End;
	stream >> dataBlock_End;
	dItem.m_pData->dataBlock_End = ECATBlock2FilePos(dataBlock_End);

	qint32 matrixStatus;
	stream >> matrixStatus;
	dItem.m_pData->status = static_cast<CECATDirectoryItem::AccessStatus>(matrixStatus);

	// output some debug information.
	D("DItem.Matrix_ID       : %08lx (%d/%d/%d/%d/%d)", matrixID,
																											dItem.m_pData->frame,
																											dItem.m_pData->plane,
																											dItem.m_pData->gate,
																											dItem.m_pData->bed,
																											dItem.m_pData->data);

	D("DItem.DataBlock_Start : %ld (%ld)", dItem.m_pData->dataBlock_Start, FilePos2ECATBlock(dItem.m_pData->dataBlock_Start));
	D("DItem.DataBlock_End   : %ld (%ld)", dItem.m_pData->dataBlock_End, FilePos2ECATBlock(dItem.m_pData->dataBlock_End));
	D("DItem.DataBlock_Status: %d", dItem.m_pData->status);
	
	LEAVE();
	return stream;
}
