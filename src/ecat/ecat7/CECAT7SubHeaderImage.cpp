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

#include "CECAT7SubHeaderImage.h"
#include "CECATDirectoryItem.h"
#include "CECATFile.h"

#include <qdatastream.h>

#include "debug.h"

CECAT7SubHeaderImage::CECAT7SubHeaderImage(CECATFile* ecatFile,
																					 CECATDirectoryItem* pDirItem)
	: CECATSubHeader(ecatFile, pDirItem)
{
	// check that the headsize is 1024 bytes long
	ASSERT(sizeof(struct ECAT7SubHeader_Image) == ECAT7_HEADERSIZE_IMAGE);

	// then clear the structure
	memset(&m_Data, 0, sizeof(struct ECAT7SubHeader_Image));
}

CECAT7SubHeaderImage::CECAT7SubHeaderImage(const CECAT7SubHeaderImage& sh)
	: CECATSubHeader(sh)
{
	// check that the headsize is 1024 bytes long
	ASSERT(sizeof(struct ECAT7SubHeader_Image) == ECAT7_HEADERSIZE_IMAGE);

	// then clear the structure
	memcpy(&m_Data, &sh.m_Data, sizeof(struct ECAT7SubHeader_Image));
}

bool CECAT7SubHeaderImage::load(void)
{
	ENTER();

	// check if the stream is readable or not and
	// set our MedIOData to the correct file position so that we can
	// read the subheader	
	if(m_pMedIOData->isReadable() == false ||
		 m_pDirItem->dataBlock_Start() == 0 ||
		 m_pMedIOData->at(m_pDirItem->dataBlock_Start()) == false)
	{
		RETURN(false);
		return false;
	}

	// we use a ByteArray buffer to speed up the endianess
	// decoding
	QByteArray buffer(sizeof(struct ECAT7SubHeader_Image));
	if(m_pMedIOData->readBlock(buffer.data(), sizeof(struct ECAT7SubHeader_Image)) 
			!= sizeof(struct ECAT7SubHeader_Image))
	{
		RETURN(false);
		return false;
	}

	// now we generate a QDataStream on our buffer so that we can read
	// out of the buffer instead of the raw file (> speed)
	QDataStream stream(buffer, IO_ReadOnly);	

	// lets read in each single data element of our
	// data structure to maintain the correct endianess of the
	// data
	stream >> m_Data.Data_Type;											//   0: Data_Type
	stream >> m_Data.Num_Dimensions;								//   2: Num_Dimensions
	stream >> m_Data.X_Dimension;										//   4: X_Dimension
	stream >> m_Data.Y_Dimension;										//   6: Y_Dimension
	stream >> m_Data.Z_Dimension;										//   8: Z_Dimension
	stream >> m_Data.X_Offset;											//  10: X_Offset
	stream >> m_Data.Y_Offset;											//  14: Y_Offset
	stream >> m_Data.Z_Offset;											//  18: Z_Offset
	stream >> m_Data.Recon_Zoom;										//  22: Recon_Zoom
	stream >> m_Data.Scale_Factor;									//  26: Scale_Factor
	stream >> m_Data.Image_Min;											//  30: Image_Min
	stream >> m_Data.Image_Max;											//  32: Image_Max
	stream >> m_Data.X_Pixel_Size;									//  34: X_Pixel_Size
	stream >> m_Data.Y_Pixel_Size;									//  38: Y_Pixel_Size
	stream >> m_Data.Z_Pixel_Size;									//  42: Z_Pixel_Size
	stream >> m_Data.Frame_Duration;								//  46: Frame_Duration
	stream >> m_Data.Frame_Start_Time;							//  50: Frame_Start_Time
	stream >> m_Data.Filter_Code;										//  54: Filter_Code
	stream >> m_Data.X_Resolution;									//  56: X_Resolution
	stream >> m_Data.Y_Resolution;									//  60: Y_Resolution
	stream >> m_Data.Z_Resolution;									//  64: Z_Resolution
	stream >> m_Data.Num_R_Elements;								//  70: Num_R_Elements
	stream >> m_Data.Num_Angles;										//  72: Num_Angles
	stream >> m_Data.Z_Rotation_Angle;							//  76: Z_Rotation_Angle
	stream >> m_Data.Decay_Corr_Fctr;								//  80: Decay_Corr_Fctr
	stream >> m_Data.Processing_Code;								//  84: Processing_Code
	stream >> m_Data.Gate_Duration;									//  88: Gate_Duration
	stream >> m_Data.R_Wave_Offset;									//  92: R_Wave_Offset
	stream >> m_Data.Num_Accepted_Beats;						//  96: Num_Accepted_Beats
	stream >> m_Data.Filter_Cutoff_Frequency;				// 100: Filter_Cutoff_Frequency
	stream >> m_Data.Filter_Resolution;							// 104: Filter_Resolution
	stream >> m_Data.Filter_Ramp_Slope;							// 108: Filter_Ramp_Slope
	stream >> m_Data.Filter_Order;									// 112: Filter_Order
	stream >> m_Data.Filter_Scatter_Fraction;				// 116: Filter_Scatter_Fraction
	stream >> m_Data.Filter_Scatter_Slope;					// 120: Filter_Scatter_Slope
	stream.readRawBytes(&m_Data.Annotation[0], 40);	// 122: Annotation
	stream >> m_Data.MT_1_1;												// 162: MT_1_1
	stream >> m_Data.MT_1_2;												// 166: MT_1_2
	stream >> m_Data.MT_1_3;												// 170: MT_1_3
	stream >> m_Data.MT_2_1;												// 174: MT_2_1
	stream >> m_Data.MT_2_2;												// 178: MT_2_2
	stream >> m_Data.MT_2_3;												// 182: MT_2_3
	stream >> m_Data.MT_3_1;												// 186: MT_3_1
	stream >> m_Data.MT_3_2;												// 190: MT_3_2
	stream >> m_Data.MT_3_3;												// 194: MT_3_3
	stream >> m_Data.RFilter_Cutoff;								// 198: RFilter_Cutoff
	stream >> m_Data.RFilter_Resolution;						// 202: RFilter_Resolution
	stream >> m_Data.RFilter_Code;									// 206: RFilter_Code
	stream >> m_Data.RFilter_Order;									// 208: RFilter_Order
	stream >> m_Data.ZFilter_Cutoff;								// 210: ZFilter_Cutoff
	stream >> m_Data.ZFilter_Resolution;						// 214: ZFilter_Resolution
	stream >> m_Data.ZFilter_Code;									// 218: ZFilter_Code
	stream >> m_Data.ZFilter_Order;									// 220: ZFilter_Order
	stream >> m_Data.MT_1_4;												// 222: MT_1_4
	stream >> m_Data.MT_2_4;												// 226: MT_2_4
	stream >> m_Data.MT_3_4;												// 230: MT_3_4
	stream >> m_Data.Scatter_Type;									// 230: Scatter_Type
	stream >> m_Data.Recon_Type;										// 236: Recon_Type
	stream >> m_Data.Recon_Views;										// 238: Recon_Views
	for(int i=0; i < 87; i++)
		stream >> m_Data.CTI_Reserved[i];							// 240: CTI_Reserved
	for(int i=0; i < 49; i++)
		stream >> m_Data.User_Reserved[i];						// 414: User_Reserved

	// some more debug output
#if defined(DEBUG)
	D("ECAT7 Image SubHeader loaded:");
	D("----------------------------");
	D("Data_Type                 : %d",				m_Data.Data_Type);
	D("Num_Dimensions            : %d",				m_Data.Num_Dimensions);
	D("X_Dimension               : %d",				m_Data.X_Dimension);
	D("Y_Dimension               : %d",				m_Data.Y_Dimension);
	D("Z_Dimension               : %d",				m_Data.Z_Dimension);
	D("X_Offset                  : %f cm",		m_Data.X_Offset);
	D("Y_Offset                  : %f cm",		m_Data.Y_Offset);
	D("Z_Offset                  : %f cm",		m_Data.Z_Offset);
	D("Recon_Zoom                : %fx",			m_Data.Recon_Zoom);
	D("Scale_Factor              : %f",				m_Data.Scale_Factor);
	D("Image_Min                 : %d",				m_Data.Image_Min);
	D("Image_Max                 : %d",				m_Data.Image_Max);
	D("X_Pixel_Size              : %f cm",		m_Data.X_Pixel_Size);
	D("Y_Pixel_Size              : %f cm",		m_Data.Y_Pixel_Size);
	D("Z_Pixel_Size              : %f cm",		m_Data.Z_Pixel_Size);
	D("Frame_Duration            : %d msec",	m_Data.Frame_Duration);
	D("Frame_Start_Time          : %d msec",	m_Data.Frame_Start_Time);
	D("Filter_Code               : %d",				m_Data.Filter_Code);
	D("X_Resolution              : %f cm",		m_Data.X_Resolution);
	D("Y_Resolution              : %f cm",		m_Data.Y_Resolution);
	D("Z_Resolution              : %f cm",		m_Data.Z_Resolution);
	D("Num_R_Elements            : %f",				m_Data.Num_R_Elements);
	D("Num_Angles                : %f",				m_Data.Num_Angles);
	D("Z_Rotation_Angle          : %f°",			m_Data.Z_Rotation_Angle);
	D("Decay_Corr_Fctr           : %f",				m_Data.Decay_Corr_Fctr);
	D("Processing_Code           : %d",				m_Data.Processing_Code);
	D("Gate_Duration             : %d msec",	m_Data.Gate_Duration);
	D("R_Wave_Offset             : %d msec",	m_Data.R_Wave_Offset);
	D("Num_Accepted_Beats        : %d",				m_Data.Num_Accepted_Beats);
	D("Filter_Cutoff_Frequency   : %f",				m_Data.Filter_Cutoff_Frequency);
	D("Filter_Resolution         : %f",				m_Data.Filter_Resolution);
	D("Filter_Ramp_Slope         : %f",				m_Data.Filter_Ramp_Slope);
	D("Filter_Order              : %d",				m_Data.Filter_Order);
	D("Filter_Scatter_Fraction   : %f",				m_Data.Filter_Scatter_Fraction);
	D("Filter_Scatter_Slope      : %f",				m_Data.Filter_Scatter_Slope);
	D("Annotation                : %s",				m_Data.Annotation);
	D("MT_1_1                    : %f",				m_Data.MT_1_1);
	D("MT_1_2                    : %f",				m_Data.MT_1_2);
	D("MT_1_3                    : %f",				m_Data.MT_1_3);
	D("MT_2_1                    : %f",				m_Data.MT_2_1);
	D("MT_2_2                    : %f",				m_Data.MT_2_2);
	D("MT_2_3                    : %f",				m_Data.MT_2_3);
	D("MT_3_1                    : %f",				m_Data.MT_3_1);
	D("MT_3_2                    : %f",				m_Data.MT_3_2);
	D("MT_3_3                    : %f",				m_Data.MT_3_3);
	D("RFilter_Cutoff            : %f",				m_Data.RFilter_Cutoff);
	D("RFilter_Resolution        : %f",				m_Data.RFilter_Resolution);
	D("RFilter_Code              : %d",				m_Data.RFilter_Code);
	D("RFilter_Order             : %d",				m_Data.RFilter_Order);
	D("ZFilter_Cutoff            : %f",				m_Data.ZFilter_Cutoff);
	D("ZFilter_Resolution        : %f",				m_Data.ZFilter_Resolution);
	D("ZFilter_Code              : %d",				m_Data.ZFilter_Code);
	D("ZFilter_Order             : %d",				m_Data.ZFilter_Order);
	D("MT_1_4                    : %f",				m_Data.MT_1_4);
	D("MT_2_4                    : %f",				m_Data.MT_2_4);
	D("MT_3_4                    : %f",				m_Data.MT_3_4);
	D("Scatter_Type              : %d",				m_Data.Scatter_Type);
	D("Recon_Type                : %d",				m_Data.Recon_Type);
	D("Recon_Views               : %d",				m_Data.Recon_Views);
#endif

	RETURN(true);
	return true;
}

bool CECAT7SubHeaderImage::save(void) const
{
	ENTER();

	// check if this stream is writeable or not
	if(m_pMedIOData->isWritable() == false ||
		 m_pDirItem->dataBlock_Start() == 0 ||
		 m_pMedIOData->at(m_pDirItem->dataBlock_Start()) == false)
	{
		RETURN(false);
		return false;
	}

	// we write to a buffer first and write out later directly to the file
	QByteArray buffer(sizeof(struct ECAT7SubHeader_Image));
	QDataStream stream(buffer, IO_WriteOnly);
 
	// lets write out each single data element of our
	// data structure to maintain the correct endianess of the
	// data
	stream << m_Data.Data_Type;											//   0: Data_Type
	stream << m_Data.Num_Dimensions;								//   2: Num_Dimensions
	stream << m_Data.X_Dimension;										//   4: X_Dimension
	stream << m_Data.Y_Dimension;										//   6: Y_Dimension
	stream << m_Data.Z_Dimension;										//   8: Z_Dimension
	stream << m_Data.X_Offset;											//  10: X_Offset
	stream << m_Data.Y_Offset;											//  14: Y_Offset
	stream << m_Data.Z_Offset;											//  18: Z_Offset
	stream << m_Data.Recon_Zoom;										//  22: Recon_Zoom
	stream << m_Data.Scale_Factor;									//  26: Scale_Factor
	stream << m_Data.Image_Min;											//  30: Image_Min
	stream << m_Data.Image_Max;											//  32: Image_Max
	stream << m_Data.X_Pixel_Size;									//  34: X_Pixel_Size
	stream << m_Data.Y_Pixel_Size;									//  38: Y_Pixel_Size
	stream << m_Data.Z_Pixel_Size;									//  42: Z_Pixel_Size
	stream << m_Data.Frame_Duration;								//  46: Frame_Duration
	stream << m_Data.Frame_Start_Time;							//  50: Frame_Start_Time
	stream << m_Data.Filter_Code;										//  54: Filter_Code
	stream << m_Data.X_Resolution;									//  56: X_Resolution
	stream << m_Data.Y_Resolution;									//  60: Y_Resolution
	stream << m_Data.Z_Resolution;									//  64: Z_Resolution
	stream << m_Data.Num_R_Elements;								//  70: Num_R_Elements
	stream << m_Data.Num_Angles;										//  72: Num_Angles
	stream << m_Data.Z_Rotation_Angle;							//  76: Z_Rotation_Angle
	stream << m_Data.Decay_Corr_Fctr;								//  80: Decay_Corr_Fctr
	stream << m_Data.Processing_Code;								//  84: Processing_Code
	stream << m_Data.Gate_Duration;									//  88: Gate_Duration
	stream << m_Data.R_Wave_Offset;									//  92: R_Wave_Offset
	stream << m_Data.Num_Accepted_Beats;						//  96: Num_Accepted_Beats
	stream << m_Data.Filter_Cutoff_Frequency;				// 100: Filter_Cutoff_Frequency
	stream << m_Data.Filter_Resolution;							// 104: Filter_Resolution
	stream << m_Data.Filter_Ramp_Slope;							// 108: Filter_Ramp_Slope
	stream << m_Data.Filter_Order;									// 112: Filter_Order
	stream << m_Data.Filter_Scatter_Fraction;				// 116: Filter_Scatter_Fraction
	stream << m_Data.Filter_Scatter_Slope;					// 120: Filter_Scatter_Slope
	stream.writeRawBytes(&m_Data.Annotation[0], 40);
	stream << m_Data.MT_1_1;												// 162: MT_1_1
	stream << m_Data.MT_1_2;												// 166: MT_1_2
	stream << m_Data.MT_1_3;												// 170: MT_1_3
	stream << m_Data.MT_2_1;												// 174: MT_2_1
	stream << m_Data.MT_2_2;												// 178: MT_2_2
	stream << m_Data.MT_2_3;												// 182: MT_2_3
	stream << m_Data.MT_3_1;												// 186: MT_3_1
	stream << m_Data.MT_3_2;												// 190: MT_3_2
	stream << m_Data.MT_3_3;												// 194: MT_3_3
	stream << m_Data.RFilter_Cutoff;								// 198: RFilter_Cutoff
	stream << m_Data.RFilter_Resolution;						// 202: RFilter_Resolution
	stream << m_Data.RFilter_Code;									// 206: RFilter_Code
	stream << m_Data.RFilter_Order;									// 208: RFilter_Order
	stream << m_Data.ZFilter_Cutoff;								// 210: ZFilter_Cutoff
	stream << m_Data.ZFilter_Resolution;						// 214: ZFilter_Resolution
	stream << m_Data.ZFilter_Code;									// 218: ZFilter_Code
	stream << m_Data.ZFilter_Order;									// 220: ZFilter_Order
	stream << m_Data.MT_1_4;												// 222: MT_1_4
	stream << m_Data.MT_2_4;												// 226: MT_2_4
	stream << m_Data.MT_3_4;												// 230: MT_3_4
	stream << m_Data.Scatter_Type;									// 230: Scatter_Type
	stream << m_Data.Recon_Type;										// 236: Recon_Type
	stream << m_Data.Recon_Views;										// 238: Recon_Views
	for(int i=0; i < 87; i++)
		stream << m_Data.CTI_Reserved[i];							// 240: CTI_Reserved
	for(int i=0; i < 49; i++)
		stream << m_Data.User_Reserved[i];						// 414: User_Reserved
	
	// now write out to our outStream
	bool result = false;
	if(m_pMedIOData->writeBlock(buffer) != -1)
		result = true;

	RETURN(result);
	return result;

}
