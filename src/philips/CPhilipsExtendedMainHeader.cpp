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
      char req_phys[64];                  /* requesting physician */
      qint16 card_phstate;                /* Physiologic state */

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

      // long assay_date;                   /* Date of radioactivity measurement */
      // long assay_time;                   /* Time of radioactivity measurement */
      // RouteTypes route;                  /* How the isotope was delivered */
      // PharmTypes pharm;                  /* Pharmaceutical used in the study */
      // char radiopharm_name[64]; /* Used when pharm == Other, to */
      //                                    /*  specify name of radiopharmaceutical used. */
      // long study_date_time;              /* Date/time study started (UTC). Use access functions to read/write. */
      // short abundance;                   /* Positron abundance ratio, in tenths of percent */
      //                                    /*  so that 1000 is 100% */
      // short height;                      /* Patient height, in mm */
      // long injection_date_time;          /* Date/time of injection (UTC). Use access functions to read/write. */
      // long acq_date_time;                /* Date/time data acquired (UTC). Use access functions to read/write. */

      // char Dserial_number[17];           /* System serial number + null */
      // char series_desc[64];   /* Description */
      // char attncor_label[64]; /* Label (UID) identifying related atten. corr. series. */
      // float window_center;               /* Suggested center value to use when displaying this image. */
      // float window_width;                /* Suggested width value to use when displaying this image. */
      // WinUnitsTypes window_units;        /* Units in which window_center and window_width are specified. */
      // short card_arrhythmia_rej_tech;    /* Card Arrhythmia rejection techniques employed: */
      //                                    /* Bi mask:   */
      //                                    /*  0 = no rejection */
      //                                    /*  1 = Rejection based on deviation from average R-R interval */
      //                                    /*  2 = Rejection based on deviation from regular QRS loop.*/
      //                                    /*  4 = Rejection based on PVC criteria. */
      // short pvc_threshold;               /* When PVC rejection used, the */
      //                                    /*  the % of R-R below which is */
      //                                    /*  considered a PVC.           */
      // short card_skip_beats;             /* Num beats skipped after arrhythmia. */
      // char ref_gated_qc_image_inst_uid[64]; /* SOP Instance UID of */
      //                                    /*  Sec. Cap. image containing picture */
      //                                    /*  of cardiac waveform and trigger level.*/
      // RespTrigLocTypes resp_trig_loc;    /* Respiratory trigger location. */
      // short resp_trig_threshold;         /* Respiratory trigger threshold in % of chest */
      //                                    /*  expansion relative to last peak. */
      // short resp_phase_duration;         /* Respiratory acquisition duration in % */
      //                                    /*  of the respiratory cycle. */
      // short resp_phase_offset;           /* For respiratory gated, the offset from the trigger */
      //                                    /*  to the beginning of the acquisition, in % */
      //                                    /*  of the respiratory cycle. */
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
  stream.skipRawData(4);                                      // 296: skip the next 4 bytes
  stream.readRawData(&m_pData->header.req_phys[0], 64);       // 300: req_phys
  stream >> m_pData->header.card_phstate;                     // 364: card_phstate
  stream.skipRawData(146);                                    // 368: skip the next 146 bytes


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
  D("Dpat_name                 : %s", m_pData->header.Dpat_name);
  D("Dpat_id                 : %s", m_pData->header.Dpat_id);
  D("study_uid                 : %s", m_pData->header.study_uid);
  D("series_uid                 : %s", m_pData->header.series_uid);
  D("view_code                 : %s", m_pData->header.view_code);
  D("sortproto_name                 : %s", m_pData->header.sortproto_name);
  D("req_phys                 : %s", m_pData->header.req_phys);
  D("card_phstate             : %d", m_pData->header.card_phstate);

  D("referring_physician      : %s", m_pData->header.referring_physician);
  D("study_id                 : %s", m_pData->header.study_id);

  D(" Dslice_thick   : %f", m_pData->header.Dslice_thick);
  D(" sex   : %c", m_pData->header. sex);
  D(" table_height   : %f", m_pData->header.table_height);
  D(" card_bt_rej   : %d", m_pData->header.card_bt_rej);
  D(" card_fr_type   : %d", m_pData->header.card_fr_type);
  D(" Dmanufacture_model_name   : %s", m_pData->header.Dmanufacture_model_name);
  D(" Dimage_type   : %s", m_pData->header.Dimage_type);
  D(" min_bed_pos   : %f", m_pData->header.min_bed_pos);
  D(" max_bed_pos   : %f", m_pData->header.max_bed_pos);
  D(" der_filled   : %d", m_pData->header.der_filled);
  D(" series_number   : %d", m_pData->header.series_number);
  D(" dep_study_date   : %ld", m_pData->header.dep_study_date);
  D(" dep_study_time   : %ld", m_pData->header.dep_study_time);
  D(" dep_acq_time   : %ld", m_pData->header.dep_acq_time);
  D(" card_slc_dir   : %d", m_pData->header.card_slc_dir);

  D("card_skip_msec:   : %d", m_pData->header.card_skip_msec);
  D("card_skip_counts  : %d", m_pData->header.card_skip_counts);
  D("card_dur_msec  : %d", m_pData->header.card_dur_msec);
  D("card_dur_counts  : %d", m_pData->header.card_dur_counts);
  D("card_beats_tot  : %d", m_pData->header.card_beats_tot);
  D("card_beats_acc  : %d", m_pData->header.card_beats_acc);
  D("dep_acq_date  : %ld", m_pData->header.dep_acq_date);

  D("contr_bolus_agent: %s", m_pData->header.contr_bolus_agent);
  D("sop_uid          : %s", m_pData->header.sop_uid);
  D("frame_ref_uid    : %s", m_pData->header.frame_ref_uid);
  D("pps_file         : %s", m_pData->header.pps_file);
  D("worklist_file    : %s", m_pData->header.worklist_file);
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

