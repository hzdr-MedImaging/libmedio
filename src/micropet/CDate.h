#ifndef CDATE_H
#define CDATE_H

#include <QDateTime>

class CDate : public QDateTime
{
	public:
		void parseKeyValue(QString KeyValue);
		static void wrapper_parseKeyValue(void* p2Object, QString KeyValue);
};

#endif
