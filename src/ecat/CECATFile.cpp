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

#include "CECATFile.h"
#include "CECATMainHeader.h"
#include "CECAT6MainHeader.h"
#include "CECAT7MainHeader.h"
#include "CECATDirectory.h"

#include <qdatastream.h>

#include "debug.h"

CECATFile::CECATFile(const QString& filename, CECATMainHeader::Type fileType)
	: QFile(filename),
		m_ECATformat(Undefined),
		m_pMainHeader(NULL),
		m_pMainDirectory(NULL)
{
	if(fileType != CECATMainHeader::Unknown)
	{
		setFileType(fileType);
	}
}

CECATFile::~CECATFile()
{
	if(m_pMainDirectory)	delete m_pMainDirectory;
	if(m_pMainHeader)			delete m_pMainHeader;
}

bool CECATFile::load(void)
{
	// first we check if we already have some valid MainHeader/MainDirectory
	// combination and if so we need to delete it first.
	if(m_pMainHeader)			delete m_pMainHeader;
	if(m_pMainDirectory)	delete m_pMainDirectory;

	// we open up the file and read in the
	// main header and directory list of the ecat file
	if(this->open(IO_ReadOnly))
	{
		QDataStream stream(this);

		// but before we start reading the main header we check
		// the magic number of the file and if it exists it is
		// a ECAT7 file and if not we probably handle a ECAT6
		// filetype or an invalid format :/
		char magic_number[14];
		stream.readRawBytes(magic_number, 14*sizeof(char));

		// close the device here because the MainHeader and
		// DirectoryList classes will anyway open it on their
		// own
		this->close();

		// compare the loaded magic_number and if we found a "MATRIX"
		// string at the file start this is a ECAT7 fileformat.
		if(strncmp(magic_number, "MATRIX", 6) == 0)
		{
			// it is/should be a ECAT7 file, so we can instantiate a
			// ECAT7 MainHeader and let it read the MainHeader information
			// accordingly
			m_ECATformat = ECAT7;
			m_pMainHeader = new CECAT7MainHeader();
		}
		else
		{
			// according to the CTI ECAT implementation this could either
			// be a ECAT6 file or a unknown file format, but as we don't
			// know exactly we simply assume it is a ECAT6 format and
			// use a ECAT6 MainHeader
			m_ECATformat	= ECAT6;
			m_pMainHeader = new CECAT6MainHeader();
		}

		// now that we have created our proper MainHeader we try
		// to load the header information and then load the
		// ECAT DirectoryList out of the ECATFile.
		if(m_pMainHeader->load(this))
		{
			m_pMainDirectory = new CECATDirectory();
			if(m_pMainDirectory->load(this))
			{
				return true;
			}
		}
	}

	return false;
}

bool CECATFile::save(void)
{
	bool result=false;

	// saving "should" be easy. Just signal the MainHeader and
	// the main Directory to write it's data to a file.
	result = m_pMainHeader->save(this);
	if(result) result = m_pMainDirectory->save(this);

	return result;
}

CECATMainHeader::Type CECATFile::fileType(void)
{
	// if the Main header isn't loaded yet we return Unknown
	if(m_pMainHeader == NULL || m_ECATformat == Undefined)
		return CECATMainHeader::Unknown;
	
	// depending on the format and DataSetType the MainHeader
	// contains, we can specify the ECAT file type and return it here
	switch(m_ECATformat)
	{
		case CECATFile::ECAT7:
		{
			switch(static_cast<CECAT7MainHeader*>(m_pMainHeader)->file_Type())
			{
				// do a 1:1 mapping for all types
				case CECAT7MainHeader::Sinogram:					return CECATMainHeader::ECAT7_Sinogram;
				case CECAT7MainHeader::Image16:						return CECATMainHeader::ECAT7_Image16;
				case CECAT7MainHeader::AttenuationCorr:		return CECATMainHeader::ECAT7_AttenCorr;
				case CECAT7MainHeader::Normalization:			return CECATMainHeader::ECAT7_Normalization;
				case CECAT7MainHeader::PolarMap:					return CECATMainHeader::ECAT7_PolarMap;
				case CECAT7MainHeader::Volume8:						return CECATMainHeader::ECAT7_Volume8;
				case CECAT7MainHeader::Volume16:					return CECATMainHeader::ECAT7_Volume16;
				case CECAT7MainHeader::Projection8:				return CECATMainHeader::ECAT7_Projection8;
				case CECAT7MainHeader::Projection16:			return CECATMainHeader::ECAT7_Projection16;
				case CECAT7MainHeader::Image8:						return CECATMainHeader::ECAT7_Image8;			
				case CECAT7MainHeader::Sinogram3D_16:			return CECATMainHeader::ECAT7_Sinogram3D_16;
				case CECAT7MainHeader::Sinogram3D_8:			return CECATMainHeader::ECAT7_Sinogram3D_8;
				case CECAT7MainHeader::Normalization_3D:	return CECATMainHeader::ECAT7_Normalization_3D;
				case CECAT7MainHeader::Sinogram3D_Float:	return CECATMainHeader::ECAT7_Sinogram3D_Float;

				default:
				{
					W("ECAT type couldn't be identified.");
				}
			}
		}
		break;

		case CECATFile::ECAT6:
		{
			W("ECAT6 SubHeader read isn't implemented yet.");
		}
		break;

		default:
		{
			W("No ECAT format is set yet, so I cant't identify the correct SubHeader to use.");
		}
	}

	return CECATMainHeader::Unknown;
}

CECATSubHeader::Type CECATFile::subHeaderType(void)
{
	return subHeaderType(fileType());
}

CECATSubHeader::Type CECATFile::subHeaderType(CECATMainHeader::Type fileType)
{
	// depending on the fileType (which is specified by the MainHeader)
	// we return the subheader that is used
	switch(fileType)
	{
		// do a 1:1 mapping for all types
		case CECATMainHeader::ECAT7_Sinogram:					return CECATSubHeader::ECAT7_Scan;
		case CECATMainHeader::ECAT7_Sinogram3D_8:
		case CECATMainHeader::ECAT7_Sinogram3D_16:
		case CECATMainHeader::ECAT7_Sinogram3D_Float:	return CECATSubHeader::ECAT7_Scan3D;
		case CECATMainHeader::ECAT7_Volume8:
		case CECATMainHeader::ECAT7_Volume16:
		case CECATMainHeader::ECAT7_Image16:					return CECATSubHeader::ECAT7_Image;
		case CECATMainHeader::ECAT7_AttenCorr:				return CECATSubHeader::ECAT7_AttenCorr;
		case CECATMainHeader::ECAT7_Normalization:		return CECATSubHeader::ECAT7_Norm;
		case CECATMainHeader::ECAT7_Normalization_3D:	return CECATSubHeader::ECAT7_Norm3D;
		case CECATMainHeader::ECAT7_PolarMap:					return CECATSubHeader::ECAT7_PolarMap;

		default:
		{
			W("ECAT fileType couldn't be identified.");
		}
	}

	return CECATSubHeader::Unknown;
}

void CECATFile::setFileType(CECATMainHeader::Type fileType)
{
	// check if we already have a valide MainHeader and Directory
	// and if so, we free them
	if(m_pMainDirectory)	delete m_pMainDirectory;
	if(m_pMainHeader)			delete m_pMainHeader;

	// we need to find out if this should be a ECAT6 or ECAT7
	// file.
	switch(fileType)
	{
		case CECATMainHeader::ECAT7_Sinogram:
		case CECATMainHeader::ECAT7_Sinogram3D_8:
		case CECATMainHeader::ECAT7_Sinogram3D_16:
		case CECATMainHeader::ECAT7_Sinogram3D_Float:
		case CECATMainHeader::ECAT7_Volume8:
		case CECATMainHeader::ECAT7_Volume16:
		case CECATMainHeader::ECAT7_Image16:					
		case CECATMainHeader::ECAT7_Image8:					
		case CECATMainHeader::ECAT7_AttenCorr:				
		case CECATMainHeader::ECAT7_Normalization:		
		case CECATMainHeader::ECAT7_Normalization_3D:
		case CECATMainHeader::ECAT7_PolarMap:
		{
			m_ECATformat			= ECAT7;
			m_pMainHeader			= new CECAT7MainHeader(fileType);
			m_pMainDirectory	= new CECATDirectory(subHeaderType(fileType));
		}
		break;

		default:
		{
			m_ECATformat			= Undefined;
			m_pMainHeader			= NULL;
			m_pMainDirectory	= NULL;
		}
		break;
	}
}

CECATSubHeader* CECATFile::getSubHeader(short frame, short plane,
																				short gate, short bed, short data)
{
	// we have to perform a search within the whole DirectoryList
	// to get the SubHeader but we just need to query the
	// Main directory here as it will forward the query to it's sub
	// directories
	ASSERT(m_pMainDirectory);
	return m_pMainDirectory->getSubHeader(frame, plane, gate, bed, data);
}


QByteArray* CECATFile::getMatrix(short frame, short plane,
																 short gate, short bed, short data)
{
	// we have to perform a search within the whole DirectoryList
	// to get the MatrixData but we just need to query the
	// Main directory here as it will forward the query to it's sub
	// directories
	ASSERT(m_pMainDirectory);
	return m_pMainDirectory->getMatrix(frame, plane, gate, bed, data);
}

void* CECATFile::getMatrixData(short frame, short plane,
															 short gate, short bed, short data)
{
	// we have to perform a search within the whole DirectoryList
	// to get the MatrixData but we just need to query the
	// Main directory here as it will forward the query to it's sub
	// directories
	ASSERT(m_pMainDirectory);
	return m_pMainDirectory->getMatrixData(frame, plane, gate, bed, data);
}

CECATSubHeader* CECATFile::newEntry(short frame, short plane,
																		short gate, short bed, short data)
{
	ASSERT(m_pMainDirectory);
	ASSERT(m_pMainHeader);
	
	// signal the MainDirectory list that we want to create a new Entry within
	// the directorylist and that it should return the corresponding
	// SubHeader after sucessfully creating the entry data within the list
	CECATSubHeader* pNewSubHeader =
		m_pMainDirectory->newEntry(frame, plane, gate, bed, data);

	// make sure the subheader refers to the maximum available frame/planes etc.
	if(pNewSubHeader)
	{
		if(numFrames() < frame)	m_pMainHeader->setNum_Frames(frame);
		if(numPlanes() < plane)	m_pMainHeader->setNum_Planes(plane);
		if(numGates()  < gate)	m_pMainHeader->setNum_Gates(gate);
		if(numBedPos() < bed)		m_pMainHeader->setNum_Bed_Pos(bed);
	}

	return pNewSubHeader;
}

bool CECATFile::setMatrix(QByteArray* matrix, short frame, short plane,
													short gate, short bed, short data)
{
	// we forward the request to set a specific matrix to the supplied
	// QByteArray, the Directory will find the corresponding entry
	// and replace the matrix associates with it.
	ASSERT(m_pMainDirectory);
	return m_pMainDirectory->setMatrix(matrix, frame, plane, gate, bed, data);
}

bool CECATFile::setMatrixData(void* matrix, unsigned size, short frame, short plane,
															short gate, short bed, short data)
{
	// we forward the request to set a specific matrix to the supplied
	// void pointer, the Directory will find the corresponding entry
	// and replace the matrix associates with it.
	ASSERT(m_pMainDirectory);
	return m_pMainDirectory->setMatrixData(matrix, size, frame, plane, gate, bed, data);
}
