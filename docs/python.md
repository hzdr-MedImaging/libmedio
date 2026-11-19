# Python: pmedio
The Python module `pmedio` (`python/`, nanobind) offers the format
independent access with numpy arrays.

## Whole images: load() and save()
`pmedio.load()` and `pmedio.save()` read and write all frames of an image at once:
```python
import pmedio

image = pmedio.load("image.v", orientation="RAS")    # ECAT7, NIfTI, Philips, microPET
image.data                                            # physical values (nx, ny, nz, frames), float32
image.affine                                          # 3 x 4, voxel -> LPS (mm)
image.frame_start, image.frame_duration               # s, one per frame
image.attributes["PatientWeight"]                     # header attributes (dict)
image.data = image.data * 2.0
image.save("image.nii.gz")                            # or pmedio.save(path, image)
```
- `pmedio.load(path, frames=[...], orientation=...)` selects frames by their
  numbers and reorders the voxels (default: the voxel order of the file). All
  frames need the same geometry; files with other frames are read with
  `pmedio.open()`.
- `pmedio.Image(data, affine=..., frame_start=..., frame_duration=...,
  attributes=...)` makes an image from a 3D or 4D array; `pmedio.save(path,
  array)` writes an array with identity affine. `save()` writes like
  `pmedio.create()` (see below), with `like=`, `format=` and `overwrite=`: the
  main header is made from the image or converted from `like`, the attributes
  of the image are set in it. Invalid images create no file, and no file is
  left behind if writing fails.
- `image.to_nibabel()` returns the image as `nibabel.Nifti1Image` (the affine
  in the RAS coordinates of nibabel, 3D for one frame), and
  `pmedio.Image.from_nibabel(nibabel_image, frame_start=..., frame_duration=...,
  attributes=...)` makes an image from any 3D or 4D nibabel image (its scaled
  values, the affine converted into LPS). Both need nibabel
  (`pip install nibabel`), which is no requirement of pmedio otherwise.

## Single frames: open() and create()
`pmedio.open()` and `pmedio.create()` read and write single volumes
(`pmedio.Volume`):
```python
import numpy as np
import pmedio

with pmedio.open("image.nii.gz") as f:              # ECAT7, NIfTI, Philips, microPET
    for frame in f.frame_numbers():
        v = f.read_volume(frame)                      # pmedio.Volume, voxel order of the file
        values = v.values()                           # physical values, shape (nx, ny, nz)
        v.affine                                      # 3 x 4, voxel -> LPS (mm)
        v.reorient("RAS")                             # e.g. the nibabel convention
        v.frame_start, v.frame_duration               # s

    # a file in another format with the main header of the source
    with pmedio.create("image.v", like=f) as out:
        for k, frame in enumerate(f.frame_numbers()):
            out.write_volume(f.read_volume(frame), k + 1)

# a file made from volumes alone, with header attributes
v = pmedio.Volume(np.zeros((64, 64, 32), dtype=np.float32), affine=np.eye(3, 4))
with pmedio.create("new.nii.gz", frames=2, attributes={"PatientWeight": 72.5}) as out:
    out.write_volume(v, 1)
    out.write_volume(v, 2)
```
- `pmedio.Volume` corresponds to `CMedIOVolume`: `stored()` and `values()`
  return copies of the voxels (x running fastest), `scale_factors`,
  `intercept`, `affine`, `position()`, `voxel_size`, `orientation`,
  `reorient()` and the frame timing.
- `pmedio.create(path, like=source)` takes the format from the file name
  (`.v`, `.nii`/`.nii.gz`/`.hdr`, `.img`/`.syn`) or from `format=pmedio.Format...`,
  and replaces an existing file only with `overwrite=True`. The volumes
  written are converted into the representation of the file (e.g. the
  datatype and scaling of a NIfTI main header).
- Without `like`, the main header is made from the first volume written
  (`CMedIOData::mainHeaderFromVolume()`). `frames=n` gives the number of
  frames, which a NIfTI file needs in advance; a NIfTI file of several frames
  stores the physical values as float32. A file closed without a volume, and
  a NIfTI file without all of its frames (with `like`: the frames of the
  source), is removed.
- `attributes={...}` sets header attributes (DICOM keywords such as
  `"PatientWeight"`, or native fields in the section of the format, as
  returned by `File.attributes()`); invalid values raise an error before the
  file is created. `File.attributes()` of a file created for writing returns
  the attributes of its main header as written.
- NIfTI files keep the frame timing of the volumes written as JSON header
  extension (`FrameTimesStart`, `FrameDuration`, as written by
  `CNIFTIConverter`), see `CMedIOData::writeMatrix()` and [NIfTI](nifti.md).

## Legacy interface (ECAT7 only)
`pmedio.MedIOImage`, `pmedio.read()` and `pmedio.write()` are the older
interface for ECAT7 files only. They are kept unchanged for existing scripts;
new code uses `load()`/`save()`, which also read and write ECAT7 files:

| ECAT7 only (legacy) | All formats |
| --- | --- |
| `img = pmedio.read(path)`, `np.asarray(img)` or `img.toarray()` | `image = pmedio.load(path, orientation="RAS")`, `image.data`: the same values in the same voxel order |
| `pmedio.read(path, volumes=[3, 1])` | `pmedio.load(path, frames=[3, 1], orientation="RAS")` |
| `cols=`, `rows=`, `planes=` (numbered from 1) | slices of `image.data`, e.g. `image.data[1:3]` for `cols=[2, 3]` |
| `img.mhead`, `img.shead` (native ECAT7 fields) | `image.attributes` (DICOM keywords, native fields in the section `"ECAT7"`), `image.frame_start`, `image.frame_duration` (s) |
| `pmedio.write(img, path)` | `image.save(path)` or `pmedio.save(path, image)`, format from the file name |
| `dataType="short"` etc. | the data type of `image.data`: integer arrays are stored as they are, float arrays as float |

## Header attributes
`File.attributes()` and `Image.attributes` hold the attributes of a file as
dict (DICOM keywords, native fields in the section of the format, frame
attributes in `Frames`), and `pmedio.dictionary_entry("(0010,1030)")` returns
an entry of the dictionary, see [Format independent header attributes](header-attributes.md).

## Installation and build
Install libmedio into `PREFIX_PATH` first, then install the package
from the checkout with pip, which builds it with scikit-build-core:
```bash
pip install ./python                              # libmedio in /usr/local/petlib
BUILD_PREFIX=/path/to/prefix pip install ./python # libmedio in another prefix
```
`pip install -e ./python` installs it for development, `pip wheel ./python`
builds a wheel, and `CMAKE_ARGS` passes further options to CMake. With Python
3.12 or newer the module is built for the stable ABI of Python: the wheel
(`cp312-abi3`) can be installed with all newer Python versions as well; Python
3.10 and 3.11 get a wheel for their version. The package comes with type stubs
(`pmedio-stubs`) for editors and type checkers like mypy and pyright, generated
from the module when it is built. The module
can also be built with CMake alone (see `.github/workflows/ci.yml`); run the
tests with `pytest python/tests`. The bindings need Python 3.10 or newer and a
C++17 compiler; pip provides the build requirements (scikit-build-core,
nanobind, CMake), a plain CMake build needs `pip install nanobind` (and builds
for the stable ABI with `-DPMEDIO_STABLE_ABI=ON`). The library
itself stays C++11. The conda package `pmedio` is built with `python/conda.recipe` (see
[conda.recipe](../conda.recipe/README.md)).
