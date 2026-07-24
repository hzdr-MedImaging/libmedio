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
    short frameNumber = -1;

    if(argc > 2)
      frameNumber = atoi(argv[2]);

    // open the file
    CNIFTIFile file(filename); // create a CNIFTIFile object with the specified filename

    //*********************************************************** 
    // DEBUG: Check if the file is opened successfully and if the format is not undefined
    /*bool isOpen = file.open(QIODevice::ReadOnly);
    int formatVal = file.format();
    std::cout << "DEBUG -> isOpen: " << isOpen <<std::endl;
    std::cout << "DEBUG -> format enum: " << formatVal << " (Undefined is " << CNIFTIFile::Undefined << ")" << std::endl;
    std::cout << "dataFormat: " << file.dataFormat() << std::endl;
    */
    //*********************************************************** 

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

        cout << "SIZEOF_HDR:" << mHeader->sizeof_Hdr() << endl;

        delete mainHeader; // delete the main header pointer to free up memory
        /*
        cout << "Main header data (NIFTI1):" << endl;
        cout << "------------------------" << endl;
        cout << "SIZE_OF_HEADER..........: " << mHeader->size_Of_Header()              << endl;
        cout << "DATA_TYPE...............: " << mHeader->data_Type()                    << endl;
        cout << "DB_NAME.................: " << mHeader->db_Name()                      << endl;
        cout << "EXTENTS.................: " << mHeader->extents()                      << endl;
        cout << "SESSION_ERROR...........: " << mHeader->session_Error()                    << endl;
        cout << "REGULAR.................: " << mHeader->regular()                    << endl;
        cout << "DIM_INFO................: " << mHeader->dim_Info()                    << endl;

        cout << "DIM.....................: ";
        for(int i=0; i < 8; i++)
          cout << mHeader->dim(i) << " ";
        cout << endl;
        cout << "INTENT_P1...............: " << mHeader->intent_P1()                    << endl;
        cout << "INTENT_P2...............: " << mHeader->intent_P2()                    << endl;
        cout << "INTENT_P3...............: " << mHeader->intent_P3()                    << endl;
        cout << "INTENT_CODE.............: " << mHeader->intent_Code()                    << endl;
        cout << "DATATYPE...............: " << mHeader->dataType()                    << endl;
        cout << "BIT_PIX.................: " << mHeader->bit_Pix()                      << endl;
        cout << "SLICE_START.............: " << mHeader->slice_Start()                    << endl;
        cout << "PIX_DIM.................: ";
        for(int i=0; i < 8; i++)
          cout << mHeader->pix_Dim(i) << " ";
        cout << endl;
        cout << "VOX_OFFSET..............: " << mHeader->vox_Offset()                    << endl;
        cout << "SCL_SLOPE...............: " << mHeader->scl_Slope()                    << endl;
        cout << "SCL_INTER...............: " << mHeader->scl_Inter()                    << endl;
        cout << "SLICE_END.................: " << mHeader->slice_End()                    << endl;
        cout << "SLICE_CODE.................: " << mHeader->slice_Code()                    << endl;
        cout << "XYZT_UNITS.................: " << mHeader->xyzt_Units()                    << endl;
        cout << "CAL_MAX.................: " << mHeader->cal_Max()                    << endl;
        cout << "CAL_MIN.................: " << mHeader->cal_Min()                    << endl;
        cout << "SLICE_DURATION..........: " << mHeader->slice_Duration()                << endl;
        cout << "TOFFSET.................: " << mHeader->toffset()                    << endl;
        cout << "GLMAX...................: " << mHeader->glmax()                        << endl;
        cout << "GLMIN...................: " << mHeader->glmin()                        << endl;
        cout << "DESCRIPTION..............: " << mHeader->description()                  << endl;
        cout << "AUX_FILE.................: " << mHeader->aux_File()                  << endl;
        cout << "QFORM_CODE...............: " << mHeader->qform_Code()                    << endl;
        cout << "SFORM_CODE...............: " << mHeader->sform_Code()                    << endl;
        cout << "QUATERN_B........................: " << mHeader->quatern_B()                    << endl;
        cout << "QUATERN_C........................: " << mHeader->quatern_C()                    << endl;
        cout << "QUATERN_D........................: " << mHeader->quatern_D()                    << endl;
        cout << "QOFFSET_X.......................: " << mHeader->qoffset_X()                    << endl;
        cout << "QOFFSET_Y.......................: " << mHeader->qoffset_Y()                    << endl;
        cout << "QOFFSET_Z.......................: " << mHeader->qoffset_Z()                    << endl;
        cout << "SROW_X...........................: " << mHeader->srow_X(0) << " " << mHeader->srow_X(1) << " " << mHeader->srow_X(2) << " "<< mHeader->srow_X(3) << endl;
        cout << "SROW_Y...........................: " << mHeader->srow_Y(0) << " " << mHeader->srow_Y(1) << " " << mHeader->srow_Y(2) << " "<< mHeader->srow_Y(3) << endl;
        cout << "SROW_Z...........................: " << mHeader->srow_Z(0) << " " << mHeader->srow_Z(1) << " " << mHeader->srow_Z(2) << " "<< mHeader->srow_Z(3) << endl;  
        cout << "INTENT_NAME...............: " << mHeader->intent_Name()                    << endl;
        cout << "MAGIC.....................: " << mHeader->magic()                    << endl;
        */

      } else if(file.format() == CNIFTIFile::NIFTI2) {
        CNIFTI2MainHeader* mHeader = static_cast<CNIFTI2MainHeader*>(mainHeader); // cast the main header to the NIFTI2 main header type
        QTextStream out(stdout);
        out << *mHeader;
        out.flush();
        delete mainHeader; // delete the main header pointer to free up memory

        /*
        cout << "Main header data (NIFTI2):" << endl;
        cout << "------------------------" << endl;
        cout << "SIZE_OF_HEADER..........: " << mHeader->size_Of_Header()              << endl;
        cout << "DATA_TYPE...............: " << mHeader->data_Type()                    << endl;
        cout << "DB_NAME.................: " << mHeader->db_Name()                      << endl;
        cout << "EXTENTS.................: " << mHeader->extents()                      << endl;
        cout << "SESSION_ERROR...........: " << mHeader->session_Error()                    << endl;
        cout << "REGULAR.................: " << mHeader->regular()                    << endl;
        cout << "DIM_INFO................: " << mHeader->dim_Info()                    << endl;

        cout << "DIM.....................: ";
        for(int i=0; i < 8; i++)
        {
          cout << mHeader->dim(i) << " ";
        }
        cout << endl;
        cout << "INTENT_P1...............: " << mHeader->intent_P1()                    << endl;

        cout << "INTENT_P2...............: " << mHeader->intent_P2()                    << endl;

        cout << "INTENT_P3...............: " << mHeader->intent_P3()                    << endl;

        cout << "INTENT_CODE...............: " << mHeader->intent_Code()                    << endl;
        cout << "DATATYPE...............: " << mHeader->dataType()                    << endl;
        cout << "BIT_PIX.................: " << mHeader->bit_Pix()                      << endl;
        cout << "SLICE_START.............: " << mHeader->slice_Start()                    << endl;
        cout << "PIX_DIM.................: ";
        for(int i=0; i < 8; i++) {
          cout << mHeader->pix_Dim(i) << " ";
        }
        cout << endl;
        cout << "VOX_OFFSET..............: " << mHeader->vox_Offset()                    << endl;
        cout << "SCL_SLOPE...............: " << mHeader->scl_Slope()                    << endl;
        cout << "SCL_INTER...............: " << mHeader->scl_Inter()                    << endl;
        cout << "SLICE END.................: " << mHeader->slice_End()                    << endl;
        cout << "SLICE CODE.................: " << mHeader->slice_Code()                    << endl;
        cout << "XYZT_UNITS.................: " << mHeader->xyzt_Units()                    << endl;
        cout << "CAL_MAX.................: " << mHeader->cal_Max()                    << endl;
        cout << "CAL_MIN.................: " << mHeader->cal_Min()                    << endl;
        cout << "SLICE_DURATION..........: " << mHeader->slice_Duration()                << endl;
        cout << "TOFFSET.................: " << mHeader->toffset()                    << endl;
        cout << "GLMAX...................: " << mHeader->glmax()                        << endl;
        cout << "GLMIN...................: " << mHeader->glmin()                        << endl;

        cout << "DESCRIPTION..............: " << mHeader->description()                  << endl;
        cout << "AUX_FILE.................: " << mHeader->aux_File()                  << endl;
        cout << "QFORM_CODE...............: " << mHeader->qform_Code()                    << endl;
        cout << "SFORM_CODE...............: " << mHeader->sform_Code()                    << endl;
        cout << "QUATERN_B...................: " << mHeader->quatern_B()                    << endl;
        cout << "QUATERN_C...................: " << mHeader->quatern_C()                    << endl;
        cout << "QUATERN_D...................: " << mHeader->quatern_D()                    << endl;
        cout << "QOFFSET_X...................: " << mHeader->qoffset_X()                    << endl;
        cout << "QOFFSET_Y...................: " << mHeader->qoffset_Y()                    << endl;
        cout << "QOFFSET_Z...................: " << mHeader->qoffset_Z()                    << endl;
        cout << "SROW_X.......................: " << mHeader->srow_X(0) << " " << mHeader->srow_X(1) << " " << mHeader->srow_X(2) << " "<< mHeader->srow_X(3) << endl;
        cout << "SROW_Y.......................: " << mHeader->srow_Y(0) << " " << mHeader->srow_Y(1) << " " << mHeader->srow_Y(2) << " "<< mHeader->srow_Y(3) << endl;
        cout << "SROW_Z.......................: " << mHeader->srow_Z(0) << " " << mHeader->srow_Z(1) << " " << mHeader->srow_Z(2) << " "<< mHeader->srow_Z(3) << endl;
        cout << "INTENT_NAME...............: " << mHeader->intent_Name()                    << endl;
        cout << "MATRIX_CODE...............: " << mHeader->matrix_Code()                    << endl;
        */
      }
      // close the NIFTI file
      file.close();
      returnCode = 0; // return success

    } 
    else {
      cout << "Error on loading file '" << argv[1] << "' as an NIFTI1/NIFTI2 file.\n";
  } 
}
 
else {
    cout << "Error: No filename was specified on the commandline" << endl;
    cout << argv[0] << " <filename> [framenumber]" << endl;
  }

  return returnCode;
}

