# Interfile support (design)

This document collects what is known about the Interfile format and its
variants and describes how libmedio supports it. The support is built in
phases (see [Phases](#phases)): phases 1 to 3 (reading and writing images) are
implemented, the optional phases 4 and 5 are plans.

## The format

Interfile was defined for the exchange of nuclear medicine data: Interfile 3.3
(Todd-Pokropek, Cradduck, Deconinck, *Nucl Med Commun* 13 (1992) 673-699) for
gamma cameras and SPECT. PET data is written with extensions of it, which
differ between the programs and vendors:

| Variant | Written by | Typical files | Characteristics |
| --- | --- | --- | --- |
| Interfile 3.3 | PMOD, (X)MedCon, gamma cameras | `.hdr`/`.img`, `.h33`/`.i33`, single file | static, tomographic and dynamic (frame groups) studies, no PET keys |
| PET proposal | basis of STIR (Thielemans et al., *A preliminary proposal for Interfile keys for PET*, version 0.3, 2001) | | `!PET data type`, `number of time frames`, vectored keys `key[f]`, sinograms, `version of keys := 3.31` |
| STIR | STIR, SIRF | `.hv`/`.v` (images), `.hs`/`.s` (projection data) | `!version of keys := STIR6.0`, `first pixel offset (mm)`, `image relative start time (sec)` |
| Siemens | Biograph mCT, Vision, mMR, HRRT; e7tools/JSRecon | `.s.hdr`/`.s` (sinograms), `.l.hdr`/`.l` (list mode), `.n.hdr`/`.n` (normalisation), `.mhdr` (main header), images | vendor keys with `%` (`%axial compression`, `%segment table`, ...), `image data byte order`, `scale factor (mm/pixel)` |
| CASToR | CASToR | `.hdr`/`.img` | own key set ("CASToRv3.1") |

**The variants of Siemens (images and raw data of Siemens reconstructions) are
the primary target of libmedio**; the other variants are supported as far as
they describe the same kind of data.

### Syntax (common to all variants)

- An ASCII header with one `key := value` per line, from `!INTERFILE :=` to
  `!END OF INTERFILE :=`; lines end with CR LF (Interfile 3.3), LF is common.
  The headers of Siemens reconstructions have **no** `!END OF INTERFILE`: the
  header ends with the file (they are always separate from the data).
- `!` marks a key required by the standard, `%` a vendor key (Siemens), `;`
  starts a comment line.
- Keys are compared without regard to the letter case, `_` and white space
  (`Image_Duration(Sec)` is `image duration (sec)`); units are part of the key
  (`(sec)`, `(mm/pixel)`).
- Indices follow the key: `matrix size [1]`, `image duration (sec)[2]`, also
  several (`total prompts[f][g][e]`). Lists are written `{a, b, c}`, also for
  matrix sizes that differ per segment of 3D sinograms.
- The data is in a separate file (`name of data file`, relative to the header)
  or in the header file after the header (Interfile 3.3 single file: the
  header ends with `^Z` (0x1A); `data offset in bytes`, or `data starting
  block` in blocks of 2048 bytes).
- Byte order `imagedata byte order` (Siemens: `image data byte order`):
  **big endian if not given**.
- Number formats: `signed integer`, `unsigned integer` (default), `float`,
  `short float` (4 bytes), `long float` (8 bytes), `bit`, `ASCII`, with
  `number of bytes per pixel`.

### Keys of image data

| Meaning | Interfile 3.3 / PMOD | PET proposal / STIR | Siemens |
| --- | --- | --- | --- |
| matrix size | `!matrix size [1..3]` (or `[1..2]` and `!number of slices`) | `!matrix size [1..3]`, `matrix axis label [n]` | `matrix size [n]` |
| voxel size | `scaling factor (mm/pixel) [n]`, `slice thickness (pixels)` | `scaling factor (mm/pixel) [n]` | `scale factor (mm/pixel) [n]` |
| position | PMOD: `origin (mm) [n]` | `first pixel offset (mm) [n]` with `patient orientation`/`patient rotation` (see [Phase 3](#phase-3-writing-images)) | `start horizontal bed position (mm)`, `start vertical bed position (mm)`, `%gantry offset (mm) [n]`, per plane `%image info[n]` (see below; to be verified) |
| orientation | `slice orientation` | `patient orientation` (`head_in`/`feet_in`), `patient rotation` (`supine`/`prone`) | `%image orientation := {1,0,0,0,1,0}`, `%patient orientation := HFS` (DICOM terms), `slice orientation` |
| frames | frame groups: `!frame group number`, `!number of images this frame group`, `!image duration (sec)`, `pause between images (sec)`, `pause between frame groups (sec)` | `number of time frames`, `image duration (sec)[f]`, `image relative start time (sec)[f]` | `image duration (sec)`, `image relative start time (sec)` |
| scaling | PMOD: `data rescale slope`, `data rescale offset` | `image scaling factor[f]`; `quantification units` (a factor in STIR files, the units in the proposal) | `%image slope`, `%image intercept` |
| units | PMOD: `units` (`BQML`, `kBq/cc`) | `quantification units` | `quantification units` (`Bq/ml`) |
| data offset | `data offset in bytes`, `data starting block` | `data offset in bytes[f]` | `data offset in bytes` |
| isotope | `radiopharmaceutical`, `isotope halflife` (PMOD) | `radionuclide name[1]`, `radionuclide halflife (sec)[1]` | `isotope name`, `isotope gamma halflife (sec)`, `isotope branching factor`, `radiopharmaceutical` |
| patient | `patient name`, `!patient ID`, `patient dob` (`yyyy:mm:dd`), `patient sex` | + `patient weight (kg)`, `patient height (cm)` | |
| study | `study date` (`yyyy:mm:dd`), `study time` (`hh:mm:ss`) | `study date`, `study time` | `%study date (yyyy:mm:dd)`, `%study time (hh:mm:ss GMT+00:00)` (UTC) |
| dose | PMOD: `pmod tracer activity` | `tracer activity at time of injection (MBq)`, `relative time of tracer injection (sec)` | `tracer activity at time of injection (Bq)`, `%tracer injection date (yyyy:mm:dd)`, `%tracer injection time (hh:mm:ss GMT+00:00)` |
| decay | | | `%decay correction` (`reftime`), `%decay correction reference date`/`time`, `%image info[n]` (factor per plane, to be verified) |
| system | `!originating system` | `originating system` | `originating system` |

Pitfalls the support has to handle:

- STIR writes `image scaling factor` per frame, but applies it per plane when
  reading; `quantification units` is a factor or a unit depending on the
  writer.
- There is no complete model of the orientation as in NIfTI or DICOM; the
  directions of the axes follow from `patient orientation`/`rotation` (STIR),
  `%patient orientation` (Siemens) or not at all (Interfile 3.3).
- Interfile uses the file name extensions that libmedio has for other formats:
  `.hdr` (NIfTI header/image pair), `.img` (Philips image), `.v` (ECAT7).
- PMOD writes PET images as Interfile 3.3 `Dynamic` (frame groups) or
  `Tomographic` studies with `!SPECT STUDY` sections and own `pmod ...` keys.

### Images of Siemens reconstructions

From samples of Siemens reconstructions (`!originating system := 1208`,
`%SMS-MI version number := 3.9`, PSF+TOF), each compared with the DICOM series
of the same reconstruction:

- test datasets `pet-bladder-static` and `pet-bladder-dynamic`: a static and a
  dynamic reconstruction (2 frames of 60 s) of one step and shoot acquisition
  of a phantom (`%type of detector motion := step and shoot`, `%scan direction
  := in`, one bed position, 32x32x159);
- an earlier sample with continuous bed motion (`%scan direction := out`,
  32x32x583, one frame; not in the test data).

The files and keys:

- The main header `<name>.mhdr`, and per data set an image header
  `<name>_<frame>_<bed>.v.hdr` and a data file `<name>_<frame>_<bed>.v`. The
  headers are written `key:=value` without spaces, with CR LF, without
  `!END OF INTERFILE`.
- The main header (`%SMS-MI header name space := image main header`) describes
  the study (`%study date`, `exam type`, `number of time frames`, `%number of
  horizontal bed offsets`, `number of time windows`) and lists the data sets:
  `!total number of data sets`, `%data set[n] := {<id>, <image header>, <data
  file>}` (dynamic: one data set per frame, id 0, 1000000, ...). It describes
  no image data itself.
- The image header (`%SMS-MI header name space := image subheader`) describes
  one frame: `number format := float`, `!number of bytes per pixel := 4`,
  `image data byte order := LITTLEENDIAN`, `matrix size[1..3]`, `scale factor
  (mm/pixel) [1..3]`, `!image relative start time (sec)`, `!image duration
  (sec)`, `quantification units := Bq/ml`, `%image slope`, `%image intercept`,
  `minimum/maximum pixel count`, and the reconstruction (`method of
  reconstruction`, `number of iterations`, `%number of subsets`, PSF and filter
  keys, the corrections applied).
- The data file holds `matrix size[1] * [2] * [3]` values, x fastest; its
  minimum and maximum are the `minimum/maximum pixel count` of the header. The
  DICOM series holds the same planes in the same order (image index i of a
  frame is plane i - 1).
- `%image info[n]` (one per plane, n = 1..planes) is a list of 15 elements.
  Compared with the DICOM series (Image Position Patient, Decay Factor,
  Scatter Fraction Factor, Actual Frame Duration, Acquisition Time of each
  slice):

  | Element | Meaning |
  | --- | --- |
  | 1 | n - 1 |
  | 2 | z position (mm) |
  | 3, 4 | x and y position (mm) of the first voxel of the plane (DICOM Image Position Patient) |
  | 5, 6, 7 | minimum, mean (not checked), maximum of the plane |
  | 10 | decay factor (DICOM Decay Factor) |
  | 11 | scatter fraction (DICOM Scatter Fraction Factor) |
  | 12 | start time of the slice (s) relative to the start of the frame (continuous bed motion; 0 for step and shoot) |
  | 13 | duration of the slice (s) (DICOM Actual Frame Duration) |
  | 14 | `%scanner quantification factor` |
  | 8, 9, 15 | not known (`1`, `0.0`, `no slice` in the samples) |

  The entries are in the order of the scan: the z of `%image info[1]` is
  `start horizontal bed position (mm)`, and z falls with n in the step and
  shoot samples (scan direction "in"), rises in the sample with continuous bed
  motion (scan direction "out").
- **The planes of the data file are from the highest to the lowest z** in all
  samples: plane k (k = 0..planes-1) has the z position, minimum and maximum
  of the entry with the k-th highest z, i.e. `%image info[k + 1]` in the step
  and shoot samples and `%image info[planes - k]` in the sample with
  continuous bed motion. In DICOM patient coordinates (LPS, with `%image
  orientation := {1,0,0,0,1,0}` and `%patient orientation := HFS`):

  - voxel (i, j, k) of the data file is at x0 + i * dx, y0 + j * dy and the z
    of plane k, with dx, dy, dz from `scale factor (mm/pixel)` (the z of
    `%image info` are rounded to 0.001 mm: libmedio takes the first plane from
    them and the plane distance from the header);
  - x0 and y0 are the elements 3 and 4 of `%image info`; in all samples they
    are also `-matrix size[1] / 2 * dx - %gantry offset (mm) [1]` and
    `-matrix size[2] / 2 * dy - start vertical bed position (mm) - %gantry
    offset (mm) [2]`, and the z of the first plane is `start horizontal bed
    position (mm)` if `end horizontal bed position (mm)` is lower, otherwise
    `start horizontal bed position (mm) + (planes - 1) * dz`. These formulas
    are used only where `%image info` is not given.
- With continuous bed motion every slice has its own start time, duration and
  decay factor. The DICOM series of that sample gives plane k those of
  `%image info[k + 1]`, i.e. not of the entry with its z position: either the
  DICOM export or the order of these elements differs from the z positions.
  libmedio uses one start time and duration per frame (`!image relative start
  time (sec)`, `!image duration (sec)`), so this does not affect the images.

## Phases

1. **Header syntax and image layout** (done): `CInterfileKeys` and
   `CInterfileImageLayout` (`src/interfile/`).
2. **Reading images** (done): `CInterfileFile` (`CMedIOData`),
   `CInterfileMainHeader` and `CInterfileFrameHeader` (`CMedIOHeader`), the
   format `CMedIOData::Interfile`, the factory, the format independent access,
   geometry, frame timing, attributes; Siemens main headers (`.mhdr`) and
   image headers; Python (`pmedio.Format.Interfile`) and R.
3. **Writing images** (done): Interfile as written by STIR (`.hv`/`.v`), from
   all formats through the format independent access; Python and R.
4. *(optional)* **Projection data** (sinograms, normalisation) through the
   matrix interface of ECAT7 scans.
5. *(optional)* **List mode**: header and metadata only (the event format is
   vendor specific).

## Design

### Phase 1: header syntax and image layout

`CInterfileKeys` (`src/interfile/CInterfileKeys.h`) reads the lines of a
header into an ordered list: the key as written, its normalized name, the
indices, the value, the `!`/`%` flags and comment lines. It keeps repeated
keys (the keys of every frame group), unknown keys (vendor keys) and comments,
so that the header can be written again as it was read (`toText()`). It is
independent of the variant:

- the header starts with `!INTERFILE` (after empty and comment lines) and ends
  with `!END OF INTERFILE`; the data after it (single files) is not looked at,
  `headerSize` gives the end of the header; without `!END OF INTERFILE`
  (Siemens) the header ends with the file, and binary data before the end is
  an error;
- lines end with LF, CR LF or CR; text is UTF-8, otherwise Latin-1;
- a line without `:=` inside the header is kept as comment;
- `isInterfile()` identifies a file by its first line, as the factory will.

`CInterfileImageLayout` (`src/interfile/CInterfileImageLayout.h`) reads the
description of the image data from the keys: the data file, the byte order,
and per frame the number format, matrix size, voxel size, position, timing,
scaling (`physical = stored * slope + intercept`) and data offset. It
understands

- Interfile 3.3 static and tomographic studies (also without `matrix size [3]`:
  `number of slices`, `slice thickness (pixels)`),
- Interfile 3.3 dynamic studies in frame groups (keys of a group apply only to
  it; start times from the durations and pauses),
- the vectored keys of the PET proposal and STIR (`number of time frames`,
  `image duration (sec)[f]`, `image scaling factor[f]`, `data offset in
  bytes[f]`),
- the spellings of Siemens (`image data byte order`, `scale factor
  (mm/pixel)`, `%image slope`, `%image intercept`) and the keys of PMOD,

and rejects what images cannot be (more than 3 dimensions, matrix sizes per
plane, `bit` and `ASCII` data) and Siemens main headers (they name the headers
of their data sets) with a message. `CInterfileKeys` is installed (the
headers of `CInterfileFile` give their keys), `CInterfileImageLayout` is
private.

### Phase 2: reading images

- `CMedIOData::Format` has `Interfile` **at the end**, `CMedIOHeader::Format`
  `InterfileMainHeader` and `InterfileFrameHeader`, so that the values of the
  other formats do not change. The name of the format is `"interfile"`
  (`CMedIODataFactory::formatName()`, Python `pmedio.Format.Interfile`, R).
- `CMedIODataFactory::identify()` and `create()` check for Interfile **before**
  Philips (`.img`) and NIfTI (`.hdr`), by content; ECAT7 is identified by its
  magic number first. A file is Interfile if `CInterfileFile` can open it:
  - an Interfile header (`CInterfileKeys::isInterfile()`) describing image data,
  - a Siemens main header (`%data set[n]`): the frames of the image headers of
    its data sets, in the order of n,
  - a data file: the header next to it that names it as data file
    (`<data file>.hdr`, `<name>.hdr`, `<name>.hv`, `<name>.h33`); a data file
    whose header is a single file (data in the header file) is not found this
    way, so that e.g. a Philips `pet-dynamic.img` next to a PMOD single file
    `pet-dynamic.hdr` stays Philips.

  The data files have to exist and hold the data the headers describe
  (`requiredDataSize()`); otherwise the file is not Interfile
  (`CInterfileFile::openError()` gives the reason).
- `CInterfileFile` is read only (`open(QIODevice::ReadOnly)`). Its frames are
  numbered 1..n; `readMatrix()` gives the voxels in the order of the file and
  the data type of the stored values (signed/unsigned 8/16/32 bit integers,
  float32, float64) in the byte order of this machine, with a
  `CInterfileFrameHeader` describing them (matrix, data type, scaling, units,
  affine, timing, the keys of the header). `allFileNames()` (and
  `CMedIOData::fileNames()`) give all headers and data files.
- Scaling: physical value = stored value × slope + intercept from
  `data rescale slope/offset` (PMOD), `image scaling factor[f]` (STIR) or
  `%image slope/intercept` (Siemens). The values are not converted into other
  units, as for the other formats: `units`/`quantification units` Bq/ml (also
  Bq/cc) give the attribute `Units` `BQML`, kBq/cc (also kBq/ml) `KBQCC` (PMOD
  writes kBq/cc for `pet-dynamic`, as in its NIfTI files); other units are kept
  as given in the frame header.
- Position of the voxels (`CInterfileFrameHeader::affine()`, LPS):
  - Siemens: x and y of the first voxel and the z of every plane from
    `%image info` (see [above](#images-of-siemens-reconstructions)), the axes
    of the rows and columns from `%image orientation`; without `%image info`
    from the bed position and the gantry offset (the formulas of the sample).
    Tested with the landmarks of `pet-bladder-static` and `pet-bladder-dynamic`
    (absolute positions of the
    DICOM series, within 0.01 mm).
  - STIR (`first pixel offset (mm) [1..3]` given): see [Phase 3](#phase-3-writing-images).
    Tested with `interfile/pet-static.stir`, written by STIR from
    `nifti/pet-static.nii.gz` (the same positions as the NIfTI file).
  - Other variants: x towards patient left, y towards posterior, the planes
    from inferior to superior (as written by PMOD, tested with the ECAT7 files
    the PMOD files were converted from), centered at 0. `origin (mm)` (PMOD) is
    not used: its meaning has not been verified with independent data.
- Attributes (`attributes()` of the headers, `CMedIOData::readAttributes()`):

  | Attribute | Keys |
  | --- | --- |
  | `PatientName`, `PatientID`, `PatientBirthDate`, `PatientSex` | `patient name`, `patient ID`, `patient dob` (yyyy:mm:dd), `patient sex` |
  | `PatientWeight`, `PatientSize` | `patient weight (kg)`, `pmod patient weight`; `patient height (cm)` / 100, `pmod patient size` |
  | `PatientPosition` | `%patient orientation` (Siemens, DICOM terms), `patient orientation` + `patient rotation` (STIR: `head_in`/`feet_in`, `supine`/`prone`) |
  | `StudyID` | `study ID` |
  | `AcquisitionDateTime` | `pmod scan datetime UTC` (ms since 1970), `study date` + `study time` (with the zone of the key, Siemens `GMT+00:00`: UTC) |
  | `Manufacturer`, `ManufacturerModelName` | `SIEMENS` for Siemens headers; `originating system` |
  | `Radiopharmaceutical`, `RadionuclideHalfLife`, `RadionuclidePositronFraction` | `radiopharmaceutical`; `isotope gamma halflife (sec)`, `radionuclide halflife (sec)`, `isotope halflife`; `isotope branching factor` |
  | `RadionuclideTotalDose` (Bq) | `tracer activity at time of injection (Bq)`, `(MBq)`, `pmod tracer activity` (MBq) |
  | `RadiopharmaceuticalStartDateTime` | `pmod administration datetime UTC`, `%tracer injection date` + `time` |
  | `EnergyWindowLowerLimit`, `EnergyWindowUpperLimit` | `energy window lower/upper level [1]` (also with `(keV)`) |
  | `Units` | see scaling |
  | `ReconstructionMethod` | `method of reconstruction` (Siemens: with ` <n>i <m>s` of `number of iterations` and `%number of subsets`, as in the DICOM series) |
  | `AttenuationCorrectionMethod`, `ScatterCorrectionMethod` | `method of attenuation correction`, `method of scatter correction` |
  | `ActualFrameDuration` (frames) | the duration of the frame |

  The section `"Interfile"` holds the keys as written that do not describe the
  layout, geometry, scaling or timing of the image data (repeated keys as
  array): the main header (Siemens: the `.mhdr`; others: the keys before the
  first frame group), and per frame the image header of a Siemens data set or
  the keys of the frame group. Siemens image headers give the attributes not
  given by the main header (patient, isotope, reconstruction).
- Tests (`testsuite/interfile/testInterfileFile.cpp`, Python, R): the stored
  values and timing against `make_expected.py`, the physical values, timing
  and attributes against the ECAT7 files of PMOD's datasets, the positions and
  attributes of `pet-bladder-static`/`pet-bladder-dynamic` against their DICOM
  series, the plane order of both scan directions, every file of a
  dataset identified, the conversion into NIfTI.

### Phase 3: writing images

New files are written as STIR writes images, so that STIR (and SIRF) read them
with their positions:

- The header `<name>.hv` and the data file `<name>.v` (the extension of the
  header replaced by `.v`), as STIR names its images; `CMedIODataFactory`
  takes `.hv` as Interfile (`formatOfFileName()`, `createForWriting()`). The
  data file has the extension of ECAT7 files, but ECAT7 is identified by its
  magic number and the header next to an Interfile data file names it.
- The keys of STIR 6 (`!version of keys := STIR6.0`, `!imaging modality := PT`,
  `!PET data type := Image`, `imagedata byte order := LITTLEENDIAN`, `!number
  format`, `!matrix size [i]`, `scaling factor (mm/pixel) [i]`, `first pixel
  offset (mm) [i]`, `patient orientation`, `patient rotation`, `number of time
  frames`), per frame `image scaling factor[f]`, `data offset in bytes[f]`,
  `image duration (sec)[f]` and `image relative start time (sec)[f]`.
- The number format of the first frame written for all frames: signed 16 bit
  integers with a scale factor per frame if the values are integers that fit,
  otherwise float32 (the physical values). A frame that does not fit into the
  number format of the file, or has another matrix or position, is not written.
- The attributes as keys (read again by libmedio, see the table above):
  `patient name`, `!patient ID`, `patient sex`, `patient weight (kg)`,
  `patient height (cm)`, `patient dob`, `!study ID`, `!originating system`,
  `study date (yyyy:mm:dd)` + `study time (hh:mm:ss GMT+00:00)` (UTC; without
  zone: `study date`/`study time`), `radiopharmaceutical`, `tracer activity at
  time of injection (Bq)`, `tracer injection date/time`, `radionuclide
  halflife (sec)[1]`, `radionuclide branching factor[1]`, `energy window
  lower/upper level[1]`, `units` (`Bq/ml`, `kBq/cc`), `method of
  reconstruction`, `method of attenuation correction`, `method of scatter
  correction`. STIR reads the radionuclide, energy window and patient position
  and ignores the others (with a warning).
- The main header gives the number of frames (`CMedIOData::requiredFrames()`):
  the header is written with the main header and again after every frame, the
  frames at their offsets; a file without all frames cannot be read (the data
  file is too short). The callers remove incomplete files
  (`CMedIOData::removeFiles()`: header and data file).

Position of the voxels in the files of STIR: `first pixel offset (mm) [i]` is
the position of the first voxel in the coordinates of STIR, whose axes are the
voxel axes (x fastest, then y, then z); their directions in DICOM patient
coordinates (LPS) depend on the patient position. Determined with STIR 6.3
(conda-forge): images written by `generate_image` and with `first pixel
offset` set to arbitrary values, with the four patient positions, converted
by STIR into NIfTI with its ITK output and read with nibabel (the voxels in
the order of the Interfile data, the offsets taken as they are):

| `patient orientation` / `patient rotation` | LPS of voxel (i, j, k), s = offset + (i dx, j dy, k dz) | voxel axes |
| --- | --- | --- |
| `head_in` / `supine` (also if not given) | (+s₁, +s₂, −s₃) | LPI |
| `feet_in` / `supine` | (−s₁, +s₂, +s₃) | RPS |
| `head_in` / `prone` | (−s₁, −s₂, −s₃) | RAI |
| `feet_in` / `prone` | (+s₁, −s₂, +s₃) | LAS |

libmedio reads STIR files with this table and writes new files in the voxel
axes of the patient position of the attributes (`PatientPosition` HFS, FFS,
HFP or FFP; otherwise HFS), so that STIR gives every voxel its position.
Verified with STIR 6.3: files written by libmedio from `ecat/pet-static.v`,
`ecat/pet-dynamic.v` (16 bit integers, 3 frames), the earlier Siemens sample and
`nifti/pet-static.nii.gz` (with the patient positions FFS, HFP, FFP) were read
by STIR (`stir_math` into NIfTI with ITK, `extract_single_images_from_dynamic_image`
for the frames): the same positions (difference 0.0000 mm), values (relative
difference < 1e-7, STIR stores float32) and frame timing.

## Test data and references

- `testsuite/data/interfile/`: Interfile 3.3 files of PMOD (single file and
  header/image pair, static and dynamic) with expected values of an own parser
  of `testsuite/data/tools/make_expected.py`. They are converted from the ECAT7
  files of the same datasets (stored values identical, z axis reversed; for
  `pet-dynamic` the calibration factor 1000 is not applied, units kBq/cc).
- `testsuite/data/interfile/pet-bladder-static.siemens/` and
  `pet-bladder-dynamic.siemens/`: the Interfile output of a static and a
  dynamic (2 frames, 2 data sets) Siemens reconstruction of a phantom, only the
  file names changed, and `testsuite/data/dicom/pet-bladder-static/`,
  `pet-bladder-dynamic/`, the DICOM series of the same reconstructions; the
  landmarks of `datasets.json` give the DICOM positions of the first and last
  voxel of the data files. The expected values of the Interfile files (own
  parser of `make_expected.py`, which follows the data sets of the main
  header) include the start times and the plane order relative to `%image
  info[n]`.
- `testsuite/data/interfile/pet-static.stir/`: written by STIR 6.3 (`stir_math`)
  from `nifti/pet-static.nii.gz` (read with ITK): float32, planes reversed,
  `first pixel offset`; its expected values include the LPS position of the
  first voxel by the table of phase 3.
- Planned: files written by (X)MedCon
  (package `xmedcon` of Ubuntu) as independent implementation. nibabel does
  not support Interfile, the Python package `interfile` is Python 2 only.

## Open questions

- Siemens: how several bed positions are stored (data sets `<frame>_<bed>`?);
  the position of the voxels for other patient orientations than HFS; the
  order of the time elements of `%image info` with continuous bed motion (see
  above).
- The meaning of `origin (mm)` (PMOD), to be verified with files whose
  positions are known.

## References

- Todd-Pokropek A, Cradduck TD, Deconinck F. A file format for the exchange of
  nuclear medicine image data: a specification of Interfile version 3.3. Nucl
  Med Commun 13 (1992) 673-699.
- [A preliminary proposal for Interfile keys for PET (version 0.3)](https://stir.sourceforge.net/links/petinterfile03.pdf)
- STIR: [interfile.cxx](https://github.com/UCL/STIR/blob/master/src/IO/interfile.cxx),
  [InterfileHeader.cxx](https://github.com/UCL/STIR/blob/master/src/IO/InterfileHeader.cxx),
  [InterfileHeaderSiemens.cxx](https://github.com/UCL/STIR/blob/master/src/IO/InterfileHeaderSiemens.cxx),
  [an mMR sinogram header](https://github.com/UCL/STIR/blob/master/examples/Siemens-mMR/template_span11.hs)
- [SIRF wiki: PET raw data](https://github.com/SyneRBI/SIRF/wiki/PET-raw-data),
  [pet-rd-tools](https://github.com/UCL/pet-rd-tools)
