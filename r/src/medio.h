/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio

  Copyright (C) 2004-2026 hzdr.de and contributors

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef MEDIO_H
#define MEDIO_H

#include <Rcpp.h>

// format independent access to images of all formats of libmedio (readMedio(),
// writeMedio(), see R/medio.R); called from R using the .Call interface

// all frames of an ECAT7, NIfTI, Philips, Interfile, microPET or HDF5 file: list with the physical
// values (4D array x, y, z, frame), affine, frame timing, header attributes,
// format and file name
RcppExport SEXP readMedio(SEXP fileName, SEXP frames, SEXP orientation);

// writes the frames of an image into an ECAT7, NIfTI, Philips, Interfile or HDF5 file; the
// arguments are checked by writeMedio() in R
RcppExport SEXP writeMedio(SEXP fileName, SEXP data, SEXP affine, SEXP frameStart, SEXP frameDuration,
                           SEXP attributes, SEXP like, SEXP format, SEXP overwrite);

// version of the HDF5 library Rmedio was built with, NA without HDF5 support
RcppExport SEXP medioHDF5();

#endif
