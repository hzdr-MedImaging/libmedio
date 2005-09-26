#include "CKeyParser.h"
#include <rtdebug.h>

void CKeyParser::addKey(const QString& Key, QString* Value)
{
	if(Value!=NULL)
	{
		CKeyValue* KValue = new CKeyValue(CKeyType::ASCII, (void*)Value);
		KeyDictionary.insert(Key,KValue);
	}
}

void CKeyParser::addKey(const QString& Key, double* Value)
{
	if(Value!=NULL)
	{
		CKeyValue* KValue = new CKeyValue(CKeyType::DOUBLE, (void*)Value);
		KeyDictionary.insert(Key,KValue);
	}
}

void CKeyParser::addKey(const QString& Key, short* Value)
{
	if(Value!=NULL)
	{
		CKeyValue* KValue = new CKeyValue(CKeyType::SHORT, (void*)Value);
		KeyDictionary.insert(Key,KValue);
	}
}

void CKeyParser::addKey(const QString& Key, unsigned short* Value)
{
	if(Value!=NULL)
	{
		CKeyValue* KValue = new CKeyValue(CKeyType::USHORT, (void*)Value);
		KeyDictionary.insert(Key,KValue);
	}
}

void CKeyParser::addKey(const QString& Key, int* Value)
{
	if(Value!=NULL)
	{
		CKeyValue* KValue = new CKeyValue(CKeyType::INT, (void*)Value);
		KeyDictionary.insert(Key,KValue);
	}
}

void CKeyParser::addKey(const QString& Key, unsigned int* Value)
{
	if(Value!=NULL)
	{
		CKeyValue* KValue = new CKeyValue(CKeyType::UINT, (void*)Value);
		KeyDictionary.insert(Key,KValue);
	}
}

void CKeyParser::addKey(const QString& Key, long* Value)
{
	if(Value!=NULL)
	{
		CKeyValue* KValue = new CKeyValue(CKeyType::LONG, (void*)Value);
		KeyDictionary.insert(Key,KValue);
	}
}

void CKeyParser::addKey(const QString& Key, unsigned long* Value)
{
	if(Value!=NULL)
	{
		CKeyValue* KValue = new CKeyValue(CKeyType::ULONG, (void*)Value);
		KeyDictionary.insert(Key,KValue);
	}
}

void CKeyParser::addKey(const QString& Key, float* Value)
{
	if(Value!=NULL)
	{
		CKeyValue* KValue = new CKeyValue(CKeyType::FLOAT, (void*)Value);
		KeyDictionary.insert(Key,KValue);
	}
}

bool CKeyParser::parse(const QString Filename)
{
	QFile file( Filename );
        if(!file.open(IO_ReadOnly))
	{
        	E("Opening of file %s failed!", Filename.ascii());
		return false;
        }

	QTextStream ts(&file);
	
	//only parse lines if either a start symbol or no stop symbol is found in file
	bool start_symbol_found = false;
	bool stop_symbol_found = false;	
	
	if(Separators.isEmpty())
	{
		E("No separators defined -> key parsing will not work without separators");		
		return false;
	}
	
	if(KeyDictionary.isEmpty())
	{
		W("No Keys found to parse -> key parsing will not work without keys");		
		return false;
	}
	
	//parse whole file
	while(!ts.eof())
	{
		//reading a line of the file
		QString Line = ts.readLine();
		
		//if line contains no content -> skip line
		if(Line.length() == 0)
			continue;
		
		//if line does not contain a start symbol -> skip line till one is found
		if(!start_symbol_found && !StartSymbols.isEmpty())
		{
			for(QStringList::Iterator it = StartSymbols.begin(); it != StartSymbols.end(); ++it ) 
			{
        			if(Line.startsWith(*it))
				{
					start_symbol_found = true;
					break;
				}
    			}
			//if line is not a start symbol -> skip it
			if(!start_symbol_found)
				continue;
		}

		//check if line is a comment -> skip line if so
		bool is_comment = false;
		for(QStringList::Iterator it = Comments.begin(); it != Comments.end(); ++it ) 
		{
        		if(Line.startsWith(*it))
			{
				is_comment = true;
				break;
			}
    		}
		if(is_comment)
			continue;
		
		//check if line is a stop symbol -> end with parsing of file
		if(!stop_symbol_found)
		{
			for(QStringList::Iterator it = StopSymbols.begin(); it != StopSymbols.end(); ++it ) 
			{
        			if(Line.startsWith(*it))
				{
					stop_symbol_found = true;
					break;
				}
    			}
			//stop parsing if stop symbol was found
			if(stop_symbol_found)
				break;	
		}
		processLine(Line);
	}
	file.close();
	return true;
}

bool CKeyParser::processLine(QString& Line)
{
	//check if keyword in line is existing in dictionary
	QString Keyword = 0;
	QString Value = 0;

	for(QStringList::Iterator it = Separators.begin(); it != Separators.end(); ++it ) 
	{
        	int start_index = Line.find(*it);
		//if separator is found
		if(start_index != -1)
		{
			//extract keyword -> all characters infront (left of) separator
			Keyword = Line.left(start_index);
			Keyword.stripWhiteSpace();
			//extract value -> all characters after (right of) separator
			int start_index_value = start_index + (*it).length();
			Value = Line.right(Line.length()-start_index_value);
			Value.stripWhiteSpace();
			break;	
		}
    	}
	if(!Keyword || !Value)
		return false;

	//lookup keyword in dictionary
	CKeyValue* KValue = KeyDictionary.find(Keyword);
	if(KValue == NULL)
		return false;

	if(KValue->getType() == CKeyType::ASCII)
		*((QString*)KValue->getPToValue()) = Value;
	
	else if(KValue->getType() == CKeyType::DOUBLE)
		*((double*)KValue->getPToValue()) = Value.toDouble();

	else if(KValue->getType() == CKeyType::SHORT)
		*((short*)KValue->getPToValue()) = Value.toShort();

	else if(KValue->getType() == CKeyType::USHORT)
		*((unsigned short*)KValue->getPToValue()) = Value.toUShort();

	else if(KValue->getType() == CKeyType::INT)
		*((int*)KValue->getPToValue()) = Value.toInt();

	else if(KValue->getType() == CKeyType::UINT)
		*((unsigned int*)KValue->getPToValue()) = Value.toUInt();

	else if(KValue->getType() == CKeyType::LONG)
		*((long*)KValue->getPToValue()) = Value.toLong();

	else if(KValue->getType() == CKeyType::ULONG)
		*((unsigned long*)KValue->getPToValue()) = Value.toULong();

	else if(KValue->getType() == CKeyType::FLOAT)
		*((float*)KValue->getPToValue()) = Value.toFloat();
	
	return true;
}
