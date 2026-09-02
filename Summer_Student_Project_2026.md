# Summer Student Project 2026 — NIfTI Support in libmedio

This document summarizes the work carried out during the 2026 Summer Student Project at the Positron Emission Tomography department of the Institute of Radiopharmaceutical Cancer Research, HZDR.

The main goal of the project was to extend `libmedio` with support for the NIfTI medical imaging format and to develop conversion tools between the existing ECAT implementation and NIfTI.

The implementation is available in the `nifti` branch.

---

## 1. NIfTI-1 and NIfTI-2 support

Support for both **NIfTI-1** and **NIfTI-2** was implemented in `libmedio`.

The implementation includes:

- NIfTI-1 and NIfTI-2 header reading and writing
- automatic recognition of NIfTI-1 and NIfTI-2 files
- voxel matrix reading and writing
- handling of `vox_offset`
- support for NIfTI header extensions
- handling of the main spatial transformation fields
- conversion between ECAT data types and NIfTI data types

The main implementation can be found under:

```text
src/nifti/
├── CNIFTIFile.cpp
├── CNIFTIFile.h
├── nifti1/
│   ├── CNIFTI1MainHeader.cpp
│   └── CNIFTI1MainHeader.h
└── nifti2/
    ├── CNIFTI2MainHeader.cpp
    └── CNIFTI2MainHeader.h
```

NIfTI-1 uses the standard 348-byte header, while NIfTI-2 uses the 540-byte header.

The same `CNIFTIFile` interface is used to handle both formats.

## 2. ECAT → NIfTI conversion

The `ecat2nifti` example was implemented to convert ECAT7 image files to either NIfTI-1 or NIfTI-2.

The converter is located in:

```text
examples/ecat2nifti/ecat2nifti.cpp
```

Basic usage:

```bash
ecat2nifti input.v output.nii
```
creates a NIfTI-1 file.

NIfTI-2 can be selected using:

```bash
ecat2nifti input.v output.nii -2
```

The converter handles:

- voxel dimensions
- voxel sizes
- ECAT → NIfTI data type conversion
- ECAT scale factors
- spatial orientation
- ECAT matrix block padding
- PET metadata preservation
- NIfTI-1 and NIfTI-2 header generation

ECAT matrices may contain additional bytes because ECAT stores matrices in 512-byte blocks. These trailing bytes are detected and excluded from the actual NIfTI voxel matrix.

## 3. NIfTI → ECAT conversion

A reverse converter was also implemented:

```text
examples/nifti2ecat/nifti2ecat.cpp
```

Usage:

```bash
nifti2ecat input.nii output.v
```

The converter reconstructs an ECAT7 image from NIfTI spatial information and voxel data.

When ECAT-specific metadata is available in the NIfTI JSON metadata extension or in the external JSON sidecar, it is also restored into the ECAT main header and image subheader.

If no ECAT-specific metadata is available, conversion is still possible using the information contained in the standard NIfTI header.

## 4. Spatial alignment and orientation

An important part of the project was ensuring that ECAT and NIfTI images describe the same physical image geometry.

ECAT and NIfTI use different conventions for image orientation. Therefore, the conversion includes an explicit reversal of the voxel order along the Z direction.

The corresponding NIfTI spatial transformations are represented using both:

```text
qform
sform
```

with:

```text
qform_code = 1
sform_code = 1
```

The transformation matrices and offsets are generated from the ECAT geometry so that the physical coordinates represented by the converted image remain consistent.

The reverse NIfTI → ECAT conversion applies the corresponding inverse Z reordering.

Round-trip tests confirmed that the voxel data are restored correctly after:

```text
ECAT → NIfTI → ECAT → NIfTI
```

for the tested static PET datasets.

## 5. Normalization mode

An optional normalization mode was implemented using:

```bash
ecat2nifti input.v output.nii --normalize
```

The default conversion preserves the original ECAT integer representation whenever possible.

For example, an ECAT short image can be represented in NIfTI using:

```text
datatype  = INT16
scl_slope = ECAT scale factor
```

so that the physical voxel value remains:

physical value = stored value × scl_slope

With --normalize, the scale factor is absorbed directly into the voxel values and the output is stored as floating-point data:

```text
datatype  = FLOAT32
scl_slope = 1
scl_inter = 0
```

This provides an alternative representation in which the stored voxel values directly correspond to the physical values.

## 6. PET metadata preservation

A standard NIfTI header cannot represent all ECAT/PET-specific acquisition information.

To avoid losing this information during:

```text
ECAT → NIfTI
```

JSON metadata support was implemented.

The converter currently generates an external JSON metadata sidecar containing relevant ECAT/PET information such as:

- isotope name
- isotope half-life
- radiopharmaceutical
- injected dose
- scan start time
- dose start time
- calibration information
- acquisition type
- patient/study information
- scanner geometry
- ECAT scale factor
- reconstruction zoom
- frame timing information

These fields allow ECAT-specific information to be reconstructed when converting the image back to ECAT.

The JSON structure is intended primarily for metadata preservation within the ECAT/NIfTI conversion workflow.

It should not be interpreted as a complete BIDS PET implementation.

## 7. NIfTI header extensions

JSON metadata can also be stored directly inside the NIfTI file using the NIfTI header extension mechanism.

The resulting single-file structure is approximately:

NIfTI-1:

- 348-byte main header
- 4-byte extension indicator
- JSON extension
- voxel data

NIfTI-2:

- 540-byte main header
- 4-byte extension indicator
- JSON extension
- voxel data

The extension size is aligned according to the NIfTI extension requirements, and `vox_offset` is updated so that voxel data start after the complete extension.

The JSON extension currently uses:

```text
ecode = 6
```

The reverse nifti2ecat converter first checks for embedded JSON metadata and then falls back to an external JSON sidecar if necessary.

## 8. Verification utility

The NIfTI header inspection example was implemented:

```text
examples/nifti/showheader/showheader_nifti.cpp
```
It can be used to inspect NIfTI-1 and NIfTI-2 headers and to export the voxel matrix for comparison.

Example:

```bash
showheader_nifti input.nii
```
or:

```bash
showheader_nifti input.nii copy.nii voxel.raw
```

The latter also writes a copy of the NIfTI file and exports its voxel payload.

## 9. Validation performed

The following workflows were tested:

```text
ECAT → NIfTI-1
ECAT → NIfTI-2
NIfTI → ECAT
ECAT → NIfTI-1 → ECAT
ECAT → NIfTI-2 → ECAT
ECAT → NIfTI-2 → ECAT → NIfTI-2
```

For the tested static PET dataset, the voxel matrix before and after the NIfTI-2 round trip was identical byte by byte.

The same tests were also used to check:

- dimensions
- voxel sizes
- data type
- scale factor
- spatial transformations
- PET metadata restoration
- NIfTI header extension reading/writing

## 10. Important implementation files

The most relevant files for reviewing the work are:

```text
src/nifti/CNIFTIFile.cpp
src/nifti/CNIFTIFile.h

src/nifti/nifti1/CNIFTI1MainHeader.cpp
src/nifti/nifti1/CNIFTI1MainHeader.h

src/nifti/nifti2/CNIFTI2MainHeader.cpp
src/nifti/nifti2/CNIFTI2MainHeader.h

examples/ecat2nifti/ecat2nifti.cpp
examples/nifti2ecat/nifti2ecat.cpp
examples/nifti/showheader/showheader_nifti.cpp
```

A useful starting point for reviewing the conversion logic is:

```text
examples/ecat2nifti/ecat2nifti.cpp
```

followed by the `convertFrom()` implementations in the NIfTI-1 and NIfTI-2 main-header classes.

## 11. Compressed NIfTI .nii.gz support

libmedio now supports gzip-compressed NIfTI files (`.nii.gz`) for both
NIfTI-1 and NIfTI-2.

Compressed files are decompressed to a temporary `.nii` file for reading. When writing `.nii.gz`, the NIfTI file is first generated as an uncompressed temporary file and subsequently compressed using zlib.

The implementation was validated by comparing the uncompressed output with the decompressed `.nii.gz` output byte-for-byte and using SHA-256 checksums.

## 12. Current limitations and possible next steps

The currently validated conversion workflow mainly concerns static 3D PET images.

Possible next steps include:

- complete multi-frame / dynamic PET support
- full ECAT multi-frame → NIfTI 4D conversion
- reconstruction of multiple ECAT frames from NIfTI 4D datasets
- per-frame scale factor handling
- further standardization of PET metadata
- evaluation of BIDS PET compatibility
- additional automated tests
- Interfile support
- HDF5 support

In particular, full dynamic 4D conversion requires special handling because ECAT may use a different scale factor and timing information for each individual frame, whereas standard NIfTI scaling is global.

## 13. Branch

Development for this project was carried out in:

```text
nifti
```

The relevant implementation commits can therefore be reviewed directly from the history of this branch.


