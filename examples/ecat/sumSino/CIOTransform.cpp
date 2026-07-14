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

#include "CIOTransform.h"
//#include "CVolumeTransformer.h"
     
// libmedio
#include <CECATFile>
#include <CECATMainHeader>
#include <CECAT7MainHeader>
#include <CECAT7SubHeaderImage>
#include <CECAT7SubHeaderScan3D>

// libmtrack

#include <QImage>
#include <QVector>

#include <iostream>
#include <iomanip>

using namespace std;

CIOTransform::CIOTransform(const QStringList& filelist)
: m_filename(filelist[0]),
  m_InfileList(filelist),
  m_bForward(true),
  m_bAllTfm(false),
  m_pInputFile(NULL),
  m_pMainHead(NULL),
  m_pSubHead(NULL),
  m_pVolume(NULL),
  m_pRawVolume(NULL),
  m_iRawSize(0),
  m_min(0.0),
  m_max(0.0)
{
}
  
CIOTransform::~CIOTransform()
{
  delete m_pMainHead;
  delete m_pSubHead;
  delete m_pInputFile;
  delete m_pVolume;
  delete []m_pRawVolume;
}

int CIOTransform::exec()
{
  int result = 1;

  // open the ECAT input file
  if(openInputFile())
  {
    cout << "loaded input file with " << m_pMainHead->num_Frames() << " frames." << endl;

    // prepare the output file
    CECATFile file(m_outfile, CECATMainHeader::ECAT7_Sinogram3D_16);
    if(file.open(QIODevice::WriteOnly))
    {
      CECAT7MainHeader* pE7MainHead = static_cast<CECAT7MainHeader*>(file.createEmptyMainHeader());
      pE7MainHead->convertFrom(m_pMainHead);
      pE7MainHead->save();
 
      // now we iterate through all possible frames of the source file
      int numFrames = m_pMainHead->num_Frames();
      int destFrame = 1;
      result = 0;
      int frame = 1;
        // load the volume data
        if(loadVolume(frame))
        {
          // prepare a new subheader
           CECAT7SubHeaderScan3D* pE7SubHead = static_cast<CECAT7SubHeaderScan3D*>(file.createEmptySubHeader());
          pE7SubHead->convertFrom(m_pSubHead);

          float*** pInputData = m_pVolume->rawArray();

              C3DArray<float> temp3DArray(1, m_iNum_Z,
                                          1, m_iNum_Y,
                                          1, m_iNum_X);

              for(int z = 1; z<=m_iNum_Z; ++z)
                for(int y=1; y <= m_iNum_Y; ++y)
                  for(int x=1; x <= m_iNum_X; ++x)
                  {
                    temp3DArray[z][y][x]=pInputData[z][y][x];
                  }
      
              for(int j=1; j<m_InfileList.size(); j++)
              {
                m_filename = m_InfileList[j];
                delete m_pMainHead;
                delete m_pInputFile;
                if(openInputFile())
                  if(loadVolume(frame))
                  {
                    float*** pInputData = m_pVolume->rawArray();
                    for(int z = 1; z<=m_iNum_Z; ++z)
                      for(int y=1; y <= m_iNum_Y; ++y)
                        for(int x=1; x <= m_iNum_X; ++x)
                        {
                          temp3DArray[z][y][x]+=pInputData[z][y][x];
                        }
                  }
              }

                C3DArray<qint16> tmpArray(m_iNum_Z - 1, m_iNum_Y - 1, m_iNum_X - 1);
      
              // convert the data for our writeMatrix() method
              for(int z = 1; z<=m_iNum_Z; ++z)
                for(int y=1; y <= m_iNum_Y; ++y)
                  for(int x=1; x <= m_iNum_X; ++x)
                    tmpArray[z - 1][y - 1][x - 1] = static_cast<qint16>(temp3DArray[z][y][x]);

      
              // Now try to save the matrix and the sub header
              if(file.writeMatrix(static_cast<const char*>(static_cast<void*>(tmpArray.rawData())),
                                  m_iRawSize,
                                  *pE7SubHead, 
                                  destFrame++) == false)
              {
                cerr << "ERROR: couldn't write out data of frame #" << frame << endl;
                result = 1;
              }
        }

      file.close();
     }
    else
      cerr << "ERROR: Couldn't open output ECAT file." << endl;
  
    cout << endl <<  "processing finished." << endl;

    m_pInputFile->close();
  }
  else
    cerr << "ERROR: couldn't open input ECAT file." << endl;

  return result;
}

bool CIOTransform::openInputFile()
{
  bool result = false;

  CECATFile* file = new CECATFile(m_filename);
  if(file->open(QIODevice::ReadOnly) &&
     file->format() == CECATFile::ECAT7 &&
     file->subHeaderType() == CECATSubHeader::ECAT7_Scan3D)
  {
    CECATMainHeader* pMainHead = NULL;
    if(file->readMainHeader(pMainHead) &&
       pMainHead->rtti() == CECATMainHeader::ECAT7MainHeader)
    {
      m_pMainHead = static_cast<CECAT7MainHeader*>(pMainHead);
      
      result = true;
      m_pInputFile = file;
    }
    else
    {
      delete pMainHead;
      file->close();
      delete file;
    }
  }
  else
    delete file;

  return result;
}

bool CIOTransform::loadVolume(int frame, int plane, int gate, int bed, int data)
{
  bool result = false; 

  if(m_pInputFile != NULL)
  {
    char *pRawVolume;
    unsigned int iRawSize;
    CECATSubHeader* pSubHead;

    // load the matrix data
    if(m_pInputFile->readMatrix(pRawVolume,
                                iRawSize,
                                pSubHead,
                                frame,
                                plane,
                                gate,
                                bed,
                                data))
    {
      delete m_pSubHead;
      m_pSubHead = static_cast<CECAT7SubHeaderScan3D*>(pSubHead);

      delete [] m_pRawVolume;
      m_pRawVolume = pRawVolume;
      m_iRawSize = iRawSize;

      delete m_pVolume;
       m_iNum_Z=m_pSubHead->num_Z_Elements(0)+m_pSubHead->num_Z_Elements(1)+m_pSubHead->num_Z_Elements(2);
       m_iNum_Y=m_pSubHead->num_Angles();
       m_iNum_X=m_pSubHead->num_R_Elements();

      m_pVolume = new C3DArray<float>(1, m_iNum_Z, 
                                      1, m_iNum_Y, 
                                      1, m_iNum_X);

      // Convert the data in float/scaled
      convertMatrixData(m_pVolume->rawArray(), 
                        m_pRawVolume, 
                        CECATSubHeader::SunShort, 
                        m_iNum_X, 
                        m_iNum_Y, 
                        m_iNum_Z, 
                        m_pSubHead->scale_Factor(), 
                        m_min, m_max);

      result = true;
    } 
  }

  return result;
}

void CIOTransform::convertMatrixData(float*** pDestination, void* pSource, CECATSubHeader::Data_Type dt, int iXDim, int iYDim, int iZDim, float fScaleFactor, float &fMin, float &fMax) const
{
  switch(dt)
  {
    case CECATSubHeader::ByteData:
    {
      unsigned char* pChar = static_cast<unsigned char*>(pSource);
      int iCounter = 0;
      for (int z=1; z <= iZDim; ++z)
        for (int y=1; y <= iYDim; ++y)
          for (int x=1; x <= iXDim; ++x)
          {
            pDestination[z][y][x] = static_cast<float>(pChar[iCounter]) * fScaleFactor;
            fMin = qMin(fMin, pDestination[z][y][x]);
            fMax = qMax(fMax, pDestination[z][y][x]);
            ++iCounter;
          }
    };break;
    case CECATSubHeader::VAX_Ix2:
    case CECATSubHeader::SunShort:
    {
      short int* pShortInt = static_cast<short int*>(pSource);
      int iCounter = 0;
      for (int z=1; z <= iZDim; ++z)
        for (int y=1; y <= iYDim; ++y)
          for (int x=1; x <= iXDim; ++x)
          {
            pDestination[z][y][x] = static_cast<float>(pShortInt[iCounter]) * fScaleFactor;
            fMin = qMin(fMin, pDestination[z][y][x]);
            fMax = qMax(fMax, pDestination[z][y][x]);
            ++iCounter;
          }
    };break;
    case CECATSubHeader::VAX_Ix4:
    case CECATSubHeader::SunLong:
    {
      int* pInt = static_cast<int*>(pSource);
      int iCounter = 0;
      for (int z=1; z <= iZDim; ++z)
        for (int y=1; y <= iYDim; ++y)
          for (int x=1; x <= iXDim; ++x)
          {
            pDestination[z][y][x] = static_cast<float>(pInt[iCounter]) * fScaleFactor;
            fMin = qMin(fMin, pDestination[z][y][x]);
            fMax = qMax(fMax, pDestination[z][y][x]);
            ++iCounter;
          }
    };break;
    case CECATSubHeader::VAX_Rx4:
    case CECATSubHeader::IEEEFloat:
    {
      float* pFloat = static_cast<float*>(pSource);
      int iCounter = 0;
      for (int z=1; z <= iZDim; ++z)
        for (int y=1; y <= iYDim; ++y)
          for (int x=1; x <= iXDim; ++x)
          {
            pDestination[z][y][x] = static_cast<float>(pFloat[iCounter]) * fScaleFactor;
            fMin = qMin(fMin, pDestination[z][y][x]);
            fMax = qMax(fMax, pDestination[z][y][x]);
            ++iCounter;
          }
     }break;
    default:
    {
      cerr << "ERROR: Can not handle data type " << static_cast<int>(dt) << endl;
    }
  }
}


