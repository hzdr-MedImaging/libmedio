# Interfile support (design)

This document collects what is known about the Interfile format and its
variants and describes how libmedio supports it. It is a design document: the
support is built in phases (see [Phases](#phases)), and the sections on the
later phases describe the plan, not the current state.

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
| position | PMOD: `origin (mm) [n]` | `first pixel offset (mm) [n]` | (to be determined) |
| orientation | `slice orientation` | `patient orientation` (`head_in`/`feet_in`), `patient rotation` (`supine`/`prone`) | `%patient orientation` (values to be determined) |
| frames | frame groups: `!frame group number`, `!number of images this frame group`, `!image duration (sec)`, `pause between images (sec)`, `pause between frame groups (sec)` | `number of time frames`, `image duration (sec)[f]`, `image relative start time (sec)[f]` | `image duration (sec)`, `image relative start time (sec)` |
| scaling | PMOD: `data rescale slope`, `data rescale offset` | `image scaling factor[f]`; `quantification units` (a factor in STIR files, the units in the proposal) | (to be determined) |
| units | PMOD: `units` (`BQML`, `kBq/cc`) | `quantification units` | |
| data offset | `data offset in bytes`, `data starting block` | `data offset in bytes[f]` | `data offset in bytes` |
| isotope | `radiopharmaceutical`, `isotope halflife` (PMOD) | `radionuclide name[1]`, `radionuclide halflife (sec)[1]` | `isotope name`, `isotope gamma halflife (sec)` |
| patient | `patient name`, `!patient ID`, `patient dob` (`yyyy:mm:dd`), `patient sex` | + `patient weight (kg)`, `patient height (cm)` | |
| study | `study date` (`yyyy:mm:dd`), `study time` (`hh:mm:ss`) | `study date`, `study time` | (to be determined) |
| dose | PMOD: `pmod tracer activity` | `tracer activity at time of injection (MBq)`, `relative time of tracer injection (sec)` | |
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

## Phases

1. **Header syntax and image layout** (done): `CInterfileKeys` and
   `CInterfileImageLayout` (private, `src/interfile/`).
2. **Reading images**: `CInterfileFile` (`CMedIOData`), `CInterfileMainHeader`
   (`CMedIOHeader`), the format `CMedIOData::Interfile`, the factory, the
   format independent access (`CMedIOMatrix`, `CMedIOVolume`), geometry, frame
   timing, attributes; Siemens main headers (`.mhdr`) and image headers from
   samples of Siemens reconstructions; Python and R.
3. **Writing images**: one canonical variant, conversions from and to the
   other formats, the command line tools.
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
  `headerSize` gives the end of the header;
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
- the spellings of Siemens and the keys of PMOD,

and rejects what images cannot be (more than 3 dimensions, matrix sizes per
plane, `bit` and `ASCII` data) with a message. Both classes are private (not
installed): the public API follows with phase 2.

### Phase 2: reading images (plan)

- `CMedIOData::Format` gets `Interfile` **at the end**, so that the values of
  the other formats do not change.
- `CMedIODataFactory::identify()` checks for Interfile **before** NIfTI (`.hdr`)
  and Philips (`.img`), by content (`CInterfileKeys::isInterfile()`); ECAT7 is
  identified by its magic number and is not affected. Given the data file, the
  header next to it is looked for (`<name>.hdr`, `<name>.hv`, `<name>.h33`,
  `<data file>.hdr`).
- `CInterfileFile` reads the matrices (frames) with the layout; the size of the
  data file is checked against `requiredDataSize()`.
- `CInterfileMainHeader` holds the keys and the layout; `convertFrom()` converts
  from and to the ECAT7 headers (the common representation of the formats, see
  `CNIFTIConverter.h`); `CInterfileHeaderMapping` maps keys to the DICOM
  attributes of `CMedIOHeaderDictionary` (`patient name` → `PatientName`,
  `patient dob` → `PatientBirthDate`, `patient weight (kg)` → `PatientWeight`,
  `study date`/`study time` → `StudyDate`/`StudyTime`, `originating system` →
  `ManufacturerModelName`, `radionuclide halflife (sec)` →
  `RadionuclideHalfLife`, `tracer activity at time of injection (MBq)` →
  `RadionuclideTotalDose` (Bq), `quantification units`/`units` → `Units`, ...);
  the other keys are kept in an `"Interfile"` section of the attributes.
- `CMedIOMatrix` and `CMedIOVolume` get the cases of the new format; the
  geometry (voxel size, position, axis directions) is verified with files
  written by STIR and with the Siemens samples, not assumed.
- Python: `pmedio.Format.Interfile`, `pmedio.load()` identifies it; R:
  `readMedio()` identifies it. Tests compare the values with the ECAT7 file of
  the same dataset (`testsuite/data/datasets.json`).

### Phase 3: writing images (plan)

- A canonical variant: the keys of Interfile 3.3 and the PET proposal as STIR
  reads them, with the keys of the source kept where they apply.
- New files by name: `.hv` (header, STIR convention) and `.h33`; `.hdr` stays
  NIfTI, Interfile with `.hdr` is written with the format given explicitly. The
  extension of the data file is to be decided (`.v` as STIR, which is also the
  extension of ECAT7, or `.i33`/`.img`).
- Files are written atomically (no partial files when writing fails).

## Test data and references

- `testsuite/data/interfile/`: Interfile 3.3 files of PMOD (single file and
  header/image pair, static and dynamic) with expected values of an own parser
  of `testsuite/data/tools/make_expected.py`. They are converted from the ECAT7
  files of the same datasets (stored values identical, z axis reversed; for
  `pet-dynamic` the calibration factor 1000 is not applied, units kBq/cc).
- Planned: anonymized Siemens files (images of e7tools/JSRecon, main headers),
  files written by STIR (conda-forge package `stir`) and by (X)MedCon
  (package `xmedcon` of Ubuntu) as independent implementations. nibabel does
  not support Interfile, the Python package `interfile` is Python 2 only.

## Open questions

- The keys of the image headers of Siemens reconstructions (`.v.hdr`), the
  structure of main headers (`.mhdr`) and how dynamic images are stored: to be
  taken from samples.
- The axis directions of each variant (Siemens `%patient orientation`, STIR
  `patient orientation`/`rotation`, PMOD).
- The extension of written data files (phase 3).

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
