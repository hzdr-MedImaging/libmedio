# Rmedio — R interface to libmedio

`Rmedio` makes libmedio available in R. `readMedio()` and `writeMedio()`
read and write images of all formats of libmedio, like `pmedio.load()` and
`pmedio.save()` of the Python bindings:

```r
library(Rmedio)
image <- readMedio("image.v", orientation = "RAS")  # ECAT7, NIfTI, Philips, microPET
image$data                           # physical values, 4D array (x, y, z, frame)
image$affine                         # 3 x 4, voxel (i, j, k) counted from 0 -> LPS (mm)
image$frame_start                    # s, also frame_duration
image$attributes$PatientWeight       # header attributes (named list)
image$data <- image$data * 2
writeMedio(image, "image.nii.gz")    # format from the file name (.v, .nii, .nii.gz, .hdr, .img, .syn)
writeMedio(image, "image.v", like = "source.v", overwrite = TRUE)  # main header of a source file
```

- `readMedio(fname, frames = c(3, 1), orientation = ...)` selects frames by
  their numbers and reorders the voxels (default: the voxel order of the
  file). All frames need the same geometry.
- `medioImage(data, affine, frame_start, frame_duration, attributes)` makes
  an image from an array; `writeMedio()` also takes a plain 3D/4D array
  (identity affine). Double data is stored as float32, integer data as
  int32. Invalid images or attributes create no file, and no file is left
  behind if writing fails.

`readEcat()` and `writeEcat()` are the older interface for ECAT files only,
kept for existing scripts:

```r
ecat <- readEcat("image.v")          # list of 3D arrays (rows, cols, planes), class "Ecat"
names(ecat)                          # "f1", "f2", ... (frames), "g1", ... (gates), "b1", ... (beds)
attr(ecat, "mhead")                  # main header
attr(ecat[["f1"]], "shead")          # sub header of frame 1
writeEcat(ecat, fname = "copy.v")    # ECAT7 file
```

`readEcat(f)[[k]]` holds the same values as
`aperm(readMedio(f)$data[, , , k], c(2, 1, 3))`: the voxel order of the
file, as rows (y) x columns (x) x planes.

## How the package is built

The R package is self-contained: it does not link against an installed
libmedio but compiles the libmedio sources together with the R interface,
using the compiler and flags of R. This is what CRAN and other R
repositories require.

- `tools/vendor.sh` copies the libmedio sources of this repository
  (`../src`) into `src/libmedio/` and a few test datasets
  (`../testsuite/data`, ECAT7, NIfTI and Philips) into `inst/extdata/`. These copies are not
  tracked in git (see `.gitignore`), they only end up in the source tarball.
- `configure` (`configure.ucrt` on Windows) runs `tools/vendor.sh` if the
  sources are not bundled yet, finds Qt5 (Core) and zlib with `pkg-config`
  and generates `src/Makevars` (`src/Makevars.ucrt`) from `src/Makevars.in`.
- librtdebug is not needed: `src/rtdebug/rtdebug.h` replaces its header
  with the no-op macros of a release build.

Requirements: R with the packages `Rcpp` (and `jsonlite` for the tests),
a C++ compiler, `pkg-config`, Qt5 (Core) and zlib, e.g. on Debian/Ubuntu

```bash
sudo apt-get install r-base-dev r-cran-rcpp r-cran-jsonlite qtbase5-dev zlib1g-dev pkg-config
```

On macOS with Homebrew (`brew install qt@5 pkg-config`), make Qt visible to
`pkg-config` first:

```bash
export PKG_CONFIG_PATH="$(brew --prefix qt@5)/lib/pkgconfig"
```

`QT_CFLAGS`/`QT_LIBS` and `ZLIB_CFLAGS`/`ZLIB_LIBS` override the detection.

With `LIBMEDIO_PREFIX`, the package is compiled against an installed libmedio
(`<prefix>/include/medio`, `<prefix>/lib/libmedio`) instead of the bundled
sources, e.g. for the conda package `r-rmedio` (recipe in `conda.recipe/`,
see [conda.recipe](../conda.recipe/README.md)). Only the test data is bundled
then (`tools/vendor.sh --data`):

```bash
LIBMEDIO_PREFIX=/usr/local/petlib R CMD INSTALL r
```

The package has the version of libmedio.

### Install from a GitHub release

Every [GitHub release](https://github.com/hzdr-MedImaging/libmedio/releases)
has the source package `Rmedio_<version>.tar.gz` with the bundled libmedio
sources, built and checked (`R CMD check --as-cran`) by the workflow
`.github/workflows/r-package.yml`. It is installed without the repository; R
compiles it, so it needs the requirements above (a C++ compiler, Qt5 and zlib
with their headers):

```r
install.packages("https://github.com/hzdr-MedImaging/libmedio/releases/download/<version>/Rmedio_<version>.tar.gz",
                 repos = NULL, type = "source")
```

The releases also have the compiled conda package `r-rmedio` for Linux and
macOS, which needs no compiler (see [conda.recipe](../conda.recipe/README.md)).

### Install from the git checkout

Run from the top directory of the libmedio repository:

```bash
R CMD INSTALL r
```

### Build and check the source tarball

```bash
r/tools/vendor.sh
R CMD build r
R CMD check --as-cran Rmedio_*.tar.gz
```

`R CMD build` packs the bundled sources into `Rmedio_<version>.tar.gz`, which
can be installed without the libmedio repository (`R CMD INSTALL
Rmedio_<version>.tar.gz`) and is what gets submitted to CRAN or other
repositories. Run `tools/vendor.sh` again whenever the libmedio sources
change; `tools/vendor.sh --clean` removes the bundled copies.

## License

Apache License 2.0, like libmedio (see `inst/COPYRIGHTS`). Binary packages
link Qt, see the notes on Qt licensing in the libmedio README.
