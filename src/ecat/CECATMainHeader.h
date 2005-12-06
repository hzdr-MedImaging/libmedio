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

#ifndef CECATMAINHEADER_H
#define CECATMAINHEADER_H

#include <qtextstream.h>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOHeader>
#else
#include <CMedIOHeader.h>
#endif

// forward declarations
class CECATFile;

class Q_EXPORT CECATMainHeader : public CMedIOHeader
{
	public:
		// possible ECAT6/7 file types
		enum HeaderType { UnknownHeaderType=0,
											ECAT6MainHeader,
											ECAT7MainHeader
										};
		
		enum Type { Unknown=0, ECAT7_Sinogram, ECAT7_Image16, ECAT7_AttenCorr,
								ECAT7_Normalization, ECAT7_PolarMap, ECAT7_Volume8,
								ECAT7_Volume16, ECAT7_Projection8, ECAT7_Projection16,
								ECAT7_Image8, ECAT7_Sinogram3D_16, ECAT7_Sinogram3D_8,
								ECAT7_Normalization_3D, ECAT7_Sinogram3D_Float
							};

		// constructor
		CECATMainHeader(CMedIOData* ecatFile);

		// copy constructur and default assignment operator
		CECATMainHeader(const CECATMainHeader& src);		
		CECATMainHeader& operator=(const CECATMainHeader& src);

		// header clear method
		virtual void clear() = 0;	

		// some pure virtual methods to load/save information
		// prepresented by this class
		virtual bool load()	= 0;
		virtual bool save() const = 0;

		// size information in bytes of specific raw mainheader structure
		virtual int rawDataSize() const = 0;	

		// interface methods that are part of the ECAT6 and
		// ECAT7 standard and each ECAT6/7 implementation has
		// to implement it.
		virtual	short num_Planes(void) const	= 0;
		virtual short num_Frames(void) const	= 0;
		virtual short num_Gates(void) const		= 0;
		virtual short num_Bed_Pos(void) const	= 0;
		
		virtual void setNum_Planes(short num)		= 0;
		virtual void setNum_Frames(short num)		= 0;
		virtual void setNum_Gates(short num)		= 0;
		virtual void setNum_Bed_Pos(short num)	= 0;

		// runtime type information methods
		virtual int rtti() const = 0;
		CMedIOHeader::Format headerFormat() const;

		// try to convert the given medio header in this header type
		virtual bool convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader* pHead2 = NULL) = 0;
		
		// create a new medio header of the same type as pSrc and
		// copy all meta information
		virtual CMedIOHeader* clone() const = 0;
};

#endif // CECATMAINHEADER_H
