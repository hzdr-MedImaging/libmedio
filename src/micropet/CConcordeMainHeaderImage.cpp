#include "CConcordeMainHeaderImage.h"

//  Class: CConcordeMainHeaderImage
//  Method: frameSize
//!
//! @return framesize of a imagevolume in bytes
////////////////////////////////////////////////////////////////////////////////
unsigned int CConcordeMainHeaderImage::frameSize()
{	
	int framesize = 0;
	int typesize = 0;
	//calculate the imageframe size: 
	//framesize = x_dimension*y_dimension*z_dimension*data_type_size
	if(m_Data.data_type == 2 | m_Data.data_type == 6)
	{
		typesize = 2;
	}
	else 
		typesize = 4;
	
	if(m_Data.data_type == 0)
	{
		typesize = 1;
	}
	if(m_Data.data_type == 1)
	{
		typesize = 1;
	}
	
	framesize = m_Data.x_dimension*m_Data.y_dimension*m_Data.z_dimension*typesize;
	return framesize;
}
