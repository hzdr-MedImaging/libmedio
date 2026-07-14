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

#include <CECAT7Assemble.h>
#include <CDataArray.h>
#include <CMultiDimArray.h>

#include <CECATFile.h>
#include <CECAT7MainHeader.h>
#include <CECAT7SubHeaderImage.h>

#include <rtdebug.h>

#include <math.h>

#define FOV (0.2425*63)

CECAT7Assemble::CECAT7Assemble()
  : m_pInputECAT7File(NULL),
    m_pInputECAT7Header(NULL),
    m_pOutputECAT7File(NULL),
    m_iFrames(0),
    m_iGates(0),
    m_iBeds(0),
    m_iXDim(0),
    m_iYDim(0),
    m_iZDim(0),
    m_iDataType(0),
    m_fReconZoom(0.0f),
    m_fXPixelSize(0.0f),
    m_fYPixelSize(0.0f),
    m_fZPixelSize(0.0f)
{
  ENTER();
  memset(m_bedPositions, 0, MAX_BEDS*sizeof(float));
  memset(m_overlap, 0, MAX_BEDS*sizeof(float));
  LEAVE();
}

CECAT7Assemble::~CECAT7Assemble()
{
  ENTER();
  if(m_pInputECAT7File != NULL)
  {
    if(m_pInputECAT7File->isOpen())
      m_pInputECAT7File->close();
    delete m_pInputECAT7File;
    m_pInputECAT7File = NULL;
  }

  if(m_pInputECAT7Header != NULL)
  {
    delete m_pInputECAT7Header;
    m_pInputECAT7Header = NULL;
  }

  if(m_pOutputECAT7File != NULL)
  {
    if(m_pOutputECAT7File->isOpen())
      m_pOutputECAT7File->close();
    delete m_pOutputECAT7File;
    m_pOutputECAT7File = NULL;
  }
  LEAVE();
}

void CECAT7Assemble::setInputFile(const QString& sFile)
{
  m_sInputFile = sFile;
}

void CECAT7Assemble::setOutputFile(const QString& sFile)
{
  m_sOutputFile = sFile;
}

bool CECAT7Assemble::assemble()
{
  ENTER();
  bool bResult = true;

  // make sure everything is cleaned up if it is called more than once
  resetValues();

  // open ecat inputfile / make sure it is an ECAT7 file
  m_pInputECAT7File = new CECATFile(m_sInputFile);
  if(m_pInputECAT7File->open(QIODevice::ReadOnly) && m_pInputECAT7File->format() == CECATFile::ECAT7)
  {
    // check if it is an ECAT7 image
    CECATMainHeader::Type type = m_pInputECAT7File->fileType();
    if(type == CECATMainHeader::ECAT7_Volume16 || type == CECATMainHeader::ECAT7_Volume8)
    {
      CECATMainHeader* inputEcatMainHeader = NULL;
      if(m_pInputECAT7File->readMainHeader(inputEcatMainHeader))
      {
        m_pInputECAT7Header = static_cast<CECAT7MainHeader*>(inputEcatMainHeader);
        if(m_pInputECAT7Header)
        {
          m_iFrames = m_pInputECAT7Header->num_Frames();
          m_iGates = m_pInputECAT7Header->num_Gates();
          m_iBeds = m_pInputECAT7Header->num_Bed_Pos();
          D("numBeds: %d, numGates: %d, numFrames: %d", m_iBeds, m_iGates, m_iFrames);
        }
        else
        {
          E("ERROR: no ecat7 header.");
          bResult = false;
        }
      }
      else
      {
        E("ERROR: could not read mainheader of inputfile.");
        bResult = false;
      }
    }
    else
    {
      E("ERROR: inputfile is no ECAT7 image volume.");
      bResult = false;
    }

    if(bResult)
    {
      m_pOutputECAT7File = new CECATFile(m_sOutputFile, CECATMainHeader::ECAT7_Volume16);
      if(m_pOutputECAT7File->open(QIODevice::ReadWrite) && m_pOutputECAT7File->format() != CECATFile::Undefined)
      {
        CECATMainHeader* pECATMainHeader = NULL;
        pECATMainHeader = m_pOutputECAT7File->createEmptyMainHeader();
        if(!pECATMainHeader)
        {
          E("ERROR: could not create new main header in destination file.");
          bResult = false;
        }
        else
        {
          *pECATMainHeader = *m_pInputECAT7Header;
          if(!m_pOutputECAT7File->writeMainHeader(*pECATMainHeader))
          {
            E("ERROR: could not write destination main header.");
            bResult = false;
          }
          delete pECATMainHeader;
          pECATMainHeader = NULL;
        }
      }
      else
      {
        E("ERROR: could not open destination file.");
        bResult = false;
      }
    }

    if(bResult)
    {
      for(int frame = 1; frame <= m_iFrames; frame++)
      {
        for(int gate = 1; gate <= m_iGates; gate++)
        {
          bResult = checkSubHeaders(frame, gate);
          if(!bResult)
            break;

          calcOverlap();

          bResult = assembleImage(frame, gate);
          if(!bResult)
            break;
        }
      }
    }

    // close input file if everything is finished
    m_pOutputECAT7File->close();
    m_pInputECAT7File->close();
  }
  else
  {
    E("ERROR: provided input file is not an ECAT7 file.");
    bResult = false;
  }

  RETURN(bResult);
  return bResult;
}

bool CECAT7Assemble::checkSubHeaders(short iFrame, short iGate)
{
  ENTER();
  bool bResult = true;

  // init values with first bed
  CECATSubHeader* pSubHeader = NULL;
  if(m_pInputECAT7File->readSubHeader(pSubHeader, iFrame, 1, iGate, 0, 0))
  {
    // we already checked if it is an ecat7 image subheader so we just do a cast
    CECAT7SubHeaderImage* pECAT7ImageSubHeader = static_cast<CECAT7SubHeaderImage*>(pSubHeader);
    m_iXDim = pECAT7ImageSubHeader->x_Dimension();
    m_iYDim = pECAT7ImageSubHeader->y_Dimension();
    m_iZDim = pECAT7ImageSubHeader->z_Dimension();
    m_iDataType = pECAT7ImageSubHeader->data_Type();
    m_fReconZoom = pECAT7ImageSubHeader->recon_Zoom();
    m_fXPixelSize = pECAT7ImageSubHeader->x_Pixel_Size();
    m_fYPixelSize = pECAT7ImageSubHeader->y_Pixel_Size();
    m_fZPixelSize = pECAT7ImageSubHeader->z_Pixel_Size();
    delete pSubHeader;
    pSubHeader = NULL;
  }
  else
  {
    E("ERROR: could not read subheader (%d,1,%d,0,0) of inputfile.", iFrame, iGate);
    bResult = false;
  }

  if(bResult)
  {
    // now looping over all other beds and check if dimension, zoom and pixelsize is equal
    for(int i = 1; i < m_iBeds+1; i++)
    {
      if(m_pInputECAT7File->readSubHeader(pSubHeader, iFrame, 1, iGate, i, 0))
      {
        // we already checked if it is an ecat7 image subheader so we just do a cast
        CECAT7SubHeaderImage* pECAT7ImageSubHeader = static_cast<CECAT7SubHeaderImage*>(pSubHeader);
        if(pECAT7ImageSubHeader->x_Dimension() != m_iXDim || pECAT7ImageSubHeader->y_Dimension() != m_iYDim || pECAT7ImageSubHeader->z_Dimension() != m_iZDim)
        {
          E("ERROR: dimensions in subheaders of inputfile differ.");
          bResult = false;
          break;
        }
        if(pECAT7ImageSubHeader->x_Pixel_Size() != m_fXPixelSize || pECAT7ImageSubHeader->y_Pixel_Size() != m_fYPixelSize || pECAT7ImageSubHeader->z_Pixel_Size() != m_fZPixelSize)
        {
          E("ERROR: pixelsizes in subheaders of inputfile differ.");
          bResult = false;
          break;
        }
        if(pECAT7ImageSubHeader->data_Type() != m_iDataType)
        {
          E("ERROR: datatype in subheaders of inputfile differ.");
          bResult = false;
          break;
        }
        if(pECAT7ImageSubHeader->recon_Zoom() != m_fReconZoom)
        {
          E("ERROR: recon zoom in subheaders of inputfile differ.");
          bResult = false;
          break;
        }
        delete pSubHeader;
        pSubHeader = NULL;
      }
      else
      {
        E("ERROR: could not read subheader (%d,1,%d,%d,0) of inputfile.", iFrame, iGate, i);
        bResult = false;
        break;
      }
    }
  }
  RETURN(bResult);
  return bResult;
}

void CECAT7Assemble::resetValues()
{
  m_iFrames = 0;
  m_iGates = 0;
  m_iBeds = 0;
  m_iXDim = 0;
  m_iYDim = 0;
  m_iZDim = 0;
  m_iDataType = 0;
  m_fReconZoom = 0.0f;
  m_fXPixelSize = 0.0f;
  m_fYPixelSize = 0.0f;
  m_fZPixelSize = 0.0f;
  memset(m_bedPositions, 0, MAX_BEDS*sizeof(float));
  memset(m_overlap, 0, MAX_BEDS*sizeof(float));

  if(m_pInputECAT7File != NULL)
  {
    if(m_pInputECAT7File->isOpen())
      m_pInputECAT7File->close();
    delete m_pInputECAT7File;
    m_pInputECAT7File = NULL;
  }

  if(m_pInputECAT7Header != NULL)
  {
    delete m_pInputECAT7Header;
    m_pInputECAT7Header = NULL;
  }

  if(m_pOutputECAT7File != NULL)
  {
    if(m_pOutputECAT7File->isOpen())
      m_pOutputECAT7File->close();
    delete m_pOutputECAT7File;
    m_pOutputECAT7File = NULL;
  }
}

void CECAT7Assemble::calcOverlap()
{
  ENTER();

  if(m_iBeds > 0)
  {
    for(int i = 0; i < m_iBeds; i++)
    {
      m_bedPositions[i] = m_pInputECAT7Header->bed_Offset(i);
      D("BED_POSITION[%d]: %f", i, m_bedPositions[i]);
    }

    if (m_bedPositions[0] >= 0) 
      m_overlap[0]= qRound((FOV - m_bedPositions[0]) / (m_fZPixelSize));
    else 
      m_overlap[0]= qRound((FOV + m_bedPositions[0]) / (m_fZPixelSize));

    D("OVERLAP[%d]: %f", 0, m_overlap[0]);

    for (int i = 1; i < m_iBeds+1; i++)
    {
      if (m_bedPositions[i] - m_bedPositions[i - 1] > 0) 
        m_overlap[i] = qRound((FOV + m_bedPositions[i-1] - m_bedPositions[i]) / (m_fZPixelSize));
      else
        m_overlap[i] = qRound((FOV - m_bedPositions[i-1] + m_bedPositions[i]) / (m_fZPixelSize));
      D("OVERLAP[%d]: %f", i, m_overlap[i]);
    }
  }

  LEAVE();
}

bool CECAT7Assemble::assembleImage(short iFrame, short iGate)
{
  ENTER();
  bool bResult = true;

  C3DArray<float>* pAssembleMatrix = NULL;
  C3DArray<float>* pOutputMatrix = NULL;
  C3DArray<int>* pIndexAndTimeMatrix = NULL;
  CECATSubHeader* pSubHeader = NULL;
  CECAT7SubHeaderImage* pECAT7ImageSubHeader = NULL;
  C3DArray<float>** inputMatrices = new C3DArray<float>*[m_iBeds+1];
  memset(inputMatrices, 0, sizeof(C3DArray<float>*)*(m_iBeds+1));

  float fInitBedPos = m_pInputECAT7Header->init_Bed_Position();
  float bedPosition[MAX_BEDS];
  memset(bedPosition, 0, MAX_BEDS*sizeof(float));
  memcpy(bedPosition+1, m_bedPositions, (MAX_BEDS-1)*sizeof(float));
  for(int i = 0; i < m_iBeds; i++)
    D("BED_POSITION[%d]: %f", i, bedPosition[i]);

  int iMinBedPosition = 0;
  int iMaxBedPosition = 0;
  float fBedPosMin = bedPosition[0];
  float fBedPosMax = bedPosition[0];
  for(int i = 1; i < m_iBeds + 1; i++)
  {
    if(fBedPosMin > bedPosition[i])
    {
      fBedPosMin = bedPosition[i];
      iMinBedPosition = i;
    }

    if(fBedPosMax < bedPosition[i])
    {
      fBedPosMax = bedPosition[i];
      iMaxBedPosition = i;
    }
  }
  D("MinBedPosition: %f", fBedPosMin);
  SHOWVALUE(iMinBedPosition);
  D("MaxBedPosition: %f", fBedPosMax);
  SHOWVALUE(iMaxBedPosition);

  int iOutputMatrixSizeZ = qRound((fabs((fInitBedPos + fBedPosMax)-(fInitBedPos + fBedPosMin)) + FOV) / m_fZPixelSize ) - 1;
  SHOWVALUE(iOutputMatrixSizeZ);

  if(bResult)
  {
    try
    {
      pAssembleMatrix = new C3DArray<float>(iOutputMatrixSizeZ, m_iYDim, m_iXDim);
      pAssembleMatrix->clear();
    }
    catch(std::bad_alloc&)
    {
      E("Error: could not alloc assemble matrix!");
      bResult = false;
    }
  }

  if(bResult)
  {
    try
    {
      pIndexAndTimeMatrix = new C3DArray<int>(m_iZDim,2,m_iBeds+1);
      pIndexAndTimeMatrix->clear();
    }
    catch(std::bad_alloc&)
    {
      E("Error: could not alloc assemble matrix!");
      bResult = false;
    }
  }

  if(bResult)
  {
    if((bResult=m_pInputECAT7File->readSubHeader(pSubHeader, iFrame, 1, iGate, 0, 0)) != true)
    {
      E("Error: could not read sub header of inputfile!");
      bResult = false;
    }
  }

  if(bResult)
  {
    pECAT7ImageSubHeader = static_cast<CECAT7SubHeaderImage*>(pSubHeader);
    if(pECAT7ImageSubHeader == NULL)
    {
      E("Error: static cast from CECATSubHeader to CECAT7SubHeaderImage failed!");
      bResult = false;
    }
  }

  if(bResult)
  {
    CECAT7MainHeader::Patient_Orientation iOrientation = m_pInputECAT7Header->patient_Orientation();
    unsigned int iFrameDuration = pECAT7ImageSubHeader->frame_Duration();
    int iAmplify = 1;

    for (int i = 0; i < m_iBeds+1; i++)
    {
      unsigned int iStartPlane = 0;
      if (iOrientation == CECAT7MainHeader::HF_Supine || iOrientation == CECAT7MainHeader::HF_Prone)
        iStartPlane = qRound((bedPosition[i] - fBedPosMin) / (m_fZPixelSize));
      else if (iOrientation == CECAT7MainHeader::FF_Supine || iOrientation == CECAT7MainHeader::FF_Prone)
        iStartPlane = qRound((fBedPosMax - bedPosition[i]) / (m_fZPixelSize));
      SHOWVALUE(iStartPlane);
      int iOverlap = m_overlap[i];
      int iCutValue = 2; // skip first and last two planes from overlap area

      for (int iIndex = 0; iIndex < m_iZDim; iIndex++)
      {
        (*pIndexAndTimeMatrix)[iIndex][0][i] = iStartPlane + iIndex;
        if(i == 0)
        {
          if(iIndex < iOverlap-iCutValue && iIndex >= iCutValue)
          {
            if (iOrientation == CECAT7MainHeader::HF_Supine || iOrientation == CECAT7MainHeader::HF_Prone)
              (*pIndexAndTimeMatrix)[iIndex][1][i] = iFrameDuration;
            else
            {
              (*pIndexAndTimeMatrix)[iIndex][1][i] = iAmplify*(iFrameDuration * (iIndex-iCutValue))/(iOverlap-(iCutValue*2)-1);
              D("Plane: %d, Factor: %d", iIndex, (*pIndexAndTimeMatrix)[iIndex][1][i]);
            }
          }
          else if(iIndex > (m_iZDim-iOverlap-1+iCutValue) && iIndex <= m_iZDim-iCutValue-1)
          {
            if (iOrientation == CECAT7MainHeader::HF_Supine || iOrientation == CECAT7MainHeader::HF_Prone)
              (*pIndexAndTimeMatrix)[iIndex][1][i] = iAmplify*(iFrameDuration * (m_iZDim - iIndex - iCutValue - 1))/(iOverlap-(iCutValue*2)-1);
            else
              (*pIndexAndTimeMatrix)[iIndex][1][i] = iFrameDuration;
          }
          else
            (*pIndexAndTimeMatrix)[iIndex][1][i] = iFrameDuration;
        }
        else if(i == m_iBeds)
        {
          if(iIndex < iOverlap-iCutValue && iIndex >= iCutValue)
          {
            if (iOrientation == CECAT7MainHeader::HF_Supine || iOrientation == CECAT7MainHeader::HF_Prone)
              (*pIndexAndTimeMatrix)[iIndex][1][i] = iAmplify*(iFrameDuration * (iIndex-iCutValue))/(iOverlap-(iCutValue*2)-1);
            else
              (*pIndexAndTimeMatrix)[iIndex][1][i] = iFrameDuration;
          }
          else if(iIndex > (m_iZDim-iOverlap-1+iCutValue) && iIndex <= m_iZDim-iCutValue-1)
          {
            if (iOrientation == CECAT7MainHeader::HF_Supine || iOrientation == CECAT7MainHeader::HF_Prone)
              (*pIndexAndTimeMatrix)[iIndex][1][i] = iFrameDuration;
            else
            {
              (*pIndexAndTimeMatrix)[iIndex][1][i] = iAmplify*(iFrameDuration * (m_iZDim - iIndex - iCutValue - 1))/(iOverlap-(iCutValue*2)-1);
              D("Plane: %d, Factor: %d", iIndex, (*pIndexAndTimeMatrix)[iIndex][1][i]);
            }
          }
          else
            (*pIndexAndTimeMatrix)[iIndex][1][i] = iFrameDuration;
        }
        else
        {
          if(iIndex < iOverlap-iCutValue && iIndex >= iCutValue)
            (*pIndexAndTimeMatrix)[iIndex][1][i] = iAmplify*(iFrameDuration * (iIndex-iCutValue))/(iOverlap-(iCutValue*2)-1);
          else if(iIndex > (m_iZDim-iOverlap-1+iCutValue) && iIndex <= m_iZDim-iCutValue-1)
            (*pIndexAndTimeMatrix)[iIndex][1][i] = iAmplify*(iFrameDuration * (m_iZDim - iIndex - iCutValue - 1))/(iOverlap-(iCutValue*2)-1);
          else
            (*pIndexAndTimeMatrix)[iIndex][1][i] = iFrameDuration;
        }
      }

      // cut the first and last two planes
      (*pIndexAndTimeMatrix)[0][0][i] = -1;
      (*pIndexAndTimeMatrix)[1][0][i] = -1;
      (*pIndexAndTimeMatrix)[m_iZDim - 2][0][i] = -1;
      (*pIndexAndTimeMatrix)[m_iZDim - 1][0][i] = -1;
    }

    // append first or last two planes of first and last bedposition depending
    // on patient orientation
    if(iOrientation == CECAT7MainHeader::HF_Supine || iOrientation == CECAT7MainHeader::HF_Prone)
    {
      (*pIndexAndTimeMatrix)[0][0][iMinBedPosition] = 0;
      (*pIndexAndTimeMatrix)[1][0][iMinBedPosition] = 1;
      (*pIndexAndTimeMatrix)[m_iZDim - 2][0][iMaxBedPosition] = iOutputMatrixSizeZ - 2;
      (*pIndexAndTimeMatrix)[m_iZDim - 1][0][iMaxBedPosition] = iOutputMatrixSizeZ - 1;
    }
    else if(iOrientation == CECAT7MainHeader::FF_Supine || iOrientation == CECAT7MainHeader::FF_Prone)
    {
      (*pIndexAndTimeMatrix)[0][0][iMaxBedPosition] = 0;
      (*pIndexAndTimeMatrix)[1][0][iMaxBedPosition] = 1;
      (*pIndexAndTimeMatrix)[m_iZDim - 2][0][iMinBedPosition] = iOutputMatrixSizeZ - 2;
      (*pIndexAndTimeMatrix)[m_iZDim - 1][0][iMinBedPosition] = iOutputMatrixSizeZ - 1;
    }
  }

  if(bResult)
  {
    for(int i = 0; i < m_iBeds+1; i++)
    {
      bResult = loadMatrix(inputMatrices[i], iFrame, 1, iGate, i, 0);
      if(!bResult)
        break;
    }
  }

  if(bResult)
  {
    C3DArray<float>* pInputMatrix = NULL;
    for(int iIndex = 0; iIndex < iOutputMatrixSizeZ; iIndex++)
    {
      int iWholeFrameDuration = 0;
      for(int iIndexBedNum = 0; iIndexBedNum < m_iBeds+1; iIndexBedNum++)
      {
        pInputMatrix = inputMatrices[iIndexBedNum];

        for(int iIndexPlane = 0; iIndexPlane < m_iZDim; iIndexPlane++)
        {
          int valuePlane = (*pIndexAndTimeMatrix)[iIndexPlane][0][iIndexBedNum];
          if(valuePlane == iIndex)
          {
            iWholeFrameDuration += (*pIndexAndTimeMatrix)[iIndexPlane][1][iIndexBedNum];
            D("ZDim: %d, FrameDuration: %d, Eff. Plane: %d", iIndexPlane, (*pIndexAndTimeMatrix)[iIndexPlane][1][iIndexBedNum], valuePlane);
            for(int y = 0; y < m_iYDim; y++)
            {
              for (int x = 0; x < m_iXDim; x++)
              {
               float valueMatrix =
                 (*pInputMatrix)[iIndexPlane][y][x] *
                 (*pIndexAndTimeMatrix)[iIndexPlane][1][iIndexBedNum] +
                 (*pAssembleMatrix)[iIndex][y][x];
               (*pAssembleMatrix)[iIndex][y][x] = valueMatrix;
              }
            }
          }
        }
      }
      SHOWVALUE(iWholeFrameDuration);
      SHOWVALUE(iOutputMatrixSizeZ);

      if(iWholeFrameDuration != 0)
      {
        for (int y = 0; y < m_iYDim; y++)
        {
          for (int x = 0; x < m_iXDim; x++)
          {
            float value = (*pAssembleMatrix)[iIndex][y][x] / iWholeFrameDuration;
            (*pAssembleMatrix)[iIndex][y][x] = value;
          }
        }
      }
    }
  }

  if(bResult)
  {
    float fNewPixelSize = m_fXPixelSize;
    int iNewZDim = qRound(iOutputMatrixSizeZ * m_fZPixelSize / fNewPixelSize);
    unifyPixelSize(pOutputMatrix, pAssembleMatrix, m_fZPixelSize, fNewPixelSize);

    CECATSubHeader* pSubHeader = NULL;
    CECATSubHeader* pDestSubHeader = NULL;
    QByteArray* pImageData = NULL;
    if(m_pInputECAT7File->readSubHeader(pSubHeader, iFrame, 1, iGate, 0, 0))
    {
      pDestSubHeader = m_pOutputECAT7File->createEmptySubHeader();
      *pDestSubHeader = *pSubHeader;
      // we already checked if it is an ecat7 image subheader so we just do a cast
      CECAT7SubHeaderImage* pECAT7ImageSubHeader = static_cast<CECAT7SubHeaderImage*>(pSubHeader);
      pECAT7ImageSubHeader->setZ_Dimension(iNewZDim);
      pECAT7ImageSubHeader->setZ_Pixel_Size(fNewPixelSize);

      int iNumElements = m_iXDim * m_iYDim * iNewZDim;
      CDataArray<float> dataArray(iNumElements);
      int iArrayIdx = 0;
      for (int iZIdx = 0; iZIdx < iNewZDim; iZIdx++)
      {
        for (int iYIdx = 0; iYIdx < m_iYDim; iYIdx++)
        {
          for (int iXIdx = 0; iXIdx < m_iXDim; iXIdx++, iArrayIdx++)
          {
            dataArray[iArrayIdx] = (*pOutputMatrix)[iZIdx][iYIdx][iXIdx];
          }
        }
      }

      float fScaleFactor = ((float)dataArray.valueRange())/32767.0;
      float fMaxValue = dataArray.maxValue();
      float fMinValue = dataArray.minValue();
      if((bResult = dataArray.shortByteArray(pImageData)) != true)
      {
        E("Error: could not convert float array to short!");
        bResult = false;
      }

      if(bResult)
      {
        pECAT7ImageSubHeader->setScale_Factor(fScaleFactor);
        if(fabs(fMaxValue) > fabs(fMinValue))
        {
          pECAT7ImageSubHeader->setImage_Min((short)floor(fMinValue*fScaleFactor));
          pECAT7ImageSubHeader->setImage_Max(32767);
        }
        else
        {
          pECAT7ImageSubHeader->setImage_Max((short)ceil(fMaxValue*fScaleFactor));
          pECAT7ImageSubHeader->setImage_Min(-32768);
        }
        if((bResult = m_pOutputECAT7File->writeSubHeader(*pECAT7ImageSubHeader, iFrame, 1, iGate, 0, 0)) != true)
        {
          E("Error: can not write subheader to output file!");
          bResult = false;
        }

        if(bResult)
        {
          if((bResult = m_pOutputECAT7File->writeMatrix(*pImageData, iFrame, 1, iGate, 0, 0)) != true)
          {
            E("Error: can not write matrix data to output file!");
            bResult = false;
          }
        }
      }
      
      if(pImageData)
      {
        delete pImageData;
        pImageData = NULL;
      }
      if(pSubHeader)
      {
        delete pSubHeader;
        pSubHeader = NULL;
      }
      if(pDestSubHeader)
      {
        delete pDestSubHeader;
        pDestSubHeader = NULL;
      }
    }

    if(bResult)
    {
      CECATMainHeader* pECATMainHeader = NULL;
      if(m_pOutputECAT7File->readMainHeader(pECATMainHeader))
      {
        CECAT7MainHeader* pECAT7MainHeader = static_cast<CECAT7MainHeader*>(pECATMainHeader);
        if(pECAT7MainHeader)
        {
          pECAT7MainHeader->setNum_Planes(iNewZDim);
          m_pOutputECAT7File->writeMainHeader(*pECATMainHeader);
        }

        if(pECATMainHeader)
        {
          delete pECATMainHeader;
          pECATMainHeader = NULL;
        }
      }
    }
  }

  if(pAssembleMatrix)
  {
    delete pAssembleMatrix;
    pAssembleMatrix = NULL;
  }
  if(pOutputMatrix)
  {
    delete pOutputMatrix;
    pOutputMatrix = NULL;
  }
  if(pIndexAndTimeMatrix)
  {
    delete pIndexAndTimeMatrix;
    pIndexAndTimeMatrix = NULL;
  }
  for(int i = 0; i < m_iBeds + 1; i++)
  {
    C3DArray<float>* pTmp = inputMatrices[i];
    if(pTmp)
    {
      delete pTmp;
      inputMatrices[i] = NULL;
    }
  }
  if(inputMatrices)
  {
    delete [] inputMatrices;
    inputMatrices = NULL;
  }
  if(pSubHeader)
  {
    delete pSubHeader;
    pSubHeader = NULL;
  }


  RETURN(bResult);
  return bResult;
}

bool CECAT7Assemble::loadMatrix(C3DArray<float>*& pMatrix, int iFrame, int iPlane, int iGate, int iBed, int iData)
{
  ENTER();
  bool bResult = true;
  CECATSubHeader* pSubHeader = NULL;
  CECAT7SubHeaderImage* pECAT7ImageSubHeader = NULL;
  QByteArray* pRawData = NULL;

  if(bResult)
  {
    if((bResult=m_pInputECAT7File->readSubHeader(pSubHeader, iFrame, iPlane, iGate, iBed, iData)) != true)
    {
      E("Error: could not read sub header of inputfile!");
      bResult = false;
    }
  }

  if(bResult)
  {
    pECAT7ImageSubHeader = static_cast<CECAT7SubHeaderImage*>(pSubHeader);
    if(pECAT7ImageSubHeader == NULL)
    {
      E("Error: static cast from CECATSubHeader to CECAT7SubHeaderImage failed!");
      bResult = false;
    }
  }

  if(bResult)
  {
    if((bResult = m_pInputECAT7File->readMatrix(pRawData, iFrame, iPlane, iGate, iBed, iData)) != true)
    {
      E("Error: Could not read data");
      bResult = false;
    }
  }

  if(bResult)
  {
    try
    {
      pMatrix = new C3DArray<float>(m_iZDim, m_iYDim, m_iXDim);
    }
    catch(std::bad_alloc&)
    {
      E("Error: could not alloc matrix!");
      bResult = false;
    }
  }

  float fScaleFactor = pECAT7ImageSubHeader->scale_Factor();
  unsigned int iNumElements = m_iXDim*m_iYDim*m_iZDim;

  if(bResult)
  {
    switch(m_iDataType)
    {
      case CECATSubHeader::VAX_Ix2:
      case CECATSubHeader::SunShort:
      {
        int iArrayIdx = 0;
        CDataArray<short> dataArray(pRawData, iNumElements);
        for (int iZIdx = 0; iZIdx < m_iZDim; iZIdx++)
        {
          for (int iYIdx = 0; iYIdx < m_iYDim; iYIdx++)
          {
            for (int iXIdx = 0; iXIdx < m_iXDim; iXIdx++, iArrayIdx++)
            {
              (*pMatrix)[iZIdx][iYIdx][iXIdx] = fScaleFactor * (float)dataArray[iArrayIdx];
            }
          }
        }
      }
      break;
      case CECATSubHeader::VAX_Rx4:
      case CECATSubHeader::IEEEFloat:
      {
        int iArrayIdx = 0;
        CDataArray<float> dataArray(pRawData, iNumElements);
        for (int iZIdx = 0; iZIdx < m_iZDim; iZIdx++)
        {
          for (int iYIdx = 0; iYIdx < m_iYDim; iYIdx++)
          {
            for (int iXIdx = 0; iXIdx < m_iXDim; iXIdx++, iArrayIdx++)
            {
              (*pMatrix)[iZIdx][iYIdx][iXIdx] = fScaleFactor * dataArray[iArrayIdx];
            }
          }
        }
      }
      break;
      default:
      {
        E("Error: datatype not supported!");
        bResult = false;
      }
    }
  }

  delete pRawData;
  delete pSubHeader;

  RETURN(bResult);
  return bResult;
}

bool CECAT7Assemble::unifyPixelSize(C3DArray<float>*& pOutMatrix, C3DArray<float>* pInMatrix, float fOldZPixelSize, float fNewZPixelSize)
{
  ENTER();
  bool bResult = true;

  int iOutputMatrixSizeZ = pInMatrix->firstLength()-1;
  SHOWVALUE(iOutputMatrixSizeZ);
  int iNewZDim = qRound(iOutputMatrixSizeZ * fOldZPixelSize / fNewZPixelSize);
  SHOWVALUE(iNewZDim);

  try
  {
    pOutMatrix  = new C3DArray<float>(iNewZDim, m_iYDim, m_iXDim);
  }
  catch(std::bad_alloc&)
  {
    E("Error: could not alloc matrix!");
    bResult = false;
  }

  if(bResult)
  {
    for(int z = 0; z < iNewZDim; z++)
    {
      for(int y = 0; y < m_iYDim; y++)
      {
        for(int x = 0; x < m_iXDim; x++)
        {
          // round towards zero
          int p1 = (int)((z - 0.5) * fNewZPixelSize / fOldZPixelSize);
          // round towards infinity
          int p2 = (int)((z - 0.5) * fNewZPixelSize / fOldZPixelSize + 1);

          if (p1 < 1)
          {
            (*pOutMatrix)[z][y][x] = (*pInMatrix)[p2][y][x];
          }
          else
          {
            float m = (*pInMatrix)[p2][y][x] - (*pInMatrix)[p1][y][x];
            float n = (*pInMatrix)[p1][y][x] - m * p1;
            float value = m * ((z - 0.5) * fNewZPixelSize / fOldZPixelSize) + n;
            (*pOutMatrix)[z][y][x] = value;
          }
        }
      }
    }
  }
  RETURN(bResult);
  return bResult;
}
