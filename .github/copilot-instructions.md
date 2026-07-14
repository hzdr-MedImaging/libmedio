# libmedio — Copilot Instructions

## Build

The project uses CMake, wrapped by a `Makefile` that manages build directories named `build-$(OS)` (e.g., `build-l64` for Linux 64-bit).

**Via Makefile (recommended):**
```bash
make              # auto-detects OS, configures + builds into build-l64/
make install      # installs to PREFIX (default: /usr/local/petlib)
make DEBUG=1      # debug build
make PREFIX=/usr/local  # override install prefix
```

**Via CMake directly:**
```bash
cmake -S . -B build-l64 -DPREFIX_PATH=/usr/local/petlib
cmake --build build-l64 -j
cmake --install build-l64
```

Default search and install prefix is `/usr/local/petlib`. Override with `-DPREFIX_PATH=…` or `make PREFIX=…`.

Cross-compilation for Windows uses MXE toolchains; set `OS=w64` or `OS=w32`.

**Python bindings** (`python/` → `pmedio` module):
```bash
cd python && pip install -e .
pytest python/tests/                    # run all Python tests
pytest python/tests/test_pmedio.py::test_constructors  # single test
```

The C++ library must be installed to `PREFIX_PATH` before building the Python bindings.

## Architecture

```
src/
  CMedIOData.h / .cpp          # Abstract base: extends QFile, owns file handle
  CMedIOHeader.h / .cpp        # Abstract base: header read/write interface
  CMedIODataFactory.h / .cpp   # Factory: auto-detects format, returns CMedIOData*
  ecat/                        # ECAT6 + ECAT7 implementation (CECATFile, headers, directory)
  micropet/                    # Siemens Concorde microPET (CConcordeFile, headers)
  philips/                     # Philips PET/Syntegra (CPhilipsFile, headers)
  helpers/                     # Shared utilities: CDate, CKeyParser, MedIOUnits, CIntVector
  include/
    bswap.h                    # Cross-platform byte-swap macros + bswap_matrix<T>()
    stubs/                     # Extension-less include stubs (e.g. <CMedIOData>) for API consumers
    config.h.in                # CMake-generated version info
python/
  src/pmedio.cpp               # pybind11 module entry point
  src/pmedio.h                 # MedIOImage class (numpy-compatible wrapper)
  src/ecat.cpp / ecat7.cpp     # Python bindings for ECAT types
  tests/test_pmedio.py
examples/                      # CLI tools (img2ecat, ecatutil, ecatCheck, bin2txt, …)
testsuite/                     # C++ integration tests (qmake .pro based, not CMake)
```

**Data flow:** `CMedIODataFactory::create(filename)` identifies the format and returns a heap-allocated `CMedIOData*` subclass. Call `open(mode)` to open the file, then use format-specific methods (e.g., `CECATFile::readMatrix()`, `readMainHeader()`, `readSubHeader()`). Matrices are identified by `(frame, plane, gate, bed, data)` index tuple — pass `-1` for defaults.

Both a **static** (`libmedio-static → libmedio.a`) and a **shared** (`libmedio-shared → libmedio.so`) library are built simultaneously from the same sources. Consumers link against whichever they need.

## Key Conventions

### Include guard: `__MEDIO_PRIVATE__`
Set automatically when compiling the library itself (`src/CMakeLists.txt`). Use this guard in every header that is part of both the public API and the internal build:
```cpp
#ifndef __MEDIO_PRIVATE__
#include <CMedIOData>      // extension-less stub for API consumers
#else
#include <CMedIOData.h>    // direct .h path when inside the library
#endif
```
Stub files in `src/include/stubs/` simply `#include "ClassName.h"`.

### Qt types everywhere
Use Qt types for cross-platform portability: `QString` (not `std::string`), `QFile`/`QByteArray`/`QIODevice`, and Qt integer types (`quint8`, `quint16`, `quint32`, `quint64`) in binary I/O and byte-swap code.

### Byte-swap macros (`src/include/bswap.h`)
Use the provided macros for endian-safe I/O:
- `BSWAP_16(x)`, `BSWAP_32(x)`, `BSWAP_64(x)` — swap in-place
- `BSWAP_FLT(x)`, `BSWAP_DBL(x)` — float/double swap via union
- `bswap_matrix<T>(data, size)` — swap an entire array

### rtdebug logging
`rtdebug` (external dependency, Apache-2.0: https://github.com/hzdr-MedImaging/librtdebug) provides macros used throughout:
- `D(fmt, ...)` — debug trace
- `W(fmt, ...)` — warning
- `E(fmt, ...)` — error
- `ASSERT(cond)` — assertion
- `RETURN(val)` — traced return

### Naming
- Classes: `C` prefix (e.g., `CECATFile`, `CMedIOHeader`)
- Member variables: `m_` prefix; pointer members: `m_p` prefix (e.g., `m_pData`)
- Private implementation (Pimpl): `ClassNamePrivate` (e.g., `CECATFilePrivate`, `CMedIOHeaderPrivate`)

### C++ standard
C++11 (`-std=c++11`). Library is compiled with `-Wall -Wextra`; Python bindings additionally add `-Werror`.

### Error handling
Methods return `bool` (true = success). Query extended error info via `CMedIOData::errorStatus()` returning an `errStatus` enum value.
