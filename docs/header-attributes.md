# Format independent header attributes
Every header (`CMedIOHeader`) describes its attributes as JSON object, so that
applications can use the attributes of any format without the header classes
of the formats:
```json
{
  "PatientWeight": 8.63,
  "RadionuclideTotalDose": 60780000,
  "AcquisitionDateTime": "2026-06-03T17:04:56Z",
  "Units": "BQML",
  "ECAT7": { "Patient_Weight": 8.63, "Dosage": 60780000, "Data_Units": "Bq/cc" }
}
```
```cpp
CMedIOHeader* mainHeader = NULL;
file->readMainHeader(mainHeader);                     // any format
QJsonObject attributes = mainHeader->attributes();
double weight = mainHeader->attribute("PatientWeight").toDouble();   // keyword
QJsonValue dose = mainHeader->attribute("(0018,1074)");              // or DICOM tag
mainHeader->setAttribute("PatientName", QString("Doe^Jane"));
attributes["PatientWeight"] = 72.5;
mainHeader->updateAttributes(attributes);            // whole object or single keys
```
- The keys are DICOM keywords, e.g. `PatientWeight` for the DICOM attribute
  (0010,1030), with the meaning and unit of the DICOM attribute (kg, Bq, s,
  ms, ...) and a JSON value: numbers, ISO 8601 dates and times (`Z` for UTC,
  without time zone if the format has none; these are taken as local time of
  the machine, as DICOM times are the local time of the scanner, so set `TZ`
  when converting data of another time zone), arrays for multiple values,
  objects for code sequences. `CMedIOHeaderDictionary` lists the keys with their
  DICOM tag (as number and as string `"(0010,1030)"`), VR, unit and the
  sequence holding them.
- The values are converted from and into the units of the formats (e.g.
  Philips weight in g, ECAT7 height in cm, microPET dose in mCi or MBq).
- The native fields of the format are in a section named after the format
  (`ECAT7`, `Philips`, `microPET`, `NIfTI`), without the fields describing the
  size, data type, scaling, geometry or layout of the image data.
- Values a format cannot store (keys of other formats, unknown keys, values
  it cannot represent) are kept in the header object and carried along by
  the conversions between the formats (`readMainHeader(header, format)`,
  `readMatrix(..., format)`, `writeMatrix()`, `CNIFTIConverter`). NIfTI files
  store all attributes in their JSON header extension, incl. the frame
  attributes of all frames (`Frames`) and BIDS keys for other tools, and the
  attributes of the dictionary in a DICOM header extension (see
  `src/nifti/CNIFTIExtensions.h`); this holds for every file written by
  libmedio, whether by the converters, `writeMainHeader()`/`writeVolume()`
  or `pmedio.create()`.
- `DecayCorrectionDateTime` is the time the values are decay corrected to. It
  is only given by formats that store it (the reference date and time of
  Siemens Interfile headers, `ImageDecayCorrectionTime` of BIDS sidecars) and
  never derived from other values.
- The other formats store only their own fields. The physical values of
  Philips slices scaled to SUV depend on the dose, times, half life and
  weight of the main header: change them before the frames are written.

The details are documented in `src/CMedIOHeader.h`, `src/CMedIOHeaderDictionary.h`
and `src/CMedIOHeaderMapping.h`. In Python, `pmedio.open(path).attributes()` and `pmedio.load(path).attributes`
returns the attributes of a file as dict (incl. the frame attributes in
`Frames`), and `pmedio.dictionary_entry("(0010,1030)")` an entry of the
dictionary; files created with `pmedio.create(path, like=source)` get the
attributes of the source (see [Python](python.md)); in R, `readMedio(path)$attributes`
holds them as named list (see [r/README.md](../r/README.md)).
