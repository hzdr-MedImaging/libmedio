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
 * $Id$
 *
 **************************************************************************/

#include "CPhilipsDirectory.h"
//#include "CPhilipsDirectoryItem.h"
#include "CPhilipsFile.h"

#include <rtdebug.h>

// some very own defines specifying the boundaries
// of a philips directory list
#define PHILIPS_DIRHEAD_SIZE 16
#define PHILIPS_DIRITEM_SIZE 16
#define PHILIPS_DIRITEM_NUM 31
#define PHILIPS_DIRLIST_SIZE (PHILIPS_DIRHEAD_SIZE+PHILIPS_DIRITEM_NUM*PHILIPS_DIRITEM_SIZE)

// the RAW structures used in the philips directory blocks
struct Philips_DirHead          // should be 16 bytes
{
  quint32 availableEntries;
  quint32 nextDirectory;
  quint32 prevDirectory;
  quint32 usedEntries;
};

struct Philips_DirItem          // should be 16 bytes
{
  quint32 matrixID;             // bits 0-15 Frame, 16-27 slice, 28-31 tilt
  quint32 dataBlock_Start;      // start position of the subHeader
  quint32 dataBlock_End;        // end position of the last data block
  quint16 compressionFlag;      // 0x0001: uncompressed, 0x0101: compressed
  quint16 contentFlag;          // 0xFFFE: header, 0xFFFF: removed, 0x0000: unused, 0x0001: used
};

struct Philips_DirList // should be 512 bytes
{
  struct Philips_DirHead head;
  struct Philips_DirItem items[PHILIPS_DIRITEM_NUM];
};
