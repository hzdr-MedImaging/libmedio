# Format independent access to the image data
Applications do not need to know the format of a file to work with its
image data in the representation they are used to. `readMatrix()` of every
file class (`CECATFile`, `CNIFTIFile`, `CPhilipsFile`, `CInterfileFile`, `CConcordeFile`)
returns a frame in the representation of the requested format:
```cpp
// any supported image file, e.g. NIfTI, Philips, Interfile or microPET
CMedIOData* file = CMedIODataFactory::create(fileName);
file->open(QIODevice::ReadOnly);

CMedIOHeader* mainHeader = NULL;                   // here: CECAT7MainHeader
file->readMainHeader(mainHeader, CMedIOData::ECAT);

foreach(short frame, file->frameNumbers()) {       // frame numbers of the file
  QByteArray* data = NULL;
  CMedIOHeader* subHeader = NULL;                  // here: CECAT7SubHeaderImage
  if(file->readMatrix(data, subHeader, CMedIOData::ECAT, frame)) {
    // ... ECAT7 voxel order, data type and scale factor ...
    delete data;
    delete subHeader;
  }
}
```
and `writeMatrix()` writes a frame given in the representation of any
format, e.g. ECAT7 data into a NIfTI file:
```cpp
CECATMainHeader* ecatMainHeader = NULL;            // bed position, calibration factor
ecatFile.readMainHeader(ecatMainHeader);
CMedIOHeader* niftiHeader = NULL;                  // NIfTI main header of the ECAT7 file
ecatFile.readMainHeader(niftiHeader, CMedIOData::NIFTI);

CNIFTIFile nifti("image.nii.gz", CNIFTIMainHeader::NIFTI1);
nifti.open(QIODevice::WriteOnly);
niftiHeader->setMedIOData(&nifti);                 // returned headers are not linked to a file
nifti.writeMainHeader(*static_cast<CNIFTIMainHeader*>(niftiHeader));

const QList<short> frames = ecatFile.frameNumbers();
for(int k=0; k < frames.size(); k++) {
  QByteArray* data = NULL;
  CECATSubHeader* subHeader = NULL;
  ecatFile.readMatrix(data, subHeader, frames[k]); // ECAT7 matrix
  nifti.writeMatrix(*data, *subHeader, k + 1, ecatMainHeader); // NIfTI volume k+1
  delete data;
  delete subHeader;
}
```
- `frameNumbers()` returns the numbers of the frames of a file. ECAT7 and
  Philips files address their frames by the frame numbers stored in the
  file, which do not have to start with 1 and can have gaps (`numFrames()`
  of `CECATFile`/`CPhilipsFile` is the highest frame number); NIfTI volumes,
  microPET and Interfile frames are numbered 1..n. The main headers returned for
  another format (and the main headers written by libmedio) give the number
  of frames.
- Every frame is converted via its ECAT7 representation, using the existing
  header conversions (`convertFrom()`); the geometry of every voxel is kept.
- Voxel order: ECAT7, Philips and microPET: x towards patient left, y
  towards posterior, planes from superior to inferior; NIfTI: the same
  with reversed planes (as written by `CNIFTIConverter`), or the
  orientation of the NIfTI file written to, whatever orientation it has;
  Interfile: the order of the file (PMOD: planes from inferior to superior,
  Siemens reconstructions: from superior to inferior, see
  [interfile.md](interfile.md)).
- Data types: the stored values are kept if the target format can represent
  them exactly (integers are widened if needed), otherwise the physical
  values are stored as float32. Writing into a file with a given data type
  (NIfTI) converts the values into its data type and scaling and fails
  without writing if a value does not fit.
- Philips matrices hold all slices of a frame with one scale factor;
  Philips frames can be written in any order and overwritten with data of
  the same slices. microPET files can only be read.
- Without a format, `readMainHeader(header)` and `readMatrix(data, header, frame)`
  return the main header and frames in the format of the file itself, e.g.
  for a file created by `CMedIODataFactory` whose format is not known.
- `writeMatrix()` takes the format of the matrix from the type of its sub
  header. Like `readMatrix()`, it can also be given explicitly:
  `writeMatrix(data, header, CMedIOData::ECAT, frame, mainHeader)` fails
  without writing if the sub header does not belong to that format. The main
  header passed with the matrix has to be a main header of the format of the
  matrix as well, otherwise nothing is written.
- `readVolume()`/`writeVolume()` exchange frames as `CMedIOVolume`: voxel
  data, scaling, the position of every voxel (DICOM patient coordinates)
  and the frame timing; `CMedIOVolume::reorient()` reorders the voxels.
- `CMedIOData::mainHeaderFromVolume(volume, format, numFrames, header)` makes
  the main header of a new ECAT7, NIfTI or Philips file from a volume, for
  writing volumes without a source file. The header has the geometry and
  number of frames, the defaults of the format otherwise (change them with
  `updateAttributes()`); a NIfTI file stores all frames with the data type
  and scaling of this volume, Philips planes are centered:
  ```cpp
  QScopedPointer<CMedIOData> out(CMedIODataFactory::createForWriting("new.nii"));
  CMedIOHeader* header = NULL;
  CMedIOData::mainHeaderFromVolume(volume, out->dataFormat(), 3, header);
  out->open(QIODevice::WriteOnly);
  out->writeMainHeader(*header);                     // any writable format
  out->writeVolume(volume, 1);                       // frames 1..3
  delete header;
  ```
- New files of any writable format: `CMedIODataFactory::formatOfFileName()`
  gives the format of a file name (`.v`, `.nii`, `.nii.gz`, `.hdr`, `.img`,
  `.syn`), `CMedIODataFactory::createForWriting()` the file object,
  `CMedIOData::writeMainHeader(header)` writes a main header of any format
  with the class of the file, `fileNames()` lists all files a file consists
  of (both files of a NIfTI header/image pair) to check for existing files,
  and `removeFiles()` removes them if writing fails. `requiredFrames()` gives
  the number of frames a NIfTI main header needs; `readAttributes()` returns
  the header attributes of a file incl. those of every frame (see
  [Format independent header attributes](header-attributes.md)).
- Files have to be opened first: on a file that is not open, the methods
  return false.
- Only image data is supported (no sinograms, normalizations, list mode).

The details are documented in `src/CMedIOData.h` and `src/CMedIOMatrix.h`.
