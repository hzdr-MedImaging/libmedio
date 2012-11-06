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
    CRECATFile(CECATMainHeader::Type fileType = CECATMainHeader::Unknown);

    // read methods
    bool readMainHeader(Rcpp::List& mainHeader);
    bool readSubHeader(Rcpp::List& subHeader,
                       short frame, short plane=1, short gate=1, short bed=0, short data=0);

    // write methods
    bool writeMainHeader(Rcpp::List& mainHeader);
    bool writeSubHeader(Rcpp::List& subHeader,
                        short frame, short plane=1, short gate=1, short bed=0, short data=0);

    CECATMainHeader* createMainHeader(Rcpp::List& rMainHeader);
    Rcpp::List createEmptyMainHeader(void);
    Rcpp::List createEmptySubHeader(void);
};
#endif
