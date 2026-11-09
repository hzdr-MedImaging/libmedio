#ifndef READECAT_H
#define READECAT_H

#include <Rcpp.h>

// this function can be called from R using the .Call interface
// and will return a structure with the volume data
RcppExport SEXP readEcat(SEXP vfile,
                         SEXP numberOfVolumes = NULL,
                         SEXP numberOfRows = NULL,
                         SEXP numberOfCols = NULL,
                         SEXP numberOfPlanes = NULL);
#endif
