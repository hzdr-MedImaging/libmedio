// vim:set ts=2:set et:nowrap:
//
//
// $Id: CRoi.h 1249 2007-04-13 13:48:43Z poetzsch $
//

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
