# Placeholder: dynamic ECAT7 image

This dataset directory waits for its data. It is skipped by the tests as
long as this file exists and `image.v` is missing.

## Files to add

- `image.v`: dynamic ECAT7 image with several frames, ideally with
  different scale factors per frame.
- `reference/<tool>.nii.gz`: the same acquisition (same reconstruction)
  as 4D file written by a tool whose geometry is trusted, e.g. converted
  from the original DICOM data with dcm2niix, or exported by PMOD or Vinci.

## Then

1. Fill in `dataset.json`: `description`, `source` and the `references`
   entry (file name, tool incl. version and options). If known, add
   `landmarks` (a point with its voxel index in `image.v` and its world
   coordinates in mm in the reference file).
2. Run `python3 testsuite/data/tools/make_expected.py testsuite/data/ecat7/dynamic`.
   This adds the expected values to `dataset.json` and removes the
   `PLACEHOLDER.md` files.
3. Rename the directory if a more descriptive name fits (e.g.
   `phantom-dynamic`), run `ctest` and commit the directory.

See `testsuite/data/README.md` for the conventions (size limits,
anonymization).
