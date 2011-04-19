#ifndef RINTERFACE_H
#define RINTERFACE_H

#include <Rcpp.h>

// this function can be called from R using the .Call interface
RcppExport SEXP readEcat(SEXP vfile,
                         SEXP numberOfVolumes = NULL,
                         SEXP numberOfRows = NULL,
                         SEXP numberOfCols = NULL,
                         SEXP numberOfPlanes = NULL);

#endif
