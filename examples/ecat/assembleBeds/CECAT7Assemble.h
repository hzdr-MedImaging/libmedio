#ifndef CECAT7ASSEMBLE_H
#define CECAT7ASSEMBLE_H

#include <QString>

#define FOV 0.2425*63
#define MAX_BEDS 15

#define FLOAT_2_SHORT(X) ((short int)(((X)>=0)?((X)+0.5):((X)-0.5)))           ///< float to signed short
#define FLOAT_2_USHORT(X) ((unsigned short int)(((X)>=0)?((X)+0.5):((X)-0.5))) ///< float to unsigned short
#define FLOAT_2_INT(X)   ((int)(((X)>=0)?((X)+0.5):((X)-0.5)))                 ///< float to signed int
#define FLOAT_2_UINT(X)   ((unsigned int)(((X)>=0)?((X)+0.5):((X)-0.5)))       ///< float to unsigned int
#define FLOAT_2_LONG(X)  ((long)(((X)>=0)?((X)+0.5):((X)-0.5)))                ///< float to singed long
#define FLOAT_2_ULONG(X)  ((unsigned long)(((X)>=0)?((X)+0.5):((X)-0.5)))      ///< float to unsigned long

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
