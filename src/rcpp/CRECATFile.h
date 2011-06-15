#ifndef CRECATFile_H
#define CRECATFile_H

#include <Rcpp.h>
#include "CECATFile.h"
#include "CECATMainHeader.h"
#include <vector>

class CRECATFile : public CECATFile
{
public:
  CRECATFile(const QString& filename,
             CECATMainHeader::Type fileType = CECATMainHeader::Unknown);

  bool readMainHeader_Rcpp(Rcpp::List& mainHeader);

  bool readSubHeader_Rcpp(Rcpp::List& subHeader,
                          short frame, short plane=1, short gate=1, short bed=0, short data=0);

  bool writeSubHeader_Rcpp(Rcpp::List& subHeader,
                           short frame, short plane=1, short gate=1, short bed=0, short data=0);

  bool writeMainHeader_Rcpp(Rcpp::List& mainHeader);

  CECATMainHeader* createMainHeaderFromRcppMainHeader(Rcpp::List& rMainHeader);
};
#endif
