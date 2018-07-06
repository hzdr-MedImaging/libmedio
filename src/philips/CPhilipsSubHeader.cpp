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

#include "CPhilipsSubHeader.h"
#include "CPhilipsDirectoryItem.h"
#include "CECATSubHeader.h"
#include "CECAT7MainHeader.h"
#include "CECAT7SubHeaderImage.h"
#include "CMedIOData.h"

#include <QDateTime>

#include <rtdebug.h>

#include "bswap.h"

class CPhilipsSubHeaderPrivate
{
  public:

  #pragma pack(1)
  struct HeaderData
  {
    char magic_number[14];            // 000: 14 chars identifying a subheader (0x010000010016FFFF0019003E003F)
    qint16 version;                   // 014: 0 -> original version (w/ reconinfo), 1 -> no reconinfo; w/ DICOM recon data
    char atten_corr[16];              // 016: Description of the attenuation correction used, “NONE” if none used
                                      //      Note: Strings to be used are defined in attnstr but currently not used by other code. This needs to
                                      //      be cleared up
    float actual_bedpos;              // 032: Absolute horizontal table position
    float orientation1[3];            // 036: Orientation of each slice/image; 1,0,0,0,1,0 for standard head-first, supine recon orientation.
                                      //      Reslicing applications will either have to set these values to all 0 for the DICOM exporter to claim an
                                      //      unknown orientation or set the values to the correct orientation.
                                      //      Specifies the direction cosines of the first row and the first column of the image with respect to the
                                      //      patient, using the DICOM patient relative coordinate system. Row values for x, y, and z axes
                                      //      respectively, followed by the column value for the x, y, and z axes respectively.
    char magic2[4];                   // 048: magic number 2: 0x0025FFFF
    float orientation2[3];            // 052: see orientation1
    qint32 card_fr_time;              // 064: Gated cardiac: Individual frame time in msec; nominal if framing type is by percentage or actual
                                      //      duration if framing type is forward or backward.
    qint32 card_high_rr;              // 068: Gated cardiac: R-R interval upper limit for beat rejection in msec
    qint32 card_low_rr;               // 072: Gated cardiac: R-R interval lower limit for beat rejection in msec
    qint32 card_tr_time;              // 076: Gated cardiac: Msec from the start of the trigger to the beginning of acquisition for this image.
                                      //      card_tr_time = gateInterval_ - beatPeriod + (myCardGateIndex_ -1) * (beatPeriod_/nCardGateFrames);
    char scatter_corr[16];            // 080: Description of scatter correction used; “NONE” if none used
                                      //      Note: Strings to be used are defined in scatter_str but currently not used by other code. This needs
                                      //      to be cleared up
    qint16 deadtime_corr;             // 096: Whether dead time correction was applied (0=No, 1=Yes)
    qint16 randoms_corr;              // 098: Type of randoms correction applied (NONE, DLYD, SING)
    qint16 det_norm;                  // 100: Indicates whether or not detector normalization was applied
    qint16 nu_radsamp_corr;           // 102: Indicates whether or not non-uniform radial sampling correction was applied
    qint16 pat_mot_corr;              // 104: Indicates whether or not patient motion correction was applied
    float echo_time;                  // 106: DICOM echo time for imported MR images; 0 for PET images.
    float exposure_time;              // 110: Length of exposure; 0 for PET images.
    float img_pos_x;                  // 114: DICOM image position (x,y,z) in mm; for PET images: x & y = -fov/2 + pixelSize/2, and z increases
                                      //      toward the head
                                      //      For NAC images the z positions start at zero.
                                      //      If the NAC is created as part of a CTAC and has the same Frame of Reference UID, then the z
                                      //      positions must still match the CTAC image.
    float img_pos_y;                  // 118: see img_pos_x
    char magic3[4];                   // 122: magic number 3: 0x0011FFFF
    qint16 datype;                    // 126: Type used in data storage
                                      //      1 (BYTE_IMG) Bytes
                                      //      2 (SHRT_IMG) Signed shorts (2 bytes)
                                      //      3 (USHRT_IMG) Unsigned shorts (2 bytes)
                                      //      4 (FLOT_IMG) Floats (4 bytes)
                                      //      Value 4 is currently not supported.
    float img_pos_z;                  // 128: see img_pos_x
    qint16 xdim;                      // 132: Sagittal (X) dimension. For images, transaxial diameter divided
                                      //      by the reconstruction voxel size. I.e. Number of voxels in x
                                      //      direction.
                                      //      For sinograms, number of rays.
    qint16 ydim;                      // 134: Coronal (Y) dimension. For images, transaxial diameter
                                      //      divided by the reconstruction voxel size. I.e. Number of voxels
                                      //      in y direction.
                                      //      For sinograms, number of angles (phi).
    qint16 slcnum;                    // 136: Slice number. For PET images, it is the z-position in mm.
                                      //      For translated CT images it is a slice number, 1 through n.
                                      //      What for Sinogram can be from zero to the (number of z-crystals**2 – 1)
                                      //      Note: Cannot be negative.
    qint16 tiltnum;                   // 138: Tilt number (0 for images and List file). Filled in for sinograms.
    qint16 gatint;                    // 140: Gating interval used in gated cardiac acquisition (in milliseconds).
                                      //      The width of each of the bins into which the cardiac cycle is divided into.
                                      //      Usually 8 or 16 evenly spaced bins.
    qint16 cntloss_corr;              // 142: Indicates whether or not count loss correction was applied (correction for count loss in gated time slots)
    float pix_spacing_x;              // 144: DICOM pixel spacing; for PETView images, dmax / numang. Center-to-center distance, in mm,
                                      //      between adjacent pixels, in the x (row) and y (column) dimensions.
    float pix_spacing_y;              // 148: see pix_spacing_x
    float xray_current;               // 152: DICOM X-ray tube current, in mA; 0 for PET images.
    float suvscl;                     // 156: Image only: Scale factor to be applied to stored values to obtain SUVs
    float kvp;                        // 160: DICOM KVP from imported CT images; 0 for PET images.
    float Dslice_loc;                 // 164: Slice location, in mm. This is the midpoint of the slice in the CT coordinate system.
    float magfac;                     // 168: Not used; always set to 1.0 in Imagio (wrimg function).
    float imgscl;                     // 172: Image only: Scale factor to be applied to stored values to obtain data values
    qint16 imgmin;                    // 176: Image only: Smallest data value
    qint16 imgmax;                    // 178: Image only: Greatest data value
    qint16 decay_corr;                // 180: Type of decay correction applied: NONE, START, ADMIN
    float scnscl;                     // 182: Sinogram only: Scale factor to be applied to stored values to obtain data values. (Scales to use
                                      //      maximum precision of a short)
    qint16 dep_strhr;                 // 186: (Deprecated) Hour in which the acquisition for the corresponding time slot or table position began
    qint16 dep_strmin;                // 188: (Deprecated) Minute in which the acquisition for the corresponding time slot or table position began
    qint16 dep_strsec;                // 190: (Deprecated) Second in which the acquisition for the corresponding time slot or table position began
    qint16 scnmin;                    // 192: Sinogram only: Smallest data value
    qint16 scnmax;                    // 194: Sinogram only: Greatest data value
    qint16 dep_endhr;                 // 196: (Deprecated) Hour in which the acquisition for the corresponding time slot or table position finished
    qint16 dep_endmin;                // 198: (Deprecated) Minute in which the acquisition for the corresponding time slot or table position finished
    qint16 dep_endsec;                // 200: (Deprecated) Second in which the acquisition for the corresponding time slot or table position finished
    qint32 midtim;                    // 202: Time from start of scan to frame midtime of frame. Not used.
    char empty1[2];                   // 206:
    qint16 mseclen;                   // 208: The remainder in milliseconds of the acquisition time for the time slot or table position. Note: This
                                      //      is not set for emission sinograms and therefore not available for emission based images.
    qint16 scnlen;                    // 210: The acquisition time for the time slot or table position
    float imgsum;                     // 212: Image only: Total number of events (sum of data)
    float scnsum;                     // 216: Sinogram only: total number of events (sum of sinogram)
    float bgdelrt;                    // 220: Delayed event rate at the beginning of the acquisition for the time slot or table position
    float enddelrt;                   // 224: Delayed event rate at the end of the acquisition for the time slot or table position
    float bgsngrt;                    // 228: Singles rate for all detectors at the beginning of the acquisition for the time slot or table position
    float bgcoincrt;                  // 232: Prompt event rate at the beginning of the acquisition for the time slot or table position
    float endsngrt;                   // 236: Singles rate for all detectors at the end of the acquisition for the time slot or table position
    float endcoincrt;                 // 240: Prompt event rate at the end of the acquisition for the time slot or table position
    float deadtimefac;                // 244: Dead-time correction factor for the slice's singles rate
    qint16 bedpos;                    // 248: Starting bed position (rounded) of a scan. Used only for whole body scanning; otherwise set to 0.
    float deadtime_bgsub;             // 250: Dead-time correction factor (accounting for background subtraction) for the slice's singles rate.
                                      //      Currently not used.
    char empty2[2];                   // 254:
    char sop_uid[64];                 // 256: DICOM SOP instance UID of this image (slice).
                                      //      Currently set internally by ImagIO in function pethdrSbhdrToBuffer().
    char recon_method[16];            // 320: Description of reconstruction method applied
    qint32 start_date_time;           // 336: Date and time at which the acquisition for the corresponding time slot or table position began
                                      //      (in seconds since Jan 1, 1970 UTC). Access functions setStartDateTime() and getStartDateTime() must be used.
    qint32 end_date_time;             // 340: Date and time at which the acquisition for the corresponding time slot or table position finished 
                                      //      (in seconds since Jan 1, 1970 UTC). Access functions setEndDateTime() and getEndDateTime() must be used.
    qint16 laterality;                // 344: Description of the laterality of (possibly paired) body parts imaged:
                                      //      0 Unknown (Note, not allowed for Enhanced DICOM)
                                      //      1 Right
                                      //      2 Left
                                      //      3 Unpaired
                                      //      4 Both left and right
    qint16 anatomy;                   // 346: Identifies the anatomic region of interest in this image (i.e. external anatomy, surface anatomy, or general region of the body):
                                      //      0 Unknown (Note, not allowed for Enhanced DICOM)
                                      //      1 Abdomen
                                      //      2 Abdomen and Pelvis
                                      //      3 Chest
                                      //      4 Chest and Abdomen
                                      //      5 Chest, Abdomen and Pelvis
                                      //      6 Entire body
                                      //      7 Head
                                      //      8 Head and Neck
                                      //      9 Heart
                                      //      10 Neck
                                      //      11 Neck and Chest
                                      //      12 Neck, Chest and Abdomen
                                      //      13 Neck, Chest, Abdomen and Pelvis
                                      //      14 Pelvis
                                      //      15 Leg
                                      //      16 Pelvis and lower extremities
    qint32 frame_ref_date_time;       // 348: The point in time that is most representative of when data was acquired for this frame.
                                      //      Corresponds to DICOM (0018,9151).
    qint32 card_rr_time;              // 352: Gated cardiac: Nominal R-R peak interval, in msec, from the cardiac cycles used for the acquisition
                                      //      of this frame.
                                      //      card_rr_time = gateIntervalAcceptedSum_/beatsAccepted
    qint32 resp_int_time;             // 356: Respiratory gated: Average measured interval time in msec from one respiratory trigger to the next
                                      //      trigger for the respiratory cycles in which this image occurs.
    qint16 start_date_time_msec;      // 360: start_date_time can only specify a time down to the second. This field, start_date_time_msec, can
                                      //      be used to specify an additional fraction of a second, in msec, to more accurately represent the time
                                      //      at which the corresponding time slot or table position began.
                                      //      Note: this is referred to as start_msec in the memory map diagram above
                                      //      Access functions setStartDateTime() and getStartDateTime() must be used.
    qint16 end_date_time_msec;        // 362: end_date_time can only specify a time down to the second. This field, end_date_time_msec, can be
                                      //      used to specify an additional fraction of a second, in msec, to more accurately represent the time at
                                      //      which the corresponding time slot or table position finished.
                                      //      Note: this is referred to as end_msec in the memory map diagram above.
                                      //      Access functions setEndDateTime() and getEndDateTime() must be used.
    qint16 frame_ref_date_time_msec;  // 364: fframe_ref_date_time can only specify a time down to the second. This field,
                                      //      frame_ref_date_time_msec, can be used to specify an additional fraction of a second, in msec, to
                                      //      more accurately represent the time that is most representative of when data was acquired for this
                                      //      frame. Filled in by recon.
                                      //      Note: this is referred to as end_msec in the memory map diagram above.
    char empty3[2];                   // 366:
    double prompt_events_sum;         // 368: Actual number of prompt events for this image frame. In a final image, events are summed across all
                                      //      bed positions used to create the final image.
    double delay_events_sum;          // 376: Actual number of delay events for this image frame. In a final image, events are summed across all
                                      //      bed positions used to create the final image.
    float scatter_fraction_average;   // 384: Scatter Fraction for this image frame. In a final image, the scatter fraction is an average across all bed
                                      //      positions.
  } header;
  #pragma pack()
};


CPhilipsSubHeader::CPhilipsSubHeader(CMedIOData* philipsFile,
                                     CPhilipsDirectoryItem* dItem)
  : CMedIOHeader(philipsFile),
    m_pDirItem(dItem)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CPhilipsSubHeaderPrivate();

  clear();

  LEAVE();
}

CPhilipsSubHeader::~CPhilipsSubHeader()
{
  ENTER();

  delete m_pData;

  LEAVE();
}

CPhilipsSubHeader::CPhilipsSubHeader(const CPhilipsSubHeader& src)
  : CMedIOHeader(src)
{
  ENTER();

  // allocate data from our private instance class
  m_pData = new CPhilipsSubHeaderPrivate(*(src.m_pData));

  LEAVE();
}

CPhilipsSubHeader& CPhilipsSubHeader::operator=(const CPhilipsSubHeader& src)
{
  ENTER();

  if(m_pData != src.m_pData)
  {
    memcpy(&m_pData->header,
           &src.m_pData->header,
           sizeof(struct CPhilipsSubHeaderPrivate::HeaderData));
  }
  
  LEAVE();
  return *this;
}

CMedIOHeader::Format CPhilipsSubHeader::headerFormat() const
{ 
  return CMedIOHeader::PhilipsSubHeader;
}

void CPhilipsSubHeader::setDirectoryItem(CPhilipsDirectoryItem* dItem)
{ 
  m_pDirItem = dItem;
}

void CPhilipsSubHeader::clear()
{
  ENTER();

  // clear our header structure first
  memset(&m_pData->header, 0, sizeof(struct CPhilipsSubHeaderPrivate::HeaderData));

  // set the fill bytes
  m_pData->header.magic_number[0]  = 0x01;
  m_pData->header.magic_number[1]  = 0x00;
  m_pData->header.magic_number[2]  = 0x00;
  m_pData->header.magic_number[3]  = 0x01;
  m_pData->header.magic_number[4]  = 0x00;
  m_pData->header.magic_number[5]  = 0x16;
  m_pData->header.magic_number[6]  = 0xFF;
  m_pData->header.magic_number[7]  = 0xFF;
  m_pData->header.magic_number[8]  = 0x00;
  m_pData->header.magic_number[9]  = 0x19;
  m_pData->header.magic_number[10] = 0x00;
  m_pData->header.magic_number[11] = 0x3E;
  m_pData->header.magic_number[12] = 0x00;
  m_pData->header.magic_number[13] = 0x4F;

  // magic number 2: 0x0025FFFF
  m_pData->header.magic2[0] = 0x00;
  m_pData->header.magic2[1] = 0x25;
  m_pData->header.magic2[2] = 0xFF;
  m_pData->header.magic2[3] = 0xFF;

  // magic number 3: 0x0011FFFF
  m_pData->header.magic3[0] = 0x00;
  m_pData->header.magic3[1] = 0x11;
  m_pData->header.magic3[2] = 0xFF;
  m_pData->header.magic3[3] = 0xFF;

  // set some default values
  m_pData->header.version = 1; // the current version
  m_pData->header.magfac = 1.0; // Not used. always set to 1.0

  LEAVE();
}

bool CPhilipsSubHeader::load(void)
{
  ENTER();
  CMedIOData* mData = medIOData();

  // check if the stream is readable or not and
  // set our MedIOData to the correct file position so that we can
  // read the subheader  
  if(mData == NULL ||
     mData->isReadable() == false ||
     m_pDirItem->dataBlock_Start() == 0 ||
     mData->seek(m_pDirItem->dataBlock_Start()) == false)
  {
    RETURN(false);
    return false;
  }

  D("about to read sub header");
  SHOWVALUE(mData->pos());

  // we read in all data at once using read()
  if(mData->read(reinterpret_cast<char*>(&m_pData->header), sizeof(m_pData->header)) != sizeof(m_pData->header))
  {
    RETURN(false);
    return false;
  }

  // now that we have streamed in all data in one run we
  // have to take care of correct endianness in the non-char
  // entries in the header structure in case this is a little endian
  // machine
  if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
  {
    // we only swap non-char elements of the header
    BSWAP_16(m_pData->header.version);
    BSWAP_FLT(m_pData->header.actual_bedpos);
    BSWAP_FLT(m_pData->header.orientation1[0]);
    BSWAP_FLT(m_pData->header.orientation1[1]);
    BSWAP_FLT(m_pData->header.orientation1[2]);
    BSWAP_FLT(m_pData->header.orientation2[0]);
    BSWAP_FLT(m_pData->header.orientation2[1]);
    BSWAP_FLT(m_pData->header.orientation2[2]);
    BSWAP_32(m_pData->header.card_fr_time);
    BSWAP_32(m_pData->header.card_high_rr);
    BSWAP_32(m_pData->header.card_low_rr);
    BSWAP_32(m_pData->header.card_tr_time);
    BSWAP_16(m_pData->header.deadtime_corr);
    BSWAP_16(m_pData->header.randoms_corr);
    BSWAP_16(m_pData->header.det_norm);
    BSWAP_16(m_pData->header.nu_radsamp_corr);
    BSWAP_16(m_pData->header.pat_mot_corr);
    BSWAP_FLT(m_pData->header.echo_time);
    BSWAP_FLT(m_pData->header.exposure_time);
    BSWAP_FLT(m_pData->header.img_pos_x);
    BSWAP_FLT(m_pData->header.img_pos_y);
    BSWAP_16(m_pData->header.datype);
    BSWAP_FLT(m_pData->header.img_pos_z);
    BSWAP_16(m_pData->header.xdim);
    BSWAP_16(m_pData->header.ydim);
    BSWAP_16(m_pData->header.slcnum);
    BSWAP_16(m_pData->header.tiltnum);
    BSWAP_16(m_pData->header.gatint);
    BSWAP_16(m_pData->header.cntloss_corr);
    BSWAP_FLT(m_pData->header.pix_spacing_x);
    BSWAP_FLT(m_pData->header.pix_spacing_y);
    BSWAP_FLT(m_pData->header.xray_current);
    BSWAP_FLT(m_pData->header.suvscl);
    BSWAP_FLT(m_pData->header.kvp);
    BSWAP_FLT(m_pData->header.Dslice_loc);
    BSWAP_FLT(m_pData->header.magfac);
    BSWAP_FLT(m_pData->header.imgscl);
    BSWAP_16(m_pData->header.imgmin);
    BSWAP_16(m_pData->header.imgmax);
    BSWAP_16(m_pData->header.decay_corr);
    BSWAP_FLT(m_pData->header.scnscl);
    BSWAP_16(m_pData->header.dep_strhr);
    BSWAP_16(m_pData->header.dep_strmin);
    BSWAP_16(m_pData->header.dep_strsec);
    BSWAP_16(m_pData->header.scnmin);
    BSWAP_16(m_pData->header.scnmax);
    BSWAP_16(m_pData->header.dep_endhr);
    BSWAP_16(m_pData->header.dep_endmin);
    BSWAP_16(m_pData->header.dep_endsec);
    BSWAP_32(m_pData->header.midtim);
    BSWAP_16(m_pData->header.mseclen);
    BSWAP_16(m_pData->header.scnlen);
    BSWAP_FLT(m_pData->header.imgsum);
    BSWAP_FLT(m_pData->header.scnsum);
    BSWAP_FLT(m_pData->header.bgdelrt);
    BSWAP_FLT(m_pData->header.enddelrt);
    BSWAP_FLT(m_pData->header.bgsngrt);
    BSWAP_FLT(m_pData->header.bgcoincrt);
    BSWAP_FLT(m_pData->header.endsngrt);
    BSWAP_FLT(m_pData->header.endcoincrt);
    BSWAP_FLT(m_pData->header.deadtimefac);
    BSWAP_16(m_pData->header.bedpos);
    BSWAP_FLT(m_pData->header.deadtime_bgsub);
    BSWAP_32(m_pData->header.start_date_time);
    BSWAP_32(m_pData->header.end_date_time);
    BSWAP_16(m_pData->header.laterality);
    BSWAP_16(m_pData->header.anatomy);
    BSWAP_32(m_pData->header.frame_ref_date_time);
    BSWAP_32(m_pData->header.card_rr_time);
    BSWAP_32(m_pData->header.resp_int_time);
    BSWAP_16(m_pData->header.start_date_time_msec);
    BSWAP_16(m_pData->header.end_date_time_msec);
    BSWAP_16(m_pData->header.frame_ref_date_time_msec);
    BSWAP_DBL(m_pData->header.prompt_events_sum);
    BSWAP_DBL(m_pData->header.delay_events_sum);
    BSWAP_FLT(m_pData->header.scatter_fraction_average);
  }

 // some more debug output
#if defined(DEBUG)
  D("Philips Image SubHeader loaded:");
  D("----------------------------");
  D("magic_number            : %02x%02x", (quint32)m_pData->header.magic_number[0], (quint32)m_pData->header.magic_number[0]);
  D("version                 : %d", m_pData->header.version);
  D("atten_corr              : %s", m_pData->header.atten_corr);
  D("actual_bedpos           : %f", m_pData->header.actual_bedpos);
  D("orientation[0]          : %f", m_pData->header.orientation1[0]);
  D("orientation[1]          : %f", m_pData->header.orientation1[1]);
  D("orientation[2]          : %f", m_pData->header.orientation1[2]);
  D("orientation[3]          : %f", m_pData->header.orientation2[0]);
  D("orientation[4]          : %f", m_pData->header.orientation2[1]);
  D("orientation[5]          : %f", m_pData->header.orientation2[2]);
  D("card_fr_time            : %d", m_pData->header.card_fr_time);
  D("card_high_rr            : %d", m_pData->header.card_high_rr);
  D("card_low_rr             : %d", m_pData->header.card_low_rr);
  D("card_high_rr            : %d", m_pData->header.card_tr_time);
  D("scatter_corr            : %s", m_pData->header.scatter_corr);
  D("deadtime_corr           : %d", m_pData->header.deadtime_corr);
  D("randoms_corr            : %d", m_pData->header.randoms_corr);
  D("det_norm                : %d", m_pData->header.det_norm);
  D("nu_radsamp_corr         : %d", m_pData->header.nu_radsamp_corr);
  D("pat_mot_corr            : %d", m_pData->header.pat_mot_corr);
  D("echo_time               : %f", m_pData->header.echo_time);
  D("exposure_time           : %f", m_pData->header.exposure_time);
  D("img_pos_x               : %f", m_pData->header.img_pos_x);
  D("img_pos_y               : %f", m_pData->header.img_pos_y);
  D("img_pos_z               : %f", m_pData->header.img_pos_z);
  D("datype                  : %d", m_pData->header.datype);
  D("xdim                    : %d", m_pData->header.xdim);
  D("ydim                    : %d", m_pData->header.ydim);
  D("slcnum                  : %d", m_pData->header.slcnum);
  D("tiltnum                 : %d", m_pData->header.tiltnum);
  D("gatint                  : %d", m_pData->header.gatint);
  D("cntloss_corr            : %d", m_pData->header.cntloss_corr);
  D("pix_spacing_x           : %f", m_pData->header.pix_spacing_x);
  D("pix_spacing_y           : %f", m_pData->header.pix_spacing_y);
  D("xray_current            : %f", m_pData->header.xray_current);
  D("suvscl                  : %f", m_pData->header.suvscl);
  D("kvp                     : %f", m_pData->header.kvp);
  D("Dslice_loc              : %f", m_pData->header.Dslice_loc);
  D("magfac                  : %f", m_pData->header.magfac);
  D("imgscl                  : %f", m_pData->header.imgscl);
  D("imgmin                  : %d", m_pData->header.imgmin);
  D("imgmax                  : %d", m_pData->header.imgmax);
  D("scnscl                  : %f", m_pData->header.scnscl);
  D("scnmin                  : %d", m_pData->header.scnmin);
  D("scnmax                  : %d", m_pData->header.scnmax);
  D("scnsum                  : %f", m_pData->header.scnsum);
  D("decay_corr              : %d", m_pData->header.decay_corr);
  D("dep_strhr               : %d", m_pData->header.dep_strhr);
  D("dep_strmin              : %d", m_pData->header.dep_strmin);
  D("dep_strsec              : %d", m_pData->header.dep_strsec);
  D("dep_endhr               : %d", m_pData->header.dep_endhr);
  D("dep_endmin              : %d", m_pData->header.dep_endmin);
  D("dep_endsec              : %d", m_pData->header.dep_endsec);
  D("midtim                  : %d", m_pData->header.midtim);
  D("mseclen                 : %d", m_pData->header.mseclen);
  D("scnlen                  : %d", m_pData->header.scnlen);
  D("imgsum                  : %f", m_pData->header.imgsum);
  D("bgdelrt                 : %f", m_pData->header.bgdelrt);
  D("enddelrt                : %f", m_pData->header.enddelrt);
  D("bgsngrt                 : %f", m_pData->header.bgsngrt);
  D("bgcoincrt               : %f", m_pData->header.bgcoincrt);
  D("endsngrt                : %f", m_pData->header.endsngrt);
  D("endcoincrt              : %f", m_pData->header.endcoincrt);
  D("deadtimefac             : %f", m_pData->header.deadtimefac);
  D("bedpos                  : %d", m_pData->header.bedpos);
  D("deadtime_bgsub          : %f", m_pData->header.deadtime_bgsub);
  D("sop_uid                 : %s", m_pData->header.sop_uid);
  D("recon_method            : %s", m_pData->header.recon_method);
  D("start_date_time         : %s", QDateTime::fromTime_t(m_pData->header.start_date_time).toString().toLatin1().constData());
  D("end_date_time           : %s", QDateTime::fromTime_t(m_pData->header.end_date_time).toString().toLatin1().constData());
  D("laterality              : %d", m_pData->header.laterality);
  D("anatomy                 : %d", m_pData->header.anatomy);
  D("frame_ref_date_time     : %s", QDateTime::fromTime_t(m_pData->header.frame_ref_date_time).toString().toLatin1().constData());
  D("card_rr_time            : %ld", m_pData->header.card_rr_time);
  D("resp_int_time           : %ld", m_pData->header.resp_int_time);
  D("start_date_time_msec    : %d", m_pData->header.start_date_time_msec);
  D("end_date_time_msec      : %d", m_pData->header.end_date_time_msec);
  D("frame_ref_date_time_msec: %d", m_pData->header.frame_ref_date_time_msec);
  D("prompt_events_sum       : %f", m_pData->header.prompt_events_sum);
  D("delay_events_sum        : %f", m_pData->header.delay_events_sum);
  D("scatter_fraction_average: %f", m_pData->header.scatter_fraction_average);
#endif

  RETURN(true);
  return true;
}

bool CPhilipsSubHeader::save(void) const
{
  ENTER();
  bool result = false;
  CMedIOData* mData = medIOData();

  // check if this stream is writeable or not
  if(mData == NULL || mData->isWritable() == false ||
     m_pDirItem == NULL || m_pDirItem->dataBlock_Start() == 0 ||
     mData->seek(m_pDirItem->dataBlock_Start()) == false)
  {
    RETURN(false);
    return false;
  }

  D("about to write sub header");

 // some more debug output
#if defined(DEBUG)
  D("Philips Image SubHeader:");
  D("-----------------------");
  D("magic_number            : %02x%02x", (quint32)m_pData->header.magic_number[0], (quint32)m_pData->header.magic_number[1]);
  D("version                 : %d", m_pData->header.version);
  D("atten_corr              : %s", m_pData->header.atten_corr);
  D("actual_bedpos           : %f", m_pData->header.actual_bedpos);
  D("orientation[0]          : %f", m_pData->header.orientation1[0]);
  D("orientation[1]          : %f", m_pData->header.orientation1[1]);
  D("orientation[2]          : %f", m_pData->header.orientation1[2]);
  D("orientation[3]          : %f", m_pData->header.orientation2[0]);
  D("orientation[4]          : %f", m_pData->header.orientation2[1]);
  D("orientation[5]          : %f", m_pData->header.orientation2[2]);
  D("card_fr_time            : %d", m_pData->header.card_fr_time);
  D("card_high_rr            : %d", m_pData->header.card_high_rr);
  D("card_low_rr             : %d", m_pData->header.card_low_rr);
  D("card_high_rr            : %d", m_pData->header.card_tr_time);
  D("scatter_corr            : %s", m_pData->header.scatter_corr);
  D("deadtime_corr           : %d", m_pData->header.deadtime_corr);
  D("randoms_corr            : %d", m_pData->header.randoms_corr);
  D("det_norm                : %d", m_pData->header.det_norm);
  D("nu_radsamp_corr         : %d", m_pData->header.nu_radsamp_corr);
  D("pat_mot_corr            : %d", m_pData->header.pat_mot_corr);
  D("echo_time               : %f", m_pData->header.echo_time);
  D("exposure_time           : %f", m_pData->header.exposure_time);
  D("img_pos_x               : %f", m_pData->header.img_pos_x);
  D("img_pos_y               : %f", m_pData->header.img_pos_y);
  D("img_pos_z               : %f", m_pData->header.img_pos_z);
  D("datype                  : %d", m_pData->header.datype);
  D("xdim                    : %d", m_pData->header.xdim);
  D("ydim                    : %d", m_pData->header.ydim);
  D("slcnum                  : %d", m_pData->header.slcnum);
  D("tiltnum                 : %d", m_pData->header.tiltnum);
  D("gatint                  : %d", m_pData->header.gatint);
  D("cntloss_corr            : %d", m_pData->header.cntloss_corr);
  D("pix_spacing_x           : %f", m_pData->header.pix_spacing_x);
  D("pix_spacing_y           : %f", m_pData->header.pix_spacing_y);
  D("xray_current            : %f", m_pData->header.xray_current);
  D("suvscl                  : %f", m_pData->header.suvscl);
  D("kvp                     : %f", m_pData->header.kvp);
  D("Dslice_loc              : %f", m_pData->header.Dslice_loc);
  D("magfac                  : %f", m_pData->header.magfac);
  D("imgscl                  : %f", m_pData->header.imgscl);
  D("imgmin                  : %d", m_pData->header.imgmin);
  D("imgmax                  : %d", m_pData->header.imgmax);
  D("scnscl                  : %f", m_pData->header.scnscl);
  D("scnmin                  : %d", m_pData->header.scnmin);
  D("scnmax                  : %d", m_pData->header.scnmax);
  D("scnsum                  : %f", m_pData->header.scnsum);
  D("decay_corr              : %d", m_pData->header.decay_corr);
  D("dep_strhr               : %d", m_pData->header.dep_strhr);
  D("dep_strmin              : %d", m_pData->header.dep_strmin);
  D("dep_strsec              : %d", m_pData->header.dep_strsec);
  D("dep_endhr               : %d", m_pData->header.dep_endhr);
  D("dep_endmin              : %d", m_pData->header.dep_endmin);
  D("dep_endsec              : %d", m_pData->header.dep_endsec);
  D("midtim                  : %d", m_pData->header.midtim);
  D("mseclen                 : %d", m_pData->header.mseclen);
  D("scnlen                  : %d", m_pData->header.scnlen);
  D("imgsum                  : %f", m_pData->header.imgsum);
  D("bgdelrt                 : %f", m_pData->header.bgdelrt);
  D("enddelrt                : %f", m_pData->header.enddelrt);
  D("bgsngrt                 : %f", m_pData->header.bgsngrt);
  D("bgcoincrt               : %f", m_pData->header.bgcoincrt);
  D("endsngrt                : %f", m_pData->header.endsngrt);
  D("endcoincrt              : %f", m_pData->header.endcoincrt);
  D("deadtimefac             : %f", m_pData->header.deadtimefac);
  D("bedpos                  : %d", m_pData->header.bedpos);
  D("deadtime_bgsub          : %f", m_pData->header.deadtime_bgsub);
  D("sop_uid                 : %s", m_pData->header.sop_uid);
  D("recon_method            : %s", m_pData->header.recon_method);
  D("start_date_time         : %s", QDateTime::fromTime_t(m_pData->header.start_date_time).toString().toLatin1().constData());
  D("end_date_time           : %s", QDateTime::fromTime_t(m_pData->header.end_date_time).toString().toLatin1().constData());
  D("laterality              : %d", m_pData->header.laterality);
  D("anatomy                 : %d", m_pData->header.anatomy);
  D("frame_ref_date_time     : %s", QDateTime::fromTime_t(m_pData->header.frame_ref_date_time).toString().toLatin1().constData());
  D("card_rr_time            : %ld", m_pData->header.card_rr_time);
  D("resp_int_time           : %ld", m_pData->header.resp_int_time);
  D("start_date_time_msec    : %d", m_pData->header.start_date_time_msec);
  D("end_date_time_msec      : %d", m_pData->header.end_date_time_msec);
  D("frame_ref_date_time_msec: %d", m_pData->header.frame_ref_date_time_msec);
  D("prompt_events_sum       : %f", m_pData->header.prompt_events_sum);
  D("delay_events_sum        : %f", m_pData->header.delay_events_sum);
  D("scatter_fraction_average: %f", m_pData->header.scatter_fraction_average);
#endif

  SHOWVALUE(mData->pos());

  // now that we have streamed in all data in one run we
  // have to take care of correct endianness in the non-char
  // entries in the header structure in case this is a little endian
  // machine
  if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
  {
    // if we need to byte swap we have to create a whole copy of m_pData->header
    struct CPhilipsSubHeaderPrivate::HeaderData beHeader;

    // copy the current m_pData->header to beHeader
    memcpy(&beHeader, &m_pData->header, sizeof(m_pData->header));

    // we only swap non-char elements of the header
    BSWAP_16(beHeader.version);
    BSWAP_FLT(beHeader.actual_bedpos);
    BSWAP_FLT(beHeader.orientation1[0]);
    BSWAP_FLT(beHeader.orientation1[1]);
    BSWAP_FLT(beHeader.orientation1[2]);
    BSWAP_FLT(beHeader.orientation2[0]);
    BSWAP_FLT(beHeader.orientation2[1]);
    BSWAP_FLT(beHeader.orientation2[2]);
    BSWAP_32(beHeader.card_fr_time);
    BSWAP_32(beHeader.card_high_rr);
    BSWAP_32(beHeader.card_low_rr);
    BSWAP_32(beHeader.card_tr_time);
    BSWAP_16(beHeader.deadtime_corr);
    BSWAP_16(beHeader.randoms_corr);
    BSWAP_16(beHeader.det_norm);
    BSWAP_16(beHeader.nu_radsamp_corr);
    BSWAP_16(beHeader.pat_mot_corr);
    BSWAP_FLT(beHeader.echo_time);
    BSWAP_FLT(beHeader.exposure_time);
    BSWAP_FLT(beHeader.img_pos_x);
    BSWAP_FLT(beHeader.img_pos_y);
    BSWAP_16(beHeader.datype);
    BSWAP_FLT(beHeader.img_pos_z);
    BSWAP_16(beHeader.xdim);
    BSWAP_16(beHeader.ydim);
    BSWAP_16(beHeader.slcnum);
    BSWAP_16(beHeader.tiltnum);
    BSWAP_16(beHeader.gatint);
    BSWAP_16(beHeader.cntloss_corr);
    BSWAP_FLT(beHeader.pix_spacing_x);
    BSWAP_FLT(beHeader.pix_spacing_y);
    BSWAP_FLT(beHeader.xray_current);
    BSWAP_FLT(beHeader.suvscl);
    BSWAP_FLT(beHeader.kvp);
    BSWAP_FLT(beHeader.Dslice_loc);
    BSWAP_FLT(beHeader.magfac);
    BSWAP_FLT(beHeader.imgscl);
    BSWAP_16(beHeader.imgmin);
    BSWAP_16(beHeader.imgmax);
    BSWAP_16(beHeader.decay_corr);
    BSWAP_FLT(beHeader.scnscl);
    BSWAP_16(beHeader.dep_strhr);
    BSWAP_16(beHeader.dep_strmin);
    BSWAP_16(beHeader.dep_strsec);
    BSWAP_16(beHeader.scnmin);
    BSWAP_16(beHeader.scnmax);
    BSWAP_16(beHeader.dep_endhr);
    BSWAP_16(beHeader.dep_endmin);
    BSWAP_16(beHeader.dep_endsec);
    BSWAP_32(beHeader.midtim);
    BSWAP_16(beHeader.mseclen);
    BSWAP_16(beHeader.scnlen);
    BSWAP_FLT(beHeader.imgsum);
    BSWAP_FLT(beHeader.scnsum);
    BSWAP_FLT(beHeader.bgdelrt);
    BSWAP_FLT(beHeader.enddelrt);
    BSWAP_FLT(beHeader.bgsngrt);
    BSWAP_FLT(beHeader.bgcoincrt);
    BSWAP_FLT(beHeader.endsngrt);
    BSWAP_FLT(beHeader.endcoincrt);
    BSWAP_FLT(beHeader.deadtimefac);
    BSWAP_16(beHeader.bedpos);
    BSWAP_FLT(beHeader.deadtime_bgsub);
    BSWAP_32(beHeader.start_date_time);
    BSWAP_32(beHeader.end_date_time);
    BSWAP_16(beHeader.laterality);
    BSWAP_16(beHeader.anatomy);
    BSWAP_32(beHeader.frame_ref_date_time);
    BSWAP_32(beHeader.card_rr_time);
    BSWAP_32(beHeader.resp_int_time);
    BSWAP_16(beHeader.start_date_time_msec);
    BSWAP_16(beHeader.end_date_time_msec);
    BSWAP_16(beHeader.frame_ref_date_time_msec);
    BSWAP_DBL(beHeader.prompt_events_sum);
    BSWAP_DBL(beHeader.delay_events_sum);
    BSWAP_FLT(beHeader.scatter_fraction_average);

    // now write out beHeader
    if(mData->write((char *)&beHeader, sizeof(beHeader)) == sizeof(beHeader))
      result = true;
  }
  else
  {
    // now write out to our outStream
    if(mData->write((char *)&m_pData->header, sizeof(m_pData->header)) == sizeof(m_pData->header))
      result = true;
  }

  // if we were successfull we need to output some more data until PHILIPS_BLOCKSIZE
  if(result == true)
  {
    if(sizeof(m_pData->header) != PHILIPS_BLOCKSIZE)
    {
      QByteArray nulArray(PHILIPS_BLOCKSIZE-sizeof(m_pData->header), 0);

      if(mData->write(nulArray) != nulArray.length())
      {
        result = false;
        E("Error while writing %d NUL data", nulArray.length());
      }
    }

    if(result == true)
      m_pDirItem->subHeaderWritten(*this);
  }

  RETURN(result);
  return result;
}

bool CPhilipsSubHeader::convertFrom(const CMedIOHeader* subHeader, const CMedIOHeader* mainHeader)
{
  ENTER();
  bool bResult = false;

  // depending on the MedIOHeader format we do have to 
  // distinguish between our copy operations.
  switch(subHeader->headerFormat())
  {
    case CMedIOHeader::ECATSubHeader:
    {
      const CECATSubHeader* eSubHeader = static_cast<const CECATSubHeader*>(subHeader);

      // depending on the source type we have to copy either every data or just 
      // some data of the src header
      switch(eSubHeader->subHeaderType())
      {
        // if the source header is an ECAT7 image subheader we convert as much as possible
        case CECATSubHeader::ECAT7_Image:
        {
          const CECAT7SubHeaderImage* sh = static_cast<const CECAT7SubHeaderImage*>(subHeader);
          const CECAT7MainHeader* mh = static_cast<const CECAT7MainHeader*>(mainHeader);

          // convert now
          CPhilipsSubHeader::Data_Type dtype = CPhilipsSubHeader::UnknownDataType;
          switch(sh->data_Type())
          {
            case CECATSubHeader::UnknownDataType:
              W("Unknown DataType found while converting");
              // do nothing
            break;

            case CECATSubHeader::ByteData:
              dtype = CPhilipsSubHeader::ByteData;
            break;

            case CECATSubHeader::VAX_Ix2:
            case CECATSubHeader::SunShort:
              dtype = CPhilipsSubHeader::SignedShort;
            break;

            case CECATSubHeader::VAX_Ix4:
            case CECATSubHeader::SunLong:
              W("SunLong found, user HAVE to care about converting the matrix data to Float himself");
              dtype = CPhilipsSubHeader::Float;
            break;

            case CECATSubHeader::VAX_Rx4:
            case CECATSubHeader::IEEEFloat:
              dtype = CPhilipsSubHeader::Float;
            break;
          }
          setDatype(dtype);

          // if the header is marked as being calibrated we go
          // and set the SUV scaling factor, otherwise we set it to 0 to signal
          // that the values are uncalibrated.
          bool calibrated = false;
          if(mh->calibration_Units() != CECAT7MainHeader::Uncalibrated)
          {
            // we have to calculate the suv scaling factor
            float fact = sh->suv_Scale_Factor(calibrated, mh);

            // if the conversion worked out fine we can assume the
            // suv scale factor to be correct. so we set the img scale
            // factor to 1.0f
            if(calibrated == true)
            {
              setSuvscl(fact);
              setImgscl(1.0f);
            }
          }

          // if we end up in having identified the file to be uncalibrated
          // we go and set the suvscl factor to 0.0f and the img scale factor
          // to the value of the main header of the ECAT file
          if(calibrated == false)
          {
            setSuvscl(0.0f);
            setImgscl(sh->scale_Factor());
          }

          setXdim(sh->x_Dimension());
          setYdim(sh->y_Dimension());
          setPix_spacing_x(sh->x_Pixel_Size() * 10.0f); // cm -> mm
          setPix_spacing_y(sh->y_Pixel_Size() * 10.0f); // cm -> mm
          setScnlen(sh->frame_Duration()/1000.0f); // ms -> s
          setMseclen(sh->frame_Duration() % 1000);

          // set the frame start_date_time and end_date_time
          setStart_date_time(mh->scan_Start_Time() + sh->frame_Start_Time() / 1000);
          setStart_date_time_msec(sh->frame_Start_Time() % 1000);
          setEnd_date_time(start_date_time() + scnlen());
          setEnd_date_time_msec(sh->frame_Start_Time() % 1000);
          setFrame_ref_date_time(mh->scan_Start_Time() + sh->frame_Start_Time() / 1000);

          if((sh->processing_Code() & CECAT7SubHeaderImage::DecayCorrection) == CECAT7SubHeaderImage::DecayCorrection)
            setDecay_corr(CPhilipsSubHeader::Acqstart);

          if((sh->processing_Code() & CECAT7SubHeaderImage::Normalized) == CECAT7SubHeaderImage::Normalized)
            setDet_norm(1);

          if((sh->processing_Code() & CECAT7SubHeaderImage::MeasuredAttenCorr) == CECAT7SubHeaderImage::MeasuredAttenCorr)
          {
            // 97 seems to be PET/MR
            if(mh->system_Type() == 97)
              setAtten_corr("MRAC");
            else
              setAtten_corr("AC");
          }

          setDeadtime_corr(1);
          setRandoms_corr(CPhilipsSubHeader::Delayed);
          setNu_radsamp_corr(1);
          setCntloss_corr(1);
          
          if(sh->scatter_Type() == CECAT7SubHeaderImage::Simulated)
            setScatter_corr("SS-SIMUL");

          if(QString(sh->annotation()).isEmpty() == false)
            setRecon_method(sh->annotation());
          else
          {
            if(sh->recon_Type() == CECAT7SubHeaderImage::FilteredBackProjection)
              setRecon_method("FBP");
            else if(sh->recon_Type() == CECAT7SubHeaderImage::BasicOsem)
              setRecon_method("OSEM");
            else if(sh->recon_Type() == CECAT7SubHeaderImage::IterativeCPURecon)
              setRecon_method("Iterative");
            else
              setRecon_method("Unknown");
          }

          setActual_bedpos(mh->init_Bed_Position() * 10.0f); // cm -> mm
          setBedpos(mh->init_Bed_Position() * 10.0f); // cm -> mm
          setDslice_loc(mh->init_Bed_Position() * 10.0f); // cm -> mm
          setImg_pos_z(mh->init_Bed_Position() * 10.0f); // cm -> mm

          bResult = true;
        }
        break;

        case CECATSubHeader::Unknown:
          // for an unknown header type we do nothing
        break;
        
        #warning "non Image copy not complete"
      }
    }
    break;
  }
 
  RETURN(bResult);
  return bResult;
}

// data acess methods
short CPhilipsSubHeader::version() const
{
  return m_pData->header.version;
}

const char* CPhilipsSubHeader::atten_corr() const
{
  return m_pData->header.atten_corr;
}

float CPhilipsSubHeader::actual_bedpos() const
{
  return m_pData->header.actual_bedpos;
}

float CPhilipsSubHeader::orientation(const short i) const
{
  if(i >= 0 && i <= 2)
    return m_pData->header.orientation1[i];
  else if(i >= 3 && i <= 5)
    return m_pData->header.orientation2[i-3];
  else
    return 0.0f;
}

int CPhilipsSubHeader::card_fr_time() const
{
  return m_pData->header.card_fr_time;
}

int CPhilipsSubHeader::card_high_rr() const
{
  return m_pData->header.card_high_rr;
}

int CPhilipsSubHeader::card_low_rr() const
{
  return m_pData->header.card_low_rr;
}

int CPhilipsSubHeader::card_tr_time() const
{
  return m_pData->header.card_tr_time;
}

const char* CPhilipsSubHeader::scatter_corr() const
{
  return m_pData->header.scatter_corr;
}

short CPhilipsSubHeader::deadtime_corr() const
{
  return m_pData->header.deadtime_corr;
}

CPhilipsSubHeader::Randoms_Type CPhilipsSubHeader::randoms_corr() const
{
  return static_cast<CPhilipsSubHeader::Randoms_Type>(m_pData->header.randoms_corr);
}

short CPhilipsSubHeader::det_norm() const
{
  return m_pData->header.det_norm;
}

short CPhilipsSubHeader::nu_radsamp_corr() const
{
  return m_pData->header.nu_radsamp_corr;
}

short CPhilipsSubHeader::pat_mot_corr() const
{
  return m_pData->header.pat_mot_corr;
}

float  CPhilipsSubHeader::echo_time() const
{
  return m_pData->header.echo_time;
}

float CPhilipsSubHeader::exposure_time() const
{
  return m_pData->header.exposure_time;
}

float CPhilipsSubHeader::img_pos_x() const
{
  return m_pData->header.img_pos_x;
}

float CPhilipsSubHeader::img_pos_y() const
{
  return m_pData->header.img_pos_y;
}

float CPhilipsSubHeader::img_pos_z() const
{
  return m_pData->header.img_pos_z;
}

CPhilipsSubHeader::Data_Type CPhilipsSubHeader::datype() const
{
  return static_cast<CPhilipsSubHeader::Data_Type>(m_pData->header.datype);
}

short CPhilipsSubHeader::xdim() const
{
  return m_pData->header.xdim;
}

short CPhilipsSubHeader::ydim() const
{
  return m_pData->header.ydim;
}

short CPhilipsSubHeader::slcnum() const
{
  return m_pData->header.slcnum;
}

short CPhilipsSubHeader::tiltnum() const
{
  return m_pData->header.tiltnum;
}

short CPhilipsSubHeader::gatint() const
{
  return m_pData->header.gatint;
}

short CPhilipsSubHeader::cntloss_corr() const
{
  return m_pData->header.cntloss_corr;
}

float CPhilipsSubHeader::pix_spacing_x() const
{
  return m_pData->header.pix_spacing_x;
}

float CPhilipsSubHeader::pix_spacing_y() const
{
  return m_pData->header.pix_spacing_y;
}

float CPhilipsSubHeader::xray_current() const
{
  return m_pData->header.xray_current;
}

float CPhilipsSubHeader::suvscl() const
{
  return m_pData->header.suvscl;
}

float CPhilipsSubHeader::kvp() const
{
  return m_pData->header.kvp;
}

float CPhilipsSubHeader::Dslice_loc() const
{
  return m_pData->header.Dslice_loc;
}

float CPhilipsSubHeader::magfac() const
{
  return m_pData->header.magfac;
}

float CPhilipsSubHeader::imgscl() const
{
  return m_pData->header.imgscl;
}

short CPhilipsSubHeader::imgmin() const
{
  return m_pData->header.imgmin;
}

short CPhilipsSubHeader::imgmax() const
{
  return m_pData->header.imgmax;
}

CPhilipsSubHeader::Decay_Type CPhilipsSubHeader::decay_corr() const
{
  return static_cast<CPhilipsSubHeader::Decay_Type>(m_pData->header.decay_corr);
}

short CPhilipsSubHeader::dep_strhr() const
{
  return m_pData->header.dep_strhr;
}

short CPhilipsSubHeader::dep_strmin() const
{
  return m_pData->header.dep_strmin;
}

short CPhilipsSubHeader::dep_strsec() const
{
  return m_pData->header.dep_strsec;
}

short CPhilipsSubHeader::dep_endhr() const
{
  return m_pData->header.dep_endhr;
}

short CPhilipsSubHeader::dep_endmin() const
{
  return m_pData->header.dep_endmin;
}

short CPhilipsSubHeader::dep_endsec() const
{
  return m_pData->header.dep_endsec;
}

int CPhilipsSubHeader::midtim() const
{
  return m_pData->header.midtim;
}

short CPhilipsSubHeader::mseclen() const
{
  return m_pData->header.mseclen;
}

short CPhilipsSubHeader::scnlen() const
{
  return m_pData->header.scnlen;
}

float CPhilipsSubHeader::imgsum() const
{
  return m_pData->header.imgsum;
}

float CPhilipsSubHeader::bgdelrt() const
{
  return m_pData->header.bgdelrt;
}

float CPhilipsSubHeader::enddelrt() const
{
  return m_pData->header.enddelrt;
}

float CPhilipsSubHeader::bgsngrt() const
{
  return m_pData->header.bgsngrt;
}

float CPhilipsSubHeader::bgcoincrt() const
{
  return m_pData->header.bgcoincrt;
}

float CPhilipsSubHeader::endsngrt() const
{
  return m_pData->header.endsngrt;
}

float CPhilipsSubHeader::endcoincrt() const
{
  return m_pData->header.endcoincrt;
}

float CPhilipsSubHeader::deadtimefac() const
{
  return m_pData->header.deadtimefac;
}

short CPhilipsSubHeader::bedpos() const
{
  return m_pData->header.bedpos;
}

float CPhilipsSubHeader::deadtime_bgsub() const
{
  return m_pData->header.deadtime_bgsub;
}

const char* CPhilipsSubHeader::sop_uid() const
{
  return m_pData->header.sop_uid;
}

const char* CPhilipsSubHeader::recon_method() const
{
  return m_pData->header.recon_method;
}

time_t CPhilipsSubHeader::start_date_time() const
{
  return m_pData->header.start_date_time;
}

time_t CPhilipsSubHeader::end_date_time() const
{
  return m_pData->header.end_date_time;
}

CPhilipsSubHeader::Laterality_Type CPhilipsSubHeader::laterality() const
{
  return static_cast<Laterality_Type>(m_pData->header.laterality);
}

CPhilipsSubHeader::Anatomy_Type CPhilipsSubHeader::anatomy() const
{
  return static_cast<Anatomy_Type>(m_pData->header.anatomy);
}

time_t CPhilipsSubHeader::frame_ref_date_time() const
{
  return m_pData->header.frame_ref_date_time;
}

int CPhilipsSubHeader::card_rr_time() const
{
  return m_pData->header.card_rr_time;
}

int CPhilipsSubHeader::resp_int_time() const
{
  return m_pData->header.resp_int_time;
}

unsigned short CPhilipsSubHeader::start_date_time_msec() const
{
  return m_pData->header.start_date_time_msec;
}

unsigned short CPhilipsSubHeader::end_date_time_msec() const
{
  return m_pData->header.end_date_time_msec;
}

unsigned short CPhilipsSubHeader::frame_ref_date_time_msec() const
{
  return m_pData->header.frame_ref_date_time_msec;
}

double CPhilipsSubHeader::prompt_events_sum() const
{
  return m_pData->header.prompt_events_sum;
}

double CPhilipsSubHeader::delay_events_sum() const
{
  return m_pData->header.scatter_fraction_average;
}

float CPhilipsSubHeader::scatter_fraction_average() const
{
  return m_pData->header.scatter_fraction_average;
}

// methods to modify elements of the SubHeader  
void CPhilipsSubHeader::setVersion(const short version)
{
  m_pData->header.version = version;
}

void CPhilipsSubHeader::setAtten_corr(const char* str)
{
  strncpy(m_pData->header.atten_corr, str, sizeof(m_pData->header.atten_corr)-1);
}

void CPhilipsSubHeader::setActual_bedpos(const float bedpos)
{
  m_pData->header.actual_bedpos = bedpos;
}

void CPhilipsSubHeader::setOrientation(const short i, const float orientation)
{
  if(i >= 0 && i <= 2)
    m_pData->header.orientation1[i] = orientation;
  else if(i >= 3 && i <= 5)
    m_pData->header.orientation2[i-3] = orientation;
  else
    E("index out of bounds: %d", i);
}

void CPhilipsSubHeader::setCard_fr_time(const int card_fr_time)
{
  m_pData->header.card_fr_time = card_fr_time;
}

void CPhilipsSubHeader::setCard_high_rr(const int card_high_rr)
{
  m_pData->header.card_high_rr = card_high_rr;
}

void CPhilipsSubHeader::setCard_low_rr(const int card_low_rr)
{
  m_pData->header.card_low_rr = card_low_rr;
}

void CPhilipsSubHeader::setCard_tr_time(const int card_high_rr)
{
  m_pData->header.card_high_rr = card_high_rr;
}

void CPhilipsSubHeader::setScatter_corr(const char* str)
{
  strncpy(m_pData->header.scatter_corr, str, sizeof(m_pData->header.scatter_corr)-1);
}

void CPhilipsSubHeader::setDeadtime_corr(const short deadtime_corr)
{
  m_pData->header.deadtime_corr = deadtime_corr;
}

void CPhilipsSubHeader::setRandoms_corr(const CPhilipsSubHeader::Randoms_Type randoms_corr)
{
  m_pData->header.randoms_corr = static_cast<qint16>(randoms_corr);
}

void CPhilipsSubHeader::setDet_norm(const short det_norm)
{
  m_pData->header.det_norm = det_norm;
}

void CPhilipsSubHeader::setNu_radsamp_corr(const short nu_radsamp_corr)
{
  m_pData->header.nu_radsamp_corr = nu_radsamp_corr;
}

void CPhilipsSubHeader::setPat_mot_corr(const short pat_mot_corr)
{
  m_pData->header.pat_mot_corr = pat_mot_corr;
}

void CPhilipsSubHeader::setEcho_time(const float  echo_time)
{
  m_pData->header.echo_time = echo_time;
}

void CPhilipsSubHeader::setExposure_time(const float exposure_time)
{
  m_pData->header.exposure_time = exposure_time;
}

void CPhilipsSubHeader::setImg_pos_x(const float img_pos_x)
{
  m_pData->header.img_pos_x = img_pos_x;
}

void CPhilipsSubHeader::setImg_pos_y(const float img_pos_y)
{
  m_pData->header.img_pos_y = img_pos_y;
}

void CPhilipsSubHeader::setImg_pos_z(const float img_pos_z)
{
  m_pData->header.img_pos_z = img_pos_z;
}

void CPhilipsSubHeader::setDatype(const CPhilipsSubHeader::Data_Type datype)
{
  m_pData->header.datype = static_cast<qint16>(datype);
}

void CPhilipsSubHeader::setXdim(const short xdim)
{
  m_pData->header.xdim = xdim;
}

void CPhilipsSubHeader::setYdim(const short ydim)
{
  m_pData->header.ydim = ydim;
}

void CPhilipsSubHeader::setSlcnum(const short slcnum)
{
  m_pData->header.slcnum = slcnum;
}

void CPhilipsSubHeader::setTiltnum(const short tiltnum)
{
  m_pData->header.tiltnum = tiltnum;
}

void CPhilipsSubHeader::setGatint(const short gatint)
{
  m_pData->header.gatint = gatint;
}

void CPhilipsSubHeader::setCntloss_corr(const short cntloss_corr)
{
  m_pData->header.cntloss_corr = cntloss_corr;
}

void CPhilipsSubHeader::setPix_spacing_x(const float pix_spacing)
{
  m_pData->header.pix_spacing_x = pix_spacing;
}

void CPhilipsSubHeader::setPix_spacing_y(const float pix_spacing)
{
  m_pData->header.pix_spacing_y = pix_spacing;
}

void CPhilipsSubHeader::setXray_current(const float xray_current)
{
  m_pData->header.xray_current = xray_current;
}

void CPhilipsSubHeader::setSuvscl(const float suvscl)
{
  m_pData->header.suvscl = suvscl;
}

void CPhilipsSubHeader::setKvp(const float kvp)
{
  m_pData->header.kvp = kvp;
}

void CPhilipsSubHeader::setDslice_loc(const float Dslice_loc)
{
  m_pData->header.Dslice_loc = Dslice_loc;
}

void CPhilipsSubHeader::setMagfac(const float magfac)
{
  m_pData->header.magfac = magfac;
}

void CPhilipsSubHeader::setImgscl(const float imgscl)
{
  m_pData->header.imgscl = imgscl;
}

void CPhilipsSubHeader::setImgmin(const short imgmin)
{
  m_pData->header.imgmin = imgmin;
}

void CPhilipsSubHeader::setImgmax(const short imgmax)
{
  m_pData->header.imgmax = imgmax;
}

void CPhilipsSubHeader::setDecay_corr(const CPhilipsSubHeader::Decay_Type decay_corr)
{
  m_pData->header.decay_corr = static_cast<qint16>(decay_corr);
}

void CPhilipsSubHeader::setDep_strhr(const short strhr)
{
  m_pData->header.dep_strhr = strhr;
}

void CPhilipsSubHeader::setDep_strmin(const short strmin)
{
  m_pData->header.dep_strmin = strmin;
}

void CPhilipsSubHeader::setDep_strsec(const short strsec)
{
  m_pData->header.dep_strsec = strsec;
}

void CPhilipsSubHeader::setDep_endhr(const short endhr)
{
  m_pData->header.dep_endhr = endhr;
}

void CPhilipsSubHeader::setDep_endmin(const short endmin)
{
  m_pData->header.dep_endmin = endmin;
}

void CPhilipsSubHeader::setDep_endsec(const short endsec)
{
  m_pData->header.dep_endsec = endsec;
}

void CPhilipsSubHeader::setMidtim(const int midtim)
{
  m_pData->header.midtim = midtim;
}

void CPhilipsSubHeader::setMseclen(const short mseclen)
{
  m_pData->header.mseclen = mseclen;
}

void CPhilipsSubHeader::setScnlen(const short scnlen)
{
  m_pData->header.scnlen = scnlen;
}

void CPhilipsSubHeader::setImgsum(const float imgsum)
{
  m_pData->header.imgsum = imgsum;
}

void CPhilipsSubHeader::setBgdelrt(const float bgdelrt)
{
  m_pData->header.bgdelrt = bgdelrt;
}

void CPhilipsSubHeader::setEnddelrt(const float enddelrt)
{
  m_pData->header.enddelrt = enddelrt;
}

void CPhilipsSubHeader::setBgsngrt(const float bgsngrt)
{
  m_pData->header.bgsngrt = bgsngrt;
}

void CPhilipsSubHeader::setBgcoincrt(const float bgcoincrt)
{
  m_pData->header.bgcoincrt = bgcoincrt;
}

void CPhilipsSubHeader::setEndsngrt(const float endsngrt)
{
  m_pData->header.endsngrt = endsngrt;
}

void CPhilipsSubHeader::setEndcoincrt(const float endcoincrt)
{
  m_pData->header.endcoincrt = endcoincrt;
}

void CPhilipsSubHeader::setDeadtimefac(const float deadtimefac)
{
  m_pData->header.deadtimefac = deadtimefac;
}

void CPhilipsSubHeader::setBedpos(const short bedpos)
{
  m_pData->header.bedpos = bedpos;
}

void CPhilipsSubHeader::setDeadtime_bgsub(const float deadtime_bgsub)
{
  m_pData->header.deadtime_bgsub = deadtime_bgsub;
}

void CPhilipsSubHeader::setSop_uid(const char* str)
{
  strncpy(m_pData->header.sop_uid, str, sizeof(m_pData->header.sop_uid)-1);
}

void CPhilipsSubHeader::setRecon_method(const char* str)
{
  strncpy(m_pData->header.recon_method, str, sizeof(m_pData->header.recon_method)-1);
}

void CPhilipsSubHeader::setStart_date_time(const time_t start_date_time)
{
  m_pData->header.start_date_time = start_date_time;

  // we also set strhr/strmin/strsec as it should always be in sync with
  // the start_date_time
  QDateTime dtime = start_date_time_Qt();
  m_pData->header.dep_strhr = dtime.time().hour();
  m_pData->header.dep_strmin = dtime.time().minute();
  m_pData->header.dep_strsec = dtime.time().second();
}

void CPhilipsSubHeader::setEnd_date_time(const time_t end_date_time)
{
  m_pData->header.end_date_time = end_date_time;

  // we also set strhr/strmin/strsec as it should always be in sync with
  // the end_date_time
  QDateTime dtime = end_date_time_Qt();
  m_pData->header.dep_endhr = dtime.time().hour();
  m_pData->header.dep_endmin = dtime.time().minute();
  m_pData->header.dep_endsec = dtime.time().second();
}

void CPhilipsSubHeader::setLaterality(const CPhilipsSubHeader::Laterality_Type laterality)
{
  m_pData->header.laterality = static_cast<qint16>(laterality);
}

void CPhilipsSubHeader::setAnatomy(const CPhilipsSubHeader::Anatomy_Type anatomy)
{
  m_pData->header.anatomy = static_cast<qint16>(anatomy);
}

void CPhilipsSubHeader::setFrame_ref_date_time(const time_t frame_ref_date_time)
{
  m_pData->header.frame_ref_date_time = frame_ref_date_time;
}

void CPhilipsSubHeader::setCard_rr_time(const int card_rr_time)
{
  m_pData->header.card_rr_time = card_rr_time;
}

void CPhilipsSubHeader::setResp_int_time(const int resp_int_time)
{
  m_pData->header.resp_int_time = resp_int_time;
}

void CPhilipsSubHeader::setStart_date_time_msec(const unsigned short msec)
{
  m_pData->header.start_date_time_msec = msec;
}

void CPhilipsSubHeader::setEnd_date_time_msec(const unsigned short msec)
{
  m_pData->header.end_date_time_msec = msec;
}

void CPhilipsSubHeader::setFrame_ref_date_time_msec(const unsigned short msec)
{
  m_pData->header.frame_ref_date_time_msec = msec;
}

void CPhilipsSubHeader::setPrompt_events_sum(const double value)
{
  m_pData->header.prompt_events_sum = value;
}

void CPhilipsSubHeader::setDelay_events_sum(const double value)
{
  m_pData->header.delay_events_sum = value;
}

void CPhilipsSubHeader::setScatter_fraction_average(const float value)
{
  m_pData->header.scatter_fraction_average = value;
}

int CPhilipsSubHeader::rawDataSize() const
{
  return 1*PHILIPS_BLOCKSIZE;
}

float CPhilipsSubHeader::scnscl() const
{
  return m_pData->header.scnscl;
}

short CPhilipsSubHeader::scnmin() const
{
  return m_pData->header.scnmin;
}

short CPhilipsSubHeader::scnmax() const
{
  return m_pData->header.scnmax;
}

float CPhilipsSubHeader::scnsum() const
{
  return m_pData->header.scnsum;
}

void CPhilipsSubHeader::setScnscl(const float scnscl)
{
  m_pData->header.scnscl = scnscl;
}

void CPhilipsSubHeader::setScnmin(const short scnmin)
{
  m_pData->header.scnmin = scnmin;
}

void CPhilipsSubHeader::setScnmax(const short scnmax)
{
  m_pData->header.scnmax = scnmax;
}

void CPhilipsSubHeader::setScnsum(const float scnsum)
{
  m_pData->header.scnsum = scnsum;
}

QDateTime CPhilipsSubHeader::start_date_time_Qt() const
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  QDateTime datetime = Jan1970.addSecs(m_pData->header.start_date_time);
  return datetime.toLocalTime();
}

void CPhilipsSubHeader::setStart_date_time_Qt(const QDateTime& datetime)
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  setStart_date_time(Jan1970.secsTo(datetime.toUTC()));
}

QDateTime CPhilipsSubHeader::end_date_time_Qt() const
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  QDateTime datetime = Jan1970.addSecs(m_pData->header.end_date_time);
  return datetime.toLocalTime();
}

void CPhilipsSubHeader::setEnd_date_time_Qt(const QDateTime& datetime)
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  setEnd_date_time(Jan1970.secsTo(datetime.toUTC()));
}

QDateTime CPhilipsSubHeader::frame_ref_date_time_Qt() const
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  QDateTime datetime = Jan1970.addSecs(m_pData->header.frame_ref_date_time);
  return datetime.toLocalTime();
}

void CPhilipsSubHeader::setFrame_ref_date_time_Qt(const QDateTime& datetime)
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  setFrame_ref_date_time(Jan1970.secsTo(datetime.toUTC()));
}
