/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004-2006 by Jens Langner <Jens.Langner@light-speed.de>

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

 $Id: ecatcmp.cpp 227 2005-12-01 16:26:45Z langner $

***************************************************************************/

#include <CECATFile>
#include <CECAT6MainHeader>
#include <CECAT7MainHeader>
#include <CECATDirectory>

#include <QFileInfo>
#include <QHash>
#include <QString>

#include <iostream>
#include <iomanip>

using namespace std;

// global data
QString inputFileName;
QString outputFileName;
QString replaceString;
bool forceOperation = false;
bool anonymizeAll = false;
bool anonymizeMore = false;

//  Function:    main
//! 
//! This is an ECAT anonymizer tool which strips off all patient relevant data
//! and may replace it against predefined strings
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

	// provide some variables to check the commandline options
	// properly.
	QHash<QString, QString> args;

	// if the user has specified some commandline options
	// lets process and parse them.
	for(int i=1; i < argc; i++)
	{
		QString option(argv[i]);

		if(option[0] == '-')
		{
			if(i+1 < argc && argv[i+1][0] != '-')
			{
				args[option] = argv[i+1];
			}
			else
				args[option] = "";
		}
		else if(i+1 == argc)
			args["infile"] = argv[i];
	}
	
	// now we check/process the different options according to their
	// priority
	if(args.isEmpty() == false && args.contains("-h") == false) // help page requested
	{
		// user wants to place the output in a separate output file
		if(args.contains("-o"))
		{
			outputFileName = args.value("-o");	

			// try to load the configData from the predefined config file
			if(outputFileName.isEmpty())
			{
				cout << "ERROR: The specified output file is invalid or already exists." << endl;
				returnCode = 2;
			}
		}

		// the user wants to replace all data with the specified string
		if(args.contains("-r"))
		{
			replaceString = args.value("-r");

			if(replaceString.isEmpty())
			{
				cout << "ERROR: empty replace string option found." << endl;
				returnCode = 2;
			}
		}

		// the user wants to force any operation that may be harmfull at the
		// first place
		if(args.contains("-f"))
			forceOperation = true;

		// the user wants to strip as much as possible
		if(args.contains("-a"))
			anonymizeAll = true;

		// the user wants to strip as much as possible
		if(args.contains("-m"))
			anonymizeMore = true;

		// check that the user really specified an input filename
		if(args.contains("infile"))
		{
			inputFileName = args.value("infile");

			if(QFileInfo(inputFileName).isFile() == false && QFileInfo(inputFileName).exists() == false)
			{
				cout << "ERROR: specified input filename isn't a file or doesn't exist." << endl;
				returnCode = 2;
			}
		}
		else
		{
			cout << "ERROR: no input file specified." << endl;
			returnCode = 2;
		}
	}
	else
		returnCode = 2;

	// now we check if the user wants to overwrite the input file by a force operation
	// or if he wants us to anonymize into a copy
	if(returnCode == 0)
	{
		if(outputFileName.isEmpty())
		{
			if(forceOperation)
				outputFileName = inputFileName;
			else
			{
				cout << "ERROR: no outputfile specified. won't overwrite input file." << endl;
				returnCode = 2;
			}
		}
		else 
		{
			if(QFileInfo(outputFileName).exists())
			{
				if(forceOperation)
				{
					if(QFile::remove(outputFileName) == false)
					{
						cout << "ERROR: couldn't remove the already existing output file prior to the anonymize operation." << endl;
						returnCode = 2;
					}
				}
				else
				{
					cout << "ERROR: output file already exists." << endl;
					returnCode = 2;
				}
			}
					
			// we have to generate a copy of the input file first
			if(returnCode == 0)
			{
				if(QFile::copy(inputFileName, outputFileName) == false)
				{
					cout << "ERROR: couldn't copy the input file to specified output filename." << endl;
					returnCode = 2;
				}
			}
		}
	}

	if(returnCode > 0)
	{
		cout << endl
		     << "libmedio ECAT6/7 file anonymizer v1.2" << endl
		     << "-------------------------------------" << endl
		     << "Usage: " << argv[0] << " <options> file" << endl
		     << "Options:" << endl
		     << "  -o <file>    : write the anonymized file to <file>" << endl
		     << "  -r <string>  : replace all stripped data with string <string>" << endl
         << "  -m           : strip more patient data:" << endl
         << "                   PATIENT_ID" << endl
         << "                   PATIENT_SEX" << endl
         << "                   PATIENT_DEXTERITY" << endl
         << "                   PATIENT_HEIGHT" << endl
         << "                   PATIENT_WEIGHT" << endl << endl
		     << "  -a           : anonymize as much as possible including scan data:" << endl
         << "                   SCAN_START_TIME" << endl
         << "                   DOSE_START_TIME" << endl
         << "                   DOSAGE" << endl << endl
		     << "  -f           : force overwrite operation" << endl
		     << "  -h           : this help page" << endl << endl
         << "Default stripped main header data:" << endl
         << "  ORIGINAL_FILE_NAME " << endl
         << "  STUDY_TYPE" << endl
         << "  PATIENT_NAME" << endl
         << "  PATIENT_AGE" << endl
         << "  PATIENT_BIRTH_DATE" << endl
         << "  PHYSICIAN_NAME" << endl
         << "  OPERATOR_NAME" << endl
         << "  STUDY_DESCRIPTION" << endl
         << "  FACILITY_NAME" << endl << endl;
	}
	else
	{
		// now we have successfully collected all data, so we can go and anonymize the main
		// header of or file.

		// open the first file
		CECATFile file(outputFileName);
		if(file.open(QIODevice::ReadWrite) && 
			 file.format() != CECATFile::Undefined)
		{
			if(file.format() == CECATFile::ECAT7)
			{
				// now we read the main header
				CECATMainHeader* mainHeader;
        if(file.readMainHeader(mainHeader))
				{
					CECAT7MainHeader* mHeader = static_cast<CECAT7MainHeader*>(mainHeader);

					// we first make sure we anonymize all must have items
					mHeader->setOriginal_File_Name(replaceString.toAscii());
					mHeader->setStudy_Type(replaceString.toAscii());
					mHeader->setPatient_Name(replaceString.toAscii());
					mHeader->setPatient_Age(0);
					mHeader->setPatient_Birth_Date(0);
					mHeader->setPhysician_Name(replaceString.toAscii());
					mHeader->setOperator_Name(replaceString.toAscii());
					mHeader->setStudy_Description(replaceString.toAscii());
					mHeader->setFacility_Name(replaceString.toAscii());

          // anonymize more data
          if(anonymizeMore == true || anonymizeAll == true)
          {
            mHeader->setPatient_ID(replaceString.toAscii());
						mHeader->setPatient_Sex(CECAT7MainHeader::Sex_Unknown);
						mHeader->setPatient_Dexterity(CECAT7MainHeader::Dext_Unknown);
						mHeader->setPatient_Height(0);
						mHeader->setPatient_Weight(0);						

					  // and if the option "-a" is also given we also
					  // strip of as much study/patient relevant data as possible
					  if(anonymizeAll == true)
					  {
						  mHeader->setScan_Start_Time(0);
						  mHeader->setDose_Start_Time(0);
						  mHeader->setDosage(0);
					  }
          }

					// now we write back the main Header for finally clearing it
					if(mHeader->save() == false)
					{
						cout << "ERROR: error while trying to write back anonymized header." << endl;
						returnCode = 2;
					}
					
					delete mHeader;
				}
				else
				{
					cout << "ERROR: main header read operation failed." << endl;
					returnCode = 2;
				}
			}
			else
			{
				cout << "ERROR: file format of input file is not supported." << endl;
				returnCode = 2;
			}

			file.close();
		}
	}	

	return returnCode;
}
