/* -*- mode: c++; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 * vim:set ts=2 sw=2 expandtab: *********************************************
 *
 * libmedio - C++ I/O Library for loading/saving medical data formats
 * Copyright (C) 2006-2010 by Jens Langner <Jens.Langner@light-speed.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id$
 *
 **************************************************************************/

#ifndef CECAT7MAINHEADER_H
#define CECAT7MAINHEADER_H

#include <QDataStream>
#include <QDateTime>
#include <QTextStream>

#ifndef __MEDIO_PRIVATE__
#include <CECATMainHeader>
#else
#include <CECATMainHeader.h>
#include <CConcordeMainHeader.h>
#endif

#include <time.h>

// forward declarations
class CECAT7MainHeaderPrivate;
class CECATFile;

class CECAT7MainHeader : public CECATMainHeader
{
  public:
    // possible ECAT7 file types
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
        
    // constructors
    CECAT7MainHeader(CECATFile* ecatFile = NULL, 
                     CECATMainHeader::Type fileType = CECATMainHeader::Unknown);
    ~CECAT7MainHeader();

    // copy constructur and default assignment operator
    CECAT7MainHeader(const CECAT7MainHeader& src);    
    CECAT7MainHeader& operator=(const CECAT7MainHeader& src);

    // header clear method
    void clear();

    // file i/o Methods
    bool load();
    bool save() const;

    // the number of bytes the data of that header requires on disk
    int rawDataSize() const;
  
    // data streaming methods
    friend QTextStream& operator<<(QTextStream& stream, const CECAT7MainHeader& mHeader);
    friend QTextStream& operator>>(QTextStream& stream, CECAT7MainHeader& mHeader);
    
    // runtime type information methods
    CECATMainHeader::HeaderType mainHeaderType() const;

    // clone methods
    CMedIOHeader* clone() const;

    // conversion methods
    bool convertFrom(const CMedIOHeader* mainHeader, const CMedIOHeader* subHeader=NULL);

    // accessor Methods
    const char* magic_Number(void) const;
    const char* original_File_Name(void) const;
    short sw_Version(void) const;
    short system_Type(void) const;
    File_Type file_Type(void)  const;
    const char* serial_Number(void) const;
    time_t scan_Start_Time(void) const;
    const char* isotope_Name(void) const;
    float isotope_Halflife(void) const;
    const char* radiopharmaceutical(void) const;
    float gantry_Tilt(void) const;
    float gantry_Rotation(void) const;
    float bed_Elevation(void) const;
    float intrinsic_Tilt(void) const;
    short wobble_Speed(void) const;
    Transm_Source_Type transm_Source_Type(void) const;
    float distance_Scanned(void) const;
    float transaxial_FOV(void) const;
    Angular_Compression angular_Compression(void) const;
    Coin_Samp_Mode coin_Samp_Mode(void) const;
    Axial_Samp_Mode axial_Samp_Mode(void) const;
    float calibration_Factor(void) const;
    Calibration_Units calibration_Units(void) const;
    Calibration_Units_Label calibration_Units_Label(void) const;
    Compression_Code compression_Code(void) const;
    const char* study_Type(void) const;
    const char* patient_ID(void) const;
    const char* patient_Name(void) const;
    Patient_Sex patient_Sex(void) const;
    Patient_Dexterity patient_Dexterity(void) const;
    float patient_Age(void) const;
    float patient_Height(void) const;
    float patient_Weight(void) const;
    time_t patient_Birth_Date(void) const;
    const char* physician_Name(void) const;
    const char* operator_Name(void) const;
    const char* study_Description(void) const;
    Acquisition_Type acquisition_Type(void) const;
    Patient_Orientation patient_Orientation(void) const;
    const char* facility_Name(void) const;
    short num_Planes(void) const;
    short num_Frames(void) const;
    short num_Gates(void) const;
    short num_Bed_Pos(void) const;
    float init_Bed_Position(void) const;
    float bed_Offset(const short i) const;
    float plane_Separation(void) const;
    short lwr_Sctr_Thres(void) const;
    short lwr_True_Thres(void) const;
    short upr_True_Thres(void) const;
    const char* user_Process_Code(void) const;
    Acquisition_Mode acquisition_Mode(void) const;
    float bin_Size(void) const;
    float branching_Fraction(void) const;
    time_t dose_Start_Time(void) const;
    float dosage(void) const;
    float well_Counter_Corr_Factor(void) const;
    const char* data_Units(void) const;
    Septa_State septa_State(void) const;
    short cti_Reserved(const short i) const;
    
    // mutator methods
    void setMagic_Number(const char* mn);
    void setOriginal_File_Name(const char* name);
    void setSW_Version(const short ver);
    void setSystem_Type(const short type);
    void setFileType(CECATMainHeader::Type fType);
    void setFile_Type(const File_Type fType);
    void setSerial_Number(const char* num);
    void setScan_Start_Time(const time_t time);
    void setIsotope_Name(const char* name);
    void setIsotope_Halflife(const float hlf);
    void setRadiopharmaceutical(const char* str);
    void setGantry_Tilt(const float tilt);
    void setGantry_Rotation(const float rot);
    void setBed_Elevation(const float elev);
    void setIntrinsic_Tilt(const float tilt);
    void setWobble_Speed(const short speed);
    void setTransm_Source_Type(const Transm_Source_Type type);
    void setDistance_Scanned(const float dist);
    void setTransaxial_FOV(const float fov);
    void setAngular_Compression(const Angular_Compression comp);
    void setCoin_Samp_Mode(const Coin_Samp_Mode mode);
    void setAxial_Samp_Mode(const Axial_Samp_Mode mode);
    void setCalibration_Factor(const float factor);
    void setCalibration_Units(const Calibration_Units unit);
    void setCalibration_Units_Label(const Calibration_Units_Label label);
    void setCompression_Code(const Compression_Code code);
    void setStudy_Type(const char* type);
    void setPatient_ID(const char* id);
    void setPatient_Name(const char* name);
    void setPatient_Sex(const Patient_Sex sex);
    void setPatient_Dexterity(const Patient_Dexterity dex);
    void setPatient_Age(const float age);
    void setPatient_Height(const float height);
    void setPatient_Weight(const float weight);
    void setPatient_Birth_Date(const time_t date);
    void setPhysician_Name(const char* name);
    void setOperator_Name(const char* name);
    void setStudy_Description(const char* descr);
    void setAcquisition_Type(const Acquisition_Type type);
    void setPatient_Orientation(const Patient_Orientation orient);
    void setFacility_Name(const char* name);
    void setNum_Planes(const short num);
    void setNum_Frames(const short num);
    void setNum_Gates(const short num);
    void setNum_Bed_Pos(const short num);
    void setInit_Bed_Position(const float pos);
    void setBed_Offset(const short i, const float offset);
    void setPlane_Separation(const float sep);
    void setLwr_Sctr_Thres(const short thres);
    void setLwr_True_Thres(const short thres);
    void setUpr_True_Thres(const short thres);
    void setUser_Process_Code(const char* code);
    void setAcquisition_Mode(const Acquisition_Mode mode);
    void setBin_Size(const float size);
    void setBranching_Fraction(const float frac);
    void setDose_Start_Time(const time_t time);
    void setDosage(const float dosage);
    void setWell_Counter_Corr_Factor(const float fac);
    void setData_Units(const char* units);
    void setSepta_State(const Septa_State state);
    void setCTI_Reserved(const short i, const short val);

    // special Qt-based methods for easy time conversion of the really
    // mad ECAT time specifications
    QDate patient_Birth_Date_Qt(void) const;
    QDateTime scan_Start_Time_Qt(void) const;
    QDateTime dose_Start_Time_Qt(void) const;
    void setPatient_Birth_Date_Qt(const QDate& date);
    void setScan_Start_Time_Qt(const QDateTime& dateTime);
    void setDose_Start_Time_Qt(const QDateTime& dateTime);

  private:
    CECAT7MainHeaderPrivate*  m_pData;
};

#endif // CECAT7MAINHEADER_H
