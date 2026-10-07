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

            qint64 expectedVoxelBytes = static_cast<qint64>(nx) * static_cast<qint64>(ny) * static_cast<qint64>(nz) * static_cast<qint64>(bytesPerVoxel);

            cout << "ECAT matrix size read by libmedio: "
                << voxelData->size()
                << " bytes" << endl;

            cout << "Expected image voxel size: "
                << expectedVoxelBytes
                << " bytes" << endl;

            cout << "Difference: "
                << (voxelData->size() - expectedVoxelBytes)
                << " bytes" << endl;

            
            // Check that the ECAT matrix contains at least all expected voxel data
            if (voxelData->size() < expectedVoxelBytes) {
                cout << "Error: ECAT matrix is smaller than the expected voxel data size."
                    << endl;

                delete ecatMainHeader;
                delete ecatSubHeader;
                delete voxelData;

                return 1;
            }

            if (voxelData->size() > expectedVoxelBytes) {
                cout << "Notice: ignoring "
                    << (voxelData->size() - expectedVoxelBytes)
                    << " trailing bytes after the voxel matrix."
                    << endl;
            }

            QByteArray* flippedData = new QByteArray(static_cast<int>(expectedVoxelBytes), 0);

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
                json["Calibration_Units"] = static_cast<int>(mainEcat->calibration_Units());
                json["Acquisition_Type"] = static_cast<int>(mainEcat->acquisition_Type());
                json["Lwr_True_Thres"] = static_cast<int>(mainEcat->lwr_True_Thres());
                json["Upr_True_Thres"] = static_cast<int>(mainEcat->upr_True_Thres());
                json["Calibration_Units_Label"] = static_cast<int>(mainEcat->calibration_Units_Label());

                json["Lwr_Sctr_Thres"] = static_cast<int>(mainEcat->lwr_Sctr_Thres());

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
                if (imgEcat) {
                    json["Recon_Zoom"] = static_cast<double>(imgEcat->recon_Zoom());
                }           


            }


            if (useNifti2) {
                CNIFTI2MainHeader* n2 = static_cast<CNIFTI2MainHeader*>(niftiHeader);

                if(n2){
                    int extensionSize = n2->headerExtensionSize(json);


                    n2->setVox_Offset(static_cast<qint64>(540 + 4 + extensionSize)); // Update vox_offset to account for the extension size
                    
                }
            } else {
                CNIFTI1MainHeader* n1 = static_cast<CNIFTI1MainHeader*>(niftiHeader);

                if(n1){
                    int extensionSize = n1->headerExtensionSize(json);

                    n1->setVox_Offset(static_cast<qint64>(348 + 4 + extensionSize)); // Update vox_offset to account for the extension size
                }
            }
            // =========================================================================================
            // Write the NIfTI file including the JSON extension 

            // If NIfTI-1 is selected and ECAT data is available, add the JSON extension

            // Write the NIfTI main header
            // Vox_offset has already been updated to account for the extension

            if(!niftiFile.writeMainHeader(*niftiHeader)) {
                cout << "Error: Failed to write the NIfTI main header." << endl;
                delete niftiHeader;
                return 1;
            }

            // Write the JSON metadata as a NIfTI header extension
            bool extensionWritten = false;

            if(mainEcat) {
                if (useNifti2) {
                    CNIFTI2MainHeader* n2 = static_cast<CNIFTI2MainHeader*>(niftiHeader);

                    // NIfTI-2 main header = 540 bytes
                    niftiFile.seek(540); // Move the file pointer to the position where the extension should be written

                    extensionWritten = n2->writeHeaderExtension(niftiFile, json);

                } else {
                    CNIFTI1MainHeader* n1 = static_cast<CNIFTI1MainHeader*>(niftiHeader);                    
                    
                    // NIfTI-1 main header = 348 bytes
                    niftiFile.seek(348);

                    extensionWritten = n1->writeHeaderExtension(niftiFile, json);
                }

                if (!extensionWritten) {
                    cout << "Error: Failed to write the NIfTI header extension." << endl;
                    delete niftiHeader;
                    return 1;
                }
                
                cout << "Notice: Embedded BIDS JSON metadata written into the NIfTI header extension." << endl;

            } else {
                cout << "Notice: No ECAT metadata available for JSON extension." << endl;
            }


            // Write the voxel matrix 
            if(!niftiFile.writeMatrix(*voxelData)) {

                cout << "Error: Failed to write the voxel matrix." << endl;

                delete niftiHeader;
                return 1;
            }
            niftiFile.close();


            // =========================================================================================
            // Continue generating the external JSON sidecar 
            if (mainEcat) {
                QFileInfo niftiFileInfo(outputFilename);

                QString baseName = niftiFileInfo.fileName();

                if(baseName.endsWith(".nii.gz", Qt::CaseInsensitive)) {
                    baseName.chop(7);
                }
                else if(baseName.endsWith(".nii", Qt::CaseInsensitive)) {
                    baseName.chop(4);
                }

                QString jsonFilename = niftiFileInfo.absolutePath() + "/" + baseName + ".json";

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