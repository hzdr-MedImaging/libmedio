#include "CMedIOData.h"
#include "debug.h"

#include <qstring.h>

CMedIOData::CMedIOData(QString file)
{
	File = file;
}

CMedIOData::~CMedIOData()
{
	ENTER();
	delete Header;
	LEAVE();
}

CMedIOHeader* CMedIOData::header()
{
	return Header;
}

void CMedIOData::setHeader(CMedIOHeader* head)
{
	Header = head;
}

QString CMedIOData::file()
{
	return File;
}
