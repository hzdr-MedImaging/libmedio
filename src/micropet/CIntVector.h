#ifndef CINTVECTOR_H
#define CINTVECTOR_H

#include <QList>
#include <QString>

class CIntVector : public QList<int>
{
	public:
		void parseKeyValue(QString KeyValue);
		static void wrapper_parseKeyValue(void* p2Object, QString KeyValue);
};

#endif
