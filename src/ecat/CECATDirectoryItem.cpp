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

#include "CECATDirectoryItem.h"
#include "CECATFile.h"

#include "CECAT7SubHeaderAttenCorr.h"
#include "CECAT7SubHeaderImage.h"
#include "CECAT7SubHeaderNorm.h"
#include "CECAT7SubHeaderNorm3D.h"
#include "CECAT7SubHeaderPolarMap.h"
#include "CECAT7SubHeaderScan.h"
#include "CECAT7SubHeaderScan3D.h"

#include <qdatastream.h>

#include "debug.h"

CECATDirectoryItem::CECATDirectoryItem(CECATFile* pFile,
																			 CECATSubHeader::Type subHeaderType,
																			 Q_UINT32 matrixID) 
	: m_pECATFile(pFile),
		m_iSubHeaderType(subHeaderType),
		m_iFrame(-1),
		m_iPlane(-1),
		m_iGate(-1),
		m_iBed(-1),
		m_iData(-1),
		m_iDataBlock_Start(0),
		m_iDataBlock_End(0),
		m_iStatus(Deleted)
{
	ENTER();

	if(matrixID != 0)
	{
		m_iFrame = matrixID2Frame(matrixID);
		m_iPlane = matrixID2Plane(matrixID);
		m_iGate  = matrixID2Gate(matrixID);
		m_iBed   = matrixID2Bed(matrixID);
		m_iData  = matrixID2Data(matrixID);
	}
	
	LEAVE();
}

bool CECATDirectoryItem::readSubHeader(CECATSubHeader*& subHeader)
{
	ENTER();

	// lets prepare the SubHeader depending on the type of the
	// ECATFile
	SHOWVALUE(m_iSubHeaderType);
	switch(m_iSubHeaderType)
	{
		case CECATSubHeader::ECAT7_AttenCorr: 
			subHeader = new CECAT7SubHeaderAttenCorr(m_pECATFile, this);	
		break;

		case CECATSubHeader::ECAT7_Image:	
			subHeader = new CECAT7SubHeaderImage(m_pECATFile, this);
		break;
		
		case CECATSubHeader::ECAT7_Norm:
			subHeader = new CECAT7SubHeaderNorm(m_pECATFile, this);
		break;
		
		case CECATSubHeader::ECAT7_Norm3D:
			subHeader = new CECAT7SubHeaderNorm3D(m_pECATFile, this);
		break;
		
		case CECATSubHeader::ECAT7_PolarMap:
			subHeader = new CECAT7SubHeaderPolarMap(m_pECATFile, this);
		break;
		
		case CECATSubHeader::ECAT7_Scan:
			subHeader = new CECAT7SubHeaderScan(m_pECATFile, this);
		break;
		
		case CECATSubHeader::ECAT7_Scan3D:
			subHeader = new CECAT7SubHeaderScan3D(m_pECATFile, this);
		break;

		default:
		{
			E("ECAT type isn't specified or not supported yet.");
		}
	}

	bool result = false;
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

	RETURN(result);
	return result;
}

bool CECATDirectoryItem::readMatrix(QByteArray*& matrixData)
{
	bool result = false;
	char* data = NULL;
	unsigned int dataLen = 0;

	// we use our method operating on a raw char pointer
	if(readMatrix(data, dataLen) && dataLen > 0)
	{
		matrixData = new QByteArray();
		matrixData->setRawData(data, dataLen);

		result = true;
	}

	RETURN(result);
	return result;
}

bool CECATDirectoryItem::readMatrix(char*& matrixData, unsigned int& matrixSize)
{
	bool result = true;
		
	// check if the file associated with this item is readable or
	// not
	if(m_pECATFile && m_pECATFile->isReadable() == false)
		return false;

	// we load the MatrixData out of the ECATFile manually but only
	// if the access status of this item is set to "Finished"
	if(m_iStatus == Finished)
	{
		// before we can read in the raw data we need to read in
		// the Subheader so that we know the data type of our raw
		// data
		CECATSubHeader* subHeader = NULL;
		if(readSubHeader(subHeader) == false || subHeader == NULL)
			return false;

		// set the ECATFile handle to the correct position of
		// the matrix start. This should normally be
		// Matrix_SubHeader_StartPosition+1
		m_pECATFile->at(m_iDataBlock_Start+subHeader->size());

		// then we allocate some memory for loading the matrixdata
		// this should be:
		matrixSize = m_iDataBlock_End-m_iDataBlock_Start-subHeader->size();
		matrixData = new char[matrixSize];

		// then we process the matrix data that is associated with
		// this directoryitem.
		// here we have to care about the correct endianess, so that
		// we convert BIG->LITTLE or vise versa depending on the
		// system we are running.
		switch(subHeader->data_Type())
		{
			case CECATSubHeader::UnkownMatrixDataType:
			{
				W("No or an unknown data type was set for the matrix data of dirItem %08lx",
						convertToMatrixID(m_iFrame, m_iPlane, m_iGate, m_iBed, m_iData));

				if(m_pECATFile->readBlock(matrixData, matrixSize) > 0)
					result = true;
			}
			break;

			// write out Byte data. (1 byte)
			case CECATSubHeader::ByteData:
			{
				if(m_pECATFile->readBlock(matrixData, matrixSize) > 0)
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
				QByteArray bufArray(8192); // read 8KB chunks
				for(unsigned int read=0; read < matrixSize;)
				{
					unsigned int toRead = matrixSize-read >= 8192 ? 8192 : matrixSize-read;
					unsigned int curRead = m_pECATFile->readBlock(bufArray.data(), toRead);
					if(curRead != toRead)
					{
						result = false;
						break;
					}

					// check if the curRead value is divide able through our data type 
					ASSERT(curRead % sizeof(Q_UINT16) == 0);

					// now that we have our chunk we use a bufferStream to stream
					// out the values from it for making sure our data is correctly
					// converted regarding to little/big endianess
					QDataStream bufStream(bufArray, IO_ReadOnly);
					bufStream.setByteOrder(QDataStream::LittleEndian);
					Q_UINT16* ptr = (Q_UINT16*)matrixData;
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

			// VAX_Ix4 is a little endian long value, so we
			// need to set the stream to little endian, read the data via the
			// QDataStream operators to ensure correct byte swapping
			case CECATSubHeader::VAX_Ix4:
			{
				// the endianess conversion takes quite some time, so
				// what we do is to us a temporarly buffer to which we read
				// some data from our fileStream and read out to our QByteArray
				QByteArray bufArray(8192); // read 8KB chunks
				for(unsigned int read=0; read < matrixSize;)
				{
					unsigned int toRead = matrixSize-read >= 8192 ? 8192 : matrixSize-read;
					unsigned int curRead = m_pECATFile->readBlock(bufArray.data(), toRead);
					if(curRead != toRead)
					{
						result = false;
						break;
					}

					// check if the curRead value is divide able through our data type 
					ASSERT(curRead % sizeof(Q_UINT32) == 0);

					// now that we have our chunk we use a bufferStream to stream
					// out the values from it for making sure our data is correctly
					// converted regarding to little/big endianess
					QDataStream bufStream(bufArray, IO_ReadOnly);
					bufStream.setByteOrder(QDataStream::LittleEndian);
					Q_UINT32* ptr = (Q_UINT32*)matrixData;
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
			
			// VAX_Rx4 is a little endian float value, so we
			// need to set the stream to little endian, read the data via the
			// QDataStream operators to ensure correct byte swapping			
			case CECATSubHeader::VAX_Rx4:
			{
				// the endianess conversion takes quite some time, so
				// what we do is to us a temporarly buffer to which we read
				// some data from our fileStream and read out to our QByteArray
				QByteArray bufArray(8192); // read 8KB chunks
				for(unsigned int read=0; read < matrixSize;)
				{
					unsigned int toRead = matrixSize-read >= 8192 ? 8192 : matrixSize-read;
					unsigned int curRead = m_pECATFile->readBlock(bufArray.data(), toRead);
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
					QDataStream bufStream(bufArray, IO_ReadOnly);
					bufStream.setByteOrder(QDataStream::LittleEndian);
					float* ptr = (float*)matrixData;
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
				QByteArray bufArray(8192); // read 8KB chunks
				for(unsigned int read=0; read < matrixSize;)
				{
					unsigned int toRead = matrixSize-read >= 8192 ? 8192 : matrixSize-read;
					unsigned int curRead = m_pECATFile->readBlock(bufArray.data(), toRead);
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
					QDataStream bufStream(bufArray, IO_ReadOnly);
					float* ptr = (float*)matrixData;
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
				QByteArray bufArray(8192); // read 8KB chunks
				for(unsigned int read=0; read < matrixSize;)
				{
					unsigned int toRead = matrixSize-read >= 8192 ? 8192 : matrixSize-read;
					unsigned int curRead = m_pECATFile->readBlock(bufArray.data(), toRead);
					if(curRead != toRead)
					{
						result = false;
						break;
					}

					// check if the curRead value is divide able through our data type 
					ASSERT(curRead % sizeof(Q_UINT16) == 0);

					// now that we have our chunk we use a bufferStream to stream
					// out the values from it for making sure our data is correctly
					// converted regarding to little/big endianess
					QDataStream bufStream(bufArray, IO_ReadOnly);
					Q_UINT16* ptr = (Q_UINT16*)matrixData;
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

			// SunLong is defined to be a big endian long value. As the QDataStream
			// is per default big endian, we don't have to set it to another byte order
			// and just use the QDataStream operators to ensure correct byte swapping
			case CECATSubHeader::SunLong:
			{
				// the endianess conversion takes quite some time, so
				// what we do is to us a temporarly buffer to which we read
				// some data from our fileStream and read out to our QByteArray
				QByteArray bufArray(8192); // read 8KB chunks
				for(unsigned int read=0; read < matrixSize;)
				{
					unsigned int toRead = matrixSize-read >= 8192 ? 8192 : matrixSize-read;
					unsigned int curRead = m_pECATFile->readBlock(bufArray.data(), toRead);
					if(curRead != toRead)
					{
						result = false;
						break;
					}
					
					// check if the curRead value is divide able through our data type 
					ASSERT(curRead % sizeof(Q_UINT32) == 0);

					// now that we have our chunk we use a bufferStream to stream
					// out the values from it for making sure our data is correctly
					// converted regarding to little/big endianess
					QDataStream bufStream(bufArray, IO_ReadOnly);
					Q_UINT32* ptr = (Q_UINT32*)matrixData;
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

		// now we delete the subHeader we loaded temporarly
		delete subHeader;
	}
	else
		result = false;

	RETURN(result);
	return result;
}

bool CECATDirectoryItem::writeSubHeader(const CECATSubHeader& subHeader)
{
	ENTER();
	bool result = false;

	// let us check if the passed subheader is an appropiate one
	if(m_iSubHeaderType != CECATSubHeader::Unknown)
	{
		if(m_iSubHeaderType != subHeader.subHeaderType())
			return false;
	}
	else
		m_iSubHeaderType = subHeader.subHeaderType();
	
	// save the SubHeader to the stream now
	result = subHeader.save();

	RETURN(result);
	return result;
}

bool CECATDirectoryItem::writeMatrix(const QByteArray& matrixData)
{
	ENTER();
  bool result;

	result = writeMatrix(matrixData.data(), matrixData.size());

	RETURN(result);
	return result;
}

bool CECATDirectoryItem::writeMatrix(const char* matrixData, unsigned int matrixSize)
{
	ENTER();
	bool result = true;
	
	// check if the file associated with this item is writeable at all
	if(m_pECATFile && m_pECATFile->isWritable() == false)
	{
		RETURN(false);
		return false;
	}

	// before we can write out the raw data we need to read in
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
	// Matrix_SubHeader_StartPosition+subHeaderSize
	m_pECATFile->at(m_iDataBlock_Start+subHeader->size());

	// then we process the matrix data that is associated with
	// this directoryitem.
	// here we have to care about the correct endianess, so that
	// we convert BIG->LITTLE or vise versa depending on the
	// system we are running.
	switch(subHeader->data_Type())
	{
		case CECATSubHeader::UnkownMatrixDataType:
		{
			W("No or an unknown data type was set for the matrix data of dirItem %08lx",
					convertToMatrixID(m_iFrame, m_iPlane, m_iGate, m_iBed, m_iData));
			
			if(m_pECATFile->writeBlock(matrixData, matrixSize) == matrixSize)
				result = true;
		}
		break;

		// write out Byte data. (1 byte)
		case CECATSubHeader::ByteData:
		{
			if(m_pECATFile->writeBlock(matrixData, matrixSize) == matrixSize)
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
			QByteArray bufArray(8192); // write in 8KB chunks
			Q_UINT16* ptr = (Q_UINT16*)matrixData;
			for(unsigned int written=0; written < matrixSize;)
			{
				unsigned int toWrite = matrixSize-written >= 8192 ? 8192 : matrixSize-written;

				// check if the curRead value is divide able through our data type 
				ASSERT(toWrite % sizeof(Q_UINT16) == 0);

				// now that we have our chunk we use a bufferStream to stream
				// in the values to it for making sure our data is correctly
				// converted regarding to little/big endianess
				QDataStream bufStream(bufArray, IO_WriteOnly);
				bufStream.setByteOrder(QDataStream::LittleEndian);
				for(unsigned int i=0; i < toWrite; i+=sizeof(Q_UINT16))
				{
					bufStream << *ptr;
					++ptr;
				}
				bufStream.setByteOrder(QDataStream::BigEndian);

				// write out the data from our buffer to the file
				if(m_pECATFile->writeBlock(bufArray.data(), toWrite) != toWrite)
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
			QByteArray bufArray(8192); // write in 8KB chunks
			Q_UINT32* ptr = (Q_UINT32*)matrixData;
			for(unsigned int written=0; written < matrixSize;)
			{
				unsigned int toWrite = matrixSize-written >= 8192 ? 8192 : matrixSize-written;

				// check if the curRead value is divide able through our data type 
				ASSERT(toWrite % sizeof(Q_UINT32) == 0);

				// now that we have our chunk we use a bufferStream to stream
				// in the values to it for making sure our data is correctly
				// converted regarding to little/big endianess
				QDataStream bufStream(bufArray, IO_WriteOnly);
				bufStream.setByteOrder(QDataStream::LittleEndian);
				for(unsigned int i=0; i < toWrite; i+=sizeof(Q_UINT16))
				{
					bufStream << *ptr;
					++ptr;
				}
				bufStream.setByteOrder(QDataStream::BigEndian);

				// write out the data from our buffer to the file
				if(m_pECATFile->writeBlock(bufArray.data(), toWrite) != toWrite)
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
			QByteArray bufArray(8192); // write in 8KB chunks
			float* ptr = (float*)matrixData;
			for(unsigned int written=0; written < matrixSize;)
			{
				unsigned int toWrite = matrixSize-written >= 8192 ? 8192 : matrixSize-written;

				// check if the curRead value is divide able through our data type 
				ASSERT(toWrite % sizeof(float) == 0);

				// now that we have our chunk we use a bufferStream to stream
				// in the values to it for making sure our data is correctly
				// converted regarding to little/big endianess
				QDataStream bufStream(bufArray, IO_WriteOnly);
				bufStream.setByteOrder(QDataStream::LittleEndian);
				for(unsigned int i=0; i < toWrite; i+=sizeof(Q_UINT16))
				{
					bufStream << *ptr;
					++ptr;
				}
				bufStream.setByteOrder(QDataStream::BigEndian);

				// write out the data from our buffer to the file
				if(m_pECATFile->writeBlock(bufArray.data(), toWrite) != toWrite)
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
			QByteArray bufArray(8192); // write in 8KB chunks
			float* ptr = (float*)matrixData;
			for(unsigned int written=0; written < matrixSize;)
			{
				unsigned int toWrite = matrixSize-written >= 8192 ? 8192 : matrixSize-written;

				// check if the curRead value is divide able through our data type 
				ASSERT(toWrite % sizeof(float) == 0);

				// now that we have our chunk we use a bufferStream to stream
				// in the values to it for making sure our data is correctly
				// converted regarding to little/big endianess
				QDataStream bufStream(bufArray, IO_WriteOnly);
				for(unsigned int i=0; i < toWrite; i+=sizeof(Q_UINT16))
				{
					bufStream << *ptr;
					++ptr;
				}

				// write out the data from our buffer to the file
				if(m_pECATFile->writeBlock(bufArray.data(), toWrite) != toWrite)
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
			QByteArray bufArray(8192); // write in 8KB chunks
			Q_UINT16* ptr = (Q_UINT16*)matrixData;
			for(unsigned int written=0; written < matrixSize;)
			{
				unsigned int toWrite = matrixSize-written >= 8192 ? 8192 : matrixSize-written;

				// check if the curRead value is divide able through our data type 
				ASSERT(toWrite % sizeof(Q_UINT16) == 0);

				// now that we have our chunk we use a bufferStream to stream
				// in the values to it for making sure our data is correctly
				// converted regarding to little/big endianess
				QDataStream bufStream(bufArray, IO_WriteOnly);
				for(unsigned int i=0; i < toWrite; i+=sizeof(Q_UINT16))
				{
					bufStream << *ptr;
					++ptr;
				}

				// write out the data from our buffer to the file
				if(m_pECATFile->writeBlock(bufArray.data(), toWrite) != toWrite)
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
			QByteArray bufArray(8192); // write in 8KB chunks
			Q_UINT32* ptr = (Q_UINT32*)matrixData;
			for(unsigned int written=0; written < matrixSize;)
			{
				unsigned int toWrite = matrixSize-written >= 8192 ? 8192 : matrixSize-written;

				// check if the curRead value is divide able through our data type 
				ASSERT(toWrite % sizeof(Q_UINT32) == 0);

				// now that we have our chunk we use a bufferStream to stream
				// in the values to it for making sure our data is correctly
				// converted regarding to little/big endianess
				QDataStream bufStream(bufArray, IO_WriteOnly);
				for(unsigned int i=0; i < toWrite; i+=sizeof(Q_UINT16))
				{
					bufStream << *ptr;
					++ptr;
				}

				// write out the data from our buffer to the file
				if(m_pECATFile->writeBlock(bufArray.data(), toWrite) != toWrite)
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

	// now we delete the subHeader we loaded temporarly
	delete subHeader;

	if(result)
	{
		// at the end of our operation we calculate the new EndPosition
		m_iDataBlock_End = m_pECATFile->at();
		m_iStatus = CECATDirectoryItem::Finished;
	}

	RETURN(result);
	return result;
}

QDataStream& operator<<(QDataStream& stream, const CECATDirectoryItem& dItem)
{
	ENTER();

	// first convert and write out the matrixID
	Q_UINT32 matrixID = convertToMatrixID(dItem.m_iFrame,
																				dItem.m_iPlane,
																				dItem.m_iGate,
																				dItem.m_iBed,
																				dItem.m_iData);
	stream << matrixID;
	
	// then we read out the rest
	Q_UINT32 dataBlock_Start = FilePos2ECATBlock(dItem.m_iDataBlock_Start);
	stream << dataBlock_Start;

	Q_UINT32 dataBlock_End = FilePos2ECATBlock(dItem.m_iDataBlock_End);
	stream << dataBlock_End;
	
	Q_INT32 matrixStatus = static_cast<Q_INT32>(dItem.m_iStatus);
	stream << matrixStatus;

	LEAVE();
	return stream;
}

QDataStream& operator>>(QDataStream& stream, CECATDirectoryItem& dItem)
{
	ENTER();

	// first read and convert the matrixID
	Q_UINT32 matrixID;
	stream >> matrixID;
	dItem.m_iFrame = matrixID2Frame(matrixID);
	dItem.m_iPlane = matrixID2Plane(matrixID);
	dItem.m_iGate  = matrixID2Gate(matrixID);
	dItem.m_iBed   = matrixID2Bed(matrixID);
	dItem.m_iData  = matrixID2Data(matrixID);

	// then we read out the rest
	Q_UINT32 dataBlock_Start;
	stream >> dataBlock_Start;
	dItem.m_iDataBlock_Start = ECATBlock2FilePos(dataBlock_Start);

	Q_UINT32 dataBlock_End;
	stream >> dataBlock_End;
	dItem.m_iDataBlock_End = ECATBlock2FilePos(dataBlock_End);

	Q_INT32 matrixStatus;
	stream >> matrixStatus;
	dItem.m_iStatus = static_cast<CECATDirectoryItem::AccessStatus>(matrixStatus);

	// output some debug information.
	D("DItem.Matrix_ID    : %08lx (%d/%d/%d/%d/%d)", matrixID,
																									 dItem.m_iFrame,
																									 dItem.m_iPlane,
																									 dItem.m_iGate,
																									 dItem.m_iBed,
																									 dItem.m_iData);

	D("DItem.Matrix_Start : %d", dItem.m_iDataBlock_Start);
	D("DItem.Matrix_End   : %d", dItem.m_iDataBlock_End);
	D("DItem.Matrix_Status: %d", dItem.m_iStatus);
	
	LEAVE();
	return stream;
}
