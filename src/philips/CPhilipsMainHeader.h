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

#ifndef CPHILIPSMAINHEADER_H
#define CPHILIPSMAINHEADER_H

#include <QTextStream>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOHeader>
#else
#include <CMedIOHeader.h>
#endif

// forward declarations
class CPhilipsMainHeaderPrivate;
class CPhilipsFile;

class CPhilipsMainHeader : public CMedIOHeader
{
  public:
    // possible Philips file types
    enum File_Type {Unknown = 0, Sinogram, Image, VID, OtherFile, Syntegra, Listmode, Rawmode};

    enum Subheader_Type {UCLA_CTI = 0, UGM};
    enum RPhi_Type {Real = 0, Crystal, CrystalProjection};
    enum Slice_Type {Milimeters = 0, CrystalSum, Encoded, ZA_Transmission, SNGANG_Transmission};
    enum Delay_Type {None = 0, Rebin, Substact, SubstactSmoothed};
    enum Scan_Type {Test = 0, Patient};
    enum Acquisition_Type {UndefinedAcq = 0, Blank, Transmission, Emission, Normalization,
                           Brain_ObliqueReslicing, Cardiac3D_BullsEye, Cardiac_ObliqueReslicing,
                           IR, Singles_EC, IRC, CT, CT_IR, MR, MR_IR, SAGI, CORO};
    enum Isotop {UndefinedIsotop = 0, F18, O15, C11, GA68, N13, RB82, CU62, CS137,
                 GE68, OtherIsotop, UnknownIsotop, CU64, BR76, NA22, O14, Y86, ZN62,
                 CU60, CU61, GA66, BR75, BR77, I124, K38, MN52, TC94M, TI45 };

    enum Patient_Orientation_hf
    {
      UndefinedOrientation_hf,
      Head_First,       /* Top of patient head is closest to scanner
                         * slice #0 while torso os along direction of
                         * increasing scanner slice # */
 
      Feet_First        /* Top of patient head is closest to scanner
                         * slice #128 while torso is along direction of
                         * decreasing scanner slice # */
    };
  
    enum Patient_Orientation_ps
    {
      Undefined_Orientation_ps,
      Prone,            /* patient is face down */
      Supine,           /* patient is on back */
      Left,             /* patient is on lying on his left side */
      Right             /* patient is on lying on his right side */
    };
  
    enum Table_Direction
    {
      UnknownDirection,
      In, // Table moving in (from the front to the back of the scanner)
      Out // Table moving out (from the back to the front of the scanner) 
    };

    enum Valid_Header_Struct
    {
      Header_Struct_Invalid = 0,
      Header_Struct_Valid = 1,
      Header_Struct_Unknown = 2,
      Header_Struct_Realigned
    };

    enum Acquisition_Protocol_Type
    {
      Undefined_Acquisition_Protocol = 0,
      Static_Emission,
      Dynamic_Emission,
      Static_Transmission_Only,
      Gated_Cardiac,
      Whole_Body_Emission,
      Whole_Body_Transmission_only,
      Not_Used,
      Singles
    };

    enum Rebin_Type
    {
      Single_Slice = 0,  /* single slice     */
      Multi_Slice,       /* multi slice      */
      Multi_Tilt_4D,     /* 4-D (multi-tilt) */
      Fourier            /* Fourier rebin    */
    };

    // constructors
    CPhilipsMainHeader(CPhilipsFile* file,
                       CPhilipsMainHeader::File_Type = CPhilipsMainHeader::Unknown);
    ~CPhilipsMainHeader();

    // copy constructor and default assignment operator
    CPhilipsMainHeader(const CPhilipsMainHeader& src);
    CPhilipsMainHeader& operator=(const CPhilipsMainHeader& src);

    // header clear method
    void clear();

    // file i/o methods
    bool load();
    bool save() const;

    // the number of bytes the data of that header requires on disk
    int rawDataSize() const;

    CMedIOHeader::Format headerFormat() const;

    // clone methods
    virtual bool convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader* pHead2 = NULL);
    virtual CMedIOHeader* clone() const;

    bool isPETMR() const;

    short file_Format() const;
    short scanner_Geometry() const;
    short hardware_Config() const;
    short edit_Flag() const;
    File_Type file_Type() const;
    short day_Created() const;
    short month_Created() const;
    short year_Created() const;
    short hour_Created() const;
    short minute_Created() const;
    short second_Created() const;
    short scan_Duration() const;
    Subheader_Type subheader_Type() const;
    short singles_PreScale_Old() const;
    float singles_PreScale() const;
    float detector_Radius() const;
    bool virtual_Crystals() const;
    short phi_Mashing() const;
    short polygon_Sides() const;
    short crystals_Per_Side() const;
    short crystal_Rows() const;
    float crystal_Thickness() const;
    float x_CrystalPitch() const;
    float z_CrystalPitch() const;
    float axial_FOV() const;
    RPhi_Type rPhi_Type() const;
    Slice_Type slice_Type() const;
    Delay_Type delay_Type() const;
    Scan_Type scan_Type() const;
    Acquisition_Type acquisition_Type() const;
    short num_Ray() const;
    short num_Ang() const;
    short slice_Thickness() const;
    Isotop isotop() const;
    float slope() const;
    float calibration_Intercept() const;
    short injection_Time() const;
    float polygon_VertexAt0Deg() const;
    short num_Slices() const;
    short num_Frames() const;
    short birthdate_Day() const;
    short birthdate_Month() const;
    short birthdate_Year() const;
    const char* short_Patient_ID() const;
    short num_Tilts() const;

    void setFile_Format(const short format);
    void setScanner_Geometry(const short geometry);
    void setHardware_Config(const short conifg);
    void setEdit_Flag(const short eFlag);
    void setFile_Type(const File_Type fType);
    void setDay_Created(const short day);
    void setMonth_Created(const short month);
    void setYear_Created(const short year);
    void setHour_Created(const short hour);
    void setMinute_Created(const short minute);
    void setSecond_Created(const short second);
    void setScan_Duration(const short seconds);
    void setSubheader_Type(const Subheader_Type sType);
    void setSingles_PreScale_Old(const short preScale);
    void setSingles_PreScale(const float preScale);
    void setDetector_Radius(const float radius);
    void setVirtual_Crystals(const bool virtualCrystals);
    void setPhi_Mashing(const short phiMashing);
    void setPolygon_Sides(const short polygonSides);
    void setCrystals_Per_Side(const short crystalsPerSide);
    void setCrystal_Rows(const short crystalRows);
    void setCrystal_Thickness(const float crystalThickness);
    void setX_CrystalPitch(const float pitch);
    void setZ_CrystalPitch(const float pitch);
    void setAxial_FOV(const float axialFOV);
    void setRPhi_Type(const RPhi_Type rType);
    void setSlice_Type(const Slice_Type sType);
    void setDelay_Type(const Delay_Type dType);
    void setScan_Type(const Scan_Type sType);
    void setAcquisition_Type(const Acquisition_Type aType);
    void setNum_Ray(const short num);
    void setNum_Ang(const short num);
    void setSlice_Thickness(const short thickness);
    void setIsotop(const Isotop isotop);
    void setSlope(const float slope);
    void setCalibration_Intercept(const float intercept);
    void setInjection_Time(const short seconds);
    void setPolygon_VertexAt0Deg(const float vertex);
    void setNum_Slices(const short num);
    void setNum_Frames(const short num);
    void setBirthdate_Day(const short day);
    void setBirthdate_Month(const short month);
    void setBirthdate_Year(const short year);
    void setShort_Patient_ID(const char* id);
    void setNum_Tilts(const short num);

  private:
    CPhilipsMainHeaderPrivate* m_pData;
};

#endif // CPHILIPSMAINHEADER_H
