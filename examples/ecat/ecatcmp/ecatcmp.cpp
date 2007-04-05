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

#include <CECATFile>
#include <CECAT6MainHeader>
#include <CECAT7MainHeader>
#include <CECATDirectory>
#include <CECAT7SubHeaderScan3D>
#include <CECAT7SubHeaderAttenCorr>

#include <iostream>
#include <iomanip>

using namespace std;

//  Function:    main
//! 
//! This is a small and simple example on how to read out raw matrix data of two
//! file and compare them to eachother.
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

	cout << "libmedio ECAT6/7 file/data compare tool 1.1" << endl;
	cout << "-------------------------------------------" << endl;

	// check if the user has specified a filename or not
	if(argc > 2)
	{
		char* fileName1 = argv[1];
		char* fileName2 = argv[2];

		// open the first file
		CECATFile file1(fileName1);
		if(file1.open(QIODevice::ReadOnly) && 
			 file1.format() != CECATFile::Undefined)
		{
			cout << "Successfully loaded file1: '" << fileName1 << "'" << endl;

			// lets open the other file now
			CECATFile file2(fileName2);
			if(file2.open(QIODevice::ReadOnly))
			{
				cout << "Successfully loaded file2: '" << fileName2 << "'" << endl;
				
				if(file1.format() != file2.format())
					cout << "Error: Specified files do have a different file format. Can't compare them" << endl;
				else
				{
					// check META information about our file
					cout << endl << "1: Checking META data:" << endl;

					cout << "   FileSize: " << file1.size() << ":" << file2.size() << " ...";
					if(file1.size() == file2.size())
						cout << "ok!" << endl;
					else
						cout << "ERROR!" << endl;
					
				  cout << "   Frames #: " << file1.numFrames() << ":" << file2.numFrames() << " ...";
					if(file1.numFrames() == file2.numFrames())
						cout << "ok!" << endl;
					else
						cout << "ERROR!" << endl;

					cout << "   Planes #: " << file1.numPlanes() << ":" << file2.numPlanes() << " ...";
					if(file1.numPlanes() == file2.numPlanes())
						cout << "ok!" << endl;
					else
						cout << "ERROR!" << endl;

					cout << "   Gates  #: " << file1.numGates() << ":" << file2.numGates() << " ...";
					if(file1.numGates() == file2.numGates())
						cout << "ok!" << endl;
					else
						cout << "ERROR!" << endl;

					cout << "   BedPos #: " << file1.numBedPos() << ":" << file2.numBedPos() << " ...";
					if(file1.numBedPos() == file2.numBedPos())
						cout << "ok!" << endl;
					else
						cout << "ERROR!" << endl;

					// check all main header data
					cout << endl << "2: Checking main header data:" << endl;
					CECATMainHeader* header1;
					CECATMainHeader* header2;
					if(file1.readMainHeader(header1) == true)
					{
						if(file2.readMainHeader(header2) == true)
						{
							cout << "   RTTI: " << header1->rtti() << ":" << header2->rtti() << " ...";

							if(header1->rtti() == header2->rtti())
							{
								if(file1.format() == CECATFile::ECAT7)
								{								
									CECAT7MainHeader* eh1 = static_cast<CECAT7MainHeader*>(header1);
									CECAT7MainHeader* eh2 = static_cast<CECAT7MainHeader*>(header2);

									cout << "ok!" << endl;

									cout << "   Checking for header entry differences: " << endl;
									
									if(strcmp(eh1->magic_Number(), eh2->magic_Number()) != 0)
										cout << "     MAGIC_NUMBER............: '" << eh1->magic_Number() << "' != '" << eh2->magic_Number() << "'" << endl;
									if(strcmp(eh1->original_File_Name(), eh2->original_File_Name()) != 0)
										cout << "     ORIGINAL_FILE_NAME......: '" << eh1->original_File_Name() << "' != '" << eh2->original_File_Name() << "'" << endl;
									if(eh1->sw_Version() != eh2->sw_Version())
										cout << "     SW_VERSION..............: '" << eh1->original_File_Name() << "' != '" << eh2->original_File_Name() << "'" << endl;
									if(eh1->system_Type() != eh2->system_Type())
										cout << "     SYSTEM_TYPE.............: '" << eh1->system_Type() << "' != '" << eh2->system_Type() << "'" << endl;
									if(eh1->file_Type() != eh2->file_Type())
										cout << "     FILE_TYPE...............: '" << eh1->file_Type() << "' != '" << eh2->file_Type() << "'" << endl;
									if(strcmp(eh1->serial_Number(), eh2->serial_Number()) != 0)
										cout << "     SERIAL_NUMBER...........: '" << eh1->serial_Number() << "' != '" << eh2->serial_Number() << "'" << endl;									
									if(eh1->scan_Start_Time() != eh2->scan_Start_Time())
										cout << "     SCAN_START_TIME.........: '" << eh1->scan_Start_Time() << "' != '" << eh2->scan_Start_Time() << "'" << endl;									
									if(strcmp(eh1->isotope_Name(), eh2->isotope_Name()))
										cout << "     ISOTOPE_NAME............: '" << eh1->isotope_Name() << "' != '" << eh2->isotope_Name() << "'" << endl;									
									if(eh1->isotope_Halflife() != eh2->isotope_Halflife())
										cout << "     ISOTOPE_HALFLIFE........: '" << eh1->isotope_Halflife() << "' != '" << eh2->isotope_Halflife() << "'" << endl;									
									if(strcmp(eh1->radiopharmaceutical(), eh2->radiopharmaceutical()))
										cout << "     RADIOPHARMACEUTICAL.....: '" << eh1->radiopharmaceutical() << "' != '" << eh2->radiopharmaceutical() << "'" << endl;	
									if(eh1->gantry_Tilt() != eh2->gantry_Tilt())
										cout << "     GANTRY_TILT.............: '" << eh1->gantry_Tilt() << "' != '" << eh2->gantry_Tilt() << "'" << endl;	
									if(eh1->gantry_Rotation() != eh2->gantry_Rotation())
										cout << "     GANTRY_ROTATION.........: '" << eh1->gantry_Rotation() << "' != '" << eh2->gantry_Rotation() << "'" << endl;	
									if(eh1->bed_Elevation() != eh2->bed_Elevation())
										cout << "     BED_ELEVATION...........: '" << eh1->bed_Elevation() << "' != '" << eh2->bed_Elevation() << "'" << endl;	
									if(eh1->intrinsic_Tilt() != eh2->intrinsic_Tilt())
										cout << "     INTRINSIC_TILT..........: '" << eh1->intrinsic_Tilt() << "' != '" << eh2->intrinsic_Tilt() << "'" << endl;	
									if(eh1->wobble_Speed() != eh2->wobble_Speed())
										cout << "     WOBBLE_SPEED............: '" << eh1->wobble_Speed() << "' != '" << eh2->wobble_Speed() << "'" << endl;	
									if(eh1->transm_Source_Type() != eh2->transm_Source_Type())
										cout << "     TRANSM_SOURCE_TYPE......: '" << eh1->transm_Source_Type() << "' != '" << eh2->transm_Source_Type() << "'" << endl;	
									if(eh1->distance_Scanned() != eh2->distance_Scanned())
										cout << "     DISTANCE_SCANNED........: '" << eh1->distance_Scanned() << "' != '" << eh2->distance_Scanned() << "'" << endl;	
									if(eh1->transaxial_FOV() != eh2->transaxial_FOV())
										cout << "     TRANSAXIAL_FOV..........: '" << eh1->transaxial_FOV() << "' != '" << eh2->transaxial_FOV() << "'" << endl;
									if(eh1->angular_Compression() != eh2->angular_Compression())
										cout << "     ANGULAR_COMPRESSION.....: '" << eh1->angular_Compression() << "' != '" << eh2->angular_Compression() << "'" << endl;
									if(eh1->coin_Samp_Mode() != eh2->coin_Samp_Mode())
										cout << "     COIN_SAMP_MODE..........: '" << eh1->coin_Samp_Mode() << "' != '" << eh2->coin_Samp_Mode() << "'" << endl;
									if(eh1->axial_Samp_Mode() != eh2->axial_Samp_Mode())
										cout << "     AXIAL_SAMP_MODE.........: '" << eh1->axial_Samp_Mode() << "' != '" << eh2->axial_Samp_Mode() << "'" << endl;
									if(eh1->calibration_Factor() != eh2->calibration_Factor())
										cout << "     CALIBRATION_FACTOR......: '" << eh1->calibration_Factor() << "' != '" << eh2->calibration_Factor() << "'" << endl;
									if(eh1->calibration_Units() != eh2->calibration_Units())
										cout << "     CALIBRATION_UNITS.......: '" << eh1->calibration_Units() << "' != '" << eh2->calibration_Units() << "'" << endl;
									if(eh1->calibration_Units_Label() != eh2->calibration_Units_Label())
										cout << "     CALIBRATION_UNITS_LABEL.: '" << eh1->calibration_Units_Label() << "' != '" << eh2->calibration_Units_Label() << "'" << endl;
									if(eh1->compression_Code() != eh2->compression_Code())
										cout << "     COMPRESSION_CODE........: '" << eh1->compression_Code() << "' != '" << eh2->compression_Code() << "'" << endl;
									if(strcmp(eh1->study_Type(), eh2->study_Type()))
										cout << "     STUDY_TYPE..............: '" << eh1->study_Type() << "' != '" << eh2->study_Type() << "'" << endl;
									if(strcmp(eh1->patient_ID(), eh2->patient_ID()))
										cout << "     PATIENT_ID..............: '" << eh1->patient_ID() << "' != '" << eh2->patient_ID() << "'" << endl;
									if(strcmp(eh1->patient_Name(), eh2->patient_Name()))
										cout << "     PATIENT_NAME............: '" << eh1->patient_Name() << "' != '" << eh2->patient_Name() << "'" << endl;
									if(eh1->patient_Sex() != eh2->patient_Sex())
										cout << "     PATIENT_SEX.............: '" << eh1->patient_Sex() << "' != '" << eh2->patient_Sex() << "'" << endl;
									if(eh1->patient_Dexterity() != eh2->patient_Dexterity())
										cout << "     PATIENT_DEXTERITY.......: '" << eh1->patient_Dexterity() << "' != '" << eh2->patient_Dexterity() << "'" << endl;
									if(eh1->patient_Age() != eh2->patient_Age())
										cout << "     PATIENT_AGE.............: '" << eh1->patient_Age() << "' != '" << eh2->patient_Age() << "'" << endl;
									if(eh1->patient_Height() != eh2->patient_Height())
										cout << "     PATIENT_HEIGHT..........: '" << eh1->patient_Height() << "' != '" << eh2->patient_Height() << "'" << endl;
									if(eh1->patient_Weight() != eh2->patient_Weight())
										cout << "     PATIENT_WEIGHT..........: '" << eh1->patient_Weight() << "' != '" << eh2->patient_Weight() << "'" << endl;
									if(eh1->patient_Birth_Date() != eh2->patient_Birth_Date())
										cout << "     PATIENT_BIRTH_DATE......: '" << eh1->patient_Birth_Date() << "' != '" << eh2->patient_Birth_Date() << "'" << endl;
									if(strcmp(eh1->physician_Name(), eh2->physician_Name()))
										cout << "     PATIENT_NAME............: '" << eh1->physician_Name() << "' != '" << eh2->physician_Name() << "'" << endl;
									if(strcmp(eh1->operator_Name(), eh2->operator_Name()))
										cout << "     OPERATOR_NAME...........: '" << eh1->operator_Name() << "' != '" << eh2->operator_Name() << "'" << endl;
									if(strcmp(eh1->study_Description(), eh2->study_Description()))
										cout << "     STUDY_DESCRIPTION.......: '" << eh1->study_Description() << "' != '" << eh2->study_Description() << "'" << endl;
									if(eh1->acquisition_Type() != eh2->acquisition_Type())
										cout << "     ACQUISITION_TYPE........: '" << eh1->acquisition_Type() << "' != '" << eh2->acquisition_Type() << "'" << endl;
									if(eh1->patient_Orientation() != eh2->patient_Orientation())
										cout << "     PATIENT_ORIENTATION.....: '" << eh1->patient_Orientation() << "' != '" << eh2->patient_Orientation() << "'" << endl;
									if(strcmp(eh1->facility_Name(), eh2->facility_Name()))
										cout << "     FACILITY_NAME...........: '" << eh1->facility_Name() << "' != '" << eh2->facility_Name() << "'" << endl;
									if(eh1->num_Planes() != eh2->num_Planes())
										cout << "     NUM_PLANES..............: '" << eh1->num_Planes() << "' != '" << eh2->num_Planes() << "'" << endl;
									if(eh1->num_Frames() != eh2->num_Frames())
										cout << "     NUM_FRAMES..............: '" << eh1->num_Frames() << "' != '" << eh2->num_Frames() << "'" << endl;
									if(eh1->num_Gates() != eh2->num_Gates())
										cout << "     NUM_GATES...............: '" << eh1->num_Gates() << "' != '" << eh2->num_Gates() << "'" << endl;
									if(eh1->num_Bed_Pos() != eh2->num_Bed_Pos())
										cout << "     NUM_BED_POS.............: '" << eh1->num_Bed_Pos() << "' != '" << eh2->num_Bed_Pos() << "'" << endl;
									if(eh1->init_Bed_Position() != eh2->init_Bed_Position())
										cout << "     INIT_BED_POSITION.......: '" << eh1->init_Bed_Position() << "' != '" << eh2->init_Bed_Position() << "'" << endl;
									for(int i=0; i < 15; i++)
									{
										if(eh1->bed_Offset(i) != eh2->bed_Offset(i))
											cout << "     BED_OFFSET[" << setw(2) << i << "]..........: '" << eh1->bed_Offset(i) << "' != '" << eh2->bed_Offset(i) << "'" << endl;
									}
									if(eh1->plane_Separation() != eh2->plane_Separation())
										cout << "     PLANE_SEPARATION........: '" << eh1->plane_Separation() << "' != '" << eh2->plane_Separation() << "'" << endl;
									if(eh1->lwr_Sctr_Thres() != eh2->lwr_Sctr_Thres())
										cout << "     LWR_SCTR_THRES..........: '" << eh1->lwr_Sctr_Thres() << "' != '" << eh2->lwr_Sctr_Thres() << "'" << endl;
									if(eh1->lwr_True_Thres() != eh2->lwr_True_Thres())
										cout << "     LWR_TRUE_THRES..........: '" << eh1->lwr_True_Thres() << "' != '" << eh2->lwr_True_Thres() << "'" << endl;
									if(eh1->upr_True_Thres() != eh2->upr_True_Thres())
										cout << "     UPR_TRUE_THRES..........: '" << eh1->upr_True_Thres() << "' != '" << eh2->upr_True_Thres() << "'" << endl;
									if(strcmp(eh1->user_Process_Code(), eh2->user_Process_Code()) != 0)
										cout << "     USER_PROCESS_CODE.......: '" << eh1->user_Process_Code() << "' != '" << eh2->user_Process_Code() << "'" << endl;
									if(eh1->acquisition_Mode() != eh2->acquisition_Mode())
										cout << "     ACQUISITION_MODE........: '" << eh1->acquisition_Mode() << "' != '" << eh2->acquisition_Mode() << "'" << endl;
									if(eh1->bin_Size() != eh2->bin_Size())
										cout << "     BIN_SIZE................: '" << eh1->bin_Size() << "' != '" << eh2->bin_Size() << "'" << endl;
									if(eh1->branching_Fraction() != eh2->branching_Fraction())
										cout << "     BRANCHING_FRACTION......: '" << eh1->branching_Fraction() << "' != '" << eh2->branching_Fraction() << "'" << endl;
									if(eh1->dose_Start_Time() != eh2->dose_Start_Time())
										cout << "     DOSE_START_TIME.........: '" << eh1->dose_Start_Time() << "' != '" << eh2->dose_Start_Time() << "'" << endl;
									if(eh1->dosage() != eh2->dosage())
										cout << "     DOSAGE..................: '" << eh1->dosage() << "' != '" << eh2->dosage() << "'" << endl;
									if(eh1->well_Counter_Corr_Factor() != eh2->well_Counter_Corr_Factor())
										cout << "     WELL_COUNTER_CORR_FACTOR: '" << eh1->well_Counter_Corr_Factor() << "' != '" << eh2->well_Counter_Corr_Factor() << "'" << endl;
									if(strcmp(eh1->data_Units(), eh2->data_Units()) != 0)
										cout << "     DATA_UNITS..............: '" << eh1->data_Units() << "' != '" << eh2->data_Units() << "'" << endl;
									if(eh1->septa_State() != eh2->septa_State())
										cout << "     SEPTA_STATE.............: '" << eh1->septa_State() << "' != '" << eh2->septa_State() << "'" << endl;
									for(int i=0; i < 6; i++)
									{
										if(eh1->cti_Reserved(i) != eh2->cti_Reserved(i))
											cout << "     CTI_RESERVED[" << i << "].........: '" << eh1->cti_Reserved(i) << "' != '" << eh2->cti_Reserved(i) << "'" << endl;
									}
									
								}
								else
									cout << "WARNING! only mainheader checking of ECAT7 files supported." << endl;
							}
							else
								cout << "ERROR!" << endl;

							delete header2;
						}
						else 
							cout << "   ERROR! while reading mainheader data of file2." << endl;

						delete header1;
					}
					else
						cout << "   ERROR! while reading mainheader data of file1." << endl;

					// check/compare the directory list
					cout << endl << "3: Checking directory list:" << endl;					

					CECATDirectory* dir1 = file1.directory();
					CECATDirectory* dir2 = file2.directory();

					cout << "   Number of direntries: " << dir1->count() << ":" << dir2->count() << " ...";
					if(dir1->count() == dir2->count())
						cout << "ok!" << endl;
					else
						cout << "ERROR!" << endl;

					// lets iterate through the dirItems
					cout << endl << "   #  | Matrix-ID  SubHdrPos   MatrEndPos  Stat | Matrix-ID  SubHdrPos   MatrEndPos  Stat" << endl;
					for(unsigned int i=0; i < dir1->count() || i < dir2->count(); i++)
					{
						CECATDirectoryItem* dirItem1 = (*dir1)[i];
						CECATDirectoryItem* dirItem2 = (*dir2)[i];

						cout << "   " << setw(2) << setfill('0') <<  i+1 << " |";

						if(dirItem1)
						{
							cout << " "   << setw(8) << setfill('0') << std::hex << dirItem1->matrixID();
							cout << "   " << std::dec << setw(10) << dirItem1->dataBlock_Start();
							cout << "  "  << std::dec << setw(10) << dirItem1->dataBlock_End();
							cout << "   " << std::dec << setw(1) << dirItem1->dataBlock_Status();
						}
						else
							cout << " no entry                             ";

						cout << "   |";
						if(dirItem2)
						{
							cout << " "   << setw(8) << setfill('0') << std::hex << dirItem2->matrixID();
							cout << "   " << std::dec << setw(10) << dirItem2->dataBlock_Start();
							cout << "  "  << std::dec << setw(10) << dirItem2->dataBlock_End();
							cout << "   " << std::dec << setw(1) << dirItem2->dataBlock_Status() << endl;
						}
						else
							cout << " no entry" << endl;
					}

					// now check if one entry in the matrix matches the other one
					cout << endl << "4: Checking directory entries:" << endl;

					for(unsigned int i=0; i < dir1->count() || i < dir2->count(); i++)
					{
						CECATDirectoryItem* dirItem1 = (*dir1)[i];
						CECATDirectoryItem* dirItem2;

						cout << "   DirItem #" << setw(2) << setfill('0') << i+1 << ": ";

						if(dirItem1 == NULL)
						{
							dirItem1 = (*dir2)[i];

							if(dirItem1)
							{
								dirItem2 = dir1->item(dirItem1->frame(),
																		  dirItem1->plane(),
																		  dirItem1->gate(),
																		  dirItem1->bed(),
																			dirItem1->data());
							}
						}
						else
						{
							dirItem2 = dir2->item(dirItem1->frame(),
																	  dirItem1->plane(),
																	  dirItem1->gate(),
																	  dirItem1->bed(),
																		dirItem1->data());							
						}
							

						if(dirItem1)
						{
							if(dirItem2)
							{
								cout << "found in both files, size ";
								
								if((dirItem2->dataBlock_End() - dirItem2->dataBlock_Start()) ==
								   (dirItem1->dataBlock_End() - dirItem1->dataBlock_Start()))
								{
									cout << "matches, status is ";
									
									if(dirItem1->dataBlock_Status() == dirItem2->dataBlock_Status())
										cout << "equal... ok!";
									else
										cout << "NOT equal... ERROR!";
								}
								else
								{
									cout << "DOES NOT match! ";
									cout << (dirItem1->dataBlock_End() - dirItem1->dataBlock_Start()) << " : ";
									cout << (dirItem2->dataBlock_End() - dirItem2->dataBlock_Start()) << "... ERROR!";
								}
							}
							else
								cout << "NOT found in both files";
						}
						else
							cout << "Unknown error";

						cout << endl;
					}

					// now we check all subheaders of all our diritems
					cout << endl << "5: Checking for differences in subheaders:" << endl;

					for(unsigned int i=0; i < dir1->count() || i < dir2->count(); i++)
					{
						CECATDirectoryItem* dirItem1 = (*dir1)[i];
						CECATDirectoryItem* dirItem2;

						cout << "   DirItem #" << setw(2) << setfill('0') << i+1 << ":" << endl;

						if(dirItem1 == NULL)
						{
							dirItem1 = (*dir2)[i];

							if(dirItem1)
							{
								dirItem2 = dir1->item(dirItem1->frame(),
																		  dirItem1->plane(),
																		  dirItem1->gate(),
																		  dirItem1->bed(),
																			dirItem1->data());
							}
						}
						else
						{
							dirItem2 = dir2->item(dirItem1->frame(),
																	  dirItem1->plane(),
																	  dirItem1->gate(),
																	  dirItem1->bed(),
																		dirItem1->data());							
						}
							

						if(dirItem1)
						{
							if(dirItem2)
							{
								// get the subheader of both items and compare them accordingly
								CECATSubHeader* subHeader1;
								CECATSubHeader* subHeader2;

								if(dirItem1->readSubHeader(subHeader1))
								{
									if(dirItem2->readSubHeader(subHeader2))
									{
										if(subHeader1->subHeaderType() == subHeader2->subHeaderType())
										{
											switch(subHeader1->subHeaderType())
											{
												case CECATSubHeader::ECAT7_Scan3D:
												{
													CECAT7SubHeaderScan3D* sh1 = static_cast<CECAT7SubHeaderScan3D*>(subHeader1);
													CECAT7SubHeaderScan3D* sh2 = static_cast<CECAT7SubHeaderScan3D*>(subHeader2);

													if(sh1->data_Type() != sh2->data_Type())
														cout << "     DATA_TYPE...............: '" << sh1->data_Type() << "' != '" << sh2->data_Type() << "'" << endl;
													if(sh1->num_Dimensions() != sh2->num_Dimensions())
														cout << "     NUM_DIMENSIONS..........: '" << sh1->num_Dimensions() << "' != '" << sh2->num_Dimensions() << "'" << endl;
													if(sh1->num_R_Elements() != sh2->num_R_Elements())
														cout << "     NUM_R_ELEMENTS..........: '" << sh1->num_R_Elements() << "' != '" << sh2->num_R_Elements() << "'" << endl;
													if(sh1->num_Angles() != sh2->num_Angles())
														cout << "     NUM_ANGLES..............: '" << sh1->num_Angles() << "' != '" << sh2->num_Angles() << "'" << endl;
													if(sh1->corrections_Applied() != sh2->corrections_Applied())
														cout << "     CORRECTIONS_APPLIED.....: '" << sh1->corrections_Applied() << "' != '" << sh2->corrections_Applied() << "'" << endl;
													for(short i=0; i < 64; i++)
													{
														if(sh1->num_Z_Elements(i) != sh2->num_Z_Elements(i))
															cout << "     NUM_Z_ELEMENTS[" << setw(2) << i << "]......: '" << sh1->num_Z_Elements(i) << "' != '" << sh2->num_Z_Elements(i) << "'" << endl;
													}
													if(sh1->ring_Difference() != sh2->ring_Difference())
														cout << "     RING_DIFFERENCE.........: '" << sh1->ring_Difference() << "' != '" << sh2->ring_Difference() << "'" << endl;
													if(sh1->storage_Order() != sh2->storage_Order())
														cout << "     STORAGE_ORDER...........: '" << sh1->storage_Order() << "' != '" << sh2->storage_Order() << "'" << endl;
													if(sh1->axial_Compression() != sh2->axial_Compression())
														cout << "     AXIAL_COMPRESSION.......: '" << sh1->axial_Compression() << "' != '" << sh2->axial_Compression() << "'" << endl;
													if(sh1->X_Resolution() != sh2->X_Resolution())
														cout << "     X_RESOLUTION............: '" << sh1->X_Resolution() << "' != '" << sh2->X_Resolution() << "'" << endl;
													if(sh1->V_Resolution() != sh2->V_Resolution())
														cout << "     Y_RESOLUTION............: '" << sh1->V_Resolution() << "' != '" << sh2->V_Resolution() << "'" << endl;
													if(sh1->Z_Resolution() != sh2->Z_Resolution())
														cout << "     Z_RESOLUTION............: '" << sh1->Z_Resolution() << "' != '" << sh2->Z_Resolution() << "'" << endl;
													if(sh1->W_Resolution() != sh2->W_Resolution())
														cout << "     W_RESOLUTION............: '" << sh1->W_Resolution() << "' != '" << sh2->W_Resolution() << "'" << endl;
													for(short i=0; i < 6; i++)
													{
														if(sh1->gate_Reserved(i) != sh2->gate_Reserved(i))
															cout << "     GATE_RESERVED[" << i << "]........: '" << sh1->gate_Reserved(i) << "' != '" << sh2->gate_Reserved(i) << "'" << endl;
													}
													if(sh1->gate_Duration() != sh2->gate_Duration())
														cout << "     GATE_DURATION...........: '" << sh1->gate_Duration() << "' != '" << sh2->gate_Duration() << "'" << endl;
													if(sh1->r_Wave_Offset() != sh2->r_Wave_Offset())
														cout << "     R_WAVE_OFFSET...........: '" << sh1->r_Wave_Offset() << "' != '" << sh2->r_Wave_Offset() << "'" << endl;
													if(sh1->num_Accepted_Beats() != sh2->num_Accepted_Beats())
														cout << "     NUM_ACCEPTED_BEATS......: '" << sh1->num_Accepted_Beats() << "' != '" << sh2->num_Accepted_Beats() << "'" << endl;
													if(sh1->scale_Factor() != sh2->scale_Factor())
														cout << "     SCALE_FACTOR............: '" << sh1->scale_Factor() << "' != '" << sh2->scale_Factor() << "'" << endl;
													if(sh1->scan_Min() != sh2->scan_Min())
														cout << "     SCAN_MIN................: '" << sh1->scan_Min() << "' != '" << sh2->scan_Min() << "'" << endl;
													if(sh1->scan_Max() != sh2->scan_Max())
														cout << "     SCAN_MAX................: '" << sh1->scan_Max() << "' != '" << sh2->scan_Max() << "'" << endl;
													if(sh1->prompts() != sh2->prompts())
														cout << "     PROMPTS.................: '" << sh1->prompts() << "' != '" << sh2->prompts() << "'" << endl;
													if(sh1->delayed() != sh2->delayed())
														cout << "     DELAYED.................: '" << sh1->delayed() << "' != '" << sh2->delayed() << "'" << endl;
													if(sh1->multiples() != sh2->multiples())
														cout << "     MULTIPLES...............: '" << sh1->multiples() << "' != '" << sh2->multiples() << "'" << endl;
													if(sh1->net_Trues() != sh2->net_Trues())
														cout << "     NET_TRUES...............: '" << sh1->net_Trues() << "' != '" << sh2->net_Trues() << "'" << endl;
													if(sh1->tot_Avg_Cor() != sh2->tot_Avg_Cor())
														cout << "     TOT_AVG_COR.............: '" << sh1->tot_Avg_Cor() << "' != '" << sh2->tot_Avg_Cor() << "'" << endl;
													if(sh1->tot_Avg_Uncor() != sh2->tot_Avg_Uncor())
														cout << "     TOT_AVG_UNCOR...........: '" << sh1->tot_Avg_Uncor() << "' != '" << sh2->tot_Avg_Uncor() << "'" << endl;
													if(sh1->total_Coin_Rate() != sh2->total_Coin_Rate())
														cout << "     TOTAL_COIN_RATE.........: '" << sh1->total_Coin_Rate() << "' != '" << sh2->total_Coin_Rate() << "'" << endl;
													if(sh1->frame_Start_Time() != sh2->frame_Start_Time())
														cout << "     FRAME_START_TIME........: '" << sh1->frame_Start_Time() << "' != '" << sh2->frame_Start_Time() << "'" << endl;
													if(sh1->frame_Duration() != sh2->frame_Duration())
														cout << "     FRAME_DURATION..........: '" << sh1->frame_Duration() << "' != '" << sh2->frame_Duration() << "'" << endl;
													if(sh1->deadtime_Correction_Factor() != sh2->deadtime_Correction_Factor())
														cout << "     DEADTIME_CORR_FACTOR....: '" << sh1->deadtime_Correction_Factor() << "' != '" << sh2->deadtime_Correction_Factor() << "'" << endl;
													for(short i=0; i < 90; i++)
													{
														if(sh1->cti_Reserved(i) != sh2->cti_Reserved(i))
															cout << "     CTI_RESERVED[" << setw(2) << i << "]........: '" << sh1->cti_Reserved(i) << "' != '" << sh2->cti_Reserved(i) << "'" << endl;
													}
													for(short i=0; i < 50; i++)
													{
														if(sh1->user_Reserved(i) != sh2->user_Reserved(i))
															cout << "     USER_RESERVED[" << setw(2) << i << "].......: '" << sh1->user_Reserved(i) << "' != '" << sh2->user_Reserved(i) << "'" << endl;
													}
													for(short i=0; i < 128; i++)
													{
														if(sh1->uncor_Singles(i) != sh2->uncor_Singles(i))
															cout << "     UNCOR_SINGLES[" << setw(3) << i << "]......: '" << sh1->uncor_Singles(i) << "' != '" << sh2->uncor_Singles(i) << "'" << endl;
													}

												}
												break;
												case CECATSubHeader::ECAT7_AttenCorr:
												{
													CECAT7SubHeaderAttenCorr* sh1 = static_cast<CECAT7SubHeaderAttenCorr*>(subHeader1);
													CECAT7SubHeaderAttenCorr* sh2 = static_cast<CECAT7SubHeaderAttenCorr*>(subHeader2);

													if(sh1->data_Type() != sh2->data_Type())
														cout << "     DATA_TYPE...............: '" << sh1->data_Type() << "' != '" << sh2->data_Type() << "'" << endl;
													if(sh1->num_Dimensions() != sh2->num_Dimensions())
														cout << "     NUM_DIMENSIONS..........: '" << sh1->num_Dimensions() << "' != '" << sh2->num_Dimensions() << "'" << endl;
													if(sh1->attenuation_Type() != sh2->attenuation_Type())
														cout << "     ATTENUATION_TYPE........: '" << sh1->attenuation_Type() << "' != '" << sh2->attenuation_Type() << "'" << endl;
													if(sh1->num_R_Elements() != sh2->num_R_Elements())
														cout << "     NUM_R_ELEMENTS..........: '" << sh1->num_R_Elements() << "' != '" << sh2->num_R_Elements() << "'" << endl;
													if(sh1->num_Angles() != sh2->num_Angles())
														cout << "     NUM_ANGLES..............: '" << sh1->num_Angles() << "' != '" << sh2->num_Angles() << "'" << endl;
													for(short i=0; i < 64; i++)
													{
														if(sh1->z_Elements(i) != sh2->z_Elements(i))
															cout << "     NUM_Z_ELEMENTS[" << setw(2) << i << "]......: '" << sh1->z_Elements(i) << "' != '" << sh2->z_Elements(i) << "'" << endl;
													}
													if(sh1->ring_Difference() != sh2->ring_Difference())
														cout << "     RING_DIFFERENCE.........: '" << sh1->ring_Difference() << "' != '" << sh2->ring_Difference() << "'" << endl;
													if(sh1->x_Resolution() != sh2->x_Resolution())
														cout << "     X_RESOLUTION............: '" << sh1->x_Resolution() << "' != '" << sh2->x_Resolution() << "'" << endl;
													if(sh1->y_Resolution() != sh2->y_Resolution())
														cout << "     Y_RESOLUTION............: '" << sh1->y_Resolution() << "' != '" << sh2->y_Resolution() << "'" << endl;
													if(sh1->z_Resolution() != sh2->z_Resolution())
														cout << "     Z_RESOLUTION............: '" << sh1->z_Resolution() << "' != '" << sh2->z_Resolution() << "'" << endl;
													if(sh1->w_Resolution() != sh2->w_Resolution())
														cout << "     W_RESOLUTION............: '" << sh1->w_Resolution() << "' != '" << sh2->w_Resolution() << "'" << endl;
													if(sh1->scale_Factor() != sh2->scale_Factor())
														cout << "     SCALE_FACTOR............: '" << sh1->scale_Factor() << "' != '" << sh2->scale_Factor() << "'" << endl;
													if(sh1->x_Offset() != sh2->x_Offset())
														cout << "     X_OFFSET................: '" << sh1->x_Offset() << "' != '" << sh2->x_Offset() << "'" << endl;
													if(sh1->y_Offset() != sh2->y_Offset())
														cout << "     X_OFFSET................: '" << sh1->y_Offset() << "' != '" << sh2->y_Offset() << "'" << endl;
													if(sh1->x_Radius() != sh2->x_Radius())
														cout << "     X_RADIUS................: '" << sh1->x_Radius() << "' != '" << sh2->x_Radius() << "'" << endl;
													if(sh1->y_Radius() != sh2->y_Radius())
														cout << "     Y_RADIUS................: '" << sh1->y_Radius() << "' != '" << sh2->y_Radius() << "'" << endl;
													if(sh1->tilt_Angle() != sh2->tilt_Angle())
														cout << "     TILT_ANGLE..............: '" << sh1->tilt_Angle() << "' != '" << sh2->tilt_Angle() << "'" << endl;
													if(sh1->skull_Thickness() != sh2->skull_Thickness())
														cout << "     SKULL_THICKNESS.........: '" << sh1->skull_Thickness() << "' != '" << sh2->skull_Thickness() << "'" << endl;
													if(sh1->num_Additional_Atten_Coeff() != sh2->num_Additional_Atten_Coeff())
														cout << "     NUM_ADDITIONAL_ATTEN_COEFF: '" << sh1->num_Additional_Atten_Coeff() << "' != '" << sh2->num_Additional_Atten_Coeff() << "'" << endl;
													for(short i=0; i < 8; i++)
													{
														if(sh1->additional_Atten_Coeff(i) != sh2->additional_Atten_Coeff(i))
															cout << "     ADDITIONAL_ATTEN_COEFF[" << setw(2) << i << "]........: '" << sh1->additional_Atten_Coeff(i) << "' != '" << sh2->additional_Atten_Coeff(i) << "'" << endl;
													}
													if(sh1->edge_Finding_Threshold() != sh2->edge_Finding_Threshold())
														cout << "     EDGE_FINDING_THRESHOLD..: '" << sh1->edge_Finding_Threshold() << "' != '" << sh2->edge_Finding_Threshold() << "'" << endl;
													if(sh1->storage_Order() != sh2->storage_Order())
														cout << "     STORAGE_ORDER...........: '" << sh1->storage_Order() << "' != '" << sh2->storage_Order() << "'" << endl;
													if(sh1->span() != sh2->span())
														cout << "     SPAN....................: '" << sh1->span() << "' != '" << sh2->span() << "'" << endl;

													for(short i=0; i < 90; i++)
													{
														if(sh1->cti_Reserved(i) != sh2->cti_Reserved(i))
															cout << "     CTI_RESERVED[" << setw(2) << i << "]........: '" << sh1->cti_Reserved(i) << "' != '" << sh2->cti_Reserved(i) << "'" << endl;
													}
													for(short i=0; i < 50; i++)
													{
														if(sh1->unused(i) != sh2->unused(i))
															cout << "     USER_RESERVED[" << setw(2) << i << "].......: '" << sh1->unused(i) << "' != '" << sh2->unused(i) << "'" << endl;
													}
												}
												break;

												default:
													cout << "     WARNING! compare of subHeaderType " << subHeader1->subHeaderType() << " currently not supported!" << endl;
											}

										}
										else
											cout << "     ERROR! subheader type of file1 and file2 are different!" << endl;

										delete subHeader2;
									}
									else
										cout << "     ERROR! on reading subheader of file2" << endl;

									delete subHeader1;
								}
								else
									cout << "     ERROR! on reading subheader of file1" << endl;
							}
							else
								cout << "     ERROR! couldn't find same diritem in file2" << endl;
						}
						else
							cout << "     ERROR! couldn't find diritem in file 1" << endl;
					}					

					// and last but not least we are doing a raw compare of the matrix
					// data in each file
					cout << endl << "6: Checking datatype & RAW matrix data:" << endl;

					for(unsigned int i=0; i < dir1->count() || i < dir2->count(); i++)
					{
						CECATDirectoryItem* dirItem1 = (*dir1)[i];
						CECATDirectoryItem* dirItem2;

						if(dirItem1 == NULL)
						{
							dirItem1 = (*dir2)[i];

							if(dirItem1)
							{
								dirItem2 = dir1->item(dirItem1->frame(),
																		  dirItem1->plane(),
																		  dirItem1->gate(),
																		  dirItem1->bed(),
																			dirItem1->data());
							}
						}
						else
						{
							dirItem2 = dir2->item(dirItem1->frame(),
																	  dirItem1->plane(),
																	  dirItem1->gate(),
																	  //dirItem1->bed(),
																		2,
																		dirItem1->data());							
						}
							
						if(dirItem1)
						{
							if(dirItem2)
							{
								cout << "   Matrix: " << hex << dirItem1->matrixID() << dec << "  ";

								// find out the datatype out of each sub header
								CECATSubHeader* subHeader1;
								CECATSubHeader* subHeader2;

								cout << "Type: ";
								dirItem1->readSubHeader(subHeader1);
								dirItem2->readSubHeader(subHeader2);

								cout << subHeader1->data_Type() << ":" << subHeader2->data_Type() << "... ";

								if(subHeader1->data_Type() != subHeader2->data_Type())
									cout << "ERROR: different data types! ";
								else
									cout << "ok!";

								if(subHeader1->subHeaderType() != subHeader2->subHeaderType())
									cout << "And different subHeaderTypes found! ";

								cout << endl;

								CECATSubHeader::Data_Type subHeaderType = subHeader1->data_Type();

								delete subHeader1;
								delete subHeader2;

								// now we load the data out of each matrix and compare it bytewise.
								QByteArray* matrixData1;
								QByteArray* matrixData2;

								cout << "     loading data... " << flush;
								if(dirItem1->readMatrix(matrixData1) == false)
									cout << "ERROR on loading data from file1!";
								else
								{
									cout << "file1 ok, " << flush;
									
									if(dirItem2->readMatrix(matrixData2) == false)
										cout << "ERROR on loading data from file2!";
									else
									{
										cout << "file2... ok!" << endl;

										// check the size
										cout << "     comparing data size " << matrixData1->size() << ":" << matrixData2->size() << "... " << flush;
										if(matrixData1->size() != matrixData2->size())
											cout << "ERROR: matrix sizes are not equal!";
										else
										{
											cout << "ok!" << endl;

											// compare the raw data
											cout << "     comparing raw data... " << flush;
											if(*matrixData1 != *matrixData2)
											{
												long diffs = 0;
												short dataTypeSize = 0;
												
												cout << "ERROR: matrix is not equal!" << endl;
												cout << "     performing deeper search for differences... " << endl << endl;

												// depending on the datatype we have to do different conversions
												switch(subHeaderType)
												{
													case CECATSubHeader::ByteData:
													{
														dataTypeSize = sizeof(char);
														cout << "BYTE datatype" << endl;
														cout << "        Pos      Value1   Value2" << endl;  

														// now we search through the byte array step by step, outputing all
														// differences
														for(unsigned int i=0; i < (unsigned int)matrixData1->count(); i++)
														{
															char c1 = (*matrixData1)[i];
															char c2 = (*matrixData2)[i];

															// do a bytewise compare
															if(c1 != c2)
															{
																cout << "     " << setw(8) << setfill('0') << hex << i << ": ";
																cout << setfill(' ') << setw(3) << dec << (qint8)c1 << " : " << (qint8)c2 << endl;
																diffs++;
															}
														}
													}
													break;

													case CECATSubHeader::IEEEFloat:
													{
														dataTypeSize = sizeof(float);
														
														float val1;
														float val2;

														QDataStream stream1(matrixData1, QIODevice::ReadOnly);
														QDataStream stream2(matrixData2, QIODevice::ReadOnly);

														cout << "     IEEEFloat datatype" << endl;
														for(int i=0; stream1.atEnd() == false; i++)
														{
															stream1 >> val1;
															stream2 >> val2;

															if(val1 != val2)
															{
																//cout << "     " << setw(8) << setfill('0') << hex << i << "| ";
																//cout << setfill(' ') << setw(3) << setprecision(3) << dec << val1 << " : " << val2 << endl;
																diffs++;
															}
														}
													}
													break;													

													case CECATSubHeader::SunShort:
													{
														dataTypeSize = sizeof(qint16);
														qint16 val1;
														qint16 val2;

														QDataStream stream1(matrixData1, QIODevice::ReadOnly);
														QDataStream stream2(matrixData2, QIODevice::ReadOnly);

														cout << "     SunShort datatype" << endl;
														cout << "        Pos    Value1 Value2    HEX    HEX" << endl; 														
														for(int i=0; stream1.atEnd() == false; i++)
														{
															stream1 >> val1;
															stream2 >> val2;

															if(val1 != val2)
															{
																//cout << "     " << setw(8) << setfill('0') << hex << i << "| ";
																//cout << setfill(' ') << setw(5) << dec << val1 << " : " << setw(5) << val2;
																//cout << " |";
																//cout << "  " << setfill('0') << setw(4) << hex << val1 << " : " << setw(4) << val2;
																//cout << " |" << endl;
																diffs++;
															}
														}
													}
													break;

													case CECATSubHeader::SunLong:
													{
														dataTypeSize = sizeof(qint32);
														qint32 val1;
														qint32 val2;

														QDataStream stream1(matrixData1, QIODevice::ReadOnly);
														QDataStream stream2(matrixData2, QIODevice::ReadOnly);

														cout << "     SunLong datatype" << endl;
														cout << "        Pos     Value1   Value2       HEX       HEX" << endl; 														
														for(int i=0; stream1.atEnd() == false; i++)
														{
															stream1 >> val1;
															stream2 >> val2;

															if(val1 != val2)
															{
																cout << "     " << setw(8) << setfill('0') << hex << i << "| ";
																cout << setfill(' ') << setw(8) << dec << val1 << " : " << setw(8) << val2;
																cout << " |";
																cout << " " << setfill('0') << setw(8) << hex << val1 << " : " << setw(8) << val2;
																cout << " |" << endl;
																diffs++;
															}
														}
													}
													break;													

													default:
														cout << "ERROR: This datatype is currently not support by ecatcmp!" << endl;
													break;
												}

												if(dataTypeSize > 0)
													cout << endl << dec << "     " << diffs << " differences in " << matrixData1->size()/dataTypeSize << " values found!";
											}
											else
												cout << "equal data... ok!";
										}
									}
								}

								cout << endl;

								delete matrixData1;
								delete matrixData2;
							}
							else
								cout << "NOT found in both files" << endl;
						}
						else
							cout << "Unknown error";
					}

					cout << endl;
					cout << "Thanks for using libmedio!.. bye" << endl;
				}

				file2.close();
			}
			else
				cout << "Error on loading file '" << fileName2 << "' as an ECAT6/7 file.\n";

			// close the ECAT file
			file1.close();
		}
		else
			cout << "Error on loading file '" << fileName1 << "' as an ECAT6/7 file.\n";
	}
	else
	{
		cout << "Error: Not enough filenames were specified on the commandline" << endl;
		cout << argv[0] << " <file1> <file2>" << endl;
	}

	return returnCode;
}
