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

#include <CPhilipsFile.h>
#include <CPhilipsMainHeader.h>

#include <rtdebug.h>

#include <iostream>

using namespace std;

int main(int argc, char** argv)
{
  int returnCode = 2; // return an error on default

  // You want this, unless you mix couts output with C output.
  // Read  http://gcc.gnu.org/onlinedocs/libstdc++/27_io/howto.html#8 for an explanation.
  ios::sync_with_stdio(false);

  // if no filename given
  if(argc != 2)
  {
    cout << "usage: " << argv[0] << " <filename>" << endl;
    return returnCode;
  }

  CPhilipsFile file(argv[1]);
  if(file.open(QIODevice::ReadOnly))
  {
    cout << "file.rtti()    : " << file.rtti() << endl;
    cout << "file.maxFrame(): " << file.maxFrame() << endl;
    cout << "file.minFrame(): " << file.minFrame() << endl;
    cout << "file.maxSlice(): " << file.maxSlice() << endl;
    cout << "file.minSlice(): " << file.minSlice() << endl;
    cout << "file.numTilts(): " << file.numTilts() << endl;

    CPhilipsMainHeader* mainHeader = NULL;
    if(file.readMainHeader(mainHeader) == true)
    {
      cout << "mainHeader.rawDataSize()     : " << mainHeader->rawDataSize() << endl;
      cout << "mainHeader.isPETMR()         : " << mainHeader->isPETMR() << endl;
      cout << "mainHeader.file_Format()     : " << mainHeader->file_Format() << endl;
      cout << "mainHeader.scanner_Geometry(): " << mainHeader->scanner_Geometry() << endl;
      cout << "mainHeader.hardware_Config() : " << mainHeader->hardware_Config() << endl;
      cout << "mainHeader.edit_Flag()       : " << mainHeader->edit_Flag() << endl;
      cout << "mainHeader.file_Type()       : " << mainHeader->file_Type() << endl;
      cout << "mainHeader.day_Created()     : " << mainHeader->day_Created() << endl;
      cout << "mainHeader.month_Created()     : " << mainHeader->month_Created() << endl;
      cout << "mainHeader.year_Created()     : " << mainHeader->year_Created() << endl;
      cout << "mainHeader.hour_Created()     : " << mainHeader->hour_Created() << endl;
      cout << "mainHeader.minute_Created()     : " << mainHeader->minute_Created() << endl;
      cout << "mainHeader.second_Created()     : " << mainHeader->second_Created() << endl;
      cout << "mainHeader.num_Frames()      : " << mainHeader->num_Frames() << endl;
      cout << "mainHeader.num_Slices()      : " << mainHeader->num_Slices() << endl;
      cout << "mainHeader.num_Tilts()       : " << mainHeader->num_Tilts() << endl;
      delete mainHeader;
    }
    else
    {
      cout << "failed to read the main header." << endl;
    }
  }
  else
  {
    cout << "failed to open the file." << endl;
  }
  
  file.close();

  return returnCode;
};
