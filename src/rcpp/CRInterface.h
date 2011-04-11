#ifndef RMDEDIO_H
#define RMDEDIO_H

#include <Rcpp.h>

#include <vector>

#include <CECATFile.h>

// this function can be called from R using the .Call interface
RcppExport SEXP readEcat(SEXP vfile,
                         SEXP numberOfVolumes = NULL,
                         SEXP numberOfRows = NULL,
                         SEXP numberOfCols = NULL,
                         SEXP numberOfPlanes = NULL);

class CRInterface : public CECATFile
{
public:
  CRInterface(const QString& filename,
             CECATMainHeader::Type fileType = CECATMainHeader::Unknown);

  operator SEXP();

  bool readMainHeader(Rcpp::List& mainHeader);
  bool readMatrix(Rcpp::NumericVector*& matrixData, Rcpp::List& subHeader,
                  short frame, short plane=1, short gate=1, short bed=0, short data=0);
};
#endif
