#ifndef WRITEECAT_H
#define WRITEECAT_H

#include <Rcpp.h>

// this function can be called from R using the .Call interface
// and will return write the volume data supplied as 'ecat' to
// a file with the name 'vfilename'.
RcppExport SEXP writeEcat(SEXP ecat,
                          SEXP vfilename = NULL,
                          SEXP overwrite = NULL,
                          SEXP dtype = NULL);

#endif
