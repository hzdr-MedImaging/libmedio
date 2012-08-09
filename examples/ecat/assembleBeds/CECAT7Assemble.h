#ifndef CECAT7ASSEMBLE_H
#define CECAT7ASSEMBLE_H

#include <QString>

#define FOV 0.2425*63
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
    bool checkSubHeaders(short iGate);
    void resetValues();
    void calcOverlap();
    bool assembleImage(short iGate);
    bool loadMatrix(C3DArray<float>*& pMatrix, int iFrame, int iPlane, int iGate, int iBed, int iData);
    bool unifyPixelSize(C3DArray<float>*& pOutMatrix, C3DArray<float>* pInMatrix, float fOldZPixelSize, float fNewZPixelSize);

    // members
    QString           m_sInputFile;
    QString           m_sOutputFile;

    CECATFile*        m_pInputECAT7File;
    CECAT7MainHeader* m_pInputECAT7Header;
    CECATFile*        m_pOutputECAT7File;
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
