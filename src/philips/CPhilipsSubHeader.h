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

#ifndef CPHILIPSSUBHEADER_H
#define CPHILIPSSUBHEADER_H

#ifndef __MEDIO_PRIVATE__
#include <CMedIOHeader>
#else
#include <CMedIOHeader.h>
#endif

#include <QDateTime>

// forward declarations
class CPhilipsSubHeaderPrivate;
class CPhilipsDirectoryItem;

class CPhilipsSubHeader : public CMedIOHeader
{
  public:
    enum Type { Unknown = 0,
                Image,
                Sinogram,
                Syntegra,
                Listmode
              };

    enum Data_Type { UnknownDataType = 0,
                     ByteData = 1,      // 1: char
                     SignedShort = 2,   // 2: short
                     UnsignedShort = 3, // 2: unsigned short
                     Float = 4          // 4: float
                   };

    enum Randoms_Type { UndefinedRandomsType = 0,
                        NoneRandoms,
                        Delayed,
                        Singles
                      };

    enum Decay_Type { UndefinedDecayType = 0,
                      NoneDecay,
                      Acqstart,
                      Injadmin
                    };

    enum Laterality_Type { UnknownLateralityType = 0,
                           Right,
                           Left,
                           Unpaired,
                           Both
                         };

    enum Anatomy_Type { UnknownAnatomyType = 0,
                        Abdomen,
                        Abdomen_And_Pelvis,
                        Chest,
                        Chest_And_Abdomen,
                        Chest_Abdomen_And_Pelvis,
                        Entire_Body,
                        Head,
                        Head_And_Neck,
                        Heart,
                        Neck,
                        Neck_And_Chest,
                        Neck_Chest_And_Abdomen,
                        Neck_Chest_Abdomen_And_Pelvis,
                        Pelvis,
                        Leg,
                        Pelvis_And_Lower_Extremeties
                      };

    // constructor
    CPhilipsSubHeader(CMedIOData* philipsFile,
                      CPhilipsDirectoryItem* dItem = NULL);
    ~CPhilipsSubHeader();
    
    // copy constructur and default assignment operator
    CPhilipsSubHeader(const CPhilipsSubHeader& src);    
    CPhilipsSubHeader& operator=(const CPhilipsSubHeader& src);

    // header clear method
    void clear();

    // our virtual load/save I/O routines
    bool load(void);
    bool save(void) const;

    // size information in bytes of specific raw subheader structure
    int rawDataSize() const;
 
    // runtime type information methods
    CMedIOHeader::Format headerFormat() const;
    virtual CPhilipsSubHeader::Type subHeaderType(void) const = 0;

    // internal method to set the directory Item of a subHeader
    void setDirectoryItem(CPhilipsDirectoryItem* dItem);

    // clone methods
    virtual CMedIOHeader* clone() const = 0;

    // convertFrom() method
    bool convertFrom(const CMedIOHeader* subHeader, const CMedIOHeader* mainHeader=NULL);

    // common data accessor methods
    short version() const;
    const char* atten_corr() const;
    float actual_bedpos() const;
    float orientation(const short i) const;
    int card_fr_time() const;
    int card_high_rr() const;
    int card_low_rr() const;
    int card_tr_time() const;
    const char* scatter_corr() const;
    short deadtime_corr() const;
    Randoms_Type randoms_corr() const;
    short det_norm() const;
    short nu_radsamp_corr() const;
    short pat_mot_corr() const;
    float echo_time() const;
    float exposure_time() const;
    float img_pos_x() const;
    float img_pos_y() const;
    float img_pos_z() const;
    Data_Type datype() const;
    short xdim() const;
    short ydim() const;
    short slcnum() const;
    short tiltnum() const;
    short gatint() const;
    short cntloss_corr() const;
    float pix_spacing_x() const;
    float pix_spacing_y() const;
    float xray_current() const;
    float kvp() const;
    float Dslice_loc() const;
    float magfac() const;
    Decay_Type decay_corr() const;
    short dep_strhr() const;
    short dep_strmin() const;
    short dep_strsec() const;
    short dep_endhr() const;
    short dep_endmin() const;
    short dep_endsec() const;
    int midtim() const;
    short mseclen() const;
    short scnlen() const;
    float bgdelrt() const;
    float enddelrt() const;
    float bgsngrt() const;
    float bgcoincrt() const;
    float endsngrt() const;
    float endcoincrt() const;
    float deadtimefac() const;
    short bedpos() const;
    float deadtime_bgsub() const;
    const char* sop_uid() const;
    const char* recon_method() const;
    time_t start_date_time() const;
    time_t end_date_time() const;
    Laterality_Type laterality() const;
    Anatomy_Type anatomy() const;
    time_t frame_ref_date_time() const;
    int card_rr_time() const;
    int resp_int_time() const;
    unsigned short start_date_time_msec() const;
    unsigned short end_date_time_msec() const;
    unsigned short frame_ref_date_time_msec() const;
    double prompt_events_sum() const;
    double delay_events_sum() const;
    float scatter_fraction_average() const;
    
    // methods to modify elements of the SubHeader  
    void setVersion(const short version);
    void setAtten_corr(const char* str);
    void setActual_bedpos(const float bedpos);
    void setOrientation(const short i, const float orientation);
    void setCard_fr_time(const int card_fr_time);
    void setCard_high_rr(const int card_high_rr);
    void setCard_low_rr(const int card_low_rr);
    void setCard_tr_time(const int card_high_rr);
    void setScatter_corr(const char* str);
    void setDeadtime_corr(const short deadtime_corr);
    void setRandoms_corr(const Randoms_Type randoms_corr);
    void setDet_norm(const short det_norm);
    void setNu_radsamp_corr(const short nu_radsamp_corr);
    void setPat_mot_corr(const short pat_mot_corr);
    void setEcho_time(const float  echo_time);
    void setExposure_time(const float exposure_time);
    void setImg_pos_x(const float img_pos_x);
    void setImg_pos_y(const float img_pos_y);
    void setImg_pos_z(const float img_pos_z);
    void setDatype(const Data_Type datype);
    void setXdim(const short xdim);
    void setYdim(const short ydim);
    void setSlcnum(const short slcnum);
    void setTiltnum(const short tiltnum);
    void setGatint(const short gatint);
    void setCntloss_corr(const short cntloss_corr);
    void setPix_spacing_x(const float pix_spacing);
    void setPix_spacing_y(const float pix_spacing);
    void setXray_current(const float xray_current);
    void setKvp(const float kvp);
    void setDslice_loc(const float Dslice_loc);
    void setMagfac(const float magfac);
    void setDecay_corr(const Decay_Type decay_corr);
    void setDep_strhr(const short strhr);
    void setDep_strmin(const short strmin);
    void setDep_strsec(const short strsec);
    void setDep_endhr(const short endhr);
    void setDep_endmin(const short endmin);
    void setDep_endsec(const short endsec);
    void setMidtim(const int midtim);
    void setMseclen(const short mseclen);
    void setScnlen(const short scnlen);
    void setBgdelrt(const float bgdelrt);
    void setEnddelrt(const float enddelrt);
    void setBgsngrt(const float bgsngrt);
    void setBgcoincrt(const float bgcoincrt);
    void setEndsngrt(const float endsngrt);
    void setEndcoincrt(const float endcoincrt);
    void setDeadtimefac(const float deadtimefac);
    void setBedpos(const short bedpos);
    void setDeadtime_bgsub(const float deadtime_bgsub);
    void setSop_uid(const char* str);
    void setRecon_method(const char* str);
    void setStart_date_time(const time_t start_date_time);
    void setEnd_date_time(const time_t end_date_time);
    void setLaterality(const Laterality_Type laterality);;
    void setAnatomy(const Anatomy_Type anatomy);
    void setFrame_ref_date_time(const time_t frame_ref_date_time);
    void setCard_rr_time(const int card_rr_time);
    void setResp_int_time(const int resp_int_time);
    void setStart_date_time_msec(const unsigned short msec);
    void setEnd_date_time_msec(const unsigned short msec);
    void setFrame_ref_date_time_msec(const unsigned short msec);
    void setPrompt_events_sum(const double value);
    void setDelay_events_sum(const double value);
    void setScatter_fraction_average(const float value);

    // special Qt-based methods
    QDateTime start_date_time_Qt() const;
    void setStart_date_time_Qt(const QDateTime& datetime);
    QDateTime end_date_time_Qt() const;
    void setEnd_date_time_Qt(const QDateTime& datetime);
    QDateTime frame_ref_date_time_Qt() const;
    void setFrame_ref_date_time_Qt(const QDateTime& datetime);

  // some methods only accessible from subclasses
  protected:

    // Image only
    float suvscl() const;
    float imgscl() const;
    short imgmin() const;
    short imgmax() const;
    float imgsum() const;
    void setSuvscl(const float suvscl);
    void setImgscl(const float imgscl);
    void setImgmin(const short imgmin);
    void setImgmax(const short imgmax);
    void setImgsum(const float imgsum);

    // Sinogram only
    float scnscl() const;
    short scnmin() const;
    short scnmax() const;
    float scnsum() const;
    void setScnscl(const float scnscl);
    void setScnmin(const short scnmin);
    void setScnmax(const short scnmax);
    void setScnsum(const float scnsum);

  protected:
    CPhilipsDirectoryItem* m_pDirItem; // the directory item to which this
                                       // subHeader belongs
    CPhilipsSubHeaderPrivate *m_pData;
};

#endif // CPHILIPSSUBHEADER_H
