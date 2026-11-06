/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio
 
  Copyright (C) 2004-2026 hzdr.de and contributors
 
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
    http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <CNIFTIFile>
#include <CNIFTI1MainHeader>
#include <CNIFTI2MainHeader>

#include <iostream>
#include <stdlib.h>
#include <QTextStream>
#include <QFileInfo>

using namespace std;

//  Function:    main
//! 
//! This is a small and simple example on how to use the libmedio c++
//! to access NIFTI1 and NIFTI2 files and just print out the header to the current
//! stdout stream. It should show how _simple_ it is to use libmedio and how
//! quickly you can access a NIFTI file... isn't it nice? ;)
//! 
//! @param       argc number of commandline options
//! @param       *argv[] pointer to a array of the commandline options
//! @return      integer of the exit code.
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {
  
  int returnCode = 2; // return an error on default

  // You want this, unless you mix couts output with C output.
  // Read  http://gcc.gnu.org/onlinedocs/libstdc++/27_io/howto.html#8 for an explanation.
  ios::sync_with_stdio(false);

  cout << "libmedio NIFTI file processing example" << endl;
  cout << "----------------------------------------" << endl;
  
  // check if the user has specified a filename or not
  if(argc > 1) {
    char* filename = argv[1]; // get the name of the file from the commandline arguments

    QString outputCopyFilename = (argc > 2) ? argv[2] : "copy_test.nii"; // get the name of the output file from the commandline arguments or use a default name
    QString outputRawFilename = (argc > 3) ? argv[3] : "voxel_output.raw"; 

    // Prevent overwriting the input file during the writing test
    QFileInfo inputInfo(QString::fromLocal8Bit(filename));
    QFileInfo outputInfo(outputCopyFilename);

    if (inputInfo.absoluteFilePath() == outputInfo.absoluteFilePath()) {
        cout << "Error: input and output filenames must be different." << endl;
        return 1;
    }


    // open the file
    CNIFTIFile file(filename); // create a CNIFTIFile object with the specified filename
    //if(isOpen && formatVal != CNIFTIFile::Undefined) { 
    if(file.open(QIODevice::ReadOnly) && file.dataFormat() == CMedIOData::NIFTI) { // check if the file is opened successfully and if the format is not undefined

      cout << "Successfully loaded file '" << filename << "'" << endl << endl;

      //===============================================================================================================
      // now that we have successfully loaded the NIFTI file we can go and
      // output some statistics on it:
      cout << "General file information:" << endl;
      cout << "------------------------" << endl;
      cout << file.dataFormat() << endl;
      cout << "NIFTI format version: " << (file.format() == CNIFTIFile::NIFTI1 ? "1" : "2") << endl;
      // now that we have put out gernal information, lets print out more detailed
      // main header information
      cout << endl;

      CNIFTIMainHeader* mainHeader; // pointer to the main header of the NIFTI1 file
      file.readMainHeader(mainHeader);

      if(file.format() == CNIFTIFile::NIFTI1) {
        CNIFTI1MainHeader* mHeader = static_cast<CNIFTI1MainHeader*>(mainHeader); // cast the main header to the NIFTI1 main header type
        
        // Calling the overloaded operator<< (defined in the CNIFTI1MainHeader class) to print the main header information to stdout
        QTextStream out(stdout);
        out << *mHeader;
        out.flush();

        //cout << "SIZEOF_HDR:" << mHeader->sizeof_Hdr() << endl;

        delete mainHeader; // delete the main header pointer to free up memory

      } else if(file.format() == CNIFTIFile::NIFTI2) {
        CNIFTI2MainHeader* mHeader = static_cast<CNIFTI2MainHeader*>(mainHeader); // cast the main header to the NIFTI2 main header type
        QTextStream out(stdout);
        out << *mHeader;
        out.flush();
        delete mainHeader; // delete the main header pointer to free up memory
      }

//===============================================================================================================
// Now that we have printed out the main header information, we can read the voxel data from the NIFTI file and store it in a QByteArray. 
// This allows us to access the voxel data in memory for further processing or analysis.
      QByteArray* pVoxelData = NULL; // null pointer to hold the voxel data read from the NIFTI file
      
      if(file.readMatrix(pVoxelData)) { // read the voxel data into the QByteArray
      // check if the readMatrix function was successful in reading the voxel data --> readMatrix returns true if the read was successful, false otherwise.
        
        cout << endl << "----------------------------------------" << endl;
        cout << "Successfully read the voxel data into a QByteArray" << endl;
        cout << "Voxel data size: " << pVoxelData->size() << " bytes" << endl;

        // Here you can process the voxel data as needed, for example we can export the matrix voxel data in binary format.
        
        // Write the QByteArray on a local binary file
        QFile rawFile(outputRawFilename); // create a QFile object for the output raw file
        if(rawFile.open(QIODevice::WriteOnly)) {
          rawFile.write(*pVoxelData);
          rawFile.close();
          cout << "Voxel data successfully exported to '" << outputRawFilename.toStdString() << "'" << endl;
        } else {
          cout << "Error: Could not open '" << outputRawFilename.toStdString() << "' for writing." << endl;
        }
        
        delete pVoxelData; // delete the voxel data pointer to free up memory
      }
      // close the NIFTI file
      file.close();
      returnCode = 0; // return success
    
// === Writing Test  ===
      QString inputFilename = argv[1];
      QString outputFilename = outputCopyFilename; // output filename for the copy test

      cout << endl << "----------------------------------------" << endl;
      cout << "Writing test to: " << outputFilename.toStdString() << endl;


      // 1. First of all open the original file in reading mode to read the main header and the matrix voxel data
      CNIFTIFile inFile(inputFilename);

      if (inFile.open(QIODevice::ReadOnly)) { // open the original file in reading mode

          // Save the detected NIfTI version
          CNIFTIFile::NIFTIFormat inputFormat = inFile.format();

          CNIFTIMainHeader* mainHeader = NULL;
          inFile.readMainHeader(mainHeader); // read the main header from the original file using the readMainHeader method of the CNIFTIFile class


          QByteArray* voxelData = NULL;
          inFile.readMatrix(voxelData);  // read the matrix voxel data from the original file using the readMatrix method of the CNIFTIFile class

          inFile.close();

          if (mainHeader && voxelData) {
              // 2. Create a new object CNIFTIFile for the output
              // Impose the format explicitly (NIFTI1)
              // Preserve the NIfTI version of the input file
              CNIFTIMainHeader::Type outputType;

              if (inputFormat == CNIFTIFile::NIFTI1) {
                  outputType = CNIFTIMainHeader::NIFTI1;
              }
              else if (inputFormat == CNIFTIFile::NIFTI2) {
                  outputType = CNIFTIMainHeader::NIFTI2;
              }
              else {
                  cout << "Error: unknown NIfTI format." << endl;
                  delete mainHeader;
                  delete voxelData;
                  return 1;
              }

              cout << "Copying as NIfTI-"
                  << (inputFormat == CNIFTIFile::NIFTI1 ? "1" : "2")
                  << endl;

              CNIFTIFile outFile(outputFilename, outputType);

              // Open the output file in writing mode
              if (outFile.open(QIODevice::WriteOnly)) {
                  
                  // Associate the read header with the new output file
                  mainHeader->setMedIOData(&outFile);

                  // 3. Write the header to the new file
                  if (outFile.writeMainHeader(*mainHeader)) { // write the main header to the new output file using the writeMainHeader method of the CNIFTIFile class
                      cout << "Header written with success." << endl;
                  } else {
                      cout << "Error while writing the header!" << endl;
                  }

                  // 4. Write the voxel matrix to the new file
                  if (outFile.writeMatrix(*voxelData)) { // write the voxel matrix to the new output file using the writeMatrix method of the CNIFTIFile class
                      cout << "Voxel matrix written with success." << endl;
                  } else {
                      cout << "Error while writing the voxel matrix!" << endl;
                  }

                  outFile.close();
                  cout << "Copy file generated correctly!" << endl;
              } else {
                  cout << "Error: unable to open " << outputFilename.toStdString() << " for writing." << endl;
              }

              // Memory cleanup
              delete mainHeader;
              delete voxelData;
          }
      }
    } 
    else {
      cout << "Error on loading file '" << argv[1] << "' as an NIFTI1/NIFTI2 file.\n";
  } 


}
 
else {
    cout << "Error: No filename was specified on the commandline" << endl;
    cout << "Usage: " << argv[0] << " <input.nii> [custom_copy.nii] [custom_voxel.raw]" << endl;
  }

  return returnCode;
}

