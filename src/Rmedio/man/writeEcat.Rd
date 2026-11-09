\name{writeEcat}
\alias{writeEcat}
\title{
  function to write out an ECAT7 file
}
\description{
  This function will take the supplied ECAT structure and write the
  data set out as a standard ECAT7 file including the embedded 3D
  volume in the structure.
}
\usage{
  writeEcat(ecat, 
            owrite=FALSE,
            fname=NULL,
            dtype=NULL)
}
\arguments{
  \item{ecat}{
    the ECAT structure to write out (supplied by e.g. readEcat)
  }
  \item{owrite}{
    if TRUE the output file will be overwritten, otherwise a
    warning will appear if the file already exists
  }
  \item{fname}{
    the filename of the output file. if omitted a standard file
    name will be given based on the ECAT variable name supplied.
  }
  \item{dtype}{
    per default writeEcat() will write out the supplied ecat data
    in the same data type as specified in the subHeader of each
    subMatrix. Using the 'dtype' argument a user can, however,
    force to write out the data set using the supplied type.
    Supported types are:
      'short' - SunShort (16 bit big endian)
      'long'  - SunLong (32 bit big endian)
      'float' - IEEEFloat (32 bit floating point)
  }
}
\details{
  Please note that the R function is only a wrapper function to a
  larger C++ function which in fact directly uses the C++-based libmedio
  library for reading the data set.

  Please note that this function also accepts arbitrary 3D data array which
  are not of class 'Ecat'. In this case writeEcat will generate a default
  header and fill it with information which can be identified by the
  object type. Thus, the following code should work to output a 3D array:

  R> x=c(1:10)
  R> attr(x, 'dim')=c(5,2,1)
  R> writeEcat(x)

  This will output the 1:10 array as a 3D Array with dimension 5,2,1 and
  voxel size 1,1,1. If you want to modify the voxel size you can also
  override it via

  R> attr(x, 'voxsiz')=c(2,2,2)

  before writing it out so that you will end up with 2x2x2 mm voxel size.
}
\value{
  The function will return the saved ECAT structure on success,
  otherwise NULL is returned.
}
\author{
  J. Maus
}
\seealso{
  readEcat, fzrutils::disp
}
