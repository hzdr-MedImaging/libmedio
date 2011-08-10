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
    float scnscl;        /* Sinogram scale factor */
    qint16 strhr;     /* (Deprecated - use start_date_time) Starting time of this frame: hour. */
    qint16 strmin;    /* (Deprecated - use start_date_time) Starting time of this frame: minute. */
    qint16 strsec;    /* (Deprecated - use start_date_time) Starting time of this frame: second. */

    qint32 start_date_time; /* UTC starting date/time of this frame. Use access functions to read/write.*/
    qint16 start_date_time_msec; /* Fraction of a second (in msec) to be added to start_date_time ... */
                                /*  to give a more accurate start time. Use access functions to read/write.*/

    qint16 scnmin;          /* Minimum value in sinogram; scaled to
                          * short by wrshdr() */
    qint16 scnmax;          /* Maximum value in sinogram; scaled to
                          * short by wrshdr() */
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
    float scnsum;        /* Total number of events in sinogram. */

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
};

CPhilipsSubHeaderImage::CPhilipsSubHeaderImage(CPhilipsFile* ecatFile,
                                               CPhilipsDirectoryItem* pDirItem)
  : CPhilipsSubHeader(ecatFile, pDirItem)
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
  
  // setData_Type(CECATSubHeader::UnknownDataType);
  // setFilter_Code(NoFilter);
  // setScatter_Type(None);
  // setRecon_Type(FilteredBackProjection);

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
  stream >> m_pData->header.scnscl;                     // 162: scnscl
  stream >> m_pData->header.strhr;                      // 166: strhr
  stream >> m_pData->header.strmin;                     // 168: strmin
  stream >> m_pData->header.strsec;                     // 170: strsec
  stream >> m_pData->header.scnmin;                     // 172: scnmin
  stream >> m_pData->header.scnmax;                     // 174: scnmax
  stream >> m_pData->header.endhr;                      // 176: endhr
  stream >> m_pData->header.endmin;                     // 178: endmin
  stream >> m_pData->header.endsec;                     // 180: endsec
  stream >> m_pData->header.midtim;                     // 182: midtim
  stream.skipRawData(2);                                // 186: skip 2 bytes
  stream >> m_pData->header.mseclen;                    // 188: mseclen
  stream >> m_pData->header.scnlen;                     // 190: scnlen
  stream >> m_pData->header.imgsum;                     // 192: imgsum
  stream >> m_pData->header.scnsum;                     // 196: scnsum
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
  D("ECAT7 Image SubHeader loaded:");
  D("----------------------------");
  D("version                 : %d",        m_pData->header.version);
  D("atten_corr : %s", m_pData->header.atten_corr);
  D("actual_bedpos : %f", m_pData->header.actual_bedpos);
  D("orientation[0] : %f", m_pData->header.orientation[0]);
  D("orientation[1] : %f", m_pData->header.orientation[1]);
  D("orientation[2] : %f", m_pData->header.orientation[2]);
  D("orientation[3] : %f", m_pData->header.orientation[3]);
  D("orientation[4] : %f", m_pData->header.orientation[4]);
  D("orientation[5] : %f", m_pData->header.orientation[5]);
  D("card_fr_time : %d", m_pData->header.card_fr_time);
  D("card_high_rr : %d", m_pData->header.card_high_rr);
  D("card_low_rr : %d", m_pData->header.card_low_rr);
  D("card_high_rr : %d", m_pData->header.card_tr_time);
  D("scatter_corr : %s", m_pData->header.scatter_corr);
  D("deadtime_corr : %d", m_pData->header.deadtime_corr);
  D("randoms_corr : %d", m_pData->header.randoms_corr);
  D("det_norm : %d", m_pData->header.det_norm);
  D("nu_radsamp_corr : %d", m_pData->header.nu_radsamp_corr);
  D("pat_mot_corr : %d", m_pData->header.pat_mot_corr);
  D("echo_time : %f", m_pData->header.echo_time);
  D("exposure_time : %f", m_pData->header.exposure_time);
  D("img_pos[0] : %f", m_pData->header.img_pos[0]);
  D("img_pos[1] : %f", m_pData->header.img_pos[1]);
  D("img_pos[2] : %f", m_pData->header.img_pos[2]);
  D("datype : %d", m_pData->header.datype);


  D("xdim : %d", m_pData->header.xdim);
  D("ydim : %d", m_pData->header.ydim);
  D("slcnum : %d", m_pData->header.slcnum);
  D("tiltnum : %d", m_pData->header.tiltnum);
  D("gatint : %d", m_pData->header.gatint);
  D("cntloss_corr : %d", m_pData->header.cntloss_corr);
  D("pix_spacing[0] : %f", m_pData->header.pix_spacing[0]);
  D("pix_spacing[1] : %f", m_pData->header.pix_spacing[1]);
  D("xray_current : %f", m_pData->header.xray_current);
  D("suvscl : %f", m_pData->header.suvscl);
  D("kvp : %f", m_pData->header.kvp);
  D("Dslice_loc : %f", m_pData->header.Dslice_loc);
  D("magfac : %f", m_pData->header.magfac); // Not used. always set to 1.0
  D("imgscl : %f", m_pData->header.imgscl);
  D("imgmin : %d", m_pData->header.imgmin);
  D("imgmax : %d", m_pData->header.imgmax);
  D("decay_corr : %d", m_pData->header.decay_corr);
  D("scnscl : %f", m_pData->header.scnscl); // Sinogram only
  D("strhr : %d", m_pData->header.strhr);
  D("strmin : %d", m_pData->header.strmin);
  D("strsec : %d", m_pData->header.strsec);
  D("scnmin : %d", m_pData->header.scnmin); // Sinogram only
  D("scnmax : %d", m_pData->header.scnmax); // Sinogram only
  D("endhr : %d", m_pData->header.endhr);
  D("endmin : %d", m_pData->header.endmin);
  D("endsec : %d", m_pData->header.endsec);
  D("midtim : %d", m_pData->header.midtim);
  D("mseclen : %d", m_pData->header.mseclen);
  D("scnlen : %d sec", m_pData->header.scnlen);
  D("imgsum : %f", m_pData->header.imgsum);
  D("scnsum : %f", m_pData->header.scnsum);
  D("bgdelrt : %f", m_pData->header.bgdelrt);
  D("enddelrt : %f", m_pData->header.enddelrt);
  D("bgsngrt : %f", m_pData->header.bgsngrt);
  D("bgcoincrt : %f", m_pData->header.bgcoincrt);
  D("endsngrt : %f", m_pData->header.endsngrt);
  D("endcoincrt : %f", m_pData->header.endcoincrt);
  D("deadtimefac : %f", m_pData->header.deadtimefac);
  D("bedpos : %d", m_pData->header.bedpos);
  D("deadtime_bgsub : %f", m_pData->header.deadtime_bgsub);
  D("sop_uid: %s", m_pData->header.sop_uid);
  D("recon_method: %s", m_pData->header.recon_method);
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
