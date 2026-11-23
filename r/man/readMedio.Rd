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
\name{readMedio}
\alias{readMedio}
\title{
  Read Images of All Formats
}
\description{
  Reads all frames of an ECAT7, NIfTI (\code{.nii}, \code{.nii.gz},
  \code{.hdr}/\code{.img}), Philips PET (\code{.img}, \code{.syn}),
  Interfile (a header, a Siemens main header \code{.mhdr} or a data file) or
  Siemens Concorde microPET image file as one 4D array, together with the
  geometry, the frame timing and the header attributes.
}
\usage{
readMedio(fname, frames = NULL, orientation = NULL)
}
\arguments{
  \item{fname}{
    path to the image file.
  }
  \item{frames}{
    numbers of the frames to read, in the order given (numbered from 1 for
    most files). \code{NULL}: all frames.
  }
  \item{orientation}{
    orientation code to reorder the voxels, e.g. \code{"RAS"} (the
    convention of nibabel and many NIfTI tools: x towards the patient's
    right, y towards anterior, z towards superior) or \code{"LPI"}. Every
    voxel keeps its position, the affine is adjusted. \code{NULL}: the voxel
    order of the file (e.g. \code{"LPI"} for ECAT7, Philips, microPET and
    Siemens Interfile: x towards left, y towards posterior, planes from
    superior to inferior).
  }
}
\details{
  The values are the physical values (the stored values multiplied with
  the scale factors, e.g. the ECAT calibration factor) as doubles. All
  frames have to have the same dimensions and geometry; read the frames of
  other files one at a time with \code{frames}.

  The header attributes are the format independent attributes of
  'libmedio': DICOM keywords with the units of DICOM (e.g.
  \code{PatientWeight} in kg), the native fields of the format in a
  section named after it (e.g. \code{ECAT7}) and in \code{Frames} the
  attributes of every frame. JSON objects are named lists, arrays unnamed
  lists, so that \code{\link{writeMedio}} writes them back unchanged.

  \code{\link{readEcat}} reads the same values of an ECAT file as arrays of
  rows x columns x planes: \code{readEcat(f)[[k]]} corresponds to
  \code{aperm(readMedio(f)$data[, , , k], c(2, 1, 3))}.
}
\value{
  An object of class \code{"medioImage"} (see \code{\link{medioImage}}):
  a list with
  \item{data}{the physical values as 4D array (x, y, z, frame), x running
    fastest.}
  \item{affine}{3 x 4 matrix transforming the voxel indices (i, j, k),
    counted from 0 (\code{data[i + 1, j + 1, k + 1, ]}), into DICOM patient
    coordinates (LPS, mm: x towards the patient's left, y towards posterior,
    z towards superior).}
  \item{frame_start, frame_duration}{start and duration of every frame in
    seconds.}
  \item{attributes}{the header attributes as named list.}
  \item{format}{the format of the file: \code{"ecat"}, \code{"nifti"},
    \code{"philips"}, \code{"interfile"} or \code{"micropet"}.}
  \item{filename}{the absolute path of the file.}
}
\author{
  Jens Maus
}
\seealso{
  \code{\link{writeMedio}}, \code{\link{medioImage}}
}
\examples{
image <- readMedio(system.file("extdata", "pet-dynamic.v", package = "Rmedio"))
image
dim(image$data)
image$frame_start
image$attributes$PatientWeight

# the second frame, voxels in the order of nibabel (RAS)
ras <- readMedio(system.file("extdata", "pet-dynamic.nii.gz", package = "Rmedio"),
                 frames = 2, orientation = "RAS")
ras$affine
}
\keyword{IO}
\keyword{file}
