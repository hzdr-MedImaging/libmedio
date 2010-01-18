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
 * $Id: ecatcmp.cpp 496 2010-01-11 09:45:23Z langner $
 *
 **************************************************************************/

#include <CECATFile>
#include <CECAT6MainHeader>
#include <CECAT7MainHeader>
#include <CECATDirectory>
#include <CECAT7SubHeaderScan3D>
#include <CECAT7SubHeaderNorm3D>
#include <CECAT7SubHeaderAttenCorr>
#include <CECAT7SubHeaderImage>

#include <QHash>
#include <QStringList>

#include <math.h>
#include <float.h>

#include <iostream>
#include <iomanip>

#include "config.h" // for big/little endianness check

using namespace std;

// local prototypes
static bool checkBedPositionOffsets(CECATMainHeader* header, bool fix);
static bool checkBedPositionNumber(CECATMainHeader* header, CECATFile* file, bool fix);

//  Function:    main
//! 
//! This is a small and simple example on how to read out some values from the
//! main header of a specific file, alter it and save it back to it.
//!
//! The concrete example searches for the BED_POSITION entries, analyses them
//! if they are incorrectly set to 'mm' and then replace them by the proper
//! 'cm' values.
//! 
//! @param       argc number of commandline options
//! @param       *argv[] pointer to a array of the commandline options
//! @return      integer of the exit code.
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
  int returnCode = 2; // return an error on default

  // You want this, unless you mix couts output with C output.
  // Read  http://gcc.gnu.org/onlinedocs/libstdc++/27_io/howto.html#8 for an explanation.
  ios::sync_with_stdio(false);

  cout << "ecatCheck 1.0 - checks & fixes an ECAT file for consistency" << endl
       << "(" __DATE__ ")  Copyright (C) 2006-2010 by Jens Langner / www.fzd.de" << endl << endl;

  // put all arguments in a temporary MultiHash
  QHash<QString, QString> args;

   // and all potential input filenames into a QStringList
  QStringList inputFileNames; 

  // if the user has specified some commandline options
  // lets process and parse them.
  for(int i=1; i < argc; i++)
  {
    QString option(argv[i]);

    if(option[0] == '-')
    {
      if(i+1 < argc && argv[i+1][0] != '-')
      {
        args.insert(option, argv[i+1]);
      }
      else
        args.insert(option, "");
    }
    else 
      inputFileNames << argv[i];
  }

  // check if the user has specified a filename or not
  if(inputFileNames.count() == 1 &&
     args.contains("-h") == false)
  {
    QString fileName = inputFileNames[0];

    bool fixMode = args.contains("-f");

    // open the first file
    CECATFile file(fileName);
    if(file.open(fixMode ? QIODevice::ReadWrite : QIODevice::ReadOnly) && 
       file.format() != CECATFile::Undefined)
    {
      cout << "checking file: '" << qPrintable(fileName) << "'" << endl;

      // now we are going to check and fix the main header first
      CECATMainHeader* header;
      if(file.readMainHeader(header) == true)
      {
        bool processed = false;

        // check if the bed position number in the directory matches
        // the one in the main header
        processed |= checkBedPositionNumber(header, &file, fixMode);

        // call the different functions which are trying to check/fix certain things
        // in the main header
        processed |= checkBedPositionOffsets(header, fixMode);

        // if we are in "fixMode" we go and save back the main header
        if(fixMode && processed)
          header->save();

        delete header;
      }
      else
        cout << "   ERROR! while reading mainheader data of file." << endl;


      // close the ECAT file
      file.close();
    }
    else
      cout << "Error on loading file '" << qPrintable(fileName) << "' as an ECAT6/7 file.\n";
  }
  else
  {
    cout << "ERROR: incorrect number of arguments" << endl;

    cout << "Usage: " << argv[0] << " <options> file1" << endl
         << "Options:" << endl
         << "  -f : fix found problems as far as possible" << endl
         << "  -h : this help page." << endl;
  }

  return returnCode;
}

static bool checkBedPositionOffsets(CECATMainHeader* header, bool fix)
{
  bool processed = false;

  CECAT7MainHeader* eh = static_cast<CECAT7MainHeader*>(header);

  // to identify if the main header needs fixing we search for the first
  // bed position to be 116.4 or 135.8
  if(qRound(eh->bed_Offset(0)) == 116 || qRound(eh->bed_Offset(0)) == 136)
  {
    cout << " found broken bed offset values (in mm):" << endl;

    for(int i=0; i < 15; i++)
    {
      if(fix)
      {
        if(eh->bed_Offset(i) > 100)
          eh->setBed_Offset(i, eh->bed_Offset(i)/10.0);
      }

      cout << "  BED_OFFSET[" << setw(2) << i << "]..........: '" << eh->bed_Offset(i) << "'" << endl;
    }

    if(fix)
      cout << "  ... fixed! ok!" << endl;

    processed = true;
  }

  return processed;
}

static bool checkBedPositionNumber(CECATMainHeader* header, CECATFile* file, bool fix)
{
  bool processed = false;

  CECAT7MainHeader* eh = static_cast<CECAT7MainHeader*>(header);

  if(file->numBedPos() != eh->num_Bed_Pos())
  {
    cout << " inconsistent number of bedpos matrices in ECAT file and main header" << endl;
  }

  return processed;
}
