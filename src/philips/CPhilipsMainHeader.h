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

#ifndef __MEDIO_PRIVATE__
#include <CMedIOHeader>
#else
#include <CMedIOHeader.h>
#endif

#include <QDateTime>
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
    enum Delay_Type {None = 0, Rebin, Substract, SubstractSmoothed};
    enum Scan_Type {Test = 0, Patient};
    enum Acquisition_Type {UndefinedAcq = 0, Blank, Transmission, Emission, Normalization,
                           Brain_ObliqueReslicing, Cardiac3D_BullsEye, Cardiac_ObliqueReslicing,
                           IR, Singles_EC, IRC, CT, CT_IR, MR, MR_IR, SAGI, CORO};
    enum Isotope {UndefinedIsotope = 0, F18, O15, C11, GA68, N13, RB82, CU62, CS137,
                 GE68, OtherIsotope, UnknownIsotope, CU64, BR76, NA22, O14, Y86, ZN62,
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

    enum PETCT_Valid_Type
    {
      PETCT_Invalid = 0,
      PETCT_Valid,
      PETCT_Unknown,
      PETCT_Realigned
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
      Singles,
      Gated_Respiratory,
      Gated_Cardiac_And_Respiratory
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
      Cardiac_Stress,
      Reinjection,
      Redistribution,
      Delayed_Redistribution
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

    enum AttnCorr_Type
    {
      Undefined_AttnCorr = 0,
      None_AttnCorr,
      Transmission_AttnCorr,
      Ellipse_AttnCorr,
      ROI_AttnCorr,
      Reproject_AttnCorr,
      CTAC_AttnCorr
    };

    enum AttnCorrEcc_Type
    {
      Undefined_AttnCorrEcc = 0,
      None_AttnCorrEcc,
      Measured_AttnCorrEcc,
      Estimated_AttnCorrEcc
    };

    enum Norm_Type
    {
      Undefined_Norm = 0,
      None_Norm,
      Axial_Norm,
      Efficiency_Norm,
      AxialEfficiency_Norm
    };

    enum Alg_Type
    {
      Undefined_Alg = 0,
      FBP_Alg,
      OSEM_Alg,
      RAMLA_Alg,
      RAMLA3D_Alg
    };

    enum PreFlt_Type
    {
      Undefined_PreFlt = 0,
      None_PreFlt,
      Wiener_PreFlt,
      Metz_PreFlt
    };

    enum PostFlt_Type
    {
      Undefined_PostFlt = 0,
      None_PostFlt,
      Wiener_PostFlt,
      PET3D_PostFlt
    };

    enum PostProc_Type
    {
      Undefined_PostProc = 0,
      None_PostProc,
      Segmentation_PostProc,
      Remapping_PostProc
    };

    enum AlgTr_Type
    {
      Undefined_AlgTr = 0,
      FBP_AlgTr,
      OSEM_AlgTr
    };

    // constructors
    CPhilipsMainHeader(CPhilipsFile* file,
                       CPhilipsMainHeader::File_Type = CPhilipsMainHeader::Unknown,
                       int mainHeaderPosition = 0);
    ~CPhilipsMainHeader();

    // copy constructor and default assignment operator
    CPhilipsMainHeader(const CPhilipsMainHeader& src);
    CPhilipsMainHeader& operator=(const CPhilipsMainHeader& src);

    // header clear method
    void clear();

    // file i/o methods
    bool load();
    bool save() const;

    // runtime type information
    CMedIOHeader::Format headerFormat() const;

    // clone methods
    virtual CMedIOHeader* clone() const;

    // convert methods
    bool convertFrom(const CMedIOHeader* mainHeader, const CMedIOHeader* subHeader=NULL);

    // data acess methods
    short file_fmt() const;
    short scan_geom() const;
    short hw_config() const;
    short edit_flag() const;
    File_Type filtyp() const;
    int minTransXtalDiff() const;
    float tofTstampScale() const;
    short dep_daycre() const;
    short dep_mocre() const;
    short dep_yrcre() const;
    short dep_hrcre() const;
    short dep_mincre() const;
    short dep_seccre() const;
    short duratn() const;
    Subheader_Type shdtyp() const;
    short sngpscl() const;
    short singopt() const;
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
    float axialFov() const;
    RPhi_Type rphiType() const;
    Slice_Type sliceType() const;
    Delay_Type delayType() const;
    Scan_Type pattyp() const;
    Acquisition_Type scntyp() const;
    short numray() const;
    short numang() const;
    short slcthk() const;
    Isotope isotop() const;
    float slope() const;
    float intcpt() const;
    short dep_injtim() const;
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
    int weight() const;
    short dep_hrinj() const;
    short dep_mininj() const;
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
    short eglob_low() const;
    short eglob_up() const;
    short eloc_low() const;
    short eloc_up() const;
    Patient_Orientation_hf orient_hf() const;
    const char* scan_swrel() const;
    short petct_sepdist() const;
    short petct_landmrk() const;
    time_t petct_align_timestamp() const;
    Table_Direction tbl_direction() const;
    Patient_Orientation_ps orient_ps() const;
    short petct_align_zoffset() const;
    short petct_align_xshift() const;
    short petct_align_yshift() const;
    short petct_align_zshift() const;
    short petct_align_acqflgs() const;
    short petct_align_xoffset() const;
    short petct_align_yoffset() const;
    short petct_align_axrot() const;
    short petct_align_horzrot() const;
    short petct_align_vertrot() const;
    float frontLeadDiameter() const;
    float backLeadDiameter() const;
    float leadSeparation() const;
    float ndelays() const;
    float slcsep() const;
    PETCT_Valid_Type petct_valid() const;
    const char* fctrfil() const;
    const char* baselin() const;
    const char* dstpkfl() const;
    const char* aqprotocol_name() const;
    Acquisition_Protocol_Type aqprotocol_type() const;
    const char* patient_name() const;
    float reslice_ang1() const;
    float reslice_ang2() const;
    float reslice_ang3() const;
    short minslc() const;
    short maxslc() const;
    short minfrm() const;
    short maxfrm() const;
    short scanner_maxslice() const;
    Rebin_Type rebin_type() const;
    const char* scnOrigin() const;
    const char* accNum() const;
    short movementCoinc() const;
    short movementSing() const;
    short crbTstampPeriod() const;
    bool trailexists() const;
    unsigned int trailbeg() const;

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
    int assay_time() const;
    const char* series_desc() const;
    short height() const;
    short abundance() const;
    short petct_realign_x() const;
    short petct_realign_y() const;
    short petct_realign_hr() const;
    time_t acq_date_time() const;
    time_t study_date_time() const;
    time_t injection_date_time() const;
    time_t file_create_date_time() const;
    Respiration_Trigger_Location resp_trig_loc() const;
    Card_Arrhythmia_Rej_Tech card_arrhythmia_rej_tech() const;
    float window_center() const;
    float window_width() const;
    short petct_realign_zr() const;
    short petct_realign_vr() const;
    short resp_trig_threshold() const;
    short resp_phase_duration() const;
    short resp_phase_offset() const;
    short petct_realign_z() const;
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
    int series_number() const;
    int dep_study_date() const;
    int dep_study_time() const;
    int dep_acq_time() const;
    Card_Slc_Dir card_slc_dir() const;
    int card_skip_msec() const;
    int card_skip_counts() const;
    int card_dur_msec() const;
    int card_dur_counts() const;
    int card_beats_tot() const;
    int card_beats_acc() const;
    short card_skip_beats() const;
    short pvc_threshold() const;
    int dep_acq_date() const;
    const char* radiopharm_name() const;
    const char* Dserial_number() const;
    const char* attncor_label() const;
    const char* contr_bolus_agent() const;
    const char* sop_uid() const;
    const char* frame_ref_uid() const;
    const char* pps_file() const;
    const char* worklist_file() const;
    const char* recon_swrel() const;
    const char* analy_swrel() const;
    const char* recprotocol_name() const;
    const char* insinofile() const;
    bool slc_add() const;
    short slc_space() const;
    short slc_thick() const;
    bool frame_add() const;
    short frame_space() const;
    short frame_thick() const;
    Fltr_Type fltr_type() const;
    short smoth() const;
    short scatcorr_type() const;
    short edge_exp() const;
    short bckang_avg() const;
    float bck_coeff() const;
    short bck_wid() const;
    AttnCorr_Type attncor_type() const;
    AttnCorrEcc_Type attncor_ecc() const;
    float attn_coeff() const;
    const char* regfile() const;
    const char* proc_transinofile() const;
    float skull_comp() const;
    Norm_Type norm_type() const;
    bool smp_norm() const;
    const char* axnfile() const;
    const char* effnormfile() const;
    bool gap_comp() const;
    Alg_Type algtype_em() const;
    short num_iter() const;
    short iter_em() const;
    short subset_em() const;
    short nsmooth_em() const;
    short nrepeat_em() const;
    short bckslc_avg() const;
    bool dead_corr() const;
    bool decay_corr() const;
    const char* transinofile() const;
    const char* blnksinofile() const;
    float tran_ray_fwhm() const;
    float tran_axl_fwhm() const;
    short surv_mask() const;
    PreFlt_Type preflt_type() const;
    PostFlt_Type postflt_type() const;
    PostProc_Type tr_posttyp() const;
    AlgTr_Type algtype_tr() const;
    short iter_tr() const;
    short subset_tr() const;
    short nsmooth_tr() const;
    short nrepeat_tr() const;
    bool attn_corr_3d() const;
    short ramla_no_it() const;
    short ramla_sysac() const;
    float ramla_lambda(const short i) const;
    float ramla_blrad() const;
    float ramla_blalpha() const;
    float ramla_bcc_rsz() const;
    time_t recon_date_time() const;
    short gating_type() const;
    const char* ref_attncor_series_uid() const;
    const char* ref_gated_qc_image_inst_uid() const;
    const char* ref_raw_data_inst_uid() const;
    int start_table_pos_abs() const;
    int start_table_pos_rel() const;
    bool mr_valid() const;
    const char* coil_type() const;

    // methods to modify elements of the MainHeader  
    void setFile_fmt(const short format);
    void setScan_geom(const short geometry);
    void setHw_config(const short config);
    void setEdit_flag(const short eFlag);
    void setFiltyp(const File_Type fType);
    void setMinTransXtalDiff(const int minTransXtalDiff);
    void setTofTstampScale(const float tofTstampScale);
    void setDep_daycre(const short daycre);
    void setDep_mocre(const short mocre);
    void setDep_yrcre(const short moccre);
    void setDep_hrcre(const short hrcre);
    void setDep_mincre(const short hrcre);
    void setDep_seccre(const short hrcre);
    void setDuratn(const short seconds);
    void setShdtyp(const Subheader_Type sType);
    void setSngpscl(const short sngpscl);
    void setSingopt(const short singopt);
    void setPscale(const float preScale);
    void setDetectorRadius(const float radius);
    void setVirtualXtal(const bool virtualCrystals);
    void setPhiMashing(const Mashing_Type phiMashing);
    void setPolygonSides(const short polygonSides);
    void setXtalsPerSide(const short crystalsPerSide);
    void setNXtalRows(const short crystalRows);
    void setCrystalThickness(const float crystalThickness);
    void setXXtalPitch(const float pitch);
    void setZXtalPitch(const float pitch);
    void setAxialFov(const float axialFOV);
    void setRphiType(const RPhi_Type rType);
    void setSliceType(const Slice_Type sType);
    void setDelayType(const Delay_Type dType);
    void setPattyp(const Scan_Type sType);
    void setScntyp(const Acquisition_Type aType);
    void setNumray(const short num);
    void setNumang(const short num);
    void setSlcthk(const short thickness);
    void setIsotop(const Isotope isotop);
    void setSlope(const float slope);
    void setIntcpt(const float intercept);
    void setDep_injtim(const short seconds);
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
    void setWeight(const int weight);
    void setDep_hrinj(const short hrinj);
    void setDep_mininj(const short mininj);
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
    void setEglob_low(const short eglob_low);
    void setEglob_up(const short eglob_up);
    void setEloc_low(const short eloc_low);
    void setEloc_up(const short eloc_up);
    void setOrient_hf(const Patient_Orientation_hf orient_hf);
    void setScan_swrel(const char* str);
    void setPetct_sepdist(const short sepdist);
    void setPetct_landmrk(const short landmrk);
    void setPetct_align_timestamp(const time_t tstamp);
    void setTbl_direction(const Table_Direction tbl_direction);
    void setOrient_ps(const Patient_Orientation_ps orient_ps);
    void setPetct_align_zoffset(const short zoffset);
    void setPetct_align_xshift(const short xshift);
    void setPetct_align_yshift(const short yshift);
    void setPetct_align_zshift(const short zshift);
    void setPetct_align_acqflgs(const short acqflgs);
    void setPetct_align_xoffset(const short xoffset);
    void setPetct_align_yoffset(const short yoffset);
    void setPetct_align_axrot(const short axrot);
    void setPetct_align_horzrot(const short horzrot);
    void setPetct_align_vertrot(const short vertrot);
    void setFrontLeadDiameter(const float frontLeadDiameter);
    void setBackLeadDiameter(const float backLeadDiameter);
    void setLeadSeparation(const float leadSeparation);
    void setNdelays(const float ndelays);
    void setSlcsep(const float slcsep);
    void setPetct_valid(const PETCT_Valid_Type type);
    void setFctrfil(const char* str);
    void setBaselin(const char* str);
    void setDstpkfl(const char* str);
    void setAqprotocol_name(const char* str);
    void setAqprotocol_type(const Acquisition_Protocol_Type aqprotocol_type);
    void setPatient_name(const char* str);
    void setReslice_ang1(const float reslice_ang1);
    void setReslice_ang2(const float reslice_ang2);
    void setReslice_ang3(const float reslice_ang3);
    void setMinslc(const short minslc);
    void setMaxslc(const short maxslc);
    void setMinfrm(const short minfrm);
    void setMaxfrm(const short maxfrm);
    void setScanner_maxslice(const short scanner_maxslice);
    void setRebin_type(const Rebin_Type rebin_type);
    void setScnOrigin(const char *str);
    void setAccNum(const char *str);
    void setMovementCoinc(const short movementCoinc);
    void setMovementSing(const short movementSing);
    void setCrbTstampPeriod(const short crbTstampPeriod);
    void setTrailexists(const short trailexists);
    void setTrailbeg(const unsigned int trailbeg);

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
    void setAssay_time(const int assay_time);
    void setSeries_desc(const char* str);
    void setHeight(const short height);
    void setAbundance(const short abundance);
    void setPetct_realign_x(const short xOffset);
    void setPetct_realign_y(const short yOffset);
    void setPetct_realign_hr(const short horizRotation);
    void setAcq_date_time(const time_t date_time);
    void setStudy_date_time(const time_t date_time);
    void setInjection_date_time(const time_t date_time);
    void setFile_create_date_time(const time_t date_time);
    void setResp_trig_loc(const Respiration_Trigger_Location location);
    void setCard_arrhythmia_rej_tech(const Card_Arrhythmia_Rej_Tech rej_tech);
    void setWindow_center(const float window_center);
    void setWindow_width(const float window_width);
    void setPetct_realign_zr(const short axialRotation);
    void setPetct_realign_vr(const short vertRotation);
    void setResp_trig_threshold(const short resp_trig_threshold);
    void setResp_phase_duration(const short resp_phase_duration);
    void setResp_phase_offset(const short resp_phase_offset);
    void setPetct_realign_z(const short zOffset);
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
    void setSeries_number(const int number);
    void setDep_study_date(const int date);
    void setDep_study_time(const int time);
    void setDep_acq_time(const int time);
    void setCard_slc_dir(const Card_Slc_Dir direction);
    void setCard_skip_msec(const int msec);
    void setCard_skip_counts(const int counts);
    void setCard_dur_msec(const int msec);
    void setCard_dur_counts(const int counts);
    void setCard_beats_tot(const int beats);
    void setCard_beats_acc(const int acc);
    void setCard_skip_beats(const short beats);
    void setPvc_threshold(const short threshold);
    void setDep_acq_date(const int date);
    void setRadiopharm_name(const char* str);
    void setDserial_number(const char* str);
    void setAttncor_label(const char* str);
    void setContr_bolus_agent(const char* str);
    void setSop_uid(const char* str);
    void setFrame_ref_uid(const char* str);
    void setPps_file(const char* str);
    void setWorklist_file(const char* str);
    void setRecon_swrel(const char* str);
    void setAnaly_swrel(const char* str);
    void setRecprotocol_name(const char* str);
    void setInsinofile(const char* str);
    void setSlc_add(const bool add);
    void setSlc_space(const short space);
    void setSlc_thick(const short thick);
    void setFrame_add(const bool add);
    void setFrame_space(const short space);
    void setFrame_thick(const short thick);
    void setFltr_type(const Fltr_Type type);
    void setSmoth(const short smoth);
    void setScatcorr_type(const short type);
    void setEdge_exp(const short exp);
    void setBckang_avg(const short avg);
    void setBck_coeff(const float coeff);
    void setBck_wid(const short wid);
    void setAttncor_type(const AttnCorr_Type type);
    void setAttncor_ecc(const AttnCorrEcc_Type type);
    void setAttn_coeff(const float coeff);
    void setRegfile(const char* file);
    void setProc_transinofile(const char* file);
    void setSkull_comp(const float comp);
    void setNorm_type(const Norm_Type type);
    void setSmp_norm(const bool norm);
    void setAxnfile(const char* file);
    void setEffnormfile(const char* file);
    void setGap_comp(const bool norm);
    void setAlgtype_em(const Alg_Type type);
    void setNum_iter(const short num);
    void setIter_em(const short em);
    void setSubset_em(const short em);
    void setNsmooth_em(const short em);
    void setNrepeat_em(const short em);
    void setBckslc_avg(const short avg);
    void setDead_corr(const bool corr);
    void setDecay_corr(const bool corr);
    void setTransinofile(const char* file);
    void setBlnksinofile(const char* file);
    void setTran_ray_fwhm(const float fwhm);
    void setTran_axl_fwhm(const float fwhm);
    void setSurv_mask(const short mask);
    void setPreflt_type(const PreFlt_Type type);
    void setPostflt_type(const PreFlt_Type type);
    void setTr_posttyp(const PostProc_Type type);
    void setAlgtype_tr(const AlgTr_Type type);
    void setIter_tr(const short tr);
    void setSubset_tr(const short tr);
    void setNsmooth_tr(const short tr);
    void setNrepeat_tr(const short tr);
    void setAttn_corr_3d(const bool attn);
    void setRamla_no_it(const short it);
    void setRamla_sysac(const short sysac);
    void setRamla_lambda(const short i, const float lambda);
    void setRamla_blrad(const float blrad);
    void setRamla_blalpha(const float blalpha);
    void setRamla_bcc_rsz(const float rsz);
    void setRecon_date_time(const time_t time);
    void setGating_type(const short type);
    void setRef_attncor_series_uid(const char* str);
    void setRef_gated_qc_image_inst_uid(const char* str);
    void setRef_raw_data_inst_uid(const char* str);
    void setStart_table_pos_abs(const int pos);
    void setStart_table_pos_rel(const int pos);
    void setMr_valid(const bool valid);
    void setCoil_type(const char* str);

    // special Qt-based methods
    QDate patient_Birth_Date_Qt() const;
    void setPatient_Birth_Date_Qt(const QDate& date);
    QDateTime petct_align_timestamp_Qt() const;
    void setPetct_align_timestamp_Qt(const QDateTime& datetime);
    QDateTime acq_date_time_Qt() const;
    void setAcq_date_time_Qt(const QDateTime& datetime);
    QDateTime study_date_time_Qt() const;
    void setStudy_date_time_Qt(const QDateTime& datetime);
    QDateTime injection_date_time_Qt() const;
    void setInjection_date_time_Qt(const QDateTime& datetime);
    QDateTime file_create_date_time_Qt() const;
    void setFile_create_date_time_Qt(const QDateTime& datetime);
    QDateTime recon_date_time_Qt() const;
    void setRecon_date_time_Qt(const QDateTime& datetime);

  private:
    CPhilipsMainHeaderPrivate* m_pData;
};

#endif // CPHILIPSMAINHEADER_H
