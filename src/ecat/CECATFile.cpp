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
#include "CMedIOData.h"
#include "CECATMainHeader.h"
#include "CECAT6MainHeader.h"
#include "CECAT7MainHeader.h"
#include "CECATDirectory.h"

#include <qdatastream.h>
#include <qfileinfo.h>

#include "debug.h"

CECATFile::CECATFile(const QString& filename, CECATMainHeader::Type fileType)
	: CMedIOData(filename),
		m_iECATformat(Undefined),
		m_iMainHeaderType(fileType),
		m_pMainDirectory(NULL),
		m_pCachedMainHeader(NULL)
{
	if(fileType != CECATMainHeader::Unknown)
		setFileType(fileType);
}

CECATFile::~CECATFile()
{
	if(m_pMainDirectory)
		delete m_pMainDirectory;

	if(m_pCachedMainHeader)
		delete m_pCachedMainHeader;
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
	if(m_pMainDirectory)
	{
		delete m_pMainDirectory;
		m_pMainDirectory = NULL;
	}

	if(m_pCachedMainHeader)
	{
		delete m_pCachedMainHeader;
		m_pCachedMainHeader = NULL;
	}

	// depending on the specified open mode we either read some META data from
	// an existing ECAT file or we create it here
	if(exists() &&
		 mode & (IO_ReadOnly|IO_WriteOnly))
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
				m_iECATformat = ECAT7;
				m_pCachedMainHeader = new CECAT7MainHeader(this);

				// now that we have created our proper MainHeader we try
				// to load the header information and then load the
				// ECAT DirectoryList out of the ECATFile.
				if(m_pCachedMainHeader->load())
				{
					m_pMainDirectory = new CECATDirectory(this);
					if(m_pMainDirectory->load())
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
				m_iECATformat	= ECAT6;
				m_pCachedMainHeader = new CECAT6MainHeader(this);
				
				// now that we have created our proper MainHeader we try
				// to load the header information and then load the
				// ECAT DirectoryList out of the ECATFile.
				if(m_pCachedMainHeader->load())
				{
					CECAT6MainHeader* mainHeader = static_cast<CECAT6MainHeader*>(m_pCachedMainHeader);

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
						m_pMainDirectory = new CECATDirectory(this);
						if(m_pMainDirectory->load())
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
		// the file doesn't exist and therefore we do not have any
		// main header or directory list. so lets create some empty ones
		if(m_iECATformat != CECATFile::Undefined)
		{
			m_pMainDirectory = new CECATDirectory(this, subHeaderType(m_iMainHeaderType));
			result = true;
		}
		else
			E("ECATformat unknown");
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
		if(m_pMainDirectory)
		{
			delete m_pMainDirectory;
			m_pMainDirectory = NULL;
		}

		if(m_pCachedMainHeader)
		{
			delete m_pCachedMainHeader;
			m_pCachedMainHeader = NULL;
		}		
	}

	RETURN(result);
	return result;
}

void CECATFile::close(void)
{
	// close the opened file and clean everything up
	QFile::close();

	if(m_pMainDirectory)
	{
		delete m_pMainDirectory;
		m_pMainDirectory = NULL;
	}

	if(m_pCachedMainHeader)
	{
		delete m_pCachedMainHeader;
		m_pCachedMainHeader = NULL;
	}			
}

CECATMainHeader::Type CECATFile::fileType(void)
{
	// if the Main header isn't loaded yet we return Unknown
	if(m_iECATformat == CECATFile::Undefined)
		return CECATMainHeader::Unknown;

	// before we can tell the correct fileType of this file
	// we have to get the mainHeader
	bool cachedMainHeaderUsed = true;
	CECATMainHeader* mainHeader = m_pCachedMainHeader;
	if(mainHeader == NULL)
	{
		cachedMainHeaderUsed = false;

		if(readMainHeader(mainHeader) == false)
			return CECATMainHeader::Unknown;
	}

	// depending on the format and DataSetType the MainHeader
	// contains, we can specify the ECAT file type and return it here
	switch(m_iECATformat)
	{
		case CECATFile::ECAT7:
		{
			switch(static_cast<CECAT7MainHeader*>(mainHeader)->file_Type())
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
			W("No ECAT format is set yet, so I cant't identify the correct SubHeader to use.");
	}

	// delete our temporary mainHeader again
	if(cachedMainHeaderUsed == false)
		delete mainHeader;

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
				m_iECATformat	= ECAT7;
			}
			break;

			default:
			{
				m_iECATformat	= Undefined;
			}
			break;
		}

		result = true;
	}

	RETURN(result);
	return result;
}

bool CECATFile::readMainHeader(CECATMainHeader*& mainHeader)
{
	ENTER();
	bool result = false;
	
	if(isReadable())
	{
		switch(m_iECATformat)
		{
			case CECATFile::ECAT7:
			{
				mainHeader = new CECAT7MainHeader(this, m_iMainHeaderType);
				if(mainHeader->load())
					result = true;
				else
					delete mainHeader;
			}
			break;

			case CECATFile::ECAT6:
			{
				mainHeader = new CECAT6MainHeader(this, m_iMainHeaderType);
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

	if(result == false)
		mainHeader = NULL;

	RETURN(result);
	return result;
}

bool CECATFile::readSubHeader(CECATSubHeader*& subHeader, short frame,
															short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	// before we are going to read the SubHeader from the ECAT file we
	// have to check wheter the file is correctly open in Read mode.
	if(isReadable())
	{
		ASSERT(m_pMainDirectory);
		result = m_pMainDirectory->readSubHeader(subHeader, frame, plane, gate, bed, data);
	}
	
	RETURN(result);
	return result;
}


bool CECATFile::readMatrix(QByteArray*& matrixData, short frame, short plane,
													 short gate, short bed, short data)
{
	// we have to perform a search within the whole DirectoryList
	// to get the MatrixData but we just need to query the
	// Main directory here as it will forward the query to it's sub
	// directories
	if(isReadable())
	{
		ASSERT(m_pMainDirectory);
		return m_pMainDirectory->readMatrix(matrixData, frame, plane, gate, bed, data);
	}

	return false;
}

bool CECATFile::readMatrix(char*& matrixData, unsigned int& len, short frame,
													 short plane, short gate, short bed, short data)
{
	// we have to perform a search within the whole DirectoryList
	// to get the MatrixData but we just need to query the
	// Main directory here as it will forward the query to it's sub
	// directories
	if(isReadable())
	{
		ASSERT(m_pMainDirectory);
		return m_pMainDirectory->readMatrix(matrixData, len, frame, plane, gate, bed, data);
	}

	return false;
}

bool CECATFile::writeMainHeader(CECATMainHeader& mainHeader)
{
	ENTER();
	bool result;

	// on default we check the consistency of our loaded directory with
	// the main header the user wants to write. So we change the frame/plane/gate
	// stuff in the main header before writing it actually.
	mainHeader.setNum_Planes(numPlanes());
	mainHeader.setNum_Frames(numFrames());
	mainHeader.setNum_Gates(numGates());
	mainHeader.setNum_Bed_Pos(numBedPos());

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

	ASSERT(m_pMainDirectory);
	
	// forward the write request to the MainDirectory which is going to manage
	// everything else for us
	if(m_pMainDirectory->writeSubHeader(subHeader, frame, plane, gate, bed, data))
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
		}
	}

	RETURN(result);
	return result;
}

bool CECATFile::writeMatrix(const QByteArray& matrixData, short frame,
														short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	ASSERT(m_pMainDirectory);
	
	// forward the write request to the MainDirectory which is going to manage
	// everything else for us
	if(m_pMainDirectory->writeMatrix(matrixData, frame, plane, gate, bed, data))
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
		}
	}

	RETURN(result);
	return result;
}

bool CECATFile::writeMatrix(const char* matrixData, unsigned int size, short frame,
														short plane, short gate, short bed, short data)
{
	ENTER();
	bool result = false;

	ASSERT(m_pMainDirectory);
	
	// forward the write request to the MainDirectory which is going to manage
	// everything else for us
	if(m_pMainDirectory->writeMatrix(matrixData, size, frame, plane, gate, bed, data))
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
		}
	}

	RETURN(result);
	return result;
}
