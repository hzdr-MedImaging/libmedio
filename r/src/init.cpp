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

// registration of the native routines called via .Call() from R

// readEcat.h/writeEcat.h include Rcpp.h, which has to come before the R headers
#include "readEcat.h"
#include "writeEcat.h"

#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>

static const R_CallMethodDef callMethods[] =
{
  { "readEcat",  (DL_FUNC)&readEcat,  5 },
  { "writeEcat", (DL_FUNC)&writeEcat, 4 },
  { NULL, NULL, 0 }
};

extern "C" attribute_visible void R_init_Rmedio(DllInfo* dll)
{
  R_registerRoutines(dll, NULL, callMethods, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
  R_forceSymbols(dll, TRUE);
}
