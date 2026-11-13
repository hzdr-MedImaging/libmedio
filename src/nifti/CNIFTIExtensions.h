/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio

  Copyright (C) 2004-2026 hzdr.de and contributors

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

//! @file CNIFTIExtensions.h
//! @brief private: header extensions holding the metadata of NIfTI files
//!
//! libmedio stores the metadata of a NIfTI file (the header attributes, see
//! CMedIOHeaderDictionary, and the frame timing) in two header extensions:
//!
//! 1. a JSON extension (code 6, NIFTI_ECODE_COMMENT): the metadata as JSON
//!    object with BIDS keys (see MedIOHeaderMapping::niftiJson()), read with
//!    nibabel by json.loads(extension.get_content()) or extension.json().
//!    It is the first extension, as in the files of earlier versions.
//! 2. a DICOM extension (code 2, NIFTI_ECODE_DICOM): the attributes as DICOM
//!    data set (explicit VR little endian, also in big endian files, as
//!    nibabel reads it; no preamble),
//!    plus the frame timing in the private tags of PMOD:
//!    (0055,0010) private creator "PMOD_1", (0055,1001) frame start times (s)
//!    and (0055,1004) frame durations (ms), both FD. PMOD reads the frame
//!    timing and the units from it, nibabel returns it as pydicom data set
//!    (Nifti1DicomExtension).
//!
//! When reading, the JSON metadata (header extension or JSON sidecar file)
//! comes first; values it does not have are taken from a DICOM extension and,
//! in files written by PMOD, from descrip ("<patient name> [<study type>]
//! <study description>").

#ifndef CNIFTIEXTENSIONS_H
#define CNIFTIEXTENSIONS_H

#include <QByteArray>
#include <QIODevice>
#include <QJsonObject>
#include <QList>
#include <QString>

class CNIFTIFile;
class CNIFTIMainHeader;

namespace NIFTIExtensions {

const qint32 DicomCode = 2; //!< NIFTI_ECODE_DICOM
const qint32 JsonCode = 6;  //!< NIFTI_ECODE_COMMENT, used for the JSON metadata

//! a header extension: its code and data (without esize and ecode, incl. padding)
struct Extension {
  qint32 code;
  QByteArray data;
};

//! header extensions between the main header (headerSize bytes) and end
//! (vox_offset of single files, end of the header file of pairs); present is
//! set if the extension indicator announces extensions, used to the number of
//! bytes of the extensions read (without the indicator)
QList<Extension> read(QIODevice& file, qint64 headerSize, qint64 end, bool swap, bool* present = NULL, qint64* used = NULL);

//! DICOM data set of the metadata (JSON of a NIfTI file, see
//! MedIOHeaderMapping::niftiJson(), or attributes), empty if it holds nothing
//! that DICOM can store
QByteArray dicomDataSet(const QJsonObject& json);

//! metadata of a DICOM data set (explicit or implicit VR, little or big
//! endian): the keys of CMedIOHeaderDictionary and FrameTimesStart/FrameDuration
//! (s) of the private tags of PMOD; pmod is set if the data set has the
//! private creator "PMOD_1"
QJsonObject metadataOfDicomDataSet(const QByteArray& data, bool* pmod = NULL);

//! the header extensions holding the metadata (without the extension
//! indicator): the JSON extension and the DICOM extension (if the metadata
//! has values for it); size > 0 enlarges the last extension to the given
//! size of all extensions (a multiple of 16 bytes); empty if json is empty
QByteArray metadataExtensions(const QJsonObject& json, bool swap, qint64 size = 0);

//! size of the header extensions of json with the timing of n volumes,
//! whatever the start times and durations are
qint64 reservedSize(const QJsonObject& json, int volumes);

//! descrip of a NIfTI file written for the attributes, as PMOD writes it:
//! "<PatientName> [<ECAT7 Study_Type>] <StudyDescription or SeriesDescription>",
//! empty if the attributes have none of them
QString niftiDescription(const QJsonObject& attributes);

//! attributes of a descrip written by PMOD (see niftiDescription()), empty if
//! it does not have this form
QJsonObject attributesOfNiftiDescription(const QString& descrip);

//! metadata of an opened NIfTI file: the JSON header extension or JSON
//! sidecar file, completed by the DICOM extension and the descrip of PMOD
QJsonObject fileMetadata(CNIFTIFile& file, const CNIFTIMainHeader* header, const QString& fileName);

} // namespace NIFTIExtensions

#endif // CNIFTIEXTENSIONS_H
