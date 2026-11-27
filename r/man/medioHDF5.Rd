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
\name{medioHDF5}
\alias{medioHDF5}
\title{
  HDF5 Support of the Package
}
\description{
  Tells whether the package was built with HDF5 support, which is needed
  to read and write images stored in HDF5 files.
}
\usage{
medioHDF5()
}
\details{
  HDF5 support is optional, as it needs the HDF5 library when the package
  is installed (see the file \file{README.md} of the package sources).
  Without it, HDF5 files are recognized, but \code{\link{readMedio}} and
  \code{\link{writeMedio}} fail for them with an error saying so.
}
\value{
  The version of the HDF5 library as character string, e.g.
  \code{"1.10.10"}, or \code{NA} if the package has no HDF5 support.
}
\seealso{
  \code{\link{readMedio}}, \code{\link{writeMedio}}
}
\examples{
if (!is.na(medioHDF5())) {
  source <- system.file("extdata", "pet-static.v", package = "Rmedio")
  target <- file.path(tempdir(), "pet-static.h5")
  writeMedio(readMedio(source), target, like = source, overwrite = TRUE)
  image <- readMedio(target)
  dim(image$data)
  unlink(target)
}
}
\keyword{IO}
\keyword{file}
