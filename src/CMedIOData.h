#ifndef CMEDIODATA_H
#define CMEDIODATA_H

#include "CMedIOHeader.h"

#include <qstring.h>

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
		virtual Format rtti() = 0;
		//virtual Format isoftype(QString file) = 0;
		CMedIOHeader* getHeader();
		void setHeader(CMedIOHeader* head);
		QString getFile();
		
		
	private :
	//members
		CMedIOHeader* header;
		QString file;
		//CRawData* rawdata;
	//methods
};

#endif
