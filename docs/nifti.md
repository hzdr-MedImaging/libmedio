# Conversion to and from NIfTI
The command line tools `ecat2nifti`/`nifti2ecat` (ECAT7) and
`philips2nifti`/`nifti2philips` (Philips image `.img` and Syntegra `.syn`
files) convert complete files:
```bash
ecat2nifti    [-2] [--float] [--sidecar] [--no-extension] input.v [output.nii[.gz]|output.hdr[.gz]]
philips2nifti [-2] [--float] [--sidecar] [--no-extension] input.img|input.syn [output.nii[.gz]|output.hdr[.gz]]
nifti2ecat    input.nii[.gz]|input.hdr[.gz]|input.img[.gz] [output.v]
nifti2philips input.nii[.gz]|input.hdr[.gz]|input.img[.gz] [output.img|output.syn]
```
- The four names are one program, [`medioConvert`](medioConvert.md), which
  selects the conversion by the name it is called with: they are installed as
  symbolic links to it (as copies on Windows). `medioConvert` itself takes
  the same options and writes the same files (`medioConvert --float input.v
  output.nii`). The installation also has the names with the version of
  libmedio, e.g. `ecat2nifti-<version>` and `nifti2ecat-<version>`.
- NIfTI files hold the physical values (stored value × scale factor ×
  calibration factor, e.g. Bq/ml); data with a scale factor per frame or
  (Philips) per slice, or with `--float`, is written as float32.
- ECAT7 voxel (i, j, k) is written as NIfTI voxel (i, j, nz-1-k) with qform
  and sform describing its position. NIfTI files of any orientation are
  reordered into the ECAT7 voxel order.
- The attributes of the source (see [Format independent header attributes](header-attributes.md),
  incl. patient data and all native header fields) and the frame timing are
  stored as JSON header extension with BIDS PET keys (`--sidecar`: also as
  `<output>.json`) and restored by `nifti2ecat`/`nifti2philips`. JSON written
  by earlier versions (ECAT7 fields at the top level) is still read.
- For other tools, a DICOM header extension holds the same attributes as
  DICOM data set and the frame timing in the private tags of PMOD, and
  `descrip` holds `<patient name> [<study type>] <study description>` like
  in the NIfTI files of PMOD. PMOD reads the units and the frame timing from
  them; in Python, nibabel returns the extensions of a file:
  ```python
  import json, nibabel as nib
  ext = {e.get_code(): e for e in nib.load("out.nii.gz").header.extensions}
  metadata = json.loads(ext[6].get_content())  # BIDS keys, e.g. metadata["FrameTimesStart"]
  ds = ext[2].get_content()                    # pydicom data set, e.g. ds.PatientWeight
  ```
- NIfTI files of other tools without this JSON metadata provide the values
  of their DICOM header extension (e.g. the units and frame timing of PMOD)
  and, for files of PMOD, the patient name, study type and description of
  `descrip`. `--no-extension` writes neither extension.
- JSON sidecars of other tools (BIDS PET, dcm2niix) are read as well:
  - without `FrameTimesStart` (dcm2niix before 2023), the frame start times
    are `FrameReferenceTime - FrameDuration / 2`, or follow each other;
  - the radionuclide comes from `TracerRadionuclide`, or from the half-life
    of a PET radionuclide (dcm2niix writes only `RadionuclideHalfLife`);
  - the injection time comes from `TimeZero` and `InjectionStart`;
  - the time the values are decay corrected to (`DecayCorrectionDateTime`)
    comes from `TimeZero` and `ImageDecayCorrectionTime`;
  - `FrameReferenceTime` (s, one value per frame, dcm2niix) is the frame
    attribute `FrameReferenceTime` (ms) of the frames in `Frames`.

  Dates and times without time zone (DICOM, dcm2niix, microPET) are taken as
  local time of the machine (`TZ`), BIDS `TimeZero` is written in local time.
- Philips files are converted via the ECAT7 representation of libmedio; the
  x/y image position of Philips slices is not used.

Applications get the same metadata when they write NIfTI files themselves:

- With the generic interface (`writeMainHeader()`/`writeVolume()` of
  `CMedIOData`), see [Format independent access to the image data](format-independent-access.md).
- With the NIfTI classes directly:
  - `CNIFTIFile::writeMainHeader()` and the `save()` of a NIfTI header store
    its attributes as JSON and DICOM extensions and set `descrip`.
  - `convertFrom()` of a NIfTI header takes over the attributes of the ECAT7,
    Philips or other NIfTI version headers. While the source main header
    belongs to an open file, this includes the timing of all frames.
  - `CNIFTIFile::writeMatrix(data, frame, start, duration)` stores the timing
    of a volume.
  - A header changed and saved again (e.g. anonymized) replaces the metadata
    extensions completely.
  - `convertFrom()` of the ECAT7 headers takes over the attributes and frame
    timing of a NIfTI header.

```cpp
CNIFTIFile nifti("out.nii.gz", CNIFTIMainHeader::NIFTI1);
nifti.open(QIODevice::WriteOnly);
QScopedPointer<CNIFTIMainHeader> header(nifti.createEmptyHeader());
header->convertFrom(ecatMainHeader, ecatSubHeader);   // geometry, attributes, timing
header->updateAttributes(QJsonObject{{"PatientWeight", 72.5}});
nifti.writeMainHeader(*header);                      // header + JSON/DICOM extensions
nifti.writeMatrix(volume, 1, 0.0, 30.0);             // volume 1, start 0 s, duration 30 s
```

An output name ending with `.hdr`/`.img` writes a header/image pair. The
details are documented in `src/nifti/CNIFTIConverter.h`. Analyze 7.5 files
(`.hdr`/`.img` without NIfTI magic) are not supported. The NIfTI support was
started in the 2026 summer student project, see
[summer-student-2026](summer-student-2026/README.md).
