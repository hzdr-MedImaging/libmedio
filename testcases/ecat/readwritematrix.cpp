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
	#define MATRIX_SIZE	(104857600/sizeof(int))
	int* matrixData = new int[MATRIX_SIZE];

	// fill it with random data
	srand(time(NULL));
	for(long i=0; i < MATRIX_SIZE; i++)
		matrixData[i] = rand();

	// open the file for writing data
	CECATFile file("readwritematrix.tst", CECATMainHeader::ECAT7_Sinogram);
	if(file.open(IO_WriteOnly))
	{
		// let us write out the data to the file in frame 1
		if(file.writeMatrix((char*)matrixData, MATRIX_SIZE*sizeof(int), 1) == false)
			cout << "Error during writeMatrix() operation" << endl;
		else
		{
			cout << "successfully written matrix data for frame 1" << endl;
		}

		// close the file again
		file.close();

		// reopen it as readonly
		if(file.open(IO_ReadOnly))
		{
			// read in the matrix in a separate buffer
			char* readBuf = NULL;
			unsigned int len = 0;

			if(file.readMatrix(readBuf, len, 1) == false)
				cout << "Error during readMatrix() operation" << endl;
			else
			{
				cout << "successfully read matrix data from frame 1" << endl;
				
				// let us compare the data
			}

			// close the file
			file.close();
		}
		else
			cout << "Error on opening the file readonly" << endl;

		delete[] matrixData;
	}
	else
		cout << "Error on opening the file writeonly" << endl;

	return returnCode;
}
