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

#include <QDataStream>
#include <QDate>

#include "CPhilipsMainHeader.h"
#include "CPhilipsFile.h"
#include "CPhilipsDirectory.h"
#include "CPhilipsDirectoryItem.h"
#include "CMedIOHeader.h"
#include "CMedIOData.h"
#include "CECAT7MainHeader.h"

#include <rtdebug.h>

#include "ByteSwap.h"

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CPhilipsMainHeaderPrivate
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

  // MainHeader structure (should be 512 bytes)
  #define MAINHEADER_SIZE 512
  #define MAINHEADER_VERSION 13
  #pragma pack(1)
  struct HeaderData
  {
    char magic_number[6];         // 000: identifier
    qint16 file_fmt;              // 006: file format (mainheader) version number
    qint16 scan_geom;             // 008: encoding of the scanner geometry
    qint16 hw_config;             // 010: encoding of hardware used in acquisition
    char empty1[4];               // 012: empty
    qint16 edit_flag;             // 016: 1 to indicat that the user has modified mainheader
    char empty2[30];              // 018: empty
    char empty3[4];               // 048: empty
    qint16 filtyp;                // 052: file type
    qint32 minTransXtalDiff;      // 054: Minimum zone difference in crystal space during all list-mode collections.
    float tofTstampScale;         // 058: Time-Of-Flight time stamp scale.
    char empty4[4];               // 062: empty
    qint16 dep_daycre;            // 066:
    qint16 dep_mocre;             // 068:
    qint16 dep_yrcre;             // 070:
    qint16 dep_hrcre;             // 072:
    qint16 dep_mincre;            // 074:
    qint16 dep_seccre;            // 076:
    qint16 duratn;                // 078: duration of scan in seconds
    qint16 shdtyp;                // 080: ImageIO subheader (1) or old subheader format (0)
    qint16 sngpscl;               // 082: singles prescale, superceded by pscale
    qint16 singopt;               // 084: 0=transmission only, 1=trans-ec, 2=ec only (obsoleted)
    float pscale;                 // 086: amount by which the actual singles events is scaled down
    float detectorRadius;         // 090: inscribed scanner radius (from middle of opening to detector face) in mm
    qint16 virtualXtal;           // 094: whether virtual crystals were used in the acquisition to simulate a larger detector
    qint16 phiMashing;            // 096: whether events from crystal phi values are combined:
                                  //      6 Undefined/Unknown (assumption is no mashing)
                                  //      7 No mashing
                                  //      8 Events from pairs of consecutive angles have been added together
    qint16 polygonSides;          // 098: Number of "sides" in the scanner opening (may refer to detectors or PMT modules
    qint16 xtalsPerSide;          // 100: Number of detectors per opening (polygon) side
    qint16 nXtalRows;             // 102: Number of crystal rows in the axial (Z) direction
    float crystalThickness;       // 104: Detetctor crystal thickness in mm
    float xXtalPitch;             // 108: X crystal pitch in mm
    float zXtalPitch;             // 112: Z crystal pitch in mm
    float axialFOV;               // 116: in mm
    qint16 rphiType;              // 120: 0 Real, 1 Crystal
    qint16 sliceType;             // 122:
    qint16 delayType;             // 124:
    char empty5[4];               // 126: empty
    qint16 pattyp;                // 130:
    qint16 scntyp;                // 132:
    qint16 numray;                // 134:
    qint16 numang;                // 136:
    qint16 slcthk;                // 138:
    qint16 isotop;                // 140:
    float slope;                  // 142: Rescale slope. (Not used)
    float intcpt;                 // 146: Offset value to data. (Not used)
    qint16 injtim;                // 150:
    float polygonVertAt0deg;      // 152:
    qint16 nslice;                // 156: number of slices per frame
    qint16 nframe;                // 158: number of frames
    qint16 bthday;                // 160:
    qint16 bthmo;                 // 162:
    qint16 bthyr;                 // 164:
    char ssn[10];                 // 166: suberceded by Dicom_Patient_ID
    qint16 ntilt;                 // 176: number of tilts per frame
    qint16 petnum;                // 178:
    char empty6[4];               // 180:
    float activity;               // 184: in MBq
    qint32 weight;                // 188: in g
    qint16 hrinj;                 // 192:
    qint16 mininj;                // 194:
    float srcRadius;              // 196:
    float srcZpos;                // 200:
    float halfLife;               // 204: in minutes
    float concfac;                // 208:
    float concfac_bgsub;          // 212:
    float dmax;                   // 216:
    float dline;                  // 220:
    float angmax;                 // 224:
    float x0;                     // 228:
    float y0;                     // 232:
    float z0;                     // 236:
    float nevent;                 // 240:
    float nsino;                  // 244:
    qint16 eglob_low;             // 248:
    qint16 eglob_up;              // 250:
    qint16 eloc_low;              // 252:
    qint16 eloc_up;               // 254:
    qint16 orient_hf;             // 256:
    char scan_swrel[6];           // 258:
    qint16 petct_sepdist;         // 264: PET separation distance at acq time (1/10mm)
    qint16 petct_landmrk;         // 266: Landmark position at acq time (1/10mm)
    qint32 petct_timestamp;       // 268: Date/Time of alignment calibration (seconds since Jan 1, 1970 [UNIX])
    qint16 tbl_direction;         // 272:
    qint16 orient_ps;             // 274:
    qint16 petct_zoffset;         // 276: Not used at this time.
    qint16 petct_xshift;          // 278: shift in x from align-cal in 1/1000mm
    qint16 petct_yshift;          // 280: shift in y from align-cal in 1/1000mm
    qint16 petct_zshift;          // 282: shift in z from align-cal in 1/1000mm
    qint16 petct_acqflgs;         // 284: Not used at this time.
    qint16 petct_xoffset;         // 286: syn - (0020,0032) Image position x pos
                                  //      shift in x of the center of the CT FOV    
                                  //      with respect to the PET FOV in 1/100mm
    qint16 petct_yoffset;         // 288: syn - (0020,0032) Image position y pos
                                  //      shift in y of the center of the CT FOV
                                  //      with respect to the PET FOV in 1/100mm
    qint16 petct_axrot;           // 290: rotation about z axis in 1/10,000 deg
    qint16 petct_horzrot;         // 292: rotation about x axis in 1/10,000 deg
    qint16 petct_vertrot;         // 294: rotation about y axis in 1/10,000 deg
    float frontLeadDiameter;      // 296:
    float backLeadDiameter;       // 300:
    float leadSeparation;         // 304:
    float ndelays;                // 308:
    float slcsep;                 // 312:
    qint16 petct_valid;           // 316: Whether the file has a valid petct struct
    char fctrfil[20];             // 318: Factor file name. (PMT gains). was 16
                                  //      syn - (0008,1090) manufacturer's model name
    char baselin[20];             // 338: Baseline file name. (DC offsets). was 16
    char dstpkfl[20];             // 358: Distortion peak file name.
    char aqprotocol_name[20];     // 378: acquisition protocol name
    qint16 aqprotocol_type;       // 398: 1=Emiss-Static, 2=Emiss-Dynamic, 3=Trans-Static, 4=Gated Cardiac,
                                  //      5=Emiss-Whole-Body, 6=Trans-Whole-Body, 7=Singles Trans, 8=Singles-Whole-Body
    char patient_name[30];        // 400: patient name
    float reslice_ang1;           // 430: Reslicing (OBL) angle 1
    float reslice_ang2;           // 434: Reslicing (OBL) angle 2
    float reslice_ang3;           // 438: Reslicing (OBL) angle 3
    qint16 minslc;                // 442: Minimum Slice number / syn - set to 1
    qint16 maxslc;                // 444: Maximum Slice number / syn - set to number of slices
    qint16 minfrm;                // 446: Minimum Frame number
    qint16 maxfrm;                // 448: Maximum Frame number
    qint16 scanner_maxslice;      // 450: Maximum Slice number for a static frame based on the useful axial extent of the scanner. (unitless)
    char empty7[2];               // 452: empty
    qint16 rebin_type;            // 454: multi-slice or single slice or LOR
    char scnOrigin[16];           // 456: indicates origin of scan data = scanner number (a000) - $SITENAME (10 chars) plus null
    char accNum[16];              // 472: Accession number. Will eventually be used with DICOM query retrieve. Initially used to support
                                  //      dicom-send to PACS.
    qint16 movementCoinc;         // 488: table movement between emission frames in mm
    qint16 movementSing;          // 490: table movement between transmission frames in mm
    qint16 crbTstampPeriod;       // 492: Period between CRB timestamps in msec
    char empty8[10];              // 494: empty
    qint16 trailexists;           // 504: 1= trailer exists, 0 = no trailer
    quint32 trailbeg;             // 506: unsigned 32 bit number = # of bytes from file beginning indicating where the trailer begins
    char empty9[2];               // 510: empty
  } header;
  #pragma pack()

  // the extended header data structure
  #define EXTHEADER_SIZE 2048
  #pragma pack(1)
  struct ExtHeaderData
  {
    char Dpat_name[64];                   // 0000: DICOM patient name
    char Dpat_id[64];                     // 0064: DICOM patient ID
    char study_uid[64];                   // 0128: unique study identifier
    char series_uid[64];                  // 0192: unique series identifier
    char view_code[20];                   // 0256: Our version of DICOM view code
    char sortproto_name[20];              // 0276: sorter protocol name
    qint16 route;                         // 0296: 
    qint16 pharm;                         // 0298:
    char req_phys[64];                    // 0300: requesting physician
    qint16 card_phstate;                  // 0364: Physiologic state
    qint32 assay_date;                    // 0366: Date of radioactivity measurement
    qint32 assay_time;                    // 0370: Time of radioactivity measurement
    char series_desc[64];                 // 0374: Description
    qint16 height;                        // 0438: Patient height, in mm
    qint16 abundance;                     // 0440: Positron abundance ratio, in tenths of percent so that 1000 is 100%
    qint16 realign_x;                     // 0442: For cardiac realignment. Additional horizontal (x) shift of center of CT
                                          //       FOV w.r.t. PET (in 1/100mm).
    qint16 realign_y;                     // 0444: For cardiac realignment. Additional vertical (y) shift of center of CT
                                          //       FOV w.r.t. PET (in 1/100mm).
    qint16 realign_hr;                    // 0446: For cardiac realignment. Additional rotation about the horizontal (X) axis
                                          //       (in 1/1,000 degree).
    qint32 acq_date_time;                 // 0448: Date/time data acquired (UTC). Use access functions to read/write.
    qint32 study_date_time;               // 0452: Date/time study started (UTC). Use access functions to read/write.
    qint32 injection_date_time;           // 0456: Date/time of injection (UTC). Use access functions to read/write.
    qint32 file_create_date_time;         // 0460: Replaces daycre, mocre, yrcre, hrcre, mincre and seccre fields. Date and
                                          //       time that the acquisition file was created. Filled in by acquisition.
    qint16 resp_trig_loc;                 // 0464: Respiratory trigger location.
    qint16 card_arrhythmia_rej_tech;      // 0466: Card Arrhythmia rejection techniques employed:
                                          //       Bit mask:
                                          //        0 = no rejection
                                          //        1 = Rejection based on deviation from average R-R interval
                                          //        2 = Rejection based on deviation from regular QRS loop
                                          //        4 = Rejection based on PVC criteria.
    float window_center;                  // 0468: Suggested center value to use when displaying this image.
    float window_width;                   // 0472: Suggested width value to use when displaying this image.
    qint16 realign_zr;                    // 0476: For cardiac realignment. Additional rotation about the axial (Z) axis
                                          //       (in 1/1,000 degree).
    qint16 realign_vr;                    // 0478: For cardiac realignment. Additional rotation about the vertical (Y) axis
                                          //       (in 1/1,000 degree).
    char empty1[8];                       // 0480: empty
    qint16 resp_trig_threshold;           // 0488: Respiratory trigger threshold in % of chest expansion relative to last peak.
    qint16 resp_phase_duration;           // 0490: Respiratory acquisition duration in % of the respiratory cycle.
    qint16 resp_phase_offset;             // 0492: For respiratory gated, the offset from the trigger to the beginning of the acquisition, in %
                                          //        of the respiratory cycle.
    qint16 realign_z;                     // 0494: For cardiac realignment. Additional (z) shift of center of CT
                                          //       FOV w.r.t. PET (in 1/100mm).
    qint16 window_units;                  // 0496: Units in which window_center and window_width are specified.
    char empty2[14];                      // 0498: empty
    char referring_physician[64];         // 0512: referring physician
    char study_id[16];                    // 0576: study identifier
    char empty3[2];                       // 0592: empty
    float Dslice_thick;                   // 0594: DICOM slice thickness
    char sex;                             // 0598: (DICOM) patient sex
    float table_height;                   // 0599: Table height
    char empty4[1];                       // 0603: empty
    qint16 card_bt_rej;                   // 0604: Heart beat duration sorting (y/n)
    qint16 card_fr_type;                  // 0606: Type of cardiac framing performed
    char Dmanufacture_model_name[64];     // 0608: DICOM manufacturers model name
    char Dimage_type[64];                 // 0672: DICOM image identification
    float min_bed_pos;                    // 0736: Minimum bed position
    float max_bed_pos;                    // 0740: Maximum bed position
    qint16 der_filled;                    // 0744: Whether derived fields filled
    qint32 series_number;                 // 0746: DICOM series number
    qint32 dep_study_date;                // 0750: (Deprecated - use study_date_time) Date study created (date_2_int())
    qint32 dep_study_time;                // 0754: (Deprecated - use study_date_time) time study created
    qint32 dep_acq_time;                  // 0758: (Deprecated - use acq_date_time) time data acquired
    qint16 card_slc_dir;                  // 0762: Slice progression direction
    char empty5[4];                       // 0764: empty
    qint32 card_skip_msec;                // 0768: initial data skipped, in msec
    qint32 card_skip_counts;              // 0772: initial data skipped, in counts
    qint32 card_dur_msec;                 // 0776: duration of binned data, in msec
    qint32 card_dur_counts;               // 0780: duration of binned data, in counts
    qint32 card_beats_tot;                // 0784: beats occurring during movie
    qint32 card_beats_acc;                // 0788: beats accepted into movie
    qint16 card_skip_beats;               // 0792: Num beats skipped after arrhythmia.
    qint16 pvc_threshold;                 // 0794: When PVC rejection used, the % of R-R below which is considered a PVC.
    char empty6[12];                      // 0796: empty
    qint32 dep_acq_date;                  // 0808: (Deprecated - use acq_date_time) Date data acquired (date_2_int())
    char empty7[4];                       // 0812: empty
    char radiopharm_name[64];             // 0816: Used when pharm == Other, to specify name of radiopharmaceutical used.
    char Dserial_number[16];              // 0880: System serial number + null
    char attncor_label[64];               // 0896: Label (UID) identifying related atten. corr. series.
    char empty8[64];                      // 0960: empty
    char contr_bolus_agent[64];           // 1024: DICOM contrast bolus agent
    char sop_uid[64];                     // 1088: SOP UID of incoming images
    char frame_ref_uid[64];               // 1152: UID of frame of reference image
    char pps_file[30];                    // 1216: Perform procedure file
    char worklist_file[30];               // 1246: Worklist file
    char empty9[4];                       // 1276: empty
    char recon_swrel[6];                  // 1280:
    char analy_swrel[6];                  // 1286:
    char recprotocol_name[19];            // 1292:
    char insinofile[19];                  // 1311:
    qint16 slc_add;                       // 1330:
    qint16 slc_space;                     // 1332:
    qint16 slc_thick;                     // 1334:
    qint16 frame_add;                     // 1336:
    qint16 frame_space;                   // 1338:
    qint16 frame_thick;                   // 1340:
    qint16 fltr_type;                     // 1342:
    qint16 smoth;                         // 1344:
    qint16 scatcorr_type;                 // 1346: 
    qint16 edge_exp;                      // 1348: Number of bins to expand edge in background subtraction
    qint16 bckang_avg;                    // 1350: Number of angles to average in background subtraction
    float bck_coeff;                      // 1352: Non-uniform background coefficient background subtraction
    qint16 bck_wid;                       // 1356: Number of bins for fitting tails radially in background subtraction
    qint16 attncor_type;                  // 1358: The attenuation correction algorithm used in reconstruction
    qint16 attncor_ecc;                   // 1360: Emission contamination correction used in attenuation correction
    float attn_coeff;                     // 1362: Coefficient in attenuation correction (in centimeters)
    char regfile[19];                     // 1366:
    char proc_transinofile[19];           // 1385:
    float skull_comp;                     // 1404:
    qint16 norm_type;                     // 1408:
    qint16 smp_norm;                      // 1410:
    char axnfile[19];                     // 1412:
    char effnormfile[19];                 // 1431:
    qint16 gap_comp;                      // 1450:
    qint16 algtype_em;                    // 1452:
    qint16 num_iter;                      // 1454:
    qint16 iter_em;                       // 1456:
    qint16 subset_em;                     // 1458:
    qint16 nsmooth_em;                    // 1460:
    qint16 nrepeat_em;                    // 1462:
    qint16 bckslc_avg;                    // 1464:
    qint16 dead_corr;                     // 1466:
    qint16 decay_corr;                    // 1468:
    char transinofile[19];                // 1470:
    char blnksinofile[19];                // 1489:
    float tran_ray_fwhm;                  // 1508:
    float tran_axl_fwhm;                  // 1512:
    qint16 surv_mask;                     // 1516:
    qint16 preflt_type;                   // 1518:
    qint16 postflt_type;                  // 1520:
    qint16 tr_posttyp;                    // 1522:
    qint16 algtype_tr;                    // 1524:
    qint16 iter_tr;                       // 1526:
    qint16 subset_tr;                     // 1528:
    qint16 nsmooth_tr;                    // 1530:
    qint16 nrepeat_tr;                    // 1532:
    qint16 attn_corr_3d;                  // 1534:
    qint16 ramla_no_it;                   // 1536:
    qint16 ramla_sysac;                   // 1538:
    float ramla_lambda[5];                // 1540:
    float ramla_blrad;                    // 1560:
    float ramla_blalpha;                  // 1564:
    float ramla_bcc_rsz;                  // 1568:
    qint32 recon_date_time;               // 1572:
    qint16 gating_type;                   // 1576:
    char empty10[6];                      // 1578:
    char ref_attncor_series_uid[64];      // 1584:
    char ref_gated_qc_image_inst_uid[64]; // 1648: SOP Instance UID of Sec. Cap. image containing picture of cardiac waveform and trigger level.
    char ref_raw_data_inst_uid[64];       // 1712:
    qint32 start_table_pos_abs;           // 1776: Absolute start table position.
    qint32 start_table_pos_rel;           // 1780: Relative start table position.
    qint16 mr_valid;                      // 1784: Whether the file contains a valid PET/MR struct
    char empty11[6];                      // 1786: empty
    char coil_type[16];                   // 1792: number of MR coils positioned in FOV during scan
    char empty12[240];                    // 1808:
  } extHeader;
  #pragma pack()

  // convert functions
  bool ecat2philipsSex(const CECAT7MainHeader::Patient_Sex sex);
  bool ecat2philipsIsotop(const QString& isotop);
  bool ecat2philipsOrientation(const CECAT7MainHeader::Patient_Orientation eOrientation);
  bool ecat2philipsAcquisitionType(const CECAT7MainHeader::Acquisition_Type acqType);
};

CPhilipsMainHeader::CPhilipsMainHeader(CPhilipsFile* file,
                                       CPhilipsMainHeader::File_Type fileType)
  : CMedIOHeader(file)
{
  ENTER();

  m_pData = new CPhilipsMainHeaderPrivate();

  // this constructor creates a empty EPhilipsMainHeader
  // with prefilled data that is always the same for a philips main header
  clear();

  setFiltyp(fileType);

  LEAVE();
}

CPhilipsMainHeader::CPhilipsMainHeader(const CPhilipsMainHeader& src)
  : CMedIOHeader(src)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CPhilipsMainHeaderPrivate(*(src.m_pData));

  LEAVE();
}

CPhilipsMainHeader& CPhilipsMainHeader::operator=(const CPhilipsMainHeader& src)
{
  ENTER();

  if(m_pData != src.m_pData)
  {
    memcpy(&m_pData->header,
           &src.m_pData->header,
           sizeof(struct CPhilipsMainHeaderPrivate::HeaderData));

    memcpy(&m_pData->extHeader,
           &src.m_pData->extHeader,
           sizeof(struct CPhilipsMainHeaderPrivate::ExtHeaderData));
  }

  LEAVE();
  return *this;
}
CPhilipsMainHeader::~CPhilipsMainHeader()
{
  ENTER();

  delete m_pData;

  LEAVE();
}

void CPhilipsMainHeader::clear()
{
  ENTER();

  // clear our MainHeader structure first
  memset(&m_pData->header, 0, sizeof(struct CPhilipsMainHeaderPrivate::HeaderData));
  memset(&m_pData->extHeader, 0, sizeof(struct CPhilipsMainHeaderPrivate::ExtHeaderData));

  // lets set some empty header elements to the predefined magic number
  m_pData->header.magic_number[0] = 0x01;
  m_pData->header.magic_number[1] = 0x00;
  m_pData->header.magic_number[2] = 0x00;
  m_pData->header.magic_number[3] = 0x00;
  m_pData->header.magic_number[4] = 0x00;
  m_pData->header.magic_number[5] = 0x16;

  m_pData->header.empty1[0] = 0x00;
  m_pData->header.empty1[1] = 0x40;
  m_pData->header.empty1[2] = 0x00;
  m_pData->header.empty1[3] = 0x5B;

  m_pData->header.empty3[0] = 0x00;
  m_pData->header.empty3[1] = 0x07;
  m_pData->header.empty3[2] = 0xFF;
  m_pData->header.empty3[3] = 0xFF;
  
  m_pData->header.empty4[0] = 0x00;
  m_pData->header.empty4[1] = 0x20;
  m_pData->header.empty4[2] = 0xFF;
  m_pData->header.empty4[3] = 0xFF;

  m_pData->header.empty5[0] = 0x00;
  m_pData->header.empty5[1] = 0x1B;
  m_pData->header.empty5[2] = 0xFF;
  m_pData->header.empty5[3] = 0xFF;

  m_pData->header.empty6[0] = 0x00;
  m_pData->header.empty6[1] = 0x21;
  m_pData->header.empty6[2] = 0xFF;
  m_pData->header.empty6[3] = 0xFF;

  // set some default values for the new Main Header
  m_pData->header.file_fmt = MAINHEADER_VERSION;

  LEAVE();
}

bool CPhilipsMainHeader::load()
{
  ENTER();

  // only go on if the device is readable at all
  if(m_pMedIOData == NULL ||
     m_pMedIOData->isReadable() == false ||
     m_pMedIOData->seek(0) == false)
  {
    RETURN(false);
    return false;
  }

  // read in the main header in one read() operation
  ASSERT(sizeof(m_pData->header) == MAINHEADER_SIZE);
  if(m_pMedIOData->read((char*)&m_pData->header, sizeof(m_pData->header)) != MAINHEADER_SIZE)
  {
    RETURN(false);
    return false;
  }

  // now we also load the extended main header. However, we first have to query
  // for the directory item of the extended header so that we can set the file
  // to the right position.
  ASSERT(sizeof(m_pData->extHeader) == EXTHEADER_SIZE);
  CPhilipsFile* philipsFile = static_cast<CPhilipsFile*>(m_pMedIOData);
  CPhilipsDirectory* philipsDirectory = philipsFile->directory();
  bool extendedHeaderFound = false;
  if(philipsDirectory != NULL)
  {
    if(philipsDirectory->isEmpty())
      philipsDirectory->load();

    if(philipsDirectory->isEmpty() == false)
    {
      CPhilipsDirectoryItem* extHeaderItem = philipsDirectory->extendedMainHeaderItem();
      if(extHeaderItem != NULL)
      {
        // now we have the extendedheader diritem. so lets get the header out of it
        if(m_pMedIOData->seek(extHeaderItem->dataBlock_Start()) == false ||
           m_pMedIOData->read((char*)&m_pData->extHeader, sizeof(m_pData->extHeader)) != EXTHEADER_SIZE)
        {
          E("Error while trying to seek&load extended main header data");

          RETURN(false);
          return false;
        }

        extendedHeaderFound = true;
      }
      else
        W("no extended header found in philips file");
    }
    else
      W("empty directory found");
  }
  else 
    W("directory == NULL");

  // now that we have streamed in all data in one run we
  // have to take care of correct endianness in the non-char
  // entries in the header structure in case this is a little endian
  // machine
  if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
  {
    // we only swap non-char elements of the header
    BSWAP_16(m_pData->header.file_fmt);
    BSWAP_16(m_pData->header.scan_geom);           
    BSWAP_16(m_pData->header.hw_config);           
    BSWAP_16(m_pData->header.edit_flag);           
    BSWAP_16(m_pData->header.filtyp);             
    BSWAP_32(m_pData->header.minTransXtalDiff);  
    BSWAP_FLT(m_pData->header.tofTstampScale);  
    BSWAP_16(m_pData->header.dep_daycre);      
    BSWAP_16(m_pData->header.dep_mocre);      
    BSWAP_16(m_pData->header.dep_yrcre);     
    BSWAP_16(m_pData->header.dep_hrcre);    
    BSWAP_16(m_pData->header.dep_mincre);  
    BSWAP_16(m_pData->header.dep_seccre);        
    BSWAP_16(m_pData->header.duratn);            
    BSWAP_16(m_pData->header.shdtyp); 
    BSWAP_16(m_pData->header.sngpscl); 
    BSWAP_16(m_pData->header.singopt);
    BSWAP_FLT(m_pData->header.pscale);
    BSWAP_FLT(m_pData->header.detectorRadius);
    BSWAP_16(m_pData->header.virtualXtal);
    BSWAP_16(m_pData->header.phiMashing);
    BSWAP_16(m_pData->header.polygonSides);
    BSWAP_16(m_pData->header.xtalsPerSide);
    BSWAP_16(m_pData->header.nXtalRows);
    BSWAP_FLT(m_pData->header.crystalThickness);
    BSWAP_FLT(m_pData->header.xXtalPitch);
    BSWAP_FLT(m_pData->header.zXtalPitch);
    BSWAP_FLT(m_pData->header.axialFOV);
    BSWAP_16(m_pData->header.rphiType);
    BSWAP_16(m_pData->header.sliceType);
    BSWAP_16(m_pData->header.delayType);
    BSWAP_16(m_pData->header.pattyp);
    BSWAP_16(m_pData->header.scntyp);
    BSWAP_16(m_pData->header.numray);
    BSWAP_16(m_pData->header.numang);
    BSWAP_16(m_pData->header.slcthk);
    BSWAP_16(m_pData->header.isotop);
    BSWAP_FLT(m_pData->header.slope);
    BSWAP_FLT(m_pData->header.intcpt);
    BSWAP_16(m_pData->header.injtim);
    BSWAP_FLT(m_pData->header.polygonVertAt0deg);
    BSWAP_16(m_pData->header.nslice);
    BSWAP_16(m_pData->header.nframe);
    BSWAP_16(m_pData->header.bthday);
    BSWAP_16(m_pData->header.bthmo);
    BSWAP_16(m_pData->header.bthyr);
    BSWAP_16(m_pData->header.ntilt);
    BSWAP_16(m_pData->header.petnum);
    BSWAP_FLT(m_pData->header.activity);
    BSWAP_32(m_pData->header.weight);
    BSWAP_16(m_pData->header.hrinj);
    BSWAP_16(m_pData->header.mininj);
    BSWAP_FLT(m_pData->header.srcRadius);
    BSWAP_FLT(m_pData->header.srcZpos);
    BSWAP_FLT(m_pData->header.halfLife);
    BSWAP_FLT(m_pData->header.concfac);
    BSWAP_FLT(m_pData->header.concfac_bgsub);
    BSWAP_FLT(m_pData->header.dmax);
    BSWAP_FLT(m_pData->header.dline);
    BSWAP_FLT(m_pData->header.angmax);
    BSWAP_FLT(m_pData->header.x0);
    BSWAP_FLT(m_pData->header.y0);
    BSWAP_FLT(m_pData->header.z0);
    BSWAP_FLT(m_pData->header.nevent);
    BSWAP_FLT(m_pData->header.nsino);
    BSWAP_16(m_pData->header.eglob_low);
    BSWAP_16(m_pData->header.eglob_up);
    BSWAP_16(m_pData->header.eloc_low);
    BSWAP_16(m_pData->header.eloc_up);
    BSWAP_16(m_pData->header.orient_hf);
    BSWAP_16(m_pData->header.petct_sepdist);
    BSWAP_16(m_pData->header.petct_landmrk);
    BSWAP_32(m_pData->header.petct_timestamp);
    BSWAP_16(m_pData->header.tbl_direction);
    BSWAP_16(m_pData->header.orient_ps);
    BSWAP_16(m_pData->header.petct_zoffset);
    BSWAP_16(m_pData->header.petct_xshift);
    BSWAP_16(m_pData->header.petct_yshift);
    BSWAP_16(m_pData->header.petct_zshift);
    BSWAP_16(m_pData->header.petct_acqflgs);
    BSWAP_16(m_pData->header.petct_xoffset);
    BSWAP_16(m_pData->header.petct_yoffset);
    BSWAP_16(m_pData->header.petct_axrot);
    BSWAP_16(m_pData->header.petct_horzrot);
    BSWAP_16(m_pData->header.petct_vertrot);
    BSWAP_FLT(m_pData->header.frontLeadDiameter);
    BSWAP_FLT(m_pData->header.backLeadDiameter);
    BSWAP_FLT(m_pData->header.leadSeparation);
    BSWAP_FLT(m_pData->header.ndelays);
    BSWAP_FLT(m_pData->header.slcsep);
    BSWAP_16(m_pData->header.petct_valid);
    BSWAP_16(m_pData->header.aqprotocol_type);
    BSWAP_FLT(m_pData->header.reslice_ang1);
    BSWAP_FLT(m_pData->header.reslice_ang2);
    BSWAP_FLT(m_pData->header.reslice_ang3);
    BSWAP_16(m_pData->header.minslc);
    BSWAP_16(m_pData->header.maxslc);
    BSWAP_16(m_pData->header.minfrm);
    BSWAP_16(m_pData->header.maxfrm);
    BSWAP_16(m_pData->header.scanner_maxslice);
    BSWAP_16(m_pData->header.rebin_type);
    BSWAP_16(m_pData->header.movementCoinc);
    BSWAP_16(m_pData->header.movementSing);
    BSWAP_16(m_pData->header.crbTstampPeriod);
    BSWAP_16(m_pData->header.trailexists);
    BSWAP_32(m_pData->header.trailbeg);

    // now swap also the extended header stuff.
    if(extendedHeaderFound == true)
    {
      BSWAP_16(m_pData->extHeader.route);
      BSWAP_16(m_pData->extHeader.pharm);
      BSWAP_16(m_pData->extHeader.card_phstate);
      BSWAP_32(m_pData->extHeader.assay_date);
      BSWAP_32(m_pData->extHeader.assay_time);
      BSWAP_16(m_pData->extHeader.height);   
      BSWAP_16(m_pData->extHeader.abundance);
      BSWAP_16(m_pData->extHeader.realign_x);
      BSWAP_16(m_pData->extHeader.realign_y);
      BSWAP_16(m_pData->extHeader.realign_hr);
      BSWAP_32(m_pData->extHeader.acq_date_time);
      BSWAP_32(m_pData->extHeader.study_date_time);
      BSWAP_32(m_pData->extHeader.injection_date_time);
      BSWAP_32(m_pData->extHeader.file_create_date_time);
      BSWAP_16(m_pData->extHeader.resp_trig_loc);
      BSWAP_16(m_pData->extHeader.card_arrhythmia_rej_tech);
      BSWAP_FLT(m_pData->extHeader.window_center);
      BSWAP_FLT(m_pData->extHeader.window_width);
      BSWAP_16(m_pData->extHeader.realign_zr);
      BSWAP_16(m_pData->extHeader.realign_vr);
      BSWAP_16(m_pData->extHeader.resp_trig_threshold);
      BSWAP_16(m_pData->extHeader.resp_phase_duration);
      BSWAP_16(m_pData->extHeader.resp_phase_offset);
      BSWAP_16(m_pData->extHeader.realign_z);
      BSWAP_16(m_pData->extHeader.window_units);
      BSWAP_FLT(m_pData->extHeader.Dslice_thick);
      BSWAP_FLT(m_pData->extHeader.table_height);
      BSWAP_16(m_pData->extHeader.card_bt_rej);
      BSWAP_16(m_pData->extHeader.card_fr_type);
      BSWAP_FLT(m_pData->extHeader.min_bed_pos);
      BSWAP_FLT(m_pData->extHeader.max_bed_pos);
      BSWAP_16(m_pData->extHeader.der_filled);
      BSWAP_32(m_pData->extHeader.series_number);
      BSWAP_32(m_pData->extHeader.dep_study_date);
      BSWAP_32(m_pData->extHeader.dep_study_time);
      BSWAP_32(m_pData->extHeader.dep_acq_time);
      BSWAP_16(m_pData->extHeader.card_slc_dir);
      BSWAP_32(m_pData->extHeader.card_skip_msec);
      BSWAP_32(m_pData->extHeader.card_skip_counts);
      BSWAP_32(m_pData->extHeader.card_dur_msec);
      BSWAP_32(m_pData->extHeader.card_dur_counts);
      BSWAP_32(m_pData->extHeader.card_beats_tot);
      BSWAP_32(m_pData->extHeader.card_beats_acc);
      BSWAP_16(m_pData->extHeader.card_skip_beats);
      BSWAP_16(m_pData->extHeader.pvc_threshold);
      BSWAP_32(m_pData->extHeader.dep_acq_date);
      BSWAP_16(m_pData->extHeader.slc_add);
      BSWAP_16(m_pData->extHeader.slc_space);
      BSWAP_16(m_pData->extHeader.slc_thick);
      BSWAP_16(m_pData->extHeader.frame_add);
      BSWAP_16(m_pData->extHeader.frame_space);
      BSWAP_16(m_pData->extHeader.frame_thick);
      BSWAP_16(m_pData->extHeader.fltr_type);
      BSWAP_16(m_pData->extHeader.smoth);
      BSWAP_16(m_pData->extHeader.scatcorr_type);
      BSWAP_16(m_pData->extHeader.edge_exp);
      BSWAP_16(m_pData->extHeader.bckang_avg);
      BSWAP_FLT(m_pData->extHeader.bck_coeff);
      BSWAP_16(m_pData->extHeader.bck_wid);
      BSWAP_16(m_pData->extHeader.attncor_type);
      BSWAP_16(m_pData->extHeader.attncor_ecc);
      BSWAP_FLT(m_pData->extHeader.attn_coeff);
      BSWAP_FLT(m_pData->extHeader.skull_comp);
      BSWAP_16(m_pData->extHeader.norm_type);
      BSWAP_16(m_pData->extHeader.smp_norm);
      BSWAP_16(m_pData->extHeader.gap_comp);
      BSWAP_16(m_pData->extHeader.algtype_em);
      BSWAP_16(m_pData->extHeader.num_iter);
      BSWAP_16(m_pData->extHeader.iter_em);
      BSWAP_16(m_pData->extHeader.subset_em);
      BSWAP_16(m_pData->extHeader.nsmooth_em);
      BSWAP_16(m_pData->extHeader.nrepeat_em);
      BSWAP_16(m_pData->extHeader.bckslc_avg);
      BSWAP_16(m_pData->extHeader.dead_corr);
      BSWAP_16(m_pData->extHeader.decay_corr);
      BSWAP_FLT(m_pData->extHeader.tran_ray_fwhm);
      BSWAP_FLT(m_pData->extHeader.tran_axl_fwhm);
      BSWAP_16(m_pData->extHeader.surv_mask);
      BSWAP_16(m_pData->extHeader.preflt_type);
      BSWAP_16(m_pData->extHeader.postflt_type);
      BSWAP_16(m_pData->extHeader.tr_posttyp);
      BSWAP_16(m_pData->extHeader.algtype_tr);
      BSWAP_16(m_pData->extHeader.iter_tr);
      BSWAP_16(m_pData->extHeader.subset_tr);
      BSWAP_16(m_pData->extHeader.nsmooth_tr);
      BSWAP_16(m_pData->extHeader.nrepeat_tr);
      BSWAP_16(m_pData->extHeader.attn_corr_3d);
      BSWAP_16(m_pData->extHeader.ramla_no_it);
      BSWAP_16(m_pData->extHeader.ramla_sysac);
      BSWAP_FLT(m_pData->extHeader.ramla_lambda[0]);
      BSWAP_FLT(m_pData->extHeader.ramla_lambda[1]);
      BSWAP_FLT(m_pData->extHeader.ramla_lambda[2]);
      BSWAP_FLT(m_pData->extHeader.ramla_lambda[3]);
      BSWAP_FLT(m_pData->extHeader.ramla_lambda[4]);
      BSWAP_FLT(m_pData->extHeader.ramla_blrad);
      BSWAP_FLT(m_pData->extHeader.ramla_blalpha);
      BSWAP_FLT(m_pData->extHeader.ramla_bcc_rsz);
      BSWAP_32(m_pData->extHeader.recon_date_time);
      BSWAP_16(m_pData->extHeader.gating_type);
      BSWAP_32(m_pData->extHeader.start_table_pos_abs);
      BSWAP_32(m_pData->extHeader.start_table_pos_rel);
      BSWAP_16(m_pData->extHeader.mr_valid);
    }
  }
  
#if defined(DEBUG)
  D("philips Main Header loaded:");
  D("--------------------------");
  D("file_fmt:        : %d", m_pData->header.file_fmt);
  D("scan_geom        : %d", m_pData->header.scan_geom);
  D("hw_config        : %d", m_pData->header.hw_config);
  D("edit_flag        : %d", m_pData->header.edit_flag);
  D("filtyp           : %d", m_pData->header.filtyp);
  D("minTransXtalDiff : %ld", m_pData->header.minTransXtalDiff);
  D("tofTstampScale   : %f", m_pData->header.tofTstampScale);
  D("dep_daycre       : %d", m_pData->header.dep_daycre);
  D("dep_mocre        : %d", m_pData->header.dep_mocre);
  D("dep_yrcre        : %d", m_pData->header.dep_yrcre);
  D("dep_hrcre        : %d", m_pData->header.dep_hrcre);
  D("dep_mincre       : %d", m_pData->header.dep_mincre);
  D("dep_seccre       : %d", m_pData->header.dep_seccre);
  D("duratn           : %d", m_pData->header.duratn);
  D("shdtyp           : %d", m_pData->header.shdtyp);
  D("sngpscl          : %d", m_pData->header.sngpscl);
  D("pscale           : %f", m_pData->header.pscale);
  D("detectorRadius   : %f", m_pData->header.detectorRadius);
  D("virtualXtal      : %d", m_pData->header.virtualXtal);
  D("phiMashing       : %d", m_pData->header.phiMashing);
  D("polygonSides     : %d", m_pData->header.polygonSides);
  D("xtalsPerSide     : %d", m_pData->header.xtalsPerSide);
  D("nXtalRows        : %d", m_pData->header.nXtalRows);
  D("crystalThickness : %f", m_pData->header.crystalThickness);
  D("xXtalPitch       : %f", m_pData->header.xXtalPitch);
  D("zXtalPitch       : %f", m_pData->header.zXtalPitch);
  D("axialFOV         : %f", m_pData->header.axialFOV);
  D("rphiType         : %d", m_pData->header.rphiType);
  D("sliceType        : %d", m_pData->header.sliceType);
  D("delayType        : %d", m_pData->header.delayType);
  D("pattyp           : %d", m_pData->header.pattyp);
  D("scntyp           : %d", m_pData->header.scntyp);
  D("numray           : %d", m_pData->header.numray);
  D("numang           : %d", m_pData->header.numang);
  D("slcthk           : %d", m_pData->header.slcthk);
  D("isotop           : %d", m_pData->header.isotop);
  D("slope            : %f", m_pData->header.slope);
  D("intcpt           : %f", m_pData->header.intcpt);
  D("injtim           : %d", m_pData->header.injtim);
  D("polygonVertAt0deg: %f", m_pData->header.polygonVertAt0deg);
  D("nslice           : %d", m_pData->header.nslice);
  D("nframe           : %d", m_pData->header.nframe);
  D("bthday           : %d", m_pData->header.bthday);
  D("bthmo            : %d", m_pData->header.bthmo);
  D("bthyr            : %d", m_pData->header.bthyr);
  D("ssn              : %s", m_pData->header.ssn);
  D("ntilt            : %d", m_pData->header.ntilt);
  D("petnum           : %d", m_pData->header.petnum);
  D("activity         : %f", m_pData->header.activity);
  D("weight           : %ld", m_pData->header.weight);
  D("hrinj            : %d", m_pData->header.hrinj);
  D("mininj           : %d", m_pData->header.mininj);
  D("srcRadius        : %f", m_pData->header.srcRadius);
  D("srcZpos          : %f", m_pData->header.srcZpos);
  D("halfLife         : %f", m_pData->header.halfLife);
  D("concfac          : %f", m_pData->header.concfac);
  D("concfac_bgsub    : %f", m_pData->header.concfac_bgsub);
  D("dmax             : %f", m_pData->header.dmax);
  D("dline            : %f", m_pData->header.dline);
  D("angmax           : %f", m_pData->header.angmax);
  D("x0               : %f", m_pData->header.x0);
  D("y0               : %f", m_pData->header.y0);
  D("z0               : %f", m_pData->header.z0);
  D("nevent           : %f", m_pData->header.nevent);
  D("nsino            : %f", m_pData->header.nsino);
  D("eglob_low        : %d", m_pData->header.eglob_low);
  D("eglob_up         : %d", m_pData->header.eglob_up);
  D("eloc_low         : %d", m_pData->header.eloc_low);
  D("eloc_up          : %d", m_pData->header.eloc_up);
  D("orient_hf        : %d", m_pData->header.orient_hf);
  D("scan_swrel       : %s", m_pData->header.scan_swrel);
  D("tbl_direction    : %d", m_pData->header.tbl_direction);
  D("orient_ps        : %d", m_pData->header.orient_ps);
  D("frontLeadDiameter: %f", m_pData->header.frontLeadDiameter);
  D("backLeadDiameter : %f", m_pData->header.backLeadDiameter);
  D("leadSeparation   : %f", m_pData->header.leadSeparation);
  D("ndelays          : %f", m_pData->header.ndelays);
  D("slcsep           : %f", m_pData->header.slcsep);
  D("fctrfil          : %s", m_pData->header.fctrfil);
  D("baselin          : %s", m_pData->header.baselin);
  D("dstpkfl          : %s", m_pData->header.dstpkfl);
  D("aqprotocol_name  : %s", m_pData->header.aqprotocol_name);
  D("aqprotocol_type  : %d", m_pData->header.aqprotocol_type);
  D("patient_name     : %s", m_pData->header.patient_name);
  D("reslice_ang1     : %f", m_pData->header.reslice_ang1);
  D("reslice_ang2     : %f", m_pData->header.reslice_ang2);
  D("reslice_ang3     : %f", m_pData->header.reslice_ang3);
  D("minslc           : %d", m_pData->header.minslc);
  D("maxslc           : %d", m_pData->header.maxslc);
  D("minfrm           : %d", m_pData->header.minfrm);
  D("maxfrm           : %d", m_pData->header.maxfrm);
  D("scanner_maxslice : %d", m_pData->header.scanner_maxslice);
  D("rebin_type       : %d", m_pData->header.rebin_type);
  D("scnOrigin        : %s", m_pData->header.scnOrigin);
  D("accNum           : %s", m_pData->header.accNum);
  D("movementCoinc    : %d", m_pData->header.movementCoinc);
  D("movementSing     : %d", m_pData->header.movementSing);
  D("crbTstampPeriod  : %d", m_pData->header.crbTstampPeriod);
  D("trailexists      : %d", m_pData->header.trailexists);
  D("trailbeg         : %ld", m_pData->header.trailbeg);
  D("--PETCT--");
  D("petct_valid                  : %d", m_pData->header.petct_valid);
  D("petct_separation             : %d", m_pData->header.petct_sepdist);
  D("petct_landmark               : %d", m_pData->header.petct_landmrk);
  D("petct_alignment_timestamp    : %d", m_pData->header.petct_timestamp);
  D("petct_alignment_zOffset      : %d", m_pData->header.petct_zoffset);
  D("petct_alignment_xShift       : %d", m_pData->header.petct_xshift);
  D("petct_alignment_yShift       : %d", m_pData->header.petct_yshift);
  D("petct_alignment_zShift       : %d", m_pData->header.petct_zshift);
  D("petct_alignment_acFlags      : %d", m_pData->header.petct_acqflgs);
  D("petct_alignment_xOffset      : %d", m_pData->header.petct_xoffset);
  D("petct_alignment_yOffset      : %d", m_pData->header.petct_yoffset);
  D("petct_alignment_axialRotation: %d", m_pData->header.petct_axrot);
  D("petct_alignment_horizRotation: %d", m_pData->header.petct_horzrot);
  D("petct_alignment_vertRotation : %d", m_pData->header.petct_vertrot);

  if(extendedHeaderFound == true)
  {
    D("Philips extended MainHeader loaded:");
    D("----------------------------------");
    D("Dpat_name                : %s", m_pData->extHeader.Dpat_name);
    D("Dpat_id                  : %s", m_pData->extHeader.Dpat_id);
    D("study_uid                : %s", m_pData->extHeader.study_uid);
    D("series_uid               : %s", m_pData->extHeader.series_uid);
  
    D("view_code                : %s", m_pData->extHeader.view_code);
    D("sortproto_name           : %s", m_pData->extHeader.sortproto_name);
    D("route                    : %d", m_pData->extHeader.route);
    D("pharm                    : %d", m_pData->extHeader.pharm);
    D("req_phys                 : %s", m_pData->extHeader.req_phys);
    D("card_phstate             : %d", m_pData->extHeader.card_phstate);
    D("assay_date               : %d", m_pData->extHeader.assay_date);
    D("assay_time               : %s", QDateTime::fromTime_t(m_pData->extHeader.assay_time).toString().toAscii().constData());
    D("series_desc              : %s", m_pData->extHeader.series_desc);
    D("height                   : %d", m_pData->extHeader.height);
    D("abundance                : %d", m_pData->extHeader.abundance);
    D("realignment.xOffset      : %d", m_pData->extHeader.realign_x);
    D("realignment.yOffset      : %d", m_pData->extHeader.realign_y);
    D("realignment.horizRotation: %d", m_pData->extHeader.realign_hr);
    D("acq_date_time            : %s", QDateTime::fromTime_t(m_pData->extHeader.acq_date_time).toString().toAscii().constData());
    D("study_date_time          : %s", QDateTime::fromTime_t(m_pData->extHeader.study_date_time).toString().toAscii().constData());
    D("injection_date_time      : %s", QDateTime::fromTime_t(m_pData->extHeader.injection_date_time).toString().toAscii().constData());
    D("file_create_date_time    : %s", QDateTime::fromTime_t(m_pData->extHeader.file_create_date_time).toString().toAscii().constData());
    D("resp_trig_loc            : %d", m_pData->extHeader.resp_trig_loc);
    D("card_arrhythmia_rej_tech : %d", m_pData->extHeader.card_arrhythmia_rej_tech);
    D("window_center            : %d", m_pData->extHeader.window_center);
    D("window_width             : %d", m_pData->extHeader.window_width);
    D("realignment_axialRotation: %d", m_pData->extHeader.realign_zr);
    D("realignment_verRotation  : %d", m_pData->extHeader.realign_vr);
    D("resp_trig_threshold      : %d", m_pData->extHeader.resp_trig_threshold);
    D("resp_phase_duration      : %d", m_pData->extHeader.resp_phase_duration);
    D("resp_phase_offset        : %d", m_pData->extHeader.resp_phase_offset);
    D("realignment.zOffset      : %d", m_pData->extHeader.realign_z);
    D("window_units             : %d", m_pData->extHeader.window_units);
  
    D("referring_physician      : %s", m_pData->extHeader.referring_physician);
    D("study_id                 : %s", m_pData->extHeader.study_id);
  
    D("Dslice_thick             : %f", m_pData->extHeader.Dslice_thick);
    D("sex                      : %c", m_pData->extHeader.sex);
    D("table_height             : %f", m_pData->extHeader.table_height);
    D("card_bt_rej              : %d", m_pData->extHeader.card_bt_rej);
    D("card_fr_type             : %d", m_pData->extHeader.card_fr_type);
    D("Dmanufacture_model_name  : %s", m_pData->extHeader.Dmanufacture_model_name);
    D("Dimage_type              : %s", m_pData->extHeader.Dimage_type);
    D("min_bed_pos              : %f", m_pData->extHeader.min_bed_pos);
    D("max_bed_pos              : %f", m_pData->extHeader.max_bed_pos);
    D("der_filled               : %d", m_pData->extHeader.der_filled);
    D("series_number            : %d", m_pData->extHeader.series_number);
    D("dep_study_date           : %ld", m_pData->extHeader.dep_study_date);
    D("dep_study_time           : %ld", m_pData->extHeader.dep_study_time);
    D("dep_acq_time             : %ld", m_pData->extHeader.dep_acq_time);
    D("card_slc_dir             : %d", m_pData->extHeader.card_slc_dir);
  
    D("card_skip_msec:          : %d", m_pData->extHeader.card_skip_msec);
    D("card_skip_counts         : %d", m_pData->extHeader.card_skip_counts);
    D("card_dur_msec            : %d", m_pData->extHeader.card_dur_msec);
    D("card_dur_counts          : %d", m_pData->extHeader.card_dur_counts);
    D("card_beats_tot           : %d", m_pData->extHeader.card_beats_tot);
    D("card_beats_acc           : %d", m_pData->extHeader.card_beats_acc);
    D("card_skip_beats          : %d", m_pData->extHeader.card_skip_beats);
    D("pvc_threshold            : %d", m_pData->extHeader.pvc_threshold);
    D("dep_acq_date             : %ld", m_pData->extHeader.dep_acq_date);
    D("radiopharm_name          : %s", m_pData->extHeader.radiopharm_name);
    D("Dserial_number           : %s", m_pData->extHeader.Dserial_number);
    D("attncor_label            : %s", m_pData->extHeader.attncor_label);
  
    D("contr_bolus_agent        : %s", m_pData->extHeader.contr_bolus_agent);
    D("sop_uid                  : %s", m_pData->extHeader.sop_uid);
    D("frame_ref_uid            : %s", m_pData->extHeader.frame_ref_uid);
    D("pps_file                 : %s", m_pData->extHeader.pps_file);
    D("worklist_file            : %s", m_pData->extHeader.worklist_file);
  }
  else
    W("no extended header found");
#endif

  RETURN(true);
  return true;
}

bool CPhilipsMainHeader::save(void) const
{
  ENTER();
  bool result = false;

  SHOWPOINTER(m_pMedIOData);

  // only go on if the device is writeable at all
  if(m_pMedIOData == NULL ||
     m_pMedIOData->isWritable() == false ||
     m_pMedIOData->seek(0) == false)
  {
    RETURN(false);
    return false;
  }

  SHOWVALUE(m_pMedIOData->pos());

  // check that the sizes are absolutely correct
  ASSERT(sizeof(m_pData->header) == MAINHEADER_SIZE);
  ASSERT(sizeof(m_pData->extHeader) == EXTHEADER_SIZE);

  // before we can start reading out some data we have to collect some
  // out data beforehand which we use instead of the data stored in our
  // data structure (such as frames/slices/tilts etc.)
  CPhilipsFile* philipsFile = static_cast<CPhilipsFile*>(m_pMedIOData);
  m_pData->header.minfrm = philipsFile->minFrame();
  m_pData->header.maxfrm = philipsFile->maxFrame();
  m_pData->header.nframe = philipsFile->numFrames();
  m_pData->header.minslc = philipsFile->minSlice();
  m_pData->header.maxslc = philipsFile->maxSlice();
  m_pData->header.nslice = philipsFile->numSlices();
  m_pData->header.ntilt = philipsFile->numTilts();
  
  // now that we have streamed in all data in one run we
  // have to take care of correct endianness in the non-char
  // entries in the header structure in case this is a little endian
  // machine
  struct CPhilipsMainHeaderPrivate::HeaderData* header = NULL;
  struct CPhilipsMainHeaderPrivate::ExtHeaderData* extHeader = NULL;
  if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
  {
    header = new CPhilipsMainHeaderPrivate::HeaderData;

    // copy the current m_pData->header to beHeader
    memcpy(header, &m_pData->header, sizeof(m_pData->header));

    // we only swap non-char elements of the header
    BSWAP_16(header->file_fmt);
    BSWAP_16(header->scan_geom);           
    BSWAP_16(header->hw_config);           
    BSWAP_16(header->edit_flag);           
    BSWAP_16(header->filtyp);             
    BSWAP_32(header->minTransXtalDiff);  
    BSWAP_FLT(header->tofTstampScale);  
    BSWAP_16(header->dep_daycre);      
    BSWAP_16(header->dep_mocre);      
    BSWAP_16(header->dep_yrcre);     
    BSWAP_16(header->dep_hrcre);    
    BSWAP_16(header->dep_mincre);  
    BSWAP_16(header->dep_seccre);        
    BSWAP_16(header->duratn);            
    BSWAP_16(header->shdtyp); 
    BSWAP_16(header->sngpscl); 
    BSWAP_16(header->singopt);
    BSWAP_FLT(header->pscale);
    BSWAP_FLT(header->detectorRadius);
    BSWAP_16(header->virtualXtal);
    BSWAP_16(header->phiMashing);
    BSWAP_16(header->polygonSides);
    BSWAP_16(header->xtalsPerSide);
    BSWAP_16(header->nXtalRows);
    BSWAP_FLT(header->crystalThickness);
    BSWAP_FLT(header->xXtalPitch);
    BSWAP_FLT(header->zXtalPitch);
    BSWAP_FLT(header->axialFOV);
    BSWAP_16(header->rphiType);
    BSWAP_16(header->sliceType);
    BSWAP_16(header->delayType);
    BSWAP_16(header->pattyp);
    BSWAP_16(header->scntyp);
    BSWAP_16(header->numray);
    BSWAP_16(header->numang);
    BSWAP_16(header->slcthk);
    BSWAP_16(header->isotop);
    BSWAP_FLT(header->slope);
    BSWAP_FLT(header->intcpt);
    BSWAP_16(header->injtim);
    BSWAP_FLT(header->polygonVertAt0deg);
    BSWAP_16(header->nslice);
    BSWAP_16(header->nframe);
    BSWAP_16(header->bthday);
    BSWAP_16(header->bthmo);
    BSWAP_16(header->bthyr);
    BSWAP_16(header->ntilt);
    BSWAP_16(header->petnum);
    BSWAP_FLT(header->activity);
    BSWAP_32(header->weight);
    BSWAP_16(header->hrinj);
    BSWAP_16(header->mininj);
    BSWAP_FLT(header->srcRadius);
    BSWAP_FLT(header->srcZpos);
    BSWAP_FLT(header->halfLife);
    BSWAP_FLT(header->concfac);
    BSWAP_FLT(header->concfac_bgsub);
    BSWAP_FLT(header->dmax);
    BSWAP_FLT(header->dline);
    BSWAP_FLT(header->angmax);
    BSWAP_FLT(header->x0);
    BSWAP_FLT(header->y0);
    BSWAP_FLT(header->z0);
    BSWAP_FLT(header->nevent);
    BSWAP_FLT(header->nsino);
    BSWAP_16(header->eglob_low);
    BSWAP_16(header->eglob_up);
    BSWAP_16(header->eloc_low);
    BSWAP_16(header->eloc_up);
    BSWAP_16(header->orient_hf);
    BSWAP_16(header->petct_sepdist);
    BSWAP_16(header->petct_landmrk);
    BSWAP_32(header->petct_timestamp);
    BSWAP_16(header->tbl_direction);
    BSWAP_16(header->orient_ps);
    BSWAP_16(header->petct_zoffset);
    BSWAP_16(header->petct_xshift);
    BSWAP_16(header->petct_yshift);
    BSWAP_16(header->petct_zshift);
    BSWAP_16(header->petct_acqflgs);
    BSWAP_16(header->petct_xoffset);
    BSWAP_16(header->petct_yoffset);
    BSWAP_16(header->petct_axrot);
    BSWAP_16(header->petct_horzrot);
    BSWAP_16(header->petct_vertrot);
    BSWAP_FLT(header->frontLeadDiameter);
    BSWAP_FLT(header->backLeadDiameter);
    BSWAP_FLT(header->leadSeparation);
    BSWAP_FLT(header->ndelays);
    BSWAP_FLT(header->slcsep);
    BSWAP_16(header->petct_valid);
    BSWAP_16(header->aqprotocol_type);
    BSWAP_FLT(header->reslice_ang1);
    BSWAP_FLT(header->reslice_ang2);
    BSWAP_FLT(header->reslice_ang3);
    BSWAP_16(header->minslc);
    BSWAP_16(header->maxslc);
    BSWAP_16(header->minfrm);
    BSWAP_16(header->maxfrm);
    BSWAP_16(header->scanner_maxslice);
    BSWAP_16(header->rebin_type);
    BSWAP_16(header->movementCoinc);
    BSWAP_16(header->movementSing);
    BSWAP_16(header->crbTstampPeriod);
    BSWAP_16(header->trailexists);
    BSWAP_32(header->trailbeg);

    // lets process the extended main header
    extHeader = new CPhilipsMainHeaderPrivate::ExtHeaderData;

    // copy the current m_pData->extHeader to extHeader
    memcpy(extHeader, &m_pData->extHeader, sizeof(m_pData->extHeader));

    // we only swap non-char elements of the header
    BSWAP_16(extHeader->route);
    BSWAP_16(extHeader->pharm);
    BSWAP_16(extHeader->card_phstate);
    BSWAP_32(extHeader->assay_date);
    BSWAP_32(extHeader->assay_time);
    BSWAP_16(extHeader->height);   
    BSWAP_16(extHeader->abundance);
    BSWAP_16(extHeader->realign_x);
    BSWAP_16(extHeader->realign_y);
    BSWAP_16(extHeader->realign_hr);
    BSWAP_32(extHeader->acq_date_time);
    BSWAP_32(extHeader->study_date_time);
    BSWAP_32(extHeader->injection_date_time);
    BSWAP_32(extHeader->file_create_date_time);
    BSWAP_16(extHeader->resp_trig_loc);
    BSWAP_16(extHeader->card_arrhythmia_rej_tech);
    BSWAP_FLT(extHeader->window_center);
    BSWAP_FLT(extHeader->window_width);
    BSWAP_16(extHeader->realign_zr);
    BSWAP_16(extHeader->realign_vr);
    BSWAP_16(extHeader->resp_trig_threshold);
    BSWAP_16(extHeader->resp_phase_duration);
    BSWAP_16(extHeader->resp_phase_offset);
    BSWAP_16(extHeader->realign_z);
    BSWAP_16(extHeader->window_units);
    BSWAP_FLT(extHeader->Dslice_thick);
    BSWAP_FLT(extHeader->table_height);
    BSWAP_16(extHeader->card_bt_rej);
    BSWAP_16(extHeader->card_fr_type);
    BSWAP_FLT(extHeader->min_bed_pos);
    BSWAP_FLT(extHeader->max_bed_pos);
    BSWAP_16(extHeader->der_filled);
    BSWAP_32(extHeader->series_number);
    BSWAP_32(extHeader->dep_study_date);
    BSWAP_32(extHeader->dep_study_time);
    BSWAP_32(extHeader->dep_acq_time);
    BSWAP_16(extHeader->card_slc_dir);
    BSWAP_32(extHeader->card_skip_msec);
    BSWAP_32(extHeader->card_skip_counts);
    BSWAP_32(extHeader->card_dur_msec);
    BSWAP_32(extHeader->card_dur_counts);
    BSWAP_32(extHeader->card_beats_tot);
    BSWAP_32(extHeader->card_beats_acc);
    BSWAP_16(extHeader->card_skip_beats);
    BSWAP_16(extHeader->pvc_threshold);
    BSWAP_32(extHeader->dep_acq_date);
    BSWAP_16(extHeader->slc_add);
    BSWAP_16(extHeader->slc_space);
    BSWAP_16(extHeader->slc_thick);
    BSWAP_16(extHeader->frame_add);
    BSWAP_16(extHeader->frame_space);
    BSWAP_16(extHeader->frame_thick);
    BSWAP_16(extHeader->fltr_type);
    BSWAP_16(extHeader->smoth);
    BSWAP_16(extHeader->scatcorr_type);
    BSWAP_16(extHeader->edge_exp);
    BSWAP_16(extHeader->bckang_avg);
    BSWAP_FLT(extHeader->bck_coeff);
    BSWAP_16(extHeader->bck_wid);
    BSWAP_16(extHeader->attncor_type);
    BSWAP_16(extHeader->attncor_ecc);
    BSWAP_FLT(extHeader->attn_coeff);
    BSWAP_FLT(extHeader->skull_comp);
    BSWAP_16(extHeader->norm_type);
    BSWAP_16(extHeader->smp_norm);
    BSWAP_16(extHeader->gap_comp);
    BSWAP_16(extHeader->algtype_em);
    BSWAP_16(extHeader->num_iter);
    BSWAP_16(extHeader->iter_em);
    BSWAP_16(extHeader->subset_em);
    BSWAP_16(extHeader->nsmooth_em);
    BSWAP_16(extHeader->nrepeat_em);
    BSWAP_16(extHeader->bckslc_avg);
    BSWAP_16(extHeader->dead_corr);
    BSWAP_16(extHeader->decay_corr);
    BSWAP_FLT(extHeader->tran_ray_fwhm);
    BSWAP_FLT(extHeader->tran_axl_fwhm);
    BSWAP_16(extHeader->surv_mask);
    BSWAP_16(extHeader->preflt_type);
    BSWAP_16(extHeader->postflt_type);
    BSWAP_16(extHeader->tr_posttyp);
    BSWAP_16(extHeader->algtype_tr);
    BSWAP_16(extHeader->iter_tr);
    BSWAP_16(extHeader->subset_tr);
    BSWAP_16(extHeader->nsmooth_tr);
    BSWAP_16(extHeader->nrepeat_tr);
    BSWAP_16(extHeader->attn_corr_3d);
    BSWAP_16(extHeader->ramla_no_it);
    BSWAP_16(extHeader->ramla_sysac);
    BSWAP_FLT(extHeader->ramla_lambda[0]);
    BSWAP_FLT(extHeader->ramla_lambda[1]);
    BSWAP_FLT(extHeader->ramla_lambda[2]);
    BSWAP_FLT(extHeader->ramla_lambda[3]);
    BSWAP_FLT(extHeader->ramla_lambda[4]);
    BSWAP_FLT(extHeader->ramla_blrad);
    BSWAP_FLT(extHeader->ramla_blalpha);
    BSWAP_FLT(extHeader->ramla_bcc_rsz);
    BSWAP_32(extHeader->recon_date_time);
    BSWAP_16(extHeader->gating_type);
    BSWAP_32(extHeader->start_table_pos_abs);
    BSWAP_32(extHeader->start_table_pos_rel);
    BSWAP_16(extHeader->mr_valid);
  }
  else
  {
    header = &m_pData->header;
    extHeader = &m_pData->extHeader;
  }

  // now write out the main header to the file
  if(m_pMedIOData->write(reinterpret_cast<char*>(header), sizeof(m_pData->header)) == MAINHEADER_SIZE)
  {
    // now we have to check/create the extended main header part which
    // is part of the directory
    CPhilipsFile* phFile = static_cast<CPhilipsFile*>(m_pMedIOData);
    CPhilipsDirectory* philipsDirectory = phFile->directory();
    if(philipsDirectory != NULL)
    {
      CPhilipsDirectoryItem* extHeaderItem = philipsDirectory->extendedMainHeaderItem();
      if(extHeaderItem != NULL)
      {
        // now we have the extendedheader diritem. so lets get the header out of it
        if(m_pMedIOData->seek(extHeaderItem->dataBlock_Start()) == true &&
           m_pMedIOData->write(reinterpret_cast<char*>(extHeader), sizeof(m_pData->extHeader)) == EXTHEADER_SIZE)
        {
          // make sure to set the DataBlock_End()
          extHeaderItem->setDataBlock_End(extHeaderItem->dataBlock_Start() + EXTHEADER_SIZE);

          // now we write out the whole directory
          // and if this works out everything worked out fine
          if(philipsDirectory->save() == true)
            result = true;
        }
        else
          E("Error while trying to seek&save extended main header data");

        result = true;
      }
      else
        W("no extended header found in philips file");
    }
    else 
      W("directory == NULL");
  }
  else
    E("error on write() of main header");

  if(result == true)
    philipsFile->mainHeaderWritten(*this);

  // if we byte swapped we have to delete the
  // temporary byte swapped header structures
  if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
  {
    delete header;
    delete extHeader;
  }

  RETURN(result);
  return result;
}

CMedIOHeader::Format CPhilipsMainHeader::headerFormat() const
{ 
  return CMedIOHeader::PhilipsMainHeader;
}

bool CPhilipsMainHeader::convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader* pHead2) 
{
  ENTER();
  bool bResult = false;

  // depending on the MedIOHeader format we do have to 
  // distinguish between our copy operations.
  switch(pHead1->headerFormat())
  {
    case CMedIOHeader::ECATMainHeader:
    {
      const CECATMainHeader* eMainHeader = static_cast<const CECATMainHeader*>(pHead1);

      // depending on the source type we have to copy either every data or just 
      // some data of the src header
      switch(eMainHeader->rtti())
      {
        // if the source header is also an ECAT7 one we can copy it in whole
        // via a simple memcpy()
        case CECATMainHeader::ECAT7MainHeader:
        {
          const CECAT7MainHeader* header = static_cast<const CECAT7MainHeader*>(pHead1);

          // clear everything first
          clear();

          // now convert
          CPhilipsMainHeader::File_Type fileType = CPhilipsMainHeader::Unknown;
          switch(header->file_Type())
          {
            case CECAT7MainHeader::Volume8:
            case CECAT7MainHeader::Volume16:
            case CECAT7MainHeader::Image8:
            case CECAT7MainHeader::Image16:
              fileType = CPhilipsMainHeader::Image;
            break;

            case CECAT7MainHeader::Sinogram:
            case CECAT7MainHeader::Sinogram3D_16:
            case CECAT7MainHeader::Sinogram3D_8:
            case CECAT7MainHeader::Sinogram3D_Float:
              fileType = CPhilipsMainHeader::Sinogram;
            break;
          }

          setFiltyp(fileType);
          setPatient_Name(header->patient_Name());
          setSsn(header->patient_ID());
          setWeight(header->patient_Weight() * 1000.0f); // kg -> g
          setPatient_Birth_Date_Qt(header->patient_Birth_Date_Qt());
          m_pData->ecat2philipsOrientation(header->patient_Orientation());
          setNslice(header->num_Planes());
          setNframe(header->num_Frames());
          setSlcthk(header->plane_Separation() * 10.0f); // cm -> mm
          m_pData->ecat2philipsIsotop(header->isotope_Name());
          setHalfLife(header->isotope_Halflife() / 60.0f); // sec -> min
          setActivity(header->dosage() / 1000000.0f); // Bq -> MBq
          setAqprotocol_Name(header->study_Type());
          m_pData->ecat2philipsAcquisitionType(header->acquisition_Type());
          m_pData->ecat2philipsSex(header->patient_Sex());

          bResult = true;
        }
        break;
      }
    }
  }

  RETURN(bResult);
  return bResult;
}

CMedIOHeader* CPhilipsMainHeader::clone() const
{
  ENTER();

  CPhilipsMainHeader* pNewHeader = new CPhilipsMainHeader(*this);

  RETURN(pNewHeader);
  return pNewHeader;
}

bool CPhilipsMainHeader::isPETMR() const
{
  return m_pData->header.hw_config & CPhilipsMainHeaderPrivate::PETMR;
}

short CPhilipsMainHeader::file_Fmt() const
{
  return m_pData->header.file_fmt;
}

short CPhilipsMainHeader::scan_Geom() const
{
  return m_pData->header.scan_geom;
}

short CPhilipsMainHeader::hw_Config() const
{
  return m_pData->header.hw_config;
}

short CPhilipsMainHeader::edit_Flag() const
{
  return m_pData->header.edit_flag;
}

CPhilipsMainHeader::File_Type CPhilipsMainHeader::filtyp() const
{
  return static_cast<File_Type>(m_pData->header.filtyp);
}

long CPhilipsMainHeader::minTransXtalDiff() const
{
  return m_pData->header.minTransXtalDiff;
}

float CPhilipsMainHeader::tofTstampScale() const
{
  return m_pData->header.tofTstampScale;
}

short CPhilipsMainHeader::duratn() const
{
  return m_pData->header.duratn;
}

CPhilipsMainHeader::Subheader_Type CPhilipsMainHeader::shdtyp() const
{
  return static_cast<Subheader_Type>(m_pData->header.shdtyp);
}

float CPhilipsMainHeader::pscale() const
{
  return m_pData->header.pscale;
}

float CPhilipsMainHeader::detectorRadius() const
{
  return m_pData->header.detectorRadius;
}

bool CPhilipsMainHeader::virtualXtal() const
{
  return (m_pData->header.virtualXtal == 1);
}

short CPhilipsMainHeader::phiMashing() const
{
  return m_pData->header.phiMashing;
}

short CPhilipsMainHeader::polygonSides() const
{
  return m_pData->header.polygonSides;
}

short CPhilipsMainHeader::xtalsPerSide() const
{
  return m_pData->header.xtalsPerSide;
}

short CPhilipsMainHeader::nXtalRows() const
{
  return m_pData->header.nXtalRows;
}

float CPhilipsMainHeader::crystalThickness() const
{
  return m_pData->header.crystalThickness;
}

float CPhilipsMainHeader::xXtalPitch() const
{
  return m_pData->header.xXtalPitch;
}

float CPhilipsMainHeader::zXtalPitch() const
{
  return m_pData->header.zXtalPitch;
}

float CPhilipsMainHeader::axialFOV() const
{
  return m_pData->header.axialFOV;
}

CPhilipsMainHeader::RPhi_Type CPhilipsMainHeader::rphiType() const
{
  return static_cast<RPhi_Type>(m_pData->header.rphiType);
}

CPhilipsMainHeader::Slice_Type CPhilipsMainHeader::sliceType() const
{
  return static_cast<Slice_Type>(m_pData->header.sliceType);
}

CPhilipsMainHeader::Delay_Type CPhilipsMainHeader::delayType() const
{
  return static_cast<Delay_Type>(m_pData->header.delayType);
}

CPhilipsMainHeader::Scan_Type CPhilipsMainHeader::pattyp() const
{
  return static_cast<Scan_Type>(m_pData->header.pattyp);
}

CPhilipsMainHeader::Acquisition_Type CPhilipsMainHeader::scntyp() const
{
  return static_cast<Acquisition_Type>(m_pData->header.scntyp);
}

short CPhilipsMainHeader::numray() const
{
  return m_pData->header.numray;
}

short CPhilipsMainHeader::numang() const
{
  return m_pData->header.numang;
}

short CPhilipsMainHeader::slcthk() const
{
  return m_pData->header.slcthk;
}

CPhilipsMainHeader::Isotop CPhilipsMainHeader::isotop() const
{
  return static_cast<Isotop>(m_pData->header.isotop);
}

float CPhilipsMainHeader::slope() const
{
  return m_pData->header.slope;
}

float CPhilipsMainHeader::intcpt() const
{
  return m_pData->header.intcpt;
}

short CPhilipsMainHeader::injtim() const
{
  return m_pData->header.injtim;
}

float CPhilipsMainHeader::polygonVertAt0deg() const
{
  return m_pData->header.polygonVertAt0deg;
}

short CPhilipsMainHeader::nslice() const
{
  return m_pData->header.nslice;
}

short CPhilipsMainHeader::nframe() const
{
  return m_pData->header.nframe;
}

short CPhilipsMainHeader::bthday() const
{
  return m_pData->header.bthday;
}

short CPhilipsMainHeader::bthmo() const
{
  return m_pData->header.bthmo;
}

short CPhilipsMainHeader::bthyr() const
{
  return m_pData->header.bthyr;
}

const char* CPhilipsMainHeader::ssn() const
{
  return m_pData->header.ssn;
}

short CPhilipsMainHeader::ntilt() const
{
  return m_pData->header.ntilt;
}

short CPhilipsMainHeader::petnum() const
{
  return m_pData->header.petnum;
}

float CPhilipsMainHeader::activity() const
{
  return m_pData->header.activity;
}

long CPhilipsMainHeader::weight() const
{
  return m_pData->header.weight;
}

short CPhilipsMainHeader::hrinj() const
{
  return m_pData->header.hrinj;
}

short CPhilipsMainHeader::mininj() const
{
  return m_pData->header.mininj;
}

float CPhilipsMainHeader::srcRadius() const
{
  return m_pData->header.srcRadius;
}

float CPhilipsMainHeader::srcZpos() const
{
  return m_pData->header.srcZpos;
}

float CPhilipsMainHeader::halfLife() const
{
  return m_pData->header.halfLife;
}

float CPhilipsMainHeader::concfac() const
{
  return m_pData->header.concfac;
}

float CPhilipsMainHeader::concfac_Bgsub() const
{
  return m_pData->header.concfac_bgsub;
}

float CPhilipsMainHeader::dmax() const
{
  return m_pData->header.dmax;
}

float CPhilipsMainHeader::dline() const
{
  return m_pData->header.dline;
}

float CPhilipsMainHeader::angmax() const
{
  return m_pData->header.angmax;
}

float CPhilipsMainHeader::x0() const
{
  return m_pData->header.x0;
}

float CPhilipsMainHeader::y0() const
{
  return m_pData->header.y0;
}

float CPhilipsMainHeader::z0() const
{
  return m_pData->header.z0;
}

float CPhilipsMainHeader::nevent() const
{
  return m_pData->header.nevent;
}

float CPhilipsMainHeader::nsino() const
{
  return m_pData->header.nsino;
}

short CPhilipsMainHeader::eglob_Low() const
{
  return m_pData->header.eglob_low;
}

short CPhilipsMainHeader::eglob_Up() const
{
  return m_pData->header.eglob_up;
}

short CPhilipsMainHeader::eloc_Low() const
{
  return m_pData->header.eloc_low;
}

short CPhilipsMainHeader::eloc_Up() const
{
  return m_pData->header.eloc_up;
}

CPhilipsMainHeader::Patient_Orientation_hf CPhilipsMainHeader::orient_Hf() const
{
  return static_cast<Patient_Orientation_hf>(m_pData->header.orient_hf);
}

const char* CPhilipsMainHeader::scan_Swrel() const
{
  return m_pData->header.scan_swrel;
}

CPhilipsMainHeader::Table_Direction CPhilipsMainHeader::tbl_Direction() const
{
  return static_cast<Table_Direction>(m_pData->header.tbl_direction);
}

CPhilipsMainHeader::Patient_Orientation_ps CPhilipsMainHeader::orient_Ps() const
{
  return static_cast<Patient_Orientation_ps>(m_pData->header.orient_ps);
}

float CPhilipsMainHeader::frontLeadDiameter() const
{
  return m_pData->header.frontLeadDiameter;
}

float CPhilipsMainHeader::backLeadDiameter() const
{
  return m_pData->header.backLeadDiameter;
}

float CPhilipsMainHeader::leadSeparation() const
{
  return m_pData->header.leadSeparation;
}

float CPhilipsMainHeader::ndelays() const
{
  return m_pData->header.ndelays;
}

float CPhilipsMainHeader::slcsep() const
{
  return m_pData->header.slcsep;
}

const char* CPhilipsMainHeader::fctrfil() const
{
  return m_pData->header.fctrfil;
}

const char* CPhilipsMainHeader::baselin() const
{
  return m_pData->header.baselin;
}

const char* CPhilipsMainHeader::dstpkfl() const
{
  return m_pData->header.dstpkfl;
}

const char* CPhilipsMainHeader::aqprotocol_Name() const
{
  return m_pData->header.aqprotocol_name;
}

CPhilipsMainHeader::Acquisition_Protocol_Type CPhilipsMainHeader::aqprotocol_Type() const
{
  return static_cast<Acquisition_Protocol_Type>(m_pData->header.aqprotocol_type);
}

const char* CPhilipsMainHeader::patient_Name() const
{
  return m_pData->header.patient_name;
}

float CPhilipsMainHeader::reslice_Ang1() const
{
  return m_pData->header.reslice_ang1;
}

float CPhilipsMainHeader::reslice_Ang2() const
{
  return m_pData->header.reslice_ang2;
}

float CPhilipsMainHeader::reslice_Ang3() const
{
  return m_pData->header.reslice_ang3;
}

short CPhilipsMainHeader::minslc() const
{
  return m_pData->header.minslc;
}

short CPhilipsMainHeader::maxslc() const
{
  return m_pData->header.maxslc;
}

short CPhilipsMainHeader::minfrm() const
{
  return m_pData->header.minfrm;
}

short CPhilipsMainHeader::maxfrm() const
{
  return m_pData->header.maxfrm;
}

short CPhilipsMainHeader::scanner_Maxslice() const
{
  return m_pData->header.scanner_maxslice;
}

CPhilipsMainHeader::Rebin_Type CPhilipsMainHeader::rebin_Type() const
{
  return static_cast<Rebin_Type>(m_pData->header.rebin_type);
}

const char* CPhilipsMainHeader::scnOrigin() const
{
  return m_pData->header.scnOrigin;
}

const char* CPhilipsMainHeader::accNum() const
{
  return m_pData->header.accNum;
}

short CPhilipsMainHeader::movementCoinc() const
{
  return m_pData->header.movementCoinc;
}

short CPhilipsMainHeader::movementSing() const
{
  return m_pData->header.movementSing;
}

short CPhilipsMainHeader::crbTstampPeriod() const
{
  return m_pData->header.crbTstampPeriod;
}

short CPhilipsMainHeader::trailexists() const
{
  return m_pData->header.trailexists;
}

unsigned long CPhilipsMainHeader::trailbeg() const
{
  return m_pData->header.trailbeg;
}

CPhilipsMainHeader::Valid_Header_Struct CPhilipsMainHeader::petct_Valid() const
{
  return static_cast<Valid_Header_Struct>(m_pData->header.petct_valid);
}

short CPhilipsMainHeader::petct_separation() const
{
  return m_pData->header.petct_sepdist;
}

short CPhilipsMainHeader::petct_landmark() const
{
  return m_pData->header.petct_landmrk;
}

time_t CPhilipsMainHeader::petct_alignment_timestamp() const
{
  return m_pData->header.petct_timestamp;
}

short CPhilipsMainHeader::petct_alignment_zOffset() const
{
  return m_pData->header.petct_zoffset;
}

short CPhilipsMainHeader::petct_alignment_xShift() const
{
  return m_pData->header.petct_xshift;
}

short CPhilipsMainHeader::petct_alignment_yShift() const
{
  return m_pData->header.petct_yshift;
}

short CPhilipsMainHeader::petct_alignment_zShift() const
{
  return m_pData->header.petct_zshift;
}

short CPhilipsMainHeader::petct_alignment_acFlags() const
{
  return m_pData->header.petct_acqflgs;
}

short CPhilipsMainHeader::petct_alignment_xOffset() const
{
  return m_pData->header.petct_xoffset;
}

short CPhilipsMainHeader::petct_alignment_yOffset() const
{
  return m_pData->header.petct_yoffset;
}

short CPhilipsMainHeader::petct_alignment_axialRotation() const
{
  return m_pData->header.petct_axrot;
}

short CPhilipsMainHeader::petct_alignment_horizRotation() const
{
  return m_pData->header.petct_horzrot;
}

short CPhilipsMainHeader::petct_alignment_vertRotation() const
{
  return m_pData->header.petct_vertrot;
}

void CPhilipsMainHeader::setFile_Fmt(const short format)
{
  m_pData->header.file_fmt = format;
}

void CPhilipsMainHeader::setScan_Geom(const short geometry)
{
  m_pData->header.scan_geom = geometry;
}

void CPhilipsMainHeader::setHw_Config(const short conifg)
{
  m_pData->header.hw_config = conifg;
}

void CPhilipsMainHeader::setEdit_Flag(const short eFlag)
{
  m_pData->header.edit_flag = eFlag;
}

void CPhilipsMainHeader::setFiltyp(const File_Type fType)
{
  m_pData->header.filtyp = fType;
}

void CPhilipsMainHeader::setMinTransXtalDiff(const long minTransXtalDiff)
{
  m_pData->header.minTransXtalDiff = minTransXtalDiff;
}

void CPhilipsMainHeader::setTofTstampScale(const float tofTstampScale)
{
  m_pData->header.tofTstampScale = tofTstampScale;
}

void CPhilipsMainHeader::setDuratn(const short seconds)
{
  m_pData->header.duratn = seconds;
}

void CPhilipsMainHeader::setShdtyp(const Subheader_Type sType)
{
  m_pData->header.shdtyp = sType;
}

void CPhilipsMainHeader::setPscale(const float preScale)
{
  m_pData->header.pscale = preScale;
}

void CPhilipsMainHeader::setDetectorRadius(const float radius)
{
  m_pData->header.detectorRadius = radius;
}

void CPhilipsMainHeader::setVirtualXtal(const bool virtualCrystals)
{
  m_pData->header.virtualXtal = virtualCrystals;
}
void CPhilipsMainHeader::setPhiMashing(const short phiMashing)
{
  m_pData->header.phiMashing = phiMashing;
}

void CPhilipsMainHeader::setPolygonSides(const short polygonSides)
{
  m_pData->header.polygonSides = polygonSides;
}

void CPhilipsMainHeader::setXtalsPerSide(const short crystalsPerSide)
{
  m_pData->header.xtalsPerSide = crystalsPerSide;
}

void CPhilipsMainHeader::setnXtalRows(const short crystalRows)
{
  m_pData->header.nXtalRows = crystalRows;
}

void CPhilipsMainHeader::setCrystalThickness(const float crystalThickness)
{
  m_pData->header.crystalThickness = crystalThickness;
}

void CPhilipsMainHeader::setXXtalPitch(const float pitch)
{
  m_pData->header.xXtalPitch = pitch;
}

void CPhilipsMainHeader::setZXtalPitch(const float pitch)
{
  m_pData->header.zXtalPitch = pitch;
}

void CPhilipsMainHeader::setAxialFOV(const float axialFOV)
{
  m_pData->header.axialFOV = axialFOV;
}

void CPhilipsMainHeader::setRphiType(const RPhi_Type rType)
{
  m_pData->header.rphiType = rType;
}

void CPhilipsMainHeader::setSliceType(const Slice_Type sType)
{
  m_pData->header.sliceType = sType;
}

void CPhilipsMainHeader::setDelayType(const Delay_Type dType)
{
  m_pData->header.delayType = dType;
}

void CPhilipsMainHeader::setPattyp(const Scan_Type sType)
{
  m_pData->header.pattyp = sType;
}

void CPhilipsMainHeader::setScntyp(const CPhilipsMainHeader::Acquisition_Type aType)
{
  m_pData->header.scntyp = aType;
}

void CPhilipsMainHeader::setNumray(const short num)
{
  m_pData->header.numray = num;
}

void CPhilipsMainHeader::setNumang(const short num)
{
  m_pData->header.numang = num;
}

void CPhilipsMainHeader::setSlcthk(const short thickness)
{
  m_pData->header.slcthk = thickness;
}

void CPhilipsMainHeader::setIsotop(const Isotop isotop)
{
  m_pData->header.isotop = isotop;
}

void CPhilipsMainHeader::setSlope(const float slope)
{
  m_pData->header.slope = slope;
}

void CPhilipsMainHeader::setIntcpt(const float intercept)
{
  m_pData->header.intcpt = intercept;
}

void CPhilipsMainHeader::setInjtim(const short seconds)
{
  m_pData->header.injtim = seconds;
}

void CPhilipsMainHeader::setPolygonVertAt0deg(const float vertex)
{
  m_pData->header.polygonVertAt0deg = vertex;
}

void CPhilipsMainHeader::setNslice(const short num)
{
  m_pData->header.nslice = num;
}

void CPhilipsMainHeader::setNframe(const short num)
{
  m_pData->header.nframe = num;
}

void CPhilipsMainHeader::setBthday(const short day)
{
  m_pData->header.bthday = day;
}

void CPhilipsMainHeader::setBthmo(const short month)
{
  m_pData->header.bthmo = month;
}

void CPhilipsMainHeader::setBthyr(const short year)
{
  m_pData->header.bthyr = year;
}

void CPhilipsMainHeader::setSsn(const char* id)
{
  strncpy(m_pData->header.ssn, id, sizeof(m_pData->header.ssn));
}

void CPhilipsMainHeader::setNtilt(const short num)
{
  m_pData->header.ntilt = num;
}

void CPhilipsMainHeader::setPetnum(const short petnum)
{
  m_pData->header.petnum = petnum;
}

void CPhilipsMainHeader::setActivity(const float activity)
{
  m_pData->header.activity = activity;
}

void CPhilipsMainHeader::setWeight(const long weight)
{
  m_pData->header.weight = weight;
}

void CPhilipsMainHeader::setHrinj(const short hrinj)
{
  m_pData->header.hrinj = hrinj;
}

void CPhilipsMainHeader::setMininj(const short mininj)
{
  m_pData->header.mininj = mininj;
}

void CPhilipsMainHeader::setSrcRadius(const float srcRadius)
{
  m_pData->header.srcRadius = srcRadius;
}

void CPhilipsMainHeader::setSrcZpos(const float srcZpos)
{
  m_pData->header.srcZpos = srcZpos;
}

void CPhilipsMainHeader::setHalfLife(const float halfLife)
{
  m_pData->header.halfLife = halfLife;
}

void CPhilipsMainHeader::setConcfac(const float concfac)
{
  m_pData->header.concfac = concfac;
}

void CPhilipsMainHeader::setConcfac_Bgsub(const float concfac_bgsub)
{
  m_pData->header.concfac_bgsub = concfac_bgsub;
}

void CPhilipsMainHeader::setDmax(const float dmax)
{
  m_pData->header.dmax = dmax;
}

void CPhilipsMainHeader::setDline(const float dline)
{
  m_pData->header.dline = dline;
}

void CPhilipsMainHeader::setAngmax(const float angmax)
{
  m_pData->header.angmax = angmax;
}

void CPhilipsMainHeader::setX0(const float x0)
{
  m_pData->header.x0 = x0;
}

void CPhilipsMainHeader::setY0(const float y0)
{
  m_pData->header.y0 = y0;
}

void CPhilipsMainHeader::setZ0(const float z0)
{
  m_pData->header.z0 = z0;
}

void CPhilipsMainHeader::setNevent(const float nevent)
{
  m_pData->header.nevent = nevent;
}

void CPhilipsMainHeader::setNsino(const float nsino)
{
  m_pData->header.nsino = nsino;
}

void CPhilipsMainHeader::setEglob_Low(const short eglob_low)
{
  m_pData->header.eglob_low = eglob_low;
}

void CPhilipsMainHeader::setEglob_Up(const short eglob_up)
{
  m_pData->header.eglob_up = eglob_up;
}

void CPhilipsMainHeader::setEloc_Low(const short eloc_low)
{
  m_pData->header.eloc_low = eloc_low;
}

void CPhilipsMainHeader::setEloc_Up(const short eloc_up)
{
  m_pData->header.eloc_up = eloc_up;
}

void CPhilipsMainHeader::setOrient_Hf(const CPhilipsMainHeader::Patient_Orientation_hf orient_hf)
{
  m_pData->header.orient_hf = static_cast<qint16>(orient_hf);
}

void CPhilipsMainHeader::scan_Swrel(const char* str)
{
  strncpy(m_pData->header.scan_swrel, str, sizeof(m_pData->header.scan_swrel));
}

void CPhilipsMainHeader::setTbl_Direction(const CPhilipsMainHeader::Table_Direction tbl_direction)
{
  m_pData->header.tbl_direction = static_cast<qint16>(tbl_direction);
}

void CPhilipsMainHeader::setOrient_Ps(const CPhilipsMainHeader::Patient_Orientation_ps orient_ps)
{
  m_pData->header.orient_ps = static_cast<qint16>(orient_ps);
}

void CPhilipsMainHeader::setFrontLeadDiameter(const float frontLeadDiameter)
{
  m_pData->header.frontLeadDiameter = frontLeadDiameter;
}

void CPhilipsMainHeader::setBackLeadDiameter(const float backLeadDiameter)
{
  m_pData->header.backLeadDiameter = backLeadDiameter;
}

void CPhilipsMainHeader::setLeadSeparation(const float leadSeparation)
{
  m_pData->header.leadSeparation = leadSeparation;
}

void CPhilipsMainHeader::setNdelays(const float ndelays)
{
  m_pData->header.ndelays = ndelays;
}

void CPhilipsMainHeader::setSlcsep(const float slcsep)
{
  m_pData->header.slcsep = slcsep;
}

void CPhilipsMainHeader::setFctrfil(const char* str)
{
  strncpy(m_pData->header.fctrfil, str, sizeof(m_pData->header.fctrfil));
}

void CPhilipsMainHeader::setBaselin(const char* str)
{
  strncpy(m_pData->header.baselin, str, sizeof(m_pData->header.baselin));
}

void CPhilipsMainHeader::setDstpkfl(const char* str)
{
  strncpy(m_pData->header.dstpkfl, str, sizeof(m_pData->header.dstpkfl));
}

void CPhilipsMainHeader::setAqprotocol_Name(const char* str)
{
  strncpy(m_pData->header.aqprotocol_name, str, sizeof(m_pData->header.aqprotocol_name));
}

void CPhilipsMainHeader::setAqprotocol_Type(const CPhilipsMainHeader::Acquisition_Protocol_Type aqprotocol_type)
{
  m_pData->header.aqprotocol_type =  static_cast<qint16>(aqprotocol_type);
}

void CPhilipsMainHeader::setPatient_Name(const char* str)
{
  strncpy(m_pData->header.patient_name, str, sizeof(m_pData->header.patient_name));
}

void CPhilipsMainHeader::setReslice_Ang1(const float reslice_ang1)
{
  m_pData->header.reslice_ang1 = reslice_ang1;
}

void CPhilipsMainHeader::setReslice_Ang2(const float reslice_ang2)
{
  m_pData->header.reslice_ang2 = reslice_ang2;
}

void CPhilipsMainHeader::setReslice_Ang3(const float reslice_ang3)
{
  m_pData->header.reslice_ang3 = reslice_ang3;
}

void CPhilipsMainHeader::setMinslc(const short minslc)
{
  m_pData->header.minslc = minslc;
}

void CPhilipsMainHeader::setMaxslc(const short maxslc)
{
  m_pData->header.maxslc = maxslc;
}

void CPhilipsMainHeader::setMinfrm(const short minfrm)
{
  m_pData->header.minfrm = minfrm;
}

void CPhilipsMainHeader::setMaxfrm(const short maxfrm)
{
  m_pData->header.maxfrm = maxfrm;
}

void CPhilipsMainHeader::setScanner_Maxslice(const short scanner_maxslice)
{
  m_pData->header.scanner_maxslice = scanner_maxslice;
}

void CPhilipsMainHeader::setRebin_Type(const Rebin_Type rebin_type)
{
  m_pData->header.rebin_type = rebin_type;
}

void CPhilipsMainHeader::setScnOrigin(const char *str)
{
  strncpy(m_pData->header.scnOrigin, str, sizeof(m_pData->header.scnOrigin));
}

void CPhilipsMainHeader::setAccNum(const char *str)
{
  strncpy(m_pData->header.accNum, str, sizeof(m_pData->header.accNum));
}

void CPhilipsMainHeader::setMovementCoinc(const short movementCoinc)
{
  m_pData->header.movementCoinc = movementCoinc;
}

void CPhilipsMainHeader::setMovementSing(const short movementSing)
{
  m_pData->header.movementSing = movementSing;
}

void CPhilipsMainHeader::setCrbTstampPeriod(const short crbTstampPeriod)
{
  m_pData->header.crbTstampPeriod = crbTstampPeriod;
}

void CPhilipsMainHeader::setTrailexists(const short trailexists)
{
  m_pData->header.trailexists = trailexists;
}

void CPhilipsMainHeader::setTrailbeg(const unsigned long trailbeg)
{
  m_pData->header.trailbeg = trailbeg;
}

void CPhilipsMainHeader::setPetct_Valid(const CPhilipsMainHeader::Valid_Header_Struct petct_valid)
{
  m_pData->header.petct_valid = static_cast<qint16>(petct_valid);
}

void CPhilipsMainHeader::setPetct_separation(const short separation)
{
  m_pData->header.petct_sepdist = separation;
}

void CPhilipsMainHeader::setPetct_landmark(const short landmark)
{
  m_pData->header.petct_landmrk = landmark;
}

void CPhilipsMainHeader::setPetct_alignment_timestamp(const time_t timestamp)
{
  m_pData->header.petct_timestamp = timestamp;
}

void CPhilipsMainHeader::setPetct_alignment_zOffset(const short zOffset)
{
  m_pData->header.petct_zoffset = zOffset;
}

void CPhilipsMainHeader::setPetct_alignment_xShift(const short xShift)
{
  m_pData->header.petct_xshift = xShift;
}

void CPhilipsMainHeader::setPetct_alignment_yShift(const short yShift)
{
  m_pData->header.petct_yshift = yShift;
}

void CPhilipsMainHeader::setPetct_alignment_zShift(const short zShift)
{
  m_pData->header.petct_zshift = zShift;
}

void CPhilipsMainHeader::setPetct_alignment_acFlags(const short acFlags)
{
  m_pData->header.petct_acqflgs = acFlags;
}

void CPhilipsMainHeader::setPetct_alignment_xOffset(const short xOffset)
{
  m_pData->header.petct_xoffset = xOffset;
}

void CPhilipsMainHeader::setPetct_alignment_yOffset(const short yOffset)
{
  m_pData->header.petct_yoffset = yOffset;
}

void CPhilipsMainHeader::setPetct_alignment_axialRotation(const short axialRotation)
{
  m_pData->header.petct_axrot = axialRotation;
}

void CPhilipsMainHeader::setPetct_alignment_horizRotation(const short horizRotation)
{
  m_pData->header.petct_horzrot = horizRotation;
}

void CPhilipsMainHeader::setPetct_alignment_vertRotation(const short vertRotation)
{
  m_pData->header.petct_vertrot = vertRotation;
}

// special Qt-based methods
QDate CPhilipsMainHeader::patient_Birth_Date_Qt() const
{
  QDate birthdate(m_pData->header.bthyr, m_pData->header.bthmo, m_pData->header.bthday);
  return birthdate;
}

void CPhilipsMainHeader::setPatient_Birth_Date_Qt(const QDate& date)
{
  m_pData->header.bthyr = date.year();
  m_pData->header.bthmo = date.month();
  m_pData->header.bthday = date.day();  
}

bool CPhilipsMainHeaderPrivate::ecat2philipsSex(const CECAT7MainHeader::Patient_Sex sex)
{
  ENTER();
  bool result = false;

  switch(sex)
  {
    case CECAT7MainHeader::Sex_Male:
      extHeader.sex = 'm';
    break;

    case CECAT7MainHeader::Sex_Female:
      extHeader.sex = 'f';
    break;

    case CECAT7MainHeader::Sex_Unknown:
      extHeader.sex = 'u';
    break;
 }

  RETURN(result);
  return result;
}

bool CPhilipsMainHeaderPrivate::ecat2philipsIsotop(const QString& isotop)
{
  ENTER();
  bool result = false;

  QString isotope = isotop.toUpper().remove(QChar('-'));
  if(isotope.contains("UNDEFINED"))
    header.isotop = CPhilipsMainHeader::UndefinedIsotop;
  else if(isotope.contains("UNKNOWN"))
    header.isotop = CPhilipsMainHeader::UnknownIsotop;
  else if(isotope.contains("F18"))
    header.isotop = CPhilipsMainHeader::F18;
  else if(isotope.contains("O15"))
    header.isotop = CPhilipsMainHeader::O15;
  else if(isotope.contains("C11"))
    header.isotop = CPhilipsMainHeader::C11;
  else if(isotope.contains("GA68"))
    header.isotop = CPhilipsMainHeader::GA68;
  else if(isotope.contains("N13"))
    header.isotop = CPhilipsMainHeader::N13;
  else if(isotope.contains("RB82"))
    header.isotop = CPhilipsMainHeader::RB82;
  else if(isotope.contains("CU62"))
    header.isotop = CPhilipsMainHeader::CU62;
  else if(isotope.contains("CS137"))
    header.isotop = CPhilipsMainHeader::CS137;
  else if(isotope.contains("GE68"))
    header.isotop = CPhilipsMainHeader::GE68;
  else if(isotope.contains("CU64"))
    header.isotop = CPhilipsMainHeader::CU64;
  else if(isotope.contains("BR76"))
    header.isotop = CPhilipsMainHeader::BR76;
  else if(isotope.contains("NA22"))
    header.isotop = CPhilipsMainHeader::NA22;
  else if(isotope.contains("O14"))
    header.isotop = CPhilipsMainHeader::O14;
  else if(isotope.contains("Y86"))
    header.isotop = CPhilipsMainHeader::Y86;
  else if(isotope.contains("ZN62"))
    header.isotop = CPhilipsMainHeader::ZN62;
  else if(isotope.contains("CU60"))
    header.isotop = CPhilipsMainHeader::CU60;
  else if(isotope.contains("CU61"))
    header.isotop = CPhilipsMainHeader::CU61;
  else if(isotope.contains("GA66"))
    header.isotop = CPhilipsMainHeader::GA66;
  else if(isotope.contains("BR75"))
    header.isotop = CPhilipsMainHeader::BR75;
  else if(isotope.contains("BR77"))
    header.isotop = CPhilipsMainHeader::BR77;
  else if(isotope.contains("I124"))
    header.isotop = CPhilipsMainHeader::I124;
  else if(isotope.contains("K38"))
    header.isotop = CPhilipsMainHeader::K38;
  else if(isotop.contains("MN52"))
    header.isotop = CPhilipsMainHeader::MN52;
  else if(isotop.contains("TC94M"))
    header.isotop = CPhilipsMainHeader::TC94M;
  else if(isotop.contains("TI45"))
    header.isotop = CPhilipsMainHeader::TI45;
  else
    header.isotop = CPhilipsMainHeader::OtherIsotop;

  result = true;

  RETURN(result);
  return result;
}

bool CPhilipsMainHeaderPrivate::ecat2philipsOrientation(const CECAT7MainHeader::Patient_Orientation eOrientation)
{
  ENTER();
  bool result = false;

  switch(eOrientation)
  {
    case CECAT7MainHeader::FF_Prone:
      header.orient_ps = CPhilipsMainHeader::Prone;
      header.orient_hf = CPhilipsMainHeader::Feet_First;
    break;

    case CECAT7MainHeader::FF_Supine:
      header.orient_ps = CPhilipsMainHeader::Supine;
      header.orient_hf = CPhilipsMainHeader::Feet_First;
    break;

    case CECAT7MainHeader::FF_Left:
      header.orient_ps = CPhilipsMainHeader::Left;
      header.orient_hf = CPhilipsMainHeader::Feet_First;
    break;

    case CECAT7MainHeader::FF_Right:
      header.orient_ps = CPhilipsMainHeader::Right;
      header.orient_hf = CPhilipsMainHeader::Feet_First;
    break;

    case CECAT7MainHeader::HF_Prone:
      header.orient_ps = CPhilipsMainHeader::Prone;
      header.orient_hf = CPhilipsMainHeader::Head_First;
    break;

    case CECAT7MainHeader::HF_Supine:
      header.orient_ps = CPhilipsMainHeader::Supine;
      header.orient_hf = CPhilipsMainHeader::Head_First;
    break;

    case CECAT7MainHeader::HF_Left:
      header.orient_ps = CPhilipsMainHeader::Left;
      header.orient_hf = CPhilipsMainHeader::Head_First;
    break;

    case CECAT7MainHeader::HF_Right:
      header.orient_ps = CPhilipsMainHeader::Right;
      header.orient_hf = CPhilipsMainHeader::Head_First;
    break;

    default:
      header.orient_ps = CPhilipsMainHeader::Undefined_Orientation_ps;
      header.orient_hf = CPhilipsMainHeader::Undefined_Orientation_hf;
    break;
  }

  result = true;

  RETURN(result);
  return result;
}

bool CPhilipsMainHeaderPrivate::ecat2philipsAcquisitionType(const CECAT7MainHeader::Acquisition_Type acqType)
{
  ENTER();
  bool result = false;
  CPhilipsMainHeader::Acquisition_Protocol_Type ptype = CPhilipsMainHeader::Undefined_Acquisition_Protocol;
  CPhilipsMainHeader::Acquisition_Type atype = CPhilipsMainHeader::UndefinedAcq;

  switch(acqType)
  {
    case CECAT7MainHeader::Undefined:
      ptype = CPhilipsMainHeader::Undefined_Acquisition_Protocol;
      atype = CPhilipsMainHeader::UndefinedAcq;
    break;

    case CECAT7MainHeader::StaticEmission:
    case CECAT7MainHeader::EmissionRectilinear:
      ptype = CPhilipsMainHeader::Static_Emission; 
      atype = CPhilipsMainHeader::Emission;
    break;

    case CECAT7MainHeader::DynamicEmission:
      ptype = CPhilipsMainHeader::Dynamic_Emission;
      atype = CPhilipsMainHeader::Emission;
    break;

    case CECAT7MainHeader::Transmission:
    case CECAT7MainHeader::TransmissionRectilinear:
      ptype = CPhilipsMainHeader::Static_Transmission_Only; 
      atype = CPhilipsMainHeader::Transmission;
    break;

    case CECAT7MainHeader::GatedEmission:
      ptype = CPhilipsMainHeader::Gated_Cardiac;
      atype = CPhilipsMainHeader::Emission;
    break;

    case CECAT7MainHeader::Blank:
      ptype = CPhilipsMainHeader::Undefined_Acquisition_Protocol;
      atype = CPhilipsMainHeader::Blank;
    break;
  }

  header.aqprotocol_type = ptype;
  header.scntyp = atype;

  RETURN(result);
  return result;
}

const char* CPhilipsMainHeader::Dpat_name() const
{
  return m_pData->extHeader.Dpat_name;
}

const char* CPhilipsMainHeader::Dpat_id() const
{
  return m_pData->extHeader.Dpat_id;
}

const char* CPhilipsMainHeader::study_uid() const
{
  return m_pData->extHeader.study_uid;
}

const char* CPhilipsMainHeader::series_uid() const
{
  return m_pData->extHeader.series_uid;
}

const char* CPhilipsMainHeader::view_code() const
{
  return m_pData->extHeader.view_code;
}

const char* CPhilipsMainHeader::sortproto_name() const
{
  return m_pData->extHeader.sortproto_name;
}

CPhilipsMainHeader::Route_Type CPhilipsMainHeader::route() const
{
  return static_cast<Route_Type>(m_pData->extHeader.route);
}

CPhilipsMainHeader::Pharm_Type CPhilipsMainHeader::pharm() const
{
  return static_cast<Pharm_Type>(m_pData->extHeader.pharm);
}

int CPhilipsMainHeader::assay_date() const
{
  return m_pData->extHeader.assay_date;
}

long CPhilipsMainHeader::assay_time() const
{
  return m_pData->extHeader.assay_time;
}

const char* CPhilipsMainHeader::series_desc() const
{
  return m_pData->extHeader.series_desc;
}

short CPhilipsMainHeader::height() const
{
  return m_pData->extHeader.height;
}

short CPhilipsMainHeader::abundance() const
{
  return m_pData->extHeader.abundance;
}

short CPhilipsMainHeader::realignment_xOffset() const
{
  return m_pData->extHeader.realign_x;
}

short CPhilipsMainHeader::realignment_yOffset() const
{
  return m_pData->extHeader.realign_y;
}

short CPhilipsMainHeader::realignment_horizRotation() const
{
  return m_pData->extHeader.realign_hr;
}

time_t CPhilipsMainHeader::acq_date_time() const
{
  return m_pData->extHeader.acq_date_time;
}

time_t CPhilipsMainHeader::study_date_time() const
{
  return m_pData->extHeader.study_date_time;
}

time_t CPhilipsMainHeader::injection_date_time() const
{
  return m_pData->extHeader.injection_date_time;
}

time_t CPhilipsMainHeader::file_create_date_time() const
{
  return m_pData->extHeader.file_create_date_time;
}

CPhilipsMainHeader::Respiration_Trigger_Location CPhilipsMainHeader::resp_trig_loc() const
{
  return static_cast<Respiration_Trigger_Location>(m_pData->extHeader.resp_trig_loc);
}

CPhilipsMainHeader::Card_Arrhythmia_Rej_Tech CPhilipsMainHeader::card_arrhythmia_rej_tech() const
{
  return static_cast<Card_Arrhythmia_Rej_Tech>(m_pData->extHeader.card_arrhythmia_rej_tech);
}

float CPhilipsMainHeader::window_center() const
{
  return m_pData->extHeader.window_center;
}

float CPhilipsMainHeader::window_width() const
{
  return m_pData->extHeader.window_width;
}

short CPhilipsMainHeader::realignment_axialRotation() const
{
  return m_pData->extHeader.realign_zr;
}

short CPhilipsMainHeader::realignment_vertRotation() const
{
  return m_pData->extHeader.realign_vr;
}

short CPhilipsMainHeader::resp_trig_threshold() const
{
  return m_pData->extHeader.resp_trig_threshold;
}

short CPhilipsMainHeader::resp_phase_duration() const
{
  return m_pData->extHeader.resp_phase_duration;
}

short CPhilipsMainHeader::resp_phase_offset() const
{
  return m_pData->extHeader.resp_phase_offset;
}

short CPhilipsMainHeader::realignment_zOffset() const
{
  return m_pData->extHeader.realign_z;
}

CPhilipsMainHeader::Window_Units CPhilipsMainHeader::window_units() const
{
  return static_cast<Window_Units>(m_pData->extHeader.window_units);
}

const char* CPhilipsMainHeader::req_phys() const
{
  return m_pData->extHeader.req_phys;
}

CPhilipsMainHeader::Card_Ph_State CPhilipsMainHeader::card_phstate() const
{
  return static_cast<Card_Ph_State>(m_pData->extHeader.card_phstate);
}

const char* CPhilipsMainHeader::referring_physician() const
{
  return m_pData->extHeader.referring_physician;
}

const char* CPhilipsMainHeader::study_id() const
{
  return m_pData->extHeader.study_id;
}

float CPhilipsMainHeader::Dslice_thick() const
{
  return m_pData->extHeader.Dslice_thick;
}

char CPhilipsMainHeader::sex() const
{
  return m_pData->extHeader.sex;
}

float CPhilipsMainHeader::table_height() const
{
  return m_pData->extHeader.table_height;
}

bool CPhilipsMainHeader::card_bt_rej() const
{
  return (m_pData->extHeader.card_bt_rej == 1);
}

CPhilipsMainHeader::Card_Fr_Type CPhilipsMainHeader::card_fr_type() const
{
  return static_cast<Card_Fr_Type>(m_pData->extHeader.card_fr_type);
}

const char* CPhilipsMainHeader::Dmanufacture_model_name() const
{
  return m_pData->extHeader.Dmanufacture_model_name;
}

const char* CPhilipsMainHeader::Dimage_type() const
{
  return m_pData->extHeader.Dimage_type;
}

float CPhilipsMainHeader::min_bed_pos() const
{
  return m_pData->extHeader.min_bed_pos;
}

float CPhilipsMainHeader::max_bed_pos() const
{
  return m_pData->extHeader.max_bed_pos;
}

bool CPhilipsMainHeader::der_filled() const
{
  return (m_pData->extHeader.der_filled == 1);
}

long CPhilipsMainHeader::series_number() const
{
  return m_pData->extHeader.series_number;
}

CPhilipsMainHeader::Card_Slc_Dir CPhilipsMainHeader::card_slc_dir() const
{
  return static_cast<Card_Slc_Dir>(m_pData->extHeader.card_slc_dir);
}

int CPhilipsMainHeader::card_skip_msec() const
{
  return m_pData->extHeader.card_skip_msec;
}

int CPhilipsMainHeader::card_skip_counts() const
{
  return m_pData->extHeader.card_skip_counts;
}

int CPhilipsMainHeader::card_dur_msec() const
{
  return m_pData->extHeader.card_dur_msec;
}

int CPhilipsMainHeader::card_dur_counts() const
{
  return m_pData->extHeader.card_dur_counts;
}

int CPhilipsMainHeader::card_beats_tot() const
{
  return m_pData->extHeader.card_beats_tot;
}

int CPhilipsMainHeader::card_beats_acc() const
{
  return m_pData->extHeader.card_beats_acc;
}

short CPhilipsMainHeader::card_skip_beats() const
{
  return m_pData->extHeader.card_skip_beats;
}

short CPhilipsMainHeader::pvc_threshold() const
{
  return m_pData->extHeader.pvc_threshold;
}

const char* CPhilipsMainHeader::radiopharm_name() const
{
  return m_pData->extHeader.radiopharm_name;
}

const char* CPhilipsMainHeader::Dserial_number() const
{
  return m_pData->extHeader.Dserial_number;
}

const char* CPhilipsMainHeader::attncor_label() const
{
  return m_pData->extHeader.attncor_label;
}

const char* CPhilipsMainHeader::contr_bolus_agent()
{
  return m_pData->extHeader.contr_bolus_agent;
}

const char* CPhilipsMainHeader::sop_uid()
{
  return m_pData->extHeader.sop_uid;
}

const char* CPhilipsMainHeader::frame_ref_uid()
{
  return m_pData->extHeader.frame_ref_uid;
}

const char* CPhilipsMainHeader::pps_file()
{
  return m_pData->extHeader.pps_file;
}

const char* CPhilipsMainHeader::worklist_file()
{
  return m_pData->extHeader.worklist_file;
}

void CPhilipsMainHeader::setDpat_name(const char* str)
{
  strncpy(m_pData->extHeader.Dpat_name, str, sizeof(m_pData->extHeader.Dpat_name));
}

void CPhilipsMainHeader::setDpat_id(const char* str)
{
  strncpy(m_pData->extHeader.Dpat_id, str, sizeof(m_pData->extHeader.Dpat_id));
}

void CPhilipsMainHeader::setStudy_uid(const char* str)
{
  strncpy(m_pData->extHeader.study_uid, str, sizeof(m_pData->extHeader.study_uid));
}

void CPhilipsMainHeader::setSeries_uid(const char* str)
{
  strncpy(m_pData->extHeader.series_uid, str, sizeof(m_pData->extHeader.series_uid));
}

void CPhilipsMainHeader::setView_code(const char* str)
{
  strncpy(m_pData->extHeader.view_code, str, sizeof(m_pData->extHeader.view_code));
}

void CPhilipsMainHeader::setSortproto_name(const char* str)
{
  strncpy(m_pData->extHeader.sortproto_name, str, sizeof(m_pData->extHeader.sortproto_name));
}

void CPhilipsMainHeader::setRoute(const Route_Type route)
{
  m_pData->extHeader.route = static_cast<qint16>(route);
}

void CPhilipsMainHeader::setPharm(const Pharm_Type pharm)
{
  m_pData->extHeader.pharm = static_cast<qint16>(pharm);
}

void CPhilipsMainHeader::setAssay_date(const int assay_date)
{
  m_pData->extHeader.assay_date = assay_date;
}

void CPhilipsMainHeader::setAssay_time(const long assay_time)
{
  m_pData->extHeader.assay_time = assay_time;
}

void CPhilipsMainHeader::setSeries_desc(const char* str)
{
  strncpy(m_pData->extHeader.series_desc, str, sizeof(m_pData->extHeader.series_desc));
}

void CPhilipsMainHeader::setHeight(const short height)
{
  m_pData->extHeader.height = height;
}

void CPhilipsMainHeader::setAbundance(const short abundance)
{
  m_pData->extHeader.abundance = abundance;
}

void CPhilipsMainHeader::setRealignment_xOffset(const short xOffset)
{
  m_pData->extHeader.realign_x = xOffset;
}

void CPhilipsMainHeader::setRealignment_yOffset(const short yOffset)
{
  m_pData->extHeader.realign_y = yOffset;
}

void CPhilipsMainHeader::setRealignment_horizRotation(const short horizRotation)
{
  m_pData->extHeader.realign_hr = horizRotation;
}

void CPhilipsMainHeader::setAcq_date_time(const time_t date_time)
{
  m_pData->extHeader.acq_date_time = date_time;
}

void CPhilipsMainHeader::setStudy_date_time(const time_t date_time)
{
  m_pData->extHeader.study_date_time = date_time;
}

void CPhilipsMainHeader::setInjection_date_time(const time_t date_time)
{
  m_pData->extHeader.injection_date_time = date_time;
}

void CPhilipsMainHeader::setFile_create_date_time(const time_t date_time)
{
  m_pData->extHeader.file_create_date_time = date_time;
}

void CPhilipsMainHeader::setResp_trig_loc(const CPhilipsMainHeader::Respiration_Trigger_Location location)
{
   m_pData->extHeader.resp_trig_loc = location;
}

void CPhilipsMainHeader::setCard_arrhythmia_rej_tech(const CPhilipsMainHeader::Card_Arrhythmia_Rej_Tech rej_tech)
{
  m_pData->extHeader.card_arrhythmia_rej_tech = rej_tech;
}

void CPhilipsMainHeader::setWindow_center(const float window_center)
{
  m_pData->extHeader.window_center = window_center;
}

void CPhilipsMainHeader::setWindow_width(const float window_width)
{
  m_pData->extHeader.window_width = window_width;
}

void CPhilipsMainHeader::setRealignment_axialRotation(const short axialRotation)
{
  m_pData->extHeader.realign_zr = axialRotation;
}

void CPhilipsMainHeader::setRealignment_vertRotation(const short vertRotation)
{
  m_pData->extHeader.realign_vr = vertRotation;
}

void CPhilipsMainHeader::setResp_trig_threshold(const short resp_trig_threshold)
{
  m_pData->extHeader.resp_trig_threshold = resp_trig_threshold;
}

void CPhilipsMainHeader::setResp_phase_duration(const short resp_phase_duration)
{
  m_pData->extHeader.resp_phase_duration = resp_phase_duration;
}

void CPhilipsMainHeader::setResp_phase_offset(const short resp_phase_offset)
{
  m_pData->extHeader.resp_phase_offset = resp_phase_offset;
}

void CPhilipsMainHeader::setRealignment_zOffset(const short zOffset)
{
  m_pData->extHeader.realign_z = zOffset;
}

void CPhilipsMainHeader::setWindow_units(const CPhilipsMainHeader::Window_Units window_units)
{
  m_pData->extHeader.window_units = static_cast<qint16>(window_units);
}

void CPhilipsMainHeader::setReq_phys(const char* str)
{
  strncpy(m_pData->extHeader.req_phys, str, sizeof(m_pData->extHeader.req_phys));
}

void CPhilipsMainHeader::setCard_phstate(const CPhilipsMainHeader::Card_Ph_State state)
{
  m_pData->extHeader.card_phstate = static_cast<quint16>(state);
}

void CPhilipsMainHeader::setReferring_physician(const char* str)
{
  strncpy(m_pData->extHeader.referring_physician, str, sizeof(m_pData->extHeader.referring_physician));
}

void CPhilipsMainHeader::setStudy_id(const char* str)
{
  strncpy(m_pData->extHeader.study_id, str, sizeof(m_pData->extHeader.study_id));
}

void CPhilipsMainHeader::setDslice_thick(const float Dslice_thick)
{
  m_pData->extHeader.Dslice_thick = Dslice_thick;
}

void CPhilipsMainHeader::setSex(const char sex)
{
  m_pData->extHeader.sex = sex;
}

void CPhilipsMainHeader::setTable_height(const float table_height)
{
  m_pData->extHeader.table_height = table_height;
}

void CPhilipsMainHeader::setCard_bt_rej(const bool flag)
{
  m_pData->extHeader.card_bt_rej = (flag == true)? 1 : 0;
}

void CPhilipsMainHeader::setCard_fr_type(const CPhilipsMainHeader::Card_Fr_Type type)
{
  m_pData->extHeader.card_fr_type = static_cast<qint16>(type);
}

void CPhilipsMainHeader::setDmanufacture_model_name(const char* str)
{
  strncpy(m_pData->extHeader.Dmanufacture_model_name, str, sizeof(m_pData->extHeader.Dmanufacture_model_name));
}

void CPhilipsMainHeader::setDimage_type(const char* str)
{
  strncpy(m_pData->extHeader.Dimage_type, str, sizeof(m_pData->extHeader.Dimage_type));
}

void CPhilipsMainHeader::setMin_bed_pos(const float min)
{
  m_pData->extHeader.min_bed_pos = min;
}

void CPhilipsMainHeader::setMax_bed_pos(const float max)
{
  m_pData->extHeader.max_bed_pos = max;
}

void CPhilipsMainHeader::setDer_filled(const bool flag)
{
  m_pData->extHeader.der_filled = (flag == true)? 1 : 0;
}

void CPhilipsMainHeader::setSeries_number(const long number)
{
  m_pData->extHeader.series_number = number;
}

void CPhilipsMainHeader::setCard_slc_dir(const CPhilipsMainHeader::Card_Slc_Dir direction)
{
  m_pData->extHeader.card_slc_dir = static_cast<qint16>(direction);
}

void CPhilipsMainHeader::setCard_skip_msec(const int msec)
{
  m_pData->extHeader.card_skip_msec = msec;
}

void CPhilipsMainHeader::setCard_skip_counts(const int counts)
{
  m_pData->extHeader.card_skip_counts = counts;
}

void CPhilipsMainHeader::setCard_dur_msec(const int msec)
{
  m_pData->extHeader.card_dur_msec = msec;
}

void CPhilipsMainHeader::setCard_dur_counts(const int counts)
{
  m_pData->extHeader.card_dur_counts = counts;
}

void CPhilipsMainHeader::setCard_beats_tot(const int beats)
{
  m_pData->extHeader.card_beats_tot = beats;
}

void CPhilipsMainHeader::setCard_beats_acc(const int acc)
{
  m_pData->extHeader.card_beats_acc = acc;
}

void CPhilipsMainHeader::setCard_skip_beats(const short beats)
{
  m_pData->extHeader.card_skip_beats = beats;
}

void CPhilipsMainHeader::setPvc_threshold(const short threshold)
{
  m_pData->extHeader.pvc_threshold = threshold;
}

void CPhilipsMainHeader::setRadiopharm_name(const char* str)
{
  strncpy(m_pData->extHeader.radiopharm_name, str, sizeof(m_pData->extHeader.radiopharm_name));
}

void CPhilipsMainHeader::setDserial_number(const char* str)
{
  strncpy(m_pData->extHeader.Dserial_number, str, sizeof(m_pData->extHeader.Dserial_number));
}

void CPhilipsMainHeader::setAttncor_label(const char* str)
{
  strncpy(m_pData->extHeader.attncor_label, str, sizeof(m_pData->extHeader.attncor_label));
}

void CPhilipsMainHeader::setContr_bolus_agent(const char* str)
{
  strncpy(m_pData->extHeader.contr_bolus_agent, str, sizeof(m_pData->extHeader.contr_bolus_agent));
}

void CPhilipsMainHeader::setSop_uid(const char* str)
{
  strncpy(m_pData->extHeader.sop_uid, str, sizeof(m_pData->extHeader.sop_uid));
}

void CPhilipsMainHeader::setFrame_ref_uid(const char* str)
{
  strncpy(m_pData->extHeader.frame_ref_uid, str, sizeof(m_pData->extHeader.frame_ref_uid));
}

void CPhilipsMainHeader::setPps_file(const char* str)
{
  strncpy(m_pData->extHeader.pps_file, str, sizeof(m_pData->extHeader.pps_file));
}

void CPhilipsMainHeader::setWorklist_file(const char* str)
{
  strncpy(m_pData->extHeader.worklist_file, str, sizeof(m_pData->extHeader.worklist_file));
}
