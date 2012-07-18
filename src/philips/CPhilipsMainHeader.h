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
    // enums for hw_config bit fields
    enum HW_Flags
    {
      MCT    = (1 <<  0),
      PAC    = (1 <<  1),
      PPU    = (1 <<  2),
      MCS    = (1 <<  3),
      PPU3   = (1 <<  4),
      CRB    = (1 <<  5),
      TFE    = (1 <<  6),
      DP_RAM = (1 <<  7),
      MCC    = (1 <<  8),
      SRC    = (1 <<  9),
      PETCT  = (1 << 10),
      PHYSIO = (1 << 11),
      PETMR  = (1 << 12)
    };

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
      Undefined_Orientation_hf,
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

    // public enumeration types
    enum Route_Type
    {
      UndefinedRoute,
      Intravenous,
      Intra_Arterial,
      Oral,
      By_Inhalation,
      Intramuscular,
      Subcutaneous,
      Intracutaneous,
      Intraperitoneal,
      Intramedullary,
      Intrathecal,
      Intra_Articular,
      Intraepithelial,
      Topical,
      Transluminal,
      Intraluminal,
      Extraluminal,
      Per_Rectum,
      Vaginal_Route,
      OtherRoute
    };

    enum Pharm_Type
    {
      UndefinedPharm,
      Acetate_C11,
      Ammonia_N13,
      Carbon_Dioxide_015,
      Carbon_Monoxide_C11,
      Carbon_Monoxide_015,
      Carfentanil_C11,
      Fluorodeoxyglucose_F18,
      Fluoro_L_Dopa_F18,
      Germanium_GE68,
      Glutamate_N13,
      Methionine_C11,
      Oxygen_015,
      Oxygen_Water_015,
      Palmitate_C11,
      Raclopride_C11,
      Rubidium_Chloride_RB82,
      Sodium_Fluoride_F18,
      Sodium_NA22,
      Spiperone_F18,
      Thymidine_F18,
      OtherPharm,
      Atsm_CU64,
      Butanol_O15,
      Edta_GA68,
      Flumazenil_C11,
      Flumazenil_F18,
      Fluoroethyltyrosin_F18,
      Fluoromisonidazole_F18,
      Fluoromethane_F18,
      Fluorouracil_F18,
      Fluorobenzothiazole_F18,
      Mespiperone_C11,
      Monoclonal_Antibody_I124,
      Ptsm_CU62,
      Sodium_Iodide_I124
    };

    enum Respiration_Trigger_Location
    {
      UnknownRespirationTriggerLocation,
      Max_Inhalation,
      Max_Exhalation,
      User_Defined
    };

    enum Card_Arrhythmia_Rej_Tech
    {
      NoRejection  = 0,
      RR_Interval  = 0x1,
      QRS_Loop     = 0x2,
      PVC_Criteria = 0x4
    };

    enum Window_Units
    {
      UndefinedWindowUnits = 0,
      Counts,
      Bqml,
      Percent_Of_Max,
      Suv,
      Hounsfield
    };

    enum Card_Ph_State
    {
      UndefinedCard_Ph_State = 0,
      Resting,
      Stress,
      Reinj,
      Redist,
      Delayed_Redist
    };

    enum Card_Fr_Type
    {
      UndefinedCard_Fr_Type = 0,
      Forward,
      Backward,
      Percent
    };

    enum Card_Slc_Dir
    {
      Undefined_Card_Slc_Dir = 0,
      Base_To_Apex,
      Apex_To_Base
    };

    enum Mashing_Type
    {
      ListmodeMashing = -1,
      UndefinedMashing = 0,
      NoMashing,
      Mashing2
    };

    enum Fltr_Type
    {
      Undefined_Fltr = 0,
      No_Fltr,
      Ramp_Fltr,
      Hanning_Fltr,
      Gaussian_Fltr,
      Butterworth_Fltr
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

    CMedIOHeader::Format headerFormat() const;

    // clone methods
    virtual bool convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader* pHead2 = NULL);
    virtual CMedIOHeader* clone() const;

    // data acess methods
    short file_Fmt() const;
    short scan_Geom() const;
    short hw_Config() const;
    short edit_Flag() const;
    File_Type filtyp() const;
    long minTransXtalDiff() const;
    float tofTstampScale() const;
    short duratn() const;
    Subheader_Type shdtyp() const;
    float pscale() const;
    float detectorRadius() const;
    bool virtualXtal() const;
    Mashing_Type phiMashing() const;
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
    // extended header functions
    const char* Dpat_name() const;
    const char* Dpat_id() const;
    const char* study_uid() const;
    const char* series_uid() const;
    const char* view_code() const;
    const char* sortproto_name() const;
    Route_Type route() const;
    Pharm_Type pharm() const;
    const char* req_phys() const;
    Card_Ph_State card_phstate() const;
    int assay_date() const;
    long assay_time() const;
    const char* series_desc() const;
    short height() const;
    short abundance() const;
    short realignment_xOffset() const;
    short realignment_yOffset() const;
    short realignment_horizRotation() const;
    time_t acq_date_time() const;
    time_t study_date_time() const;
    time_t injection_date_time() const;
    time_t file_create_date_time() const;
    Respiration_Trigger_Location resp_trig_loc() const;
    Card_Arrhythmia_Rej_Tech card_arrhythmia_rej_tech() const;
    float window_center() const;
    float window_width() const;
    short realignment_axialRotation() const;
    short realignment_vertRotation() const;
    short resp_trig_threshold() const;
    short resp_phase_duration() const;
    short resp_phase_offset() const;
    short realignment_zOffset() const;
    Window_Units window_units() const;
    const char* referring_physician() const;
    const char* study_id() const;
    float Dslice_thick() const;
    char sex() const;
    float table_height() const;
    bool card_bt_rej() const;
    Card_Fr_Type card_fr_type() const;
    const char* Dmanufacture_model_name() const;
    const char* Dimage_type() const;
    float min_bed_pos() const;
    float max_bed_pos() const;
    bool der_filled() const;
    long series_number() const;
    Card_Slc_Dir card_slc_dir() const;
    int card_skip_msec() const;
    int card_skip_counts() const;
    int card_dur_msec() const;
    int card_dur_counts() const;
    int card_beats_tot() const;
    int card_beats_acc() const;
    short card_skip_beats() const;
    short pvc_threshold() const;
    const char* radiopharm_name() const;
    const char* Dserial_number() const;
    const char* attncor_label() const;
    const char* contr_bolus_agent() const;
    const char* sop_uid() const;
    const char* frame_ref_uid() const;
    const char* pps_file() const;
    const char* worklist_file() const;
    Fltr_Type fltr_type() const;
    short decay_corr() const;

    // methods to modify elements of the MainHeader  
    void setFile_Fmt(const short format);
    void setScan_Geom(const short geometry);
    void setHw_Config(const short config);
    void setEdit_Flag(const short eFlag);
    void setFiltyp(const File_Type fType);
    void setMinTransXtalDiff(const long minTransXtalDiff);
    void setTofTstampScale(const float tofTstampScale);
    void setDuratn(const short seconds);
    void setShdtyp(const Subheader_Type sType);
    void setPscale(const float preScale);
    void setDetectorRadius(const float radius);
    void setVirtualXtal(const bool virtualCrystals);
    void setPhiMashing(const Mashing_Type phiMashing);
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
    // extended header functions
    void setDpat_name(const char* str);
    void setDpat_id(const char* str);
    void setStudy_uid(const char* str);
    void setSeries_uid(const char* str);
    void setView_code(const char* str);
    void setSortproto_name(const char* str);
    void setRoute(const Route_Type route);
    void setPharm(const Pharm_Type pharm);
    void setReq_phys(const char* str);
    void setCard_phstate(const Card_Ph_State state);
    void setAssay_date(const int assay_date);
    void setAssay_time(const long assay_time);
    void setSeries_desc(const char* str);
    void setHeight(const short height);
    void setAbundance(const short abundance);
    void setRealignment_xOffset(const short xOffset);
    void setRealignment_yOffset(const short yOffset);
    void setRealignment_horizRotation(const short horizRotation);
    void setAcq_date_time(const time_t date_time);
    void setStudy_date_time(const time_t date_time);
    void setInjection_date_time(const time_t date_time);
    void setFile_create_date_time(const time_t date_time);
    void setResp_trig_loc(const Respiration_Trigger_Location location);
    void setCard_arrhythmia_rej_tech(const Card_Arrhythmia_Rej_Tech rej_tech);
    void setWindow_center(const float window_center);
    void setWindow_width(const float window_width);
    void setRealignment_axialRotation(const short axialRotation);
    void setRealignment_vertRotation(const short vertRotation);
    void setResp_trig_threshold(const short resp_trig_threshold);
    void setResp_phase_duration(const short resp_phase_duration);
    void setResp_phase_offset(const short resp_phase_offset);
    void setRealignment_zOffset(const short zOffset);
    void setWindow_units(const Window_Units window_units);
    void setReferring_physician(const char* str);
    void setStudy_id(const char* str);
    void setDslice_thick(const float Dslice_thick);
    void setSex(const char sex);
    void setTable_height(const float table_height);
    void setCard_bt_rej(const bool flag);
    void setCard_fr_type(const Card_Fr_Type type);
    void setDmanufacture_model_name(const char* str);
    void setDimage_type(const char* str);
    void setMin_bed_pos(const float min);
    void setMax_bed_pos(const float max);
    void setDer_filled(const bool flag);
    void setSeries_number(const long number);
    void setCard_slc_dir(const Card_Slc_Dir direction);
    void setCard_skip_msec(const int msec);
    void setCard_skip_counts(const int counts);
    void setCard_dur_msec(const int msec);
    void setCard_dur_counts(const int counts);
    void setCard_beats_tot(const int beats);
    void setCard_beats_acc(const int acc);
    void setCard_skip_beats(const short beats);
    void setPvc_threshold(const short threshold);
    void setRadiopharm_name(const char* str);
    void setDserial_number(const char* str);
    void setAttncor_label(const char* str);
    void setContr_bolus_agent(const char* str);
    void setSop_uid(const char* str);
    void setFrame_ref_uid(const char* str);
    void setPps_file(const char* str);
    void setWorklist_file(const char* str);
    void setFltr_type(const Fltr_Type type);
    void setDecay_corr(const short decay);

    // special Qt-based methods
    QDate patient_Birth_Date_Qt() const;
    void setPatient_Birth_Date_Qt(const QDate& date);

  private:
    CPhilipsMainHeaderPrivate* m_pData;
};

#endif // CPHILIPSMAINHEADER_H
