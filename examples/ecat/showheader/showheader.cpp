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

#include <CECATFile>
#include <CECAT6MainHeader>
#include <CECAT7MainHeader>

#include <iostream>

using namespace std;

//  Function:    main
//! 
//! This is a small and simple example on how to use the libmedio c++
//! to access ECAT6/7 files and just print out the header to the current
//! stdout stream. It should show how _simple_ it is to use libmedio and how
//! quickly you can access an ECAT file... isn't it nice? ;)
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

  cout << "libmedio ECAT6/7 file processing example" << endl;
  cout << "----------------------------------------" << endl;
  
  // check if the user has specified a filename or not
  if(argc > 1)
  {
    char* filename = argv[1];
    short frameNumber = -1;

    if(argc > 2)
      frameNumber = atoi(argv[2]);

    // open the file
    CECATFile file(filename);
    if(file.open(QIODevice::ReadOnly) && 
       file.format() != CECATFile::Undefined)
    {
      cout << "Successfully loaded file '" << filename << "'" << endl << endl;

      // now that we have successfully loaded the ECAT file we can go and
      // output some statistics on it:
      cout << "General file information:" << endl;
      cout << "------------------------" << endl;
      cout << "ECAT format version: " << (file.format() == CECATFile::ECAT7 ? "7" : "6") << endl;
      cout << "# of frames........: " << file.numFrames() << endl;
      cout << "# of gates.........: " << file.numGates() << endl;
      cout << "# of planes........: " << file.numPlanes() << endl;
      cout << "# of bedpos........: " << file.numBedPos() << endl;
  
      // now that we have put out gernal information, lets print out more detailed
      // main header information
      cout << endl;

      if(file.format() == CECATFile::ECAT7)
      {
        CECATMainHeader* mainHeader;
        file.readMainHeader(mainHeader);

        CECAT7MainHeader* mHeader = static_cast<CECAT7MainHeader*>(mainHeader);
        cout << "Main header data (ECAT7):" << endl;
        cout << "------------------------" << endl;
        cout << "MAGIC_NUMBER............: " << mHeader->magic_Number()              << endl;
        cout << "ORIGINAL_FILE_NAME......: " << mHeader->original_File_Name()        << endl;
        cout << "SW_VERSION..............: "  << mHeader->sw_Version()                << endl;
        cout << "SYSTEM_TYPE.............: "  << mHeader->system_Type()                << endl;
        cout << "FILE_TYPE...............: "  << mHeader->file_Type()                  << endl;
        cout << "SERIAL_NUMBER...........: "  << mHeader->serial_Number()              << endl;
        cout << "SCAN_START_TIME.........: "  << mHeader->scan_Start_Time()            << endl;
        cout << "ISOTOPE_NAME............: "  << mHeader->isotope_Name()              << endl;
        cout << "ISOTOPE_HALFLIFE........: "  << mHeader->isotope_Halflife()          << endl;
        cout << "RADIOPHARMACEUTICAL.....: "  << mHeader->radiopharmaceutical()        << endl;
        cout << "GANTRY_TILT.............: "  << mHeader->gantry_Tilt()                << endl;
        cout << "ROTATION_TILT...........: "  << mHeader->gantry_Rotation()            << endl;
        cout << "BED_ELEVATION...........: "  << mHeader->bed_Elevation()              << endl;
        cout << "INTRINSIC_TILT..........: "  << mHeader->intrinsic_Tilt()            << endl;
        cout << "WOBBLE_SPEED............: "  << mHeader->wobble_Speed()              << endl;
        cout << "TRANSM_SOURCE_TYPE......: "  << mHeader->transm_Source_Type()        << endl;
        cout << "DISTANCE_SCANNED........: "  << mHeader->distance_Scanned()          << endl;
        cout << "TRANSAXIAL_FOV..........: "  << mHeader->transaxial_FOV()            << endl;
        cout << "ANGULAR_COMPRESSION.....: "  << mHeader->angular_Compression()        << endl;
        cout << "COIN_SAMP_MODE..........: "  << mHeader->coin_Samp_Mode()            << endl;
        cout << "AXIAL_SAMP_MODE.........: "  << mHeader->axial_Samp_Mode()            << endl;
        cout << "ECAT_CALIBRATION_FACTOR.: "  << mHeader->calibration_Factor()        << endl;
        cout << "CALIBRATION_UNITS.......: "  << mHeader->calibration_Units()          << endl;
        cout << "CALIBRATION_UNITS_LABEL.: "  << mHeader->calibration_Units_Label()    << endl;
        cout << "COMPRESSION_CODE........: "  << mHeader->compression_Code()          << endl;
        cout << "STUDY_TYPE..............: "  << mHeader->study_Type()                << endl;
        cout << "PATIENT_ID..............: "  << mHeader->patient_ID()                << endl;
        cout << "PATIENT_NAME............: "  << mHeader->patient_Name()              << endl;
        cout << "PATIENT_SEX.............: "  << mHeader->patient_Sex()                << endl;
        cout << "PATIENT_DEXTERITY.......: "  << mHeader->patient_Dexterity()          << endl;
        cout << "PATIENT_AGE.............: "  << mHeader->patient_Age()                << endl;
        cout << "PATIENT_HEIGHT..........: "  << mHeader->patient_Height()            << endl;
        cout << "PATIENT_WEIGHT..........: "  << mHeader->patient_Weight()            << endl;
        cout << "PATIENT_BIRTH_DATE......: "  << mHeader->patient_Birth_Date()        << endl;
        cout << "PHYSICIAN_NAME..........: "  << mHeader->physician_Name()            << endl;
        cout << "OPERATOR_NAME...........: "  << mHeader->operator_Name()              << endl;
        cout << "STUDY_DESCRIPTION.......: "  << mHeader->study_Description()          << endl;
        cout << "ACQUISITION_TYPE........: "  << mHeader->acquisition_Type()          << endl;
        cout << "PATIENT_ORIENTATION.....: "  << mHeader->patient_Orientation()        << endl;
        cout << "FACILITY_NAME...........: "  << mHeader->facility_Name()              << endl;
        cout << "NUM_PLANES..............: "  << mHeader->num_Planes()                << endl;
        cout << "NUM_FRAMES..............: "  << mHeader->num_Frames()                << endl;
        cout << "NUM_GATES...............: "  << mHeader->num_Gates()                  << endl;
        cout << "NUM_BED_POS.............: "  << mHeader->num_Bed_Pos()                << endl;
        cout << "INIT_BED_POSITION.......: "  << mHeader->init_Bed_Position()          << endl;
        cout << "BED_POSITION............: ";
        for(int i=0; i < 15; i++)
        {
          cout << mHeader->bed_Offset(i) << " ";
        }
        cout << endl;
        cout << "PLANE_SEPARATION........: "  << mHeader->plane_Separation()          << endl;
        cout << "LWR_SCTR_THRES..........: "  << mHeader->lwr_Sctr_Thres()            << endl;
        cout << "LWR_TRUE_THRES..........: "  << mHeader->lwr_True_Thres()            << endl;
        cout << "UPR_TRUE_THRES..........: "  << mHeader->upr_True_Thres()            << endl;
        cout << "USER_PROCESS_CODE.......: "  << mHeader->user_Process_Code()          << endl;
        cout << "ACQUISITION_MODE........: "  << mHeader->acquisition_Mode()          << endl;
        cout << "BIN_SIZE................: "  << mHeader->bin_Size()                  << endl;
        cout << "BRANCHING_FRACTION......: "  << mHeader->branching_Fraction()        << endl;
        cout << "DOSE_START_TIME.........: "  << mHeader->dose_Start_Time()            << endl;
        cout << "DOSAGE..................: "  << mHeader->dosage()                    << endl;
        cout << "WELL_COUNTER_CORR_FACTOR: "  << mHeader->well_Counter_Corr_Factor()  << endl;
        cout << "DATA_UNITS..............: "  << mHeader->data_Units()                << endl;
        cout << "SEPTA_STATE.............: " << mHeader->septa_State()                << endl;
        cout << "FILL....................: ";
        for(int i=0; i < 6; i++)
        {
          cout << mHeader->cti_Reserved(i) << " ";
        }
        cout << endl;

        if(frameNumber > 0)
        {
          CECATSubHeader* subHeader = NULL;

          file.readSubHeader(subHeader, frameNumber);
          if(subHeader)
          {
            cout << "successfully read sub Header data for frame " << frameNumber << "." << endl;
            delete subHeader;
          }
          else
            cout << "no Sub Header data found for frame: " << frameNumber << endl;
        }
        else
          cout << endl << "No framenumber > 0 specified for displaying the subHeader data" << endl;
      }
      else
      {
        //CECAT6MainHeader* mHeader = static_cast<CECAT6MainHeader*>(file.mainHeader());
        cout << "Main header data (ECAT6):" << endl;
        cout << "------------------------" << endl;
        #warning "ECAT6 output implementation missing"
      }

      // close the ECAT file
      file.close();
    }
    else
      cout << "Error on loading file '" << argv[1] << "' as an ECAT6/7 file.\n";
  }
  else
  {
    cout << "Error: No filename was specified on the commandline" << endl;
    cout << argv[0] << " <filename> [framenumber]" << endl;
  }

  return returnCode;
}
