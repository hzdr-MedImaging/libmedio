readEcat <- function(fname, nv = NULL, rows = NULL, cols = NULL, planes = NULL)
{
  # we use the libmedio Rcpp interface if the library can be loaded
  if(file.exists("/usr/local/petlib/lib/libmedio.so"))
  {
    dyn.load("/usr/local/petlib/lib/libmedio.so")
    v = .Call("readEcat", fname, nv, rows, cols, planes, PACKAGE="libmedio")
  }
  else
  {
    cat("libmedio library not found. Please make sure to properly install libmedio in '/usr/local/petlib/lib'\n")
    v = NULL
  }

  v
}
