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
#include "CConcordeMainHeader.h"
#include "CConcordeFrameHeader.h"

#include <QDataStream>

#include <rtdebug.h>

CECAT7SubHeaderImage::CECAT7SubHeaderImage(CECATFile* ecatFile,
																					 CECATDirectoryItem* pDirItem)
	: CECATSubHeader(ecatFile, pDirItem)
{
	clear();
}

CECAT7SubHeaderImage::CECAT7SubHeaderImage()
	: CECATSubHeader(NULL)
{
	clear();
}

void CECAT7SubHeaderImage::clear()
{
	ENTER();

	// clear our header structure first
	memset(&m_Data, 0, sizeof(struct ECAT7SubHeader_Image));
	
	setData_Type(CECATSubHeader::UnknownDataType);
	setFilter_Code(NoFilter);
	setScatter_Type(None);
	setRecon_Type(FilteredBackProjection);

	LEAVE();
}

bool CECAT7SubHeaderImage::load(void)
{
	ENTER();

	// check if the stream is readable or not and
	// set our MedIOData to the correct file position so that we can
	// read the subheader	
	if(m_pMedIOData->isReadable() == false ||
		 m_pDirItem->dataBlock_Start() == 0 ||
		 m_pMedIOData->seek(m_pDirItem->dataBlock_Start()) == false)
	{
		RETURN(false);
		return false;
	}

	// we use a ByteArray buffer to speed up the endianess
	// decoding
	QByteArray buffer(rawDataSize(), 0);
	if(m_pMedIOData->read(buffer.data(), buffer.size()) != rawDataSize())
	{
		RETURN(false);
		return false;
	}

	// now we generate a QDataStream on our buffer so that we can read
	// out of the buffer instead of the raw file (> speed)
	QDataStream stream(buffer);	

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
	stream.readRawData(&m_Data.Annotation[0], 40);	// 122: Annotation
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
	if(m_pMedIOData == NULL || m_pMedIOData->isWritable() == false ||
		 m_pDirItem == NULL || m_pDirItem->dataBlock_Start() == 0 ||
		 m_pMedIOData->seek(m_pDirItem->dataBlock_Start()) == false)
	{
		RETURN(false);
		return false;
	}

	SHOWVALUE(m_pMedIOData->pos());

	// we write to a buffer first and write out later directly to the file
	QByteArray buffer(rawDataSize(), 0);
	QDataStream stream(&buffer, QIODevice::WriteOnly);
 
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
	stream.writeRawData(&m_Data.Annotation[0], 40);
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
	if(m_pMedIOData->write(buffer) != -1)
	{
		m_pDirItem->subHeaderWritten(*this);
		result = true;
	}

	RETURN(result);
	return result;

}

int CECAT7SubHeaderImage::rawDataSize() const
{
	return 1*ECAT_BLOCKSIZE;
}

CECATSubHeader::Type CECAT7SubHeaderImage::subHeaderType(void) const
{
	return CECATSubHeader::ECAT7_Image;
}

bool CECAT7SubHeaderImage::convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader* pHead2) 
{
	ENTER();

	bool bResult = false;

	// depending on the MedIOHeader format we do have to 
	// distinguish between our copy operations.
	switch(pHead1->headerFormat())
	{
		case CMedIOHeader::ECATSubHeader:
		{
			const CECATSubHeader* eSubHeader = static_cast<const CECATSubHeader*>(pHead1);

			// depending on the source type we have to copy either every data or just 
			// some data of the src header
			switch(eSubHeader->subHeaderType())
			{
				// if the source header is also an ECAT7 one we can copy it in whole
				// via a simple memcpy()
				case CECATSubHeader::ECAT7_Image:
				{
					memcpy(&(this->m_Data), &(static_cast<const CECAT7SubHeaderImage*>(pHead1)->m_Data), sizeof(struct ECAT7SubHeader_Image));
					bResult = true;
				}
				break;

				case CECATSubHeader::Unknown:
					// for an unknown header type we do nothing
				break;
				
				#warning "non Image copy not complete"
			}
		}

		case CMedIOHeader::ECATMainHeader:
		case CMedIOHeader::ConcordeMicroPetMainHeader:
			// copying a main header into a sub header doesn't make much sense, so we
			// do nothing here
		break;

		case CMedIOHeader::ConcordeMicroPetFrameHeader:
		{
			const CConcordeFrameHeader* head = static_cast<const CConcordeFrameHeader*>(pHead1);
			// first clean up
			clear();
			// now convert 
			setData_Type(CECATSubHeader::SunShort);
			setNum_Dimensions(3);
			setRecon_Zoom(1.0);
			setScale_Factor(head->scaleFactor());
			setImage_Min(static_cast<short>(head->minimum()));
			setImage_Max(static_cast<short>(head->maximum()));
			setFrame_Duration(static_cast<int>(head->frameDuration()*1000.0));
			setFrame_Start_Time(static_cast<int>(head->frameStart()*1000.0));
			setDecay_Corr_Fctr(head->decayCorrection());
			setGate_Duration(0);
			// check for additional information
			if(pHead2)
			{
				switch(pHead2->headerFormat())
				{
					case CMedIOHeader::ConcordeMicroPetMainHeader:
					{
						D("Setting additional information to ECAT7 sub header");
						const CConcordeMainHeader* mainHeader = static_cast<const CConcordeMainHeader*>(pHead2);
						setX_Dimension(mainHeader->xDimension());
						setY_Dimension(mainHeader->yDimension());
						setZ_Dimension(mainHeader->zDimension());
						setX_Pixel_Size(mainHeader->pixelSize());
						setY_Pixel_Size(mainHeader->pixelSize());
						setZ_Pixel_Size(mainHeader->axialPlaneSize());
						setNum_R_Elements(mainHeader->yDimension());
						setNum_Angles(mainHeader->xDimension());
					};break;
					default:break;
				}
			}
			bResult = true;
		}
		break;

		case CMedIOHeader::Unknown:
			// for an unknown header type we do nothing
		break;
	}

	RETURN(bResult);
	return bResult;
}

CMedIOHeader* CECAT7SubHeaderImage::clone() const
{
	ENTER();

	CECAT7SubHeaderImage* pNewHead = new CECAT7SubHeaderImage();
	pNewHead->convertFrom(this);
	
	RETURN(pNewHead);
	return pNewHead;
}

// data acess methods
CECATSubHeader::Data_Type CECAT7SubHeaderImage::data_Type(void) const
{
	return static_cast<CECATSubHeader::Data_Type>(m_Data.Data_Type);
}

short CECAT7SubHeaderImage::num_Dimensions(void) const
{
	return m_Data.Num_Dimensions;
}

short CECAT7SubHeaderImage::x_Dimension(void) const
{
	return m_Data.X_Dimension;
}

short CECAT7SubHeaderImage::y_Dimension(void) const
{
	return m_Data.Y_Dimension;
}

short CECAT7SubHeaderImage::z_Dimension(void) const
{
	return m_Data.Z_Dimension;
}

float CECAT7SubHeaderImage::x_Offset(void) const
{
	return m_Data.X_Offset;
}

float CECAT7SubHeaderImage::y_Offset(void) const
{
	return m_Data.Y_Offset;
}

float CECAT7SubHeaderImage::z_Offset(void) const
{
	return m_Data.Z_Offset;
}

float CECAT7SubHeaderImage::recon_Zoom(void) const
{
	return m_Data.Recon_Zoom;
}

float CECAT7SubHeaderImage::scale_Factor(void) const
{
	return m_Data.Scale_Factor;
}

short CECAT7SubHeaderImage::image_Min(void) const
{
	return m_Data.Image_Min;
}

short CECAT7SubHeaderImage::image_Max(void) const
{
	return m_Data.Image_Max;
}

float CECAT7SubHeaderImage::x_Pixel_Size(void) const
{
	return m_Data.X_Pixel_Size;
}

float CECAT7SubHeaderImage::y_Pixel_Size(void) const
{
	return m_Data.Y_Pixel_Size;
}

float CECAT7SubHeaderImage::z_Pixel_Size(void) const
{
	return m_Data.Z_Pixel_Size;
}

unsigned int CECAT7SubHeaderImage::frame_Duration(void) const
{
	return m_Data.Frame_Duration;
}

unsigned int CECAT7SubHeaderImage::frame_Start_Time(void) const
{
	return m_Data.Frame_Start_Time;
}

CECAT7SubHeaderImage::Filter_Code CECAT7SubHeaderImage::filter_Code(void) const
{
	return static_cast<CECAT7SubHeaderImage::Filter_Code>(m_Data.Filter_Code);
}

float CECAT7SubHeaderImage::x_Resolution(void) const
{
	return m_Data.X_Resolution;
}

float CECAT7SubHeaderImage::y_Resolution(void) const
{
	return m_Data.Y_Resolution;
}

float CECAT7SubHeaderImage::z_Resolution(void) const
{
	return m_Data.Z_Resolution;
}

float CECAT7SubHeaderImage::num_R_Elements(void) const
{
	return m_Data.Num_R_Elements;
}

float CECAT7SubHeaderImage::num_Angles(void) const
{
	return m_Data.Num_Angles;
}

float CECAT7SubHeaderImage::z_Rotation_Angle(void) const
{
	return m_Data.Z_Rotation_Angle;
}

float CECAT7SubHeaderImage::decay_Corr_Fctr(void) const
{
	return m_Data.Decay_Corr_Fctr;
}

unsigned int CECAT7SubHeaderImage::processing_Code(void) const
{
	return m_Data.Processing_Code;
}

unsigned int CECAT7SubHeaderImage::gate_Duration(void) const
{
	return m_Data.Gate_Duration;
}

unsigned int CECAT7SubHeaderImage::r_Wave_Offset(void) const
{
	return m_Data.R_Wave_Offset;
}

unsigned int CECAT7SubHeaderImage::num_Accepted_Beats(void) const
{
	return m_Data.Num_Accepted_Beats;
}

float CECAT7SubHeaderImage::filter_Cutoff_Frequency(void) const
{
	return m_Data.Filter_Cutoff_Frequency;
}

float CECAT7SubHeaderImage::filter_Resolution(void) const
{
	return m_Data.Filter_Resolution;
}

float CECAT7SubHeaderImage::filter_Ramp_Slope(void) const
{
	return m_Data.Filter_Ramp_Slope;
}

short CECAT7SubHeaderImage::filter_Order(void) const
{
	return m_Data.Filter_Order;
}

float CECAT7SubHeaderImage::filter_Scatter_Fraction(void) const
{
	return m_Data.Filter_Scatter_Fraction;
}

float CECAT7SubHeaderImage::filter_Scatter_Slope(void) const
{
	return m_Data.Filter_Scatter_Slope;
}

const char* CECAT7SubHeaderImage::annotation(void) const 
{
	return m_Data.Annotation;
}

float CECAT7SubHeaderImage::mt_1_1(void) const
{
	return m_Data.MT_1_1;
}

float CECAT7SubHeaderImage::mt_1_2(void) const
{
	return m_Data.MT_1_2;
}

float CECAT7SubHeaderImage::mt_1_3(void) const
{
	return m_Data.MT_1_3;
}

float CECAT7SubHeaderImage::mt_2_1(void) const
{
	return m_Data.MT_2_1;
}

float CECAT7SubHeaderImage::mt_2_2(void) const
{
	return m_Data.MT_2_2;
}

float CECAT7SubHeaderImage::mt_2_3(void) const
{
	return m_Data.MT_2_3;
}

float CECAT7SubHeaderImage::mt_3_1(void) const
{
	return m_Data.MT_3_1;
}

float CECAT7SubHeaderImage::mt_3_2(void) const
{
	return m_Data.MT_3_2;
}

float CECAT7SubHeaderImage::mt_3_3(void) const
{
	return m_Data.MT_3_3;
}

float CECAT7SubHeaderImage::rfilter_Cutoff(void) const
{
	return m_Data.RFilter_Cutoff;
}

float CECAT7SubHeaderImage::rfilter_Resolution(void) const
{
	return m_Data.RFilter_Resolution;
}

short CECAT7SubHeaderImage::zfilter_Code(void) const
{
	return m_Data.ZFilter_Code;
}

short CECAT7SubHeaderImage::rfilter_Order(void) const
{
	return m_Data.RFilter_Order;
}

float CECAT7SubHeaderImage::mt_1_4(void) const
{
	return m_Data.MT_1_4;
}

float CECAT7SubHeaderImage::mt_2_4(void) const
{
	return m_Data.MT_2_4;
}

float CECAT7SubHeaderImage::mt_3_4(void) const
{
	return m_Data.MT_3_4;
}

CECAT7SubHeaderImage::Scatter_Type CECAT7SubHeaderImage::scatter_Type(void) const
{
	return static_cast<CECAT7SubHeaderImage::Scatter_Type>(m_Data.Scatter_Type);
}

CECAT7SubHeaderImage::Recon_Type CECAT7SubHeaderImage::recon_Type(void) const
{
	return static_cast<CECAT7SubHeaderImage::Recon_Type>(m_Data.Recon_Type);
}

short CECAT7SubHeaderImage::recon_Views(void) const
{
	return m_Data.Recon_Views;
}

short CECAT7SubHeaderImage::cti_Reserved(const short i) const						
{
	return m_Data.CTI_Reserved[i];
}

short CECAT7SubHeaderImage::user_Reserved(const short i) const
{
	return m_Data.User_Reserved[i];
}


// methods to modify elements of the SubHeader
void CECAT7SubHeaderImage::setData_Type(const CECATSubHeader::Data_Type dType)
{
	m_Data.Data_Type = static_cast<quint16>(dType);
}

void CECAT7SubHeaderImage::setNum_Dimensions(const short dim)						
{
	m_Data.Num_Dimensions = dim;
}

void CECAT7SubHeaderImage::setX_Dimension(const short dim)						
{
	m_Data.X_Dimension = dim;
}

void CECAT7SubHeaderImage::setY_Dimension(const short dim)						
{
	m_Data.Y_Dimension = dim;
}

void CECAT7SubHeaderImage::setZ_Dimension(const short dim)						
{
	m_Data.Z_Dimension = dim;
}

void CECAT7SubHeaderImage::setX_Offset(const float off)						
{
	m_Data.X_Offset = off;
}

void CECAT7SubHeaderImage::setY_Offset(const float off)						
{
	m_Data.Y_Offset = off;
}

void CECAT7SubHeaderImage::setZ_Offset(const float off)						
{
	m_Data.Z_Offset = off;
}

void CECAT7SubHeaderImage::setRecon_Zoom(const float zoom)						
{
	m_Data.Recon_Zoom = zoom;
}

void CECAT7SubHeaderImage::setScale_Factor(const float scale)						
{
	m_Data.Scale_Factor = scale;
}

void CECAT7SubHeaderImage::setImage_Min(const short min)						
{
	m_Data.Image_Min = min;
}

void CECAT7SubHeaderImage::setImage_Max(const short max)						
{
	m_Data.Image_Max = max;
}

void CECAT7SubHeaderImage::setX_Pixel_Size(const float size)						
{
	m_Data.X_Pixel_Size = size;
}

void CECAT7SubHeaderImage::setY_Pixel_Size(const float size)						
{
	m_Data.Y_Pixel_Size = size;
}

void CECAT7SubHeaderImage::setZ_Pixel_Size(const float size)						
{
	m_Data.Z_Pixel_Size = size;
}

void CECAT7SubHeaderImage::setFrame_Duration(const unsigned int dur)		
{
	m_Data.Frame_Duration = dur;
}

void CECAT7SubHeaderImage::setFrame_Start_Time(const unsigned int time)		
{
	m_Data.Frame_Start_Time = time;
}

void CECAT7SubHeaderImage::setFilter_Code(const Filter_Code code)						
{
	m_Data.Filter_Code = static_cast<quint16>(code);
}

void CECAT7SubHeaderImage::setX_Resolution(const float res)						
{
	m_Data.X_Resolution = res;
}

void CECAT7SubHeaderImage::setY_Resolution(const float res)						
{
	m_Data.Y_Resolution = res;
}

void CECAT7SubHeaderImage::setZ_Resolution(const float res)						
{
	m_Data.Z_Resolution = res;
}

void CECAT7SubHeaderImage::setNum_R_Elements(const float elem)						
{
	m_Data.Num_R_Elements = elem;
}

void CECAT7SubHeaderImage::setNum_Angles(const float angles)							
{
	m_Data.Num_Angles = angles;
}

void CECAT7SubHeaderImage::setZ_Rotation_Angle(const float angle)						
{
	m_Data.Z_Rotation_Angle = angle;
}

void CECAT7SubHeaderImage::setDecay_Corr_Fctr(const float factor)							
{
	m_Data.Decay_Corr_Fctr = factor;
}

void CECAT7SubHeaderImage::setProcessing_Code(const unsigned int code)		
{
	m_Data.Processing_Code = code;
}

void CECAT7SubHeaderImage::setGate_Duration(const unsigned int dur)		
{
	m_Data.Gate_Duration = dur;
}

void CECAT7SubHeaderImage::setR_Wave_Offset(const unsigned int off)		
{
	m_Data.R_Wave_Offset = off;
}

void CECAT7SubHeaderImage::setNum_Accepted_Beats(const unsigned int beats)		
{
	m_Data.Num_Accepted_Beats = beats;
}

void CECAT7SubHeaderImage::setFilter_Cutoff_Frequency(const float freq)						
{
	m_Data.Filter_Cutoff_Frequency = freq;
}

void CECAT7SubHeaderImage::setFilter_Resolution(const float res)						
{
	m_Data.Filter_Resolution = res;
}

void CECAT7SubHeaderImage::setFilter_Ramp_Slope(const float slope)						
{
	m_Data.Filter_Ramp_Slope = slope;
}

void CECAT7SubHeaderImage::setFilter_Order(const short order)						
{
	m_Data.Filter_Order = order;
}

void CECAT7SubHeaderImage::setFilter_Scatter_Fraction(const float frac)						
{
	m_Data.Filter_Scatter_Fraction = frac;
}

void CECAT7SubHeaderImage::setFilter_Scatter_Slope(const float slope)						
{
	m_Data.Filter_Scatter_Slope = slope;
}

void CECAT7SubHeaderImage::setAnnotation(const char* str)													
{
	strncpy(m_Data.Annotation, str, 40);
}

void CECAT7SubHeaderImage::setMT_1_1(const float value)						
{
	m_Data.MT_1_1 = value;
}

void CECAT7SubHeaderImage::setMT_1_2(const float value)						
{
	m_Data.MT_1_2 = value;
}

void CECAT7SubHeaderImage::setMT_1_3(const float value)						
{
	m_Data.MT_1_3 = value;
}

void CECAT7SubHeaderImage::setMT_2_1(const float value)						
{
	m_Data.MT_2_1 = value;
}

void CECAT7SubHeaderImage::setMT_2_2(const float value)						
{
	m_Data.MT_2_2 = value;
}

void CECAT7SubHeaderImage::setMT_2_3(const float value)						
{
	m_Data.MT_2_3 = value;
}

void CECAT7SubHeaderImage::setMT_3_1(const float value)						
{
	m_Data.MT_3_1 = value;
}

void CECAT7SubHeaderImage::setMT_3_2(const float value)						
{
	m_Data.MT_3_2 = value;
}

void CECAT7SubHeaderImage::setMT_3_3(const float value)						
{
	m_Data.MT_3_3 = value;
}

void CECAT7SubHeaderImage::setRFilter_Cutoff(const float cutoff)						
{
	m_Data.RFilter_Cutoff = cutoff;
}

void CECAT7SubHeaderImage::setRFilter_Resolution(const float res)						
{
	m_Data.RFilter_Resolution = res;
}

void CECAT7SubHeaderImage::setZFilter_Code(const short code)						
{
	m_Data.ZFilter_Code = code;
}

void CECAT7SubHeaderImage::setRFilter_Order(const short order)						
{
	m_Data.RFilter_Order = order;
}

void CECAT7SubHeaderImage::setMT_1_4(const float value)						
{
	m_Data.MT_1_4 = value;
}

void CECAT7SubHeaderImage::setMT_2_4(const float value)						
{
	m_Data.MT_2_4 = value;
}

void CECAT7SubHeaderImage::setMT_3_4(const float value)						
{
	m_Data.MT_3_4 = value;
}

void CECAT7SubHeaderImage::setScatter_Type(const Scatter_Type type)						
{
	m_Data.Scatter_Type = static_cast<quint16>(type);
}

void CECAT7SubHeaderImage::setRecon_Type(const Recon_Type type)						
{
	m_Data.Recon_Type = static_cast<quint16>(type);
}

void CECAT7SubHeaderImage::setRecon_Views(const short views)						
{
	m_Data.Recon_Views = views;
}

void CECAT7SubHeaderImage::setCTI_Reserved(const short i, const short n)						
{
	m_Data.CTI_Reserved[i] = n;
}

void CECAT7SubHeaderImage::setUser_Reserved(const short i, const short n)						
{
	m_Data.User_Reserved[i] = n;
}
