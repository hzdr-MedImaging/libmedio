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
- Format independent access to image matrices: every image file can be read
  in the matrix representation of another format (voxel order, data type,
  scaling, header) and matrices of any format can be written into files of
  another format (`CMedIOData::readMatrix()`/`writeMatrix()`, `CMedIOVolume`)
- Format independent header attributes: every header describes its attributes as JSON
  object with DICOM keywords as keys (`CMedIOHeader::attributes()`,
  `CMedIOHeaderDictionary`), carried along by all conversions between the formats
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
- The attributes of the source (see [Format independent header attributes](#format-independent-header-attributes),
  incl. patient data and all native header fields) and the frame timing are
  stored as JSON header extension (`--sidecar`: also as `<output>.json`) and
  restored by `nifti2ecat`/`nifti2philips`. JSON written by earlier versions
  (ECAT7 fields at the top level) is still read.
- Philips files are converted via the ECAT7 representation of libmedio; the
  x/y image position of Philips slices is not used.

An output name ending with `.hdr`/`.img` writes a header/image pair. The
details are documented in `src/nifti/CNIFTIConverter.h`. Analyze 7.5 files
(`.hdr`/`.img` without NIfTI magic) are not supported. The NIfTI support was
started in the 2026 summer student project, see
[docs/summer-student-2026](docs/summer-student-2026/README.md).

## Format independent matrix access
Applications do not need to know the format of a file to work with its
image data in the representation they are used to. `readMatrix()` of every
file class (`CECATFile`, `CNIFTIFile`, `CPhilipsFile`, `CConcordeFile`)
returns a frame in the representation of the requested format:
```cpp
// any supported image file, e.g. NIfTI, Philips or microPET
CMedIOData* file = CMedIODataFactory::create(fileName);
file->open(QIODevice::ReadOnly);

CMedIOHeader* mainHeader = NULL;                   // here: CECAT7MainHeader
file->readMainHeader(mainHeader, CMedIOData::ECAT);

foreach(short frame, file->frameNumbers()) {       // frame numbers of the file
  QByteArray* data = NULL;
  CMedIOHeader* subHeader = NULL;                  // here: CECAT7SubHeaderImage
  if(file->readMatrix(data, subHeader, CMedIOData::ECAT, frame)) {
    // ... ECAT7 voxel order, data type and scale factor ...
    delete data;
    delete subHeader;
  }
}
```
and `writeMatrix()` writes a frame given in the representation of any
format, e.g. ECAT7 data into a NIfTI file:
```cpp
CECATMainHeader* ecatMainHeader = NULL;            // bed position, calibration factor
ecatFile.readMainHeader(ecatMainHeader);
CMedIOHeader* niftiHeader = NULL;                  // NIfTI main header of the ECAT7 file
ecatFile.readMainHeader(niftiHeader, CMedIOData::NIFTI);

CNIFTIFile nifti("image.nii.gz", CNIFTIMainHeader::NIFTI1);
nifti.open(QIODevice::WriteOnly);
niftiHeader->setMedIOData(&nifti);                 // returned headers are not linked to a file
nifti.writeMainHeader(*static_cast<CNIFTIMainHeader*>(niftiHeader));

const QList<short> frames = ecatFile.frameNumbers();
for(int k=0; k < frames.size(); k++) {
  QByteArray* data = NULL;
  CECATSubHeader* subHeader = NULL;
  ecatFile.readMatrix(data, subHeader, frames[k]); // ECAT7 matrix
  nifti.writeMatrix(*data, *subHeader, k + 1, ecatMainHeader); // NIfTI volume k+1
  delete data;
  delete subHeader;
}
```
- `frameNumbers()` returns the numbers of the frames of a file. ECAT7 and
  Philips files address their frames by the frame numbers stored in the
  file, which do not have to start with 1 and can have gaps (`numFrames()`
  of `CECATFile`/`CPhilipsFile` is the highest frame number); NIfTI volumes
  and microPET frames are numbered 1..n. The main headers returned for
  another format (and the main headers written by libmedio) give the number
  of frames.
- Every frame is converted via its ECAT7 representation, using the existing
  header conversions (`convertFrom()`); the geometry of every voxel is kept.
- Voxel order: ECAT7, Philips and microPET: x towards patient left, y
  towards posterior, planes from superior to inferior; NIfTI: the same
  with reversed planes (as written by `CNIFTIConverter`), or the
  orientation of the NIfTI file written to, whatever orientation it has.
- Data types: the stored values are kept if the target format can represent
  them exactly (integers are widened if needed), otherwise the physical
  values are stored as float32. Writing into a file with a given data type
  (NIfTI) converts the values into its data type and scaling and fails
  without writing if a value does not fit.
- Philips matrices hold all slices of a frame with one scale factor;
  Philips frames can be written in any order and overwritten with data of
  the same slices. microPET files can only be read.
- Without a format, `readMainHeader(header)` and `readMatrix(data, header, frame)`
  return the main header and frames in the format of the file itself, e.g.
  for a file created by `CMedIODataFactory` whose format is not known.
- `writeMatrix()` takes the format of the matrix from the type of its sub
  header. Like `readMatrix()`, it can also be given explicitly:
  `writeMatrix(data, header, CMedIOData::ECAT, frame, mainHeader)` fails
  without writing if the sub header does not belong to that format. The main
  header passed with the matrix has to be a main header of the format of the
  matrix as well, otherwise nothing is written.
- `readVolume()`/`writeVolume()` exchange frames as `CMedIOVolume`: voxel
  data, scaling, the position of every voxel (DICOM patient coordinates)
  and the frame timing; `CMedIOVolume::reorient()` reorders the voxels.
- Only image data is supported (no sinograms, normalizations, list mode).

The details are documented in `src/CMedIOData.h` and `src/CMedIOMatrix.h`.

## Format independent header attributes
Every header (`CMedIOHeader`) describes its attributes as JSON object, so that
applications can use the attributes of any format without the header classes
of the formats:
```json
{
  "PatientWeight": 8.63,
  "RadionuclideTotalDose": 60780000,
  "AcquisitionDateTime": "2026-06-03T17:04:56Z",
  "Units": "BQML",
  "ECAT7": { "Patient_Weight": 8.63, "Dosage": 60780000, "Data_Units": "Bq/cc" }
}
```
```cpp
CMedIOHeader* mainHeader = NULL;
file->readMainHeader(mainHeader);                     // any format
QJsonObject attributes = mainHeader->attributes();
double weight = mainHeader->attribute("PatientWeight").toDouble();   // keyword
QJsonValue dose = mainHeader->attribute("(0018,1074)");              // or DICOM tag
mainHeader->setAttribute("PatientName", QString("Doe^Jane"));
attributes["PatientWeight"] = 72.5;
mainHeader->updateAttributes(attributes);            // whole object or single keys
```
- The keys are DICOM keywords, e.g. `PatientWeight` for the DICOM attribute
  (0010,1030), with the meaning and unit of the DICOM attribute (kg, Bq, s,
  ms, ...) and a JSON value: numbers, ISO 8601 dates and times (`Z` for UTC,
  without time zone if the format has none), arrays for multiple values,
  objects for code sequences. `CMedIOHeaderDictionary` lists the keys with their
  DICOM tag (as number and as string `"(0010,1030)"`), VR, unit and the
  sequence holding them.
- The values are converted from and into the units of the formats (e.g.
  Philips weight in g, ECAT7 height in cm, microPET dose in mCi or MBq).
- The native fields of the format are in a section named after the format
  (`ECAT7`, `Philips`, `microPET`, `NIfTI`), without the fields describing the
  size, data type, scaling, geometry or layout of the image data.
- Values a format cannot store (keys of other formats, unknown keys, values
  it cannot represent) are kept in the header object and carried along by
  the conversions between the formats (`readMainHeader(header, format)`,
  `readMatrix(..., format)`, `writeMatrix()`, `CNIFTIConverter`). NIfTI files
  store all attributes in their JSON header extension, incl. the frame
  attributes of all frames (`Frames`) and BIDS keys for other tools.
- The other formats store only their own fields. The physical values of
  Philips slices scaled to SUV depend on the dose, times, half life and
  weight of the main header: change them before the frames are written.

The details are documented in `src/CMedIOHeader.h`, `src/CMedIOHeaderDictionary.h`
and `src/CMedIOHeaderMapping.h`. In Python, `pmedio.attributes(fileName)` returns
the attributes of a file as dict and `pmedio.readVolume(fileName, frame)` a
frame as numpy array with its affine.

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
