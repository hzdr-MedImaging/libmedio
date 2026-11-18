# conda-forge recipes

Recipes of the conda-forge packages of libmedio, in the recipe format of
rattler-build (`recipe.yaml`):

| Recipe | Package | Source |
| --- | --- | --- |
| [librtdebug](librtdebug/recipe.yaml) | `librtdebug`: build dependency of libmedio | release archive of [librtdebug](https://github.com/hzdr-MedImaging/librtdebug) |
| [libmedio](libmedio/recipe.yaml) | `libmedio`: the shared library, its headers and the command line tools | release archive of libmedio |
| [pmedio](pmedio/recipe.yaml) | `pmedio`: the Python bindings (`python/`), with the version of libmedio | release archive of libmedio |

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
  `performance` tests; the pmedio package is tested with `pytest python/tests`.

## Test the recipes locally

With [rattler-build](https://rattler.build), the variant configuration of
conda-forge (`conda install -c conda-forge rattler-build conda-forge-pinning`)
and checkouts of libmedio and librtdebug:

```bash
conda-forge/build-local.sh /path/to/librtdebug
```

The script builds all three recipes with the checkouts as source instead of
the release archives (into `build-conda-forge/`).

## Submit to conda-forge (first release)

1. Create the releases whose archives the recipes use: librtdebug with the tag
   of its version (e.g. `3.0.0`), libmedio with the tag `PKG_VERSION` of
   `CMakeLists.txt` (e.g. `4.0.0`, without `v` like the earlier tags).
   pmedio has the same version (`version` of `python/pyproject.toml`).
2. Set the checksums of the archives in the recipes (`sha256`):
   ```bash
   curl -sL https://github.com/hzdr-MedImaging/libmedio/archive/refs/tags/<version>.tar.gz | sha256sum
   ```
3. Fork [conda-forge/staged-recipes](https://github.com/conda-forge/staged-recipes),
   copy the three recipe directories into `recipes/` of a new branch and open
   a pull request. The recipes depend on each other and are built together in
   it.
4. Follow the checks of the linter and the build of the pull request, and
   answer the reviewers. Every maintainer listed in `extra.recipe-maintainers`
   confirms in the pull request that they want to maintain the package.
5. After the merge, conda-forge creates the feedstocks; the packages are
   installed with `conda install -c conda-forge libmedio pmedio`.

## New releases

The bot of conda-forge (`regro-cf-autotick-bot`) finds new releases of
libmedio and librtdebug on GitHub and opens a pull request with the new
version and checksum in each feedstock; it is merged by a maintainer. Changed
dependencies or build steps are made in the feedstocks (and here).
