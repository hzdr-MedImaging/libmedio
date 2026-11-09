\name{readEcat}
\alias{readEcat}
\title{
  Read ECAT7 Volume Files
}
\description{
  This function imports a volumetric ECAT7 data set, i.e. one or more
  3D arrays together with the corresponding meta data.
}
\usage{
  readEcat(fname,
           nv = NULL,
           rows = NULL,
           cols = NULL,
           planes = NULL)
}
\arguments{
  \item{fname}{
    path to the ECAT file to read
  }
  \item{nv}{
    vector of indices of the volumes (frames, gates) to read in. 
    \code{NULL}: all volumes.
  }
  \item{rows}{
    subset the 3D volumn with this vector of rows (y-) indices 
  }
  \item{cols}{
    subset the 3D volumn with this vector of column (x-) indices 
  }
  \item{planes}{
    subset the 3D volumn with this vector of plane (z-) indices 
  }
}
\details{
  The imported data are returned as a list of 3D volumes. The data are
  already scaled to float and multiplied by the ECAT calibration factor. The
  ECAT headers are attached to the list and its components (the individual
  volumes) as attributes.

  Via the 'rows, cols, planes' arguments it is possible to specify a voxel
  subset (subvolume or equally spaced samples) to be imported. This is
  especially helpful to reduce the otherwise potentially quite considerable
  memory usage (e.g. 1GB for 32 volumes of dimensions 256 x 256 x 63 ...).
  Note that if this subsetting is used, the subheader entries for dimensions
  do no longer reflect the actual size of the 'R' arrays.

  Note that this function depends on availability of the \code{libmedio} C++ library.
}
\value{
  a list of class 'Ecat' (with attributes 'mhead', 'shead', 'voxsiz'
  containing the metadata) of 3D arrays of class 'Volume'. There
  are several attributes to the 'Volume' object, which are utilized
  by 'disp.array' (the set is not yet stable).
}
\author{
  J. Maus
}
\seealso{
  writeEcat, fzrutils::disp
}
\keyword{utilities}
