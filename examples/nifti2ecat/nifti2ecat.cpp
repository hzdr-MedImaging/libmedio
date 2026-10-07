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

#include "CNIFTIFile.h"
#include "CNIFTIMainHeader.h"
#include "CNIFTI1MainHeader.h"
#include "CNIFTI2MainHeader.h"
#include "CECATFile.h"
#include "CECAT7MainHeader.h"
#include "CECAT7SubHeaderImage.h"

#include <cmath>
#include <iostream>
#include <QString>
#include <QByteArray>

// Necessary libraries to handle JSON files
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>

using namespace std;

int main(int argc, char* argv[]){
    cout << "libmedio - NIfTI to ECAT converter" << endl;
    cout << "-----------------------------------------------------------" << endl;
    
    // 1. Parse command line arguments to get input and output filenames
    if(argc < 3) {
        cout << "Usage: " << argv[0] << " <input_nifti_file> <output_ecat_file>" << endl;
        return 1;
    }

    QString inputFilename = argv[1]; // get the input NIfTI file name from the command line arguments
    QString outputFilename = argv[2]; // get the output ECAT file name from the command line arguments

    // 2. Open the NIfTI file in read-only mode
    CNIFTIFile niftiFile(inputFilename); // create an instance of the CNIFTIFile class with the input NIfTI file name
    if(!niftiFile.open(QIODevice::ReadOnly)) { // open the NIfTI file in read-only mode
        cout << "Error: Could not open input NIfTI file: " << inputFilename.toStdString() << endl; // print an error message if the file could not be opened
        return 1;
    }

    // 3. Read the main header from the NIfTI file
    CNIFTIMainHeader* niftiHeader = NULL; // initialize a pointer to the NIfTI main header to NULL
    niftiFile.readMainHeader(niftiHeader); // read the main header from the NIfTI file and store it in the niftiHeader pointer

    QByteArray* voxelData = NULL; // initialize a QByteArray to hold the voxel data
    niftiFile.readMatrix(voxelData); // read the voxel data from the NIfTI file

    if(niftiHeader && voxelData) {

        // 4. Create a new ECAT file (ECAT7) in write-only mode and write the main header and voxel data
        CECATFile ecatFile(outputFilename, CECATMainHeader::CECATMainHeader::ECAT7_Volume16); // create an instance of the CECATFile class with the output ECAT file name and specify the file type as ECAT7_Volume16
        
        if(ecatFile.open(QIODevice::WriteOnly)) {
            
            // 5. Create and convert the MainHeader
            CECAT7MainHeader ecatMainHeader(&ecatFile, CECATMainHeader::CECATMainHeader::ECAT7_Volume16);
            ecatMainHeader.convertFrom(niftiHeader, NULL); 
            
            int nz = 0;
            float pixDim3 = 1.0f;
            int numFrames = 1;

            if (niftiHeader->mainHeaderType() == 1) {
                const CNIFTI1MainHeader* n1 = static_cast<const CNIFTI1MainHeader*>(niftiHeader);
                nz = n1->dim(3);
                numFrames = (n1->dim(4) > 0) ? n1->dim(4) : 1;
                pixDim3 = std::fabs(n1->pix_Dim(3));
            } else if (niftiHeader->mainHeaderType() == 2) {
                const CNIFTI2MainHeader* n2 = static_cast<const CNIFTI2MainHeader*>(niftiHeader);
                nz = n2->dim(3);
                numFrames = (n2->dim(4) > 0) ? n2->dim(4) : 1;
                pixDim3 = std::fabs(n2->pix_Dim(3));
            }

            // Overload manually the critical parameters for the 3D geometry
            ecatMainHeader.setNum_Planes(nz);
            ecatMainHeader.setNum_Frames(numFrames);
            ecatMainHeader.setPlane_Separation(pixDim3 / 10.0f); // mm -> cm

            // 6. Create and convert the SubHeader (USIAMO createEmptySubHeader PER REGISTRARE LA DIRECTORY!)
            CECAT7SubHeaderImage* ecatSubHeader = static_cast<CECAT7SubHeaderImage*>(ecatFile.createEmptySubHeader());
            if (ecatSubHeader != NULL) {
                ecatSubHeader->convertFrom(NULL, niftiHeader); 
                
                // Annotation fissa senza dipendere da config.h
                ecatSubHeader->setAnnotation("Converted with libmedio");
            }
            // =========================================================================================
            // Reading PET metadata (Try NIfTI Header Extension first, then fallback to Sidecar JSON)
            QJsonObject jsonObj;
            bool metadataFound = false;

            // Try to read from NIfTI Header Extension (Support for BOTH NIfTI-1 and NIfTI-2)
            if (niftiHeader->mainHeaderType() == 1) {
                const CNIFTI1MainHeader* n1 = static_cast<const CNIFTI1MainHeader*>(niftiHeader);
                jsonObj = n1->readHeaderExtension(niftiFile);
            } else if (niftiHeader->mainHeaderType() == 2) {
                const CNIFTI2MainHeader* n2 = static_cast<const CNIFTI2MainHeader*>(niftiHeader);
                jsonObj = n2->readHeaderExtension(niftiFile);
            }

            niftiFile.close();

            if (!jsonObj.isEmpty()) {
                cout << "Notice: Found embedded BIDS JSON in NIfTI Extension. Restoring PET metadata..." << endl;
                metadataFound = true;
            }

            // Fallback to external Sidecar JSON if extension was empty
            if (!metadataFound) {
                QFileInfo niftiFileInfo(inputFilename); 
                QString jsonFilename = niftiFileInfo.absolutePath() + "/" + niftiFileInfo.completeBaseName() + ".json";
                
                QFile jsonFile(jsonFilename);
                
                if (jsonFile.exists() && jsonFile.open(QIODevice::ReadOnly)) {
                    QByteArray jsonData = jsonFile.readAll();
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
                    jsonObj = jsonDoc.object();
                    cout << "Notice: Found external BIDS Sidecar JSON. Restoring PET metadata..." << endl;
                    metadataFound = true;
                    jsonFile.close();
                }
            }

            // Apply the extracted metadata to the ECAT header
            if (metadataFound) {
                if (jsonObj.contains("Original_File_Name")) ecatMainHeader.setOriginal_File_Name(jsonObj["Original_File_Name"].toString().toStdString().c_str());
                if (jsonObj.contains("Study_Description")) ecatMainHeader.setStudy_Description(jsonObj["Study_Description"].toString().toStdString().c_str());
                if (jsonObj.contains("Physician_Name")) ecatMainHeader.setPhysician_Name(jsonObj["Physician_Name"].toString().toStdString().c_str());
                if (jsonObj.contains("Operator_Name")) ecatMainHeader.setOperator_Name(jsonObj["Operator_Name"].toString().toStdString().c_str());
                if (jsonObj.contains("Facility_Name")) ecatMainHeader.setFacility_Name(jsonObj["Facility_Name"].toString().toStdString().c_str());
                if (jsonObj.contains("Study_Type")) ecatMainHeader.setStudy_Type(jsonObj["Study_Type"].toString().toStdString().c_str());
                    
                // Restoring Main Header 
                if (jsonObj.contains("Patient_Name")) ecatMainHeader.setPatient_Name(jsonObj["Patient_Name"].toString().toStdString().c_str());
                if (jsonObj.contains("Patient_ID")) ecatMainHeader.setPatient_ID(jsonObj["Patient_ID"].toString().toStdString().c_str());
                if (jsonObj.contains("Patient_Sex")) ecatMainHeader.setPatient_Sex(static_cast<CECAT7MainHeader::Patient_Sex>(jsonObj["Patient_Sex"].toInt()));
                if (jsonObj.contains("Patient_Age")) ecatMainHeader.setPatient_Age(static_cast<float>(jsonObj["Patient_Age"].toDouble()));
                if (jsonObj.contains("Patient_Weight")) ecatMainHeader.setPatient_Weight(static_cast<float>(jsonObj["Patient_Weight"].toDouble()));
                if (jsonObj.contains("Patient_Height")) ecatMainHeader.setPatient_Height(static_cast<float>(jsonObj["Patient_Height"].toDouble()));
                if (jsonObj.contains("Patient_Birth_Date")) ecatMainHeader.setPatient_Birth_Date(jsonObj["Patient_Birth_Date"].toInt());
                if (jsonObj.contains("Patient_Orientation")) ecatMainHeader.setPatient_Orientation(static_cast<CECAT7MainHeader::Patient_Orientation>(jsonObj["Patient_Orientation"].toInt()));
                    
                // Geometry
                if (jsonObj.contains("Init_Bed_Position")) ecatMainHeader.setInit_Bed_Position(static_cast<float>(jsonObj["Init_Bed_Position"].toDouble()));
                if (jsonObj.contains("Distance_Scanned")) ecatMainHeader.setDistance_Scanned(static_cast<float>(jsonObj["Distance_Scanned"].toDouble()));
                if (jsonObj.contains("Transaxial_FOV")) ecatMainHeader.setTransaxial_FOV(static_cast<float>(jsonObj["Transaxial_FOV"].toDouble()));
                if (jsonObj.contains("Bin_Size")) ecatMainHeader.setBin_Size(static_cast<float>(jsonObj["Bin_Size"].toDouble()));
                    
                // PET Data
                if (jsonObj.contains("Isotope_Name")) ecatMainHeader.setIsotope_Name(jsonObj["Isotope_Name"].toString().toStdString().c_str());
                if (jsonObj.contains("Isotope_Halflife")) ecatMainHeader.setIsotope_Halflife(static_cast<float>(jsonObj["Isotope_Halflife"].toDouble()));
                if (jsonObj.contains("Dosage")) ecatMainHeader.setDosage(static_cast<float>(jsonObj["Dosage"].toDouble()));
                if (jsonObj.contains("Radiopharmaceutical")) ecatMainHeader.setRadiopharmaceutical(jsonObj["Radiopharmaceutical"].toString().toStdString().c_str());
                if (jsonObj.contains("Data_Units")) ecatMainHeader.setData_Units(jsonObj["Data_Units"].toString().toStdString().c_str());
                    
                if (jsonObj.contains("Dose_Start_Time")) ecatMainHeader.setDose_Start_Time(jsonObj["Dose_Start_Time"].toInt());
                if (jsonObj.contains("Scan_Start_Time")) ecatMainHeader.setScan_Start_Time(jsonObj["Scan_Start_Time"].toInt());

                // Restore ECAT acquisition/calibration metadata
                if (jsonObj.contains("Calibration_Units")) {
                    ecatMainHeader.setCalibration_Units(static_cast<CECAT7MainHeader::Calibration_Units>(jsonObj["Calibration_Units"].toInt()));
                }

                if (jsonObj.contains("Calibration_Units_Label")) {
                    ecatMainHeader.setCalibration_Units_Label(static_cast<CECAT7MainHeader::Calibration_Units_Label>(jsonObj["Calibration_Units_Label"].toInt()));
                }

                if (jsonObj.contains("Acquisition_Type")) {
                    ecatMainHeader.setAcquisition_Type(static_cast<CECAT7MainHeader::Acquisition_Type>(jsonObj["Acquisition_Type"].toInt()));
                }

                if (jsonObj.contains("Lwr_Sctr_Thres")) {
                    ecatMainHeader.setLwr_Sctr_Thres(static_cast<short>(jsonObj["Lwr_Sctr_Thres"].toInt()));
                }

                if (jsonObj.contains("Lwr_True_Thres")) {
                    ecatMainHeader.setLwr_True_Thres(static_cast<short>(jsonObj["Lwr_True_Thres"].toInt()));
                }

                if (jsonObj.contains("Upr_True_Thres")) {
                    ecatMainHeader.setUpr_True_Thres(static_cast<short>(jsonObj["Upr_True_Thres"].toInt()));
                }
                    
                // Sub Header metadata(Dynamic Data 4D)
                CECAT7SubHeaderImage* imgEcat = dynamic_cast<CECAT7SubHeaderImage*>(ecatSubHeader);
                if (imgEcat) {
                    if (jsonObj.contains("Frame_Start_Time")) imgEcat->setFrame_Start_Time(jsonObj["Frame_Start_Time"].toInt());
                    if (jsonObj.contains("Frame_Duration")) imgEcat->setFrame_Duration(jsonObj["Frame_Duration"].toInt());
                    if (jsonObj.contains("Recon_Zoom")) imgEcat->setRecon_Zoom(static_cast<float>(jsonObj["Recon_Zoom"].toDouble()));
                }
            } else {
                cout << "Notice: No embedded or external JSON metadata found. Converting using spatial NIfTI data only." << endl;
            }
            // =========================================================================================
                
                
            // 7. Writing the regenerated headers to the ECAT file
            ecatFile.writeMainHeader(ecatMainHeader); 
            
            if (ecatSubHeader != NULL) {
                // Write the Subheader and the matrix using frame 1
                ecatFile.writeSubHeader(*ecatSubHeader, 1);
                delete ecatSubHeader; // delete the subheader to free memory
            }

            //====================================================================================================
            // Z orientation correction for ECAT: Invert the Z-axis of the voxel data to match ECAT's coordinate system if the NIfTI header indicates that the Z-axis is inverted (i.e., if the Srow_Z[2] value is negative)
            bool ZFlip = false; // initialize a boolean variable to track if the Z-axis needs to be flipped
            int nx = 0, ny = 0; // initialize variables to hold the dimensions of the voxel data
            int bitpix = 0; // initialize a variable to hold the number of bits per voxel
            int niftiDatatype = 0;

            // Exploit the polymorphism of the CNIFTIMainHeader class (1 = NIfTI-1, 2 = NIfTI-2)
            if (niftiHeader->mainHeaderType() == 1) { 
                const CNIFTI1MainHeader* nifti1Header = static_cast<const CNIFTI1MainHeader*>(niftiHeader); 

                // Check if the Z-axis is inverted by Sform or Qform
                if(nifti1Header->sform_Code() > 0 && nifti1Header->srow_Z(2) > 0.0f) {
                    ZFlip = true; 
                } else if(nifti1Header->sform_Code() == 0 && nifti1Header->qform_Code() > 0 && nifti1Header->pix_Dim(0) > 0.0f) {
                    ZFlip = true;
                }
                
                nx = nifti1Header->dim(1); 
                ny = nifti1Header->dim(2); 
                nz = nifti1Header->dim(3); 
                bitpix = nifti1Header->bit_Pix();
                niftiDatatype = nifti1Header->dataType();

            } else if (niftiHeader->mainHeaderType() == 2) { 
                const CNIFTI2MainHeader* nifti2Header = static_cast<const CNIFTI2MainHeader*>(niftiHeader); 
                    
                if(nifti2Header->sform_Code() > 0 && nifti2Header->srow_Z(2) > 0.0f) {
                    ZFlip = true; 
                } else if(nifti2Header->sform_Code() == 0 && nifti2Header->qform_Code() > 0 && nifti2Header->pix_Dim(0) > 0.0f) {
                    ZFlip = true;
                }

                nx = nifti2Header->dim(1); 
                ny = nifti2Header->dim(2); 
                nz = nifti2Header->dim(3);
                bitpix = nifti2Header->bit_Pix(); 
                niftiDatatype = nifti2Header->dataType();
            }

            // 8. If ZFlip is true, we need to flip the voxel data along the Z-axis to match ECAT's coordinate system
            if(ZFlip && nx > 0 && ny > 0 && nz > 0) {
                int voxelSize = bitpix / 8; // calculate the size of each voxel in bytes
                int sliceSize = nx * ny * voxelSize; // calculate the size of each slice in bytes
                QByteArray* flippedData = new QByteArray(voxelData->size(), 0); // create a new QByteArray to hold the flipped voxel data

                for(int z = 0; z < nz; ++z) { // loop through each slice in the Z direction to invert the order of the slices
                    int srcOffset = z * sliceSize; // calculate the offset for the source slice
                    int destOffset = (nz - 1 - z) * sliceSize; // calculate the offset for the destination slice (flipped)
                    memcpy(flippedData->data() + destOffset, voxelData->constData() + srcOffset, sliceSize); // copy the slice data from the source to the destination
                }

                delete voxelData; // delete the original voxel data to free memory
                voxelData = flippedData; // assign the flipped data to voxelData
                
                cout << "Notice: Applied geometric Z-flip to match ECAT physical layout." << endl;            
            } 
            
            //====================================================================================================
            // NIfTI FLOAT64 has no direct ECAT7 image equivalent.
            // Convert the voxel matrix numerically from double (64 bit)
            // to float (32 bit) before writing it as ECAT IEEEFloat.
            if (niftiDatatype == 64) { // DT_FLOAT64

                qint64 numVoxels = static_cast<qint64>(nx) *static_cast<qint64>(ny) * static_cast<qint64>(nz);

                qint64 expectedSize = numVoxels * static_cast<qint64>(sizeof(double));

                if (voxelData->size() != expectedSize) {
                    cout << "Error: Invalid FLOAT64 voxel matrix size. Expected " << expectedSize << " bytes, got " << voxelData->size() << " bytes." << endl;

                    ecatFile.close();
                    delete niftiHeader;
                    delete voxelData;
                    return 1;
                }

                const double* input = reinterpret_cast<const double*>(voxelData->constData());

                QByteArray* convertedData = new QByteArray(numVoxels * sizeof(float), 0);

                float* output = reinterpret_cast<float*>(convertedData->data());

                for (qint64 i = 0; i < numVoxels; ++i) {
                    output[i] = static_cast<float>(input[i]);
                }

                delete voxelData;
                voxelData = convertedData;

                cout << "Notice: Converted NIfTI FLOAT64 voxel data to ECAT FLOAT32." << endl;
            }

            // 9. Finally, write the voxel data to the ECAT file
            ecatFile.writeMatrix(voxelData->constData(), voxelData->size(), 1); // write the voxel data to the ECAT 

            // 10. Close the ECAT file
            ecatFile.close(); // close the ECAT file
            cout << "Successfully converted NIfTI file " << inputFilename.toStdString() << " to ECAT file " << outputFilename.toStdString() << endl; 
        } else {
            cout << "Error: Could not open output ECAT file: " << outputFilename.toStdString() << endl;
            return 1;
        }
    }

    delete niftiHeader;
    delete voxelData;
    return 0;
}