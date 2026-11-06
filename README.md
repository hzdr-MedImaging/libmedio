# libmedio — C++ I/O library for medical imaging formats

[![CI](https://github.com/hzdr-MedImaging/libmedio/actions/workflows/ci.yml/badge.svg)](https://github.com/hzdr-MedImaging/libmedio/actions/workflows/ci.yml)

**libmedio** is a portable, C++-based shared library that provides
high-level I/O APIs to read and write image and data files commonly used in
medical imaging. Originating from PET research, libmedio focuses on
nuclear-medicine formats while remaining useful for general imaging pipelines.

The library is written in modern C++ and uses the **Qt** framework to ensure
portability across Linux, macOS, and Windows.

## Scope & supported formats
libmedio targets PET-centric workflows and currently focuses on:
- **ECAT 6 / ECAT 7** images and sinograms (Siemens/CTI)
- **NIfTI-1 / NIfTI-2** single files (`.nii`, `.nii.gz`) and header/image
  pairs (`.hdr`/`.img`, `.hdr.gz`/`.img.gz`), both byte orders, incl. JSON
  header extensions
- **Philips PET** image data, including **Syntegra**-related variants
- **microPET** (Siemens Concorde)

> Additional containers may be supported over time. Contributions and test
> data (de-identified) are welcome.

## Features
- Object-oriented C++ API (header-only interfaces with a shared library core)
- Read/write support for PET images and sinograms (ECAT6/7, Philips IMG, microPET, etc.)
- Robust metadata handling and endian-safe I/O
- Conversion of ECAT7 and Philips images to NIfTI and back
  (`CNIFTIConverter`), incl. dynamic (4D) data, patient orientation/geometry,
  physical units and the metadata (stored as JSON header extension)
- Frame-wise access to NIfTI voxel data in the ECAT7 voxel order and data
  representation (`CNIFTIFile::readMatrix()`/`writeMatrix()`), so that
  applications working with ECAT7 data can use NIfTI files directly
- Frame-wise access to Philips image and Syntegra files in the ECAT7 data
  representation (`CPhilipsFile::readMatrix()`), so that applications can
  read ECAT7, NIfTI and Philips images as ECAT7 volumes
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

The [CI workflow](.github/workflows/ci.yml) builds libmedio on Linux with
GCC and Clang (Release and Debug) against
[librtdebug](https://github.com/hzdr-MedImaging/librtdebug), runs the test
suite, checks the installed library and the Python bindings and verifies that
the expected values of the test data are reproducible
(`testsuite/data/tools/make_expected.py --check`). The example `image2ecat`
requires the COREG library and is only built if it is found.

## Conversion to and from NIfTI
The command line tools `ecat2nifti`/`nifti2ecat` (ECAT7) and
`philips2nifti`/`nifti2philips` (Philips image `.img` and Syntegra `.syn`
files) convert complete files:
```bash
ecat2nifti    [-2] [--float] [--sidecar] [--no-extension] input.v [output.nii[.gz]|output.hdr[.gz]]
philips2nifti [-2] [--float] [--sidecar] [--no-extension] input.img|input.syn [output.nii[.gz]|output.hdr[.gz]]
nifti2ecat    input.nii[.gz]|input.hdr[.gz]|input.img[.gz] [output.v]
nifti2philips input.nii[.gz]|input.hdr[.gz]|input.img[.gz] [output.img|output.syn]
```
- NIfTI files hold the physical values (stored value × scale factor ×
  calibration factor, e.g. Bq/ml); data with a scale factor per frame or
  (Philips) per slice, or with `--float`, is written as float32.
- ECAT7 voxel (i, j, k) is written as NIfTI voxel (i, j, nz-1-k) with qform
  and sform describing its position. NIfTI files of any orientation are
  reordered into the ECAT7 voxel order.
- The main header values (incl. patient data) and the frame timing are
  stored as JSON header extension (`--sidecar`: also as `<output>.json`) and
  restored by `nifti2ecat`/`nifti2philips`.
- Philips files are converted via the ECAT7 representation of libmedio; the
  x/y image position of Philips slices is not used.

An output name ending with `.hdr`/`.img` writes a header/image pair. The
details are documented in `src/nifti/CNIFTIConverter.h`. Analyze 7.5 files
(`.hdr`/`.img` without NIfTI magic) are not supported. The NIfTI support was
started in the 2026 summer student project, see
[docs/summer-student-2026](docs/summer-student-2026/README.md).

### NIfTI data in the ECAT7 voxel order
Applications working with ECAT7 data arrays can read and write NIfTI files
frame by frame in the ECAT7 voxel order (x towards patient left, y towards
posterior, planes from superior to inferior), whatever orientation the NIfTI
file has. The ECAT7 representation of a volume consists of the voxel data in
an ECAT7 data type and a `CECAT7SubHeaderImage` (dimensions, voxel sizes,
offsets, scale factor, frame timing):
```cpp
CNIFTIFile nifti("image.nii.gz");
nifti.open(QIODevice::ReadOnly);

// numFrames: num_Frames() of CECAT7MainHeader::convertFrom(niftiMainHeader)
for(short frame=1; frame <= numFrames; frame++) {
  QByteArray* data = NULL;
  CECATSubHeader* subHeader = NULL;
  if(nifti.readMatrix(data, subHeader, frame)) {  // as CECATFile::readMatrix()
    // ... work with the ECAT7 data ...
    delete data;
    delete subHeader;
  }
}
```
- `readMatrix(data, CMedIOData::ECATOrientation, frame)` only reorders the
  voxels and keeps the NIfTI datatype, `CMedIOData::NativeOrientation` reads a
  frame in the voxel order of the file.
- `writeMatrix(data, subHeader, frame, ecatMainHeader)` writes a volume given
  in the ECAT7 representation into the datatype, scaling and orientation of
  the NIfTI main header written before (e.g. created with
  `CNIFTIMainHeader::convertFrom(ecatMainHeader, ecatSubHeader)`).
- The ECAT7 main header of a NIfTI file is obtained with
  `CECAT7MainHeader::convertFrom(niftiMainHeader)`.

The rules for data types and scaling are documented in `src/nifti/CNIFTIFile.h`.

### Philips images in the ECAT7 representation
Philips image (`.img`) and Syntegra (`.syn`) files store one sub header and
matrix per slice. `CPhilipsFile` assembles a complete frame from its slices
and returns it in the same ECAT7 representation as `CECATFile` and
`CNIFTIFile`: voxel data in an ECAT7 data type and a `CECAT7SubHeaderImage`.
```cpp
CPhilipsFile philips("image.img");
philips.open(QIODevice::ReadOnly);

for(short frame=1; frame <= philips.numFrames(); frame++) {
  QByteArray* data = NULL;
  CECATSubHeader* subHeader = NULL;
  if(philips.readMatrix(data, subHeader, frame)) {  // as CECATFile::readMatrix()
    // ... work with the ECAT7 data ...
    delete data;
    delete subHeader;
  }
}
```
- `frame` is the frame number of the Philips file as for the slice-wise
  `readMatrix()` methods (usually 1 to `numFrames()`), `-1` selects the first
  frame.
- The stored values are returned as `ByteData`, `SunShort`, `SunLong`
  (unsigned short) or `IEEEFloat` with the scale factor of the slices
  (`CPhilipsSubHeaderImage::ecat_Scale_Factor()`). If the slices of a frame
  have different scale factors, the physical values are returned as
  `IEEEFloat` with scale factor 1.
- The ECAT7 main header of a Philips file is obtained with
  `CECAT7MainHeader::convertFrom(philipsMainHeader, philipsSubHeader)` and
  the sub header of the first slice (`readSubHeader()`).

The details are documented in `src/philips/CPhilipsFile.h`.

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
