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
\name{readEcat}
\alias{readEcat}
\title{
  Read ECAT Image Volumes
}
\description{
  Imports a volumetric ECAT 6 or ECAT 7 data set, i.e. one or more 3D
  arrays together with the corresponding meta data.
}
\usage{
readEcat(fname = NULL, nv = NULL, rows = NULL, cols = NULL, planes = NULL)
}
\arguments{
  \item{fname}{
    path to the ECAT file to read. If \code{NULL}, an empty template
    structure (default main header and one 10 x 10 x 10 volume) is
    returned, which can be filled with own data and written with
    \code{\link{writeEcat}}.
  }
  \item{nv}{
    vector of indices of the volumes (frames, gates or bed positions) to
    read. \code{NULL}: all volumes.
  }
  \item{rows}{
    subset the 3D volumes with this vector of row (y) indices.
  }
  \item{cols}{
    subset the 3D volumes with this vector of column (x) indices.
  }
  \item{planes}{
    subset the 3D volumes with this vector of plane (z) indices.
  }
}
\details{
  The imported data are returned as a list of 3D volumes. The data are
  already scaled to floating point values and multiplied by the ECAT
  calibration factor. The ECAT headers are attached to the list and its
  components (the individual volumes) as attributes.

  Via the \code{rows}, \code{cols} and \code{planes} arguments it is
  possible to import only a subset of the voxels (a subvolume or equally
  spaced samples). This reduces the otherwise potentially considerable
  memory usage (e.g. 1 GB for 32 volumes of dimensions 256 x 256 x 63).
  Note that if this subsetting is used, the dimensions in the sub headers
  no longer reflect the actual size of the R arrays.

  A file can only contain one dynamic dimension: several frames, gates or
  bed positions. The volumes are named after it, e.g. \code{"f1"},
  \code{"f2"}, ... for frames, \code{"g1"}, ... for gates and
  \code{"b1"}, ... for bed positions.
}
\value{
  A list of class \code{"Ecat"} of 3D arrays of class \code{"Volume"}
  with the dimensions rows (y), columns (x) and planes (z). The list has
  the attributes \code{"mhead"} (main header) and
  \code{"frame_midpoint"} (mid time of each volume in seconds), each
  volume the attributes \code{"shead"} (sub header), \code{"voxsiz"}
  (voxel size in cm, in the order of the array dimensions) and
  \code{"vgrid"} (indices of the imported rows, columns and planes).
  If the file cannot be read, an error message is printed and an empty
  list is returned.
}
\author{
  Jens Maus
}
\seealso{
  \code{\link{writeEcat}}
}
\examples{
fname <- system.file("extdata", "pet-dynamic.v", package = "Rmedio")
ecat <- readEcat(fname)
names(ecat)
dim(ecat[["f1"]])
attr(ecat[["f1"]], "voxsiz")
attr(ecat, "frame_midpoint")

# only the second frame and the planes 10 to 12
sub <- readEcat(fname, nv = 2, planes = 10:12)
dim(sub[["f2"]])
}
\keyword{IO}
\keyword{file}
