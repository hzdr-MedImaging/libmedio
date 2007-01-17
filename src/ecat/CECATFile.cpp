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

#include "CECATFile.h"
#include "CMedIOData.h"
#include "CECATMainHeader.h"
#include "CECAT6MainHeader.h"
#include "CECAT7MainHeader.h"
#include "CECATDirectory.h"
#include "CECAT7SubHeaderAttenCorr.h"
#include "CECAT7SubHeaderImage.h"
#include "CECAT7SubHeaderNorm.h"
#include "CECAT7SubHeaderNorm3D.h"
#include "CECAT7SubHeaderPolarMap.h"
#include "CECAT7SubHeaderScan.h"
#include "CECAT7SubHeaderScan3D.h"

#include <qdatastream.h>
#include <qfileinfo.h>

#include <rtdebug.h>

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CECATFilePrivate
{
	public:
		CECATFile::ECATFormat	iECATformat;
		CECATDirectory*				directory;
		CECATMainHeader::Type	iMainHeaderType;
		CECATMainHeader*			cachedMainHeader; // for speed reasons we cache the loaded main header
};		

CECATFile::CECATFile(const QString& filename, CECATMainHeader::Type fileType)
	: CMedIOData(filename)
{
	ENTER();

	// allocate data from our private instance class
	m_pData = new CECATFilePrivate();
	m_pData->iECATformat = CECATFile::Undefined;
	m_pData->directory = NULL;
	m_pData->iMainHeaderType = fileType;
	m_pData->cachedMainHeader = NULL;

	if(fileType != CECATMainHeader::Unknown)
		setFileType(fileType);

	LEAVE();
}

CECATFile::~CECATFile()
{
	ENTER();

	delete m_pData->directory;
	delete m_pData->cachedMainHeader;
	delete m_pData;

	LEAVE();
}

int CECATFile::rtti() const
{ 
	return CMedIOData::ECAT;
}

CECATFile::ECATFormat CECATFile::format(void) const
{ 
	return m_pData->iECATformat;
}

short CECATFile::numFrames(void) const
{ 
	return m_pData->directory->numFrames();
}

short CECATFile::numPlanes(void) const
{ 
	return m_pData->directory->numPlanes();
}

short CECATFile::numGates(void) const
{ 
	return m_pData->directory->numGates();
}

short CECATFile::numBedPos(void) const
{ 
	return m_pData->directory->numBedPos();
}

CECATDirectory* CECATFile::directory(void) const
{ 
	return m_pData->directory;
}

bool CECATFile::isOfType(const QString& filename)
{
	ENTER();
	bool result = false;

	// try to open the file and identify it as a ECAT6 or 7 file
	CECATFile file(filename);
	if(file.open(IO_ReadOnly))
	{
		if(file.format() != CECATFile::Undefined)
			result = true;

		file.close();
	}

	RETURN(result);
	return result;
}

CMedIOData* CECATFile::createFromFile(const QString& fileName)
{
	ENTER();

	CMedIOData* mData = NULL;

	if(isOfType(fileName))
		mData = new CECATFile(fileName);

	RETURN(mData);
	return mData;
}

bool CECATFile::open(int mode)
{
	ENTER();

	bool result = false;
	
	// first we check if we already have some valid MainHeader/MainDirectory
	// combination and if so we need to delete it first.
	delete m_pData->directory;
	m_pData->directory = NULL;
	delete m_pData->cachedMainHeader;
	m_pData->cachedMainHeader = NULL;

	// depending on the specified open mode we either read some META data from
	// an existing ECAT file or we create it here
	if(exists() &&
		 mode & (IO_ReadOnly|IO_WriteOnly) == (IO_ReadOnly|IO_WriteOnly))
	{
		// we open the file and read in the
		// main header and directory list of the ecat file
		if(QFile::open(IO_ReadOnly))
		{
			QDataStream stream(this);

			// but before we start reading the main header we check
			// the magic number of the file and if it exists it is
			// a ECAT7 file and if not we probably handle a ECAT6
			// filetype or an invalid format :/
			char magic_number[14];
			stream.readRawBytes(magic_number, 14*sizeof(char));

			// compare the loaded magic_number and if we found a "MATRIX"
			// string at the file start this is a ECAT7 fileformat.
			if(strncmp(magic_number, "MATRIX", 6) == 0)
			{
				D("found magic number: %s", magic_number);

				// it is/should be a ECAT7 file, so we can instantiate a
				// ECAT7 MainHeader and let it read the MainHeader information
				// accordingly
				m_pData->iECATformat = ECAT7;
				m_pData->cachedMainHeader = new CECAT7MainHeader(this);

				// now that we have created our proper MainHeader we try
				// to load the header information and then load the
				// ECAT DirectoryList out of the ECATFile.
				if(m_pData->cachedMainHeader->load())
				{
					m_pData->directory = new CECATDirectory(this);
					if(m_pData->directory->load())
					{
						// only if the directory loading also suceeded we
						// finally loaded the ECAT file
						result = true;
					}
				}
			}
			else
			{
				D("NO magic number found, assuming ECAT6 file");
				
				// according to the CTI ECAT implementation this could either
				// be an ECAT6 file or an unknown file format, but as we don't
				// know exactly we simply assume it is a ECAT6 format and
				// use a ECAT6 MainHeader for error checking later on
				m_pData->iECATformat	= ECAT6;
				m_pData->cachedMainHeader = new CECAT6MainHeader(this);
				
				// now that we have created our proper MainHeader we try
				// to load the header information and then load the
				// ECAT DirectoryList out of the ECATFile.
				if(m_pData->cachedMainHeader->load())
				{
					CECAT6MainHeader* mainHeader = static_cast<CECAT6MainHeader*>(m_pData->cachedMainHeader);

					// in case of an ECAT6 file we try to do the error checking here
					// now. As ECAT6 doesn't have a 'magic number' this check is somewhat
					// heuristic. However, checks are only done on the main header.
					// The procedure is:
					// 1. check 'SW_Version' field to be between 0 and 69
					// 2. check 'file_type' field to be one of 'enum File_Type'
					// 3. check 'num_frames' to be > 0
					if(mainHeader->sw_Version() > 0 && mainHeader->sw_Version() <= 69 &&
						 mainHeader->file_Type() != CECAT6MainHeader::Unknown &&
						 mainHeader->num_Frames() > 0)
					{
						m_pData->directory = new CECATDirectory(this);
						if(m_pData->directory->load())
						{
							// only if the directory loading also suceeded we
							// finally loaded the ECAT file
							result = true;
						}
					}
					else
						W("heuristic ECAT6 check didn't find correct values -> NO ECAT file");
				}
				else
					W("main header loading failed");
			}
		
			// close the file
			QFile::close();
		}
	}
	else if(mode & IO_WriteOnly)
	{
		D("preparing IO_WriteOnly mode: %d", m_pData->iECATformat);

		// the file doesn't exist and therefore we do not have any
		// main header or directory list. so lets create some empty ones
		if(m_pData->iECATformat != CECATFile::Undefined)
		{
			m_pData->directory = new CECATDirectory(this);

			switch(m_pData->iECATformat)
			{
				case CECATFile::ECAT7:
					m_pData->cachedMainHeader = new CECAT7MainHeader(this, m_pData->iMainHeaderType);
				break;

				case CECATFile::ECAT6:
					m_pData->cachedMainHeader = new CECAT6MainHeader(this, m_pData->iMainHeaderType);
				break;

				default:
					W("No ECAT format is set yet, so no cached Mainheader can be prepared.");
			}

			result = true;
		}
		else
			E("ECATformat unknown");

		// make sure the file is removed upon opening it
		if(result)
			QFile::remove(name());
	}

	// to make the open operation a bit safer we mask out the IO operation
	// bits we don't need
	mode &= ~(IO_Raw|IO_Append|IO_Truncate|IO_Translate);

	// only if we succeeded with our mainheader/dirlisting loading
	// we can assume everything worked out fine and reopen the file
	// with the user settings.
	if(result)
	{
		// make sure the file is always openend in read mode also
		// because we need to read in data as we write as well 
		if((result = QFile::open(mode|IO_ReadOnly)) == false)
			QFile::close();
	}
		
	if(result == false)
	{
		// otherwise we go and delete our main directory again
		if(m_pData->directory)
		{
			delete m_pData->directory;
			m_pData->directory = NULL;
		}

		if(m_pData->cachedMainHeader)
		{
			delete m_pData->cachedMainHeader;
			m_pData->cachedMainHeader = NULL;
		}		
	}

	SHOWPOINTER(m_pData->directory);
	SHOWPOINTER(m_pData->cachedMainHeader);

	RETURN(result);
	return result;
}

void CECATFile::close(void)
{
	// close the opened file and clean everything up
	QFile::close();

	if(m_pData->directory)
	{
		delete m_pData->directory;
		m_pData->directory = NULL;
	}

	if(m_pData->cachedMainHeader)
	{
		delete m_pData->cachedMainHeader;
		m_pData->cachedMainHeader = NULL;
	}			
}

CECATMainHeader::Type CECATFile::fileType(void)
{
	ENTER();

	// if the Main header isn't loaded yet we return Unknown
	if(m_pData->iECATformat == CECATFile::Undefined)
	{
		RETURN(CECATMainHeader::Unknown);
		return CECATMainHeader::Unknown;
	}

	// before we can tell the correct fileType of this file
	// we have to get the mainHeader
	bool cachedMainHeaderUsed = true;
	CECATMainHeader* mainHeader = m_pData->cachedMainHeader;
	SHOWPOINTER(mainHeader);
	if(mainHeader == NULL)
	{
		cachedMainHeaderUsed = false;

		if(readMainHeader(mainHeader) == false)
		{
			delete mainHeader;

			RETURN(CECATMainHeader::Unknown);
			return CECATMainHeader::Unknown;
		}
	}

	// depending on the format and DataSetType the MainHeader
	// contains, we can specify the ECAT file type and return it here
	CECATMainHeader::Type type = CECATMainHeader::Unknown;
	switch(m_pData->iECATformat)
	{
		case CECATFile::ECAT7:
		{
			switch(static_cast<CECAT7MainHeader*>(mainHeader)->file_Type())
			{
				// do a 1:1 mapping for all types
				case CECAT7MainHeader::Sinogram:
					type = CECATMainHeader::ECAT7_Sinogram;
				break;
				
				case CECAT7MainHeader::Image16:						
					type = CECATMainHeader::ECAT7_Image16;
				break;
				
				case CECAT7MainHeader::AttenuationCorr:		
					type = CECATMainHeader::ECAT7_AttenCorr;
				break;
				
				case CECAT7MainHeader::Normalization:	
					type = CECATMainHeader::ECAT7_Normalization;
				break;
				
				case CECAT7MainHeader::PolarMap:
					type = CECATMainHeader::ECAT7_PolarMap;
				break;
				
				case CECAT7MainHeader::Volume8:
					type = CECATMainHeader::ECAT7_Volume8;
				break;

				case CECAT7MainHeader::Volume16:
					type = CECATMainHeader::ECAT7_Volume16;
				break;
				
				case CECAT7MainHeader::Projection8:
					type = CECATMainHeader::ECAT7_Projection8;
				break;
				
				case CECAT7MainHeader::Projection16:
					type = CECATMainHeader::ECAT7_Projection16;
				break;
				
				case CECAT7MainHeader::Image8:
					type = CECATMainHeader::ECAT7_Image8;			
				break;
				
				case CECAT7MainHeader::Sinogram3D_16:
					type = CECATMainHeader::ECAT7_Sinogram3D_16;
				break;
				
				case CECAT7MainHeader::Sinogram3D_8:
					type = CECATMainHeader::ECAT7_Sinogram3D_8;
				break;
				
				case CECAT7MainHeader::Normalization_3D:
					type = CECATMainHeader::ECAT7_Normalization_3D;
				break;
				
				case CECAT7MainHeader::Sinogram3D_Float:
					type = CECATMainHeader::ECAT7_Sinogram3D_Float;
				break;

				default:
				{
					W("ECAT type couldn't be identified: %d", static_cast<CECAT7MainHeader*>(mainHeader)->file_Type());
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
			W("No ECAT format is set yet, so I cant't identify the correct SubHeader to use.");
	}

	// delete our temporary mainHeader again
	if(cachedMainHeaderUsed == false)
		delete mainHeader;

	RETURN(type);
	return type;
}

CECATSubHeader::Type CECATFile::subHeaderType(void)
{
	ENTER();

	CECATSubHeader::Type type =  subHeaderType(fileType());
	
	RETURN(type);
	return type;
}

CECATSubHeader::Type CECATFile::subHeaderType(CECATMainHeader::Type fileType)
{
	ENTER();

	CECATSubHeader::Type type =  CECATSubHeader::Unknown;
	
	// depending on the fileType (which is specified by the MainHeader)
	// we return the subheader that is used
	switch(fileType)
	{
		// do a 1:1 mapping for all types
		case CECATMainHeader::ECAT7_Sinogram:
			type = CECATSubHeader::ECAT7_Scan;
		break;
		
		case CECATMainHeader::ECAT7_Sinogram3D_8:
		case CECATMainHeader::ECAT7_Sinogram3D_16:
		case CECATMainHeader::ECAT7_Sinogram3D_Float:
			type = CECATSubHeader::ECAT7_Scan3D;
		break;
		
		case CECATMainHeader::ECAT7_Volume8:
		case CECATMainHeader::ECAT7_Volume16:
		case CECATMainHeader::ECAT7_Image16:
			type = CECATSubHeader::ECAT7_Image;
		break;
		
		case CECATMainHeader::ECAT7_AttenCorr:				
			type = CECATSubHeader::ECAT7_AttenCorr;
		break;
		
		case CECATMainHeader::ECAT7_Normalization:		
			type = CECATSubHeader::ECAT7_Norm;
		break;
		
		case CECATMainHeader::ECAT7_Normalization_3D:
			type = CECATSubHeader::ECAT7_Norm3D;
		break;
		
		case CECATMainHeader::ECAT7_PolarMap:					
			type = CECATSubHeader::ECAT7_PolarMap;
		break;

		default:
		{
			W("ECAT fileType couldn't be identified.");
		}
	}

	RETURN(type);
	return type;
}

bool CECATFile::setFileType(CECATMainHeader::Type fileType)
{
	ENTER();

	bool result = false;
	
	// we first make sure the file isn't open right now so that
	// we don't interfere our data processing
	if(isOpen() == false)
	{
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
				m_pData->iECATformat	= ECAT7;
			}
			break;

			default:
			{
				m_pData->iECATformat	= Undefined;
			}
			break;
		}

		result = true;
	}
  else
	  E("Filetype change requested while ECAT file is already open");

	RETURN(result);
	return result;
}

bool CECATFile::readMainHeader(CECATMainHeader*& mainHeader)
{
	ENTER();
	bool result = false;
	
  // check if the file was open as readable.
	if(isReadable())
	{
		// check if we have a cached main header somewhere around so
		// we can take that one instead of loading the main header from
		// scratch again
		if(m_pData->cachedMainHeader)
		{
			switch(m_pData->iECATformat)
			{
				case CECATFile::ECAT7:
				{
					mainHeader = new CECAT7MainHeader(*static_cast<const CECAT7MainHeader*>(m_pData->cachedMainHeader));
					result = true;
				}
				break;

				case CECATFile::ECAT6:
				{
					mainHeader = new CECAT6MainHeader(*static_cast<const CECAT6MainHeader*>(m_pData->cachedMainHeader));
					result = true;
				}
				break;

				default:
					// nothing
				break;
			}

			// we set the MedIOData object of the newly created main
			// header now
			mainHeader->setMedIOData(this);
		}
		else
		{
			switch(m_pData->iECATformat)
			{
				case CECATFile::ECAT7:
				{
					mainHeader = new CECAT7MainHeader(this, m_pData->iMainHeaderType);
					if(mainHeader->load())
						result = true;
					else
						delete mainHeader;
				}
				break;

				case CECATFile::ECAT6:
				{
					mainHeader = new CECAT6MainHeader(this, m_pData->iMainHeaderType);
					if(mainHeader->load())
						result = true;
					else
						delete mainHeader;
				}
				break;

				default:
					// nothing
				break;
			}
		}
	}

	if(result == false)
		mainHeader = NULL;
	else
	{
		// if the loading was successfull we have to make sure to use that
		// new instance of the main header as the new cached one.
		if(m_pData->cachedMainHeader == NULL)
		{
			switch(m_pData->iECATformat)
			{
				case CECATFile::ECAT7:
					m_pData->cachedMainHeader = new CECAT7MainHeader(*static_cast<CECAT7MainHeader*>(mainHeader));
				break;

				case CECATFile::ECAT6:
					m_pData->cachedMainHeader = new CECAT6MainHeader(*static_cast<CECAT6MainHeader*>(mainHeader));
				break;

				default:
					// nothing
				break;
			}

			// set the CECATFile object as the MedIOData object
			// of the cached header
			m_pData->cachedMainHeader->setMedIOData(this);
		}
	}

	RETURN(result);
	return result;
}

bool CECATFile::readSubHeader(CECATSubHeader*& subHeader, short frame,
															short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	ASSERT(m_pData->directory);

	// before we are going to read the SubHeader from the ECAT file we
	// have to check wheter the file is correctly open in Read mode.
	if(isReadable() && m_pData->directory)
		result = m_pData->directory->readSubHeader(subHeader, frame, plane, gate, bed, data);
	
	RETURN(result);
	return result;
}


bool CECATFile::readMatrix(QByteArray*& matrixData, short frame, short plane,
													 short gate, short bed, short data)
{
	ENTER();
	bool result = false;
	ASSERT(m_pData->directory);
	
	// we have to perform a search within the whole DirectoryList
	// to get the MatrixData but we just need to query the
	// Main directory here as it will forward the query to it's sub
	// directories
	if(isReadable() && m_pData->directory)
		result = m_pData->directory->readMatrix(matrixData, frame, plane, gate, bed, data);

	RETURN(result);
	return result;
}

bool CECATFile::readMatrix(char*& matrixData, unsigned int& len, short frame,
													 short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	ASSERT(m_pData->directory);

	// we have to perform a search within the whole DirectoryList
	// to get the MatrixData but we just need to query the
	// Main directory here as it will forward the query to it's sub
	// directories
	if(isReadable() && m_pData->directory)
		result = m_pData->directory->readMatrix(matrixData, len, frame, plane, gate, bed, data);

	RETURN(result);
	return result;
}

bool CECATFile::readMatrix(QByteArray*& matrixData, CECATSubHeader*& subHeader,
													 short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;
	ASSERT(m_pData->directory);
	
	// we have to perform a search within the whole DirectoryList
	// to get the MatrixData but we just need to query the
	// Main directory here as it will forward the query to it's sub
	// directories
	if(isReadable() && m_pData->directory)
		result = m_pData->directory->readMatrix(matrixData, subHeader, frame, plane, gate, bed, data);

	RETURN(result);
	return result;
}

bool CECATFile::readMatrix(char*& matrixData, unsigned int& len, CECATSubHeader*& subHeader,
													 short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	ASSERT(m_pData->directory);

	// we have to perform a search within the whole DirectoryList
	// to get the MatrixData but we just need to query the
	// Main directory here as it will forward the query to it's sub
	// directories
	if(isReadable() && m_pData->directory)
		result = m_pData->directory->readMatrix(matrixData, len, subHeader, frame, plane, gate, bed, data);

	RETURN(result);
	return result;
}

bool CECATFile::writeMainHeader(CECATMainHeader& mainHeader)
{
	ENTER();
	bool result;

	// we forward the write request to the main Header's own save
	// routines.
	result = mainHeader.save();
	
	RETURN(result);
	return result;
}
		
bool CECATFile::writeSubHeader(const CECATSubHeader& subHeader, short frame,
															 short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	ASSERT(m_pData->directory);
	
	// forward the write request to the MainDirectory which is going to manage
	// everything else for us
	if(m_pData->directory &&
		 m_pData->directory->writeSubHeader(subHeader, frame, plane, gate, bed, data))
	{
		// make sure the frames/planes/gates/bedpos parameters in the mainheader
		// are in sync
		CECATMainHeader* mainHeader = NULL;
		if(readMainHeader(mainHeader))
		{
			if(mainHeader->num_Frames() < frame ||
				 mainHeader->num_Planes() < plane ||
				 mainHeader->num_Gates()  < gate	||
				 mainHeader->num_Bed_Pos()< bed)
			{
				result = writeMainHeader(*mainHeader);
			}
			else
				result = true;
			
			delete mainHeader;
		}
	}

	RETURN(result);
	return result;
}

bool CECATFile::writeMatrix(const QByteArray& matrixData, 
														short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	ASSERT(m_pData->directory);
	
	// forward the write request to the MainDirectory which is going to manage
	// everything else for us
	if(m_pData->directory &&
		 m_pData->directory->writeMatrix(matrixData, frame, plane, gate, bed, data))
	{
		// make sure the frames/planes/gates/bedpos parameters in the mainheader
		// are in sync
		CECATMainHeader* mainHeader = NULL;
		if(readMainHeader(mainHeader))
		{
			if(mainHeader->num_Frames() < frame ||
				 mainHeader->num_Planes() < plane ||
				 mainHeader->num_Gates()  < gate	||
				 mainHeader->num_Bed_Pos()< bed)
			{
				result = writeMainHeader(*mainHeader);
			}
			else
				result = true;

			delete mainHeader;
		}
	}

	RETURN(result);
	return result;
}

bool CECATFile::writeMatrix(const char* matrixData, unsigned int size,
														short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	ASSERT(m_pData->directory);
	
	// forward the write request to the MainDirectory which is going to manage
	// everything else for us
	if(m_pData->directory && 
		 m_pData->directory->writeMatrix(matrixData, size, frame, plane, gate, bed, data))
	{
		// make sure the frames/planes/gates/bedpos parameters in the mainheader
		// are in sync
		CECATMainHeader* mainHeader = NULL;
		if(readMainHeader(mainHeader))
		{
			if(mainHeader->num_Frames() < frame ||
				 mainHeader->num_Planes() < plane ||
				 mainHeader->num_Gates()  < gate	||
				 mainHeader->num_Bed_Pos()< bed)
			{
				result = writeMainHeader(*mainHeader);
			}
			else
				result = true;

			delete mainHeader;
		}
	}

	RETURN(result);
	return result;
}

bool CECATFile::writeMatrix(const QByteArray& matrixData, const CECATSubHeader& subHeader,
														short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	ASSERT(m_pData->directory);
	
	// forward the write request to the MainDirectory which is going to manage
	// everything else for us
	if(m_pData->directory &&
		 m_pData->directory->writeMatrix(matrixData, subHeader, frame, plane, gate, bed, data))
	{
		// make sure the frames/planes/gates/bedpos parameters in the mainheader
		// are in sync
		CECATMainHeader* mainHeader = NULL;
		if(readMainHeader(mainHeader))
		{
			if(mainHeader->num_Frames() < frame ||
				 mainHeader->num_Planes() < plane ||
				 mainHeader->num_Gates()  < gate	||
				 mainHeader->num_Bed_Pos()< bed)
			{
				result = writeMainHeader(*mainHeader);
			}
			else
				result = true;

			delete mainHeader;
		}
	}

	RETURN(result);
	return result;
}

bool CECATFile::writeMatrix(const char* matrixData, unsigned int size, const CECATSubHeader& subHeader,
														short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	ASSERT(m_pData->directory);
	
	// forward the write request to the MainDirectory which is going to manage
	// everything else for us
	if(m_pData->directory && 
		 m_pData->directory->writeMatrix(matrixData, size, subHeader, frame, plane, gate, bed, data))
	{
		// make sure the frames/planes/gates/bedpos parameters in the mainheader
		// are in sync
		CECATMainHeader* mainHeader = NULL;
		if(readMainHeader(mainHeader))
		{
			if(mainHeader->num_Frames() < frame ||
				 mainHeader->num_Planes() < plane ||
				 mainHeader->num_Gates()  < gate	||
				 mainHeader->num_Bed_Pos()< bed)
			{
				result = writeMainHeader(*mainHeader);
			}
			else
				result = true;

			delete mainHeader;
		}
	}

	RETURN(result);
	return result;
}

bool CECATFile::writeMatrix(const QByteArray& matrixData, CECATSubHeader::Data_Type type, 
														short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	ASSERT(m_pData->directory);
	
	// forward the write request to the MainDirectory which is going to manage
	// everything else for us
	if(m_pData->directory &&
		 m_pData->directory->writeMatrix(matrixData, type, frame, plane, gate, bed, data))
	{
		// make sure the frames/planes/gates/bedpos parameters in the mainheader
		// are in sync
		CECATMainHeader* mainHeader = NULL;
		if(readMainHeader(mainHeader))
		{
			if(mainHeader->num_Frames() < frame ||
				 mainHeader->num_Planes() < plane ||
				 mainHeader->num_Gates()  < gate	||
				 mainHeader->num_Bed_Pos()< bed)
			{
				result = writeMainHeader(*mainHeader);
			}
			else
				result = true;

			delete mainHeader;
		}
	}

	RETURN(result);
	return result;
}

bool CECATFile::writeMatrix(const char* matrixData, unsigned int size, CECATSubHeader::Data_Type type,
														short frame, short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	ASSERT(m_pData->directory);
	
	// forward the write request to the MainDirectory which is going to manage
	// everything else for us
	if(m_pData->directory && 
		 m_pData->directory->writeMatrix(matrixData, size, type, frame, plane, gate, bed, data))
	{
		// make sure the frames/planes/gates/bedpos parameters in the mainheader
		// are in sync
		CECATMainHeader* mainHeader = NULL;
		if(readMainHeader(mainHeader))
		{
			if(mainHeader->num_Frames() < frame ||
				 mainHeader->num_Planes() < plane ||
				 mainHeader->num_Gates()  < gate	||
				 mainHeader->num_Bed_Pos()< bed)
			{
				result = writeMainHeader(*mainHeader);
			}
			else
				result = true;

			delete mainHeader;
		}
	}

	RETURN(result);
	return result;
}

CECATMainHeader* CECATFile::createEmptyMainHeader(void)
{
	ENTER();
	CECATMainHeader* pEmptyMainHeader = NULL;
  if(isOpen())
	{
			switch(m_pData->iECATformat)
			{
				case CECATFile::ECAT7:
					pEmptyMainHeader = new CECAT7MainHeader(this, m_pData->iMainHeaderType);
				break;

				case CECATFile::ECAT6:
					pEmptyMainHeader = new CECAT6MainHeader(this, m_pData->iMainHeaderType);
				break;

				default:
					// nothing
				break;
			}
	}

	RETURN(pEmptyMainHeader);
	return pEmptyMainHeader;
}

CECATSubHeader* CECATFile::createEmptySubHeader(void)
{
	ENTER();
	CECATSubHeader* subHeader = NULL;

	switch(subHeaderType())
	{
		case CECATSubHeader::ECAT7_AttenCorr: 
			subHeader = new CECAT7SubHeaderAttenCorr(this);
		break;

		case CECATSubHeader::ECAT7_Image:	
			subHeader = new CECAT7SubHeaderImage(this);
		break;

		case CECATSubHeader::ECAT7_Norm:
			subHeader = new CECAT7SubHeaderNorm(this);
		break;
		
		case CECATSubHeader::ECAT7_Norm3D:
			subHeader = new CECAT7SubHeaderNorm3D(this);
		break;
		
		case CECATSubHeader::ECAT7_PolarMap:
			subHeader = new CECAT7SubHeaderPolarMap(this);
		break;
		
		case CECATSubHeader::ECAT7_Scan:
			subHeader = new CECAT7SubHeaderScan(this);
		break;
		
		case CECATSubHeader::ECAT7_Scan3D:
			subHeader = new CECAT7SubHeaderScan3D(this);
		break;

		default:
			E("ECAT type isn't specified or not supported yet.");
	}

	RETURN(subHeader);
	return subHeader;
}

void CECATFile::mainHeaderWritten(const CECATMainHeader& mainHeader)
{
	ENTER();

	// now that a new main Header has been written we have to
	// update our cached copy accordingly.
	switch(m_pData->iECATformat)
	{
		case CECATFile::ECAT7:
		{
			if(m_pData->cachedMainHeader)
				*static_cast<CECAT7MainHeader*>(m_pData->cachedMainHeader) = *static_cast<const CECAT7MainHeader*>(&mainHeader);
			else
				m_pData->cachedMainHeader = new CECAT7MainHeader(*static_cast<const CECAT7MainHeader*>(&mainHeader));
		}
		break;

		case CECATFile::ECAT6:
		{
			if(m_pData->cachedMainHeader)
				*static_cast<CECAT6MainHeader*>(m_pData->cachedMainHeader) = *static_cast<const CECAT6MainHeader*>(&mainHeader);
			else
				m_pData->cachedMainHeader = new CECAT6MainHeader(*static_cast<const CECAT6MainHeader*>(&mainHeader));
		}
		break;

		default:
			// nothing
		break;
	}

	// in addition to that we have to place the correct frames/planes/gates/bedPos
	// stuff in the cached header to be totally correct.
	if(m_pData->cachedMainHeader)
	{
		// Please note that we do not specify any PLANES here as in ECAT7 the
		// planes are normally integrated in one matrix file. So we have to
		// allow the user to specify the planes himself.
		m_pData->cachedMainHeader->setNum_Frames(numFrames());
		m_pData->cachedMainHeader->setNum_Gates(numGates());
		m_pData->cachedMainHeader->setNum_Bed_Pos(numBedPos());
		
		// set the CECATFile object as the MedIOData object
		// of the cached header
		m_pData->cachedMainHeader->setMedIOData(this);
	}

	LEAVE();
}

bool CECATFile::reWriteMainHeader(void)
{
	ENTER();
	bool result = false;

	if(m_pData->cachedMainHeader)
		result = m_pData->cachedMainHeader->save();
	else
	{
		CECATMainHeader* mHeader;

		if(readMainHeader(mHeader))
		{
			result = mHeader->save();

			delete mHeader;
		}
	}

	RETURN(result);
	return result;
}
