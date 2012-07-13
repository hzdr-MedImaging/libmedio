/* -*- mode: c++; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 * vim:set ts=2 sw=2 expandtab: *********************************************
 *
 * libmedio - C++ I/O Library for loading/saving medical data formats
 * Copyright (C) 2006-2010 by Jens Langner <Jens.Langner@light-speed.de>
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

#include <byteswap.h>
#include <rtdebug.h>

#define bswap_flt(x) ({ union { quint32 i; float f;} u; u.f = x; u.i = bswap_32(u.i); u.f; })
#define rol32(word, shift) ((word) << (shift)) | ((word) >> (32 - (shift)))
#define rol64(word, shift) ((word) << (shift)) | ((word) >> (64 - (shift)))
#define BSWAP_16(x)   ((x) = bswap_16(x))
#define BSWAP_32(x)   ((x) = bswap_32(x))
#define BSWAP_FLT(x)  ((x) = bswap_flt(x))

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
    case sizeof(quint16):
    {
      if((matrixSize % sizeof(quint32)) != 0)
      {
        *swappedMatrixData = bswap_16(*matrixData);
        matrixData++;
        swappedMatrixData++;
        matrixSize -= sizeof(quint16);
      }

      const quint32* matrixData32 = reinterpret_cast<const quint32*>(matrixData);
      quint32* swappedMatrixData32 = reinterpret_cast<quint32*>(swappedMatrixData);
      for(quint32 i=0; i < (matrixSize/sizeof(quint32)); i++)
      {
        quint32 tmp = bswap_32(matrixData32[i]);
        swappedMatrixData32[i] = rol32(tmp, 16);
      }
    }
    break;

    case sizeof(quint32):
    {
      if((matrixSize % sizeof(quint64)) != 0)
      {
        *swappedMatrixData = bswap_32(*matrixData);
        matrixData++;
        swappedMatrixData++;
        matrixSize -= sizeof(quint32);
      }

      const quint64* matrixData64 = reinterpret_cast<const quint64*>(matrixData);
      quint64* swappedMatrixData64 = reinterpret_cast<quint64*>(swappedMatrixData);
      for(quint32 i=0; i < (matrixSize/sizeof(quint64)); i++)
      {
        quint64 tmp = bswap_64(matrixData64[i]);
        swappedMatrixData64[i] = rol64(tmp, 32);
      }
    }
    break;

    default:
      E("unsupported type passed: %d", sizeof(T));
    break;
  }

  RETURN(swappedMatrixData);
  return swappedMatrixData;
}
