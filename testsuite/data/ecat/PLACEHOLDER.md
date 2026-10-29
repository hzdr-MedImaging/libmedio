# Placeholder: ECAT test data

The following files are listed in `../datasets.json` and still have to be
added here:

- `pet-static.v`: static ECAT7 image (single frame), ideally with
  `x_offset`/`y_offset` != 0 in the image sub header.
- `pet-dynamic.v`: dynamic ECAT7 image with several frames, ideally with
  different scale factors per frame.

Rename the datasets if more descriptive names fit (file names here and in
`nifti/` as well as the keys in `datasets.json`). Then fill in the `TODO`
entries in `../datasets.json` and run
`python3 testsuite/data/tools/make_expected.py`, which generates the
expected values and removes this file once all ECAT files listed in
`datasets.json` exist.

See `../README.md` for the conventions (size limits, anonymization).
