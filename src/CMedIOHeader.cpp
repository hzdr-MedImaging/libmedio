#include "CMedIOHeader.h"
#include "debug.h"

CMedIOHeader::CMedIOHeader(QString file)
{
	File = file;
}

CMedIOHeader::~CMedIOHeader()
{
	ENTER();
	LEAVE();
}

QString CMedIOHeader::file()
{
	return File;
}
