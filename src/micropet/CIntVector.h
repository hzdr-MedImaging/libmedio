#ifndef CINTVECTOR_H
#define CINTVECTOR_H

#include <qvaluelist.h>

class CIntVector : public QValueList<int>
{
	public:
		void parseKeyValue(QString KeyValue);
		static void wrapper_parseKeyValue(void* p2Object, QString KeyValue);
};

#endif
