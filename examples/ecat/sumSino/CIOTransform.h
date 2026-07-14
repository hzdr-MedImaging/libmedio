/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio
 
  Copyright (C) 2004-2026 hzdr.de and contributors
 
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

#ifndef CPROJECTION_H
#define CPROJECTION_H

#include "CMultiDimArray.h"

#include <CECATSubHeader>

#include <QString>
#include <QStringList>

class CECATFile;
class CECAT7MainHeader;
class CECAT7SubHeaderImage;
class CECAT7SubHeaderScan3D;

class CIOTransform
{
public:
  enum OutputType { EcatOutput, ImageOutput };

  CIOTransform(const QStringList& filelist);
  ~CIOTransform();

  int exec();

  void setOutfile(const QString& outfile) {m_outfile = outfile;}

private:
  bool openInputFile();
  bool loadVolume(int frame=1, int plane=1, int gate=1, int bed=0, int data=0);
  void convertMatrixData(float*** pDestination, void* pSource, CECATSubHeader::Data_Type dt, int iXDim, int iYDim, int iZDim, float fScaleFactor, float &fMin, float &fMax) const;

  QString m_filename;
  QStringList m_InfileList;
  QString m_tfmfile;
  QString m_outfile;
  QString m_sTfmString;
  bool m_bForward;
  bool m_bAllTfm;

  CECATFile* m_pInputFile;
  CECAT7MainHeader* m_pMainHead;
  CECAT7SubHeaderScan3D* m_pSubHead;
  C3DArray<float>* m_pVolume;
  char *m_pRawVolume;
  unsigned int m_iRawSize;
  unsigned int m_iNum_Z;
  unsigned int m_iNum_Y;
  unsigned int m_iNum_X;

  float m_min;
  float m_max;

};

#endif // CPROJECTION_H
