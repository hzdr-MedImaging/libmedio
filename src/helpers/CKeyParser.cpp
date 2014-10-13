#include "CKeyParser.h"

#include <QTextStream>
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

void CKeyParser::addKey(const QString& Key, void* p2Object, void (*p2Function)(void* p2Object, QString KeyValue))
{
  if(p2Object!=NULL)
  {
    CKeyValue* KValue = new CKeyValue(CKeyType::USEROBJECT, p2Object, p2Function);
    KeyDictionary.insert(Key,KValue);
  }    
}

bool CKeyParser::parse(const QString Filename)
{
  QFile file( Filename );
  if(!file.open(QIODevice::ReadOnly))
  {
    E("Opening of file %s failed!", Filename.toLatin1().data());
    return false;
  }

  // using QTextStream to read a file on harddisk takes a long time
  // so we read the file all at once 
  //int iFileSize = file.size();

  //QByteArray* p_Tmp = new QByteArray(iFileSize, 0);
  //file.read(p_Tmp->data(), iFileSize);

  // do not read the file line by line anymore - instead use QByteArray
  QTextStream ts(&file);
  //QTextStream ts(p_Tmp);
  
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
  while(!ts.atEnd())
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
          int start_index = Line.indexOf(*it);
    //if separator is found
    if(start_index != -1)
    {
      //extract keyword -> all characters infront (left of) separator
      Keyword = Line.left(start_index);
      Keyword.simplified();
      //extract value -> all characters after (right of) separator
      int start_index_value = start_index + (*it).length();
      Value = Line.right(Line.length()-start_index_value);
      Value.simplified();
      break;  
    }
      }
  //D("Line : %s", Line.toLatin1().data());
  //D("Keyword : %s : Value : %s", Keyword.toLatin1().data(), Value.toLatin1().data());
  if(Keyword == 0 || Value == 0)
    return false;

  //lookup keyword in dictionary
  QHash<QString, CKeyValue*>::iterator it = KeyDictionary.find(Keyword);
  if(it == KeyDictionary.end())
    return false;

  CKeyValue* KValue = it.value();
  
  switch(KValue->getType())
  {
    case CKeyType::ASCII:
    {
      //D("is an ascii value");
      *((QString*)KValue->getPToValue()) = Value;
    }
    break;
    case CKeyType::DOUBLE:
    {
      //D("is a double value");
      *((double*)KValue->getPToValue()) = Value.toDouble();
    }
    break;
    case CKeyType::SHORT:
    {
      //D("is a short value");
      *((short*)KValue->getPToValue()) = Value.toShort();
    }
    break;
    case CKeyType::USHORT:
    {
      //D("is an ushort value");
      *((unsigned short*)KValue->getPToValue()) = Value.toUShort();
    }
    break;
    case CKeyType::INT:
    {
      //D("is a int value");
      *((int*)KValue->getPToValue()) = Value.toInt();
    }
    break;
    case CKeyType::UINT:
    {
      //D("is an uint value");
      *((unsigned int*)KValue->getPToValue()) = Value.toUInt();
    }
    break;
    case CKeyType::LONG:
    {
      //D("is a long value");
      *((long*)KValue->getPToValue()) = Value.toLong();
    }
    break;
    case CKeyType::ULONG:
    {
      //D("is an ulong value");
      *((unsigned long*)KValue->getPToValue()) = Value.toULong();
    }
    break;
    case CKeyType::FLOAT:
    {
      //D("is a float value");
      *((float*)KValue->getPToValue()) = Value.toFloat();
    }
    break;
    case CKeyType::USEROBJECT:
    {
      //D("is an user defined object value");
      KValue->callBack(Value);
    }
    break;
  }
  return true;
}
