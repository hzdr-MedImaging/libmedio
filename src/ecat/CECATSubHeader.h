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

#define ECAT7_ATTENCORR_HEADER_SIZE 512
#define ECAT7_IMAGE_HEADER_SIZE 512
#define ECAT7_NORM_HEADER_SIZE 512
#define ECAT7_NORM3D_HEADER_SIZE 512
#define ECAT7_POLARMAP_HEADER_SIZE 512
#define ECAT7_SCAN_HEADER_SIZE 512
#define ECAT7_SCAN3D_HEADER_SIZE 1024

#include <qdatastream.h>

class Q_EXPORT CECATSubHeader
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
		virtual Data_Type data_Type(void) const = 0;
		virtual void setData_Type(const Data_Type dType) = 0;

		// runtime type information
		virtual Type rtti(void) const = 0;

		//size information in bytes of specific subheader
		virtual int size() const = 0;
};

#endif // CECATSUBHEADER_H
