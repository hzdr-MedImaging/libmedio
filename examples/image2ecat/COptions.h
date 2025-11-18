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

#ifndef COPTION3_HH
#define COPTION3_HH

// qt includes
#include <QList>
#include <QStringList>
#include <QObject>

class COptions : public QObject
{

public:

  COptions();

  void read_options(int argc, char **argv);

  QString inputFile() const {return m_InputFile;}
  QStringList inputFileList() const {return m_InputFileList;}
  QString outputFile() const {return m_OutputFile;}

  float xDim() const {return m_xDim;}
  void setXDim(float d) {m_xDim = d;}

  float yDim() const {return m_yDim;}
  void setYDim(float d) {m_zDim = d;}

  float zDim() const {return m_zDim;}
  void setZDim(float d) {m_zDim = d;}

  float xSize() const {return m_xSize;}
  float ySize() const {return m_ySize;}
  float zSize() const {return m_zSize;}

  bool force() const {return m_Force;}
  bool rgb() const {return m_Rgb;}
  bool verbose() const {return m_Verbose;} 

  int rescale() const {return m_Rescale;}

  bool block() const {return m_Block;}
  int pixel() const {return m_Pixel;}

protected:

  QString m_InputFile;
  QStringList m_InputFileList;
  QString m_OutputFile;

  float m_xDim;
  float m_yDim;
  float m_zDim;

  float m_xSize;
  float m_ySize;
  float m_zSize;

  bool m_Force;
  bool m_Rgb;
  bool m_Verbose;

  int m_Rescale;

  bool m_Block;
  int m_Pixel;

  void usage();
};


#endif
