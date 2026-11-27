# Converting between all formats: medioConvert
The command line tool `medioConvert` converts a complete image file of any
format libmedio reads (ECAT7, NIfTI, Philips, Interfile, microPET, HDF5) into
any format it writes (ECAT7, NIfTI, Philips, Interfile, HDF5):
```bash
medioConvert [-f|--format ecat|nifti|philips|interfile|hdf5] [-2] [--float] [--sidecar] [--no-extension] <input> <output>
```
The format of the output is the one of its file name (`.v`, `.nii`,
`.nii.gz`, `.hdr`, `.img`, `.syn`, `.hv`, `.h5`, `.hdf5`, `.hdf`) unless
`--format` gives it. HDF5 files need a libmedio built with HDF5 support
(`medioConvert --help` says whether it has it). `-2`, `--float`, `--sidecar`
and `--no-extension` are the options for ECAT7 and Philips files converted into
NIfTI, see [NIfTI](nifti.md); other conversions refuse them.

## One program per conversion
The same program is installed under a name per conversion,
`<source>2<target>`, as symbolic link to `medioConvert` (as copy on Windows):
```bash
ecat2nifti --float pet.v pet.nii.gz
ecat2hdf5 pet.v                # writes pet.h5
hdf52nifti pet.h5 pet.nii.gz
interfile2ecat pet.hv out.v
```
- Called as `<source>2<target>` the program takes `<input> [output]`. The
  input has to be a file of the source format, and the output is written in
  the target format whatever its name is; it defaults to the name of the input
  with the extension of the target (`.v`, `.nii`, `.img`, `.hv`, `.h5`).
- Sources are `ecat`, `nifti`, `philips`, `interfile`, `micropet` and `hdf5`,
  targets `ecat`, `nifti`, `philips`, `interfile` and `hdf5`. The names with
  `hdf5` are only installed if libmedio has HDF5 support.
- `ecat2nifti`, `nifti2ecat`, `philips2nifti` and `nifti2philips` were
  programs of their own before. They keep their arguments, their default
  output names and the files they write, see [NIfTI](nifti.md).
- Like the other tools, all names are also installed with the version of
  libmedio as suffix (e.g. `ecat2hdf5-<version>`).

## What is converted
ECAT7 and Philips files are converted into NIfTI and back by
`CNIFTIConverter` with all metadata, as described in [NIfTI](nifti.md);
`medioConvert pet.v pet.nii` writes the same file as `ecat2nifti pet.v pet.nii`.

For all other conversions the frames are read and written as volumes
(`CMedIOData::readVolume()` and `writeVolume()`, see
[Format independent access](format-independent-access.md)), and the main header
is converted into the one of the target format:
- the physical values, the position of every voxel, the start and duration
  of every frame and the header attributes the target format stores are kept;
- the values are stored as the target writes volumes (e.g. float32 in HDF5
  files and in NIfTI files of several frames);
- the attributes of the frames are kept as far as the target has a place
  for them (`DecayFactor`: ECAT7, NIfTI, HDF5; `DeadTimeFactor`: Philips,
  NIfTI, HDF5), and Philips files store no unit;
- of files with several gates or bed positions, the frames of the first gate
  and bed are converted.

An existing output file is replaced. The output must not be one of the files
of the input (e.g. the data file `.v` of an Interfile header `.hv`). If the
conversion fails, the program prints a message, returns the exit status 1 and
leaves no output file behind.
