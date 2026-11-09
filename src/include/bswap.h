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

// copy of n bytes for the loads and stores of bswap_matrix(). GCC and Clang get
// the builtin: with _FORTIFY_SOURCE (the default of e.g. Ubuntu), memcpy() into
// an array whose size the compiler knows (a new[] array) becomes a call of
// __memcpy_chk() per value instead of a plain store, which is several times
// slower; the bounds are checked by bswap_matrix() itself.
#if defined(__GNUC__)
#define bswap_copy(dst, src, n) __builtin_memcpy((dst), (src), (n))
#else
#define bswap_copy(dst, src, n) memcpy((dst), (src), (n))
#endif

// swap the byte order of every value of type T of an array of matrixSize
// bytes into dstMatrix (a new array if NULL, to be deleted with delete[] as
// char array; it may be matrixData itself) and return it.
//
// The array is processed in 64 bit words that are copied into a local
// variable, so it needs no alignment beyond that of char (accessing it as
// a wider type would be undefined behaviour). A word holds four 16 bit
// values (bytes swapped with masks and shifts), two 32 bit values (bswap_64
// and a rotation by 32 bits, which swaps the two halves back) or one 64 bit
// value; remaining values are swapped one by one. This works in either byte
// order of the machine, and GCC and Clang compile it into plain loads,
// byte swaps or vector instructions. The testsuite benchmark
// testBswapPerformance measures it.
template <typename T>
static inline T* bswap_matrix(const T* matrixData, quint32 matrixSize, T* dstMatrix = NULL)
{
  // make sure we have a multiple of sizeof(T)
  ASSERT((matrixSize % sizeof(T)) == 0);

  T* swappedMatrixData = (dstMatrix == NULL) ? reinterpret_cast<T*>(new char[matrixSize]) : dstMatrix;

  const char* src = reinterpret_cast<const char*>(matrixData);
  char* dst = reinterpret_cast<char*>(swappedMatrixData);
  const quint32 count = matrixSize / sizeof(T);
  const quint32 words = matrixSize / sizeof(quint64);
  quint32 done = 0; // values swapped in words

  switch(sizeof(T))
  {
    case sizeof(quint8):
      // nothing to swap
      if(dst != src)
        memcpy(dst, src, count);
      done = count;
    break;

    case sizeof(quint16):
      for(quint32 i=0; i < words; i++)
      {
        quint64 word;
        bswap_copy(&word, src + i * sizeof(word), sizeof(word));
        word = ((word & Q_UINT64_C(0x00ff00ff00ff00ff)) << 8) | ((word >> 8) & Q_UINT64_C(0x00ff00ff00ff00ff));
        bswap_copy(dst + i * sizeof(word), &word, sizeof(word));
      }
      done = words * 4;
    break;

    case sizeof(quint32):
      for(quint32 i=0; i < words; i++)
      {
        quint64 word;
        bswap_copy(&word, src + i * sizeof(word), sizeof(word));
        word = bswap_64(word);
        word = (word << 32) | (word >> 32);
        bswap_copy(dst + i * sizeof(word), &word, sizeof(word));
      }
      done = words * 2;
    break;

    case sizeof(quint64):
    break;

    default:
      E("unsupported type passed: %d", static_cast<int>(sizeof(T)));
      done = count;
    break;
  }

  // the remaining values one by one
  for(quint32 i=done; i < count; i++)
  {
    switch(sizeof(T))
    {
      case sizeof(quint16):
      {
        quint16 value;
        bswap_copy(&value, src + i * sizeof(value), sizeof(value));
        value = bswap_16(value);
        bswap_copy(dst + i * sizeof(value), &value, sizeof(value));
      }
      break;

      case sizeof(quint32):
      {
        quint32 value;
        bswap_copy(&value, src + i * sizeof(value), sizeof(value));
        value = bswap_32(value);
        bswap_copy(dst + i * sizeof(value), &value, sizeof(value));
      }
      break;

      default:
      {
        quint64 value;
        bswap_copy(&value, src + i * sizeof(value), sizeof(value));
        value = bswap_64(value);
        bswap_copy(dst + i * sizeof(value), &value, sizeof(value));
      }
      break;
    }
  }

  return swappedMatrixData;
}

#endif
