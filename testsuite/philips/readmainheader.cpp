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

#include <iostream>

#include <rtdebug.h>

using namespace std;

int main(int argc, char** argv)
{
  int returnCode = 2; // return an error on default

  // if no filename given
  if(argc != 2)
  {
    cout << "usage: " << argv[0] << " <filename>" << endl;
    return returnCode;
  }
  // You want this, unless you mix couts output with C output.
  // Read  http://gcc.gnu.org/onlinedocs/libstdc++/27_io/howto.html#8 for an explanation.
  ios::sync_with_stdio(false);

  CPhilipsFile file(argv[1]);
  if(file.open(QIODevice::ReadOnly))
  {
    CPhilipsMainHeader* mainHeader = NULL;
    if(file.readMainHeader(mainHeader) == true)
    {
      SHOWVALUE(mainHeader->file_Format());
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
