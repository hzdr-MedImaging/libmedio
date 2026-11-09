readEcat <- function(fname = NULL, nv = NULL, rows = NULL, cols = NULL, planes = NULL)
{
  res = .Call('readEcat', fname, nv, rows, cols, planes, PACKAGE='Rmedio')
  invisible(res)
}
