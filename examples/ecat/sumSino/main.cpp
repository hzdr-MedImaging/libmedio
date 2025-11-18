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

#include "CIOTransform.h"

#include <QApplication>
#include <QThread>
#include <QHash>
#include <QFileInfo>
#include <QStringList>

void printHelp(const QString& name)
{
  cout << "sumSino 1.1 - an ECAT7 tool for summation of sinogram matrices" << endl;
  cout << "(" << __DATE__ << ")  Copyright (c) 2007 by S.Dittrich, C.Poetzsch, J.Maus / hzdr.de" << endl;
  cout << endl;
  cout << "Usage: " << name.toLatin1().data() << " <options> infile1.S [infile2.S ...]" << endl;
  cout << "Options:" << endl;
  cout << "  -h             : this help page" << endl;
  cout << "  -o <file>      : output file name (default: first input file + '_sum.S')." << endl;
  cout << "  -f             : force overwrite of existing files." << endl;
  cout << endl;
  cout << "Examples:" << endl;
  cout << endl;
}

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  // You want this, unless you mix couts output with C output.
  // Read  http://gcc.gnu.org/onlinedocs/libstdc++/27_io/howto.html#8 for an explanation.
  ios::sync_with_stdio(false);

  // provide some variables to check the commandline options
  // properly.
  QHash<QString, QString> args;

  // and all potential input filenames into a QStringList
  QStringList inputFileNames; 

  // if the user has specified some commandline options
  // lets process and parse them.
  for(int i=1; i < argc; i++)
  {
    QString option(argv[i]);

    if(option[0] == '-')
    {
      QString nextOption(argv[i+1]);

      if(i+1 < argc && (nextOption[0] != '-' || nextOption[0].isLetter() == false))
        args.insert(option, nextOption);
      else
        args.insert(option, "");
    }
    else 
      inputFileNames << argv[i];
  }

  // Help needed?
  //if(args.isEmpty() || args.contains("-h")) // help page requested
  if(args.contains("-h")) // help page requested
  {
    printHelp(argv[0]);
    return 1;
  }

  QString outfile;
  if(args.contains("-o"))
  {
    outfile = args.value("-o");
    inputFileNames.removeAll(args.value("-o"));
  }
  
  if(inputFileNames.isEmpty())
  {
    cerr << "Error: No input file given!" << endl;
    return 1;
  }

  QFileInfo fi(inputFileNames[0]);
  if(fi.exists() == false)
  {
    cerr << "ERROR: required input file '" << inputFileNames[0].toLatin1().constData() << "' does not exist." << endl;
    return 1;
  }

  if(outfile.isEmpty())
  {
    outfile = inputFileNames[0];
    outfile.insert(outfile.lastIndexOf("."),QString("_sum"));
  }

  // check if outfile already exists
  if(args.contains("-f") == false)
  {
    QFileInfo fi(outfile);
    if(fi.exists())
    {
      cerr << "Error: output file already exists!" << endl;
      return 1;
    }
  }

  CIOTransform main(inputFileNames);
  main.setOutfile(outfile);
  
  return main.exec();
}
