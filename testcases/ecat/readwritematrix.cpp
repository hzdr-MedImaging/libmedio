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

#include <CECATFile.h>
#include <CECAT6MainHeader.h>
#include <CECAT7MainHeader.h>
#include <CECATSubHeader.h>
#include <CECATDirectory.h>
#include <CECAT7SubHeaderImage.h>

#include <iostream>

#define ADDITIONAL_ITEMS	35

using namespace std;

//  Function:    main
//! 
//! This is a small test program that writes a predefined matrix data to a file and
//! read it out again for verifying the read and write functionality of libmedio
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

	cout << "libmedio ECAT6/7 file read/write test" << endl;
	cout << "-------------------------------------" << endl;

	// generate some huge matrix data which we can use for verification later
	//#define MATRIX_SIZE	(2064414/sizeof(Q_UINT16)) // non dividable through ECAT_BLOCKSIZE
	//#define MATRIX_SIZE	(2063872/sizeof(Q_UINT16))
	#define MATRIX_SIZE	(2064384/sizeof(Q_UINT16))
	Q_UINT16* matrixData_frame1 = new Q_UINT16[MATRIX_SIZE];
	Q_UINT16* matrixData_frame2 = new Q_UINT16[MATRIX_SIZE];

	// fill it with random data
	srand(time(NULL));
	for(unsigned int i=0; i < MATRIX_SIZE; i++)
	{
		matrixData_frame1[i] = 1;
		matrixData_frame2[i] = 2;
	}

	// open the file for writing data
	CECATFile file("readwritematrix.v", CECATMainHeader::ECAT7_Volume16);
	if(file.open(IO_WriteOnly))
	{
		// first we write an own main header into the file
		CECATMainHeader* mainHeader = file.createEmptyMainHeader();
		if(mainHeader->rtti() == CECATMainHeader::ECAT7MainHeader)
		{
			CECAT7MainHeader* e7mainHeader = static_cast<CECAT7MainHeader*>(mainHeader);

			e7mainHeader->setOriginal_File_Name("1. MainHeader write operation");
			e7mainHeader->save();
		}		
		delete mainHeader; // delete the temporary main Header

		// let us write out the data to the file in frame 1
		if(file.writeMatrix((char*)matrixData_frame2, 
												MATRIX_SIZE*sizeof(Q_UINT16), 
												CECATSubHeader::SunShort, 2) == false)
			cout << "Error during writeMatrix() operation for frame 2" << endl;
		else
		{
			cout << "successfully written matrix data for frame 2" << endl;

			if(file.writeMatrix((char*)matrixData_frame1,
													MATRIX_SIZE*sizeof(Q_UINT16),
													CECATSubHeader::SunShort, 1) == false)
			{
				cout << "Error during writeMatrix() operation for frame 1" << endl;
			}
			else
				cout << "successfully written matrix data for frame 1" << endl;

			// write the subheaders of frame 1 & 2 separatly
			CECATSubHeader* subHeader;
			if(file.readSubHeader(subHeader, 1) &&
				 subHeader->subHeaderType() == CECATSubHeader::ECAT7_Image)
			{
				CECAT7SubHeaderImage* imageHeader = static_cast<CECAT7SubHeaderImage*>(subHeader);

				imageHeader->setNum_Dimensions(3);
				imageHeader->setX_Dimension(128);
				imageHeader->setY_Dimension(128);
				imageHeader->setZ_Dimension(63);
				imageHeader->setScale_Factor(1);
				imageHeader->setX_Pixel_Size(0.205941);
				imageHeader->setY_Pixel_Size(0.205941);
				imageHeader->setZ_Pixel_Size(0.2425);
				imageHeader->save();
				
				file.writeSubHeader(*imageHeader, 2);
			}
			else
				cout << "ERROR: couldn't read subHeader from file." << endl;

			// now we create an empty subheader from another temporary file so that we can
			// test of the subheader copy routines work as expected
			CECATFile file2("readwritematrix2.v", CECATMainHeader::ECAT7_Volume16);
			if(file2.open(IO_WriteOnly))
			{
				CECATSubHeader* newSubHeader = file2.createEmptySubHeader();
				if(newSubHeader->subHeaderType() == CECATSubHeader::ECAT7_Image)
				{
					CECAT7SubHeaderImage* imageHeader = static_cast<CECAT7SubHeaderImage*>(newSubHeader);

					imageHeader->setNum_Dimensions(3);
					imageHeader->setX_Dimension(256);
					imageHeader->setY_Dimension(256);
					imageHeader->setZ_Dimension(63);
					imageHeader->setScale_Factor(1);
					imageHeader->setX_Pixel_Size(0.205941);
					imageHeader->setY_Pixel_Size(0.205941);
					imageHeader->setZ_Pixel_Size(0.2425);
					imageHeader->save();

					// use the assignment operator for copying all data from one header to another
					*static_cast<CECAT7SubHeaderImage*>(subHeader) = *imageHeader;

					// we make sure we set the data type correctly before we write out the matrix later
					// one
					subHeader->setData_Type(CECATSubHeader::SunShort);
				}
				else
					cout << "ERROR: created subHeader of file2 isn't a Image header!" << endl;

				delete newSubHeader;
				file2.close();
			}
			else
				cout << "ERROR: newSubHeader has wrong type!" << endl;;

			cout << "writing out matrixData to frame 3..." << endl;

			// and to test the write operations to write the matrix together
			// with the subheader lets do it now and write frame 3
			if(file.writeMatrix((char*)matrixData_frame2, MATRIX_SIZE*sizeof(Q_UINT16),
													*subHeader, 3) == false)
			{
				cout << "Error during writeMatrix(data, subheader) operation for frame 3" << endl;
			}
			else
				cout << "successfully written matrix&subheader to frame 3" << endl;

			CECATMainHeader* mainHeader = NULL;
			if(file.readMainHeader(mainHeader) &&
				 mainHeader->rtti() == CECATMainHeader::ECAT7MainHeader)
			{
				CECAT7MainHeader* e7mainHeader = static_cast<CECAT7MainHeader*>(mainHeader);

				e7mainHeader->setStudy_Description("2. mainheader write operation");
				e7mainHeader->setNum_Planes(63);
				e7mainHeader->setCalibration_Factor(1);
				e7mainHeader->save();
			}
			delete mainHeader;

			// for testing that the directory list stuff works above > 31 frames we write out
			// 30 more subheaders to the file
			CECAT7SubHeaderImage* imageHeader = static_cast<CECAT7SubHeaderImage*>(subHeader);
			cout << "writting " << ADDITIONAL_ITEMS << " additional frames for directory list testing";
			for(int i=0; i < ADDITIONAL_ITEMS; i++)
			{
				cout << ".";
				imageHeader->setImage_Min(i);

				//file.writeSubHeader(*imageHeader, i+3);
				file.writeMatrix((char*)matrixData_frame2, MATRIX_SIZE*sizeof(Q_UINT16), *imageHeader, i+3);
			}
			cout << endl;

			delete subHeader;
		}

		// close the file again
		file.close();

		// reopen it as readonly
		if(file.open(IO_ReadOnly))
		{
			// read in the matrix in a separate buffer
			Q_UINT16* readBuf = NULL;
			unsigned int len = 0;

			if(file.readMatrix((char*&)readBuf, len, 1) == false)
				cout << "Error during readMatrix() operation" << endl;
			else
			{
				cout << "successfully read matrix data from frame 1" << endl;
				
				// let us compare the data
				cout << "read " << len << " bytes of data" << endl;
				cout << "comparing read data with written data..." << endl;
				
				unsigned int i=0;
				for(; i < MATRIX_SIZE; i++)
				{
					if(matrixData_frame1[i] != readBuf[i])
						break;
				}

				if(i < MATRIX_SIZE)
				{
					cout << "read MatrixData != written MatrixData at position " << i << endl;
					cout << "read: " << hex << uppercase << readBuf[i] << "- written: " << matrixData_frame1[i] << endl;
				}
				else
					cout << "successfully compared read with written matrixData!" << endl;

				// free the read matrix data
				delete[] readBuf;
			}

			if(file.readMatrix((char*&)readBuf, len, 2) == false)
				cout << "Error during readMatrix() operation" << endl;
			else
			{
				cout << "successfully read matrix data from frame 2" << endl;
				
				// let us compare the data
				cout << "read " << len << " bytes of data" << endl;
				cout << "comparing read data with written data..." << endl;
				
				unsigned int i=0;
				for(; i < MATRIX_SIZE; i++)
				{
					if(matrixData_frame2[i] != readBuf[i])
						break;
				}

				if(i < MATRIX_SIZE)
				{
					cout << "read MatrixData != written MatrixData at position " << i << endl;
					cout << "read: " << hex << uppercase << readBuf[i] << "- written: " << matrixData_frame2[i] << endl;
				}
				else
					cout << "successfully compared read with written matrixData!" << endl;

				// free the read matrix data
				delete[] readBuf;				
			}

			// for testing that the directory list stuff works above > 31 frames we read out
			// the additional subheaders and matrix data and compare it against the written ones
			cout << "reading the data of the " << ADDITIONAL_ITEMS << " frames";
			for(int i=0; i < ADDITIONAL_ITEMS; i++)
			{
				cout << ".";
				CECATSubHeader* subHeader = NULL;
					
				if(file.readSubHeader(subHeader, i+3) == false || 
					 subHeader == NULL || static_cast<CECAT7SubHeaderImage*>(subHeader)->image_Min() != i)
				{
					cout << "ERROR: in reading image_Min() of frame #" << i+3;
					delete subHeader;
					break;
				}

				if(file.readMatrix((char*&)readBuf, len, i+3) == false)
				{
					cout << "ERROR: in reading matrix data of frame #" << i+3;
					delete subHeader;
					break;
				}

				unsigned int j=0;
				for(; j < MATRIX_SIZE; j++)
				{
					if(matrixData_frame2[j] != readBuf[j])
						break;
				}

				if(j < MATRIX_SIZE)
				{
					cout << "ERROR: read MatrixData != written MatrixData at position " << j << " (" << FilePos2ECATBlock(j)<< ") in frame #" << i+3 << endl;
					cout << "ERROR: read: " << hex << uppercase << readBuf[j] << " - written: " << matrixData_frame2[j];
					delete[] readBuf;
					delete subHeader;
					break;
				}

				// free the read matrix data
				delete[] readBuf;	
				delete subHeader;
			}
			cout << endl;
		
			// close the file
			file.close();
		}
		else
			cout << "Error on opening the file readonly" << endl;

		delete[] matrixData_frame1;
		delete[] matrixData_frame2;
	}
	else
		cout << "Error on opening the file writeonly" << endl;

	return returnCode;
}
