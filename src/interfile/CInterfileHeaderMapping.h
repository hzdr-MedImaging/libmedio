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

//! @file CInterfileHeaderMapping.h
//! @brief private: the format independent header attributes of Interfile headers
//!        (see CMedIOHeaderMapping.h and docs/interfile.md)

#ifndef CINTERFILEHEADERMAPPING_H
#define CINTERFILEHEADERMAPPING_H

#include <QJsonObject>
#include <QList>
#include <QPair>
#include <QString>

class CInterfileKeys;

namespace InterfileMapping {

//! @brief units of the physical values as attribute Units: Bq/ml, Bq/cc, BQML, ... are
//!        "BQML" (DICOM), kBq/ml, kBq/cc, KBQCC "KBQCC" (as libmedio gives the values of
//!        PMOD's NIfTI files); other units are returned as given. The values are not
//!        converted.
QString units(const QString& units);

//! @brief true for the keys describing the layout, scaling, geometry or timing of
//!        the image data (normalized names, see CInterfileKeys::normalizedName()):
//!        they are not part of the "Interfile" section of the attributes
bool isLayoutKey(const QString& name);

//! @brief the keys [begin, end) that are no layout keys as JSON object: the keys as
//!        written, repeated keys as array of their values (empty values are left out)
QJsonObject section(const CInterfileKeys& keys, int begin, int end);

//! @brief attributes of a main header
//! @param keys the keys of the main header and the range of its general keys
//! @param dataSet the image header of the first data set of a Siemens main header
//!        (keys not given by the main header are taken from it), or NULL
//! @param units the units of the first frame (see units())
QJsonObject mainAttributes(const CInterfileKeys& keys, int begin, int end, const CInterfileKeys* dataSet,
                           const QString& units);

//! @brief attributes of a frame: ActualFrameDuration (ms) and the keys [begin, end) as
//!        section "Interfile"
QJsonObject frameAttributes(const CInterfileKeys& keys, int begin, int end, double duration);

//! @brief the keys of a header written by libmedio for the attributes of a main header
//!        (key as written, value), read again by mainAttributes(): patient, study,
//!        equipment, radiopharmaceutical, energy window, units and reconstruction;
//!        times with a zone are written in UTC ("GMT+00:00" in the name of the key)
QList<QPair<QString, QString> > keysOfAttributes(const QJsonObject& attributes);

} // namespace InterfileMapping

#endif // CINTERFILEHEADERMAPPING_H
