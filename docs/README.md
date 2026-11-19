# libmedio documentation

- [Format independent access to the image data](format-independent-access.md):
  reading and writing the frames of every format in C++ (`CMedIOData`,
  `CMedIOVolume`), new files of any writable format
- [Format independent header attributes](header-attributes.md): the attributes
  of every header as JSON object with DICOM keywords
- [Conversion to and from NIfTI](nifti.md): the command line tools, the metadata
  in JSON and DICOM header extensions, writing NIfTI files from applications
- [Python: pmedio](python.md): `load()`/`save()`, `open()`/`create()`, nibabel
  images, the legacy ECAT7 interface, installation
- [R: Rmedio](../r/README.md)
- [2026 summer student project](summer-student-2026/README.md): the start of
  the NIfTI support

The public headers in `src/` document the C++ API in detail.
