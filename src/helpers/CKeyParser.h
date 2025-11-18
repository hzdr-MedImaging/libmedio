/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio
 
  Copyright (C) 2004-2025 hzdr.de and contributors
 
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
    http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

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
    //  void* p2Object, void (*p2Function)(void* p2Object, void* variable, QString KeyValue));

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
