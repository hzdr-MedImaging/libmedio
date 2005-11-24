//! @file CConcordeMainHeaderImage.h
//! @author Hagen Moelle
//! @date 09/10/2005
//! @brief contains definition of the class CConcordeMainHeaderImage

#ifndef CCONCORDEMAINHEADERIMAGE_H
#define CCONCORDEMAINHEADERIMAGE_H

#include "CConcordeMainHeader.h"

class CConcordeMainHeaderImage : public CConcordeMainHeader
{
	public:
		CConcordeMainHeaderImage(QString File) : CConcordeMainHeader(File){};
		CConcordeMainHeaderImage(CConcordeFile* file) : CConcordeMainHeader(file){};
		unsigned int frameSize();
		CMedIOHeader* clone() const;
};

#endif
