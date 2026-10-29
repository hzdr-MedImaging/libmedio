# Placeholder: NIfTI test data

The following files are listed in `../datasets.json` and still have to be
added here:

- `pet-static.nii.gz`: the same acquisition (same reconstruction) as
  `../ecat/pet-static.v`, written by a tool whose geometry is trusted,
  e.g. converted from the original DICOM data with dcm2niix, or exported
  by PMOD or Vinci.
- `pet-dynamic.nii.gz`: the same for `../ecat/pet-dynamic.v` (4D file).

Enter the tool incl. version and options as `tool` of the file in
`../datasets.json`. If known, add `landmarks` (a point with its world
coordinates in mm and its voxel indices in both files). Then run
`python3 testsuite/data/tools/make_expected.py`, which generates the
expected values and removes this file once all NIfTI files listed in
`datasets.json` exist.

See `../README.md` for the conventions (size limits, anonymization).
