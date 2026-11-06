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
- **NIfTI-1 / NIfTI-2** single files (`.nii`, gzip-compressed `.nii.gz`),
  both byte orders, incl. JSON header extensions
- **Philips PET** image data, including **Syntegra**-related variants
- **microPET** (Siemens Concorde)

> Additional containers may be supported over time. Contributions and test
> data (de-identified) are welcome.

## Features
- Object-oriented C++ API (header-only interfaces with a shared library core)
- Read/write support for PET images and sinograms (ECAT6/7, Philips IMG, microPET, etc.)
- Robust metadata handling and endian-safe I/O
- Conversion of ECAT7 images to NIfTI and back (`CNIFTIConverter`), incl.
  dynamic (4D) data, patient orientation/geometry, physical units and the
  ECAT7 metadata (stored as JSON header extension)
- Cross-platform build with CMake; Qt5 backend

## Requirements
- **C++** compiler (GCC/Clang/MSVC)
- **CMake ≥ 3.20**
- **Qt 5.15+** (at least `Core`; additional modules as needed)
- **librtdebug**
- **zlib** (for `.nii.gz`)

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

## Tests
The test suite uses QtTest and is run with CTest after building:
```bash
ctest --test-dir build --output-on-failure
```
Test datasets in several formats (DICOM, ECAT7, NIfTI, Philips, Interfile,
raw) are stored in `testsuite/data`; see
[testsuite/data/README.md](testsuite/data/README.md) for their layout and how
to add data.

## ECAT7 ↔ NIfTI conversion
The command line tools `ecat2nifti` and `nifti2ecat` convert complete files:
```bash
ecat2nifti [-2] [--float] [--sidecar] [--no-extension] input.v [output.nii[.gz]]
nifti2ecat input.nii[.gz] [output.v]
```
- NIfTI files hold the physical values (stored value × scale factor ×
  calibration factor, e.g. Bq/ml); dynamic data with a scale factor per frame
  (or `--float`) is written as float32.
- ECAT7 voxel (i, j, k) is written as NIfTI voxel (i, j, nz-1-k) with qform
  and sform describing its position. NIfTI files of any orientation are
  reordered into the ECAT7 voxel order.
- The ECAT7 main header values (incl. patient data) and the frame timing are
  stored as JSON header extension (`--sidecar`: also as `<output>.json`) and
  restored by `nifti2ecat`.

The details are documented in `src/nifti/CNIFTIConverter.h`. NIfTI
header/image pairs (`.hdr`/`.img`) are not supported yet. The NIfTI support
was started in the 2026 summer student project, see
[docs/summer-student-2026](docs/summer-student-2026/README.md).

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
