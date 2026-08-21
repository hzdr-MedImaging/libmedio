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

#include <CECATFile.h>
#include <CECATMainHeader.h>
#include <CECAT7MainHeader>
#include <CECATSubHeader.h>
#include <CECAT7SubHeaderImage.h>
#include <CNIFTIFile.h>
#include <CNIFTI1MainHeader.h>
#include <CNIFTI2MainHeader.h>

#include <iostream>
#include <QString>

// Necessary libraries to handle JSON files
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>

using namespace std;

int main(int argc, char* argv[]) {

    cout << "libmedio - ECAT to NIfTI converter" << endl;
    cout << "-----------------------------------------------------------" << endl;
    
    
    // to choose between NIfTI1 and NIfTI2 format, we can use a command line argument or a flag in the code. For example, you can add a command line argument to specify the desired NIfTI format:
    // Parameter to do the parsing of the command line arguments 
    bool useNifti2 = false; // di default usa NIfTI-1
    bool normalize = false;
    QString inputFilename;
    QString outputFilename = "output.nii";

    // Check arguments in the command line to see if the user wants to use NIfTI2 format
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            QString arg = argv[i];
            if (arg == "-2") {
                useNifti2 = true;
            } else if (arg == "--normalize") {
                normalize = true;
            } else if (inputFilename.isEmpty()) {
                inputFilename = arg;
            } else if (outputFilename == "output.nii") {
                outputFilename = arg;
            }
        }
    }

    if (inputFilename.isEmpty()) {
        cout << "Error: No filename was specified on the command line" << endl;
        cout << "Usage: " << argv[0] << " <input_ecat_file> [output_nifti.nii] [-2] [--normalize]" << endl;
        return 1;
    }
    
    // 1. Open the ECAT file
    CECATFile ecatFile(inputFilename); // create an instance of the CECATFile class with the input ECAT filename
    if(!ecatFile.open(QIODevice::ReadOnly)) { // open the ECAT file in read-only mode
        cout << "Error: unable to open " << inputFilename.toStdString() << " for reading." << endl;
        return 1;
    }

    // 2. Read the main header of the ECAT file
    CECATMainHeader* ecatMainHeader = NULL; // get the main header of the ECAT file --> pointer to a CECATMainHeader object
    ecatFile.readMainHeader(ecatMainHeader); // read the main header of the ECAT file using the readMainHeader method of the CECATFile class --> in ecatMainHeader is stored the main header of the ECAT file
    
    CECATSubHeader* ecatSubHeader = NULL; // get the subheader of the ECAT file --> pointer to a CECATSubHeader object
    ecatFile.readSubHeader(ecatSubHeader, 1); // read the subheader of the ECAT file using the readSubHeader method of the CECATFile class --> in ecatSubHeader is stored the subheader of the ECAT file

    // 3. Create a QByteArray to hold the voxel data and read the voxel data from the ECAT file
    QByteArray* voxelData = NULL; // create a QByteArray to hold the voxel data
    ecatFile.readMatrix(voxelData, 1); // read the voxel data from the ECAT file using the readMatrix method of the CECATFile class --> in voxelData is stored the voxel data of the ECAT file
    
    
    //=========================================================================================================
    // Z orientation correction for ECAT to NIfTI conversion
    if (ecatSubHeader && voxelData) {
        int nz = 0;
        CECAT7SubHeaderImage* imgSubHeader = dynamic_cast<CECAT7SubHeaderImage*>(ecatSubHeader);
        if (imgSubHeader != NULL) {
            nz = imgSubHeader->z_Dimension();
        } else if (ecatMainHeader != NULL) {
            nz = ecatMainHeader->num_Planes();
        }

        if (nz > 1) {

            int nx = imgSubHeader->x_Dimension();
            int ny = imgSubHeader->y_Dimension();
            
            int bytesPerVoxel = (imgSubHeader->data_Type() == CECATSubHeader::IEEEFloat || 
                                 imgSubHeader->data_Type() == CECATSubHeader::VAX_Rx4) ? 4 : 2;
            
            int sliceSize = nx * ny * bytesPerVoxel;

            QByteArray* flippedData = new QByteArray(voxelData->size(), 0); 

            for (int z = 0; z < nz; ++z) {
                int srcOffset = z * sliceSize;
                int destOffset = (nz - 1 - z) * sliceSize; // Calculate the destination offset for the flipped slice
                memcpy(flippedData->data() + destOffset, voxelData->constData() + srcOffset, sliceSize); // Copy the slice data to the flipped position
            }

            delete voxelData;
            voxelData = flippedData;
            cout << "Notice: Applied geometric Z-flip to match NIfTI orientation." << endl;
        }
    }
    //=========================================================================================================

    // 4. Close the ECAT file
    ecatFile.close(); // close the ECAT file using the close method of the CECATFile class
    

    if(ecatMainHeader && ecatSubHeader && voxelData) { // if the main header, subheader and voxel data are successfully read from the ECAT file
        // 5. Create a NIfTI file in output and write the voxel data to it
        //CNIFTIFile niftiFile(outputFilename, CNIFTIMainHeader::NIFTI1); // create an instance of the CNIFTIFile class with the output NIfTI file name
        
        /*
        if(!niftiFile.open(QIODevice::WriteOnly)) { // open the NIfTI file in write-only mode using the open method of the CNIFTIFile class
            cout << "Error: unable to open " << outputFilename.toStdString() << " for writing." << endl;
            return 1;
        }

        // 6. Create a NIfTI main header and set its properties based on the ECAT main header and subheader
        CNIFTI1MainHeader niftiMainHeader; // create an instance of the CNIFTI1MainHeader class
        
        // using the setter methods of the CNIFTI1MainHeader class to set the properties of the NIfTI main header based on the ECAT main header and subheader
        niftiMainHeader.setDim(ecatMainHeader->num_Planes(), ecatMainHeader->num_Frames(), ecatMainHeader->num_Gates(), ecatMainHeader->num_Bed_Pos()); // set the dimensions of the NIfTI main header based on the ECAT main header
        niftiMainHeader.setDataType(ecatSubHeader->data_Type()); // set the data type of the NIfTI main header based on the ECAT subheader
        // niftiMainHeader.setVoxelSize( .... );
        // ...

        // 7. Write the NIfTI main header and voxel data to the NIfTI file
        niftiFile.writeMainHeader(niftiMainHeader); // write the NIfTI main header to the NIfTI file using the writeMainHeader method of the CNIFTIFile class
        niftiFile.writeMatrix(*voxelData); // write the voxel data to the NIfTI file using the writeMatrix method of the CNIFTIFile class

        // 8. Close the NIfTI file using the close method of the CNIFTIFile class
        niftiFile.close(); // close the NIfTI file using the close method of the CNIFTIFile class
        delete ecatMainHeader; // delete the ECAT main header to free up memory
        delete ecatSubHeader; // delete the ECAT subheader to free up memory
        delete voxelData; // delete the voxel data to free up memory
        */

        // =========================================================================================
        // Normalization-Mode: Voxel conversion from Short (16-bit) to Float (32-bit) and scale absorption
        if (normalize) {

            CECAT7SubHeaderImage* imgSubHeader = dynamic_cast<CECAT7SubHeaderImage*>(ecatSubHeader);

            if (imgSubHeader && (imgSubHeader->data_Type() == CECATSubHeader::SunShort || 
                                 imgSubHeader->data_Type() == CECATSubHeader::VAX_Ix2)) {
                
                cout << "Notice: Normalization ON. Converting 16-bit integers to 32-bit floats." << endl;
                
                int numVoxels = voxelData->size() / 2;
                QByteArray* floatData = new QByteArray(numVoxels * 4, 0); 
                
                const qint16* shortPtr = reinterpret_cast<const qint16*>(voxelData->constData());
                float* floatPtr = reinterpret_cast<float*>(floatData->data());
                
                float ecatScale = imgSubHeader->scale_Factor();
                if (ecatScale == 0.0f) ecatScale = 1.0f;

                for (int i = 0; i < numVoxels; ++i) {
                    floatPtr[i] = static_cast<float>(shortPtr[i]) * ecatScale;
                }
                
                delete voxelData;
                voxelData = floatData; 

                // Convert in float 
                imgSubHeader->setData_Type(CECATSubHeader::IEEEFloat);
                imgSubHeader->setScale_Factor(1.0f);
            }
        }
        // =========================================================================================
        // Dynamically select the NIfTI format based on the -2 flag
        CNIFTIMainHeader::Type targetFormat = useNifti2 ? CNIFTIMainHeader::NIFTI2 : CNIFTIMainHeader::NIFTI1;

        // Initialize the NIfTI output file with the selected format
        CNIFTIFile niftiFile(outputFilename, targetFormat);

        if (niftiFile.open(QIODevice::WriteOnly)) {

            CNIFTIMainHeader* niftiHeader = NULL;
            if (useNifti2) {
                niftiHeader = new CNIFTI2MainHeader(&niftiFile, CNIFTIMainHeader::NIFTI2);
            } else {
                niftiHeader = new CNIFTI1MainHeader(&niftiFile, CNIFTIMainHeader::NIFTI1);
            }

            // Convert ECAT metadata into the chosen NIfTI format
            niftiHeader->convertFrom(ecatMainHeader, ecatSubHeader);

            // =========================================================================================
            // Normalization-Mode: Overwrite and zero-out spatial header via text stream
            if (normalize) {
                QString overrideStr;
                QTextStream stream(&overrideStr);
                
                stream << "DATA_TYPE 16\nBIT_PIX 32\nQFORM_CODE 0\nSFORM_CODE 2\n"
                       << "QOFFSET_X 0.0\nQOFFSET_Y 0.0\nQOFFSET_Z 0.0\n";

                if (useNifti2) {
                    CNIFTI2MainHeader* n2 = static_cast<CNIFTI2MainHeader*>(niftiHeader);
                    stream << "DIM 3 " << n2->dim(1) << " " << n2->dim(2) << " " << n2->dim(3) << " 1 1 1 1\n"
                           << "PIX_DIM 1 " << n2->pix_Dim(1) << " " << n2->pix_Dim(2) << " " << n2->pix_Dim(3) << " 1 1 1 1\n"
                           << "SROW_X " << n2->srow_X(0) << " " << n2->srow_X(1) << " " << n2->srow_X(2) << " 0.0\n"
                           << "SROW_Y " << n2->srow_Y(0) << " " << n2->srow_Y(1) << " " << n2->srow_Y(2) << " 0.0\n"
                           << "SROW_Z " << n2->srow_Z(0) << " " << n2->srow_Z(1) << " " << n2->srow_Z(2) << " 0.0\n";

                    stream.seek(0);
                    stream >> *n2; 
                } else {
                    CNIFTI1MainHeader* n1 = static_cast<CNIFTI1MainHeader*>(niftiHeader);
                    stream << "DIM 3 " << n1->dim(1) << " " << n1->dim(2) << " " << n1->dim(3) << " 1 1 1 1\n"
                           << "PIX_DIM 1 " << n1->pix_Dim(1) << " " << n1->pix_Dim(2) << " " << n1->pix_Dim(3) << " 1 1 1 1\n"
                           << "SROW_X " << n1->srow_X(0) << " " << n1->srow_X(1) << " " << n1->srow_X(2) << " 0.0\n"
                           << "SROW_Y " << n1->srow_Y(0) << " " << n1->srow_Y(1) << " " << n1->srow_Y(2) << " 0.0\n"
                           << "SROW_Z " << n1->srow_Z(0) << " " << n1->srow_Z(1) << " " << n1->srow_Z(2) << " 0.0\n";
                    
                    stream.seek(0);
                    stream >> *n1;
                }
            }
            // =========================================================================================
            
            //==========================================================================================================
            // SLC_SLOPE correction: Ensure that the scl_slope is not zero to avoid division by zero during scaling
            if (useNifti2) {
                CNIFTI2MainHeader* n2 = static_cast<CNIFTI2MainHeader*>(niftiHeader);
                if (n2->scl_Slope() == 0.0f) {
                    n2->setScl_Slope(1.0f);
                }
            } else {
                CNIFTI1MainHeader* n1 = static_cast<CNIFTI1MainHeader*>(niftiHeader);
                if (n1->scl_Slope() == 0.0f) {
                    n1->setScl_Slope(1.0f);
                }
            }
            //==========================================================================================================

            // =========================================================================================
            // Prepare the JSON before writing the NIfTI file
            CECAT7MainHeader* mainEcat = dynamic_cast<CECAT7MainHeader*>(ecatMainHeader);
            CECAT7SubHeaderImage* imgEcat = dynamic_cast<CECAT7SubHeaderImage*>(ecatSubHeader); 
            
            QJsonObject json; // Initialize the JSON object
            
            if (mainEcat) {
                // Spatial data and patient information
                json["Original_File_Name"] = QString(mainEcat->original_File_Name());
                json["Study_Description"] = QString(mainEcat->study_Description());
                json["Physician_Name"] = QString(mainEcat->physician_Name());
                json["Operator_Name"] = QString(mainEcat->operator_Name());
                json["Facility_Name"] = QString(mainEcat->facility_Name());
                json["Study_Type"] = QString(mainEcat->study_Type());
                json["Patient_Name"] = QString(mainEcat->patient_Name());
                json["Patient_ID"] = QString(mainEcat->patient_ID());
                json["Patient_Sex"] = mainEcat->patient_Sex();
                json["Patient_Age"] = static_cast<double>(mainEcat->patient_Age());
                json["Patient_Weight"] = static_cast<double>(mainEcat->patient_Weight());
                json["Patient_Height"] = static_cast<double>(mainEcat->patient_Height());
                json["Patient_Birth_Date"] = static_cast<int>(mainEcat->patient_Birth_Date());
                json["Patient_Orientation"] = mainEcat->patient_Orientation();

                // Physical and Clinical Data of the PET
                json["Isotope_Name"] = QString(mainEcat->isotope_Name()); 
                json["Isotope_Halflife"] = static_cast<double>(mainEcat->isotope_Halflife());
                json["Radiopharmaceutical"] = QString(mainEcat->radiopharmaceutical());
                json["Dosage"] = static_cast<double>(mainEcat->dosage());
                json["Data_Units"] = QString(mainEcat->data_Units());
                json["Scan_Start_Time"] = static_cast<int>(mainEcat->scan_Start_Time());
                json["Dose_Start_Time"] = static_cast<int>(mainEcat->dose_Start_Time());

                // Geometry
                json["Init_Bed_Position"] = static_cast<double>(mainEcat->init_Bed_Position());
                json["Distance_Scanned"] = static_cast<double>(mainEcat->distance_Scanned());
                json["Transaxial_FOV"] = static_cast<double>(mainEcat->transaxial_FOV());
                json["Bin_Size"] = static_cast<double>(mainEcat->bin_Size());

                // Dynamic 4D Data (Frame timing) from SubHeader
                if (imgEcat) {
                    json["Frame_Start_Time"] = static_cast<double>(imgEcat->frame_Start_Time());
                    json["Frame_Duration"] = static_cast<double>(imgEcat->frame_Duration());
                }
            }

            // =========================================================================================
            // Write the NIfTI file including the JSON extension 
            
            // Write the 348 bytes of the main header
            niftiFile.writeMainHeader(*niftiHeader);

            // If NIfTI-1 is selected and ECAT data is available, add the JSON extension
            /*
            if (!useNifti2 && mainEcat) {
                CNIFTI1MainHeader* n1 = static_cast<CNIFTI1MainHeader*>(niftiHeader);

                // Force the file stream to position 348 explicitly before writing the extension
                niftiFile.seek(348);

                n1->writeHeaderExtension(niftiFile, json);
                std::cout << "Notice: Embedded BIDS-like JSON metadata successfully into NIfTI Header Extension." << std::endl;
            }
            */
           
            // Write the voxel matrix 
            niftiFile.writeMatrix(*voxelData);
            niftiFile.close();

            // Recalculate the resize taking into account the dynamic vox_Offset which now includes the extension (it's no longer fixed to 352)
            int finalOffset = (useNifti2) ? 544 : static_cast<CNIFTI1MainHeader*>(niftiHeader)->vox_Offset();
            QFile::resize(outputFilename, finalOffset + voxelData->size());
            
            delete niftiHeader;
            
            // =========================================================================================
            // Continue generating the external JSON sidecar 
            if (mainEcat) {
                QFileInfo niftiFileInfo(outputFilename); 
                QString jsonFilename = niftiFileInfo.absolutePath() + "/" + niftiFileInfo.completeBaseName() + ".json";

                QFile jsonFile(jsonFilename);
                if (jsonFile.open(QIODevice::WriteOnly)) {
                    QJsonDocument jsonDoc(json);
                    jsonFile.write(jsonDoc.toJson(QJsonDocument::Indented));
                    jsonFile.close();
                    cout << "Notice: BIDS Sidecar JSON generated: " << jsonFilename.toStdString() << endl;
                }
            }
            // =========================================================================================

            cout << "Successfully converted " << inputFilename.toStdString() << " to " << outputFilename.toStdString() 
                 << " (NIfTI-" << (useNifti2 ? "2" : "1") << ")" << endl;
                 
        } else {
            cout << "Error: unable to open " << outputFilename.toStdString() << " for writing." << endl;
        }
    } else {
        cout << "Error: failed to read data from " << inputFilename.toStdString() << endl;
        return 1;
    }
  
    // Clean up
    delete ecatMainHeader;
    delete ecatSubHeader;
    delete voxelData;

    return 0;
}