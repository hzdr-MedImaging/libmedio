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

class CRInterfacePrivate;

class CRInterface : public CECATFile
{
public:
  CRInterface(const QString& filename,
             CECATMainHeader::Type fileType = CECATMainHeader::Unknown);

  ~CRInterface();

  operator SEXP();

  bool readMainHeader(Rcpp::List& mainHeader);
  bool readMatrix(Rcpp::NumericVector*& matrixData, Rcpp::List& subHeader,
                  short frame, short plane=1, short gate=1, short bed=0, short data=0);

  Rcpp::List asRList(std::vector<short>& volumes,
                     std::vector<short>& rows,
                     std::vector<short>& cols,
                     std::vector<short>& planes);

private:
  CRInterfacePrivate *m_pData;
};
#endif
