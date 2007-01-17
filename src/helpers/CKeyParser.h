/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004-2007 by Jens Langner <Jens.Langner@light-speed.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

#ifndef CKEYPARSER_H
#define CKEYPARSER_H

#include <QFile>
#include <QString>
#include <QHash>
#include <QStringList>

#include "CKeyValue.h"

class CKeyParser
{
	public:
		//constructor
		CKeyParser(){};		
		//destructor
		~CKeyParser(){};
		//methods
		void addKey(const QString& Key, QString* Value);
		void addKey(const QString& Key, double* Value);
		void addKey(const QString& Key, short* Value);
		void addKey(const QString& Key, unsigned short* Value);
		void addKey(const QString& Key, int* Value);
		void addKey(const QString& Key, unsigned int* Value);
		void addKey(const QString& Key, long* Value);
		void addKey(const QString& Key, unsigned long* Value);
		void addKey(const QString& Key, float* Value);
		void addKey(const QString& Key, void* p2Object, void (*p2Function)(void* p2Object, QString KeyValue));
		//void addKey(const QString& Key, void* variable, 
		//	void* p2Object, void (*p2Function)(void* p2Object, void* variable, QString KeyValue));

		bool parse(const QString Filename);

		void addSeparator(const QString Sep){Separators.append(Sep);};
		void addComment(const QString Com){Comments.append(Com);};
		void addStartSymbol(const QString Start){StartSymbols.append(Start);};
		void addStopSymbol(const QString Stop){StopSymbols.append(Stop);};

		void clearSeparators(){Separators.clear();};
		void clearComments(){Comments.clear();};
		void clearStartSymbols(){StartSymbols.clear();};
		void clearStopSymbols(){StopSymbols.clear();};
		
	protected:

	private:
		//members
		QHash<QString, CKeyValue*> KeyDictionary;
		QStringList Separators;
		QStringList Comments;
		QStringList StartSymbols;
		QStringList StopSymbols;

		//methods
		bool processLine(QString& Line);
};

#endif
