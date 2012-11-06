writeEcat <- function(ecat, owrite=FALSE, fname=NULL)
{
  # we use the libmedio Rcpp interface if the library can be loaded
  if(file.exists("/usr/local/petlib/lib/libmedio.so"))
  {
    if(is.null(fname))
      fname = paste(substitute(ecat), ".v", sep='')

    dyn.load("/usr/local/petlib/lib/libmedio.so")
    res = .Call("writeEcat", ecat, fname, owrite, PACKAGE="libmedio")
  }
  else
  {
    cat("libmedio library not found. Please make sure to properly install libmedio in '/usr/local/petlib/lib'\n")
    res = NULL
  }

  invisible(res)
}
