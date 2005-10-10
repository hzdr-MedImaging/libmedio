//! @file CConcordeMainHeaderSinogram.h
//! @author Hagen Moelle
//! @date 09/10/2005
//! @brief contains definition of the class CConcordeMainHeaderSinogram

#ifndef CCONCORDEMAINHEADERSINOGRAM_H
#define CCONCORDEMAINHEADERSINOGRAM_H

#include "CConcordeMainHeader.h"

class CConcordeMainHeaderSinogram : public CConcordeMainHeader
{
	public:
		CConcordeMainHeaderSinogram(QString File) : CConcordeMainHeader(File){}
		CConcordeMainHeaderSinogram(CConcordeFile* file = NULL) : CConcordeMainHeader(file){}
		unsigned int frameSize();
};

#endif
