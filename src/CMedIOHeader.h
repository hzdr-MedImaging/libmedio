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
		virtual Format rtti() = 0;
		QString getFile();
		
	private :
	//members
		QString file;
	//methods
};

#endif
