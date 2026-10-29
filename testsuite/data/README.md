# libmedio test data

This directory holds the data files used to validate libmedio. The same
acquisition (dataset) can be stored in several formats, so the tests can
use the files both for unit tests of the individual formats and to
compare the format conversions of libmedio with each other. The tests pick
up all files found here automatically; adding data requires no changes to
the test code.

## Layout

```
testsuite/data/
├── README.md
├── datasets.json                  # description of all datasets (maintained by hand)
├── tools/
│   └── make_expected.py           # generates the expected values of the data files
├── ecat/                          # one directory per format (named like src/<format>)
│   ├── pet-static.v               # data file of dataset "pet-static"
│   ├── pet-static.v.json          # expected values of this file (generated)
│   ├── pet-dynamic.v
│   └── pet-dynamic.v.json
├── nifti/
│   ├── pet-static.nii.gz          # the same acquisition as ecat/pet-static.v
│   ├── pet-static.nii.gz.json
│   └── ...
├── philips/
└── micropet/
```

- **One directory per format**, named like the corresponding source
  directory (`ecat`, `nifti`, `philips`, `micropet`, ...).
- **The dataset name is the file name up to its first dot.**
  `ecat/pet-static.v` and `nifti/pet-static.nii.gz` hold the same
  acquisition. Use lower case letters, digits and dashes for dataset
  names (e.g. `phantom-nema-static`, `brain-dynamic-28frames`).
- **Several files of the same dataset and format** (e.g. written by
  different tools) get an additional suffix after the dataset name:
  `nifti/pet-static.pmod.nii.gz`.
- **Formats with several files per dataset** (e.g. header and data file)
  use a directory named like the dataset (optionally with a suffix)
  instead of a single file, e.g. `nifti/pet-static.pair/` holding
  `pet-static.hdr` and `pet-static.img`.
- **Every data file has a `<file>.json` next to it** holding its expected
  values (for a dataset directory: `<directory>.json`).

## datasets.json

`datasets.json` describes all datasets and is maintained by hand:

```json
{
  "datasets": {
    "pet-static": {
      "description": "NEMA IEC body phantom, static, 10 min",
      "source": {
        "scanner": "ECAT EXACT HR+",
        "reconstruction": "OSEM 4 iterations/16 subsets, 5 mm Gauss",
        "units": "Bq/ml",
        "notes": "head of the phantom at high z"
      },
      "files": {
        "ecat/pet-static.v": {
          "origin": "original",
          "notes": "exported from the scanner"
        },
        "nifti/pet-static.nii.gz": {
          "origin": "converted",
          "tool": "dcm2niix v1.0.20250505 -z y",
          "same_reconstruction": true,
          "notes": "converted from the DICOM export of the same reconstruction"
        }
      },
      "landmarks": [
        {
          "name": "center of the 37 mm sphere",
          "world_mm": [0.0, 32.9, -12.4],
          "voxel": {
            "ecat/pet-static.v": [64, 80, 31],
            "nifti/pet-static.nii.gz": [64, 47, 31]
          }
        }
      ]
    }
  }
}
```

| Key           | Required | Content                                                              |
|---------------|----------|----------------------------------------------------------------------|
| `description` | yes      | what the dataset contains                                            |
| `source`      | no       | scanner, reconstruction, units and other free-form information       |
| `files`       | yes      | all files of the dataset, relative to `testsuite/data`               |
| `origin`      | yes      | `original` (written by the scanner/reconstruction) or `converted`    |
| `tool`        | for `converted` | tool incl. version and options used to write the file        |
| `same_reconstruction` | no | whether the file holds exactly the same reconstruction as the original |
| `layout`      | for `raw/` | `dims`, `data_type` (e.g. `float32`) and `byte_order` (`little`/`big`) of raw voxel data, first dimension varying fastest |
| `landmarks`   | no       | points with known world coordinates (mm) and their voxel indices per file |

The conversion tests compare the files of a dataset with each other, the
`origin`/`tool` entries tell which files are a trustworthy reference.

## Expected values

The `<file>.json` files are **generated** with `tools/make_expected.py`,
which reads the data files with [nibabel](https://nipy.org/nibabel) as an
independent implementation, so libmedio is not validated against itself:

```bash
pip install nibabel numpy
python3 testsuite/data/tools/make_expected.py            # all files without .json
python3 testsuite/data/tools/make_expected.py ecat/pet-static.v   # single file
```

Supported are ECAT7 image files (`ecat/`), NIfTI-1/NIfTI-2 (`nifti/`,
`.nii`, `.nii.gz` and header/image pair directories) and raw voxel data
(`raw/`, using the `layout` of the file in `datasets.json`). For other
files (e.g. `philips/`) the expected values are written by hand with
`"generator": "manual"` and a note where the values come from; the tests
only check the keys that are present. Do not edit generated files by hand,
run the script again instead (`--force` regenerates existing files).

- **ECAT7** (`"format": "ecat7"`): main header values (`file_type`,
  `num_frames`, `num_planes`, `calibration_factor`, `data_units`) and for
  every matrix its index (`frame`, `plane`, `gate`, `bed`, `data`), the
  sub header values (`data_type`, `dims`, `pixel_size_cm`,
  `scale_factor`, `frame_start_time_ms`, `frame_duration_ms`) and
  statistics of the stored, unscaled voxel values (`raw_min`, `raw_max`,
  `raw_sum`).
- **NIfTI** (`"format": "nifti1"`/`"nifti2"`): header values (`dim`,
  `pixdim`, `datatype`, `bitpix`, `scl_slope`, `scl_inter`, `qform_code`,
  `sform_code`, `quatern`, `qoffset`, `srow`, `xyzt_units`, `byte_order`)
  and statistics of the stored voxel values per volume
  (`"format": "nifti1-pair"`/`"nifti2-pair"` for header/image pairs).
- **Raw** (`"format": "raw"`): the layout and statistics of the voxel
  values per volume.

## Placeholders

Files which are listed in `datasets.json` but do not exist yet are
reported as pending by the tests (and otherwise ignored). A
`PLACEHOLDER.md` in a format directory describes which files are
expected there; `make_expected.py` removes it once all files listed for
that format exist.

## Consistency checks

The `testdata_layout` test reports an error if

- `datasets.json` is missing or invalid,
- a data file is not listed in `datasets.json` (under the dataset given by
  its name) or has no `<file>.json` with expected values,
- a `<file>.json` exists without its data file.

## Adding data

1. Copy the files into the format directories, named after the dataset.
2. Add the dataset and its files to `datasets.json`.
3. Run `python3 testsuite/data/tools/make_expected.py`.
4. Build and run the tests (`ctest`), then commit the files.

## Rules

- **Size:** keep datasets small; a phantom or a small matrix with a few
  frames is sufficient for validation. Aim for a few MB per file; GitHub
  rejects files larger than 100 MB. Git LFS is intentionally not used.
- **Anonymization:** the repository is public. Data files must not contain
  personal data: anonymize patient name, ID, birth date and similar fields
  before adding a dataset.
- **License:** only add data that may be redistributed under the license
  of this repository.

## Larger local datasets

The location of the test data can be changed when configuring the build:

```bash
cmake -S . -B build -DMEDIO_TEST_DATA_DIR=/path/to/testdata
```

The directory has to follow the layout described above. This allows
running the tests on larger datasets which are not part of the repository.
