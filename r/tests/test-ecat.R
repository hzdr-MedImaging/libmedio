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
# Reads the bundled ECAT7 test datasets and compares them with the expected
# values in <file>.json, which were generated with nibabel (independent of
# libmedio, see testsuite/data/README.md of the libmedio repository).
# Writes the data again and checks that it reads back unchanged.
#

library(Rmedio)

if (!requireNamespace("jsonlite", quietly = TRUE))
  stop("the tests need the 'jsonlite' package")

check_file <- function(name)
{
  fname <- system.file("extdata", name, package = "Rmedio", mustWork = TRUE)
  expected <- jsonlite::fromJSON(paste0(fname, ".json"), simplifyVector = FALSE)

  ecat <- readEcat(fname)
  mhead <- attr(ecat, "mhead")
  stopifnot(inherits(ecat, "Ecat"),
            mhead[["num_frames"]] == expected$main_header$num_frames,
            mhead[["num_planes"]] == expected$main_header$num_planes,
            length(ecat) == length(expected$matrices))

  calibration <- mhead[["ecat_calibration_factor"]]
  stopifnot(isTRUE(all.equal(calibration, expected$main_header$calibration_factor)))

  for (m in expected$matrices)
  {
    volume <- ecat[[paste0("f", m$frame)]]
    shead <- attr(volume, "shead")
    dims <- unlist(m$dims)

    # R array: rows (y), columns (x), planes (z)
    stopifnot(identical(dim(volume), as.integer(dims[c(2, 1, 3)])),
              shead[["data_type"]] == m$data_type,
              shead[["frame_start"]] == m$frame_start_time_ms,
              shead[["frame_duration"]] == m$frame_duration_ms,
              isTRUE(all.equal(shead[["scale_factor"]], m$scale_factor, tolerance = 1e-6)),
              isTRUE(all.equal(attr(volume, "voxsiz"),
                               unlist(m$pixel_size_cm)[c(2, 1, 3)], tolerance = 1e-6)))

    # readEcat() returns the stored values times scale and calibration factor
    stored <- round(as.vector(volume) / (shead[["scale_factor"]] * calibration))
    stopifnot(min(stored) == m$raw_min,
              max(stored) == m$raw_max,
              sum(stored) == m$raw_sum)
  }
}

# writes the data of an ECAT file again and checks that it reads back unchanged
check_roundtrip <- function(name)
{
  ecat <- readEcat(system.file("extdata", name, package = "Rmedio", mustWork = TRUE))
  out <- tempfile(fileext = ".v")
  on.exit(unlink(out))
  writeEcat(ecat, fname = out)
  ecat2 <- readEcat(out)
  stopifnot(identical(names(ecat2), names(ecat)))
  for (i in seq_along(ecat))
    stopifnot(isTRUE(all.equal(as.vector(ecat2[[i]]), as.vector(ecat[[i]]),
                               tolerance = 1e-6)))
}

check_file("pet-static.v")
check_file("pet-dynamic.v")
check_roundtrip("pet-static.v")
# calibration factor 1000: the stored integers must still use the full range
check_roundtrip("pet-dynamic.v")

# small values written as integers keep their precision (scale factor for the
# full range of the type instead of truncation to whole numbers)
x <- array(seq(-0.5, 1, length.out = 60), dim = c(5, 4, 3))
for (dtype in c("short", "int"))
{
  out <- tempfile(fileext = ".v")
  writeEcat(x, fname = out, dtype = dtype)
  y <- as.vector(readEcat(out)[["f1"]])
  unlink(out)
  stopifnot(max(abs(y - as.vector(x))) <= 1 / 32767)
}

# subsetting of volumes, rows, columns and planes
fname <- system.file("extdata", "pet-dynamic.v", package = "Rmedio", mustWork = TRUE)
all <- readEcat(fname)
sub <- readEcat(fname, nv = 2, rows = 3:5, cols = c(1, 7), planes = 10)
stopifnot(identical(names(sub), "f2"),
          identical(dim(sub[["f2"]]), c(3L, 2L, 1L)),
          isTRUE(all.equal(as.vector(sub[["f2"]]),
                           as.vector(all[["f2"]][3:5, c(1, 7), 10, drop = FALSE]))))

# writing a plain array creates an ECAT7 file with default headers
x <- array(as.numeric(1:60), dim = c(5, 4, 3))
attr(x, "voxsiz") <- c(2, 2, 3)
out <- tempfile(fileext = ".v")
writeEcat(x, fname = out)
y <- readEcat(out)
unlink(out)
stopifnot(identical(dim(y[["f1"]]), dim(x)),
          isTRUE(all.equal(as.vector(y[["f1"]]), as.vector(x))),
          isTRUE(all.equal(as.vector(attr(y[["f1"]], "voxsiz")), c(2, 2, 3))))

# without a file name, readEcat() returns a template structure
template <- readEcat()
stopifnot(inherits(template, "Ecat"), length(template) == 1L)
