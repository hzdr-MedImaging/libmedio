/* -*- mode: c++; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 * vim:set ts=2 sw=2 expandtab: *********************************************
 *
 * libmedio - C++ I/O Library for loading/saving medical data formats
 * Copyright (C) 2006-2019 by Jens Maus <mail@jens-maus.de>
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

#include <CECATFile.h>
#include <CECAT6MainHeader.h>
#include <CECAT7MainHeader.h>
#include <CECATDirectory.h>
#include <CECAT7SubHeaderScan3D.h>
#include <CECAT7SubHeaderNorm3D.h>
#include <CECAT7SubHeaderAttenCorr.h>
#include <CECAT7SubHeaderImage.h>

#include <QHash>
#include <QStringList>

#include <math.h>
#include <float.h>

#include <iostream>
#include <iomanip>

#include <rtdebug.h>

#include "config.h" // for big/little endianness check

using namespace std;

// local prototypes
static bool checkBedPositionOffsets(CECATMainHeader* header, bool fix);
static bool checkBedPositionNumber(CECATMainHeader* header, CECATFile* file, bool fix);
static bool checkFramesNumber(CECATMainHeader* header, CECATFile* file, bool fix);
static bool checkDirectoryList(CECATFile* file, bool fix);

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

  // before we start anything serious, we need to initialize our
  // debug class
  #if defined(DEBUG)
  CRTDebug::instance()->init("ecatCheck", true);
  #else
  CRTDebug::instance()->init("ecatCheck", false);
  #endif

  ENTER();

  Info("ecatCheck %s - checks & fixes an ECAT file for consistency", PROJECT_VERSION);
  Info("(%s) Copyright (C) 2006-2019 by Jens Maus / www.hzdr.de\n", __DATE__);

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
      Info("checking file: '%s'", qPrintable(fileName));

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
        processed |= checkFramesNumber(header, &file, fixMode);

        // if we are in "fixMode" we go and save back the main header
        if(fixMode && processed)
          header->save();

        delete header;

        // check if all matrix data can be read
        processed |= checkDirectoryList(&file, fixMode);
      }
      else
        Error("while reading mainheader data of file.");


      // close the ECAT file
      file.close();
    }
    else
      Error("on loading file '%s' as an ECAT6/7 file.", qPrintable(fileName));
  }
  else
  {
    Error("incorrect number of arguments");

    Info("Usage: %s <options> file1", argv[0]);
    Info("Options:");
    Info(" -f : fix found problems as far as possible");
    Info("  -h : this help page.");
  }

  RETURN(returnCode);
  CRTDebug::destroy();
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
    Info(" found broken bed offset values (in mm):");

    for(int i=0; i < 15; i++)
    {
      if(fix)
      {
        if(eh->bed_Offset(i) > 100)
          eh->setBed_Offset(i, eh->bed_Offset(i)/10.0);
      }

      Info("  BED_OFFSET[%02d]..........: %.2f", i, eh->bed_Offset(i));
    }

    if(fix)
      Info("  ... fixed! ok!");

    processed = true;
  }

  return processed;
}

static bool checkBedPositionNumber(CECATMainHeader* header, CECATFile* file, bool)
{
  bool processed = false;

  CECAT7MainHeader* eh = static_cast<CECAT7MainHeader*>(header);

  Info("Number of bed positions: %d", eh->num_Bed_Pos());

  if(file->numBedPos() != eh->num_Bed_Pos())
    Error(" inconsistent number of bedpos matrices in ECAT file (%d) and main header (%d)", file->numBedPos(), eh->num_Bed_Pos());

  return processed;
}

static bool checkFramesNumber(CECATMainHeader* header, CECATFile* file, bool fix)
{
  bool processed = false;

  CECAT7MainHeader* eh = static_cast<CECAT7MainHeader*>(header);

  Info("Number of frames: %d", eh->num_Frames());

  if(static_cast<short>(file->directory()->count()) != eh->num_Frames())
  {
    Info(" inconsistent number of frames in ECAT file (%d) and main header (%d)", file->directory()->count(), eh->num_Frames());

    if(fix)
    {
      CECATSubHeader* subHeader = NULL;

      // find out which frame is missing
      for(int i=1; i < eh->num_Frames(); i++)
      {
        if(file->readSubHeader(subHeader, i) == false)
        {
          int reuseFrame = 0;

          Info("  frame %d missing", i);

          // read the next subheader
          if(file->readSubHeader(subHeader, i+1) == true)
          {
            switch(subHeader->subHeaderType())
            {
              case CECATSubHeader::ECAT7_Image:
              {
                CECAT7SubHeaderImage* sh = static_cast<CECAT7SubHeaderImage*>(subHeader);
                sh->setFrame_Start_Time(sh->frame_Start_Time()-sh->frame_Duration());
                sh->setScale_Factor(1.0);
                sh->setImage_Min(0);
              }
              break;

              default:
                subHeader = NULL;
              break;
            }

            reuseFrame = i+1;
          }
          else if(file->readSubHeader(subHeader, i-1) == true)
          {
            switch(subHeader->subHeaderType())
            {
              case CECATSubHeader::ECAT7_Image:
              {
                CECAT7SubHeaderImage* sh = static_cast<CECAT7SubHeaderImage*>(subHeader);
                sh->setFrame_Start_Time(sh->frame_Start_Time()+sh->frame_Duration());
                sh->setScale_Factor(1.0);
                sh->setImage_Min(0);
              }
              break;

              default:
                subHeader = NULL;
              break;
            }

            reuseFrame = i-1;
          }
          else
          {
            Error("there is no other frame subheader we can reuse to create the new one");
            subHeader = NULL;
          }

          if(subHeader != NULL)
          {
            char* data = NULL;
            unsigned int len = 0;

            // load the matrix data of the reused Frame so that we can create a new one
            if(file->readMatrix(data, len, reuseFrame) == true)
            {
              // clear the dataset to reuse it immediately
              memset(data, 0, len);

              // finally write the new empty frame to the file
              if(file->writeMatrix(data, len, *subHeader, i) == true)
              {
                Info("   copying empty clone from frame %d as new frame %d", reuseFrame, i);
              }
              else
                Error("while trying to write new frame %d", i);
            }
            else
              Error("couldn't load data of frame %d", reuseFrame);
          }
        }
      }
    }
  }

  return processed;
}

static bool checkDirectoryList(CECATFile* file, bool fix)
{
  bool processed = false;

  CECATDirectory* dir = file->directory();
  if(dir->count() > 0)
  {
    for(unsigned int i=0; i < dir->count(); i++)
    {
      CECATDirectoryItem* dirItem = (*dir)[i];
      if(dirItem->dataBlock_Status() != CECATDirectoryItem::Finished)
      {
        Warning("directory item %d has not status 'Finished': %d", i, dirItem->dataBlock_Status());
        if(fix == true)
        {
          dirItem->setDataBlock_Status(CECATDirectoryItem::Finished);
          processed = true;
        }
      }
    }

    if(processed)
      dir->save();
  }
  else
    Warning("ecat directory is empty");

  return processed;
}
