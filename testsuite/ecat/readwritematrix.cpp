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

 $Id$

***************************************************************************/

#include <CECATFile.h>
#include <CECAT6MainHeader.h>
#include <CECAT7MainHeader.h>
#include <CECATSubHeader.h>
#include <CECATDirectory.h>
#include <CECAT7SubHeaderImage.h>

#include <rtdebug.h>

#include <iostream>

// defines for generating the ECAT7 files
#define NUM_DIMENSIONS 3
#define X_DIMENSION 128
#define Y_DIMENSION 128
#define Z_DIMENSION 63
#define X_PIXELSIZE 0.205941f
#define Y_PIXELSIZE 0.205941f
#define Z_PIXELSIZE 0.2425f
#define SCALE_FACTOR 1.0f

// defines for managing memory
#define MATRIX_DIMENSION	(X_DIMENSION*Y_DIMENSION*Z_DIMENSION)
#define MATRIX_SIZE				(MATRIX_DIMENSION*sizeof(quint16))
//#define ADDITIONAL_FRAMES	33
#define ADDITIONAL_FRAMES	126

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
	quint16* matrixData_frame1 = new quint16[MATRIX_DIMENSION];
	quint16* matrixData_frame2 = new quint16[MATRIX_DIMENSION];

	cout << "generated helper matrices with size '" << MATRIX_SIZE << " bytes'." << endl;

	// fill it with random data
	srand(time(NULL));
	for(unsigned int i=0; i < MATRIX_DIMENSION; i++)
	{
		matrixData_frame1[i] = 1;
		matrixData_frame2[i] = 2;
	}
	
	/////////////////////////////////////////////////////////////////////
	//
	// We generate a 'readwritematrix.v' file here with initially 2 
	// matrixes and nearly the same subheader
	//
	CECATFile file("readwritematrix.v", CECATMainHeader::ECAT7_Volume16);
	if(file.open(QIODevice::WriteOnly))
	{
		// first we write an own main header into the file
		CECATMainHeader* mainHeader = file.createEmptyMainHeader();
		if(mainHeader->rtti() == CECATMainHeader::ECAT7MainHeader)
		{
			CECAT7MainHeader* e7mainHeader = static_cast<CECAT7MainHeader*>(mainHeader);

			e7mainHeader->setOriginal_File_Name("1. MainHeader write operation");
			if(e7mainHeader->save() == false)
			{
				cout << "ERROR on saving main header" << endl;
				exit(2);
			}
		}		
		delete mainHeader; // delete the temporary main Header

		// if we are at writing mainHeader data we go and test our
		// Qt-based time conversion routines
		CECAT7MainHeader* e7mainHeader = static_cast<CECAT7MainHeader*>(file.createEmptyMainHeader());
		QDateTime dt(QDate(1970, 1, 1), QTime());
		QDateTime dtUTC(QDate(1970, 1, 1), QTime(), Qt::UTC);

		int utcDiff = dtUTC.secsTo(dt);

		// test == 1.1.1970
		e7mainHeader->setScan_Start_Time_Qt(dt);
		if(e7mainHeader->scan_Start_Time() != 0+utcDiff || e7mainHeader->scan_Start_Time_Qt() != dt)
		{
			cout << "Error on testing datetime conversion routines." << endl;
			exit(2);
		}

		// test < 1970 (31.01.1924)
		dt.setDate(QDate(1924, 1, 31));
		e7mainHeader->setScan_Start_Time_Qt(dt);
		if(e7mainHeader->scan_Start_Time() != -1449100800+utcDiff || e7mainHeader->scan_Start_Time_Qt() != dt)
		{
			cout << "Error on testing datetime conversion routines." << endl;
			exit(2);
		}

		// test > 1975 (6.1.1975)
		dt.setDate(QDate(1975, 1, 6));
		e7mainHeader->setScan_Start_Time_Qt(dt);
		if(e7mainHeader->scan_Start_Time() != 158198400+utcDiff || e7mainHeader->scan_Start_Time_Qt() != dt)
		{
			cout << "Error on testing datetime conversion routines." << endl;
			exit(2);
		}

		// test > 1975 (3.7.2015)
		dt.setDate(QDate(2015, 7, 3));
		e7mainHeader->setScan_Start_Time_Qt(dt);
		if(e7mainHeader->scan_Start_Time() != 1435878000+utcDiff || e7mainHeader->scan_Start_Time_Qt() != dt)
		{
			cout << "Error on testing datetime conversion routines." << endl;
			exit(2);
		}

		// now some tests with additional times included
		
		// test < 1975 (7.2.1955 10:39:59)
		dt.setDate(QDate(1955, 2, 7));
		dt.setTime(QTime(10, 39, 59));
		e7mainHeader->setScan_Start_Time_Qt(dt);
		if(e7mainHeader->scan_Start_Time() != -470150401+utcDiff || e7mainHeader->scan_Start_Time_Qt() != dt)
		{
			cout << "Error on testing datetime conversion routines." << endl;
			exit(2);
		}
		
		// test > 1975 (11.9.2001 08:44:29)
		dt.setDate(QDate(2001, 9, 11));
		dt.setTime(QTime(8, 44, 29));
		e7mainHeader->setScan_Start_Time_Qt(dt);
		if(e7mainHeader->scan_Start_Time() != 1000194269+utcDiff || e7mainHeader->scan_Start_Time_Qt() != dt)
		{
			cout << "Error on testing datetime conversion routines." << endl;
			exit(2);
		}

		// now we check the patient birth date set/read methods
		QDate bdate(1924, 1, 31);
		e7mainHeader->setPatient_Birth_Date_Qt(bdate);
		if(e7mainHeader->patient_Birth_Date() != -1449100800+utcDiff || e7mainHeader->patient_Birth_Date_Qt() != bdate)
		{
			cout << "Error on testing date conversion routines." << endl;
			exit(2);
		}
		
		delete e7mainHeader;

		cout << "datetime conversion routines check succeeded." << endl;

		// let us write out the data to the file in frame 1
		if(file.writeMatrix((char*)matrixData_frame2, 
												MATRIX_SIZE, 
												CECATSubHeader::SunShort, 2) == false)
			cout << "Error during writeMatrix() operation for frame 2" << endl;
		else
		{
			cout << "successfully written matrix data for frame 2" << endl;

			if(file.writeMatrix((char*)matrixData_frame1,
													MATRIX_SIZE,
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

				imageHeader->setNum_Dimensions(NUM_DIMENSIONS);
				imageHeader->setX_Dimension(X_DIMENSION);
				imageHeader->setY_Dimension(Y_DIMENSION);
				imageHeader->setZ_Dimension(Z_DIMENSION);
				imageHeader->setScale_Factor(SCALE_FACTOR);
				imageHeader->setX_Pixel_Size(X_PIXELSIZE);
				imageHeader->setY_Pixel_Size(Y_PIXELSIZE);
				imageHeader->setZ_Pixel_Size(Z_PIXELSIZE);

				// put the real generated frame number in the user section of
				// the image header
				imageHeader->setUser_Reserved(0, 1);
				if(imageHeader->save() == false)
				{
					cout << "ERROR on saving subheader of frame #1" << endl;
					exit(2);
				}
					
				// now we also write out the 2nd frame.
				imageHeader->setUser_Reserved(0, 2);

				// and write the subheader again to frame 2
				if(file.writeSubHeader(*imageHeader, 2) == false)
				{
					cout << "ERROR on saving subheader of frame #2" << endl;
					exit(2);
				}					
			}
			else
				cout << "ERROR: couldn't read subHeader from file." << endl;

			/////////////////////////////////////////////////////////////////////
			//
			// We generate a 'readwritematrix2.v' file here with initially 1 frame
			// and a subheader for testing the file2file copy routines.
			//
			CECATFile file2("readwritematrix2.v", CECATMainHeader::ECAT7_Volume16);
			if(file2.open(QIODevice::WriteOnly))
			{
				CECATSubHeader* newSubHeader = file2.createEmptySubHeader();
				if(newSubHeader->subHeaderType() == CECATSubHeader::ECAT7_Image)
				{
					CECAT7SubHeaderImage* imageHeader = static_cast<CECAT7SubHeaderImage*>(newSubHeader);

					imageHeader->setNum_Dimensions(NUM_DIMENSIONS);
					imageHeader->setX_Dimension(X_DIMENSION);
					imageHeader->setY_Dimension(Y_DIMENSION);
					imageHeader->setZ_Dimension(Z_DIMENSION);
					imageHeader->setScale_Factor(SCALE_FACTOR);
					imageHeader->setX_Pixel_Size(X_PIXELSIZE);
					imageHeader->setY_Pixel_Size(Y_PIXELSIZE);
					imageHeader->setZ_Pixel_Size(Z_PIXELSIZE);
					
					if(file2.writeSubHeader(*imageHeader, 1) == false)
					{
						cout << "ERROR on saving subheader of readwritematrix2.v" << endl;
						exit(2);
					}					

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
				cout << "ERROR: newSubHeader has wrong type!" << endl;

			/////////////////////////////////////////////////////////////////////
			//
			// Now we write out matrix data to test the writeMatrix() routines
			//			
			cout << "writing out matrixData to frame 3..." << endl;

			// and to test the write operations to write the matrix together
			// with the subheader lets do it now and write frame 3
			CECAT7SubHeaderImage* imageHeader = static_cast<CECAT7SubHeaderImage*>(subHeader);
			imageHeader->setUser_Reserved(0, 3);
			if(file.writeMatrix((char*)matrixData_frame2, MATRIX_SIZE,
													*imageHeader, 3) == false)
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

				if(e7mainHeader->save() == false)
				{
					cout << "ERROR on saving mainheader of file 'readwritematrix.v'" << endl;
					exit(2);
				}										
			}
			delete mainHeader;

			/////////////////////////////////////////////////////////////////////
			//	
			// for testing that the directory list stuff works above > 31 frames we write out
			// 30 more subheaders to the file via writeMatrix()
			// 
			cout << "writing " << ADDITIONAL_FRAMES << " additional frames via 'writeMatrix()'";
			for(int i=0; i < ADDITIONAL_FRAMES; i++)
			{
				cout << ".";
			  imageHeader->setUser_Reserved(0, i+3);

				if(file.writeMatrix((char*)matrixData_frame2, MATRIX_SIZE, *imageHeader, i+3) == false)
				{
					cout << "ERROR on saving matrix/subheader of frame #" << i+3 << endl;
					exit(2);
				}										
			}
			cout << endl;

			/////////////////////////////////////////////////////////////////////
			//	
			// for testing that the directory list stuff works above > 31 frames we write out
			// 30 more subheaders to the file via writeMatrix()
			// 
			cout << "writing " << ADDITIONAL_FRAMES << " additional frames via 'writeSubHeader()/writeMatrix()'";
			for(int i=0; i < ADDITIONAL_FRAMES; i++)
			{
				cout << ".";
			  imageHeader->setUser_Reserved(0, i+3+ADDITIONAL_FRAMES);

				if(file.writeSubHeader(*imageHeader, i+3+ADDITIONAL_FRAMES) == false)
				{
					cout << "ERROR on saving subheader of frame #" << i+3+ADDITIONAL_FRAMES << endl;
					exit(2);
				}					
					
				if(file.writeMatrix((char*)matrixData_frame2, MATRIX_SIZE, i+3+ADDITIONAL_FRAMES) == false)
				{
					cout << "ERROR on saving matrix data of frame #" << i+3+ADDITIONAL_FRAMES << endl;
					exit(2);
				}										
			}
			cout << endl;
			
			delete subHeader;
		}

		// close the file again
		file.close();
	}
	else
	{
		cout << "ERROR on trying to open readwritematrix.v for writing" << endl;
		exit(2);
	}

	/////////////////////////////////////////////////////////////////////
	//	
	// now we open the file read-only and check that all data was
	// successfully written to the file
	// 
	if(file.open(QIODevice::ReadOnly))
	{
		// read in the matrix in a separate buffer
		quint16* readBuf = NULL;
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
			for(; i < MATRIX_DIMENSION; i++)
			{
				if(matrixData_frame1[i] != readBuf[i])
					break;
			}

			if(i < MATRIX_DIMENSION)
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
			for(; i < MATRIX_DIMENSION; i++)
			{
				if(matrixData_frame2[i] != readBuf[i])
					break;
			}

			if(i < MATRIX_DIMENSION)
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
		cout << "reading the data of the " << ADDITIONAL_FRAMES << " frames";
		bool success = true;
		for(int i=0; i < ADDITIONAL_FRAMES; i++)
		{
			cout << ".";
			CECATSubHeader* subHeader = NULL;
				
			if(file.readSubHeader(subHeader, i+3) == false || 
				 subHeader == NULL)
			{
				cout << "ERROR: in checking subheader of frame #" << i+3;
				delete subHeader;
				success = false;
				break;
			}

			CECAT7SubHeaderImage* imageHeader = static_cast<CECAT7SubHeaderImage*>(subHeader);
			if(imageHeader->user_Reserved(0) != i+3 ||
				 imageHeader->num_Dimensions() != NUM_DIMENSIONS ||
				 imageHeader->x_Dimension() != X_DIMENSION ||
				 imageHeader->y_Dimension() != Y_DIMENSION ||
				 imageHeader->z_Dimension() != Z_DIMENSION ||
				 (float)imageHeader->scale_Factor() != (float)SCALE_FACTOR ||
				 (float)imageHeader->x_Pixel_Size() != (float)X_PIXELSIZE ||
				 (float)imageHeader->y_Pixel_Size() != (float)Y_PIXELSIZE ||
				 (float)imageHeader->z_Pixel_Size() != (float)Z_PIXELSIZE)
			{
				cout << endl << "ERROR: header data of subheader frame #" << i+3 << " seems to be invalid!" << endl;
				cout << "framenum: " << dec << imageHeader->user_Reserved(0) << endl;
				cout << "numDimen: " << dec << imageHeader->num_Dimensions() << endl;
				cout << "x_Dimens: " << dec << imageHeader->x_Dimension() << endl;
				cout << "y_Dimens: " << dec << imageHeader->y_Dimension() << endl;
				cout << "z_Dimens: " << dec << imageHeader->z_Dimension() << endl;
				cout << "scaleFac: " << dec << imageHeader->scale_Factor() << endl;
				cout << "x_PixelS: " << dec << imageHeader->x_Pixel_Size() << endl;
				cout << "y_PixelS: " << dec << imageHeader->y_Pixel_Size() << endl;
				cout << "z_PixelS: " << dec << imageHeader->z_Pixel_Size() << endl;

				delete subHeader;
				success = false;
				break;
			}
				 
			if(file.readMatrix((char*&)readBuf, len, i+3) == false)
			{
				cout << "ERROR: in reading matrix data of frame #" << i+3;
				delete subHeader;
				success = false;
				break;
			}

			unsigned int j=0;
			for(; j < MATRIX_DIMENSION; j++)
			{
				if(matrixData_frame2[j] != readBuf[j])
				{
					cout << "ERROR: read MatrixData != written MatrixData at position " << j << " (" << FilePos2ECATBlock(j)<< ") in frame #" << i+3 << endl;
					cout << "ERROR: read: " << hex << uppercase << readBuf[j] << " - written: " << matrixData_frame2[j];
					delete[] readBuf;
					delete subHeader;
					success = false;
					break;
				}					
			}

			// free the read matrix data
			delete[] readBuf;	
			delete subHeader;
		}
		cout << endl;

		if(success)
			cout << "NO error on compare detected." << endl;
		else
			cout << "ERROR on compare detected." << endl;
	
		// close the file
		file.close();
	}
	else
	{
		cout << "Error on opening the file readonly" << endl;
		exit(2);
	}

	delete[] matrixData_frame1;
	delete[] matrixData_frame2;

	return returnCode;
}
