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

	cout << "libmedio ECAT6/7 file compare" << endl;
	cout << "-----------------------------" << endl;

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

					cout << endl << "2: Checking directory list:" << endl;					

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
					cout << endl << "3: Checking directory entries:" << endl;

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

					// and last but not least we are doing a raw compare of the matrix
					// data in each file
					cout << endl << "4: Checking datatype & RAW matrix data:" << endl;

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
																	  dirItem1->bed(),
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
														for(unsigned int i=0; i < matrixData1->count(); i++)
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
																cout << "     " << setw(8) << setfill('0') << hex << i << "| ";
																cout << setfill(' ') << setw(3) << setprecision(3) << dec << val1 << " : " << val2 << endl;
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
																cout << "     " << setw(8) << setfill('0') << hex << i << "| ";
																cout << setfill(' ') << setw(5) << dec << val1 << " : " << setw(5) << val2;
																cout << " |";
																cout << "  " << setfill('0') << setw(4) << hex << val1 << " : " << setw(4) << val2;
																cout << " |" << endl;
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
								cout << "NOT found in both files";
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
