/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004 by Jens Langner <Jens.Langner@light-speed.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

#include <medio/CECATFile.h>
#include <medio/CECAT6MainHeader.h>
#include <medio/CECAT7MainHeader.h>

#include <iostream>

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

  // You want this, unless you mix std::couts output with C output.
  // Read  http://gcc.gnu.org/onlinedocs/libstdc++/27_io/howto.html#8 for an explanation.
  std::ios::sync_with_stdio(false);

	std::cout << "libmedio ECAT6/7 file processing example" << std::endl;
	std::cout << "----------------------------------------" << std::endl;

	// check if the user has specified a filename or not
	if(argc > 1)
	{
		char* filename = argv[1];

		CECATFile file(filename);

		// load the file
		if(file.load() && file.format() != CECATFile::Undefined)
		{
			std::cout << "Successfully loaded file '" << filename << "'" << std::endl << std::endl;

			// now that we have successfully loaded the ECAT file we can go and
			// output some statistics on it:
			std::cout << "General file information:" << std::endl;
			std::cout << "------------------------" << std::endl;
			std::cout << "ECAT format version: " << (file.format() == CECATFile::ECAT7 ? "7" : "6") << std::endl;
			std::cout << "# of frames........: " << file.numFrames() << std::endl;
			std::cout << "# of gates.........: " << file.numGates() << std::endl;
			std::cout << "# of planes........: " << file.numPlanes() << std::endl;
			std::cout << "# of bedpos........: " << file.numBedPos() << std::endl;
	
			// now that we have put out gernal information, lets print out more detailed
			// main header information
			std::cout << std::endl;

			if(file.format() == CECATFile::ECAT7)
			{
				CECAT7MainHeader* mHeader = static_cast<CECAT7MainHeader*>(file.mainHeader());
				std::cout << "Main header data (ECAT7):" << std::endl;
				std::cout << "------------------------" << std::endl;
				std::cout << "MAGIC_NUMBER............: " << mHeader->magic_Number()							<< std::endl;
				std::cout << "ORIGINAL_FILE_NAME......: " << mHeader->original_File_Name()				<< std::endl;
				std::cout << "SW_VERSION..............: "	<< mHeader->sw_Version()								<< std::endl;
				std::cout << "SYSTEM_TYPE.............: "	<< mHeader->system_Type()								<< std::endl;
				std::cout << "FILE_TYPE...............: "	<< mHeader->file_Type()									<< std::endl;
				std::cout << "SERIAL_NUMBER...........: "	<< mHeader->serial_Number()							<< std::endl;
				std::cout << "SCAN_START_TIME.........: "	<< mHeader->scan_Start_Time()						<< std::endl;
				std::cout << "ISOTOPE_NAME............: "	<< mHeader->isotope_Name()							<< std::endl;
				std::cout << "ISOTOPE_HALFLIFE........: "	<< mHeader->isotope_Halflife()					<< std::endl;
				std::cout << "RADIOPHARMACEUTICAL.....: "	<< mHeader->radiopharmaceutical()				<< std::endl;
				std::cout << "GANTRY_TILT.............: "	<< mHeader->gantry_Tilt()								<< std::endl;
				std::cout << "ROTATION_TILT...........: "	<< mHeader->gantry_Rotation()						<< std::endl;
				std::cout << "BED_ELEVATION...........: "	<< mHeader->bed_Elevation()							<< std::endl;
				std::cout << "INTRINSIC_TILT..........: "	<< mHeader->intrinsic_Tilt()						<< std::endl;
				std::cout << "WOBBLE_SPEED............: "	<< mHeader->wobble_Speed()							<< std::endl;
				std::cout << "TRANSM_SOURCE_TYPE......: "	<< mHeader->transm_Source_Type()				<< std::endl;
				std::cout << "DISTANCE_SCANNED........: "	<< mHeader->distance_Scanned()					<< std::endl;
				std::cout << "TRANSAXIAL_FOV..........: "	<< mHeader->transaxial_FOV()						<< std::endl;
				std::cout << "ANGULAR_COMPRESSION.....: "	<< mHeader->angular_Compression()				<< std::endl;
				std::cout << "COIN_SAMP_MODE..........: "	<< mHeader->coin_Samp_Mode()						<< std::endl;
				std::cout << "AXIAL_SAMP_MODE.........: "	<< mHeader->axial_Samp_Mode()						<< std::endl;
				std::cout << "ECAT_CALIBRATION_FACTOR.: "	<< mHeader->calibration_Factor()				<< std::endl;
				std::cout << "CALIBRATION_UNITS.......: "	<< mHeader->calibration_Units()					<< std::endl;
				std::cout << "CALIBRATION_UNITS_LABEL.: "	<< mHeader->calibration_Units_Label()		<< std::endl;
				std::cout << "COMPRESSION_CODE........: "	<< mHeader->compression_Code()					<< std::endl;
				std::cout << "STUDY_TYPE..............: "	<< mHeader->study_Type()								<< std::endl;
				std::cout << "PATIENT_ID..............: "	<< mHeader->patient_ID()								<< std::endl;
				std::cout << "PATIENT_NAME............: "	<< mHeader->patient_Name()							<< std::endl;
				std::cout << "PATIENT_SEX.............: "	<< mHeader->patient_Sex()								<< std::endl;
				std::cout << "PATIENT_DEXTERITY.......: "	<< mHeader->patient_Dexterity()					<< std::endl;
				std::cout << "PATIENT_AGE.............: "	<< mHeader->patient_Age()								<< std::endl;
				std::cout << "PATIENT_HEIGHT..........: "	<< mHeader->patient_Height()						<< std::endl;
				std::cout << "PATIENT_WEIGHT..........: "	<< mHeader->patient_Weight()						<< std::endl;
				std::cout << "PATIENT_BIRTH_DATE......: "	<< mHeader->patient_Birth_Date()				<< std::endl;
				std::cout << "PHYSICIAN_NAME..........: "	<< mHeader->physician_Name()						<< std::endl;
				std::cout << "OPERATOR_NAME...........: "	<< mHeader->operator_Name()							<< std::endl;
				std::cout << "STUDY_DESCRIPTION.......: "	<< mHeader->study_Description()					<< std::endl;
				std::cout << "ACQUISITION_TYPE........: "	<< mHeader->acquisition_Type()					<< std::endl;
				std::cout << "PATIENT_ORIENTATION.....: "	<< mHeader->patient_Orientation()				<< std::endl;
				std::cout << "FACILITY_NAME...........: "	<< mHeader->facility_Name()							<< std::endl;
				std::cout << "NUM_PLANES..............: "	<< mHeader->num_Planes()								<< std::endl;
				std::cout << "NUM_FRAMES..............: "	<< mHeader->num_Frames()								<< std::endl;
				std::cout << "NUM_GATES...............: "	<< mHeader->num_Gates()									<< std::endl;
				std::cout << "NUM_BED_POS.............: "	<< mHeader->num_Bed_Pos()								<< std::endl;
				std::cout << "INIT_BED_POSITION.......: "	<< mHeader->init_Bed_Position()					<< std::endl;
				std::cout << "BED_POSITION............: ";
				for(int i=0; i < 15; i++)
				{
					std::cout << mHeader->bed_Offset(i) << " ";
				}
				std::cout << std::endl;
				std::cout << "PLANE_SEPARATION........: "	<< mHeader->plane_Separation()					<< std::endl;
				std::cout << "LWR_SCTR_THRES..........: "	<< mHeader->lwr_Sctr_Thres()						<< std::endl;
				std::cout << "LWR_TRUE_THRES..........: "	<< mHeader->lwr_True_Thres()						<< std::endl;
				std::cout << "UPR_TRUE_THRES..........: "	<< mHeader->upr_True_Thres()						<< std::endl;
				std::cout << "USER_PROCESS_CODE.......: "	<< mHeader->user_Process_Code()					<< std::endl;
				std::cout << "ACQUISITION_MODE........: "	<< mHeader->acquisition_Mode()					<< std::endl;
				std::cout << "BIN_SIZE................: "	<< mHeader->bin_Size()									<< std::endl;
				std::cout << "BRANCHING_FRACTION......: "	<< mHeader->branching_Fraction()				<< std::endl;
				std::cout << "DOSE_START_TIME.........: "	<< mHeader->dose_Start_Time()						<< std::endl;
				std::cout << "DOSAGE..................: "	<< mHeader->dosage()										<< std::endl;
				std::cout << "WELL_COUNTER_CORR_FACTOR: "	<< mHeader->well_Counter_Corr_Factor()	<< std::endl;
				std::cout << "DATA_UNITS..............: "	<< mHeader->data_Units()								<< std::endl;
				std::cout << "SEPTA_STATE.............: " << mHeader->septa_State()								<< std::endl;
				std::cout << "FILL....................: ";
				for(int i=0; i < 6; i++)
				{
					std::cout << mHeader->cti_Reserved(i) << " ";
				}
				std::cout << std::endl;

				std::cout << std::endl << "Everything seems to be fine... bye." << std::endl;
			}
			else
			{
				//CECAT6MainHeader* mHeader = static_cast<CECAT6MainHeader*>(file.mainHeader());
				std::cout << "Main header data (ECAT6):" << std::endl;
				std::cout << "------------------------" << std::endl;
				#warning "ECAT6 output implementation missing"
			}
		}
		else
			std::cout << "Error on loading file '" << argv[1] << "' as an ECAT6/7 file.\n";
	}
	else
	{
		std::cout << "Error: No filename was specified on the commandline\n";
	}

	return returnCode;
}
