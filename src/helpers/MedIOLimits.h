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

//! @file MedIOLimits.h
//! @brief private helpers (not installed): size limits of the buffers holding matrix data

#ifndef MEDIOLIMITS_H
#define MEDIOLIMITS_H

#include <QtGlobal>

#include <limits>

namespace MedIOLimits {

//! largest QByteArray (bytes): its size is an int, Qt5 needs some bytes of
//! it for its own header. A larger size converted to int becomes negative,
//! and QByteArray(data, size) then treats data as a nul-terminated string.
const qint64 MaxByteArraySize = std::numeric_limits<int>::max() - 64;

//! largest matrix read or written through a length of type unsigned int
const qint64 MaxUnsignedSize = std::numeric_limits<unsigned int>::max();

//! true if size bytes fit into a QByteArray
inline bool fitsByteArray(qint64 size)
{
  return size >= 0 && size <= MaxByteArraySize;
}

} // namespace MedIOLimits

#endif // MEDIOLIMITS_H
