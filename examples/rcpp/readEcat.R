readEcat <- function(vfile, nv = NULL, rows = NULL, cols = NULL, planes = NULL)
{
  dyn.load("/usr/local/petlib/lib/libmedio.so.2.7")
  v = .Call("readEcat", vfile, nv, rows, cols, planes);
  v
}
