#ifndef CMEDIODATA_H
#define CMEDIODATA_H

#include <qstring.h>

#include "CMedIOHeader.h"

class CMedIOData
{
	public :
	//contructors
		CMedIOData(QString file);
	//destructor
		virtual ~CMedIOData();
		
	//members
		enum Format {	Unknown = 0,
				ConcordeMicropet_Sinogram,
				ConcordeMicropet_Image,
				ECAT7_Volume16
				};
		
	//methods
		virtual bool load() = 0;
		virtual bool save() = 0;
		virtual int rtti() = 0;
		//virtual int isoftype(QString file) = 0;
		CMedIOHeader* header();
		void setHeader(CMedIOHeader* head);
		QString file();
		
		
	private :
	//members
		CMedIOHeader* Header;
		QString File;
	//methods
};

#endif
