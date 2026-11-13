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

#include "CNIFTIMainHeader.h"
#include "CNIFTIExtensions.h"

#include <QJsonDocument>
#include <QJsonParseError>

#include <rtdebug.h>

#include "bswap.h"

// extension code used for the JSON metadata extension (NIFTI_ECODE_COMMENT)
#define NIFTI_ECODE_JSON 6

//=============================================================================================
// Constructors 
CNIFTIMainHeader::CNIFTIMainHeader(CMedIOData* niftiFile) : CMedIOHeader(niftiFile) {
  ENTER();

  LEAVE();
}

// Copy constructor
CNIFTIMainHeader::CNIFTIMainHeader(const CNIFTIMainHeader& src) : CMedIOHeader(src) {
  ENTER();

  // do nothing

  LEAVE();
}

// Default assignment operator
CNIFTIMainHeader& CNIFTIMainHeader::operator=(const CNIFTIMainHeader& src) {
  ENTER();

  copyAttributes(src);

  convertFrom(&src);
  
  LEAVE();
  
  return *this;
}

// runtime type information
CMedIOHeader::Format CNIFTIMainHeader::headerFormat() const { 
  return CMedIOHeader::NIFTIMainHeader;
}

//=============================================================================================
// Header extensions
//
// A single file NIfTI may contain header extensions between the main header and
// vox_offset, the header file of a header/image pair after the main header up to
// the end of the header file. They are announced by a 4 byte extension indicator directly after the
// main header (first byte != 0). Each extension starts with its size in bytes (esize,
// a multiple of 16 which includes the 8 bytes for esize and ecode) and its code
// (ecode), followed by the extension data.

// size in bytes of the extensions holding the given metadata: a DICOM
// extension for other tools (e.g. PMOD) and the JSON extension (see CNIFTIExtensions.h)
int CNIFTIMainHeader::jsonExtensionSize(const QJsonObject& json) {
  return static_cast<int>(NIFTIExtensions::metadataExtensions(json, false).size());
}

// write the extension indicator and the extensions holding the metadata
// directly after the main header
bool CNIFTIMainHeader::writeJsonExtension(QIODevice& file, qint64 headerSize, const QJsonObject& json, bool swap) {
  ENTER();

  QByteArray extension;
  extension.append("\x01\0\0\0", 4); // extension indicator
  extension.append(NIFTIExtensions::metadataExtensions(json, swap));

  bool result = file.seek(headerSize) &&
                file.write(extension) == extension.size();

  RETURN(result);
  return result;
}

// return the JSON metadata of the first JSON extension found in the
// extensions between the main header and vox_offset
QJsonObject CNIFTIMainHeader::readJsonExtension(QIODevice& file, qint64 headerSize, qint64 voxOffset, bool swap) {
  ENTER();

  QJsonObject json;
  char extender[4] = { 0, 0, 0, 0 };

  // no extensions present
  if(file.seek(headerSize) == false ||
     file.read(extender, 4) != 4 ||
     extender[0] == 0) {
    RETURN(false);
    return json;
  }

  qint64 pos = headerSize + 4;
  while(pos + 8 <= voxOffset && file.seek(pos)) {
    qint32 esize = 0;
    qint32 ecode = 0;

    if(file.read(reinterpret_cast<char*>(&esize), 4) != 4 ||
       file.read(reinterpret_cast<char*>(&ecode), 4) != 4) {
      break;
    }

    if(swap) {
      BSWAP_32(esize);
      BSWAP_32(ecode);
    }

    if(esize < 8 || pos + esize > voxOffset) {
      W("invalid NIfTI header extension size %d at offset %lld", esize, pos);
      break;
    }

    if(ecode == NIFTI_ECODE_JSON) {
      QByteArray jsonData = file.read(esize - 8);

      // remove the extension padding
      while(jsonData.endsWith('\0'))
        jsonData.chop(1);

      QJsonParseError parseError;
      QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

      // other (e.g. plain text comment) extensions using the same code are skipped
      if(parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
        json = jsonDoc.object();
        break;
      }

      D("skipping non-JSON extension: %s", parseError.errorString().toLatin1().constData());
    }

    pos += esize;
  }

  RETURN(json.isEmpty() == false);
  return json;
}
