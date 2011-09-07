writeEcat <- function(ecat, vfilename=NULL, overwrite=NULL)
{
  dyn.load("/usr/local/petlib/lib/libmedio.so.2.7")
  res <- .Call("writeEcat", ecat, vfilename, overwrite);
  res
}
