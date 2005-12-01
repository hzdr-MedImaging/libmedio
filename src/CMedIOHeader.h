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

//! @file CMedIOHeader.h
//! @author Hagen Moelle
//! @brief contains definition of the class CMedIOHeader

#ifndef CMEDIOHEADER_H
#define CMEDIOHEADER_H

#include <QString>

// forward declarations
class CMedIOData;

//! @class CMedIOHeader
//! @brief an abstract class which provides an interface for header information of medical data
//! @bug no bugs reported
//! @warning still in development
//!
//! This class is an interface for easier handling of different medical data headers.
//! All headers of data formats which are included in Medical IO Library should use the class
//! as a baseclass.
class CMedIOHeader
{
	public:
		enum Format 
		{	
			Unknown = 0,
			ConcordeMicroPetMainHeader,	//!< main header of concorde microPET scanner
			ConcordeMicroPetFrameHeader,	//!< frame header of concorde microPET scanner
			ECATMainHeader,			//!< main header of ECAT6/7 PET scanners
			ECATSubHeader			//!< sub header of ECAT6/7 PET scanners
		};

		CMedIOHeader(CMedIOData* data=NULL);
		virtual ~CMedIOHeader();
		
		//! @brief set the header to a defined state
		virtual void clear() = 0;

		//! @brief loads a given file 
		//! @return true if loading is succesful otherwise false
		virtual bool load() = 0;
		
		//! @brief saves the data  
		//! @return true if saving is succesful otherwise false
		virtual bool save() const = 0;

		//! @brief run time typeinformation  
		//! @return specific header format at runtime
		virtual CMedIOHeader::Format headerFormat() const = 0;

		//! @brief try to convert the given medio header in this header type
		//! @return true for success; false for failure
		virtual bool convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader* pHead2 = NULL) = 0;
		
		//! @brief create a new medio header of the same type as pSrc and
		//!				 copy all meta information
		//! @return the new created CMedIOHeader object
		virtual CMedIOHeader* clone() const = 0;

		//! @brief default assignment operator which enforces that m_pMedIOData
		//!        is NOT copied as well.
		CMedIOHeader& operator=(const CMedIOHeader& src);

#ifdef __MEDIO_PRIVATE__
	protected:
		CMedIOData* m_pMedIOData; //!< the associated MedIOData this header belongs to
#endif
};

#endif
