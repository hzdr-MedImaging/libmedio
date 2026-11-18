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
\name{writeMedio}
\alias{writeMedio}
\title{
  Write Images of All Formats
}
\description{
  Writes an image (as returned by \code{\link{readMedio}} or made with
  \code{\link{medioImage}}) or a 3D/4D array into an ECAT7, NIfTI or
  Philips PET image file.
}
\usage{
writeMedio(image, fname, like = NULL, format = NULL, overwrite = FALSE)
}
\arguments{
  \item{image}{
    a \code{"medioImage"} or a numeric 3D array (x, y, z) or 4D array
    (x, y, z, frame), which is written with an identity affine.
  }
  \item{fname}{
    path of the file to write. The format is taken from its extension:
    \code{.v} ECAT7, \code{.nii}, \code{.nii.gz} or \code{.hdr} NIfTI
    (\code{.hdr} with the image file \code{.img}), \code{.img} or
    \code{.syn} Philips.
  }
  \item{like}{
    path of a source file of any format whose main header is converted
    into the new file, e.g. to keep its patient and acquisition data. A
    NIfTI file made from a source file needs all of its frames.
    \code{NULL}: the main header is made from the image.
  }
  \item{format}{
    \code{"ecat"}, \code{"nifti"} or \code{"philips"} to give the format
    independent of the file name.
  }
  \item{overwrite}{
    if \code{TRUE}, an existing file is replaced, otherwise it is an error.
  }
}
\details{
  The header attributes of the image (e.g. \code{PatientWeight}) are set
  in the main header; invalid values are an error. Double data is stored
  as 32 bit floating point values, integer data as 32 bit integers, every
  frame with scale factor 1. Philips files store no position within the
  planes: the planes are centered.

  All arguments are checked before the file is created, and the file is
  removed again if writing fails, so no partial file is left behind.
}
\value{
  The absolute path of the written file (invisibly).
}
\author{
  Jens Maus
}
\seealso{
  \code{\link{readMedio}}, \code{\link{medioImage}}
}
\examples{
image <- readMedio(system.file("extdata", "pet-dynamic.v", package = "Rmedio"))
out <- tempfile(fileext = ".nii.gz")
image$attributes <- list(PatientWeight = 75)
writeMedio(image, out, like = system.file("extdata", "pet-dynamic.v", package = "Rmedio"))
readMedio(out)$attributes$PatientWeight

# a plain array: identity affine (1 mm voxels), no timing
x <- array(runif(60), dim = c(5, 4, 3))
writeMedio(x, out, overwrite = TRUE)
unlink(out)
}
\keyword{IO}
\keyword{file}
