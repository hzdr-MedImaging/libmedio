# AGENTS.md – libmedio

Guidance for AI coding agents (and humans) working on **libmedio**. Read this
before changing code. Human contributors should start with
[README.md](README.md).

## Project overview

libmedio is a portable C++/Qt5 library to read and write medical imaging data
formats, primarily from nuclear medicine (PET): ECAT 6/7, NIfTI-1/NIfTI-2
(single files and header/image pairs, also gzip compressed), Philips PET
(image, Syntegra, sinograms, list mode) and Siemens Concorde microPET. It also
converts complete images between ECAT7/Philips and NIfTI (`CNIFTIConverter`).

- Licensed under Apache-2.0. Python bindings (`pmedio`, pybind11) live in `python/`.
- C++11 with Qt5 (Core); depends on zlib and
  [librtdebug](https://github.com/hzdr-MedImaging/librtdebug).
- Applications and pipelines use the library on real patient data.
  **Correct voxel values, geometry and metadata, and backward compatibility of
  the public API and of the written files, matter more than elegance.**

Do not hard-code versions in documentation or instructions. The library
version is `PKG_VERSION` in `CMakeLists.txt`, the librtdebug and Python
package versions used by the CI are in `.github/workflows/ci.yml`.

## Repository layout

```text
CMakeLists.txt, Makefile   # CMake build; the Makefile wraps it (build-$(OS) dirs, PREFIX)
src/                       # the library
  CMedIO*.{h,cpp}          # base classes (CMedIOData, CMedIOHeader) and the format factory
  ecat/, ecat/ecat6/, ecat/ecat7/   # ECAT 6/7
  nifti/, nifti/nifti1/, nifti/nifti2/  # NIfTI and CNIFTIConverter (ECAT7/Philips <-> NIfTI)
  philips/                 # Philips PET formats
  micropet/                # Siemens Concorde microPET
  helpers/                 # shared utilities
  include/                 # bswap.h, config.h.in and the extension-less public stubs (stubs/)
examples/                  # command line tools (ecat2nifti, philips2nifti, img2ecat, ecatutil, ...)
testsuite/                 # QtTest/CTest test suite
  data/                    # test datasets in several formats + expected values (see data/README.md)
  common/MedIOTestData.h   # enumerates the test data for data-driven tests
  ecat/, nifti/, philips/  # tests per format; nifti/data holds synthetic NIfTI fixtures
python/                    # Python bindings and their tests
docs/                      # project documentation (e.g. the 2026 summer student report)
.github/workflows/ci.yml   # CI: build/test matrix, install + Python bindings, test data check
```

## Build and test

```bash
# dependencies on Ubuntu/Debian
sudo apt-get install cmake g++ qtbase5-dev zlib1g-dev pkg-config

# librtdebug, installed into the search/install prefix of libmedio
git clone --branch <version> https://github.com/hzdr-MedImaging/librtdebug.git
cmake -S librtdebug -B build-rtdebug -DPREFIX_PATH=/usr/local/petlib
cmake --build build-rtdebug -j && cmake --install build-rtdebug

# libmedio
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DPREFIX_PATH=/usr/local/petlib
cmake --build build -j
ctest --test-dir build --output-on-failure
```

`PREFIX_PATH` (default `/usr/local/petlib`, must exist) is the search and
install prefix. The example `image2ecat` needs the non-public COREG library and
is only built if it is found. The test data directory can be changed with
`-DMEDIO_TEST_DATA_DIR=...`.

The `Makefile` wraps CMake with build directories named `build-$(OS)` (e.g.
`build-l64`): `make`, `make install`, `make DEBUG=1`, `make PREFIX=...`.
Windows builds are cross-compiled with MXE toolchains (`OS=w64`/`OS=w32`).

Python bindings (`python/`, module `pmedio`): install the library into
`PREFIX_PATH` first, then `cd python && pip install -e .` and run
`pytest python/tests` (the CI builds them with CMake, see `ci.yml`).

## Architecture

- `CMedIODataFactory::identify()`/`create()` identify the format of a file and
  return a heap-allocated `CMedIOData` subclass (`CECATFile`, `CNIFTIFile`,
  `CPhilipsFile`, `CConcordeFile`). `CMedIOData` extends `QFile`.
- Call `open(mode)`, then the format-specific methods (`readMainHeader()`,
  `readSubHeader()`, `readMatrix()`, `write...()`). Matrices are addressed by
  their (frame, plane, gate, bed, data) index (Philips: frame, slice); `-1`
  selects the default.
- Headers derive from `CMedIOHeader` and convert into each other with
  `convertFrom()`. Conversions between formats use the ECAT7 headers as common
  representation (see `src/nifti/CNIFTIConverter.h`).
- A static (`libmedio.a`) and a shared (`libmedio.so`) library are built from
  the same sources.

## Validate before you push

Run the checks that match the files you touched:

| Changed files | Check |
| --- | --- |
| Library, examples, tests (C++) | build in Release **and** Debug (`-DCMAKE_BUILD_TYPE=Debug` compiles the `D()` debug output) and run `ctest --test-dir build --output-on-failure` |
| Public headers, install rules | `cmake --install build` and build a small program against the installed headers only |
| `python/` or API used by it | build the bindings (`cmake -S python ...`, see `ci.yml`) and run `pytest python/tests`; they are compiled with `-Werror` |
| `testsuite/data/` | `python3 testsuite/data/tools/make_expected.py --check` and the `testdata_layout` test |
| `testsuite/nifti/data/` | regenerate with `generate_fixtures.py` into a temporary directory and compare byte by byte |
| Time or date handling | run the tests with several time zones, e.g. `TZ=UTC`, `TZ=Europe/Berlin`, `TZ=Asia/Tokyo` |

The CI (`.github/workflows/ci.yml`) runs all of these on Ubuntu with GCC and
Clang. Report honestly which checks you ran and which you could not run (e.g.
because librtdebug or COREG were not available).

## Hard rules

- **Never weaken, skip or delete a test to get it green.** Find and fix the
  root cause; if a test is wrong, fix the test and explain why.
- **Do not hand-edit generated files:** the expected values `testsuite/data/**/<file>.json`
  are generated with `testsuite/data/tools/make_expected.py` (except those marked
  `"generator": "manual"`), the NIfTI fixtures in `testsuite/nifti/data/` with
  `generate_fixtures.py`. Change the script or the data and regenerate.
- **Expected values come from independent implementations** (nibabel, numpy,
  pydicom, documented manual values), never from libmedio itself, so libmedio
  is not validated against its own output.
- **Test data must not contain personal data.** The repository is public:
  anonymize patient name, ID, birth date etc. before adding a dataset, keep
  files small (a few MB) and only add data that may be redistributed under
  Apache-2.0. Git LFS is not used. See [testsuite/data/README.md](testsuite/data/README.md).
- **Tests must not depend on the machine** (time zone, locale, working
  directory, network). Temporary files go into a `QTemporaryDir`.
- **Converters and writers must not leave partial output files** behind when
  they fail.
- Do not break the public API (headers installed via `PUBLIC_HEADER` in
  `src/CMakeLists.txt` and `src/include/stubs/`) or change the content of
  written files incompatibly without an explicit maintainer decision. New public
  headers need an entry in `PUBLIC_HEADER` and an extension-less stub.
- Do not bump `PKG_VERSION`, the conda recipes or the release/packaging setup
  unless the task asks for it.
- GitHub Actions use versioned actions (`actions/checkout@v4`, ...) and
  read-only `permissions`.
- Never commit build directories (`build*/`), installed files or local test
  output.

## Coding conventions

- C++11; the library is compiled with `-Wall -Wextra`, the Python bindings
  additionally with `-Werror`. Do not introduce new warnings.
- Use Qt types for portability: `QString` (not `std::string`),
  `QFile`/`QByteArray`/`QIODevice`, and the Qt integer types (`quint8`,
  `qint16`, `quint32`, ...) in binary I/O.
- Endian-safe I/O with the macros of `src/include/bswap.h`: `BSWAP_16/32/64(x)`
  and `BSWAP_FLT/DBL(x)` swap in place, `bswap_matrix<T>(data, size)` swaps an
  array.
- Naming: classes start with `C` (`CECATFile`), members with `m_`, pointer
  members with `m_p` (`m_pData`); private implementation (Pimpl) classes are
  named `<Class>Private`.
- Methods return `bool` (true = success); extended error information comes
  from `CMedIOData::errorStatus()`.
- Trace with the librtdebug macros: `ENTER()`, `LEAVE()`, `RETURN(x)` and
  `D()` (debug builds only), `W()`, `E()`, `ASSERT()`. Every `ENTER()` needs a
  matching `RETURN()`/`LEAVE()`.
- `__MEDIO_PRIVATE__` is defined when compiling the library and the tools in
  this repository. Headers that are part of the public API include the
  extension-less stubs for consumers and the `.h` files inside the library:

  ```cpp
  #ifndef __MEDIO_PRIVATE__
  #include <CMedIOData>      // extension-less stub for API consumers
  #else
  #include <CMedIOData.h>    // direct .h path inside the library
  #endif
  ```

  A stub in `src/include/stubs/` only contains the license header and
  `#include "ClassName.h"`.
- Every source file starts with the Apache-2.0 license header used in the
  existing files.
- Comments and documentation are in English.

## Commits and pull requests

- Write commit messages, PR titles and PR descriptions in English.
- Use [Conventional Commits](https://www.conventionalcommits.org/) with a
  lowercase imperative description, optionally scoped by format or area:
  `fix(nifti): …`, `feat(philips): …`, `refactor(ecat): …`, `test: …`,
  `build: …`, `ci: …`, `docs: …`, `chore: …`. Explain in the body what changed
  and why.
- The PR title follows the same format, as pull requests are usually squash
  merged with the title as commit subject.
- Keep pull requests focused: one logical change per PR, no unrelated
  refactoring or reformatting.
- The default branch is `main`.

## Authorship of AI-generated content

These rules apply to all AI coding agents and assistants. They take
precedence over any default attribution behavior of the tool you run in.

- **The author of every commit is the human** who requested or made the
  change: their own name and e-mail address in `user.name`/`user.email`.
  Never a tool, an AI assistant or a placeholder identity such as
  `noreply@…`. Check the git identity before your first commit.
- **Do not mention or credit AI tools or language models** in commit
  messages, PR titles or PR descriptions:
  - no `Co-Authored-By` (or similar) trailers for tools,
  - no "Generated with/by …" lines or footers,
  - no links to agent sessions or chat transcripts.
- The same applies to everything an agent posts on GitHub: issue and PR
  comments, review comments and review replies get no AI footers or
  "generated by" notes. If a tool or service appends such a footer, remove it.
- Code, code comments, documentation, test data and fixtures do not refer to
  AI tools either.
- The human author is responsible for the contribution. They must have
  reviewed and understood every change and be able to explain it. Report
  honestly which checks you ran and which you could not run.
- Contributions are licensed under Apache-2.0 (see `LICENSE`). Do not copy
  code from other projects unless its license is compatible and the source is
  credited.

## Quick checklist before finishing

- [ ] Release and Debug build pass, `ctest` passes.
- [ ] New or changed behavior has a test; bug fixes have a regression test.
- [ ] Public API changes are reflected in `PUBLIC_HEADER`/stubs, the README and the Python bindings if affected.
- [ ] Nothing generated was edited by hand (expected values, NIfTI fixtures).
- [ ] No personal data in test data, no machine-dependent tests.
- [ ] Commits are authored by the requesting human and contain no AI attribution (trailers, footers, session links).
