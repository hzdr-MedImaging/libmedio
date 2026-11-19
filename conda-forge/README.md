# conda-forge recipes

Recipes of the conda-forge packages of libmedio, in the recipe format of
rattler-build (`recipe.yaml`). They need the package `librtdebug` (a build
dependency of libmedio), whose recipe is in the librtdebug repository
([conda.recipe/recipe.yaml](https://github.com/hzdr-MedImaging/librtdebug/tree/main/conda.recipe)):

| Recipe | Package | Source |
| --- | --- | --- |
| [libmedio](libmedio/recipe.yaml) | `libmedio`: the shared library, its headers and the command line tools | release archive of libmedio |
| [pmedio](pmedio/recipe.yaml) | `pmedio`: the Python bindings (`python/`), with the version of libmedio | release archive of libmedio |
| [r-rmedio](r-rmedio/recipe.yaml) | `r-rmedio`: the R package (`r/`), with the version of libmedio, compiled against the libmedio package (`LIBMEDIO_PREFIX`) | release archive of libmedio |

They are the source for the submission to
[conda-forge/staged-recipes](https://github.com/conda-forge/staged-recipes);
after it, every package has its own feedstock repository
(`conda-forge/<package>-feedstock`) holding its recipe. The recipes here stay
as reference and for local tests (`conda.recipe/` and `python/conda.recipe/`
build packages from the checkout for local channels).

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

## Pre-built packages of the GitHub releases

As long as the packages are not available from conda-forge, the workflow
[conda-packages.yml](../.github/workflows/conda-packages.yml) builds them with
these recipes and the variants of conda-forge for every published release and
attaches them to the release as the archive of a local conda channel per
platform: `libmedio-<version>-conda-linux-64.tar.gz` and
`libmedio-<version>-conda-osx-arm64.tar.gz`. Each holds `libmedio`, `pmedio`
(for the Python versions of conda-forge), `r-rmedio` (for its R versions) and
the `librtdebug` used for building. They are installed with the dependencies
from conda-forge:

```bash
tar -xzf libmedio-<version>-conda-linux-64.tar.gz
conda install -c "file://$PWD/libmedio-<version>-conda-linux-64" -c conda-forge libmedio pmedio r-rmedio
```

(only the packages needed, e.g. `pmedio` alone; it requires `libmedio`).

The workflow takes librtdebug from the archive attached to the librtdebug
release `RTDEBUG_VERSION` of [ci.yml](../.github/workflows/ci.yml), so that
release needs its conda packages first (workflow `conda-packages.yml` of
librtdebug). Pull requests that change the recipes or the workflow build and
test the packages without attaching them. For a release published before the
workflow existed, it is started by hand with the tag (Actions,
"conda packages", "Run workflow"; or
`gh workflow run conda-packages.yml -f tag=<version>`), which builds the
source of the tag with the recipes of the selected branch.

## Test the recipes locally

With [rattler-build](https://rattler.build), the variant configuration of
conda-forge (`conda install -c conda-forge rattler-build conda-forge-pinning`)
and a checkout of librtdebug:

```bash
conda-forge/build-local.sh /path/to/librtdebug
```

The script builds the recipes of this directory and the recipe of librtdebug
(`conda.recipe/recipe.yaml` of its checkout) with the checkouts as source
instead of the release archives (into `build-conda-forge/`). Instead of the
checkout, it also takes a conda channel with the librtdebug package, e.g. the
extracted archive of a librtdebug release, and then builds only the recipes of
this directory.

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
   copy the recipe directories of this directory into `recipes/` of a new
   branch, and `conda.recipe/recipe.yaml` of librtdebug as
   `recipes/librtdebug/recipe.yaml`, and open a pull request. The recipes
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
