/* vim:set ts=2 nowrap: ****************************************************

 LMMC - ListMode Movement Correction
 Copyright (C) 2003 by Jens Langner <Jens.Langner@light-speed.de>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
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

CECATDirectoryItem::CECATDirectoryItem(CECATSubHeader::Type subHeaderType) 
	: m_pECATFile(NULL),
		m_FilePosition(-1),
		m_pSubHeader(NULL),
		m_pMatrixData(NULL)
{
	// make sure our entry structure is really 16bytes wide
	ASSERT(sizeof(m_Entry) == 16);

	// clear our item structure
	memset(&m_Entry, 0, sizeof(m_Entry));

	// lets prepare the SubHeader depending on the type of the
	// ECATFile
	SHOWVALUE(subHeaderType);
	switch(subHeaderType)
	{
		case CECATSubHeader::ECAT7_AttenCorr:	m_pSubHeader = new CECAT7SubHeaderAttenCorr();	break;
		case CECATSubHeader::ECAT7_Image:			m_pSubHeader = new CECAT7SubHeaderImage();			break;
		case CECATSubHeader::ECAT7_Norm:			m_pSubHeader = new CECAT7SubHeaderNorm();				break;
		case CECATSubHeader::ECAT7_Norm3D:		m_pSubHeader = new CECAT7SubHeaderNorm3D();			break;
		case CECATSubHeader::ECAT7_PolarMap:	m_pSubHeader = new CECAT7SubHeaderPolarMap();		break;
		case CECATSubHeader::ECAT7_Scan:			m_pSubHeader = new CECAT7SubHeaderScan();				break;
		case CECATSubHeader::ECAT7_Scan3D:		m_pSubHeader = new CECAT7SubHeaderScan3D();			break;

		default:
		{
			E("ECAT type isn't specified or not supported yet.");
		}
	}
}

bool CECATDirectoryItem::loadFromFile(CECATFile* pECATFile, long filePosition)
{
	bool result = false;
	
	if(pECATFile)
	{
		bool openedByUs = false;

		// check if the file associated with this item is already open or not
		if(pECATFile->isReadable() == false)
		{
			pECATFile->open(IO_ReadOnly);
			openedByUs = true;
		}
		
		// set the file to the fileposition we initialized this item to
		m_FilePosition = filePosition;
		pECATFile->at(m_FilePosition);

		// first we read out the Entry information in our help structure
		QDataStream stream(pECATFile);

		// lets stream out each structure element separatly so that we
		// maintain the correct endianess
		stream >> m_Entry.Matrix_ID;
		stream >> m_Entry.Matrix_SubHeader_StartPosition;
		stream >> m_Entry.Matrix_DataBlock_EndPosition;
		stream >> m_Entry.Matrix_Status;

		// output some debug information.
		D("DItem.Matrix_ID    : %08lx", m_Entry.Matrix_ID);
		D("DItem.Matrix_Start : %d",		m_Entry.Matrix_SubHeader_StartPosition);
		D("DItem.Matrix_End   : %d",		m_Entry.Matrix_DataBlock_EndPosition);
		D("DItem.Matrix_Status: %d",		m_Entry.Matrix_Status);

		// now that we have generated the correct SubHeader object we go
		// and read out the information we need
		if(m_pSubHeader)
		{
			// now that we have identify and load the SubHeader
			// but before we load the SubHeader via a stream operation, we
			// have to "remember" the current file position so that we can 
			// jump back after we got the subheader information
			long posAfterDItem = pECATFile->at();

			// set the file to the start position of the SubHeader of this
			// directory item and load the subheader information from the stream
			pECATFile->at(ECATBlock2FilePos(m_Entry.Matrix_SubHeader_StartPosition));
			m_pSubHeader->load(stream);

			// set the IODevice position back to the position after the DItem,
			// so that other following operations can start from this
			// position
			pECATFile->at(posAfterDItem);
		}

		if(openedByUs) pECATFile->close();

		// because we don't load the matrixdata now we have to save the
		// pointer to the ECATFile in our instance data for later reference
		m_pECATFile = pECATFile;

		result=true;
	}

	return result;
}

bool CECATDirectoryItem::saveToFile(CECATFile* pECATFile)
{
	bool result = false;

	// we only continue if the file is actually writeable
	if(pECATFile->isWritable() && m_pMatrixData)
	{
		QDataStream stream(pECATFile);
		
		// first we stream out the SubHeader
		m_pSubHeader->save(stream);

		// then we process the matrix data that is associated with
		// this directoryitem.
		// here we have to care about the correct endianess, so that
		// we convert BIG->LITTLE or vise versa depending on the
		// system we are running.
		switch(m_pSubHeader->data_Type())
		{
			case CECATSubHeader::UnkownMatrixDataType:
			{
				W("No or an unknown data type was set for the matrix data of dirItem %ld", m_Entry.Matrix_ID);
				stream.writeRawBytes(m_pMatrixData->data(), m_pMatrixData->count());	
			}
			break;

			// write out Byte data. (1 byte)
			case CECATSubHeader::ByteData:
			{
				// we use the RawBytes() method here.
				stream.writeRawBytes(m_pMatrixData->data(), m_pMatrixData->count());
			}
			break;

			// VAX_Ix2 is a little endian short value, so we
			// need to set the stream to little endian, write the data via the
			// QDataStream operators to ensure correct byte swapping
			case CECATSubHeader::VAX_Ix2:
			{
				stream.setByteOrder(QDataStream::LittleEndian);
				Q_UINT16* ptr = (Q_UINT16*)m_pMatrixData->data();
				for(unsigned int i=0; i < m_pMatrixData->count(); i+=sizeof(Q_UINT16))
				{
					stream << *ptr;
					++ptr;
				}
				stream.setByteOrder(QDataStream::BigEndian);
			}
			break;

			// VAX_Ix4 is a little endian long value, so we
			// need to set the stream to little endian, write the data via the
			// QDataStream operators to ensure correct byte swapping
			case CECATSubHeader::VAX_Ix4:
			{
				stream.setByteOrder(QDataStream::LittleEndian);
				Q_UINT32* ptr = (Q_UINT32*)m_pMatrixData->data();
				for(unsigned int i=0; i < m_pMatrixData->count(); i+=sizeof(Q_UINT32))
				{
					stream << *ptr;
					++ptr;
				}
				stream.setByteOrder(QDataStream::BigEndian);
			}
			break;
			
			// VAX_Rx4 is a little endian float value, so we
			// need to set the stream to little endian, write the data via the
			// QDataStream operators to ensure correct byte swapping			
			case CECATSubHeader::VAX_Rx4:
			{
				stream.setByteOrder(QDataStream::LittleEndian);
				float* ptr = (float*)m_pMatrixData->data();
				for(unsigned int i=0; i < m_pMatrixData->count(); i+=sizeof(float))
				{
					stream << *ptr;
					++ptr;
				}
				stream.setByteOrder(QDataStream::BigEndian);
			}
			break;

			// IEEEFloat is defined to be a big endian float value. As the QDataStream
			// is per default big endian, we don't have to set it to another byte order
			// and just use the QDataStream operators to ensure correct byte swapping
			case CECATSubHeader::IEEEFloat:
			{
				float* ptr = (float*)m_pMatrixData->data();
				for(unsigned int i=0; i < m_pMatrixData->count(); i+=sizeof(float))
				{
					stream << *ptr;
					++ptr;
				}
			}
			break;

			// SunShort is defined to be a big endian short value. As the QDataStream
			// is per default big endian, we don't have to set it to another byte order
			// and just use the QDataStream operators to ensure correct byte swapping
			case CECATSubHeader::SunShort:
			{
				Q_UINT16* ptr = (Q_UINT16*)m_pMatrixData->data();
				for(unsigned int i=0; i < m_pMatrixData->count(); i+=sizeof(Q_UINT16))
				{
					stream << *ptr;
					++ptr;
				}
			}
			break;

			// SunLong is defined to be a big endian long value. As the QDataStream
			// is per default big endian, we don't have to set it to another byte order
			// and just use the QDataStream operators to ensure correct byte swapping
			case CECATSubHeader::SunLong:
			{
				Q_UINT32* ptr = (Q_UINT32*)m_pMatrixData->data();
				for(unsigned int i=0; i < m_pMatrixData->count(); i+=sizeof(Q_UINT32))
				{
					stream << *ptr;
					++ptr;
				}
			}
			break;
		}
	}
	
	return result;
}

bool CECATDirectoryItem::loadMatrixData()
{
	bool result = false;

	// we load the MatrixData out of the ECATFile manually but only
	// if the access status of this item is set to "Finished"
	if(m_Entry.Matrix_Status == Finished && m_pECATFile && m_pSubHeader)
	{
		bool openedByUs = false;

		// check if the file associated with this item is already open or not
		if(m_pECATFile->isReadable() == false)
		{
			if(m_pECATFile->open(IO_ReadOnly) == false) return false;
			openedByUs = true;
		}

		// set the ECATFile handle to the correct position of
		// the matrix start. This should normally be
		// Matrix_SubHeader_StartPosition+1
		m_pECATFile->at(ECATBlock2FilePos(m_Entry.Matrix_SubHeader_StartPosition+1));

		// then we allocate some memory for loading the matrixdata
		// this should be:
		// Size = (Matrix_DataBlock_EndPosition-Matrix_SubHeader_StartPosition+1)*512
		m_pMatrixData = new QByteArray((m_Entry.Matrix_DataBlock_EndPosition-m_Entry.Matrix_SubHeader_StartPosition+1)*512);

		// now that we have our MatrixData we can
		// fill it with the raw data from the file
		QDataStream stream(m_pECATFile);

		// then we process the matrix data that is associated with
		// this directoryitem.
		// here we have to care about the correct endianess, so that
		// we convert BIG->LITTLE or vise versa depending on the
		// system we are running.
		switch(m_pSubHeader->data_Type())
		{
			case CECATSubHeader::UnkownMatrixDataType:
			{
				W("No or an unknown data type was set for the matrix data of dirItem %ld", m_Entry.Matrix_ID);
				stream.readRawBytes(m_pMatrixData->data(), m_pMatrixData->size());	
			}
			break;

			// write out Byte data. (1 byte)
			case CECATSubHeader::ByteData:
			{
				// we use the RawBytes() method here.
				stream.readRawBytes(m_pMatrixData->data(), m_pMatrixData->size());
			}
			break;

			// VAX_Ix2 is a little endian short value, so we
			// need to set the stream to little endian, read the data via the
			// QDataStream operators to ensure correct byte swapping
			case CECATSubHeader::VAX_Ix2:
			{
				stream.setByteOrder(QDataStream::LittleEndian);
				Q_UINT16* ptr = (Q_UINT16*)m_pMatrixData->data();
				for(unsigned int i=0; i < m_pMatrixData->size(); i+=sizeof(Q_UINT16))
				{
					stream >> *ptr;
					++ptr;
				}
				stream.setByteOrder(QDataStream::BigEndian);
			}
			break;

			// VAX_Ix4 is a little endian long value, so we
			// need to set the stream to little endian, read the data via the
			// QDataStream operators to ensure correct byte swapping
			case CECATSubHeader::VAX_Ix4:
			{
				stream.setByteOrder(QDataStream::LittleEndian);
				Q_UINT32* ptr = (Q_UINT32*)m_pMatrixData->data();
				for(unsigned int i=0; i < m_pMatrixData->size(); i+=sizeof(Q_UINT32))
				{
					stream >> *ptr;
					++ptr;
				}
				stream.setByteOrder(QDataStream::BigEndian);
			}
			break;
			
			// VAX_Rx4 is a little endian float value, so we
			// need to set the stream to little endian, read the data via the
			// QDataStream operators to ensure correct byte swapping			
			case CECATSubHeader::VAX_Rx4:
			{
				stream.setByteOrder(QDataStream::LittleEndian);
				float* ptr = (float*)m_pMatrixData->data();
				for(unsigned int i=0; i < m_pMatrixData->size(); i+=sizeof(float))
				{
					stream >> *ptr;
					++ptr;
				}
				stream.setByteOrder(QDataStream::BigEndian);
			}
			break;

			// IEEEFloat is defined to be a big endian float value. As the QDataStream
			// is per default big endian, we don't have to set it to another byte order
			// and just use the QDataStream operators to ensure correct byte swapping
			case CECATSubHeader::IEEEFloat:
			{
				float* ptr = (float*)m_pMatrixData->data();
				for(unsigned int i=0; i < m_pMatrixData->size(); i+=sizeof(float))
				{
					stream >> *ptr;
					++ptr;
				}
			}
			break;

			// SunShort is defined to be a big endian short value. As the QDataStream
			// is per default big endian, we don't have to set it to another byte order
			// and just use the QDataStream operators to ensure correct byte swapping
			case CECATSubHeader::SunShort:
			{
				Q_UINT16* ptr = (Q_UINT16*)m_pMatrixData->data();
				for(unsigned int i=0; i < m_pMatrixData->size(); i+=sizeof(Q_UINT16))
				{
					stream >> *ptr;
					++ptr;
				}
			}
			break;

			// SunLong is defined to be a big endian long value. As the QDataStream
			// is per default big endian, we don't have to set it to another byte order
			// and just use the QDataStream operators to ensure correct byte swapping
			case CECATSubHeader::SunLong:
			{
				Q_UINT32* ptr = (Q_UINT32*)m_pMatrixData->data();
				for(unsigned int i=0; i < m_pMatrixData->size(); i+=sizeof(Q_UINT32))
				{
					stream >> *ptr;
					++ptr;
				}
			}
			break;
		}		
	
		// and close the ECATFile
		if(openedByUs)
			m_pECATFile->close();

		result = true;
	}

	return result;
}

QByteArray* CECATDirectoryItem::getMatrix(void)
{
	// first we check wheter we already have loaded the Matrix
	// data and if not we have to first load the whole data into
	// our private ByteArray
	if(m_pMatrixData == NULL && loadMatrixData() == false)
	{
		return NULL;
	}
	
	return m_pMatrixData;
}

void* CECATDirectoryItem::getMatrixData(void)
{
	QByteArray* mData = getMatrix();

	if(mData)
		return mData->data();

	return NULL;
}

void CECATDirectoryItem::setMatrixData(void* pMatrix, unsigned int size)
{
	if(m_pMatrixData)
	{
		m_pMatrixData->resetRawData(static_cast<char*>(pMatrix), size);
	}
	else
	{
		m_pMatrixData = new QByteArray(size);
		m_pMatrixData->setRawData(static_cast<char*>(pMatrix), size);
	}
}
