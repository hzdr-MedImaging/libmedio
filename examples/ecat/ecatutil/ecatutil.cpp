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

 $Id: ecatcmp.cpp 317 2006-03-17 17:51:06Z langner $

***************************************************************************/

#include <CECATFile>
#include <CECAT6MainHeader>
#include <CECAT7MainHeader>
#include <CECATDirectory>
#include <CECATDirectoryItem>
#include <CECAT7SubHeaderScan3D>

#include <QFileInfo>
#include <QMultiHash>
#include <QString>
#include <QStringList>
#include <QDateTime>

#include <iostream>
#include <iomanip>

#define INFEED_3D 116.40
#define INFEED_2D 135.80

using namespace std;

// global data
QString inputFileName;
QString outputFileName;
QStringList appendFilesList;
float g_fInfeed = INFEED_3D;
quint32 replaceMatrixID=0;
quint32 newMatrixID=0;
int g_iFrameStartTime = 0;
QDateTime g_scanStartTime;
QDateTime g_doseStartTime;
double g_dBedElevation;

//  Function:    main
//! 
//! This is a small and simple example utility to modify an ECAT file and process
//! some operations on it (e.g. remove or add a frame/matrix)
//! 
//! @param       argc number of commandline options
//! @param       *argv[] pointer to a array of the commandline options
//! @return      integer of the exit code.
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	int returnCode = 0; // return no error on default

  // You want this, unless you mix couts output with C output.
  // Read  http://gcc.gnu.org/onlinedocs/libstdc++/27_io/howto.html#8 for an explanation.
  ios::sync_with_stdio(false);

	// put all arguments in a temporary MultiHash
	QMultiHash<QString, QString> args;

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
				i++;
			}
			else
				args.insert(option, "");
		}
		else
		{
			args.insert("infile", argv[i]);
		}
	}
		
	// check if the user has specified at least 2 arguments which
	// is the bare minimum
	if(args.isEmpty() == false &&
		 args.contains("-h") == false)
	{
		// if the user wants to remove a specific
		// matrix from the input ECAT file, we do it now
		if(args.contains("-r"))
		{
			QString replaceString = args.value("-r");

			if(replaceString.isEmpty())
			{
				cout << "ERROR: empty replace string option found." << endl;
				returnCode = 2;
			}
			else
			{
				// now we generate the replace matrixID
				QStringList list = replaceString.split(',');
				if(list.isEmpty() == false)
				{
					short frame=1;
					short plane=1;
					short gate=1;
					short bed=0;
					short data=0;

					QStringListIterator it(list);
					int i;
					for(i=0; it.hasNext() && i <= 4; i++)
					{
						bool success = false;
						short num = it.next().toInt(&success);

						if(success == false)
						{
							i=-1;
							break;
						}

						if(i==0)
							frame = num;
						else if(i==1)
							plane = num;
						else if(i==2)
							gate = num;
						else if(i==3)
							bed = num;
						else if(i==4)
							data = num;
					}

					if(i != -1)
					{
						replaceMatrixID = convertToMatrixID(frame, plane, gate, bed, data);
					}
					else
					{
						cout << "ERROR: on converting supplied replace matrix ID: '" << replaceString.toAscii().constData() << "'" << endl;
						returnCode = 2;
					}
				}
				else
				{
					cout << "ERROR: invalid replace matrix definition found." << endl;
					returnCode = 2;
				}
			}
		}

		if(args.contains("-n"))
		{
			QString newString = args.value("-n");

			if(newString.isEmpty())
			{
				cout << "ERROR: empty new string option found." << endl;
				returnCode = 2;
			}
			else
			{
				// now we generate the replace matrixID
				QStringList list = newString.split(',');
				if(list.isEmpty() == false)
				{
					short frame=1;
					short plane=1;
					short gate=1;
					short bed=0;
					short data=0;

					QStringListIterator it(list);
					int i;
					for(i=0; it.hasNext() && i <= 4; i++)
					{
						bool success = false;
						short num = it.next().toInt(&success);

						if(success == false)
						{
							i=-1;
							break;
						}

						if(i==0)
							frame = num;
						else if(i==1)
							plane = num;
						else if(i==2)
							gate = num;
						else if(i==3)
							bed = num;
						else if(i==4)
							data = num;
					}

					if(i != -1)
					{
						newMatrixID = convertToMatrixID(frame, plane, gate, bed, data);
					}
					else
					{
						cout << "ERROR: on converting supplied new matrix ID: '" << newString.toAscii().constData() << "'" << endl;
						returnCode = 2;
					}			
				}
			}
		}
		if(args.contains("-f"))
		{
			g_fInfeed = args.value("-f").toFloat();
		}
		if(args.contains("-s"))
		{
			g_iFrameStartTime = args.value("-s").toInt();
		}
		if(args.contains("-d"))
		{
			g_doseStartTime = QDateTime::fromString(args.value("-d"), QString("dd.MM.yyyy hh:mm:ss"));
		}
		if(args.contains("-c"))
		{
			g_scanStartTime = QDateTime::fromString(args.value("-c"), QString("dd.MM.yyyy hh:mm:ss"));
		}
		if(args.contains("-e"))
		{
			g_dBedElevation = args.value("-e").toDouble();
		}
		if(args.contains("-t"))
		{
			args.insert("infile", args.value("-t"));
		}
		if(args.contains("-a"))
		{
			appendFilesList = args.value("-a").split(',', QString::SkipEmptyParts);
			QStringListIterator i(appendFilesList);
			if(appendFilesList.isEmpty())
			{
				cout << "ERROR: no files found to append." << endl;
				returnCode = 2;
			}
			else
			{
				inputFileName = appendFilesList[0];
				QRegExp rx("([a-z,A-Z,-]+)");
				int pos = 0;
				if((pos = rx.indexIn(QFileInfo(appendFilesList[0]).baseName(), pos)) != -1)
				{
					outputFileName = rx.cap(1) + QString(".v");
					cout << outputFileName.toAscii().data() << endl;
				}
				else
				{
					cout << "ERROR: could not generate outputfile." << endl;
					returnCode = 2;
				}
			}
		}
		if(args.contains("infile") && !args.contains("-a"))
		{
			inputFileName = args.value("infile");

			if(QFileInfo(inputFileName).isFile() == false && QFileInfo(inputFileName).exists() == false)
			{
				cout << "ERROR: specified input filename isn't a file or doesn't exist." << endl;
				returnCode = 2;
			}
		}
		else if(!args.contains("-a"))
		{
			cout << "ERROR: no input file specified." << endl;
			returnCode = 2;
		}

		if(args.contains("-o") && args.value("-o").isEmpty() == false)
			outputFileName = args.value("-o");
		else if(!args.contains("-a"))
			outputFileName = inputFileName+"_modified";
	}
	else
		returnCode = 2;

	if(returnCode > 0)
	{
		cout << endl;
		cout << "libmedio ECAT6/7 file utility v1.0" << endl;
		cout << "----------------------------------" << endl;
		cout << "Usage: " << argv[0] << " <options> file" << endl;
		cout << "Options:" << endl;
		cout << "  -o <file>    : write the output data to <file> instead to '*_new'" << endl;
		cout << "  -r <matrixID>: remove matrix from ECAT file with ID <frame,plane,gate,data,bed>." << endl;
		cout << "  -n <matrixID>: create an empty matrix." << endl;
		cout << "  -a <file1,file2,...>: append first matrix from each file to new file in specified order." << endl;
		cout << "  -f <infeed>  : defines infeed in mm for each bedposition (default: 3D - 116.40mm)" << endl;
		cout << "  -s <starttime>: sets frame start time [msecs] in given file" << endl;
		cout << "  -d <startdatetime>: sets dose start date and time [DD.MM.YYYY HH:MM:SS]" << endl;
		cout << "  -c <startdatetime>: sets scan start date and time [DD.MM.YYYY HH:MM:SS]" << endl;
		cout << "  -e <elevation>: sets bed bed elevation [cm]" << endl;
		cout << "  -t           : get type of ecat file" << endl;
		cout << "  -b <file>    : rectify header values of smoother processed infile with values from file." << endl;

		cout << "  -h           : this help page." << endl << endl;
	}
	else
	{
		// open the first file
		CECATFile infile(inputFileName);
		if(infile.open(QIODevice::ReadWrite) && 
			 infile.format() != CECATFile::Undefined)
		{
			if(!args.contains("-s") && !args.contains("-d") && !args.contains("-c") && !args.contains("-e") && !args.contains("-t") && !args.contains("-b"))
			{
				cout << "Successfully loaded file: '" << inputFileName.toAscii().constData() << "'" << endl;
				// lets open the output file now
				CECATFile outfile(outputFileName, infile.fileType());
				if(outfile.open(QIODevice::WriteOnly))
				{
					cout << "Successfully opened file: '" << outputFileName.toAscii().constData() << "'" << endl;

					// now we first copy the main headers
					CECATMainHeader* mainHeader;
					if(infile.readMainHeader(mainHeader))
					{
						cout << "processing main header: ";
						CECATMainHeader* outMainHeader = outfile.createEmptyMainHeader();
						if(outMainHeader)
						{
							*outMainHeader = *mainHeader;
							CECAT7MainHeader* pTmp = static_cast<CECAT7MainHeader*>(outMainHeader);
							for(int i = 0; i < appendFilesList.count()-1; i++)
								pTmp->setBed_Offset(i, (i+1)*g_fInfeed/10.0F);

							if(outfile.writeMainHeader(*outMainHeader))
							{
								cout << "copied." << endl;

								if(!args.contains("-a"))
								{
									// now we iterate through the directory list of the input
									// file and write out each matrix to the output file
									CECATDirectory* inDir = infile.directory();
									CECATDirectoryItem* inDitem;

									// on a first operation we first go and add new matrices
									if(newMatrixID != 0 && (inDitem = (*inDir)[0]))
									{
										CECATSubHeader* dummySubHeader;
										QByteArray* dummyData;

										// we read in the very first dir item to clone it with
										// a new matrixID
										cout << "processing matrix (" << std::hex << newMatrixID << "): ";
										if(inDitem->readMatrix(dummyData, dummySubHeader))
										{
											// now we clear the data and write it back under the new matrixID
											dummyData->fill(0);

											// write the data to the new matrix
											if(outfile.writeMatrix(*dummyData, *dummySubHeader, matrixID2Frame(newMatrixID),
																																					matrixID2Plane(newMatrixID),
																																					matrixID2Gate(newMatrixID),
																																					matrixID2Bed(newMatrixID),
																																					matrixID2Data(newMatrixID)))
											{
												cout << "new empty matrix generated." << endl;
											}
											else
												cout << "ERROR! couldn't write to output file." << endl;

											delete dummyData;
											delete dummySubHeader;
										}
										else
											cout << "ERROR! couldn't read in first diritem" << endl;
									}							

									for(int i=0; (inDitem = (*inDir)[i]); i++)
									{
										quint32 matrixID = inDitem->matrixID();
										CECATSubHeader* subHeader;
										QByteArray* matrixData;

										cout << "processing matrix (" << std::hex << matrixID << "): ";
										if(matrixID != newMatrixID && matrixID != replaceMatrixID)
										{
											// read in the matrix data and subheader
											if(inDitem->readMatrix(matrixData, subHeader))
											{
												// now we generate a new matrix with the same ID in our
												// output file
												if(outfile.writeMatrix(*matrixData, *subHeader, matrixID2Frame(matrixID)))
												{
													cout << "copied." << endl;
												}
												else
													cout << "ERROR! couldn't write to output file." << endl;

												delete matrixData;
												delete subHeader;
											}
											else
												cout << "ERROR! couldn't read matrix data." << endl;
										}
										else
											cout << "skipped." << endl;
									}
								}
								else
								{
									cout << g_fInfeed << endl;
									QStringListIterator it(appendFilesList);
									int i = 1;
									while(it.hasNext())
									{
										QString appendFile = it.next();
										cout << "processing: " << appendFile.toAscii().data() << endl;
										CECATFile file(appendFile);
										if(file.open(QIODevice::ReadOnly) && infile.format() != CECATFile::Undefined)
										{
											QByteArray* pMatrixData = NULL;
											CECATSubHeader* pSubHeader = NULL;
											if(!file.readMatrix(pMatrixData, pSubHeader, 1))
											{
												cout << "ERROR! couldn't read matrix from: " << appendFile.toAscii().data() << endl;
												file.close();
												break;
											}
											if(outfile.writeMatrix(*pMatrixData, *pSubHeader, 1,1,1,i-1,0))
											{
												cout << "matrix copied." << endl;
											}
											else
											{
												cout << "ERROR! couldn't write matrix to: " << outputFileName.toAscii().data() << endl;
												file.close();
												break;
											}
											file.close();
										}
										else
										{
											cout << "ERROR! couldn't open" << appendFile.toAscii().data() << endl;
											break;
										}
										i++;
									}
								}
							}
							else
								cout << "ERROR! couldn't write main header." << endl;

							delete outMainHeader;
						}

						delete mainHeader;
					}
					else
						cout << "ERROR! couldn't read main header." << endl;

					outfile.close();
				}
				else
					cout << "ERROR: couldn't open file '" << outputFileName.toAscii().constData() << "'" << endl;
			}
			else
			{
				if(args.contains("-d") || args.contains("-c") || args.contains("-e") || args.contains("-t"))
				{
					CECATMainHeader* mainHeader;
					if(infile.readMainHeader(mainHeader))
					{
						CECAT7MainHeader* pTmp = static_cast<CECAT7MainHeader*>(mainHeader);
						if(args.contains("-d"))
							pTmp->setDose_Start_Time_Qt(g_doseStartTime);
						if(args.contains("-c"))
							pTmp->setScan_Start_Time_Qt(g_scanStartTime);
						if(args.contains("-e"))
							pTmp->setBed_Elevation(g_dBedElevation);
						if(args.contains("-t"))
						{
							cout << pTmp->file_Type() << endl;
						}
						else
						{
							if(infile.writeMainHeader(*pTmp))
								cout << "Successfully updated main header '" << inputFileName.toAscii().constData() << "'" << endl;
							else
								cout << "ERROR: couldn't write main header '" << inputFileName.toAscii().constData() << "'" << endl;
						}
					}
					else
						cout << "ERROR: couldn't read main header '" << inputFileName.toAscii().constData() << "'" << endl;
				}

				if(args.contains("-b"))
				{
					CECATMainHeader* mainHeader;
					if(infile.readMainHeader(mainHeader))
					{
						CECAT7MainHeader* pTmp = static_cast<CECAT7MainHeader*>(mainHeader);
						CECATFile reffile(args.value("-b"));
						if(reffile.open(QIODevice::ReadOnly) && 
							reffile.format() != CECATFile::Undefined)
						{
							CECATMainHeader* refMainHeader;
							if(reffile.readMainHeader(refMainHeader))
							{
								CECAT7MainHeader* pTmp_Ref = static_cast<CECAT7MainHeader*>(refMainHeader);
								pTmp->setOriginal_File_Name(pTmp_Ref->original_File_Name());
								pTmp->setAngular_Compression(pTmp_Ref->angular_Compression());
								pTmp->setCalibration_Units_Label(pTmp_Ref->calibration_Units_Label());
								pTmp->setData_Units(pTmp_Ref->data_Units());
								if(infile.writeMainHeader(*pTmp))
									cout << "Successfully updated main header '" << inputFileName.toAscii().constData() << "'" << endl;
								else
									cout << "ERROR: couldn't write main header '" << inputFileName.toAscii().constData() << "'" << endl;
								reffile.close();
							}
							else 
							cout << "ERROR: couldn't read main header '" << args.value("-b").toAscii().constData() << "'" << endl;
						}
						else
							cout << "ERROR: couldn't read reference file '" << args.value("-b").toAscii().constData() << "'" << endl;
					}
					else
						cout << "ERROR: couldn't read main header '" << inputFileName.toAscii().constData() << "'" << endl;
				}

				if(args.contains("-s"))
				{
					CECATSubHeader* pSubHeader = NULL;
					if(infile.readSubHeader(pSubHeader, 1))
					{
						if(pSubHeader->subHeaderType() == CECATSubHeader::ECAT7_Scan3D)
						{
							CECAT7SubHeaderScan3D* pTmp = static_cast<CECAT7SubHeaderScan3D*>(pSubHeader);
							pTmp->setFrame_Start_Time(g_iFrameStartTime);
							if(infile.writeSubHeader(*pTmp, 1,1,1,0,0))
							{
								cout << "Successfully updated frame start time '" << inputFileName.toAscii().constData() << "'" << endl;
							}
							else
							{
								cout << "ERROR: couldn't write subheader '" << inputFileName.toAscii().constData() << "'" << endl;
							}
						}
						else
						{
							cout << "ERROR: no scan 3d subheader '" << inputFileName.toAscii().constData() << "'" << endl;
						}
					}
					else
					{
						cout << "ERROR: couldn't read subheader '" << inputFileName.toAscii().constData() << "'" << endl;
					}
				}
			}
			infile.close();
		}
		else
			cout << "ERROR: couldn't open file '" << inputFileName.toAscii().constData() << "'" << endl;
	}

	return returnCode;
}
