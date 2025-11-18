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

#ifndef CAPPLICATION_H
#define CAPPLICATION_H

#include <QString>
#include <QStringList>
#include <QDir>

class CApplication
{
  public:
    //constructors
    CApplication();

    //methods
    bool parseCmdLine(int argc, char* argv[]);
    bool process();

  protected:
    bool convertFile(const QFileInfo& inputFile);
    bool walkDirectory(const QDir& dir);

  private:
    // methods
    void showUsage(int argc, char* argv[]);
    void showAppInfo();
    void showVersion();
    bool checkOutputFile(const QFileInfo& inputFile);

    // members
    QStringList m_sInputFileNames;
    QString     m_sOutputFileName;
    QString     m_sPreferedOutputFile;
    QString     m_sPatientName;
    QString     m_sStartDirectory;
    bool        m_bOverWrite;
    bool        m_bPreserveDataType;
    bool        m_bRecursive;
};

#endif // CAPPLICATION_H
