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

#ifndef CPHILIPSEXTENDEDMAINHEADER_H
#define CPHILIPSEXTENDEDMAINHEADER_H

#ifndef __MEDIO_PRIVATE__
#include <CMedIOHeader>
#else
#include <CMedIOHeader.h>
#endif

class CPhilipsFile;
class CPhilipsDirectoryItem;
class CPhilipsExtendedMainHeaderPrivate;

class CPhilipsExtendedMainHeader : public CMedIOHeader
{
  public:
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

    enum Card_Slc_Dir{
      Undefined_Card_Slc_Dir = 0,
      Base_To_Apex,
      Apex_To_Base
    };

    CPhilipsExtendedMainHeader(CPhilipsFile* file = NULL,
                               CPhilipsDirectoryItem* pDirItem = NULL);
    ~CPhilipsExtendedMainHeader();

    // copy constructor and default assignment operator
    CPhilipsExtendedMainHeader(const CPhilipsExtendedMainHeader& src);
    CPhilipsExtendedMainHeader& operator=(const CPhilipsExtendedMainHeader& src);

    // header clear method
    void clear();

    // file i/o methods
    bool load();
    bool save() const;

    // the number of bytes the data of that header requires on disk
    unsigned int rawDataSize() const;

    CMedIOHeader::Format headerFormat() const;

    // internal method to set the directory Item of a extendedMainHeader
    void setDirectoryItem(CPhilipsDirectoryItem* dItem);

    // clone methods
    virtual bool convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader* pHead2 = NULL);
    virtual CMedIOHeader* clone() const;

    // data acess methods
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

    const char* contr_bolus_agent();
    const char* sop_uid();
    const char* frame_ref_uid();
    const char* pps_file();
    const char* worklist_file();


    // methods to modify elements of the extendedMainHeader
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

  private:
    CPhilipsExtendedMainHeaderPrivate* m_pData;
    CPhilipsDirectoryItem* m_pDirItem; // the directory item to which this
                                       // extendedMainHeader belongs

};

#endif // CPHILIPSEXTENDEDMAINHEADER_H
