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

#ifndef CECAT7ASSEMBLE_H
#define CECAT7ASSEMBLE_H

#include <QString>

#define MAX_BEDS 15

// forward declarations
class CECATFile;
class CECAT7MainHeader;
template<class T> class C3DArray;

class CECAT7Assemble
{
  public:
    // constructors
    CECAT7Assemble();
    ~CECAT7Assemble();

    // copy constructur and default assignment operator
    CECAT7Assemble(const CECAT7Assemble& src);
    CECAT7Assemble& operator=(const CECAT7Assemble& src);

    // set methods
    void setInputFile(const QString& sFile);
    void setOutputFile(const QString& sFile);

    // processing methods
    bool assemble();

  private:
    // methods
    bool checkSubHeaders(short iFrame, short iGate);
    void resetValues();
    void calcOverlap();
    bool assembleImage(short iFrame, short iGate);
    bool loadMatrix(C3DArray<float>*& pMatrix, int iFrame, int iPlane, int iGate, int iBed, int iData);
    bool unifyPixelSize(C3DArray<float>*& pOutMatrix, C3DArray<float>* pInMatrix, float fOldZPixelSize, float fNewZPixelSize);

    // members
    QString           m_sInputFile;
    QString           m_sOutputFile;

    CECATFile*        m_pInputECAT7File;
    CECAT7MainHeader* m_pInputECAT7Header;
    CECATFile*        m_pOutputECAT7File;
    short             m_iFrames;
    short             m_iGates;
    short             m_iBeds;
    short             m_iXDim;
    short             m_iYDim;
    short             m_iZDim;
    short             m_iDataType;
    float             m_fReconZoom;
    float             m_fXPixelSize;
    float             m_fYPixelSize;
    float             m_fZPixelSize;
    float             m_bedPositions[MAX_BEDS];
    float             m_overlap[MAX_BEDS];
};

#endif // CECAT7ASSEMBLE_H //
