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

#ifndef _BSWAP_H_
#define _BSWAP_H_

#include <cstring>

#include <rtdebug.h>

// bswap_XX() byteswap macros. These macros are usually coming from linux. But
// as we need to get this compiled on mac and windows as well we use different
// definitions here
#if defined(Q_OS_MAC) 
#include <libkern/OSByteOrder.h> 
#define bswap_8(x)  ((x) & 0xff)
#define bswap_16(x) OSSwapInt16(x) 
#define bswap_32(x) OSSwapInt32(x) 
#define bswap_64(x) OSSwapInt64(x) 
#elif defined(Q_OS_WIN)
#define bswap_8(x)  ((x) & 0xff)
#define bswap_16(x) ((bswap_8(x) << 8) | bswap_8((x) >> 8))
#define bswap_32(x) ((bswap_16(x) << 16) | bswap_16((x) >> 16))
#define bswap_64(x) ((bswap_32(x) << 32) | bswap_32((x) >> 32))
#else
#include <byteswap.h>
#define bswap_8(x)  ((x) & 0xff)
#endif

// macro for byteswapping float (32bit) values using
// a GNU extension
#define bswap_flt(x) ({ union { quint32 i; float f;} u; u.f = x; u.i = bswap_32(u.i); u.f; })
#define bswap_dbl(x) ({ union { quint64 i; double d;} u; u.d = x; u.i = bswap_64(u.i); u.d; })

// common BSWAP_X() macros replacing the same
// variable which is going to be swapped
#define BSWAP_8(x)   ((x) = bswap_8(x))
#define BSWAP_16(x)  ((x) = bswap_16(x))
#define BSWAP_32(x)  ((x) = bswap_32(x))
#define BSWAP_64(x)  ((x) = bswap_64(x))
#define BSWAP_FLT(x) ((x) = bswap_flt(x))
#define BSWAP_DBL(x) ((x) = bswap_dbl(x))

// common rotate left macros
#define rol16(word, shift)  ((word) << (shift)) | ((word) >> (16 - (shift)))
#define rol32(word, shift)  ((word) << (shift)) | ((word) >> (32 - (shift)))
#define rol64(word, shift)  ((word) << (shift)) | ((word) >> (64 - (shift)))

// swap the byte order of every value of type T of an array of matrixSize
// bytes into dstMatrix (a new array if NULL, to be deleted with delete[] as
// char array; it may be matrixData itself) and return it. The values are
// accessed with memcpy(), so the arrays need no alignment beyond that of
// char (accessing them as wider types would be undefined behaviour); the
// compilers turn this into plain loads and byte swaps.
template <typename T>
static inline T* bswap_matrix(const T* matrixData, quint32 matrixSize, T* dstMatrix = NULL)
{
  // make sure we have a multiple of sizeof(T)
  ASSERT((matrixSize % sizeof(T)) == 0);

  T* swappedMatrixData = (dstMatrix == NULL) ? reinterpret_cast<T*>(new char[matrixSize]) : dstMatrix;

  const char* src = reinterpret_cast<const char*>(matrixData);
  char* dst = reinterpret_cast<char*>(swappedMatrixData);
  const quint32 count = matrixSize / sizeof(T);

  switch(sizeof(T))
  {
    case sizeof(quint8):
      // nothing to swap
      if(dst != src)
        memcpy(dst, src, count);
    break;

    case sizeof(quint16):
      for(quint32 i=0; i < count; i++)
      {
        quint16 value;
        memcpy(&value, src + i * sizeof(value), sizeof(value));
        value = bswap_16(value);
        memcpy(dst + i * sizeof(value), &value, sizeof(value));
      }
    break;

    case sizeof(quint32):
      for(quint32 i=0; i < count; i++)
      {
        quint32 value;
        memcpy(&value, src + i * sizeof(value), sizeof(value));
        value = bswap_32(value);
        memcpy(dst + i * sizeof(value), &value, sizeof(value));
      }
    break;

    case sizeof(quint64):
      for(quint32 i=0; i < count; i++)
      {
        quint64 value;
        memcpy(&value, src + i * sizeof(value), sizeof(value));
        value = bswap_64(value);
        memcpy(dst + i * sizeof(value), &value, sizeof(value));
      }
    break;

    default:
      E("unsupported type passed: %d", static_cast<int>(sizeof(T)));
    break;
  }

  return swappedMatrixData;
}

#endif
