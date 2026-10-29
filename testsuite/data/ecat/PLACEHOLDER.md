# Placeholder: ECAT test data

The following files are listed in `../datasets.json` and still have to be
added here:

- `pet-dynamic.v`: dynamic ECAT7 image with several frames, ideally with
  different scale factors per frame.

Rename the dataset if a more descriptive name fits (file names here and in
`nifti/` as well as the key in `datasets.json`). Then fill in the `TODO`
entries in `../datasets.json` and run
`python3 testsuite/data/tools/make_expected.py`, which generates the
expected values and removes this file once all ECAT files listed in
`datasets.json` exist.

See `../README.md` for the conventions (size limits, anonymization).
