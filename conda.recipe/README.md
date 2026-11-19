# conda recipes

The conda packages of libmedio are built with recipes for conda-forge, in the
recipe format of [rattler-build](https://rattler.build) (`recipe.yaml`), each
next to the part of the repository it packages. They need the package
`librtdebug` (a build dependency of libmedio), whose recipe is in the
librtdebug repository
([conda.recipe/recipe.yaml](https://github.com/hzdr-MedImaging/librtdebug/tree/main/conda.recipe)):

| Recipe | Package | Source |
| --- | --- | --- |
| [conda.recipe](recipe.yaml) | `libmedio`: the shared library, its headers and the command line tools | release archive of libmedio |
| [python/conda.recipe](../python/conda.recipe/recipe.yaml) | `pmedio`: the Python bindings (`python/`), with the version of libmedio | release archive of libmedio |
| [r/conda.recipe](../r/conda.recipe/recipe.yaml) | `r-rmedio`: the R package `Rmedio` (`r/`), with the version of libmedio, compiled against the libmedio package (`LIBMEDIO_PREFIX`) | release archive of libmedio |

They are the source for the submission to
[conda-forge/staged-recipes](https://github.com/conda-forge/staged-recipes);
after it, every package has its own feedstock repository
(`conda-forge/<package>-feedstock`) holding its recipe. The recipes here stay
as reference and build packages from the checkout for local tests and local
channels (`build-local.sh`). The R package is named `r-rmedio` as conda-forge
names the packages of R packages (`r-` and the name in lower case); in R it is
still `library(Rmedio)`.

Differences to the build of the repository:

- Only shared libraries are packaged (conda-forge policy); `libmedio.a` and
  `librtdebug.a` are removed after the installation. pmedio then links
  `libmedio.so`, and libmedio uses `librtdebug.so` while it is built (see
  `cmake/FindRTDEBUG.cmake`, `python/cmake/FindMEDIO.cmake`).
- Release builds of libmedio do not depend on librtdebug at run time, so the
  libmedio package does not require it.
- Windows is skipped: the Windows builds of libmedio are cross-compiled with
  MXE, not built with MSVC.
- The libmedio build runs the test suite without the `large` and
  `performance` tests; the pmedio package is tested with `pytest python/tests`,
  the r-rmedio package with the tests of `r/tests`.
- The R package is linked against the libmedio package instead of compiling
  the libmedio sources it bundles for CRAN (conda-forge does not accept
  bundled copies of libraries it ships).

## Build the packages locally

With rattler-build, the variant configuration of conda-forge
(`conda install -c conda-forge rattler-build conda-forge-pinning`) and
checkouts of libmedio and librtdebug:

```bash
conda.recipe/build-local.sh /path/to/librtdebug    # or: make conda RTDEBUG=/path/to/librtdebug
```

The script builds the three recipes and the recipe of librtdebug
(`conda.recipe/recipe.yaml` of its checkout) with the checkouts as source
instead of the release archives, into `build-conda/`, which is then a local
conda channel:

```bash
conda install -c "file://$PWD/build-conda" -c conda-forge libmedio pmedio r-rmedio
```

## Submit to conda-forge (first release)

1. Create the releases whose archives the recipes use: librtdebug with the tag
   of its version (e.g. `3.0.0`), libmedio with the tag `PKG_VERSION` of
   `CMakeLists.txt` (e.g. `4.0.0`, without `v` like the earlier tags).
   pmedio and Rmedio have the same version (`version` of
   `python/pyproject.toml`, `Version` of `r/DESCRIPTION`).
2. Set the checksums of the archives in the recipes (`sha256`):
   ```bash
   curl -sL https://github.com/hzdr-MedImaging/libmedio/archive/refs/tags/<version>.tar.gz | sha256sum
   ```
3. Fork [conda-forge/staged-recipes](https://github.com/conda-forge/staged-recipes),
   copy the recipes into a new branch as `recipes/libmedio/recipe.yaml`
   (`conda.recipe/recipe.yaml`), `recipes/pmedio/recipe.yaml`
   (`python/conda.recipe/recipe.yaml`), `recipes/r-rmedio/recipe.yaml`
   (`r/conda.recipe/recipe.yaml`) and `recipes/librtdebug/recipe.yaml`
   (`conda.recipe/recipe.yaml` of librtdebug), and open a pull request. The recipes
   depend on each other and are built together in it.
4. Follow the checks of the linter and the build of the pull request, and
   answer the reviewers. Every maintainer listed in `extra.recipe-maintainers`
   confirms in the pull request that they want to maintain the package.
5. After the merge, conda-forge creates the feedstocks; the packages are
   installed with `conda install -c conda-forge libmedio pmedio r-rmedio`.

## New releases

The bot of conda-forge (`regro-cf-autotick-bot`) finds new releases of
libmedio and librtdebug on GitHub and opens a pull request with the new
version and checksum in each feedstock; it is merged by a maintainer. Changed
dependencies or build steps are made in the feedstocks (and here or in the
librtdebug repository).
