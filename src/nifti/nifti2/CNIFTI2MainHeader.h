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
    short dim(const short index) const;
    float pix_Dim(const short index) const;
    short qform_Code(void) const;
    short sform_Code(void) const;
    float qoffset_X(void) const;
    float qoffset_Y(void) const;
    float qoffset_Z(void) const;
    const char* descrip(void) const;
    const char* magic(void) const;

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
  private:
    CNIFTI2MainHeaderPrivate*  m_pData;
};

#endif // CNIFTI2MAINHEADER_H
