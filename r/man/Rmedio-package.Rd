%
% libmedio - C++ I/O Library for loading/saving medical data formats
%            https://github.com/hzdr-MedImaging/libmedio
%
% Copyright (C) 2004-2026 hzdr.de and contributors
%
% Licensed under the Apache License, Version 2.0 (the "License");
% you may not use this file except in compliance with the License.
% You may obtain a copy of the License at
%
%   http://www.apache.org/licenses/LICENSE-2.0
%
% Unless required by applicable law or agreed to in writing, software
% distributed under the License is distributed on an "AS IS" BASIS,
% WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
% See the License for the specific language governing permissions and
% limitations under the License.
\name{Rmedio-package}
\alias{Rmedio-package}
\alias{Rmedio}
\docType{package}
\title{
  R Interface to libmedio
}
\description{
  Reads and writes medical imaging data with the 'libmedio' C++ library
  (\url{https://github.com/hzdr-MedImaging/libmedio}). While 'libmedio'
  supports several formats used in nuclear medicine (ECAT, NIfTI, Philips
  PET, Siemens Concorde microPET), the package currently only reads and
  writes ECAT image volumes.
}
\details{
  \code{\link{readEcat}} reads an ECAT file into a list of 3D arrays with
  all meta data attached as attributes. \code{\link{writeEcat}} writes
  such a structure (or a plain 3D array) as an ECAT7 file.

  The 'libmedio' sources are bundled with the package and compiled during
  its installation, which requires Qt5 (Core module) and zlib.
}
\author{
  Jens Maus <mail@jens-maus.de>
}
\seealso{
  \code{\link{readEcat}}, \code{\link{writeEcat}}
}
\keyword{package}
