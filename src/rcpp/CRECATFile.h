#ifndef CRECATFile_H
#define CRECATFile_H

#include <Rcpp.h>

#include <vector>

#include <CECATFile.h>

class CRECATFile : public CECATFile
{
public:
  CRECATFile(const QString& filename,
             CECATMainHeader::Type fileType = CECATMainHeader::Unknown);

  bool readMainHeader(Rcpp::List& mainHeader);

  bool readSubHeader(Rcpp::List& subHeader,
                     short frame, short plane=1, short gate=1, short bed=0, short data=0);

  bool readMatrix(Rcpp::NumericVector*& matrixData, Rcpp::List& subHeader,
                  short frame, short plane=1, short gate=1, short bed=0, short data=0);
};
#endif
