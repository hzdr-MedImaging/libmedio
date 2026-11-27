# HDF5 support (design draft)

**Status: draft for discussion. libmedio reads and writes images of this layout
if it is built with the HDF5 library (`CHDF5File`, see
[Reading and writing](#reading-and-writing)). The layout may still change until
the first release of libmedio with HDF5 support.**

This document describes how libmedio is going to store PET images in HDF5
files, why it defines a layout of its own for that, and the phases of the
implementation (see [Phases](#phases)). Points that still have to be decided
or checked are collected in [Open points](#open-points).

## Why a layout of its own

HDF5 is a container: groups, n-dimensional datasets and attributes, without
any meaning attached to them. A reader can only interpret a file if both sides
agree on a layout and on the names and units of the metadata. A survey of the
existing uses of HDF5 found no published or de-facto standard for reconstructed
PET images:

| Format | Content | Why it is not the basis |
| --- | --- | --- |
| PETSIRD (ETSI) | list mode data, scanner and exam description; HDF5 is one of the serialisations generated from its yardl model | raw data only, no image type; draft without a stable version |
| GE RDF9 | list mode, sinograms, normalisation of GE scanners | raw data only, no public specification |
| MINC 2 | images of any modality with named axes (`xspace`, `yspace`, `zspace`, `time`), `acquisition` attributes for radionuclide, tracer and dose | irregular frames, but no gates and bed positions; integer voxels scaled per slice; in practice defined by the behaviour of libminc |
| ITK (`/ITKImage`) | n-dimensional image with origin, spacing and directions | geometry only: no frame timing, units or PET metadata |
| ISMRMRD | MR raw data and images | image header specific to MR |
| NeXus, netCDF/CF, SNIRF, NWB | other communities | not for PET, but good examples of how to specify a layout: name and version of the convention in the file, required and optional fields, units on every quantity |

The image side of PET uses DICOM, NIfTI with BIDS sidecars, Interfile and
ECAT. libmedio therefore defines a small, versioned layout and borrows the
names and units of the metadata from DICOM and BIDS instead of inventing
them. MINC 2 and the ITK layout are planned as additional formats for the
exchange with other tools (phase 5).

## Design principles

- **One file, one image**: a single 4D dataset with all volumes (frames,
  gates, bed positions) of an image, and tables with one row per volume.
- **Physical values**: the voxels are stored as float32 in the unit given
  with the dataset. There are no scale factors a reader could forget.
- **Borrowed vocabulary**: the metadata keys are the DICOM keywords libmedio
  already uses for its [format independent header attributes](header-attributes.md),
  with the meaning and unit of the DICOM attribute. BIDS PET names are used
  where DICOM has no fitting attribute (`FrameTimesStart`, `FrameDuration`).
- **Readable without libmedio**: every value a reader needs is a typed
  dataset or attribute, so that `h5py`, MATLAB, R or `h5dump` show it
  directly. Only conservative HDF5 features are used.
- **Lossless round trip**: the complete attributes of libmedio, including the
  native fields of the source format, are stored once more as JSON.
- **Self-describing**: the file names the layout and its version.

## Layout

Example of a dynamic study with three frames (`h5py` notation; `@` marks an
attribute):

```text
/                                    @Format = "MEDIO-IMAGE", @FormatVersion = "0.1", @Creator
/image/data                          float32 [3][47][128][128], chunked, shuffle + deflate
                                     @Units = "BQML", @units = "Bq/mL"
/image/affine                        float64 [3][4], @units = "mm"
/image/volumes/Frame                 int32   [3]
/image/volumes/FrameTimesStart       float64 [3], @units = "s"
/image/volumes/FrameDuration         float64 [3], @units = "s"
/image/volumes/FrameReferenceTime    float64 [3], @units = "ms"
/image/volumes/DecayFactor           float64 [3], @units = "1"
/header                            @PatientWeight, @RadionuclideTotalDose, @AcquisitionDateTime, ...
/header/RadionuclideCodeSequence   @CodeValue, @CodingSchemeDesignator, @CodeMeaning
/header/json                       string (JSON object)
```

"Required" in the tables below means that a file without the object is not a
valid file of this layout; "recommended" objects are written whenever the
source provides the value.

### Root group

| Attribute | Type | Level | Content |
| --- | --- | --- | --- |
| `Format` | string | required | `MEDIO-IMAGE`, identifies the layout |
| `FormatVersion` | string | required | `<major>.<minor>` of this specification. A new minor version only adds optional objects; readers refuse files with an unknown major version |
| `Creator` | string | recommended | program and version that wrote the file |

Objects that are not described here may be present; readers ignore them.

### `/image/data`: the voxels

- One dataset with the dimensions `[volume][z][y][x]` in the order of the
  HDF5 C API, i.e. x runs fastest. Volume `v` is the hyperslab `data[v]`,
  which is exactly the voxel buffer of a `CMedIOVolume`. Programs that count
  dimensions the Fortran way (MATLAB, R) see `(x, y, z, volume)`.
- Static images have one volume: the first dimension is always present.
- Data type: IEEE float32, little-endian (`H5T_IEEE_F32LE`); float64
  (`H5T_IEEE_F64LE`) if the source holds float64 values. The stored value is
  the physical value. Integer data with scale factors is converted when it is
  written.
- All volumes have the same dimensions. An image whose volumes differ in
  size cannot be written into one file. The first dimension may be extendable
  (libmedio writes a dataset that grows by the volumes written).
- Storage: chunked, with the filters shuffle and deflate. A chunk holds whole
  planes of one volume and never spans volumes; the number of planes per
  chunk is chosen so that a chunk has at most 1 MiB of raw data (at least one
  plane). A reader with the default chunk cache of the HDF5 library can then
  read plane by plane and volume by volume without decompressing more than it
  needs.

| Attribute | Type | Level | Content |
| --- | --- | --- | --- |
| `Units` | string | recommended | DICOM Units (0054,1001), e.g. `BQML`, `CNTS`, `GML`; the same value as the attribute `Units` of `/header` |
| `units` | string | recommended | the same unit in UCUM notation, e.g. `Bq/mL`, for tools that do not know the DICOM terms; `1` for values without unit |

Both attributes are present or both are absent. Without them the unit of the
values is unknown, as in files of formats or programs that do not store it
(e.g. NIfTI files of other tools); a reader must not assume one.

### `/image/affine`: the geometry

- float64 `[3][4]`: the affine transformation of the voxel index (i, j, k),
  0-based, into DICOM patient coordinates (LPS, mm): x towards the patient's
  left, y towards posterior, z towards superior. i is the index of the last
  dimension of `data` (x), k that of the plane. The position is that of the
  centre of the voxel. This is the affine of `CMedIOVolume`.
- The voxel order is not prescribed: the affine describes any orientation.
  libmedio writes the order of its own formats (x towards left, y towards
  posterior, planes from superior to inferior).
- If the volumes do not have the same position (bed positions stored as
  separate volumes), `/image/volumes/affine`, float64 `[volume][3][4]`, gives
  the affine of every volume and `/image/affine` that of the first one.

### `/image/volumes`: one row per volume

Datasets with one dimension and one value per volume, in the order of the
first dimension of `data`. Quantities carry the attribute `units` (UCUM).

| Dataset | Type | Unit | Level | Content |
| --- | --- | --- | --- | --- |
| `Frame` | int32 | | required | frame number, starting with 1; the numbers of ECAT7 and Philips files are kept, incl. gaps |
| `Gate` | int32 | | optional | gate number, starting with 1; absent: one gate |
| `Bed` | int32 | | optional | bed position, starting with 0; absent: one bed position |
| `FrameTimesStart` | float64 | s | required | start of the acquisition of the volume relative to `AcquisitionDateTime` (BIDS) |
| `FrameDuration` | float64 | s | required | duration of the acquisition of the volume (BIDS). The DICOM attribute `ActualFrameDuration` (ms) is this value × 1000 and not stored separately |
| `FrameReferenceTime` | float64 | ms | recommended | DICOM (0054,1300): the time, relative to the start of the series, at which the activity equals the average activity of the frame |
| `DecayFactor` | float64 | 1 | recommended | DICOM (0054,1321): the factor the values of the volume were multiplied with for the decay correction |
| `DeadTimeFactor` | float64 | 1 | optional | DICOM (0054,1324) |
| `TriggerTime` | float64 | ms | optional | DICOM (0018,1060), gated studies |
| `TablePosition` | float64 | mm | optional | position of the patient table the scanner reported for the volume, as DICOM Table Position (0018,9327): the bed position of ECAT7 (main header, cm × 10), `start horizontal bed position (mm)` of Interfile. Reference point and sign are those of the scanner |
| `affine` | float64 `[volume][3][4]` | mm | optional | see above |

`TablePosition` keeps the value of the scanner for display and for the
conversion back into the source format. **The position of the voxels is
given by the affine alone**; a reader must not add the table position to it.
libmedio derives the affine from the bed position when it reads a file (ECAT7:
z of the first plane = bed position + z offset of the frame), so the two
values agree in the files it writes.

Together `Frame`, `Gate` and `Bed` are the (frame, gate, bed) part of the
matrix index of libmedio; the plane is a dimension of the data.

### `/header`: attributes of the study

The attributes of the group `/header` are the keys of
`CMedIOHeaderDictionary`, written with a type that follows from the DICOM
value representation (VR):

| DICOM VR | HDF5 type | Examples |
| --- | --- | --- |
| DS, FD (decimal) | float64 | `PatientWeight` (kg), `PatientSize` (m), `RadionuclideTotalDose` (Bq), `RadionuclideHalfLife` (s), `RadionuclidePositronFraction`, `EnergyWindowLowerLimit`/`UpperLimit` (keV) |
| IS, US, UL (integer) | int32 | `SeriesNumber` |
| DT (date and time) | string, ISO 8601 with UTC offset | `AcquisitionDateTime`, `RadiopharmaceuticalStartDateTime`, `DecayCorrectionDateTime` |
| DA (date) | string `YYYY-MM-DD` | `PatientBirthDate` |
| PN, LO, SH, CS, UI, AS (text) | string | `PatientName` (`Doe^Jane`), `PatientID`, `PatientSex`, `PatientAge`, `PatientPosition`, the UIDs, `StudyDescription`, `Manufacturer`, `ManufacturerModelName`, `Radiopharmaceutical`, `Units`, `DecayCorrection`, `ReconstructionMethod`, `AttenuationCorrectionMethod`, `ScatterCorrectionMethod` |
| several values (VM > 1) | 1D array of the type | `CorrectedImage`, `SoftwareVersions`, `OperatorsName` |
| SQ (code sequence) | sub group with the string attributes `CodeValue`, `CodingSchemeDesignator`, `CodingSchemeVersion`, `CodeMeaning` | `RadionuclideCodeSequence`, `RadiopharmaceuticalCodeSequence` |

- The units are those of DICOM and are not repeated in the file (HDF5
  attributes cannot have attributes); they are listed by
  `CMedIOHeaderDictionary` and will be listed in this document for every key.
- An attribute that is not known is absent. There are no placeholder values.
- The attributes of the single frames are not in `/header` but in the
  datasets of `/image/volumes`.
- All attributes are optional for a reader that only displays the image. For
  quantitative use the following are **recommended** and written by libmedio
  whenever the source has them: `AcquisitionDateTime`, `DecayCorrection`,
  `DecayCorrectionDateTime`, `CorrectedImage`, `RadionuclideCodeSequence`,
  `RadionuclideHalfLife`, `RadionuclideTotalDose`,
  `RadiopharmaceuticalStartDateTime` and `PatientWeight`. With `Units` and the
  frame timing these are the values a standardised uptake value (SUV) is
  computed from.

`/header/json` is a scalar string dataset with the complete attribute
object of libmedio as JSON, as returned by `CMedIOData::readAttributes()`:
the keys above, the frame attributes (`Frames`), the native fields of the
source format (`ECAT7`, `Philips`, `microPET`, `NIfTI`, ...) and keys libmedio
does not know. It makes the conversion into HDF5 and back lossless. **If a
typed attribute or dataset and the JSON disagree, the typed value is valid**,
as these are the values other tools show and change; keys that only exist in
the JSON are taken from there. The dataset is optional.

### Times

- Dates and times are absolute and carry their UTC offset
  (`2026-06-03T17:04:56+02:00`, or `Z`). A time without offset is only
  written if the source format has no time zone; it is then the local time
  of the scanner.
- `AcquisitionDateTime` is the start of the acquisition and the origin of
  `FrameTimesStart`.
- `RadiopharmaceuticalStartDateTime` is the start of the administration.
- `DecayCorrectionDateTime` is the time the values are decay corrected to,
  written explicitly even if it equals one of the other two (as
  `DecayCorrection` = `START` or `ADMIN` says). The time the decay correction
  refers to is the most common source of wrong SUVs when PET data is
  converted, so a reader must not have to guess it.
- Relative times of other vocabularies (`InjectionStart`, `ScanStart`,
  `TimeZero` of BIDS) are derived from these when exporting, not stored.

### Encoding rules

- Strings are variable-length UTF-8 strings (`H5T_VARIABLE`,
  `H5T_CSET_UTF8`). Readers also accept fixed-length and ASCII strings, as
  some tools write them.
- Scalars have a scalar dataspace (not an array with one element).
- Numbers are stored little-endian. Readers accept what the HDF5 library
  converts (other byte orders, integers for the numbers of `/image/volumes`).
- No compound types, references, external links, virtual datasets, user
  block, SWMR and no filters other than shuffle and deflate, so that every
  program built with any HDF5 library since 1.8 can read the files. For this
  libmedio sets the bounds of the file format version explicitly
  (`H5Pset_libver_bounds()`, lowest format).
- File name extensions: `.h5`, `.hdf5` and `.hdf`; libmedio writes the name it
  is given. Files are identified by their content, not by their name.
  Compressed files (`.h5.gz`) are not supported; the compression is part of
  the dataset.

### Reading a file without libmedio

```python
import json
import h5py

with h5py.File("image.h5", "r") as f:
    assert f.attrs["Format"] == "MEDIO-IMAGE"
    data = f["image/data"]                       # [volume][z][y][x], physical values
    frame2 = data[1]                             # numpy array (z, y, x) of the second volume
    unit = data.attrs["units"]                   # "Bq/mL"
    affine = f["image/affine"][:]                # voxel (i, j, k) -> LPS (mm)
    start = f["image/volumes/FrameTimesStart"][:]
    duration = f["image/volumes/FrameDuration"][:]
    weight = f["header"].attrs["PatientWeight"]
    everything = json.loads(f["header/json"][()])
```

## Reading and writing

libmedio reads the layout as follows (`CHDF5File`, `src/hdf5/`):

- A file is opened if its root attribute `Format` is `MEDIO-IMAGE` and the
  major version of `FormatVersion` is 0. A file that violates the layout is
  refused, with the reason in `CHDF5File::openError()`; objects that are not
  part of the layout are ignored.
- The format independent interface (`frameNumbers()`, `readVolume()`,
  `readMatrix()`, `readAttributes()`, in Python `pmedio.open()` and
  `pmedio.load()`) gives the frames of the first gate and bed position of the
  file. `CHDF5File::numVolumes()`, `readVolumeHeader()` and
  `readVolumeMatrix()` give every volume with its frame, gate and bed
  position (`CHDF5FrameHeader`).
- The voxels are returned in the order of the file as float32 or float64 with
  the affine of the volume; other formats get them reordered as usual.
- The attributes of the main header are the typed attributes of `/header` and
  the keys of `/header/json` without a typed attribute. The attributes of a
  volume are `ActualFrameDuration` (from `FrameDuration`), the values of the
  further datasets of `/image/volumes` and the keys of the entry of the
  volume in `Frames` of the JSON object without a dataset.
- All calls into the HDF5 library are serialised by one lock, and the error
  handler of the application is not changed.

and writes it like this:

- A new file is written like the files of the other formats: the main header
  with the attributes, then the volumes (`writeMainHeader()`, `writeVolume()`
  or `writeMatrix()`, in Python `pmedio.create()` and `pmedio.save()`). File
  names ending with `.h5`, `.hdf5` or `.hdf` are HDF5 files.
- The values are the physical values as float32 (float64 for float64 data);
  integers with scale factors are converted. The format independent
  interface writes the voxels in the order of ECAT7 (x towards left, y
  towards posterior, planes from superior to inferior).
- All volumes have the dimensions and data type of the first one. The file
  takes any number of volumes; `CHDF5File::writeMatrix()` also takes gates and
  bed positions. `Gate`, `Bed` and `/image/volumes/affine` are only written if
  the volumes differ in them.
- The attributes of the dictionary are typed attributes of `/header`, all
  attributes are in `/header/json`. A value every volume has (`DecayFactor`,
  `DeadTimeFactor`, `FrameReferenceTime`, `TriggerTime`, `TablePosition`) is a
  dataset of `/image/volumes`.
- `Units` and `units` are written for the terms libmedio knows in UCUM
  notation (`BQML`, `KBQCC`, `CNTS`, `CPS`, `NONE`, `CM2`, `PCNT`, `GML`); other
  terms are only in `/header`.
- The file is written under a temporary name in its directory and gets its
  name when it is closed with its main header and at least one volume. Until
  then, and if writing fails, there is no file of that name, and an existing
  one is left as it is.
- The `QFile` of a file opened for writing is not opened (that would create or
  truncate the file of that name); only its open state is set.

## Identification

1. An HDF5 file starts with the signature `\x89HDF\r\n\x1a\n`, at offset 0 or,
   after a user block, at offset 512, 1024, 2048, ... This is checked without
   the HDF5 library, so that a build without HDF5 support can still report
   "HDF5 file, but this build has no HDF5 support".
2. The root attribute `Format` identifies the layout. Other HDF5 files are
   reported as HDF5 files of an unknown layout, until phase 5 adds the
   layouts of other programs (group `/ITKImage`: ITK, group `/minc-2.0`:
   MINC 2).

## Implementation

- HDF5 is an **optional** dependency: the CMake option `MEDIO_WITH_HDF5`
  (`AUTO`, `ON`, `OFF`) and the define `MEDIO_HAVE_HDF5`. The format, its name
  `hdf5` and the class `CHDF5File` exist in every build, so that the API does
  not depend on the option; without HDF5 its methods fail with an error
  status.
- The serial HDF5 library, version 1.10 or newer, with its C API, used only
  inside the implementation (`CHDF5FilePrivate`); no public header of libmedio
  includes an HDF5 header.
- The HDF5 library reads and writes the file itself, by its name. `CHDF5File`
  keeps the `QFile` of `CMedIOData` open for reading besides, so that the open
  state checked by the base classes stays correct without a change of the
  public API.
- Writing goes into a temporary file in the directory of the target, which is
  renamed after the file was closed successfully. A failed or interrupted
  write leaves no partial file.
- All calls into the HDF5 library are serialised by one mutex, and its error
  messages are passed to librtdebug.
- R package: the sources compile without HDF5 (R compiles all vendored
  sources). `Rmedio` has HDF5 support if its `configure` finds the serial
  HDF5 library with `pkg-config` when the package is installed
  (`RMEDIO_HDF5=auto|yes|no`, see [r/README.md](../r/README.md)); `medioHDF5()`
  tells whether it has. Windows builds have none by default.
- Tests: the expected values of HDF5 test files come from `h5py`
  (`make_expected.py`), and files written by libmedio are read back with
  `h5py`, as the other formats are checked with nibabel and numpy.

## Phases

| Phase | Content | Check |
| --- | --- | --- |
| 1 (done) | this specification; a prototype in Python (`testsuite/hdf5/tools/medio2hdf5.py`, `pmedio` + `h5py`) that writes the test datasets in this layout and a checker for it (`check_hdf5.py`, `h5py` only) | review of the example files, reading them with MATLAB/Octave and R |
| 2 (done) | attribute dictionary: `DecayCorrectionDateTime` and `FrameReferenceTime` for all formats | attribute tests, several time zones |
| 3 (done) | build option `MEDIO_WITH_HDF5`, format `CMedIOData::HDF5` and class `CHDF5File` as stub, identification by the signature | CI with and without HDF5, R package |
| 4 (done) | reading and writing (C++, Python and R) and the conversion from and into all writable formats, on the command line with [`medioconvert`](medioconvert.md) (`ecat2hdf5`, `hdf52nifti`, ...) | fixtures of `testsuite/hdf5/data` with the values `h5py` reads (`generate_fixtures.py`), files of the prototype compared with `h5py`; `testConversions` with HDF5 as target, files written by libmedio checked with `check_hdf5.py` and read with `h5py` |
| 5 | optional: reading and writing the ITK layout; MINC 2 with libminc as further optional dependency | comparison with SimpleITK and the MINC tools |

## Decisions

| Question | Decision |
| --- | --- |
| Layout | own, versioned layout (this document) |
| Voxel values | physical values as float32, no scale factors |
| Metadata names | DICOM keywords; attributes of the Enhanced PET IOD only where the classic PET IOD has none (`DecayCorrectionDateTime`) |
| Compression | shuffle + deflate only |
| Oldest HDF5 library | 1.10 |
| R package | HDF5 optional, with the HDF5 library of the system |
| Open state | the `QFile` stays open besides the HDF5 handle, no API change |
| Group of the study attributes | `/header`, as the attributes are those of the main header of libmedio |
| File name extensions | `.h5`, `.hdf5`, `.hdf` |
| Bed position | optional dataset `TablePosition` per volume; the affine alone defines the geometry |
| Matrix index | frame, gate and bed position; no column for the fifth part of the ECAT matrix index (data), which images do not use |
| Number of volumes | not fixed in advance: the dataset grows by the volumes written |
| Further layouts | ITK and MINC 2 in phase 5 |

## Open points

Checked for this draft:

- All DICOM keywords named here exist in the DICOM data dictionary of pydicom
  with the tag and VR given (the source `generate_dictionary.py` uses for the
  attribute dictionary), among them `DecayCorrectionDateTime` (0018,9701, DT),
  `FrameReferenceTime` (0054,1300, DS) and `TriggerTime` (0018,1060, DS).
- An example file of this layout written with `h5py` is read identically by
  `h5py` and by pyfive, a reader that does not use the HDF5 library. With the
  lowest format bound it has a version 0 superblock.
- The user block sizes (powers of two from 512) are documented with
  `H5Pset_userblock()` in the HDF5 sources.

Found with the prototype (`testsuite/hdf5/tools/`):

- The test datasets of all formats can be written in this layout and pass the
  checker. Several of them have no unit (Philips, NIfTI and STIR files), which
  is why the unit is not required.
- libmedio reports `KBQCC` as `Units` for some files, which is no defined term
  of DICOM Units.
- None of the test datasets provides `DecayCorrection`, `CorrectedImage` or
  the code of the radionuclide. `DecayCorrectionDateTime` is given by the
  Siemens Interfile datasets only, `FrameReferenceTime` by none of them
  (sidecars of dcm2niix have it).
- The bed position is not among the attributes of a file (it is part of the
  geometry), so the prototype cannot write `TablePosition`.

Still to be checked or decided:

- The definitions of `DecayCorrectionDateTime`, `FrameReferenceTime`,
  `TablePosition` and `DecayFactor` and the defined terms of `Units`, `DecayCorrection` and
  `CorrectedImage` have to be compared with the text of the DICOM standard
  (PS3.3 C.8.9 and C.8.22); the data dictionary only confirms names, tags and
  types.
- The UCUM notation of the SUV variants of `Units` (`GML`, `CM2ML`, ...).
- Chunks of images with planes larger than 1 MiB (more than 512 × 512 voxels):
  one plane per chunk, or planes split into several chunks.
- Whether the frame attributes get further datasets (e.g. the scatter
  fraction or the reconstruction parameters of a frame) in the first version.
- The behaviour of MATLAB, R and Java with variable-length UTF-8 string
  attributes (phase 1).
- Opening a file with `QFile` and the HDF5 library at the same time on
  Windows (phase 3); if that is not possible, the open state needs a virtual
  method in `CMedIOData`.
- File names with characters outside of ASCII on Windows with HDF5 libraries
  older than 2.0.
