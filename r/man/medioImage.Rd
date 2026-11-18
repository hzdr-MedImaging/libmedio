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
\name{medioImage}
\alias{medioImage}
\alias{print.medioImage}
\title{
  Images of All Formats
}
\description{
  Makes an image from voxel data, geometry, frame timing and header
  attributes, which \code{\link{writeMedio}} writes into a file of any
  format. \code{\link{readMedio}} returns images of the same structure.
}
\usage{
medioImage(data, affine = NULL, frame_start = NULL, frame_duration = NULL,
           attributes = NULL)

\method{print}{medioImage}(x, \dots)
}
\arguments{
  \item{data}{
    numeric (double or integer) 3D array (x, y, z) of one frame or 4D array
    (x, y, z, frame) with the physical values.
  }
  \item{affine}{
    3 x 4 (or 4 x 4) matrix transforming the voxel indices (i, j, k),
    counted from 0, into DICOM patient coordinates (LPS, mm), see
    \code{\link{readMedio}}. \code{NULL}: identity (1 mm voxels).
  }
  \item{frame_start, frame_duration}{
    start and duration of every frame in seconds. \code{NULL}: 0.
  }
  \item{attributes}{
    header attributes as named list, as \code{\link{readMedio}} returns
    them (e.g. \code{list(PatientWeight = 72.5)}). \code{NULL}: none.
  }
  \item{x}{
    a \code{"medioImage"}.
  }
  \item{\dots}{
    ignored.
  }
}
\value{
  An object of class \code{"medioImage"}: a list with the elements
  \code{data} (4D array), \code{affine} (3 x 4 matrix),
  \code{frame_start}, \code{frame_duration}, \code{attributes} and
  \code{format} and \code{filename} (\code{NULL}, set by
  \code{\link{readMedio}}). The elements can be changed;
  \code{\link{writeMedio}} checks them again.
}
\author{
  Jens Maus
}
\seealso{
  \code{\link{readMedio}}, \code{\link{writeMedio}}
}
\examples{
# a 4D image of two frames with 2 x 2 x 3 mm voxels (LPS voxel order)
data <- array(rnorm(5 * 4 * 3 * 2), dim = c(5, 4, 3, 2))
affine <- cbind(diag(c(2, 2, 3)), c(-4, -3, -3))
image <- medioImage(data, affine, frame_start = c(0, 60), frame_duration = c(60, 60),
                    attributes = list(PatientName = "Test^Name"))
image
}
\keyword{IO}
