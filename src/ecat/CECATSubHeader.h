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

#ifndef CECATSUBHEADER_H
#define CECATSUBHEADER_H

#include <qdatastream.h>

#include "debug.h"

class CECATSubHeader
{
	public:
		enum Type	{ Unknown=0, ECAT7_AttenCorr, ECAT7_Image, ECAT7_Norm,
								ECAT7_Norm3D, ECAT7_PolarMap, ECAT7_Scan,
								ECAT7_Scan3D
							};

		enum Data_Type	{	UnkownMatrixDataType	=	0,
											ByteData							=	1, // 1: char
											VAX_Ix2								=	2, // 2: short - little endian
											VAX_Ix4								=	3, // 4: long  - little endian
											VAX_Rx4								=	4, // 4: float - little endian
										  IEEEFloat							=	5, // 4: float - big endian
											SunShort							=	6, // 2: short - big endian
											SunLong								=	7  // 4: long  - big endian
										};							

		// our virtual load/save I/O routines
		virtual bool load(QDataStream& stream) = 0;
		virtual bool save(QDataStream& stream) = 0;

		// some must have accessor methods
		virtual Data_Type data_Type(void) = 0;
		virtual void setData_Type(Data_Type dType) = 0;

		// runtime type information
		virtual Type rtti(void) = 0;
};

#endif // CECATSUBHEADER_H
