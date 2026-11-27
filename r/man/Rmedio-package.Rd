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
  (\url{https://github.com/hzdr-MedImaging/libmedio}): images of the
  formats used in nuclear medicine, ECAT7, NIfTI, Philips PET, Interfile,
  (read only) Siemens Concorde microPET and, if the package was built with
  the HDF5 library (\code{\link{medioHDF5}}), HDF5.
}
\details{
  \code{\link{readMedio}} reads all frames of an image file of any format
  as one 4D array with its geometry, frame timing and header attributes
  (\code{\link{medioImage}}). \code{\link{writeMedio}} writes such an
  image, or a plain array, into an ECAT7, NIfTI, Philips, Interfile or
  HDF5 file.

  \code{\link{readEcat}} and \code{\link{writeEcat}} are the older
  interface for ECAT files only, kept for existing scripts: they read an
  ECAT file into a list of 3D arrays (rows x columns x planes) with the
  native ECAT headers attached as attributes and write such a structure
  as ECAT7 file.

  The 'libmedio' sources are bundled with the package and compiled during
  its installation, which requires Qt5 (Core module) and zlib.
}
\author{
  Jens Maus <mail@jens-maus.de>
}
\seealso{
  \code{\link{readMedio}}, \code{\link{writeMedio}},
  \code{\link{medioImage}}, \code{\link{readEcat}}, \code{\link{writeEcat}}
}
\keyword{package}
