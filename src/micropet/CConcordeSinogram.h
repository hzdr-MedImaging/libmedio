#ifndef CCONCORDESINOGRAM_H
#define CCONCORDESINOGRAM_H

#include "CConcordeFile.h"

#include <qstring.h>

class CConcordeSinogram : public CConcordeFile
{
	public :
	//contructors
		CConcordeSinogram(QString file);
		
	//destructor
		~CConcordeSinogram();
		
	//members
		
		
	//methods
		QByteArray* get(short frame, short plane, 
			short gate, short bed, short data);
		int rtti() {return CMedIOData::ConcordeMicropet_Sinogram;}
		
	private :
	//members
		
	//methods
};

#endif
