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

#include "CECAT7SubHeaderNorm.h"
#include "CECATFile.h"

#include <qdatastream.h>

#include "debug.h"

CECAT7SubHeaderNorm::CECAT7SubHeaderNorm()
{
	// check that the headsize is 512 bytes long
	ASSERT(sizeof(struct ECAT7SubHeader_Norm) == 512);

	// then clear the structure
	memset(&m_Data, 0, sizeof(struct ECAT7SubHeader_Norm));
}

bool CECAT7SubHeaderNorm::load(QDataStream& stream)
{
	// check if the stream is readable or not.
	if(stream.device()->isReadable() == false)
		return false;

	// lets read in each single data element of our
	// data structure to maintain the correct endianess of the
	// data
	stream >> m_Data.Data_Type;											//   0: Data_Type
	stream >> m_Data.Num_Dimensions;								//   2: Num_Dimensions
	stream >> m_Data.Num_R_Elements;								//   4: Num_R_Elements
	stream >> m_Data.Num_Angles;										//   6: Num_Angles
	stream >> m_Data.Num_Z_Elements;								//   8: Num_Z_Elements
	stream >> m_Data.Ring_Difference;								//  10: Ring_Difference
	stream >> m_Data.Scale_Factor;									//  12: Scale_Factor
	stream >> m_Data.Norm_Min;											//  16: Norm_Min
	stream >> m_Data.Norm_Max;											//  20: Norm_Max
	stream >> m_Data.FOV_Source_Width;							//  24: FOV_Source_Width
	stream >> m_Data.Norm_Quality_Factor;						//  28: Norm_Quality_Factor
	stream >> m_Data.Norm_Quality_Factor_Code;			//  32: Norm_Quality_Factor_Code
	stream >> m_Data.Storage_Order;									//	34: Storage_Order
	stream >> m_Data.Span;													//  36: Span
	for(int i=0; i < 64; i++)
		stream >> m_Data.Z_Elements[i];								//  38: Z_Elements
	for(int i=0; i < 123; i++)
		stream >> m_Data.CTI_reserved[i];							// 123: CTI_reserved
	for(int i=0; i < 50; i++)
		stream >> m_Data.User_Reserved[i];						// 412: User_Reserved

	// some more debug output
#if defined(DEBUG)
	D("ECAT7 Normalization SubHeader loaded:");
	D("------------------------------------");
	D("Data_Type	               : %d",				m_Data.Data_Type);
	D("Num_Dimensions					   : %d",				m_Data.Num_Dimensions);
	D("Num_R_Elements		         : %d",				m_Data.Num_R_Elements);
	D("Num_Angles                : %d",				m_Data.Num_Angles);
	D("Num_Z_Elements            : %d",				m_Data.Num_Z_Elements);
	D("Ring_Difference           : %d",				m_Data.Ring_Difference);
	D("Scale_Factor              : %f",				m_Data.Scale_Factor);
	D("Norm_Min                  : %f",				m_Data.Norm_Min);
	D("Norm_Max                  : %f",				m_Data.Norm_Max);
	D("FOV_Source_Width          : %f cm",		m_Data.FOV_Source_Width);
	D("Norm_Quality_Factor       : %f",				m_Data.Norm_Quality_Factor);
	D("Norm_Quality_Factor_Code  : %d",				m_Data.Norm_Quality_Factor_Code);
	D("Storage_Order             : %d",				m_Data.Storage_Order);
	D("Span                      : %d",				m_Data.Span);
	for(int i=0; i < 64; i++)
	{
		D("Z_Elements							[%d]: %f", i+1, m_Data.Z_Elements[i]);
	}
#endif

	return true;
}

bool CECAT7SubHeaderNorm::save(QDataStream& stream)
{
	// check if this stream is writeable or not
	if(stream.device()->isWritable() == false)
		return false;

	// lets write out each single data element of our
	// data structure to maintain the correct endianess of the
	// data
	stream << m_Data.Data_Type;											//   0: Data_Type
	stream << m_Data.Num_Dimensions;								//   2: Num_Dimensions
	stream << m_Data.Num_R_Elements;								//   4: Num_R_Elements
	stream << m_Data.Num_Angles;										//   6: Num_Angles
	stream << m_Data.Num_Z_Elements;								//   8: Num_Z_Elements
	stream << m_Data.Ring_Difference;								//  10: Ring_Difference
	stream << m_Data.Scale_Factor;									//  12: Scale_Factor
	stream << m_Data.Norm_Min;											//  16: Norm_Min
	stream << m_Data.Norm_Max;											//  20: Norm_Max
	stream << m_Data.FOV_Source_Width;							//  24: FOV_Source_Width
	stream << m_Data.Norm_Quality_Factor;						//  28: Norm_Quality_Factor
	stream << m_Data.Norm_Quality_Factor_Code;			//  32: Norm_Quality_Factor_Code
	stream << m_Data.Storage_Order;									//	34: Storage_Order
	stream << m_Data.Span;													//  36: Span
	for(int i=0; i < 64; i++)
		stream << m_Data.Z_Elements[i];								//  38: Z_Elements
	for(int i=0; i < 123; i++)
		stream << m_Data.CTI_reserved[i];							// 123: CTI_reserved
	for(int i=0; i < 50; i++)
		stream << m_Data.User_Reserved[i];						// 412: User_Reserved
	
	return true;
}
