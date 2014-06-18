/* -*- mode: c++; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 * vim:set ts=2 sw=2 expandtab: *********************************************
 *
 * libmedio - C++ I/O Library for loading/saving medical data formats
 * Copyright (C) 2006-2012 by Jens Langner <Jens.Langner@light-speed.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: CMedIO.h 496 2010-01-11 09:45:23Z langner $
 *
 **************************************************************************/

#ifndef _BSWAP_H_
#define _BSWAP_H_

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

template <typename T>
static inline T* bswap_matrix(const T* matrixData, quint32 matrixSize, T* dstMatrix = NULL)
{
  // make sure we have a multiple of sizeof(T)
  ASSERT((matrixSize % sizeof(T)) == 0);

  // create a new array
  T* swappedMatrixData;

  if(dstMatrix == NULL)
    swappedMatrixData = reinterpret_cast<T*>(new char[matrixSize]);
  else
    swappedMatrixData = dstMatrix;

  // lets swap now
  switch(sizeof(T))
  {
    case sizeof(quint8):
    {
      quint32 i;
      const quint16* matrixData16 = reinterpret_cast<const quint16*>(matrixData);
      quint16* swappedMatrixData16 = reinterpret_cast<quint16*>(swappedMatrixData);

      for(i=0; i < (matrixSize/sizeof(quint16)); i++)
      {
        quint16 tmp = bswap_16(matrixData16[i]);
        swappedMatrixData16[i] = rol16(tmp, 8);
      }

      D("matrixSize %% sizeof(quint16) = %d %d", matrixSize % sizeof(quint16), i);
      switch(matrixSize % sizeof(quint16))
      {
        case 0:
          // do nothing, finished
        break;

        case sizeof(quint8): // 1
          swappedMatrixData16[i] = bswap_8(matrixData16[i]);
        break;

        default:
          // ERROR! only 0 and 1 should happen!
          ASSERT((matrixSize % sizeof(quint16) == 0));
        break;
      }
    }
    break;

    case sizeof(quint16):
    {
      quint32 i;
      const quint32* matrixData32 = reinterpret_cast<const quint32*>(matrixData);
      quint32* swappedMatrixData32 = reinterpret_cast<quint32*>(swappedMatrixData);

      for(i=0; i < (matrixSize/sizeof(quint32)); i++)
      {
        quint32 tmp = bswap_32(matrixData32[i]);
        swappedMatrixData32[i] = rol32(tmp, 16);
      }

      D("matrixSize %% sizeof(quint32) = %d %d", matrixSize % sizeof(quint32), i);
      switch(matrixSize % sizeof(quint32))
      {
        case 0:
          // do nothing, finished
        break;

        case sizeof(quint16): // 2
          swappedMatrixData32[i] = bswap_16(matrixData32[i]);
        break;

        default:
          // ERROR! only 0 and 2 should happen!
          ASSERT((matrixSize % sizeof(quint32) == 0));
        break;
      }
    }
    break;

    case sizeof(quint32):
    {
      quint32 i;
      const quint64* matrixData64 = reinterpret_cast<const quint64*>(matrixData);
      quint64* swappedMatrixData64 = reinterpret_cast<quint64*>(swappedMatrixData);

      for(i=0; i < (matrixSize/sizeof(quint64)); i++)
      {
        quint64 tmp = bswap_64(matrixData64[i]);
        swappedMatrixData64[i] = rol64(tmp, 32);
      }

      D("matrixSize %% sizeof(quint64) = %d %d", matrixSize % sizeof(quint32), i);
      switch(matrixSize % sizeof(quint64))
      {
        case 0:
          // do nothing, finished
        break;

        case sizeof(quint32): // 4
          swappedMatrixData64[i] = bswap_32(matrixData64[i]);
        break;

        default:
          // ERROR! only 0 and 4 should happen!
          ASSERT((matrixSize % sizeof(quint64) == 0));
        break;
      }
 
    }
    break;

    case sizeof(quint64):
    {
      const quint64* matrixData64 = reinterpret_cast<const quint64*>(matrixData);
      quint64* swappedMatrixData64 = reinterpret_cast<quint64*>(swappedMatrixData);

      for(quint32 i=0; i < (matrixSize/sizeof(quint64)); i++)
        swappedMatrixData64[i] = bswap_64(matrixData64[i]);
    }
    break;

    default:
      E("unsupported type passed: %d", sizeof(T));
    break;
  }

  RETURN(swappedMatrixData);
  return swappedMatrixData;
}

#endif
