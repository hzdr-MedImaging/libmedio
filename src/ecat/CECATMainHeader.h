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

#ifndef CECATMAINHEADER_H
#define CECATMAINHEADER_H

#include <qtextstream.h>

// forward declarations
class CECATFile;

class CECATMainHeader
{
	public:
		// possible ECAT6/7 file types
		enum Type { Unknown=0, ECAT7_Sinogram, ECAT7_Image16, ECAT7_AttenCorr,
								ECAT7_Normalization, ECAT7_PolarMap, ECAT7_Volume8,
								ECAT7_Volume16, ECAT7_Projection8, ECAT7_Projection16,
								ECAT7_Image8, ECAT7_Sinogram3D_16, ECAT7_Sinogram3D_8,
								ECAT7_Normalization_3D, ECAT7_Sinogram3D_Float
							};

		// some pure virtual methods to load/save information
		// prepresented by this class
		virtual bool load(CECATFile* pFile)			= 0;
		virtual bool load(QTextStream& stream)	= 0;
		virtual bool save(CECATFile* pFile)			= 0;
		virtual bool save(QTextStream& stream)	= 0;

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
};

#endif // CECATMAINHEADER_H
