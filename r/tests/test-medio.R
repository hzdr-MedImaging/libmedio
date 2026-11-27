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
# Tests of readMedio(), writeMedio() and medioImage(): images of all formats.
#
# The bundled test files are compared with the expected values in <file>.json,
# which were generated with nibabel or documented manually (independent of
# libmedio, see testsuite/data/README.md of the libmedio repository).
# Synthetic images are their own expected values.
#

library(Rmedio)

if (!requireNamespace("jsonlite", quietly = TRUE))
  stop("the tests need the 'jsonlite' package")

extdata <- function(name) system.file("extdata", name, package = "Rmedio", mustWork = TRUE)
expected <- function(name) jsonlite::fromJSON(paste0(extdata(name), ".json"), simplifyVector = FALSE)

# the error message of an expression that has to fail
fails <- function(expr, pattern)
{
  message <- tryCatch({ expr; NULL }, error = function(e) conditionMessage(e))
  if (is.null(message))
    stop("no error, expected: ", pattern)
  if (!grepl(pattern, message))
    stop("error '", message, "' does not match '", pattern, "'")
  invisible(message)
}

near <- function(a, b, tolerance = 1e-6) isTRUE(all.equal(a, b, tolerance = tolerance, check.attributes = FALSE))

# 'name' is an absolute path of the file 'path'. The file names libmedio returns are
# absolute but keep symbolic links, while normalizePath() resolves them (e.g. the
# temporary directory /var/folders/... of macOS is /private/var/folders/...)
same_file <- function(name, path)
  is.character(name) && length(name) == 1L && grepl("^(/|[A-Za-z]:[/\\\\])", name) &&
    identical(normalizePath(name), normalizePath(path))

#-----------------------------------------------------------------------------------------------
# reading: the test files against their expected values

ecat <- expected("pet-dynamic.v")
calibration <- ecat$main_header$calibration_factor
ecat_scales <- vapply(ecat$matrices, function(m) m$scale_factor * calibration, 0)
ecat_start <- vapply(ecat$matrices, function(m) m$frame_start_time_ms / 1000, 0)
ecat_duration <- vapply(ecat$matrices, function(m) m$frame_duration_ms / 1000, 0)

check_read <- function(name, format, dims, raw_sums, scales, start = NULL, duration = NULL)
{
  image <- readMedio(extdata(name))
  stopifnot(inherits(image, "medioImage"),
            identical(image$format, format),
            same_file(image$filename, extdata(name)),
            identical(dim(image$data), as.integer(c(dims, length(raw_sums)))),
            is.double(image$data),
            identical(dim(image$affine), c(3L, 4L)),
            is.list(image$attributes))
  # physical values: stored values x scale factor
  for (k in seq_along(raw_sums))
    stopifnot(near(sum(image$data[, , , k]), raw_sums[k] * scales[k], 1e-6))
  if (!is.null(start))
    stopifnot(near(image$frame_start, start, 1e-9), near(image$frame_duration, duration, 1e-9))
  invisible(image)
}

raw_sums <- function(e, key) vapply(e[[key]], function(v) v$raw_sum, 0)

dynamic <- check_read("pet-dynamic.v", "ecat", unlist(ecat$matrices[[1]]$dims), raw_sums(ecat, "matrices"),
                      ecat_scales, ecat_start, ecat_duration)
static <- expected("pet-static.v")
check_read("pet-static.v", "ecat", unlist(static$matrices[[1]]$dims), raw_sums(static, "matrices"),
           static$matrices[[1]]$scale_factor * static$main_header$calibration_factor,
           static$matrices[[1]]$frame_start_time_ms / 1000, static$matrices[[1]]$frame_duration_ms / 1000)
nifti <- expected("pet-dynamic.nii.gz")
check_read("pet-dynamic.nii.gz", "nifti", unlist(nifti$header$dim)[2:4], raw_sums(nifti, "volumes"),
           rep(nifti$header$scl_slope, 3))
philips <- expected("pet-dynamic.img")
check_read("pet-dynamic.img", "philips", unlist(philips$dims), raw_sums(philips, "volumes"),
           ecat_scales, ecat_start, ecat_duration)

# Interfile: PMOD converted the ECAT7 file into Interfile 3.3 with the planes from inferior
# to superior and the values in kBq/cc (the ECAT7 file gives Bq/cc)
interfile <- readMedio(extdata("pet-dynamic.hdr"), orientation = "LPI")
stopifnot(identical(interfile$format, "interfile"),
          identical(dim(interfile$data), dim(dynamic$data)),
          near(interfile$data * 1000, dynamic$data, 1e-6),
          near(interfile$frame_start, dynamic$frame_start, 1e-9),
          near(interfile$frame_duration, dynamic$frame_duration, 1e-9),
          identical(interfile$attributes$PatientName, dynamic$attributes$PatientName),
          identical(interfile$attributes$Units, "KBQCC"),
          is.list(interfile$attributes$Interfile))
# written as Interfile (as STIR writes images) and read again
hv <- file.path(tempdir(), "interfile.hv")
writeMedio(interfile, hv, overwrite = TRUE)
stopifnot(file.exists(file.path(tempdir(), "interfile.v")))
again <- readMedio(hv, orientation = "LPI")
stopifnot(identical(again$format, "interfile"),
          near(again$data, interfile$data, 1e-6),
          near(again$affine, interfile$affine, 1e-6),
          near(again$frame_start, interfile$frame_start, 1e-9),
          identical(again$attributes$PatientName, interfile$attributes$PatientName))

# the header attributes (DICOM keywords, native fields in the section of the format)
stopifnot(identical(dynamic$attributes$PatientName, "Phantom_NEMA_Cu61_kontrast8"),
          near(dynamic$attributes$PatientWeight, 8.63),
          is.list(dynamic$attributes$ECAT7),
          identical(dynamic$attributes$OperatorsName, list("0.000000;;0")),
          length(dynamic$attributes$Frames) == 3L,
          is.null(dynamic$attributes$NIfTI))

# readEcat() returns the same values in the voxel order of the file, as rows x columns
for (k in 1:3)
  stopifnot(near(aperm(dynamic$data[, , , k], c(2, 1, 3)), unclass(readEcat(extdata("pet-dynamic.v"))[[k]])))

# frames by their numbers, in the given order
selected <- readMedio(extdata("pet-dynamic.v"), frames = c(3, 1))
stopifnot(identical(dim(selected$data)[4], 2L),
          identical(selected$data[, , , 1], dynamic$data[, , , 3]),
          identical(selected$data[, , , 2], dynamic$data[, , , 1]),
          identical(selected$frame_start, dynamic$frame_start[c(3, 1)]))

# reoriented voxels: ECAT7 files are stored LPI, so RAS reverses all three axes
ras <- readMedio(extdata("pet-dynamic.v"), orientation = "RAS")
stopifnot(identical(ras$data, dynamic$data[31:1, 31:1, 39:1, , drop = FALSE]))
voxel <- function(affine, i) affine %*% c(i, 1)
stopifnot(near(voxel(ras$affine, c(30, 30, 38)), voxel(dynamic$affine, c(0, 0, 0))))

fails(readMedio(extdata("pet-dynamic.v"), frames = 4), "frame 4 does not exist")
fails(readMedio(extdata("pet-dynamic.v"), frames = 1.5), "frame numbers")
fails(readMedio(extdata("pet-dynamic.v"), orientation = "XYZ"), "invalid orientation")
fails(readMedio(file.path(tempdir(), "missing.v")), "does not exist")
fails(readMedio(extdata("pet-dynamic.v.json")), "not a supported image file")

#-----------------------------------------------------------------------------------------------
# writing: a synthetic image in every format

shape <- c(5L, 4L, 3L)
affine <- rbind(c(-2, 0, 0, 10), c(0, -2.5, 0, -20), c(0, 0, 3, -30))   # RAS voxel order
start <- c(0, 60, 120)
duration <- c(30, 31, 32.5)
attributes <- list(PatientName = "Test^Name", PatientID = "4711", PatientWeight = 72.5, Radiopharmaceutical = "FDG")
values <- array(seq_len(prod(shape) * 3) + 0.25, dim = c(shape, 3))   # distinct, exact as float32
image <- medioImage(values, affine, start, duration, attributes)

formats <- c(".v" = "ecat", ".nii" = "nifti", ".nii.gz" = "nifti", ".hdr" = "nifti", ".img" = "philips", ".syn" = "philips",
             ".hv" = "interfile")
for (suffix in names(formats))
{
  dir <- tempfile()
  dir.create(dir)
  out <- file.path(dir, paste0("image", suffix))
  stopifnot(same_file(writeMedio(image, out), out))
  back <- readMedio(out, orientation = "RAS")
  expected_affine <- affine
  if (formats[[suffix]] == "philips") {
    # Philips files store no position within the planes: they are centered
    expected_affine[1, 4] <- -(shape[1] - 1) / 2 * affine[1, 1]
    expected_affine[2, 4] <- -(shape[2] - 1) / 2 * affine[2, 2]
  }
  stopifnot(identical(back$format, formats[[suffix]]),
            identical(back$data, values),
            near(back$affine, expected_affine, 1e-5),
            near(back$frame_start, start), near(back$frame_duration, duration))
  for (key in names(attributes))
    stopifnot(identical(back$attributes[[key]], attributes[[key]]))

  # integer data is stored as integers
  integers <- array(seq_len(prod(shape)) - 7L, dim = shape)
  writeMedio(integers, out, overwrite = TRUE)
  stopifnot(identical(readMedio(out, orientation = "LPS")$data[, , , 1], array(as.double(integers), dim = shape)))

  # an existing file is replaced only with overwrite = TRUE
  fails(writeMedio(image, out), "exists already")
  unlink(dir, recursive = TRUE)
}

# the image file of a NIfTI header/image pair is not replaced without overwrite = TRUE,
# e.g. a Philips file of the same name
dir <- tempfile()
dir.create(dir)
writeMedio(image, file.path(dir, "pair.img"))
fails(writeMedio(image, file.path(dir, "pair.hdr")), "pair.img', the image file of .*exists already")
stopifnot(!file.exists(file.path(dir, "pair.hdr")), identical(readMedio(file.path(dir, "pair.img"))$format, "philips"))
writeMedio(image, file.path(dir, "pair.hdr"), overwrite = TRUE)
stopifnot(identical(readMedio(file.path(dir, "pair.hdr"))$format, "nifti"))
# a .hdr file of another format has no image file
writeMedio(image, file.path(dir, "other.img"))
writeMedio(image, file.path(dir, "other.hdr"), format = "ecat")
stopifnot(identical(readMedio(file.path(dir, "other.img"))$format, "philips"),
          identical(readMedio(file.path(dir, "other.hdr"))$format, "ecat"))
# an existing image file in the other letter case is the image file of the pair (see CNIFTIFile);
# file systems that ignore the letter case (macOS) find it as mixed.img, the same file
writeLines("other data", file.path(dir, "mixed.IMG"))
fails(writeMedio(image, file.path(dir, "mixed.hdr")), "mixed\\.(IMG|img)', the image file of .*exists already")
stopifnot(identical(readLines(file.path(dir, "mixed.IMG")), "other data"))
unlink(dir, recursive = TRUE)

# every test file in every format: the same values, positions and timing
for (name in c("pet-dynamic.v", "pet-dynamic.nii.gz", "pet-dynamic.img"))
  for (suffix in names(formats))
  {
    source <- readMedio(extdata(name), orientation = "RAS")
    out <- tempfile(fileext = suffix)
    writeMedio(source, out)
    back <- readMedio(out, orientation = "RAS")
    unlink(c(out, sub("\\.hdr$", ".img", out)))
    columns <- if (formats[[suffix]] == "philips") 1:3 else 1:4
    stopifnot(near(back$data, source$data),
              near(back$affine[, columns], source$affine[, columns], 1e-5),
              identical(back$frame_start, source$frame_start),
              identical(back$attributes$PatientName, source$attributes$PatientName))
  }

# the attributes of a file written into another file keep their structure
out <- tempfile(fileext = ".nii")
writeMedio(dynamic, out)
written <- readMedio(out)$attributes
for (key in setdiff(names(dynamic$attributes), "NIfTI"))
  stopifnot(identical(written[[key]], dynamic$attributes[[key]]))
unlink(out)

# with the main header of a source file
out <- tempfile(fileext = ".nii")
changed <- dynamic
changed$attributes <- list(PatientWeight = 99)
writeMedio(changed, out, like = extdata("pet-dynamic.v"))
written <- readMedio(out)$attributes
stopifnot(identical(written$PatientWeight, 99), identical(written$PatientName, dynamic$attributes$PatientName))
unlink(out)
# a NIfTI file needs all frames of the source
fails(writeMedio(medioImage(dynamic$data[, , , 1]), out, like = extdata("pet-dynamic.v")), "needs 3")
stopifnot(!file.exists(out))

#-----------------------------------------------------------------------------------------------
# errors: nothing is written

dir <- tempfile()
dir.create(dir)
out <- file.path(dir, "image.v")
fails(writeMedio(medioImage(values, attributes = list(PatientWeight = "heavy")), out), "invalid header attributes")
fails(writeMedio(medioImage(values, attributes = list(PatientName = factor("x"))), out), "factor")
fails(writeMedio(medioImage(values, attributes = list(PatientWeight = Inf)), out), "infinite")
fails(writeMedio(medioImage(values, attributes = list(PatientWeight = sum)), out), "cannot be stored")
fails(writeMedio(medioImage(values, attributes = list(ECAT7 = list(1, 2))), out), "invalid header attributes")
fails(writeMedio(values, file.path(dir, "image.xyz")), "unknown file format")
fails(writeMedio(values, file.path(dir, "image.xyz"), format = "micropet"), "cannot be written")
fails(writeMedio(values, file.path(dir, "image.xyz"), format = "dicom"), "unknown format 'dicom'")
fails(writeMedio(array(c(1L, NA), dim = c(2, 1, 1)), out), "NA")
fails(writeMedio(values, out, like = file.path(dir, "missing.v")), "unable to open the source file")
stopifnot(length(list.files(dir)) == 0L)
# HDF5 files (here only the signature of the format) are identified; they cannot be read
# without HDF5 support, and this one is no image
hdf5 <- file.path(dir, "image.h5")
writeBin(as.raw(c(0x89, 0x48, 0x44, 0x46, 0x0d, 0x0a, 0x1a, 0x0a, rep(0, 504))), hdf5)
fails(readMedio(hdf5), "is an HDF5 file")
unlink(hdf5)

# HDF5 support is optional (medioHDF5()): with it, images are written and read like
# those of the other formats, without it both fail saying so
version <- medioHDF5()
stopifnot(is.character(version), length(version) == 1L)
if (is.na(version)) {
  fails(writeMedio(values, hdf5), "without HDF5 support")
  fails(writeMedio(values, file.path(dir, "image.dat"), format = "hdf5"), "without HDF5 support")
  stopifnot(length(list.files(dir)) == 0L)
} else {
  stopifnot(grepl("^[0-9]+\\.[0-9]+\\.[0-9]+$", version))
  for (name in c("image.h5", "image.hdf5", "image.hdf")) {
    path <- file.path(dir, name)
    image <- medioImage(array(as.double(1:120) / 8, dim = c(5, 4, 3, 2)),
                        affine = rbind(c(2, 0, 0, -4), c(0, 2.5, 0, -3), c(0, 0, -3, 10)),
                        frame_start = c(0, 60), frame_duration = c(60, 120),
                        attributes = list(PatientWeight = 72.5, Units = "BQML", SeriesNumber = 7))
    stopifnot(identical(writeMedio(image, path), normalizePath(path)))
    stopifnot(identical(list.files(dir), name))
    back <- readMedio(path)
    stopifnot(identical(back$format, "hdf5"),
              identical(dim(back$data), dim(image$data)),
              isTRUE(all.equal(back$data, image$data, tolerance = 1e-7, check.attributes = FALSE)),
              isTRUE(all.equal(back$affine, image$affine, check.attributes = FALSE)),
              identical(back$frame_start, c(0, 60)), identical(back$frame_duration, c(60, 120)),
              identical(back$attributes$PatientWeight, 72.5), identical(back$attributes$Units, "BQML"),
              isTRUE(all.equal(back$attributes$SeriesNumber, 7)))
    # an existing file is only replaced with overwrite = TRUE
    fails(writeMedio(image, path), "exists")
    stopifnot(isTRUE(all.equal(readMedio(path)$data, image$data, tolerance = 1e-7, check.attributes = FALSE)))
    doubled <- image
    doubled$data <- image$data * 2
    writeMedio(doubled, path, overwrite = TRUE)
    stopifnot(isTRUE(all.equal(readMedio(path)$data, doubled$data, tolerance = 1e-7, check.attributes = FALSE)))
    unlink(path)
  }
  # the format given explicitly
  writeMedio(values, file.path(dir, "image.dat"), format = "hdf5")
  stopifnot(identical(readMedio(file.path(dir, "image.dat"))$format, "hdf5"))
  unlink(file.path(dir, "image.dat"))

  # the test files of the other formats converted into HDF5 and back: the same values,
  # positions, timing and attributes
  for (name in list.files(system.file("extdata", package = "Rmedio"), pattern = "\\.(v|gz|img|hdr)$", full.names = TRUE)) {
    source <- readMedio(name, orientation = "LPI")
    writeMedio(source, hdf5, like = name)
    back <- readMedio(hdf5)
    stopifnot(isTRUE(all.equal(back$data, source$data, tolerance = 1e-6, check.attributes = FALSE)),
              isTRUE(all.equal(back$affine, source$affine, tolerance = 1e-6, check.attributes = FALSE)),
              isTRUE(all.equal(back$frame_start, source$frame_start)),
              isTRUE(all.equal(back$frame_duration, source$frame_duration)))
    for (key in intersect(c("PatientName", "PatientWeight", "RadionuclideTotalDose", "Units"), names(source$attributes)))
      stopifnot(identical(back$attributes[[key]], source$attributes[[key]]))
    unlink(hdf5)
  }
  stopifnot(length(list.files(dir)) == 0L)
}
# the format given explicitly
writeMedio(values, file.path(dir, "image.data"), format = "nifti")
stopifnot(identical(readMedio(file.path(dir, "image.data"))$format, "nifti"))
# format names in any letter case and abbreviated, like the Python bindings
writeMedio(values, file.path(dir, "image2.data"), format = "NIfTI")
writeMedio(values, file.path(dir, "image3.data"), format = "e")
stopifnot(identical(readMedio(file.path(dir, "image2.data"))$format, "nifti"),
          identical(readMedio(file.path(dir, "image3.data"))$format, "ecat"))
unlink(dir, recursive = TRUE)

#-----------------------------------------------------------------------------------------------
# medioImage()

default <- medioImage(array(0, dim = shape))
stopifnot(identical(dim(default$data), c(shape, 1L)),
          identical(default$affine, cbind(diag(3), 0)),
          identical(default$frame_start, 0), identical(default$frame_duration, 0),
          identical(default$attributes, list()),
          is.null(default$format), is.null(default$filename),
          identical(medioImage(values, rbind(affine, c(0, 0, 0, 1)))$affine, affine))
printed <- capture.output(print(dynamic))
stopifnot(grepl("31 x 31 x 39 x 3", printed[1]), grepl("ecat file", printed[1]))

fails(medioImage(matrix(0, 2, 2)), "3D array")
fails(medioImage(array("a", dim = shape)), "numeric")
fails(medioImage(array(0, dim = c(2, 2, 0))), "at least one voxel")
fails(medioImage(values, affine = diag(3)), "3 x 4 or 4 x 4")
fails(medioImage(values, affine = rbind(affine, c(1, 0, 0, 1))), "last row")
fails(medioImage(values, frame_start = 1:2), "one value per frame")
fails(medioImage(values, attributes = list(1)), "named list")
fails(writeMedio(image, tempfile(fileext = ".v"), overwrite = NA), "TRUE or FALSE")
