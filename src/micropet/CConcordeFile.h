#ifndef CCONCORDEFILE_H
#define CCONCORDEFILE_H

#include "CMedIOData.h"

class CConcordeFile : public CMedIOData
{
	public :
	//contructors
		CConcordeFile(QString file);
	//destructor
		virtual ~CConcordeFile();
	//members
		bool load();
		bool save();
		virtual int rtti() = 0;
	//methods
		static int isoftype(QString file);
		
	private :
	//members
		
	//methods
};

#endif
