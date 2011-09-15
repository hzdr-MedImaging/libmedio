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
#include "CPhilipsExtendedMainHeader.h"
#include "CPhilipsFile.h"
#include "CPhilipsDirectoryItem.h"
#include "CMedIOHeader.h"
#include "CMedIOData.h"

#include <QDateTime>

#include <rtdebug.h>

class CPhilipsExtendedMainHeaderPrivate
{
  public:
    struct HeaderData
    {
    //  /* flags */
    //   ValidHdrStruct valid;              /* Validity of extended header */


      char Dpat_name[64];                 /* DICOM patient name */
      char Dpat_id[64];                   /* DICOM patient ID */
      char study_uid[64];                 /* unique study identifier */
      char series_uid[64];                /* unique series identifier */

      char view_code[20];                 /* Our version of DICOM view code */
      char sortproto_name[20];            /* sorter protocol name */
      qint16 route;
      qint16 pharm;
      char req_phys[64];                  /* requesting physician */
      qint16 card_phstate;                /* Physiologic state */
      qint32 assay_date;                  /* Date of radioactivity measurement */
      qint32 assay_time;                  /* Time of radioactivity measurement */
      char series_desc[64];               /* Description */
      qint16 height;                      /* Patient height, in mm */
      qint16 abundance;                    /* Positron abundance ratio, in tenths of percent */
                                          /*  so that 1000 is 100% */
      qint32 acq_date_time;                /* Date/time data acquired (UTC). Use access functions to read/write. */
      qint32 study_date_time;              /* Date/time study started (UTC). Use access functions to read/write. */
      qint32 injection_date_time;          /* Date/time of injection (UTC). Use access functions to read/write. */
      qint32 file_create_date_time;        /* Replaces daycre, mocre, yrcre, hrcre, mincre and seccre fields. Date and
                                              time that the acquisition file was created. Filled in by acquisition. */
      qint16 resp_trig_loc;    /* Respiratory trigger location. */
      qint16 card_arrhythmia_rej_tech;    /* Card Arrhythmia rejection techniques employed: */
                                         /* Bi mask:   */
                                         /*  0 = no rejection */
                                         /*  1 = Rejection based on deviation from average R-R interval */
                                         /*  2 = Rejection based on deviation from regular QRS loop.*/
                                         /*  4 = Rejection based on PVC criteria. */
      float window_center;               /* Suggested center value to use when displaying this image. */
      float window_width;                /* Suggested width value to use when displaying this image. */

      qint16 resp_trig_threshold;         /* Respiratory trigger threshold in % of chest */
                                         /*  expansion relative to last peak. */
      qint16 resp_phase_duration;         /* Respiratory acquisition duration in % */
                                         /*  of the respiratory cycle. */
      qint16 resp_phase_offset;           /* For respiratory gated, the offset from the trigger */
                                         /*  to the beginning of the acquisition, in % */
                                         /*  of the respiratory cycle. */
      qint16 window_units;        /* Units in which window_center and window_width are specified. */

      char referring_physician[64];       /* referring physician */
      char study_id[16];                  /* study identifier */
      float Dslice_thick;                 /* DICOM slice thickness */
      char sex;                           /* (DICOM) patient sex */
      float table_height;                 /* Table height */
      qint16 card_bt_rej;                 /* Heart beat duration sorting (y/n) */
      qint16 card_fr_type;                /* Type of cardiac framing performed */
      char Dmanufacture_model_name[64];   /* DICOM manufacturers model name */
      char Dimage_type[64];               /* DICOM image identification */
      float min_bed_pos;                  /* Minimum bed position */
      float max_bed_pos;                  /* Maximum bed position */
      qint16 der_filled;                  /* Whether derived fields filled */
      qint32 series_number;               /* DICOM series number */
      qint32 dep_study_date;              /* (Deprecated - use study_date_time) Date study created (date_2_int()) */
      qint32 dep_study_time;              /* (Deprecated - use study_date_time) time study created */
      qint32 dep_acq_time;                /* (Deprecated - use acq_date_time) time data acquired */
      qint16 card_slc_dir;                /* Slice progression direction */

      qint32 card_skip_msec;		 /* initial data skipped, in msec */
      qint32 card_skip_counts;		 /* initial data skipped, in counts */
      qint32 card_dur_msec;		 /* duration of binned data, in msec */
      qint32 card_dur_counts;		 /* duration of binned data, in counts */
      qint32 card_beats_tot;		 /* beats occurring during movie */
      qint32 card_beats_acc;             /* beats accepted into movie */
      qint32 dep_acq_date;               /* (Deprecated - use acq_date_time) Date data acquired (date_2_int()) */

      char contr_bolus_agent[64];        /* DICOM contrast bolus agent */
      char sop_uid[64];                  /* SOP UID of incoming images */
      char frame_ref_uid[64];            /* UID of frame of reference image */
      char pps_file[30];                 /* Perform procedure file */
      char worklist_file[30];            /* Worklist file */

      struct
      {
         short xOffset;       /* For cardiac realignment. Additional    */
                              /*  horizontal (x) shift of center of CT  */
                              /*  FOV w.r.t. PET (in 1/100mm).          */
         short yOffset;       /* For cardiac realignment. Additional    */
                              /*  vertical (y) shift of center of CT    */
                              /*  FOV w.r.t. PET (in 1/100mm).          */
         short zOffset;       /* For cardiac realignment. Additional    */
                              /*  (z)  shift of center of CT            */
                              /*  FOV w.r.t. PET (in 1/100mm).          */
         short axialRotation; /* For cardiac realignment. Additional    */
                              /*  rotation about the axial (Z) axis     */
                              /*  (in 1/1,000 degree).                  */
         short horizRotation; /* For cardiac realignment. Additional    */
                              /*  rotation about the horizontal (X) axis*/
                              /*  (in 1/1,000 degree).                  */
         short vertRotation;  /* For cardiac realignment. Additional    */
                              /*  rotation about the vertical (Y) axis  */
                              /*  (in 1/1,000 degree).                  */
      } realignment;
      // RouteTypes route;                  /* How the isotope was delivered */
      // PharmTypes pharm;                  /* Pharmaceutical used in the study */
      // char radiopharm_name[64]; /* Used when pharm == Other, to */
      //                                    /*  specify name of radiopharmaceutical used. */


      // char Dserial_number[17];           /* System serial number + null */

      // char attncor_label[64]; /* Label (UID) identifying related atten. corr. series. */

      // short pvc_threshold;               /* When PVC rejection used, the */
      //                                    /*  the % of R-R below which is */
      //                                    /*  considered a PVC.           */
      // short card_skip_beats;             /* Num beats skipped after arrhythmia. */
      // char ref_gated_qc_image_inst_uid[64]; /* SOP Instance UID of */
      //                                    /*  Sec. Cap. image containing picture */
      //                                    /*  of cardiac waveform and trigger level.*/

      // ValidHdrStruct mr_valid;     /* Whether the file has a valid petmr struct */
      // unsigned char coil_type[MAX_MR_COILS]; /* MR coils present. */
      // long start_table_pos_abs;    /* Absolute start table position. */
      // long start_table_pos_rel;    /* Relative start table position. */
    } header;
};

CPhilipsExtendedMainHeader::CPhilipsExtendedMainHeader(CPhilipsFile* file, CPhilipsDirectoryItem* pDirItem)
  : CMedIOHeader(file),
    m_pDirItem(pDirItem)
{
  ENTER();

  m_pData = new CPhilipsExtendedMainHeaderPrivate();

  // this constructor creates a empty EPhilipsExtendedMainHeader
  // with prefilled data that is always the same for a philips main header
  clear();

  LEAVE();
}

CPhilipsExtendedMainHeader::CPhilipsExtendedMainHeader(const CPhilipsExtendedMainHeader& src)
  : CMedIOHeader(src)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CPhilipsExtendedMainHeaderPrivate(*(src.m_pData));

  LEAVE();
}

CPhilipsExtendedMainHeader& CPhilipsExtendedMainHeader::operator=(const CPhilipsExtendedMainHeader& src)
{
  ENTER();

  if(m_pData != src.m_pData)
  {
    memcpy(&m_pData->header,
           &src.m_pData->header,
           sizeof(struct CPhilipsExtendedMainHeaderPrivate::HeaderData));
  }

  LEAVE();
  return *this;
}
CPhilipsExtendedMainHeader::~CPhilipsExtendedMainHeader()
{
  ENTER();

  delete m_pData;

  LEAVE();
}
void CPhilipsExtendedMainHeader::clear()
{
  ENTER();
  // clear our MainHeader structure first
  memset(&m_pData->header, 0, sizeof(struct CPhilipsExtendedMainHeaderPrivate::HeaderData));

  LEAVE();
}

bool CPhilipsExtendedMainHeader::load()
{
  ENTER();

  // only go on if the device is readable at all
  if(m_pMedIOData == NULL ||
     m_pMedIOData->isReadable() == false ||
     m_pMedIOData->seek(m_pDirItem->dataBlock_Start()) == false)
  {
    RETURN(false);
    return false;
  }

  // we use a ByteArray buffer to speed up the endianess
  // decoding
  QByteArray buffer(rawDataSize(), 0);
  if(m_pMedIOData->read(buffer.data(), buffer.size()) != rawDataSize())
  {
    RETURN(false);
    return false;
  }

  // now we generate a QDataStream on our buffer so that we can read
  // out of the buffer instead of the raw file (> speed)
  QDataStream stream(buffer);  

  // we have to set the QDataStream version to the Qt4.5 version
  // because with Qt4.6 the floating point precision changed and
  // otherwise causes our streaming to fail
  stream.setVersion(QDataStream::Qt_4_5);
  
  // lets read in each single data element of our
  // data structure to maintain the correct endianess of the
  // data
  stream.readRawData(&m_pData->header.Dpat_name[0], 64);      // 0:   Dpat_name
  stream.readRawData(&m_pData->header.Dpat_id[0], 64);        // 64:  Dpat_id
  stream.readRawData(&m_pData->header.study_uid[0], 64);      // 128: study_uid
  stream.readRawData(&m_pData->header.series_uid[0], 64);     // 192: series_uid

  stream.readRawData(&m_pData->header.view_code[0], 20);      // 256: view_code
  stream.readRawData(&m_pData->header.sortproto_name[0], 20); // 276: sortproto_name
  stream >> m_pData->header.route;                            // 296: route
  stream >> m_pData->header.pharm;                            // 298: pharm
  stream.readRawData(&m_pData->header.req_phys[0], 64);       // 300: req_phys
  stream >> m_pData->header.card_phstate;                     // 364: card_phstate
  stream >> m_pData->header.assay_date;                       // 366: assay_date
  stream >> m_pData->header.assay_time;                       // 370: assay_time
  stream.readRawData(&m_pData->header.series_desc[0], 64);    // 374: series_desc
  stream >> m_pData->header.height;                           // 438: height
  stream >> m_pData->header.abundance;                        // 440: abundance
  stream >> m_pData->header.realignment.xOffset;              // 442: realignment.xOffset
  stream >> m_pData->header.realignment.yOffset;              // 444: realignment.yOffset
  stream >> m_pData->header.realignment.horizRotation;        // 446: realignment.horizRotation
  stream >> m_pData->header.acq_date_time;                    // 448: acq_date_time
  stream >> m_pData->header.study_date_time;                  // 452: study_date_time
  stream >> m_pData->header.injection_date_time;              // 456: injection_date_time
  stream >> m_pData->header.file_create_date_time;            // 460: file_create_date_time
  stream >> m_pData->header.resp_trig_loc;                    // 464: resp_trig_loc
  stream >> m_pData->header.card_arrhythmia_rej_tech;         // 468: card_arrhythmia_rej_tech
  stream >> m_pData->header.window_center;                    // 470: window_center
  stream >> m_pData->header.window_width;                     // 474: window_width
  stream >> m_pData->header.realignment.axialRotation;        // 478: realignment.axialRotation
  stream >> m_pData->header.realignment.vertRotation;         // 480: realignment.vertRotation
  stream.skipRawData(8);                                      // 482: skip the next 8 bytes
  stream >> m_pData->header.resp_trig_threshold;              // 450: resp_trig_threshold
  stream >> m_pData->header.resp_phase_duration;              // 452: resp_phase_duration
  stream >> m_pData->header.resp_phase_offset;                // 454: resp_phase_offset
  stream >> m_pData->header.realignment.zOffset;              // 456: realignment.zOffset
  stream >> m_pData->header.window_units;                     // 458: window_units
  stream.skipRawData(14);

  stream.readRawData(&m_pData->header.referring_physician[0], 64);
  stream.readRawData(&m_pData->header.study_id[0], 16);
  stream.skipRawData(2);
  stream >> m_pData->header.Dslice_thick;
  stream.readRawData(&m_pData->header.sex, 1);
  stream >> m_pData->header.table_height;
  stream.skipRawData(1);
  stream >> m_pData->header.card_bt_rej;
  stream >> m_pData->header.card_fr_type;
  stream.readRawData(&m_pData->header.Dmanufacture_model_name[0], 64);
  stream.readRawData(&m_pData->header.Dimage_type[0], 64);
  stream >> m_pData->header.min_bed_pos;
  stream >> m_pData->header.max_bed_pos;
  stream >> m_pData->header.der_filled;
  stream >> m_pData->header.series_number;
  stream >> m_pData->header.dep_study_date;
  stream >> m_pData->header.dep_study_time;
  stream >> m_pData->header.dep_acq_time;
  stream >> m_pData->header.card_slc_dir;
  stream.skipRawData(4);

  stream >> m_pData->header.card_skip_msec;
  stream >> m_pData->header.card_skip_counts;
  stream >> m_pData->header.card_dur_msec;
  stream >> m_pData->header.card_dur_counts;
  stream >> m_pData->header.card_beats_tot;
  stream >> m_pData->header.card_beats_acc;
  stream.skipRawData(16);
  stream >> m_pData->header.dep_acq_date;
  stream.skipRawData(212);

  stream.readRawData(m_pData->header.contr_bolus_agent, 64);
  stream.readRawData(m_pData->header.sop_uid, 64);
  stream.readRawData(m_pData->header.frame_ref_uid, 64);
  stream.readRawData(m_pData->header.pps_file, 30);
  stream.readRawData(m_pData->header.worklist_file, 30);
  stream.skipRawData(4);

  // some more debug output
#if defined(DEBUG)
  D("Philips extended MainHeader loaded:");
  D("----------------------------");
  D("Dpat_name                : %s", m_pData->header.Dpat_name);
  D("Dpat_id                  : %s", m_pData->header.Dpat_id);
  D("study_uid                : %s", m_pData->header.study_uid);
  D("series_uid               : %s", m_pData->header.series_uid);
                             
  D("view_code                : %s", m_pData->header.view_code);
  D("sortproto_name           : %s", m_pData->header.sortproto_name);
  D("route                    : %d", m_pData->header.route);
  D("pharm                    : %d", m_pData->header.pharm);
  D("req_phys                 : %s", m_pData->header.req_phys);
  D("card_phstate             : %d", m_pData->header.card_phstate);
  D("assay_date               : %d", m_pData->header.assay_date);
  D("assay_time               : %s", QDateTime::fromTime_t(m_pData->header.assay_time).toString().toAscii().constData());
  D("series_desc              : %s", m_pData->header.series_desc);
  D("height                   : %d", m_pData->header.height);
  D("abundance                : %d", m_pData->header.abundance);
  D("realignment.xOffset      : %d", m_pData->header.realignment.xOffset);
  D("realignment.yOffset      : %d", m_pData->header.realignment.yOffset);
  D("realignment.horizRotation: %d", m_pData->header.realignment.horizRotation);
  D("acq_date_time            : %s", QDateTime::fromTime_t(m_pData->header.acq_date_time).toString().toAscii().constData());
  D("study_date_time          : %s", QDateTime::fromTime_t(m_pData->header.study_date_time).toString().toAscii().constData());
  D("injection_date_time      : %s", QDateTime::fromTime_t(m_pData->header.injection_date_time).toString().toAscii().constData());
  D("file_create_date_time    : %s", QDateTime::fromTime_t(m_pData->header.file_create_date_time).toString().toAscii().constData());
  D("resp_trig_loc            : %d", m_pData->header.resp_trig_loc);
  D("card_arrhythmia_rej_tech : %d", m_pData->header.card_arrhythmia_rej_tech);
  D("window_center            : %d", m_pData->header.window_center);
  D("window_width             : %d", m_pData->header.window_width);
  D("realignment_axialRotation: %d", m_pData->header.realignment.axialRotation);
  D("realignment_verRotation  : %d", m_pData->header.realignment.vertRotation);  
  D("resp_trig_threshold      : %d", m_pData->header.resp_trig_threshold);
  D("resp_phase_duration      : %d", m_pData->header.resp_phase_duration);
  D("resp_phase_offset        : %d", m_pData->header.resp_phase_offset);
  D("realignment.zOffset      : %d", m_pData->header.realignment.zOffset);
  D("window_units             : %d", m_pData->header.window_units);

  D("referring_physician      : %s", m_pData->header.referring_physician);
  D("study_id                 : %s", m_pData->header.study_id);

  D(" Dslice_thick            : %f", m_pData->header.Dslice_thick);
  D(" sex                     : %c", m_pData->header. sex);
  D(" table_height            : %f", m_pData->header.table_height);
  D(" card_bt_rej             : %d", m_pData->header.card_bt_rej);
  D(" card_fr_type            : %d", m_pData->header.card_fr_type);
  D(" Dmanufacture_model_name : %s", m_pData->header.Dmanufacture_model_name);
  D(" Dimage_type             : %s", m_pData->header.Dimage_type);
  D(" min_bed_pos             : %f", m_pData->header.min_bed_pos);
  D(" max_bed_pos             : %f", m_pData->header.max_bed_pos);
  D(" der_filled              : %d", m_pData->header.der_filled);
  D(" series_number           : %d", m_pData->header.series_number);
  D(" dep_study_date          : %ld", m_pData->header.dep_study_date);
  D(" dep_study_time          : %ld", m_pData->header.dep_study_time);
  D(" dep_acq_time            : %ld", m_pData->header.dep_acq_time);
  D(" card_slc_dir            : %d", m_pData->header.card_slc_dir);

  D("card_skip_msec:          : %d", m_pData->header.card_skip_msec);
  D("card_skip_counts         : %d", m_pData->header.card_skip_counts);
  D("card_dur_msec            : %d", m_pData->header.card_dur_msec);
  D("card_dur_counts          : %d", m_pData->header.card_dur_counts);
  D("card_beats_tot           : %d", m_pData->header.card_beats_tot);
  D("card_beats_acc           : %d", m_pData->header.card_beats_acc);
  D("dep_acq_date             : %ld", m_pData->header.dep_acq_date);

  D("contr_bolus_agent        : %s", m_pData->header.contr_bolus_agent);
  D("sop_uid                  : %s", m_pData->header.sop_uid);
  D("frame_ref_uid            : %s", m_pData->header.frame_ref_uid);
  D("pps_file                 : %s", m_pData->header.pps_file);
  D("worklist_file            : %s", m_pData->header.worklist_file);
#endif

  RETURN(true);
  return true;
}

bool CPhilipsExtendedMainHeader::save(void) const
{
  ENTER();

  SHOWPOINTER(m_pMedIOData);

  // only go on if the device is writeable at all
  if(m_pMedIOData == NULL ||
     m_pMedIOData->isWritable() == false ||
     m_pMedIOData->seek(0) == false)
  {
    RETURN(false);
    return false;
  }

  RETURN(false);
  return false;
}

unsigned int CPhilipsExtendedMainHeader::rawDataSize() const
{
  return m_pDirItem->dataBlock_End()+PHILIPS_BLOCKSIZE - m_pDirItem->dataBlock_Start();
}

CMedIOHeader::Format CPhilipsExtendedMainHeader::headerFormat() const
{ 
  return CMedIOHeader::PhilipsExtendedMainHeader;
}

bool CPhilipsExtendedMainHeader::convertFrom(const CMedIOHeader* srcHeader1, const CMedIOHeader* srcHeader2) 
{
  ENTER();
  bool result = false;

  switch(srcHeader1->headerFormat())
  {
    case CMedIOHeader::PhilipsExtendedMainHeader:
    {
      const CPhilipsExtendedMainHeader* head = static_cast<const CPhilipsExtendedMainHeader*>(srcHeader1);

      // use the assignment operator to copy everything
      *this = *head;
      result = true;
    }
    break;

    default:
    {
      E("File format not supported yet");
      result = false;
    }

  }

  RETURN(result);
  return result;
}

CMedIOHeader* CPhilipsExtendedMainHeader::clone() const
{
  ENTER();

  CPhilipsExtendedMainHeader* pNewHeader = new CPhilipsExtendedMainHeader(*this);

  RETURN(pNewHeader);
  return pNewHeader;
}

void CPhilipsExtendedMainHeader::setDirectoryItem(CPhilipsDirectoryItem* dItem)
{ 
  m_pDirItem = dItem;
}

const char* CPhilipsExtendedMainHeader::Dpat_name() const
{
  return m_pData->header.Dpat_name;
}

const char* CPhilipsExtendedMainHeader::Dpat_id() const
{
  return m_pData->header.Dpat_id;
}

const char* CPhilipsExtendedMainHeader::study_uid() const
{
  return m_pData->header.study_uid;
}

const char* CPhilipsExtendedMainHeader::series_uid() const
{
  return m_pData->header.series_uid;
}

const char* CPhilipsExtendedMainHeader::view_code() const
{
  return m_pData->header.view_code;
}

const char* CPhilipsExtendedMainHeader::sortproto_name() const
{
  return m_pData->header.sortproto_name;
}

CPhilipsExtendedMainHeader::Route_Type CPhilipsExtendedMainHeader::route() const
{
  return static_cast<Route_Type>(m_pData->header.route);
}

CPhilipsExtendedMainHeader::Pharm_Type CPhilipsExtendedMainHeader::pharm() const
{
  return static_cast<Pharm_Type>(m_pData->header.pharm);
}

int CPhilipsExtendedMainHeader::assay_date() const
{
  return m_pData->header.assay_date;
}

long CPhilipsExtendedMainHeader::assay_time() const
{
  return m_pData->header.assay_time;
}

const char* CPhilipsExtendedMainHeader::series_desc() const
{
  return m_pData->header.series_desc;
}

short CPhilipsExtendedMainHeader::height() const
{
  return m_pData->header.height;
}

short CPhilipsExtendedMainHeader::abundance() const
{
  return m_pData->header.abundance;
}

short CPhilipsExtendedMainHeader::realignment_xOffset() const
{
  return m_pData->header.realignment.xOffset;
}

short CPhilipsExtendedMainHeader::realignment_yOffset() const
{
  return m_pData->header.realignment.yOffset;
}

short CPhilipsExtendedMainHeader::realignment_horizRotation() const
{
  return m_pData->header.realignment.horizRotation;
}

time_t CPhilipsExtendedMainHeader::acq_date_time() const
{
  return m_pData->header.acq_date_time;
}

time_t CPhilipsExtendedMainHeader::study_date_time() const
{
  return m_pData->header.study_date_time;
}

time_t CPhilipsExtendedMainHeader::injection_date_time() const
{
  return m_pData->header.injection_date_time;
}

time_t CPhilipsExtendedMainHeader::file_create_date_time() const
{
  return m_pData->header.file_create_date_time;
}

CPhilipsExtendedMainHeader::Respiration_Trigger_Location CPhilipsExtendedMainHeader::resp_trig_loc() const
{
  return static_cast<Respiration_Trigger_Location>(m_pData->header.resp_trig_loc);
}

CPhilipsExtendedMainHeader::Card_Arrhythmia_Rej_Tech CPhilipsExtendedMainHeader::card_arrhythmia_rej_tech() const
{
  return static_cast<Card_Arrhythmia_Rej_Tech>(m_pData->header.card_arrhythmia_rej_tech);
}

float CPhilipsExtendedMainHeader::window_center() const
{
  return m_pData->header.window_center;
}

float CPhilipsExtendedMainHeader::window_width() const
{
  return m_pData->header.window_width;
}

short CPhilipsExtendedMainHeader::realignment_axialRotation() const
{
  return m_pData->header.realignment.axialRotation;
}

short CPhilipsExtendedMainHeader::realignment_vertRotation() const
{
  return m_pData->header.realignment.vertRotation;
}

short CPhilipsExtendedMainHeader::resp_trig_threshold() const
{
  return m_pData->header.resp_trig_threshold;
}

short CPhilipsExtendedMainHeader::resp_phase_duration() const
{
  return m_pData->header.resp_phase_duration;
}

short CPhilipsExtendedMainHeader::resp_phase_offset() const
{
  return m_pData->header.resp_phase_offset;
}

short CPhilipsExtendedMainHeader::realignment_zOffset() const
{
  return m_pData->header.realignment.zOffset;
}

CPhilipsExtendedMainHeader::Window_Units CPhilipsExtendedMainHeader::window_units() const
{
  return static_cast<Window_Units>(m_pData->header.window_units);
}

const char* CPhilipsExtendedMainHeader::req_phys() const
{
  return m_pData->header.req_phys;
}

CPhilipsExtendedMainHeader::Card_Ph_State CPhilipsExtendedMainHeader::card_phstate() const
{
  return static_cast<Card_Ph_State>(m_pData->header.card_phstate);
}

const char* CPhilipsExtendedMainHeader::referring_physician() const
{
  return m_pData->header.referring_physician;
}

const char* CPhilipsExtendedMainHeader::study_id() const
{
  return m_pData->header.study_id;
}

float CPhilipsExtendedMainHeader::Dslice_thick() const
{
  return m_pData->header.Dslice_thick;
}

char CPhilipsExtendedMainHeader::sex() const
{
  return m_pData->header.sex;
}

float CPhilipsExtendedMainHeader::table_height() const
{
  return m_pData->header.table_height;
}

bool CPhilipsExtendedMainHeader::card_bt_rej() const
{
  return (m_pData->header.card_bt_rej == 1);
}

CPhilipsExtendedMainHeader::Card_Fr_Type CPhilipsExtendedMainHeader::card_fr_type() const
{
  return static_cast<Card_Fr_Type>(m_pData->header.card_fr_type);
}

const char* CPhilipsExtendedMainHeader::Dmanufacture_model_name() const
{
  return m_pData->header.Dmanufacture_model_name;
}

const char* CPhilipsExtendedMainHeader::Dimage_type() const
{
  return m_pData->header.Dimage_type;
}

float CPhilipsExtendedMainHeader::min_bed_pos() const
{
  return m_pData->header.min_bed_pos;
}

float CPhilipsExtendedMainHeader::max_bed_pos() const
{
  return m_pData->header.max_bed_pos;
}

bool CPhilipsExtendedMainHeader::der_filled() const
{
  return (m_pData->header.der_filled == 1);
}

long CPhilipsExtendedMainHeader::series_number() const
{
  return m_pData->header.series_number;
}

CPhilipsExtendedMainHeader::Card_Slc_Dir CPhilipsExtendedMainHeader::card_slc_dir() const
{
  return static_cast<Card_Slc_Dir>(m_pData->header.card_slc_dir);
}

int CPhilipsExtendedMainHeader::card_skip_msec() const
{
  return m_pData->header.card_skip_msec;
}

int CPhilipsExtendedMainHeader::card_skip_counts() const
{
  return m_pData->header.card_skip_counts;
}

int CPhilipsExtendedMainHeader::card_dur_msec() const
{
  return m_pData->header.card_dur_msec;
}

int CPhilipsExtendedMainHeader::card_dur_counts() const
{
  return m_pData->header.card_dur_counts;
}

int CPhilipsExtendedMainHeader::card_beats_tot() const
{
  return m_pData->header.card_beats_tot;
}

int CPhilipsExtendedMainHeader::card_beats_acc() const
{
  return m_pData->header.card_beats_acc;
}

const char* CPhilipsExtendedMainHeader::contr_bolus_agent()
{
  return m_pData->header.contr_bolus_agent;
}

const char* CPhilipsExtendedMainHeader::sop_uid()
{
  return m_pData->header.sop_uid;
}

const char* CPhilipsExtendedMainHeader::frame_ref_uid()
{
  return m_pData->header.frame_ref_uid;
}

const char* CPhilipsExtendedMainHeader::pps_file()
{
  return m_pData->header.pps_file;
}

const char* CPhilipsExtendedMainHeader::worklist_file()
{
  return m_pData->header.worklist_file;
}

void CPhilipsExtendedMainHeader::setDpat_name(const char* str)
{
  strncpy(m_pData->header.Dpat_name, str, 64);
}

void CPhilipsExtendedMainHeader::setDpat_id(const char* str)
{
  strncpy(m_pData->header.Dpat_id, str, 64);
}

void CPhilipsExtendedMainHeader::setStudy_uid(const char* str)
{
  strncpy(m_pData->header.study_uid, str, 64);
}

void CPhilipsExtendedMainHeader::setSeries_uid(const char* str)
{
  strncpy(m_pData->header.series_uid, str, 64);
}

void CPhilipsExtendedMainHeader::setView_code(const char* str)
{
  strncpy(m_pData->header.view_code, str, 20);
}

void CPhilipsExtendedMainHeader::setSortproto_name(const char* str)
{
  strncpy(m_pData->header.sortproto_name, str, 20);
}

void CPhilipsExtendedMainHeader::setRoute(const Route_Type route)
{
  m_pData->header.route = static_cast<qint16>(route);
}

void CPhilipsExtendedMainHeader::setPharm(const Pharm_Type pharm)
{
  m_pData->header.pharm = static_cast<qint16>(pharm);
}

void CPhilipsExtendedMainHeader::setAssay_date(const int assay_date)
{
  m_pData->header.assay_date = assay_date;
}

void CPhilipsExtendedMainHeader::setAssay_time(const long assay_time)
{
  m_pData->header.assay_time = assay_time;
}

void CPhilipsExtendedMainHeader::setSeries_desc(const char* str)
{
  strncpy(m_pData->header.series_desc, str, 64);
}

void CPhilipsExtendedMainHeader::setHeight(const short height)
{
  m_pData->header.height = height;
}

void CPhilipsExtendedMainHeader::setAbundance(const short abundance)
{
  m_pData->header.abundance = abundance;
}

void CPhilipsExtendedMainHeader::setRealignment_xOffset(const short xOffset)
{
  m_pData->header.realignment.xOffset = xOffset;
}

void CPhilipsExtendedMainHeader::setRealignment_yOffset(const short yOffset)
{
  m_pData->header.realignment.yOffset = yOffset;
}

void CPhilipsExtendedMainHeader::setRealignment_horizRotation(const short horizRotation)
{
  m_pData->header.realignment.horizRotation = horizRotation;
}

void CPhilipsExtendedMainHeader::setAcq_date_time(const time_t date_time)
{
  m_pData->header.acq_date_time = date_time;
}

void CPhilipsExtendedMainHeader::setStudy_date_time(const time_t date_time)
{
  m_pData->header.study_date_time = date_time;
}

void CPhilipsExtendedMainHeader::setInjection_date_time(const time_t date_time)
{
  m_pData->header.injection_date_time = date_time;
}

void CPhilipsExtendedMainHeader::setFile_create_date_time(const time_t date_time)
{
  m_pData->header.file_create_date_time = date_time;
}

void CPhilipsExtendedMainHeader::setResp_trig_loc(const CPhilipsExtendedMainHeader::Respiration_Trigger_Location location)
{
   m_pData->header.resp_trig_loc = location;
}

void CPhilipsExtendedMainHeader::setCard_arrhythmia_rej_tech(const CPhilipsExtendedMainHeader::Card_Arrhythmia_Rej_Tech rej_tech)
{
  m_pData->header.card_arrhythmia_rej_tech = rej_tech;
}

void CPhilipsExtendedMainHeader::setWindow_center(const float window_center)
{
  m_pData->header.window_center = window_center;
}

void CPhilipsExtendedMainHeader::setWindow_width(const float window_width)
{
  m_pData->header.window_width = window_width;
}

void CPhilipsExtendedMainHeader::setRealignment_axialRotation(const short axialRotation)
{
  m_pData->header.realignment.axialRotation = axialRotation;
}

void CPhilipsExtendedMainHeader::setRealignment_vertRotation(const short vertRotation)
{
  m_pData->header.realignment.vertRotation = vertRotation;
}

void CPhilipsExtendedMainHeader::setResp_trig_threshold(const short resp_trig_threshold)
{
  m_pData->header.resp_trig_threshold = resp_trig_threshold;
}

void CPhilipsExtendedMainHeader::setResp_phase_duration(const short resp_phase_duration)
{
  m_pData->header.resp_phase_duration = resp_phase_duration;
}

void CPhilipsExtendedMainHeader::setResp_phase_offset(const short resp_phase_offset)
{
  m_pData->header.resp_phase_offset = resp_phase_offset;
}

void CPhilipsExtendedMainHeader::setRealignment_zOffset(const short zOffset)
{
  m_pData->header.realignment.zOffset = zOffset;
}

void CPhilipsExtendedMainHeader::setWindow_units(const CPhilipsExtendedMainHeader::Window_Units window_units)
{
  m_pData->header.window_units = static_cast<qint16>(window_units);
}

void CPhilipsExtendedMainHeader::setReq_phys(const char* str)
{
  strncpy(m_pData->header.req_phys, str, 64);
}

void CPhilipsExtendedMainHeader::setCard_phstate(const CPhilipsExtendedMainHeader::Card_Ph_State state)
{
  m_pData->header.card_phstate = static_cast<quint16>(state);
}

void CPhilipsExtendedMainHeader::setReferring_physician(const char* str)
{
  strncpy(m_pData->header.referring_physician, str, 64);
}

void CPhilipsExtendedMainHeader::setStudy_id(const char* str)
{
  strncpy(m_pData->header.study_id, str, 16);
}

void CPhilipsExtendedMainHeader::setDslice_thick(const float Dslice_thick)
{
  m_pData->header.Dslice_thick = Dslice_thick;
}

void CPhilipsExtendedMainHeader::setSex(const char sex)
{
  m_pData->header.sex = sex;
}

void CPhilipsExtendedMainHeader::setTable_height(const float table_height)
{
  m_pData->header.table_height = table_height;
}

void CPhilipsExtendedMainHeader::setCard_bt_rej(const bool flag)
{
  m_pData->header.card_bt_rej = (flag == true)? 1 : 0;
}

void CPhilipsExtendedMainHeader::setCard_fr_type(const CPhilipsExtendedMainHeader::Card_Fr_Type type)
{
  m_pData->header.card_fr_type = static_cast<qint16>(type);
}

void CPhilipsExtendedMainHeader::setDmanufacture_model_name(const char* str)
{
  strncpy(m_pData->header.Dmanufacture_model_name, str, 64);
}

void CPhilipsExtendedMainHeader::setDimage_type(const char* str)
{
  strncpy(m_pData->header.Dimage_type, str, 64);
}

void CPhilipsExtendedMainHeader::setMin_bed_pos(const float min)
{
  m_pData->header.min_bed_pos = min;
}

void CPhilipsExtendedMainHeader::setMax_bed_pos(const float max)
{
  m_pData->header.max_bed_pos = max;
}

void CPhilipsExtendedMainHeader::setDer_filled(const bool flag)
{
  m_pData->header.der_filled = (flag == true)? 1 : 0;
}

void CPhilipsExtendedMainHeader::setSeries_number(const long number)
{
  m_pData->header.series_number = number;
}

void CPhilipsExtendedMainHeader::setCard_slc_dir(const CPhilipsExtendedMainHeader::Card_Slc_Dir direction)
{
  m_pData->header.card_slc_dir = static_cast<qint16>(direction);
}

void CPhilipsExtendedMainHeader::setCard_skip_msec(const int msec)
{
  m_pData->header.card_skip_msec = msec;
}

void CPhilipsExtendedMainHeader::setCard_skip_counts(const int counts)
{
  m_pData->header.card_skip_counts = counts;
}

void CPhilipsExtendedMainHeader::setCard_dur_msec(const int msec)
{
  m_pData->header.card_dur_msec = msec;
}

void CPhilipsExtendedMainHeader::setCard_dur_counts(const int counts)
{
  m_pData->header.card_dur_counts = counts;
}

void CPhilipsExtendedMainHeader::setCard_beats_tot(const int beats)
{
  m_pData->header.card_beats_tot = beats;
}

void CPhilipsExtendedMainHeader::setCard_beats_acc(const int acc)
{
  m_pData->header.card_beats_acc = acc;
}

void CPhilipsExtendedMainHeader::setContr_bolus_agent(const char* str)
{
  strncpy(m_pData->header.contr_bolus_agent, str, 64);
}

void CPhilipsExtendedMainHeader::setSop_uid(const char* str)
{
  strncpy(m_pData->header.sop_uid, str, 64);
}

void CPhilipsExtendedMainHeader::setFrame_ref_uid(const char* str)
{
  strncpy(m_pData->header.frame_ref_uid, str, 64);
}

void CPhilipsExtendedMainHeader::setPps_file(const char* str)
{
  strncpy(m_pData->header.pps_file, str, 30);
}

void CPhilipsExtendedMainHeader::setWorklist_file(const char* str)
{
  strncpy(m_pData->header.worklist_file, str, 30);
}

