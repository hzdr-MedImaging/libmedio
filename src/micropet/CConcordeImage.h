#ifndef CCONCORDEIMAGE_H
#define CCONCORDEIMAGE_H

#include "CConcordeFile.h"

#include <qstring.h>

class CConcordeImage : public CConcordeFile
{
	public :
	//contructors
		CConcordeImage(QString file);
		
	//destructor
		~CConcordeImage();
		
	//members
		
		
	//methods
		QByteArray* get(short frame, short plane, 
			short gate, short bed, short data);
		int rtti() {return CMedIOData::ConcordeMicropet_Image;}
		
	private :
	//members
		
	//methods
};

#endif
