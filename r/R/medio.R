#
# libmedio - C++ I/O Library for loading/saving medical data formats
#            https://github.com/hzdr-MedImaging/libmedio
#
# Copyright (C) 2004-2026 hzdr.de and contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Format independent access to images of all formats of libmedio: all frames of
# a file as one 4D array with affine, frame timing and header attributes (like
# pmedio.load()/pmedio.save() of the Python bindings).

medioImage <- function(data, affine = NULL, frame_start = NULL, frame_duration = NULL, attributes = NULL)
{
  if (!(is.double(data) || is.integer(data)) || is.null(dim(data)) || !(length(dim(data)) %in% c(3L, 4L)))
    stop("'data' has to be a numeric 3D array (x, y, z) or 4D array (x, y, z, frame)")
  if (length(dim(data)) == 3L)
    dim(data) <- c(dim(data), 1L)
  if (any(dim(data) < 1L))
    stop("'data' needs at least one voxel in every dimension")
  frames <- dim(data)[4L]
  if (frames > 32767L)
    stop("an image has 1..32767 frames, not ", frames)

  if (is.null(affine))
    affine <- cbind(diag(3), 0)
  if (!is.numeric(affine) || !is.matrix(affine) || ncol(affine) != 4L || !(nrow(affine) %in% c(3L, 4L)) ||
      anyNA(affine))
    stop("'affine' has to be a numeric 3 x 4 or 4 x 4 matrix")
  if (nrow(affine) == 4L) {
    if (!identical(as.numeric(affine[4L, ]), c(0, 0, 0, 1)))
      stop("the last row of a 4 x 4 affine has to be 0 0 0 1")
    affine <- affine[1:3, , drop = FALSE]
  }
  storage.mode(affine) <- "double"
  dimnames(affine) <- NULL

  timing <- function(values, name) {
    if (is.null(values))
      return(rep(0, frames))
    if (!is.numeric(values) || length(values) != frames || anyNA(values))
      stop("'", name, "' has to give one value per frame (", frames, ")")
    as.double(values)
  }
  frame_start <- timing(frame_start, "frame_start")
  frame_duration <- timing(frame_duration, "frame_duration")

  if (is.null(attributes))
    attributes <- list()
  if (!is.list(attributes) || (length(attributes) > 0L &&
      (is.null(names(attributes)) || anyNA(names(attributes)) || any(names(attributes) == ""))))
    stop("'attributes' has to be a named list")

  structure(list(data = data, affine = affine, frame_start = frame_start, frame_duration = frame_duration,
                 attributes = attributes, format = NULL, filename = NULL),
            class = "medioImage")
}

readMedio <- function(fname, frames = NULL, orientation = NULL)
{
  if (!is.character(fname) || length(fname) != 1L || is.na(fname))
    stop("'fname' has to be the name of a file")
  if (!is.null(frames)) {
    if (!is.numeric(frames) || anyNA(frames) || any(frames != round(frames)))
      stop("'frames' has to give frame numbers")
    frames <- as.integer(frames)
  }
  if (!is.null(orientation) && (!is.character(orientation) || length(orientation) != 1L || is.na(orientation)))
    stop("'orientation' has to be an orientation code, e.g. \"RAS\"")

  image <- .Call(C_readMedio, path.expand(fname), frames, orientation)
  class(image) <- "medioImage"
  image
}

writeMedio <- function(image, fname, like = NULL, format = NULL, overwrite = FALSE)
{
  if (!is.character(fname) || length(fname) != 1L || is.na(fname))
    stop("'fname' has to be the name of a file")
  if (!is.null(like) && (!is.character(like) || length(like) != 1L || is.na(like)))
    stop("'like' has to be the name of a file")
  if (!is.null(format) && (!is.character(format) || length(format) != 1L || is.na(format)))
    stop("'format' has to be \"ecat\", \"nifti\", \"philips\" or \"interfile\"")
  if (!is.logical(overwrite) || length(overwrite) != 1L || is.na(overwrite))
    stop("'overwrite' has to be TRUE or FALSE")

  # an array, or an image checked again (its elements can have been changed)
  image <- if (inherits(image, "medioImage"))
    medioImage(image$data, image$affine, image$frame_start, image$frame_duration, image$attributes)
  else
    medioImage(image)

  written <- .Call(C_writeMedio, path.expand(fname), image$data, image$affine, image$frame_start,
                   image$frame_duration, image$attributes, if (is.null(like)) NULL else path.expand(like),
                   format, overwrite)
  invisible(written)
}

print.medioImage <- function(x, ...)
{
  d <- dim(x$data)
  cat("medioImage ", paste(d, collapse = " x "), " (x, y, z, frames)", sep = "")
  if (!is.null(x$filename))
    cat(", ", x$format, " file '", x$filename, "'", sep = "")
  cat("\n")
  cat("  voxel size (mm):", format(sqrt(colSums(x$affine[, 1:3, drop = FALSE]^2)), digits = 4), "\n")
  if (d[4L] > 1L || any(x$frame_duration > 0))
    cat("  frames (s):", paste0(format(x$frame_start), "+", format(x$frame_duration), collapse = ", "), "\n")
  cat("  attributes:", length(x$attributes), "\n")
  invisible(x)
}
