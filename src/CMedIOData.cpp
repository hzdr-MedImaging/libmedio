#include "CMedIOData.h"
#include "debug.h"

#include <qstring.h>

CMedIOData::CMedIOData(QString file)
{
	this->file = file;
}

CMedIOData::~CMedIOData()
{
	ENTER();
	delete header;
	LEAVE();
}

CMedIOHeader* CMedIOData::getHeader()
{
	return header;
}

void CMedIOData::setHeader(CMedIOHeader* head)
{
	header = head;
}

QString CMedIOData::getFile()
{
	return file;
}
