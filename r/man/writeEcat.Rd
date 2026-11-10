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
\name{writeEcat}
\alias{writeEcat}
\title{
  Write ECAT7 Image Volumes
}
\description{
  Writes an ECAT structure (as returned by \code{\link{readEcat}}) or a 3D
  array as an ECAT7 image volume file.
}
\usage{
writeEcat(ecat, owrite = FALSE, fname = NULL, dtype = NULL)
}
\arguments{
  \item{ecat}{
    the ECAT structure to write (e.g. returned by \code{\link{readEcat}})
    or a 3D array.
  }
  \item{owrite}{
    if \code{TRUE}, an existing output file is overwritten, otherwise an
    error is reported if the file already exists.
  }
  \item{fname}{
    the name of the output file. If omitted, the file name is derived from
    the name of the variable passed as \code{ecat} (with extension
    \code{.v}) and the file is written to the working directory.
  }
  \item{dtype}{
    by default, the volumes are written with the data type given in their
    sub headers. \code{dtype} forces another data type: \code{"short"}
    (16 bit integer), \code{"int"} (32 bit integer) or \code{"float"} (32
    bit floating point), or the numeric ECAT data type code.
  }
}
\details{
  \code{writeEcat} also accepts plain 3D arrays that are not of class
  \code{"Ecat"}. In this case, default headers are created and the data
  are written as floating point values with a voxel size of 1 x 1 x 1,
  unless the array has a \code{"voxsiz"} attribute with the voxel size in
  the order of the array dimensions.
}
\value{
  The written ECAT structure (invisibly) if the file was written,
  otherwise \code{0}; the reason is printed as error message.
}
\author{
  Jens Maus
}
\seealso{
  \code{\link{readEcat}}
}
\examples{
ecat <- readEcat(system.file("extdata", "pet-static.v", package = "Rmedio"))
out <- tempfile(fileext = ".v")
writeEcat(ecat, fname = out)
dim(readEcat(out)[["f1"]])

# a plain 3D array with a voxel size of 2 x 2 x 3
x <- array(runif(60), dim = c(5, 4, 3))
attr(x, "voxsiz") <- c(2, 2, 3)
writeEcat(x, fname = out, owrite = TRUE)
unlink(out)
}
\keyword{IO}
\keyword{file}
