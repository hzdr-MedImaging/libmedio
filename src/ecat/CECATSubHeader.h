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

#ifndef CECATSUBHEADER_H
#define CECATSUBHEADER_H

#include <CMedIOHeader.h>

#include <QDataStream>

// forward declarations
class CECATDirectoryItem;

class CECATSubHeader : public CMedIOHeader
{
	public:
		enum Type	{ Unknown=0, 
								ECAT7_AttenCorr, 
								ECAT7_Image, 
								ECAT7_Norm,
								ECAT7_Norm3D, 
								ECAT7_PolarMap, 
								ECAT7_Scan,
								ECAT7_Scan3D
							};

		enum Data_Type	{	UnknownDataType	=	0,
											ByteData				=	1, // 1: char
											VAX_Ix2					=	2, // 2: short - little endian
											VAX_Ix4					=	3, // 4: long  - little endian
											VAX_Rx4					=	4, // 4: float - little endian
										  IEEEFloat				=	5, // 4: float - big endian
											SunShort				=	6, // 2: short - big endian
											SunLong					=	7  // 4: long  - big endian
										};	

		// constructor
		CECATSubHeader(CMedIOData* ecatFile,
									 CECATDirectoryItem* dItem = NULL);

		// our virtual load/save I/O routines
		virtual bool load(void) = 0;
		virtual bool save(void) const = 0;

		// size information in bytes of specific raw subheader structure
		virtual int rawDataSize() const = 0;

		// some must have accessor methods
		virtual Data_Type data_Type(void) const = 0;
		virtual void setData_Type(const Data_Type dType) = 0;

		// runtime type information methods
		CMedIOHeader::Format headerFormat() const;
		virtual CECATSubHeader::Type subHeaderType(void) const = 0;

		// internal method to set the directory Item of a subHeader
		void setDirectoryItem(CECATDirectoryItem* dItem);

	protected:
		// required method to copy relevant data from another MedIOHeader object
		virtual CMedIOHeader& copyData(const CMedIOHeader& src) = 0;

	protected:
		CECATDirectoryItem* m_pDirItem; // the directory item to which this
																		// subHeader belongs
};

#endif // CECATSUBHEADER_H
