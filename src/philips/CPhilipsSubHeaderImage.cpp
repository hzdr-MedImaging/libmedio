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

#include "CPhilipsSubHeaderImage.h"
#include "CPhilipsDirectoryItem.h"
#include "CPhilipsFile.h"

#include <QDataStream>

#include <rtdebug.h>

class CPhilipsSubHeaderImagePrivate
{
  public:
  struct HeaderData
  {
    qint16 version;        /* 0 -> original version (w/ reconinfo)
                             1 -> no reconinfo; w/ DICOM recon data */
    qint16 datype;        /* Data type
                          * (1=byte,2=I*2,4=R*4orI*4,8=R*8) */
    qint16 xdim;          /* X dimension (128 for .IMG, 256 or 128 for .SCN) */
    qint16 ydim;          /* Y dimension (128 for .IMG, 192 or 96 for .SCN) */
    qint16 slcnum;        /* Slice number. */
    /* syn - numbered 1-n starting with slice at head */
    /* after volume reoriented and sorted head first supine */
    qint16 tiltnum;       /* Tilt Number. */
    qint16 gatint;        /* Gating interval (msec). */
    float magfac;        /* Reconstruction magnification (nominally
                          * 1). Used in analyze anz2u.c in imagio.c
                          * Convert UCLA to Analyze Format */
    float imgscl;        /* Image scale factor (nominally 1.) */
    qint16 imgmin;          /* Minimum value in image; scaled to short by 
                          * wrshdr() */
    qint16 imgmax;          /* Maximum value in image; scaled to short
                          * by wrshdr() */
    float suvscl;        /* SUV scale factor (non-zero only after
                          * running suv program) */
    qint16 strhr;     /* (Deprecated - use start_date_time) Starting time of this frame: hour. */
    qint16 strmin;    /* (Deprecated - use start_date_time) Starting time of this frame: minute. */
    qint16 strsec;    /* (Deprecated - use start_date_time) Starting time of this frame: second. */

    qint32 start_date_time; /* UTC starting date/time of this frame. Use access functions to read/write.*/
    qint16 start_date_time_msec; /* Fraction of a second (in msec) to be added to start_date_time ... */
                                /*  to give a more accurate start time. Use access functions to read/write.*/

    qint16 endhr;     /* (Deprecated - use end_date_time) Ending time of this frame: hour. */
    qint16 endmin;    /* (Deprecated - use end_date_time) Ending time of this frame: minute. */
    qint16 endsec;    /* (Deprecated - use end_date_time) Ending time of this frame: second. */

    qint32 end_date_time;  /* UTC ending date/time of this frame. Use access functions to read/write.*/
    qint16 end_date_time_msec; /* Fraction of a second (in msec) to be added to end_date_time ... */
                              /*  to give a more accurate end time. Use access functions to read/write.*/

    qint32 midtim;         /* Time from start of scan to frame midtime
                          * of frame. */
    qint32 frame_ref_date_time; /* The point in time most representative */
    /*  of when data was acquired for this frame. Use access functions to read/write.*/
    qint16 frame_ref_date_time_msec; /* fraction of a second (in msec) to add to frame_ref_date_time */
                                    /*  to give a more accurate frame reference time. Use access functions */
                                    /*  to read/write this field. */
    /* midtim used by cmrglu.c */
    qint16 mseclen;       /* Duration in millesecs (used for EC scan, e.g. */
    qint16 scnlen;        /* Scan duration in seconds. */
    float imgsum;        /* Total number of events in image. */

    /* additional parameters to add to the subheader: */
    /* Space available from words 113 - 256 */

    float bgsngrt;       /* Beginning singles rate - all detectors */
    /* syn - (0020,1041) Slice Location used as */
    /* table position of image, Image position z */
    /* is used if slice location unavailable */
    float bgcoincrt;     /* Beginning coincidence rate - all
                          * bankpairs */
    /* syn - (0018,0050) Slice Thickness */
    /* needed because slcthk not float	*/
    float bgdelrt;       /* Beginning delays rate - all bankpairs */
    float endsngrt;      /* Ending singles rate - all detectors */
    float endcoincrt;    /* Ending coincidence rate - all bankpairs */
    float enddelrt;      /* Ending delays rate - all bankpairs */
    float deadtimefac;   /* Deadtime Correction Factor default=1.0 */
    qint16 bedpos;        /* Bed position for whole body scanning */
    float actual_bedpos; /* Actual bed position for whole body scanning */
    float deadtime_bgsub;/* Deadtime Correction Factor with background 
                          * subtraction */

    float xray_current;   /* x ray current in mA */
    float kvp;            /* peak kilo voltage output of x ray generator */
    float img_pos[3];     /* mm x,y,z coordinates of upper left hand corner */
    float pix_spacing[2]; /* mm between ctr of pixel and next row and column */
    float Dslice_loc;     /* relative position of exposure in mm */
    float echo_time;      /* DICOM echo time */
    float exposure_time;  /* Exposure time */
    float orientation[6]; /* Image orientation */
    char sop_uid[64];  /* UID of an image within a series */
    char atten_corr[16];  /* Atten corr used */
    qint32 card_fr_time;    /* Nominal/actual cardiac frame time */
    qint32 card_high_rr;    /* R-R interval upper limit, msec */
    qint32 card_low_rr;     /* R-R interval lower limit, msec */
    qint32 card_tr_time;    /* Msec from trigger to scanning */
    qint32 card_rr_time;    /* R-R (nominal) peak interval, in msec, */
    /*  from cardiac cycles used to acquire */
    /*  this frame. */
    qint32 resp_int_time;   /* Average measured interval time, in msec, */
    /*  from one resp trigger to the next for */
    /*  resp cycles in which this image occurs. */
    char scatter_corr[16];  /* Scatter corr used */

    qint16 deadtime_corr;  /* Whether deadtime corr was used (bool)*/

    /*Randoms_Type*/qint16 randoms_corr;  /* Type of randoms correction */

    qint16 det_norm;   /* Whether the detector was normalized (bool)*/
    qint16 nu_radsamp_corr;  /* Whether non-unif radial samp corr was used (bool)*/
    qint16 pat_mot_corr;   /* Whether patient motion corr was used (bool)*/
    qint16 cntloss_corr;   /* Whether count loss corr was applied (bool)*/

    /*Decay_Type*/qint16 decay_corr; /* Type of decay correction */
    char recon_method[16];  /* Recon algorithm used */
    /* Laterality_Type */qint16 laterality; /* Description of the laterality of */
                                /*  (possibly paired) body parts.   */
    /*Anatomy_Type*/qint16 anatomy;   /* Identifies the anatomic region of interest */
  } header;

  static const short currentSubHeaderVersion = 1;
};

CPhilipsSubHeaderImage::CPhilipsSubHeaderImage(CPhilipsFile* philipsFile,
                                               CPhilipsDirectoryItem* pDirItem)
  : CPhilipsSubHeader(philipsFile, pDirItem)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CPhilipsSubHeaderImagePrivate();
  
  clear();

  LEAVE();
}

CPhilipsSubHeaderImage::~CPhilipsSubHeaderImage()
{
  ENTER();

  delete m_pData;

  LEAVE();
}

CPhilipsSubHeaderImage::CPhilipsSubHeaderImage(const CPhilipsSubHeaderImage& src)
  : CPhilipsSubHeader(src)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CPhilipsSubHeaderImagePrivate(*(src.m_pData));

  LEAVE();
}

CPhilipsSubHeaderImage& CPhilipsSubHeaderImage::operator=(const CPhilipsSubHeaderImage& src)
{
  ENTER();

  if(m_pData != src.m_pData)
  {
    memcpy(&m_pData->header, 
           &src.m_pData->header, 
           sizeof(struct CPhilipsSubHeaderImagePrivate::HeaderData));
  }

  LEAVE();
  return *this;
}

void CPhilipsSubHeaderImage::clear()
{
  ENTER();

  // clear our header structure first
  memset(&m_pData->header, 0, sizeof(struct CPhilipsSubHeaderImagePrivate::HeaderData));
  
  // set some default values
  m_pData->header.version = CPhilipsSubHeaderImagePrivate::currentSubHeaderVersion;
  m_pData->header.magfac = 1.0; // Not used. always set to 1.0

  LEAVE();
}

bool CPhilipsSubHeaderImage::load(void)
{
  ENTER();

  // check if the stream is readable or not and
  // set our MedIOData to the correct file position so that we can
  // read the subheader  
  if(m_pMedIOData->isReadable() == false ||
     m_pDirItem->dataBlock_Start() == 0 ||
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
  stream.skipRawData(14);                               // 0: skip 14 bytes
  stream >> m_pData->header.version;                    // 0: version
  stream.readRawData(&m_pData->header.atten_corr[0], 16);  // 2: atten_corr
  stream >> m_pData->header.actual_bedpos;              // 18: actual_bedpos
  stream >> m_pData->header.orientation[0];             // 22: orientation[0]
  stream >> m_pData->header.orientation[1];             // 26: orientation[1]
  stream >> m_pData->header.orientation[2];             // 30: orientation[2]
  stream.skipRawData(4);                                // 34: skip 4 bytes
  stream >> m_pData->header.orientation[3];             // 38: orientation[3]
  stream >> m_pData->header.orientation[4];             // 42: orientation[4]
  stream >> m_pData->header.orientation[5];             // 46: orientation[5]
  stream >> m_pData->header.card_fr_time;               // 50: card_fr_time
  stream >> m_pData->header.card_high_rr;               // 54: card_high_rr
  stream >> m_pData->header.card_low_rr;                // 58 : card_low_rr
  stream >> m_pData->header.card_tr_time;               // 54: card_high_rr
  stream.readRawData(m_pData->header.scatter_corr, 16); // 58: scatter_corr
  stream >> m_pData->header.deadtime_corr;              // 74: deadtime_corr
  stream >> m_pData->header.randoms_corr;               // 76: randoms_corr
  stream >> m_pData->header.det_norm;                   // 78: det_norm
  stream >> m_pData->header.nu_radsamp_corr;            // 80: nu_radsamp_corr
  stream >> m_pData->header.pat_mot_corr;               // 82: pat_mot_corr
  stream >> m_pData->header.echo_time;                  // 84: echo_time
  stream >> m_pData->header.exposure_time;              // 88: exposure_time
  stream >> m_pData->header.img_pos[0];                 // 94: img_pos[0]
  stream >> m_pData->header.img_pos[1];                 // 98: img_pos[1]
  stream.skipRawData(4);                                // 102: skip next 4 bytes
  stream >> m_pData->header.datype;                     // 106: datype
  stream >> m_pData->header.img_pos[2];                 // 108: img_pos[2]
  stream >> m_pData->header.xdim;                       // 112: xdim
  stream >> m_pData->header.ydim;                       // 114: ydim
  stream >> m_pData->header.slcnum;                     // 116: slcnum
  stream >> m_pData->header.tiltnum;                    // 118: tiltnum
  stream >> m_pData->header.gatint;                     // 120: gatint
  stream >> m_pData->header.cntloss_corr;               // 122: cntloss_corr
  stream >> m_pData->header.pix_spacing[0];             // 124: pix_spacing[0]
  stream >> m_pData->header.pix_spacing[1];             // 128: pix_spacing[1]
  stream >> m_pData->header.xray_current;               // 132: xray_current
  stream >> m_pData->header.suvscl;                     // 136: suvscl
  stream >> m_pData->header.kvp;                        // 140: kvp
  stream >> m_pData->header.Dslice_loc;                 // 144: Dslice_loc
  stream >> m_pData->header.magfac;                     // 148: magfac
  stream >> m_pData->header.imgscl;                     // 152: imgscl
  stream >> m_pData->header.imgmin;                     // 156: imgmin
  stream >> m_pData->header.imgmax;                     // 158: imgmax
  stream >> m_pData->header.decay_corr;                 // 160: decay_corr
  stream.skipRawData(4);                                // 162: skip: scnscl (Sinogram only)
  stream >> m_pData->header.strhr;                      // 166: strhr
  stream >> m_pData->header.strmin;                     // 168: strmin
  stream >> m_pData->header.strsec;                     // 170: strsec
  stream.skipRawData(4);                                // 172: skip: scnmin and scnmax (Sinogram only)
  stream >> m_pData->header.endhr;                      // 176: endhr
  stream >> m_pData->header.endmin;                     // 178: endmin
  stream >> m_pData->header.endsec;                     // 180: endsec
  stream >> m_pData->header.midtim;                     // 182: midtim
  stream.skipRawData(2);                                // 186: skip 2 bytes
  stream >> m_pData->header.mseclen;                    // 188: mseclen
  stream >> m_pData->header.scnlen;                     // 190: scnlen
  stream >> m_pData->header.imgsum;                     // 192: imgsum
  stream.skipRawData(4);                                // 196: skip: scnsum (Sinogram only)
  stream >> m_pData->header.bgdelrt;                    // 200: bgdelrt
  stream >> m_pData->header.enddelrt;                   // 204: enddelrt
  stream >> m_pData->header.bgsngrt;                    // 208: bgsngrt
  stream >> m_pData->header.bgcoincrt;                  // 212: bgcoincrt
  stream >> m_pData->header.endsngrt;                   // 216: endsngrt
  stream >> m_pData->header.endcoincrt;                 // 220: endcoincrt
  stream >> m_pData->header.deadtimefac;                // 224: deadtimefac
  stream >> m_pData->header.bedpos;                     // 230: bedpos
  stream >> m_pData->header.deadtime_bgsub;             // 232: deadtime_bgsub
  stream.skipRawData(2);                                // 234: skip next 2 bytes
  stream.readRawData(&m_pData->header.sop_uid[0], 64);  //
  stream.readRawData(&m_pData->header.recon_method[0], 16);  //

 // some more debug output
#if defined(DEBUG)
  D("Philips Image SubHeader loaded:");
  D("----------------------------");
  D("version        : %d", m_pData->header.version);
  D("atten_corr     : %s", m_pData->header.atten_corr);
  D("actual_bedpos  : %f", m_pData->header.actual_bedpos);
  D("orientation[0] : %f", m_pData->header.orientation[0]);
  D("orientation[1] : %f", m_pData->header.orientation[1]);
  D("orientation[2] : %f", m_pData->header.orientation[2]);
  D("orientation[3] : %f", m_pData->header.orientation[3]);
  D("orientation[4] : %f", m_pData->header.orientation[4]);
  D("orientation[5] : %f", m_pData->header.orientation[5]);
  D("card_fr_time   : %d", m_pData->header.card_fr_time);
  D("card_high_rr   : %d", m_pData->header.card_high_rr);
  D("card_low_rr    : %d", m_pData->header.card_low_rr);
  D("card_high_rr   : %d", m_pData->header.card_tr_time);
  D("scatter_corr   : %s", m_pData->header.scatter_corr);
  D("deadtime_corr  : %d", m_pData->header.deadtime_corr);
  D("randoms_corr   : %d", m_pData->header.randoms_corr);
  D("det_norm       : %d", m_pData->header.det_norm);
  D("nu_radsamp_corr: %d", m_pData->header.nu_radsamp_corr);
  D("pat_mot_corr   : %d", m_pData->header.pat_mot_corr);
  D("echo_time      : %f", m_pData->header.echo_time);
  D("exposure_time  : %f", m_pData->header.exposure_time);
  D("img_pos[0]     : %f", m_pData->header.img_pos[0]);
  D("img_pos[1]     : %f", m_pData->header.img_pos[1]);
  D("img_pos[2]     : %f", m_pData->header.img_pos[2]);
  D("datyp          : %d", m_pData->header.datype);
  D("xdim           : %d", m_pData->header.xdim);
  D("ydim           : %d", m_pData->header.ydim);
  D("slcnum         : %d", m_pData->header.slcnum);
  D("tiltnum        : %d", m_pData->header.tiltnum);
  D("gatint         : %d", m_pData->header.gatint);
  D("cntloss_corr   : %d", m_pData->header.cntloss_corr);
  D("pix_spacing[0] : %f", m_pData->header.pix_spacing[0]);
  D("pix_spacing[1] : %f", m_pData->header.pix_spacing[1]);
  D("xray_current   : %f", m_pData->header.xray_current);
  D("suvscl         : %f", m_pData->header.suvscl);
  D("kvp            : %f", m_pData->header.kvp);
  D("Dslice_loc     : %f", m_pData->header.Dslice_loc);
  D("magfac         : %f", m_pData->header.magfac);
  D("imgscl         : %f", m_pData->header.imgscl);
  D("imgmin         : %d", m_pData->header.imgmin);
  D("imgmax         : %d", m_pData->header.imgmax);
  D("decay_corr     : %d", m_pData->header.decay_corr);
  D("strhr          : %d", m_pData->header.strhr);
  D("strmin         : %d", m_pData->header.strmin);
  D("strsec         : %d", m_pData->header.strsec);
  D("endhr          : %d", m_pData->header.endhr);
  D("endmin         : %d", m_pData->header.endmin);
  D("endsec         : %d", m_pData->header.endsec);
  D("midtim         : %d", m_pData->header.midtim);
  D("mseclen        : %d", m_pData->header.mseclen);
  D("scnlen         : %d sec", m_pData->header.scnlen);
  D("imgsum         : %f", m_pData->header.imgsum);
  D("bgdelrt        : %f", m_pData->header.bgdelrt);
  D("enddelrt       : %f", m_pData->header.enddelrt);
  D("bgsngrt        : %f", m_pData->header.bgsngrt);
  D("bgcoincrt      : %f", m_pData->header.bgcoincrt);
  D("endsngrt       : %f", m_pData->header.endsngrt);
  D("endcoincrt     : %f", m_pData->header.endcoincrt);
  D("deadtimefac    : %f", m_pData->header.deadtimefac);
  D("bedpos         : %d", m_pData->header.bedpos);
  D("deadtime_bgsub : %f", m_pData->header.deadtime_bgsub);
  D("sop_uid        : %s", m_pData->header.sop_uid);
  D("recon_method   : %s", m_pData->header.recon_method);
#endif

  RETURN(true);
  return true;
}

bool CPhilipsSubHeaderImage::save(void) const
{
  ENTER();
#warning TODO: implement CPhilipsSubHeaderImage::save()
  RETURN(false);
  return false;
}

int CPhilipsSubHeaderImage::rawDataSize() const
{
  return 1*PHILIPS_BLOCKSIZE;
}

CPhilipsSubHeader::Type CPhilipsSubHeaderImage::subHeaderType(void) const
{
  return CPhilipsSubHeader::Image;
}

bool CPhilipsSubHeaderImage::convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader* pHead2) 
{
  ENTER();
  bool bResult = false;
#warning TODO: implement CPhilipsSubHeaderImage::convertFrom()
  RETURN(bResult);
  return bResult;
}

CMedIOHeader* CPhilipsSubHeaderImage::clone() const
{
  ENTER();

  CPhilipsSubHeaderImage* pNewHead = new CPhilipsSubHeaderImage();
  pNewHead->convertFrom(this);
  
  RETURN(pNewHead);
  return pNewHead;
}

// data acess methods
short CPhilipsSubHeaderImage::version() const
{
  return m_pData->header.version;
}

const char* CPhilipsSubHeaderImage::atten_corr() const
{
  return m_pData->header.atten_corr;
}

float CPhilipsSubHeaderImage::actual_bedpos() const
{
  return m_pData->header.actual_bedpos;
}

float CPhilipsSubHeaderImage::orientation(const short i) const
{
  if((i >= 0) || (i <= 6))
    return m_pData->header.orientation[i];
  else
    return 0.0f;
}

long CPhilipsSubHeaderImage::card_fr_time() const
{
  return m_pData->header.card_fr_time;
}

long CPhilipsSubHeaderImage::card_high_rr() const
{
  return m_pData->header.card_high_rr;
}

long CPhilipsSubHeaderImage::card_low_rr() const
{
  return m_pData->header.card_low_rr;
}

long CPhilipsSubHeaderImage::card_tr_time() const
{
  return m_pData->header.card_tr_time;
}

const char* CPhilipsSubHeaderImage::scatter_corr() const
{
  return m_pData->header.scatter_corr;
}

short CPhilipsSubHeaderImage::deadtime_corr() const
{
  return m_pData->header.deadtime_corr;
}

CPhilipsSubHeaderImage::Randoms_Type CPhilipsSubHeaderImage::randoms_corr() const
{
  return static_cast<CPhilipsSubHeaderImage::Randoms_Type>(m_pData->header.randoms_corr);
}

short CPhilipsSubHeaderImage::det_norm() const
{
  return m_pData->header.det_norm;
}

short CPhilipsSubHeaderImage::nu_radsamp_corr() const
{
  return m_pData->header.nu_radsamp_corr;
}

short CPhilipsSubHeaderImage::pat_mot_corr() const
{
  return m_pData->header.pat_mot_corr;
}

float  CPhilipsSubHeaderImage::echo_time() const
{
  return m_pData->header.echo_time;
}

float CPhilipsSubHeaderImage::exposure_time() const
{
  return m_pData->header.exposure_time;
}

float CPhilipsSubHeaderImage::img_pos_x() const
{

  return m_pData->header.img_pos[0];
}

float CPhilipsSubHeaderImage::img_pos_y() const
{
  return m_pData->header.img_pos[1];
}

float CPhilipsSubHeaderImage::img_pos_z() const
{
  return m_pData->header.img_pos[2];
}

CPhilipsSubHeader::Data_Type CPhilipsSubHeaderImage::datype() const
{
  return static_cast<CPhilipsSubHeader::Data_Type>(m_pData->header.datype);
}

short CPhilipsSubHeaderImage::xdim() const
{
  return m_pData->header.xdim;
}

short CPhilipsSubHeaderImage::ydim() const
{
  return m_pData->header.ydim;
}

short CPhilipsSubHeaderImage::slcnum() const
{
  return m_pData->header.slcnum;
}

short CPhilipsSubHeaderImage::tiltnum() const
{
  return m_pData->header.tiltnum;
}

short CPhilipsSubHeaderImage::gatint() const
{
  return m_pData->header.gatint;
}

short CPhilipsSubHeaderImage::cntloss_corr() const
{
  return m_pData->header.cntloss_corr;
}

float CPhilipsSubHeaderImage::pix_spacing(const short i) const
{
  if((i >= 0) || (i <= 2))
    return m_pData->header.pix_spacing[i];
  else
    return 0.0;
}

float CPhilipsSubHeaderImage::xray_current() const
{
  return m_pData->header.xray_current;
}

float CPhilipsSubHeaderImage::suvscl() const
{
  return m_pData->header.suvscl;
}

float CPhilipsSubHeaderImage::kvp() const
{
  return m_pData->header.kvp;
}

float CPhilipsSubHeaderImage::Dslice_loc() const
{
  return m_pData->header.Dslice_loc;
}

float CPhilipsSubHeaderImage::magfac() const
{
  return m_pData->header.magfac;
}

float CPhilipsSubHeaderImage::imgscl() const
{
  return m_pData->header.imgscl;
}

short CPhilipsSubHeaderImage::imgmin() const
{
  return m_pData->header.imgmin;
}

short CPhilipsSubHeaderImage::imgmax() const
{
  return m_pData->header.imgmax;
}

CPhilipsSubHeaderImage::Decay_Type CPhilipsSubHeaderImage::decay_corr() const
{
  return static_cast<CPhilipsSubHeaderImage::Decay_Type>(m_pData->header.decay_corr);
}
short CPhilipsSubHeaderImage::strhr() const
{
  return m_pData->header.strhr;
}

short CPhilipsSubHeaderImage::strmin() const
{
  return m_pData->header.strmin;
}

short CPhilipsSubHeaderImage::strsec() const
{
  return m_pData->header.strsec;
}

short CPhilipsSubHeaderImage::endhr() const
{
  return m_pData->header.endhr;
}

short CPhilipsSubHeaderImage::endmin() const
{
  return m_pData->header.endmin;
}

short CPhilipsSubHeaderImage::endsec() const
{
  return m_pData->header.endsec;
}

long CPhilipsSubHeaderImage::midtim() const
{
  return m_pData->header.midtim;
}

short CPhilipsSubHeaderImage::mseclen() const
{
  return m_pData->header.mseclen;
}

short CPhilipsSubHeaderImage::scnlen() const
{
  return m_pData->header.scnlen;
}

float CPhilipsSubHeaderImage::imgsum() const
{
  return m_pData->header.imgsum;
}

float CPhilipsSubHeaderImage::bgdelrt() const
{
  return m_pData->header.bgdelrt;
}

float CPhilipsSubHeaderImage::enddelrt() const
{
  return m_pData->header.enddelrt;
}

float CPhilipsSubHeaderImage::bgsngrt() const
{
  return m_pData->header.bgsngrt;
}

float CPhilipsSubHeaderImage::bgcoincrt() const
{
  return m_pData->header.bgcoincrt;
}

float CPhilipsSubHeaderImage::endsngrt() const
{
  return m_pData->header.endsngrt;
}

float CPhilipsSubHeaderImage::endcoincrt() const
{
  return m_pData->header.endcoincrt;
}

float CPhilipsSubHeaderImage::deadtimefac() const
{
  return m_pData->header.deadtimefac;
}

short CPhilipsSubHeaderImage::bedpos() const
{
  return m_pData->header.bedpos;
}

float CPhilipsSubHeaderImage::deadtime_bgsub() const
{
  return m_pData->header.deadtime_bgsub;
}

const char* CPhilipsSubHeaderImage::sop_uid() const
{
  return m_pData->header.sop_uid;
}

const char* CPhilipsSubHeaderImage::recon_method() const
{
  return m_pData->header.recon_method;
}

// methods to modify elements of the SubHeader  
void CPhilipsSubHeaderImage::setVersion(const short version)
{
  m_pData->header.version = version;
}

void CPhilipsSubHeaderImage::setAtten_corr(const char* str)
{
  strncpy(m_pData->header.atten_corr, str, 16);
}

void CPhilipsSubHeaderImage::setActual_bedpos(const float bedpos)
{
  m_pData->header.bedpos = bedpos;
}

void CPhilipsSubHeaderImage::setOrientation(const short i, const float orientation)
{
  if((i >= 0) || (i <= 6))
    m_pData->header.orientation[i] = orientation;
}

void CPhilipsSubHeaderImage::setCard_fr_time(const long card_fr_time)
{
  m_pData->header.card_fr_time = card_fr_time;
}

void CPhilipsSubHeaderImage::setCard_high_rr(const long card_high_rr)
{
  m_pData->header.card_high_rr = card_high_rr;
}

void CPhilipsSubHeaderImage::setCard_low_rr(const long card_low_rr)
{
  m_pData->header.card_low_rr = card_low_rr;
}

void CPhilipsSubHeaderImage::setCard_tr_time(const long card_high_rr)
{
  m_pData->header.card_high_rr = card_high_rr;
}

void CPhilipsSubHeaderImage::setScatter_corr(const char* str)
{
  strncpy(m_pData->header.scatter_corr, str, 16);
}

void CPhilipsSubHeaderImage::setDeadtime_corr(const short deadtime_corr)
{
  m_pData->header.deadtime_corr = deadtime_corr;
}

void CPhilipsSubHeaderImage::setRandoms_corr(const CPhilipsSubHeaderImage::Randoms_Type randoms_corr)
{
  m_pData->header.randoms_corr = static_cast<qint16>(randoms_corr);
}

void CPhilipsSubHeaderImage::setDet_norm(const short det_norm)
{
  m_pData->header.det_norm = det_norm;
}

void CPhilipsSubHeaderImage::setNu_radsamp_corr(const short nu_radsamp_corr)
{
  m_pData->header.nu_radsamp_corr = nu_radsamp_corr;
}

void CPhilipsSubHeaderImage::setPat_mot_corr(const short pat_mot_corr)
{
  m_pData->header.pat_mot_corr = pat_mot_corr;
}

void CPhilipsSubHeaderImage::setEcho_time(const float  echo_time)
{
  m_pData->header.echo_time = echo_time;
}

void CPhilipsSubHeaderImage::setExposure_time(const float exposure_time)
{
  m_pData->header.exposure_time = exposure_time;
}

void CPhilipsSubHeaderImage::setImg_pos_x(const float img_pos_x)
{
  m_pData->header.img_pos[0] = img_pos_x;
}

void CPhilipsSubHeaderImage::setImg_pos_y(const float img_pos_y)
{
  m_pData->header.img_pos[1] = img_pos_y;
}

void CPhilipsSubHeaderImage::setImg_pos_z(const float img_pos_z)
{
  m_pData->header.img_pos[2] = img_pos_z;
}

void CPhilipsSubHeaderImage::setDatype(const CPhilipsSubHeader::Data_Type datype)
{
  m_pData->header.datype = static_cast<qint16>(datype);
}

void CPhilipsSubHeaderImage::setXdim(const short xdim)
{
  m_pData->header.xdim = xdim;
}

void CPhilipsSubHeaderImage::setYdim(const short ydim)
{
  m_pData->header.ydim = ydim;
}

void CPhilipsSubHeaderImage::setSlcnum(const short slcnum)
{
  m_pData->header.slcnum = slcnum;
}

void CPhilipsSubHeaderImage::setTiltnum(const short tiltnum)
{
  m_pData->header.tiltnum = tiltnum;
}

void CPhilipsSubHeaderImage::setGatint(const short gatint)
{
  m_pData->header.gatint = gatint;
}

void CPhilipsSubHeaderImage::setCntloss_corr(const short cntloss_corr)
{
  m_pData->header.cntloss_corr = cntloss_corr;
}

void CPhilipsSubHeaderImage::setPix_spacing(const short i, const float pix_spacing)
{
  if((i >= 0) || (i <= 2))
    m_pData->header.pix_spacing[i] = pix_spacing;
}

void CPhilipsSubHeaderImage::setXray_current(const float xray_current)
{
  m_pData->header.xray_current = xray_current;
}

void CPhilipsSubHeaderImage::setSuvscl(const float suvscl)
{
  m_pData->header.suvscl = suvscl;
}

void CPhilipsSubHeaderImage::setKvp(const float kvp)
{
  m_pData->header.kvp = kvp;
}

void CPhilipsSubHeaderImage::setDslice_loc(const float Dslice_loc)
{
  m_pData->header.Dslice_loc = Dslice_loc;
}

void CPhilipsSubHeaderImage::setMagfac(const float magfac)
{
  m_pData->header.magfac = magfac;
}

void CPhilipsSubHeaderImage::setImgscl(const float imgscl)
{
  m_pData->header.imgscl = imgscl;
}

void CPhilipsSubHeaderImage::setImgmin(const short imgmin)
{
  m_pData->header.imgmin = imgmin;
}

void CPhilipsSubHeaderImage::setImgmax(const short imgmax)
{
  m_pData->header.imgmax = imgmax;
}

void CPhilipsSubHeaderImage::setDecay_corr(const CPhilipsSubHeaderImage::Decay_Type decay_corr)
{
  m_pData->header.decay_corr = static_cast<qint16>(decay_corr);
}

void CPhilipsSubHeaderImage::setStrhr(const short strhr)
{
  m_pData->header.strhr = strhr;
}

void CPhilipsSubHeaderImage::setStrmin(const short strmin)
{
  m_pData->header.strmin = strmin;
}

void CPhilipsSubHeaderImage::setStrsec(const short strsec)
{
  m_pData->header.strsec = strsec;
}

void CPhilipsSubHeaderImage::setEndhr(const short endhr)
{
  m_pData->header.endhr = endhr;
}

void CPhilipsSubHeaderImage::setEndmin(const short endmin)
{
  m_pData->header.endmin = endmin;
}

void CPhilipsSubHeaderImage::setEndsec(const short endsec)
{
  m_pData->header.endsec = endsec;
}

void CPhilipsSubHeaderImage::setMidtim(const long midtim)
{
  m_pData->header.midtim = midtim;
}

void CPhilipsSubHeaderImage::setMseclen(const short mseclen)
{
  m_pData->header.mseclen = mseclen;
}

void CPhilipsSubHeaderImage::setScnlen(const short scnlen)
{
  m_pData->header.scnlen = scnlen;
}

void CPhilipsSubHeaderImage::setImgsum(const float imgsum)
{
  m_pData->header.imgsum = imgsum;
}

void CPhilipsSubHeaderImage::setBgdelrt(const float bgdelrt)
{
  m_pData->header.bgdelrt = bgdelrt;
}

void CPhilipsSubHeaderImage::setEnddelrt(const float enddelrt)
{
  m_pData->header.enddelrt = enddelrt;
}

void CPhilipsSubHeaderImage::setBgsngrt(const float bgsngrt)
{
  m_pData->header.bgsngrt = bgsngrt;
}

void CPhilipsSubHeaderImage::setBgcoincrt(const float bgcoincrt)
{
  m_pData->header.bgcoincrt = bgcoincrt;
}

void CPhilipsSubHeaderImage::setEndsngrt(const float endsngrt)
{
  m_pData->header.endsngrt = endsngrt;
}

void CPhilipsSubHeaderImage::setEndcoincrt(const float endcoincrt)
{
  m_pData->header.endcoincrt = endcoincrt;
}

void CPhilipsSubHeaderImage::setDeadtimefac(const float deadtimefac)
{
  m_pData->header.deadtimefac = deadtimefac;
}

void CPhilipsSubHeaderImage::setBedpos(const short bedpos)
{
  m_pData->header.bedpos = bedpos;
}

void CPhilipsSubHeaderImage::setDeadtime_bgsub(const float deadtime_bgsub)
{
  m_pData->header.deadtime_bgsub = deadtime_bgsub;
}

void CPhilipsSubHeaderImage::setSop_uid(const char* str)
{
  strncpy(m_pData->header.sop_uid, str, 64);
}

void CPhilipsSubHeaderImage::setRecon_method(const char* str)
{
  strncpy(m_pData->header.recon_method, str, 16);
}

