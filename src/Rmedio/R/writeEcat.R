writeEcat <- function(ecat, owrite=FALSE, fname=NULL, dtype=NULL)
{
  if(is.null(fname))
    fname = paste(make.names(deparse(substitute(ecat))), ".v", sep='')

  res = .Call('writeEcat', ecat, fname, owrite, dtype, PACKAGE='Rmedio')
  invisible(res)
}
