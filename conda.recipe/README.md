## Conda build
In order to create conda package:
1. Run: `conda build -c conda-forge --use-local conda.recipe` (or `make conda`).
   It needs the conda package of librtdebug, e.g. built with its `conda.recipe`
   into the local channel before. The version of the package is `PKG_VERSION`
   of `CMakeLists.txt`; the environment variable `PROJECT_VERSION` overrides it.
   Compilers, Qt and zlib come from conda-forge, the package does not use those
   of the system.
2. Install locally with `conda install -c conda-forge --use-local libmedio` (optional)
3. Upload to conda cloud: `anaconda upload conda-bld/linux-64/libmedio-<version>-.tar.bz2`
