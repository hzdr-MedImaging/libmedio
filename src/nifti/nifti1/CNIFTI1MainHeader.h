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

#ifndef CNIFTI1MAINHEADER_H
#define CNIFTI1MAINHEADER_H

#include <QDataStream>
#include <QDateTime>
#include <QTextStream>

#ifndef __MEDIO_PRIVATE__
#include <CNIFTIMainHeader>
#else
#include <CNIFTIMainHeader.h>
#include <CConcordeMainHeader.h>
#endif

#include <time.h>

// forward declarations
class CNIFTI1MainHeaderPrivate;
class CNIFTIFile;

class CNIFTI1MainHeader : public CNIFTIMainHeader { // derived class from CNIFTIMainHeader (base class for NIfTI main headers)
  public:
    // possible NIFTI1 file types
    enum File_Type { Unknown=0, Sinogram, Image16, AttenuationCorr,
                     Normalization, PolarMap, Volume8, Volume16,
                     Projection8, Projection16, Image8, Sinogram3D_16,
                     Sinogram3D_8, Normalization_3D, Sinogram3D_Float
                   };

    enum Transm_Source_Type        { SRC_NONE=0, SRC_RRING, SRC_RING, SRC_ROD,
                                    SRC_RROD };
    enum Angular_Compression      { No_Mash=0, Mash2, Mash4 };
    enum Coin_Samp_Mode            {  NetTrues=0, PromptsAndDelayed,
                                    PromptsDelayedMultiples=3 };
    enum Axial_Samp_Mode          {  Normal=0, X2, X3 };
    enum Calibration_Units        {  Uncalibrated=0, Calibrated, CalibrationUnits_Processed };
    enum Calibration_Units_Label  {  Blood_Flow=0, LMRGLU, Label_Processed };
    enum Compression_Code          {  Comp_None=0 };
    enum Patient_Sex              { Sex_Male='M', Sex_Female='F', Sex_Unknown='U' };
    enum Patient_Dexterity        { Dext_RT='R', Dext_LF='L', Dext_Unknown='U' };
    enum Patient_Orientation      { FF_Prone=0, HF_Prone, FF_Supine, HF_Supine,
                                    FF_Right, HF_Right, FF_Left, HF_Left,
                                    Orient_Unknown };
    enum Acquisition_Type          {  Undefined=0, Blank, Transmission,
                                    StaticEmission, DynamicEmission,
                                    GatedEmission, TransmissionRectilinear,
                                    EmissionRectilinear };
    enum Acquisition_Mode          {  AcqNormal=0, Windowed, WindowedAndNonWindowed,
                                    DualEnergy, UpperEnergy, EmissionTransmission };
    enum Septa_State              { Extended=0, Retracted };
       
//===========================================================================================================
// Constructors
    CNIFTI1MainHeader(CNIFTIFile* niftiFile = NULL, CNIFTIMainHeader::Type fileType = CNIFTIMainHeader::Unknown);
// Destructor  
    ~CNIFTI1MainHeader();

// Copy constructor
    CNIFTI1MainHeader(const CNIFTI1MainHeader& src);    
// Default assignment operator
    CNIFTI1MainHeader& operator=(const CNIFTI1MainHeader& src);

// header clear method
    void clear();

// file i/o Methods
    bool load();
    bool save() const;

// the number of bytes the data of that header requires on disk
    int rawDataSize() const;
  
// data streaming methods
    friend QTextStream& operator<<(QTextStream& stream, const CNIFTI1MainHeader& mHeader);
    friend QTextStream& operator>>(QTextStream& stream, CNIFTI1MainHeader& mHeader);
    
// runtime type information methods
    CNIFTIMainHeader::HeaderType mainHeaderType() const;

// clone methods
    CMedIOHeader* clone() const;

// conversion methods
    bool convertFrom(const CMedIOHeader* mainHeader, const CMedIOHeader* subHeader=NULL);

// Getter Methods
    qint32 sizeof_Hdr(void) const;
    short dim(const short index) const;
    float pix_Dim(const short index) const;
    float vox_Offset(void) const;
    short qform_Code(void) const;
    short sform_Code(void) const;
    float qoffset_X(void) const;
    float qoffset_Y(void) const;
    float qoffset_Z(void) const;
    const char* descrip(void) const;
    const char* magic(void) const;
    short bit_Pix(void) const;
    float srow_X(const short index) const;
    float srow_Y(const short index) const;
    float srow_Z(const short index) const;
    const char* data_Type(void) const;
    const char* db_Name(void) const;
    quint32 extents(void) const;
    short session_Error(void) const;
    char regular(void) const;
    char dim_Info(void) const;
    float intent_P1(void) const;
    float intent_P2(void) const;
    float intent_P3(void) const;
    short intent_Code(void) const;
    short dataType(void) const;
    short slice_Start(void) const;
    short slice_End(void) const;
    char slice_Code(void) const;
    char xyzt_Units(void) const;
    float cal_Max(void) const;
    float cal_Min(void) const;
    float slice_Duration(void) const;
    float toffset(void) const;
    quint32 glmax(void) const;
    quint32 glmin(void) const;
    const char* aux_File(void) const;
    const char* intent_Name(void) const;
    float quatern_B(void) const;
    float quatern_C(void) const;
    float quatern_D(void) const;
    float scl_Slope(void) const;
    float scl_Inter(void) const;

// Setter Methods
    void setSizeof_Hdr(const qint32 size);
    void setDim(const short index, const short value);
    void setPix_Dim(const short index, const float value);
    void setQform_Code(const short code);
    void setSform_Code(const short code);
    void setQoffset_X(const float offset);
    void setQoffset_Y(const float offset);
    void setQoffset_Z(const float offset);
    void setQuatern_B(const float val);
    void setQuatern_C(const float val);
    void setQuatern_D(const float val);
    void setDescrip(const char* desc);
    void setMagic(const char* magic);
    void setData_Type(const char* dataType);
    void setDb_Name(const char* dbName);
    void setExtents(const quint32 extents);
    void setSession_Error(const short error);
    void setRegular(const char regular);
    void setDim_Info(const char dimInfo);
    void setIntent_P1(const float p1);
    void setIntent_P2(const float p2);
    void setIntent_P3(const float p3);
    void setIntent_Code(const short code);
    void setDataType(const short dataType);
    void setBit_Pix(const short bitPix);
    void setSlice_Start(const short start);
    void setVox_Offset(const float offset);
    void setSlice_End(const short end);
    void setSlice_Code(const char code);
    void setXyzt_Units(const char units);
    void setCal_Max(const float max);
    void setCal_Min(const float min);
    void setSlice_Duration(const float duration);
    void setToffset(const float toffset);
    void setGlmax(const quint32 glmax);
    void setGlmin(const quint32 glmin);
    void setAux_File(const char* auxFile);
    void setSrow_X(const short index, const float value);
    void setSrow_Y(const short index, const float value);
    void setSrow_Z(const short index, const float value);
    void setIntent_Name(const char* intentName);    
    void setScl_Slope(float slope);
    void setScl_Inter(float inter);
/*
    // special Qt-based methods for easy time conversion of the really
    // mad ECAT time specifications
    QDate patient_Birth_Date_Qt(void) const;
    QDateTime scan_Start_Time_Qt(void) const;
    QDateTime dose_Start_Time_Qt(void) const;
    void setPatient_Birth_Date_Qt(const QDate& date);
    void setScan_Start_Time_Qt(const QDateTime& dateTime);
    void setDose_Start_Time_Qt(const QDateTime& dateTime);
*/

    // ===============================================================================
    // Method to write the header extension to a NIfTI file. 
    // It takes a reference to a CNIFTIFile object and a QJsonObject containing the metadata to be written. 
    bool writeHeaderExtension(CNIFTIFile& file, const QJsonObject& json);

    // Method to read the header extension from a NIfTI file.
    // It takes a reference to a CNIFTIFile object and returns a QJsonObject containing the metadata read from the header extension.        
    QJsonObject readHeaderExtension(CNIFTIFile& file) const;
    //================================================================================

  private:
    CNIFTI1MainHeaderPrivate*  m_pData;
};

#endif // CNIFTI1MAINHEADER_H
