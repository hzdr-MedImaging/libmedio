# libmedio — C++ I/O library for medical imaging formats

**libmedio** is a portable, C++-based shared library that provides
high-level I/O APIs to read and write image and data files commonly used in
medical imaging. Originating from PET research, libmedio focuses on
nuclear-medicine formats while remaining useful for general imaging pipelines.

The library is written in modern C++ and uses the **Qt** framework to ensure
portability across Linux, macOS, and Windows.

## Scope & supported formats
libmedio targets PET-centric workflows and currently focuses on:
- **ECAT 6 / ECAT 7** images and sinograms (Siemens/CTI)
- **Philips PET** image data, including **Syntegra**-related variants
- **microPET** (Siemens Concorde)

> Additional containers may be supported over time. Contributions and test
> data (de-identified) are welcome.

## Features
- Object-oriented C++ API (header-only interfaces with a shared library core)
- Read/write support for PET images and sinograms (ECAT6/7, Philips IMG, microPET, etc.)
- Robust metadata handling and endian-safe I/O
- Cross-platform build with CMake; Qt5 backend

## Requirements
- **C++** compiler (GCC/Clang/MSVC)
- **CMake ≥ 3.20**
- **Qt 5.15+** (at least `Core`; additional modules as needed)
- **librtdebug**

## Build & install
```bash
# Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr/local

# Build
cmake --build build -j

# Install (may require sudo)
cmake --install build
```

## License
libmedio is licensed under the **Apache License, Version 2.0.**
See `LICENSE` for the full text.

ECAT, microPET, Philips, and Syntegra are names/trademarks of their respective
owners and are used here for identification only.

### Notes on Qt licensing
libmedio is released under **Apache-2.0**. It links against **Qt**, which is
typically available under **LGPLv3** (and other licenses). If you distribute
binaries that include libmedio, you are responsible for **LGPL compliance** for
Qt (dynamic linking recommended, include license texts, do not prohibit
relinking, and provide installation information for locked-down devices).
