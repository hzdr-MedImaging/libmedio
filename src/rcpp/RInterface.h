#ifndef RINTERFACE_H
#define RINTERFACE_H

#include <Rcpp.h>
#include "CECATMainHeader.h"

class CRECATFile;

typedef enum
{
  FRAMES = 'f',
  GATES = 'g',
  BEDS = 'b'
} as_volume;

// this function can be called from R using the .Call interface
RcppExport SEXP readEcat(SEXP vfile,
                         SEXP numberOfVolumes = NULL,
                         SEXP numberOfRows = NULL,
                         SEXP numberOfCols = NULL,
                         SEXP numberOfPlanes = NULL);

RcppExport SEXP writeEcat(SEXP ecat, SEXP vfilename);

#endif
