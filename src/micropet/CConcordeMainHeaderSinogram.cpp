#include "CConcordeMainHeaderSinogram.h"

unsigned int CConcordeMainHeaderSinogram::frameSize()
{	
	int framesize = 0;
	int typesize = 0;
	//calculate the frame size: 
	//frame_size = number_of_sinograms*x_dimension*y_dimension*data_type_size
	//number_of_dimensions = sum(sinograms) over all delta_elements
	if(m_Data.data_type == 2 || m_Data.data_type == 6)
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
	
	for(int i = 0; i < 11; i++) framesize += m_Data.delta_elements[i];
	framesize = framesize*m_Data.x_dimension*m_Data.y_dimension*typesize;
	return framesize;
}

CMedIOHeader* CConcordeMainHeaderSinogram::clone()
{
	CConcordeMainHeaderSinogram* pTmp = new CConcordeMainHeaderSinogram(NULL);
	if(pTmp->convertFrom(this))
		return pTmp;
	else
		return NULL;
}
