# Rmedio — R interface to libmedio

`Rmedio` makes libmedio available in R. It currently reads and writes ECAT
image volumes (`readEcat()`, `writeEcat()`).

```r
library(Rmedio)
ecat <- readEcat("image.v")          # list of 3D arrays (rows, cols, planes), class "Ecat"
names(ecat)                          # "f1", "f2", ... (frames), "g1", ... (gates), "b1", ... (beds)
attr(ecat, "mhead")                  # main header
attr(ecat[["f1"]], "shead")          # sub header of frame 1
writeEcat(ecat, fname = "copy.v")    # ECAT7 file
```

## How the package is built

The R package is self-contained: it does not link against an installed
libmedio but compiles the libmedio sources together with the R interface,
using the compiler and flags of R. This is what CRAN and other R
repositories require.

- `tools/vendor.sh` copies the libmedio sources of this repository
  (`../src`) into `src/libmedio/` and a few test datasets
  (`../testsuite/data/ecat`) into `inst/extdata/`. These copies are not
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
