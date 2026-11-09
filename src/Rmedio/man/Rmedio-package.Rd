\name{Rmedio-package}
\alias{Rmedio-package}
\alias{Rmedio}
\alias{libRmedio}
\alias{libmedio}
\docType{package}
\title{
  R interface to libmedio
}
\description{
  This R package provides an interface to the functionality
  in libmedio for reading and writing medical data sets. While this
  package aims at providing functionality for all supported data
  formats supported by libmedio, it currently only supports reading
  and writing of ECAT datasets.
}
\details{
  \tabular{ll}{
    Package: \tab Rmedio\cr
    Type: \tab Package\cr
    Version: \tab 1.0\cr
    Date: \tab 2013-11-06\cr
    License: \tab GPL (>=2)\cr
  }
  The package currently consists of two main functions (readEcat, writeEcat)
  which will allow to read and write ECAT datasets. When using readEcat()
  you will get a structure with all metainformation and the 3D dataset attached
  to it. This structure can then be, e.g., supplied to the fzrutils::disp()
  function to display the slices accordingly. When the same structure is
  supplied to writeEcat() it will write out the data set as a standard ECAT7
  file.
}
\author{
  Jens Maus <mail@jens-maus.de>
}
\keyword{ libmedio }
\keyword{ medio }
\keyword{ ecat }
\seealso{
  readEcat, writeEcat, fzrutils::disp
}
