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

#include <time.h>

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

    // data acess methods
    bool isPETMR() const;
    short file_Fmt() const;
    short scan_Geom() const;
    short hw_Config() const;
    short edit_Flag() const;
    File_Type filtyp() const;
    short daycre() const;
    short mocre() const;
    short yrcre() const;
    short hrcre() const;
    short mincre() const;
    short seccre() const;
    short duratn() const;
    Subheader_Type shdtyp() const;
    float pscale() const;
    float detectorRadius() const;
    bool virtualXtal() const;
    short phiMashing() const;
    short polygonSides() const;
    short xtalsPerSide() const;
    short nXtalRows() const;
    float crystalThickness() const;
    float xXtalPitch() const;
    float zXtalPitch() const;
    float axialFOV() const;
    RPhi_Type rphiType() const;
    Slice_Type sliceType() const;
    Delay_Type delayType() const;
    Scan_Type pattyp() const;
    Acquisition_Type scntyp() const;
    short numray() const;
    short numang() const;
    short slcthk() const;
    Isotop isotop() const;
    float slope() const;
    float intcpt() const;
    short injtim() const;
    float polygonVertAt0deg() const;
    short nslice() const;
    short nframe() const;
    short bthday() const;
    short bthmo() const;
    short bthyr() const;
    const char* ssn() const;
    short ntilt() const;
    short petnum() const;
    float activity() const;
    long weight() const;
    short hrinj() const;
    short mininj() const;
    float srcRadius() const;
    float srcZpos() const;
    float halfLife() const;
    float concfac() const;
    float concfac_Bgsub() const;
    float dmax() const;
    float dline() const;
    float angmax() const;
    float x0() const;
    float y0() const;
    float z0() const;
    float nevent() const;
    float nsino() const;
    short eglob_Low() const;
    short eglob_Up() const;
    short eloc_Low() const;
    short eloc_Up() const;
    Patient_Orientation_hf orient_Hf() const;
    const char* scan_Swrel() const;
    Table_Direction tbl_Direction() const;
    Patient_Orientation_ps orient_Ps() const;
    float frontLeadDiameter() const;
    float backLeadDiameter() const;
    float leadSeparation() const;
    float ndelays() const;
    float slcsep() const;
    const char* fctrfil() const;
    const char* baselin() const;
    const char* dstpkfl() const;
    const char* aqprotocol_Name() const;
    Acquisition_Protocol_Type aqprotocol_Type() const;
    const char* patient_Name() const;
    float reslice_Ang1() const;
    float reslice_Ang2() const;
    float reslice_Ang3() const;
    short minslc() const;
    short maxslc() const;
    short minfrm() const;
    short maxfrm() const;
    short scanner_Maxslice() const;
    Rebin_Type rebin_Type() const;
    const char* scnOrigin() const;
    const char* accNum() const;
    short movementCoinc() const;
    short movementSing() const;
    short crbTstampPeriod() const;
    short trailexists() const;
    unsigned long trailbeg() const;
    Valid_Header_Struct petct_Valid() const;
    short petct_separation() const;
    short petct_landmark() const;
    time_t petct_alignment_timestamp() const;
    short petct_alignment_zOffset() const;
    short petct_alignment_xShift() const;
    short petct_alignment_yShift() const;
    short petct_alignment_zShift() const;
    short petct_alignment_acFlags() const;
    short petct_alignment_xOffset() const;
    short petct_alignment_yOffset() const;
    short petct_alignment_axialRotation() const;
    short petct_alignment_horizRotation() const;
    short petct_alignment_vertRotation() const;

    // methods to modify elements of the MainHeader  
    void setFile_Fmt(const short format);
    void setScan_Geom(const short geometry);
    void setHw_Config(const short config);
    void setEdit_Flag(const short eFlag);
    void setFiltyp(const File_Type fType);
    void setDaycre(const short day);
    void setMocre(const short month);
    void setYrcre(const short year);
    void setHrcre(const short hour);
    void setMincre(const short minute);
    void setSeccre(const short second);
    void setDuratn(const short seconds);
    void setShdtyp(const Subheader_Type sType);
    void setPscale(const float preScale);
    void setDetectorRadius(const float radius);
    void setVirtualXtal(const bool virtualCrystals);
    void setPhiMashing(const short phiMashing);
    void setPolygonSides(const short polygonSides);
    void setXtalsPerSide(const short crystalsPerSide);
    void setnXtalRows(const short crystalRows);
    void setCrystalThickness(const float crystalThickness);
    void setXXtalPitch(const float pitch);
    void setZXtalPitch(const float pitch);
    void setAxialFOV(const float axialFOV);
    void setRphiType(const RPhi_Type rType);
    void setSliceType(const Slice_Type sType);
    void setDelayType(const Delay_Type dType);
    void setPattyp(const Scan_Type sType);
    void setScntyp(const Acquisition_Type aType);
    void setNumray(const short num);
    void setNumang(const short num);
    void setSlcthk(const short thickness);
    void setIsotop(const Isotop isotop);
    void setSlope(const float slope);
    void setIntcpt(const float intercept);
    void setInjtim(const short seconds);
    void setPolygonVertAt0deg(const float vertex);
    void setNslice(const short num);
    void setNframe(const short num);
    void setBthday(const short day);
    void setBthmo(const short month);
    void setBthyr(const short year);
    void setSsn(const char* id);
    void setNtilt(const short num);
    void setPetnum(const short petnum);
    void setActivity(const float activity);
    void setWeight(const long weight);
    void setHrinj(const short hrinj);
    void setMininj(const short mininj);
    void setSrcRadius(const float srcRadius);
    void setSrcZpos(const float srcZpos);
    void setHalfLife(const float halfLife);
    void setConcfac(const float concfac);
    void setConcfac_Bgsub(const float concfac_bgsub);
    void setDmax(const float dmax);
    void setDline(const float dline);
    void setAngmax(const float angmax);
    void setX0(const float x0);
    void setY0(const float y0);
    void setZ0(const float z0);
    void setNevent(const float nevent);
    void setNsino(const float nsino);
    void setEglob_Low(const short eglob_low);
    void setEglob_Up(const short eglob_up);
    void setEloc_Low(const short eloc_low);
    void setEloc_Up(const short eloc_up);
    void setOrient_Hf(const Patient_Orientation_hf orient_hf);
    void scan_Swrel(const char* str);
    void setTbl_Direction(const Table_Direction tbl_direction);
    void setOrient_Ps(const Patient_Orientation_ps orient_ps);
    void setFrontLeadDiameter(const float frontLeadDiameter);
    void setBackLeadDiameter(const float backLeadDiameter);
    void setLeadSeparation(const float leadSeparation);
    void setNdelays(const float ndelays);
    void setSlcsep(const float slcsep);
    void setFctrfil(const char* str);
    void setBaselin(const char* str);
    void setDstpkfl(const char* str);
    void setAqprotocol_Name(const char* str);
    void setAqprotocol_Type(const Acquisition_Protocol_Type aqprotocol_type);
    void setPatient_Name(const char* str);
    void setReslice_Ang1(const float reslice_ang1);
    void setReslice_Ang2(const float reslice_ang2);
    void setReslice_Ang3(const float reslice_ang3);
    void setMinslc(const short minslc);
    void setMaxslc(const short maxslc);
    void setMinfrm(const short minfrm);
    void setMaxfrm(const short maxfrm);
    void setScanner_Maxslice(const short scanner_maxslice);
    void setRebin_Type(const Rebin_Type rebin_type);
    void setScnOrigin(const char *str);
    void setAccNum(const char *str);
    void setMovementCoinc(const short movementCoinc);
    void setMovementSing(const short movementSing);
    void setCrbTstampPeriod(const short crbTstampPeriod);
    void setTrailexists(const short trailexists);
    void setTrailbeg(const unsigned long trailbeg);
    void setPetct_Valid(const Valid_Header_Struct petct_valid);
    void setPetct_separation(const short separation);
    void setPetct_landmark(const short landmark);
    void setPetct_alignment_timestamp(const time_t timestamp);
    void setPetct_alignment_zOffset(const short zOffset);
    void setPetct_alignment_xShift(const short xShift);
    void setPetct_alignment_yShift(const short yShift);
    void setPetct_alignment_zShift(const short zShift);
    void setPetct_alignment_acFlags(const short acFlags);
    void setPetct_alignment_xOffset(const short xOffset);
    void setPetct_alignment_yOffset(const short yOffset);
    void setPetct_alignment_axialRotation(const short axialRotation);
    void setPetct_alignment_horizRotation(const short horizRotation);
    void setPetct_alignment_vertRotation(const short vertRotation);

  private:
    CPhilipsMainHeaderPrivate* m_pData;
};

#endif // CPHILIPSMAINHEADER_H
