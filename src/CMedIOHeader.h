#ifndef CMEDIOHEADER_H
#define CMEDIOHEADER_H

#include <qstring.h>

class CMedIOHeader
{
	public :
	//contructors
		CMedIOHeader(QString file);
		CMedIOHeader(){}
	//destructor
		virtual ~CMedIOHeader();
		
	//members
		enum Format {ConcordeMicropet=0};
		
	//methods
		virtual bool load() = 0;
		virtual bool save() = 0;
		virtual int rtti() = 0;
		
		virtual int numframes(void) = 0;
		virtual void setNumFrames(int value) = 0;
		
		QString file();
		
	private :
	//members
		QString File;
	//methods
};

#endif
