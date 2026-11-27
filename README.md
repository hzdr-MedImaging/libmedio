# libmedio

**Read and write medical imaging data in C++, Python and R: ECAT, NIfTI, Philips PET, Interfile and microPET through one interface.**

[![CI](https://github.com/hzdr-MedImaging/libmedio/actions/workflows/ci.yml/badge.svg)](https://github.com/hzdr-MedImaging/libmedio/actions/workflows/ci.yml)
[![License: Apache-2.0](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)

libmedio is a portable C++/Qt5 library for the file formats of nuclear
medicine, developed for PET research at the Helmholtz-Zentrum
Dresden-Rossendorf (HZDR). It reads every supported format into one common
representation (voxels, scaling, geometry, frame timing and header
attributes) and writes it into any writable format, so applications,
pipelines and conversions do not have to know the formats they work with.

## Highlights

- **One interface for all formats**: `CMedIOData::readVolume()` and
  `writeVolume()` read and write the frames of any format; a file can also be
  read in the representation (voxel order, data type, header) of another
  format.
- **Geometry kept exactly**: every voxel keeps its position in DICOM patient
  coordinates when files are converted or reoriented, also between formats
  with different voxel orders.
- **Header attributes with DICOM keywords**: patient, isotope, dose and
  acquisition data of every format as one JSON object (e.g.
  `"PatientWeight"` in kg), carried along by all conversions; NIfTI files
  keep them in JSON and DICOM header extensions readable by nibabel, PMOD
  and BIDS tools.
- **Python and R** bindings with the same format independent interface:
  numpy arrays and nibabel images in Python, arrays and lists in R.
- **Safe writing**: everything is checked before a file is created; files
  that cannot be written completely are removed, existing files are only
  replaced on request.
- **Validated**: the test data is checked against independent
  implementations (nibabel, numpy, pydicom); the CI builds and tests the
  library with GCC and Clang, the Python bindings and the R package.

## Supported formats

| Format | Files | All languages: images | C++ format classes |
| --- | --- | --- | --- |
| ECAT 7 (Siemens/CTI) | `.v` | read, write | images, sinograms, normalizations, attenuation, polar maps |
| ECAT 6 | `.v` | | main headers |
| NIfTI-1, NIfTI-2 | `.nii`, `.nii.gz`, `.hdr`/`.img` (also `.gz`) | read, write (NIfTI-1) | both byte orders, JSON and DICOM header extensions |
| Philips PET | `.img` (image), `.syn` (Syntegra) | read, write | images, sinograms, list mode |
| Interfile (3.3, STIR, PMOD, Siemens reconstructions) | header (`.hdr`, `.hv`, `.h33`, Siemens `.mhdr`/`.v.hdr`) with data file or single file | read, write (`.hv`/`.v` as STIR) | images ([docs/interfile.md](docs/interfile.md)) |
| Siemens Concorde microPET | `.img` + `.img.hdr` | read | images, sinograms |
| HDF5 (layout of libmedio, [docs/hdf5.md](docs/hdf5.md); optional) | `.h5`, `.hdf5`, `.hdf` | read (C++, Python) | images with frames, gates and bed positions |

"All languages" is the format independent interface of C++, Python and R;
the C++ format classes (`CECATFile`, `CNIFTIFile`, `CPhilipsFile`,
`CInterfileFile`, `CConcordeFile`) give access to the native headers and the other data types.

## Quick start

The same task in all three languages: read an ECAT7 image, look at its header
attributes and write it as compressed NIfTI file.

**C++**

```cpp
#include <CMedIOData>
#include <CMedIODataFactory>
#include <CMedIOHeader>
#include <CMedIOVolume>

#include <QDebug>
#include <QJsonObject>
#include <QScopedPointer>

int main()
{
  // any supported format: the format is identified from the content of the file
  QScopedPointer<CMedIOData> in(CMedIODataFactory::create("pet.v"));
  in->open(QIODevice::ReadOnly);

  QJsonObject attributes;                                  // DICOM keywords and units
  in->readAttributes(attributes);
  qDebug() << attributes.value("PatientWeight").toDouble(); // kg

  // the format of the new file from its name, the main header converted from the source
  QScopedPointer<CMedIOData> out(CMedIODataFactory::createForWriting("pet.nii.gz"));
  CMedIOHeader* header = NULL;
  in->readMainHeader(header, out->dataFormat());
  QScopedPointer<CMedIOHeader> headerGuard(header);
  out->open(QIODevice::WriteOnly);
  out->writeMainHeader(*header);

  const QList<short> frames = in->frameNumbers();
  for(int k = 0; k < frames.size(); k++) {
    CMedIOVolume volume;                                   // voxels, scaling, geometry, timing
    in->readVolume(volume, frames[k]);
    out->writeVolume(volume, k + 1);
  }
  out->close();
  return 0;
}
```

**Python** (`pmedio`)

```python
import pmedio

image = pmedio.load("pet.v", orientation="RAS")  # all frames: 4D numpy array, affine, timing
print(image.shape, image.attributes["PatientWeight"])
image.data = image.data / 1000.0                  # Bq/ml -> kBq/ml
image.save("pet.nii.gz")                          # format from the file name

nifti = image.to_nibabel()                        # or continue with nibabel
```

**R** (`Rmedio`)

```r
library(Rmedio)

image <- readMedio("pet.v", orientation = "RAS")  # 4D array, affine, timing, attributes
dim(image$data)
image$attributes$PatientWeight
image$data <- image$data / 1000
writeMedio(image, "pet.nii.gz")
```

Command line tools convert complete files between ECAT7 or Philips and NIfTI
with all metadata, e.g. `ecat2nifti pet.v pet.nii.gz` and
`nifti2ecat pet.nii.gz pet.v` (also `philips2nifti` and `nifti2philips`; each
pair is one program that selects the direction by the name it is called with.
They are installed with the version of libmedio as suffix, e.g.
`ecat2nifti-<version>`, and with a link without version to the version
installed last, see [NIfTI](docs/nifti.md)).

## Installation

### conda packages

The [GitHub releases](https://github.com/hzdr-MedImaging/libmedio/releases)
have conda packages of the library and the command line tools (`libmedio`),
the Python package (`pmedio`) and the R package (`r-rmedio`) for Linux
(`linux-64`) and macOS (`osx-arm64`), built with the packages of conda-forge,
as the archive of a local conda channel:

```bash
tar -xzf libmedio-<version>-conda-linux-64.tar.gz
conda install -c "file://$PWD/libmedio-<version>-conda-linux-64" -c conda-forge libmedio pmedio r-rmedio
```

Only the packages needed have to be named, e.g. `pmedio` alone. See
[conda.recipe](conda.recipe/README.md#pre-built-packages-of-the-github-releases).
Without conda, the R package is installed from its source package
`Rmedio_<version>.tar.gz` of the release, which R compiles (it needs a C++
compiler and Qt5 and zlib with their headers, see
[r/README.md](r/README.md#install-from-a-github-release)); `<version>` is the
tag of the release:

```bash
Rscript -e 'install.packages("https://github.com/hzdr-MedImaging/libmedio/releases/download/<version>/Rmedio_<version>.tar.gz", repos = NULL, type = "source")'
```

### From source

libmedio needs a C++11 compiler, CMake 3.20 or newer, Qt 5 (Core), zlib and
[librtdebug](https://github.com/hzdr-MedImaging/librtdebug). `PREFIX_PATH`
(default `/usr/local/petlib`) is where the dependencies are searched and
libmedio is installed:

```bash
# Debian/Ubuntu
sudo apt-get install cmake g++ qtbase5-dev zlib1g-dev pkg-config

# librtdebug
git clone https://github.com/hzdr-MedImaging/librtdebug.git
cmake -S librtdebug -B build-rtdebug -DPREFIX_PATH=/usr/local/petlib
cmake --build build-rtdebug -j && cmake --install build-rtdebug

# libmedio
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DPREFIX_PATH=/usr/local/petlib
cmake --build build -j && cmake --install build
```

The command line tools are installed with their version in the name (e.g.
`bin/ecat2nifti-<version>`), so that several versions can be installed side by
side, and with a link without version (`bin/ecat2nifti`) to the version
installed last. `-DMEDIO_INSTALL_TOOL_LINKS=OFF` installs no links; Windows
builds have none.

HDF5 support is being built and optional: HDF5 files are identified in every
build (`CMedIOData::HDF5`, `CHDF5File`). With the HDF5 library, images of the
layout of [docs/hdf5.md](docs/hdf5.md) are read like the other formats; they
cannot be written yet. With the serial HDF5 library (C API, 1.10 or newer; Debian/Ubuntu:
`libhdf5-dev`) libmedio is built with it, as `CHDF5File::hasSupport()` and
`libmedio_HAS_HDF5` of the CMake package tell. `-DMEDIO_WITH_HDF5=ON` requires
the library, `OFF` builds without it; the default `AUTO` uses it if it is found
(not when cross-compiling).

### Use the installed library in C++

The installation (also the conda package `libmedio`) has a CMake package and a
pkg-config file. The CMake targets bring the include directory and Qt5Core,
which the public headers use:

```cmake
find_package(libmedio 4 REQUIRED)        # CMAKE_PREFIX_PATH=/usr/local/petlib or $CONDA_PREFIX
target_link_libraries(app PRIVATE libmedio::medio)          # shared library
# target_link_libraries(app PRIVATE libmedio::medio-static) # static library (if libmedio_HAS_STATIC)
```

```bash
# PKG_CONFIG_PATH=/usr/local/petlib/lib/pkgconfig (conda: $CONDA_PREFIX/lib/pkgconfig)
c++ -std=c++11 -fPIC app.cpp -o app $(pkg-config --cflags --libs medio)
```

Qt5 requires position independent code (`-fPIC`); the CMake targets add it.
The conda package has the shared library only.

### Python and R from source

The Python package is then built from the checkout, the R package compiles
the bundled libmedio sources itself and needs no installed libmedio:

```bash
pip install ./python                              # BUILD_PREFIX=... for another prefix
R CMD INSTALL r
```

The conda packages `libmedio`, `pmedio` and `r-rmedio` are built with the
recipes for conda-forge in `conda.recipe/`, `python/conda.recipe/` and
`r/conda.recipe/` (see [conda.recipe](conda.recipe/README.md), also for a
local build with `make conda`). Details:
[Python](docs/python.md#installation-and-build), [R](r/README.md).

## Documentation

- [Format independent access to the image data](docs/format-independent-access.md) (C++)
- [Format independent header attributes](docs/header-attributes.md)
- [Conversion to and from NIfTI](docs/nifti.md): command line tools, metadata, JSON and DICOM extensions
- [Python: pmedio](docs/python.md): `load()`/`save()`, `open()`/`create()`, nibabel, the legacy ECAT7 interface
- [R: Rmedio](r/README.md)
- The public headers in `src/` document the C++ API in detail, e.g.
  `src/CMedIOData.h`, `src/CMedIOVolume.h`, `src/CMedIOHeader.h`.

## Development

The test suite (QtTest/CTest) runs after a build with
`ctest --test-dir build --output-on-failure`; the test data and its expected
values are described in [testsuite/data/README.md](testsuite/data/README.md).
The [CI workflow](.github/workflows/ci.yml) builds and tests the library,
the Python bindings and the R package on Linux with GCC and Clang. The
conventions for contributions (code, tests, commits) are in
[AGENTS.md](AGENTS.md). The NIfTI support was started in the 2026 summer
student project, see [docs/summer-student-2026](docs/summer-student-2026/README.md).

## License

libmedio is licensed under the **Apache License, Version 2.0**, see
[LICENSE](LICENSE). ECAT, microPET, Philips and Syntegra are names or
trademarks of their respective owners and are used here for identification
only.

### Notes on Qt licensing

libmedio links against **Qt**, which is typically available under **LGPLv3**
(and other licenses). If you distribute binaries that include libmedio, you
are responsible for **LGPL compliance** for Qt (dynamic linking recommended,
include license texts, do not prohibit relinking, and provide installation
information for locked-down devices).
