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

#ifndef CNIFTI2MAINHEADER_H
#define CNIFTI2MAINHEADER_H

#include <QDataStream>
#include <QDateTime>
#include <QTextStream>
#include <QSysInfo>

#ifndef __MEDIO_PRIVATE__
#include <CNIFTIMainHeader>
#else
#include <CNIFTIMainHeader.h>
#include <CConcordeMainHeader.h>
#endif

#include <time.h>

// forward declarations
class CNIFTI2MainHeaderPrivate;
class CNIFTIFile;

class CNIFTI2MainHeader : public CNIFTIMainHeader { // derived class from CNIFTIMainHeader (base class for NIfTI main headers)
  public:
    // possible NIFTI2 file types
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
    CNIFTI2MainHeader(CNIFTIFile* niftiFile = NULL, CNIFTIMainHeader::Type fileType = CNIFTIMainHeader::Unknown);
// Destructor  
    ~CNIFTI2MainHeader();

// Copy constructor
    CNIFTI2MainHeader(const CNIFTI2MainHeader& src);    
// Default assignment operator
    CNIFTI2MainHeader& operator=(const CNIFTI2MainHeader& src);

// header clear method
    void clear();

// file i/o Methods
    bool load();
    bool save() const;

// the number of bytes the data of that header requires on disk
    int rawDataSize() const;
  
// data streaming methods
    friend QTextStream& operator<<(QTextStream& stream, const CNIFTI2MainHeader& mHeader);
    friend QTextStream& operator>>(QTextStream& stream, CNIFTI2MainHeader& mHeader);
    
// runtime type information methods
    CNIFTIMainHeader::HeaderType mainHeaderType() const;

// clone methods
    CMedIOHeader* clone() const;

// conversion methods
    bool convertFrom(const CMedIOHeader* mainHeader, const CMedIOHeader* subHeader=NULL);

// Getter Methods
    qint32 sizeof_Hdr(void) const;
    qint64 dim(const short index) const;
    double pix_Dim(const short index) const;
    qint64 vox_Offset(void) const;
    qint32 qform_Code(void) const;
    qint32 sform_Code(void) const;
    double qoffset_X(void) const;
    double qoffset_Y(void) const;
    double qoffset_Z(void) const;
    const char* descrip(void) const;
    const char* magic(void) const;
    qint16 bit_Pix(void) const;    
    double srow_X(const short index) const;
    double srow_Y(const short index) const;
    double srow_Z(const short index) const;
    qint16 dataType(void) const;
    double intent_P1(void) const;
    double intent_P2(void) const;
    double intent_P3(void) const;
    double cal_Max(void) const;
    double cal_Min(void) const;
    double slice_Duration(void) const;
    double toffset(void) const;
    qint64 slice_Start(void) const;
    qint64 slice_End(void) const;
    const char* aux_File(void) const;
    double quatern_B(void) const;
    double quatern_C(void) const;
    double quatern_D(void) const;
    qint32 slice_Code(void) const;
    qint32 xyzt_Units(void) const;
    qint32 intent_Code(void) const;
    const char* intent_Name(void) const;
    char dim_Info(void) const;
    double scl_Slope(void) const;
    double scl_Inter(void) const;
    


// Setter Methods
    void setSizeof_Hdr(const qint32 size);
    void setDim(const short index, const qint64 value);
    void setPix_Dim(const short index, const double value);
    void setQform_Code(const qint32 code);
    void setSform_Code(const qint32 code);
    void setQoffset_X(const double offset);
    void setQoffset_Y(const double offset);
    void setQoffset_Z(const double offset);
    void setQuatern_B(const double val);
    void setQuatern_C(const double val);
    void setQuatern_D(const double val);
    void setDescrip(const char* desc);
    void setMagic(const char* magic);
    void setDataType(const qint16 dataType);
    void setBit_Pix(const qint16 bitPix);
    void setIntent_P1(const double p1);
    void setIntent_P2(const double p2);
    void setIntent_P3(const double p3);
    void setCal_Max(const double max);
    void setCal_Min(const double min);
    void setSlice_Duration(const double duration);
    void setToffset(const double toffset);
    void setSlice_Start(const qint64 start);
    void setSlice_End(const qint64 end);
    void setAux_File(const char* auxFile);
    void setSlice_Code(const qint32 code);
    void setXyzt_Units(const qint32 units);
    void setIntent_Code(const qint32 code);
    void setIntent_Name(const char* intentName);
    void setDim_Info(const char dimInfo);
    void setSrow_X(const short index, const double value);
    void setSrow_Y(const short index, const double value);
    void setSrow_Z(const short index, const double value);
    void setScl_Slope(const double slope);
    void setScl_Inter(const double inter);
    void setVox_Offset(const qint64 offset);

    // byte order of the file the header was loaded from or will be
    // written to (headers of new files: little endian)
    QSysInfo::Endian byteOrder(void) const;
    void setByteOrder(QSysInfo::Endian order);

    // ===============================================================================
    int headerExtensionSize(const QJsonObject& json) const;
    // Method to write the header extension to a NIfTI file. 
    // It takes a reference to a CNIFTIFile object and a QJsonObject containing the metadata to be written. 
    bool writeHeaderExtension(CNIFTIFile& file, const QJsonObject& json);
    
    // Method to read the header extension from a NIfTI file.
    // It takes a reference to a CNIFTIFile object and returns a QJsonObject containing the metadata read from the header extension.        
    QJsonObject readHeaderExtension(CNIFTIFile& file) const;
    // ===============================================================================        

  private:
    CNIFTI2MainHeaderPrivate*  m_pData;
};

#endif // CNIFTI2MAINHEADER_H
