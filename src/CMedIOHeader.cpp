#include "CMedIOHeader.h"
#include "debug.h"

CMedIOHeader::CMedIOHeader(QString file)
{
	this->file = file;
}

CMedIOHeader::~CMedIOHeader()
{
	ENTER();
	LEAVE();
}

QString CMedIOHeader::getFile()
{
	return file;
}
