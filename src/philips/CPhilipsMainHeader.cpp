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

#include "CPhilipsMainHeader.h"
#include "CPhilipsFile.h"
#include "CPhilipsDirectoryItem.h"
#include "CMedIOHeader.h"
#include "CMedIOData.h"

#include <rtdebug.h>

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
  struct HeaderData
  {
    qint16 file_fmt;       // file format (mainheader) version number
    qint16 scan_geom;  // encoding of the scanner geometry
    qint16 hw_config;   // encoding of hardware used in acquisition
    qint16 edit_flag;         // 1 to indicat that the user has modified mainheader
    qint16 filtyp;
    qint32 minTransXtalDiff;    // Minimum zone difference in crystal space during all list-mode collections.
    float tofTstampScale;      // Time-Of-Flight time stamp scale.

    qint16 dep_daycre;
    qint16 dep_mocre;
    qint16 dep_yrcre;
    qint16 dep_hrcre;
    qint16 dep_mincre;
    qint16 dep_seccre;
    qint16 duratn;     // duration of scan in seconds
    qint16 shdtyp;    // ImageIO subheader (1) or old subheader format (0)
    qint16 sngpscl; // singles prescale, superceded by pscale
    qint16 singopt; // 0=transmission only, 1=trans-ec, 2=ec only (obsoleted)
    float pscale;      // amount by which the actual singles events is scaled down
    float detectorRadius;       /* inscribed scanner radius (from middle of opening to
                                   detector face) in mm */
    qint16 virtualXtal; /* whether virtual crystals were used in the acquisition
                           to simulate a larger detector */
    qint16 phiMashing;       /* whether events from crystal phi values are combined
                                6 Undefined/Unknown (assumption is no mashing)
                                7 No mashing
                                8 Events from pairs of consecutive angles have been added together */
    qint16 polygonSides;    /* Number of "sides" in the scanner opening
                               (may refer to detectors or PMT modules */
    qint16 xtalsPerSide; // Number of detectors per opening (polygon) side
    qint16 nXtalRows;      // Number of crystal rows in the axial (Z) direction
    float crystalThickness;  // Detetctor crystal thickness in mm
    float xXtalPitch;     // X crystal pitch in mm
    float zXtalPitch;     // Z crystal pitch in mm
    float axialFOV;          // in mm
    qint16 rphiType;         // 0 Real, 1 Crystal
    qint16 sliceType;
    qint16 delayType;
    qint16 pattyp;
    qint16 scntyp;
    qint16 numray;
    qint16 numang;
    qint16 slcthk;
    qint16 isotop;
    float slope;                 // Rescale slope. (Not used)
    float intcpt; // Offset value to data. (Not used)
    qint16 injtim;
    float polygonVertAt0deg;
    qint16 nslice;        // number of slices per frame
    qint16 nframe;        // number of frames
    qint16 bthday;
    qint16 bthmo;
    qint16 bthyr;
    char ssn[10]; // suberceded by Dicom_Patient_ID
    qint16 ntilt;         // number of tilts per frame
    qint16 petnum;
    float activity;           // in MBq
    qint32 weight;            // in g
    qint16 hrinj;
    qint16 mininj;
    float srcRadius;
    float srcZpos;
    float halfLife;           // in minutes
    float concfac;
    float concfac_bgsub;
    float dmax;
    float dline;
    float angmax;
    float x0;
    float y0;
    float z0;
    float nevent;
    float nsino;
    qint16 eglob_low;
    qint16 eglob_up;
    qint16 eloc_low;
    qint16 eloc_up;
    qint16 orient_hf;
    char scan_swrel[6];
    qint16 tbl_direction;
    qint16 orient_ps;
    float frontLeadDiameter;
    float backLeadDiameter;
    float leadSeparation;
    float ndelays;
    float slcsep;
    char fctrfil[20];           /* Factor file name. (PMT gains). was 16 */
                                /* syn - (0008,1090) manufacturer's model name*/
    char baselin[20];        /* Baseline file name. (DC offsets). was 16 */
    char dstpkfl[20];        /* Distortion peak file name. */
    char aqprotocol_name[20];/* acquisition protocol name */
    qint16 aqprotocol_type;   /* 1=Emiss-Static, 2=Emiss-Dynamic,
                               * 3=Trans-Static, 4=Gated Cardiac,
                               * 5=Emiss-Whole-Body, 6=Trans-Whole-Body 
                               * 7=Singles Trans, 8=Singles-Whole-Body */
    char patient_name[30];   /* patient name */
    float reslice_ang1;      /* Reslicing (OBL) angle 1 */
    float reslice_ang2;      /* Reslicing (OBL) angle 2 */
    float reslice_ang3;      /* Reslicing (OBL) angle 3 */
    qint16 minslc;            /* Minimum Slice number */
    /* syn - set to 1		*/
    qint16 maxslc;            /* Maximum Slice number */
    /* syn - set to number of slices */
    qint16 minfrm;            /* Minimum Frame number */
    qint16 maxfrm;            /* Maximum Frame number */
    qint16 scanner_maxslice;  /* Maximum Slice number for a static frame
                              * based on the useful axial extent of the
                              * scanner. (unitless) */
    qint16 rebin_type;         /* multi-slice or single slice or LOR */
    char scnOrigin[16];       /* indicates origin of scan data = scanner number
                               * (a000) - $SITENAME (10 chars) plus null */
    char accNum[16]; /* Accession number. Will eventually be used with 
                                 DICOM query retrieve. Initially used to support
                                 dicom-send to PACS. */
    qint16 movementCoinc;      /* table movement between emission frames in mm */
    qint16 movementSing;   /* table movement between transmission frames in mm */
    qint16 crbTstampPeriod;    /* Period between CRB timestamps in msec */
    qint16 trailexists;       /* 1= trailer exists, 0 = no trailer */
    quint32 trailbeg;           /* unsigned 32 bit number = # of bytes from
                              * file beginning indicating where the
                              * trailer begins */
    struct 
    {
      qint16 valid;            /* Whether the file has a valid petct struct */
      qint16 separation;       /* PET separation distance at acq time (1/10mm) */
      qint16 landmark;         /* Landmark position at acq time (1/10mm) */
      struct
      {
        qint32 timestamp;     /* Date/Time of alignment calibration 
                                (seconds since Jan 1, 1970 [UNIX]) */
        qint16 zOffset;       /* Not used at this time.  */
        qint16 xShift;        /* shift in x from align-cal in 1/1000mm */
        qint16 yShift;        /* shift in y from align-cal in 1/1000mm */
        qint16 zShift;        /* shift in z from align-cal in 1/1000mm */
        qint16 acFlags;       /* Not used at this time. */
        qint16 xOffset;	      /* syn - (0020,0032) Image position x pos */
                              /* shift in x of the center of the CT FOV
                                 with respect to the PET FOV in 1/100mm */
        qint16 yOffset;	      /* syn - (0020,0032) Image position y pos */
                              /* shift in y of the center of the CT FOV
                                 with respect to the PET FOV in 1/100mm */
        qint16 axialRotation; /* rotation about z axis in 1/10,000 deg  */
        qint16 horizRotation; /* rotation about x axis in 1/10,000 deg  */
        qint16 vertRotation;  /* rotation about y axis in 1/10,000 deg  */
        qint16 unused[2];     /* Not used at this time. */
      } alignment;
    } petCt;
  } header;

  static const short currentMainHeaderFormat = 13;
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

  // set some default values for the new Main Header
  m_pData->header.file_fmt = CPhilipsMainHeaderPrivate::currentMainHeaderFormat;

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

  // we now read out the header information stepwise
  // because there are some gaps between the header entries
  // we have to skip some bytes
  stream.skipRawData(6);                      // 0: skip the first 6 bytes
  stream >> m_pData->header.file_fmt;      // 6: file_fmt
  stream >> m_pData->header.scan_geom; // 8: scan_geom
  stream >> m_pData->header.hw_config;  // 10: hw_config
  stream.skipRawData(4);                      // 12: skip the next 4 bytes
  stream >> m_pData->header.edit_flag;        // 16: edit_flag
  stream.skipRawData(34);                     // 18: skip the next 34 bytes
  stream >> m_pData->header.filtyp;        // 52: filtyp
  stream >> m_pData->header.minTransXtalDiff; // 54: minTransXtalDiff
  stream >> m_pData->header.tofTstampScale;   // 58: tofTstampScale
  stream.skipRawData(4);                     // 62: skip the next 4 bytes
  stream >> m_pData->header.dep_daycre;      // 66: dep_daycre
  stream >> m_pData->header.dep_mocre;    // 68: dep_mocre
  stream >> m_pData->header.dep_yrcre;     // 70: dep_yrcre
  stream >> m_pData->header.dep_hrcre;     // 72: dep_hrcre
  stream >> m_pData->header.dep_mincre;   // 74: dep_mincre
  stream >> m_pData->header.dep_seccre;   // 76: dep_seccre
  stream >> m_pData->header.duratn;    // 78: duratn
  stream >> m_pData->header.shdtyp;   // 80: shdtyp
  stream >> m_pData->header.sngpscl; // 82: sngpscl
  stream >> m_pData->header.singopt; // 84: singopt
  stream >> m_pData->header.pscale; // 86: Singles_Prescale
  stream >> m_pData->header.detectorRadius;  // 90: detectorRadius
  stream >> m_pData->header.virtualXtal; // 94: virtualXtal
  stream >> m_pData->header.phiMashing;      // 96: phiMashing
  stream >> m_pData->header.polygonSides;    // 98: polygonSides
  stream >> m_pData->header.xtalsPerSide; // 100: xtalsPerSide
  stream >> m_pData->header.nXtalRows;      // 102: nXtalRows
  stream >> m_pData->header.crystalThickness; // 104: crystalThickness
  stream >> m_pData->header.xXtalPitch;    // 108: xXtalPitch
  stream >> m_pData->header.zXtalPitch;    // 112: zXtalPitch
  stream >> m_pData->header.axialFOV;         // 116: axialFOV
  stream >> m_pData->header.rphiType;         // 120: rphiType
  stream >> m_pData->header.sliceType;        // 122: sliceType
  stream >> m_pData->header.delayType;        // 124: delayType
  stream.skipRawData(4);                       // 126: skip the next 4 bytes
  stream >> m_pData->header.pattyp;         // 130: pattyp
  stream >> m_pData->header.scntyp;  // 132: scntyp
  stream >> m_pData->header.numray;           // 134: numray
  stream >> m_pData->header.numang;           // 136: numang
  stream >> m_pData->header.slcthk;   // 138: slcthk
  stream >> m_pData->header.isotop;            // 140: isotop
  stream >> m_pData->header.slope;             // 142: slope
  stream >> m_pData->header.intcpt; // 146: intcpt
  stream >> m_pData->header.injtim;        // 150: injtim
  stream >> m_pData->header.polygonVertAt0deg;  // 152: polygonVertAt0deg
  stream >> m_pData->header.nslice;            // 156: nslice
  stream >> m_pData->header.nframe;            // 158: nframe
  stream >> m_pData->header.bthday;         // 160: bthday
  stream >> m_pData->header.bthmo;       // 162: bthmo
  stream >> m_pData->header.bthyr;        // 164: bthyr
  stream.readRawData(&m_pData->header.ssn[0], 10); // 166: ssn
  stream >> m_pData->header.ntilt;                         // 176: ntilt
  stream >> m_pData->header.petnum; // 178: petnum
  stream.skipRawData(4);      // 180: skip 4 bytes 0x00,0x21,0xff,0xff
  stream >> m_pData->header.activity;      // 184: activity 
  stream >> m_pData->header.weight;        // 188: weight
  stream >> m_pData->header.hrinj;         // 192: hrinj
  stream >> m_pData->header.mininj;        // 194: mininj
  stream >> m_pData->header.srcRadius;     // 196: srcRadius
  stream >> m_pData->header.srcZpos;       // 200: srcZpos
  stream >> m_pData->header.halfLife;                     // 204: halfLife
  stream >> m_pData->header.concfac;                      // 208: concfac
  stream >> m_pData->header.concfac_bgsub;                // 212: concfac_bgsub
  stream >> m_pData->header.dmax;                         // 216: dmax
  stream >> m_pData->header.dline;                        // 220: dline
  stream >> m_pData->header.angmax;                       // 224: angmax
  stream >> m_pData->header.x0;                           // 228: x0
  stream >> m_pData->header.y0;                           // 232: y0
  stream >> m_pData->header.z0;                           // 236: z0
  stream >> m_pData->header.nevent;                       // 240: nevent
  stream >> m_pData->header.nsino;                        // 244: nsino
  stream >> m_pData->header.eglob_low;                   // 248: eglob_low
  stream >> m_pData->header.eglob_up;                    // 250: eglob_up
  stream >> m_pData->header.eloc_low;                    // 252: eloc_low
  stream >> m_pData->header.eloc_up;                     // 254: eloc_up
  stream >> m_pData->header.orient_hf; // 256: orient_hf
  stream.readRawData(&m_pData->header.scan_swrel[0], 6); // 258: scan_swrel
  stream >> m_pData->header.petCt.separation;
  stream >> m_pData->header.petCt.landmark;
  stream >> m_pData->header.petCt.alignment.timestamp;
  stream >> m_pData->header.tbl_direction; // 272: tbl_direction
  stream >> m_pData->header.orient_ps;     // 274: orient_ps
  stream >> m_pData->header.petCt.alignment.zOffset;
  stream >> m_pData->header.petCt.alignment.xShift;
  stream >> m_pData->header.petCt.alignment.yShift;
  stream >> m_pData->header.petCt.alignment.zShift;
  stream >> m_pData->header.petCt.alignment.acFlags;
  stream >> m_pData->header.petCt.alignment.xOffset;
  stream >> m_pData->header.petCt.alignment.yOffset;
  stream >> m_pData->header.petCt.alignment.axialRotation;
  stream >> m_pData->header.petCt.alignment.horizRotation;
  stream >> m_pData->header.petCt.alignment.vertRotation;
  stream >> m_pData->header.frontLeadDiameter; // 296: frontLeadDiameter
  stream >> m_pData->header.backLeadDiameter;  // 300: backLeadDiameter
  stream >> m_pData->header.leadSeparation;    // 304: leadSeparation
  stream >> m_pData->header.ndelays;           // 308: ndelays
  stream >> m_pData->header.slcsep;            // 312: slcsep
  stream >> m_pData->header.petCt.valid;       // 316: petct_valid
  stream.readRawData(&m_pData->header.fctrfil[0], 20); // 318: fctrfil
  stream.readRawData(&m_pData->header.baselin[0], 20); // 338: baselin
  stream.readRawData(&m_pData->header.dstpkfl[0], 20); // 358: dstpkfl
  stream.readRawData(&m_pData->header.aqprotocol_name[0], 20); // 378: aqprotocol_name
  stream >> m_pData->header.aqprotocol_type;                   // 398: aqprotocol_type
  stream.readRawData(&m_pData->header.patient_name[0], 30); // 400: patient_name
  stream >> m_pData->header.reslice_ang1;       // 430: reslice_ang1
  stream >> m_pData->header.reslice_ang2;       // 434: reslice_ang2
  stream >> m_pData->header.reslice_ang3;       // 438: reslice_ang3
  stream >> m_pData->header.minslc;            // 442: minslc
  stream >> m_pData->header.maxslc;            // 444: maxslc
  stream >> m_pData->header.minfrm;            // 448: minfrm
  stream >> m_pData->header.maxfrm;            // 450: maxfrm
  stream >> m_pData->header.scanner_maxslice;  // 452: scanner_maxslice
  stream.skipRawData(2);                       // 454: skip the next 2 bytes
  stream >> m_pData->header.rebin_type; // 456: rebin_type
  stream.readRawData(&m_pData->header.scnOrigin[0], 16); // 458: scnOrigin
  stream.readRawData(&m_pData->header.accNum[0], 16); // 474: accNum
  stream >> m_pData->header.movementCoinc;            // 490: movementCoinc
  stream >> m_pData->header.movementSing;            // 492: movementSing
  stream >> m_pData->header.crbTstampPeriod;            // 494: crbTstampPeriod
  stream.skipRawData(10);                            // 496: skip the next 10 bytes
  stream >> m_pData->header.trailexists;            // 506: trailexists
  stream >> m_pData->header.trailbeg;               // 508: trailbeg

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
  D("petct_valid                  : %d", m_pData->header.petCt.valid);
  D("petct_separation             : %d", m_pData->header.petCt.separation);
  D("petct_landmark               : %d", m_pData->header.petCt.landmark);
  D("petct_alignment_timestamp    : %d", m_pData->header.petCt.alignment.timestamp);
  D("petct_alignment_zOffset      : %d", m_pData->header.petCt.alignment.zOffset);
  D("petct_alignment_xShift       : %d", m_pData->header.petCt.alignment.xShift);
  D("petct_alignment_yShift       : %d", m_pData->header.petCt.alignment.yShift);
  D("petct_alignment_zShift       : %d", m_pData->header.petCt.alignment.zShift);
  D("petct_alignment_acFlags      : %d", m_pData->header.petCt.alignment.acFlags);
  D("petct_alignment_xOffset      : %d", m_pData->header.petCt.alignment.xOffset);
  D("petct_alignment_yOffset      : %d", m_pData->header.petCt.alignment.yOffset);
  D("petct_alignment_axialRotation: %d", m_pData->header.petCt.alignment.axialRotation);
  D("petct_alignment_horizRotation: %d", m_pData->header.petCt.alignment.horizRotation);
  D("petct_alignment_vertRotation : %d", m_pData->header.petCt.alignment.vertRotation);
#endif

  RETURN(true);
  return true;
}

bool CPhilipsMainHeader::save(void) const
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

  // before we can start reading out some data we have to collect some
  // out data beforehand which we use instead of the data stored in our
  // data structure (such as frames/slices/tilts etc.)
  CPhilipsFile* philipsFile = static_cast<CPhilipsFile*>(m_pMedIOData);

  short minFrame = philipsFile->minFrame();
  short maxFrame = philipsFile->maxFrame();
  short numFrames = philipsFile->numFrames();
  short minSlice = philipsFile->minSlice();
  short maxSlice = philipsFile->maxSlice();
  short numSlices  = philipsFile->numSlices();
  short numTilts = philipsFile->numTilts();
  
  // we write to a buffer first and write out later directly to the file
  QByteArray buffer(rawDataSize(), 0);
  QDataStream stream(&buffer, QIODevice::WriteOnly);

  // we have to set the QDataStream version to the Qt4.5 version
  // because with Qt4.6 the floating point precision changed and
  // otherwise causes our streaming to fail
  stream.setVersion(QDataStream::Qt_4_5);

  // we now read out the header information stepwise
  // as we have to care about big/little endianess, which
  // is automatically done by the QT framework

  // to write out the fill bytes
  // we need a buffer
  char byte[50];

  byte[0]= 0x01;
  byte[1]= 0x00;
  byte[2]= 0x00;
  byte[3]= 0x00;
  byte[4]= 0x00;
  byte[5]= 0x16;
  stream.writeRawData(byte, 6); // 0: write the "file header"


  stream << m_pData->header.file_fmt;      // 6: file_fmt
  stream << m_pData->header.scan_geom; // 8: scan_geom
  stream << m_pData->header.hw_config;  // 10: hw_config

  byte[0]= 0x00;
  byte[1]= 0x40;
  byte[2]= 0x00;
  byte[3]= 0x5B;
  stream.writeRawData(byte, 4); // 12: write the next 4 bytes

  stream << m_pData->header.edit_flag;        // 16: edit_flag

  memset(byte, 0, 30);
  stream.writeRawData(byte, 30);                     // 18: write the next 34 bytes
  byte[0]= 0x00;
  byte[1]= 0x07;
  byte[2]= 0xFF;
  byte[3]= 0xFF;
  stream.writeRawData(byte, 4);

  stream << m_pData->header.filtyp;        // 52: filtyp
  stream << m_pData->header.minTransXtalDiff; // 54: minTransXtalDiff
  stream << m_pData->header.tofTstampScale;   // 58: tofTstampScale

  byte[0]= 0x00;
  byte[1]= 0x20;
  byte[2]= 0xFF;
  byte[3]= 0xFF;  
  stream.writeRawData(byte, 4);

  stream << m_pData->header.dep_daycre;      // 66: dep_daycre
  stream << m_pData->header.dep_mocre;    // 68: dep_mocre
  stream << m_pData->header.dep_yrcre;     // 70: dep_yrcre
  stream << m_pData->header.dep_hrcre;     // 72: dep_hrcre
  stream << m_pData->header.dep_mincre;   // 74: dep_mincre
  stream << m_pData->header.dep_seccre;   // 76: dep_seccre
  stream << m_pData->header.duratn;    // 78: duratn
  stream << m_pData->header.shdtyp;   // 80: shdtyp
  stream << m_pData->header.sngpscl; // 82: sngpscl
  stream << m_pData->header.singopt; // 84: singopt
  stream << m_pData->header.pscale; // 86: Singles_Prescale
  stream << m_pData->header.detectorRadius;  // 90: detectorRadius
  stream << m_pData->header.virtualXtal; // 94: virtualXtal
  stream << m_pData->header.phiMashing;      // 96: phiMashing
  stream << m_pData->header.polygonSides;    // 98: polygonSides
  stream << m_pData->header.xtalsPerSide; // 100: xtalsPerSide
  stream << m_pData->header.nXtalRows;      // 102: nXtalRows
  stream << m_pData->header.crystalThickness; // 104: crystalThickness
  stream << m_pData->header.xXtalPitch;    // 108: xXtalPitch
  stream << m_pData->header.zXtalPitch;    // 112: zXtalPitch
  stream << m_pData->header.axialFOV;         // 116: axialFOV
  stream << m_pData->header.rphiType;         // 120: rphiType
  stream << m_pData->header.sliceType;        // 122: sliceType
  stream << m_pData->header.delayType;        // 124: delayType

  byte[0]= 0x00;
  byte[1]= 0x1B;
  byte[2]= 0xFF;
  byte[3]= 0xFF;  
  stream.writeRawData(byte, 4);

  stream << m_pData->header.pattyp;         // 130: pattyp
  stream << m_pData->header.scntyp;  // 132: scntyp
  stream << m_pData->header.numray;           // 134: numray
  stream << m_pData->header.numang;           // 136: numang
  stream << m_pData->header.slcthk;   // 138: slcthk
  stream << m_pData->header.isotop;            // 140: isotop
  stream << m_pData->header.slope;             // 142: slope
  stream << m_pData->header.intcpt; // 146: intcpt
  stream << m_pData->header.injtim;        // 150: injtim
  stream << m_pData->header.polygonVertAt0deg;  // 152: polygonVertAt0deg
  stream << numSlices;            // 156: nslice
  stream << numFrames;            // 158: nframe
  stream << m_pData->header.bthday;         // 160: bthday
  stream << m_pData->header.bthmo;       // 162: bthmo
  stream << m_pData->header.bthyr;        // 164: bthyr
  stream.writeRawData(&m_pData->header.ssn[0], 10); // 166: ssn
  stream << numTilts;                         // 176: ntilt
  stream << m_pData->header.petnum; // 178: petnum

  byte[0]= 0x00;
  byte[1]= 0x21;
  byte[2]= 0xFF;
  byte[3]= 0xFF;  
  stream.writeRawData(byte, 4);

  stream << m_pData->header.activity;      // 184: activity 
  stream << m_pData->header.weight;        // 188: weight
  stream << m_pData->header.hrinj;         // 192: hrinj
  stream << m_pData->header.mininj;        // 194: mininj
  stream << m_pData->header.srcRadius;     // 196: srcRadius
  stream << m_pData->header.srcZpos;       // 200: srcZpos
  stream << m_pData->header.halfLife;                     // 204: halfLife
  stream << m_pData->header.concfac;                      // 208: concfac
  stream << m_pData->header.concfac_bgsub;                // 212: concfac_bgsub
  stream << m_pData->header.dmax;                         // 216: dmax
  stream << m_pData->header.dline;                        // 220: dline
  stream << m_pData->header.angmax;                       // 224: angmax
  stream << m_pData->header.x0;                           // 228: x0
  stream << m_pData->header.y0;                           // 232: y0
  stream << m_pData->header.z0;                           // 236: z0
  stream << m_pData->header.nevent;                       // 240: nevent
  stream << m_pData->header.nsino;                        // 244: nsino
  stream << m_pData->header.eglob_low;                   // 248: eglob_low
  stream << m_pData->header.eglob_up;                    // 250: eglob_up
  stream << m_pData->header.eloc_low;                    // 252: eloc_low
  stream << m_pData->header.eloc_up;                     // 254: eloc_up
  stream << m_pData->header.orient_hf; // 256: orient_hf
  stream.writeRawData(&m_pData->header.scan_swrel[0], 6); // 258: scan_swrel

  stream << m_pData->header.petCt.separation;
  stream << m_pData->header.petCt.landmark;
  stream << m_pData->header.petCt.alignment.timestamp;

  stream << m_pData->header.tbl_direction; // 272: tbl_direction
  stream << m_pData->header.orient_ps;     // 274: orient_ps

  stream << m_pData->header.petCt.alignment.zOffset;
  stream << m_pData->header.petCt.alignment.xShift;
  stream << m_pData->header.petCt.alignment.yShift;
  stream << m_pData->header.petCt.alignment.zShift;
  stream << m_pData->header.petCt.alignment.acFlags;
  stream << m_pData->header.petCt.alignment.xOffset;
  stream << m_pData->header.petCt.alignment.yOffset;
  stream << m_pData->header.petCt.alignment.axialRotation;
  stream << m_pData->header.petCt.alignment.horizRotation;
  stream << m_pData->header.petCt.alignment.vertRotation;

  stream << m_pData->header.frontLeadDiameter; // 296: frontLeadDiameter
  stream << m_pData->header.backLeadDiameter;  // 300: backLeadDiameter
  stream << m_pData->header.leadSeparation;    // 304: leadSeparation
  stream << m_pData->header.ndelays;           // 308: ndelays
  stream << m_pData->header.slcsep;            // 312: slcsep
  stream << m_pData->header.petCt.valid;       // 316: petct_valid
  stream.writeRawData(&m_pData->header.fctrfil[0], 20); // 318: fctrfil
  stream.writeRawData(&m_pData->header.baselin[0], 20); // 338: baselin
  stream.writeRawData(&m_pData->header.dstpkfl[0], 20); // 358: dstpkfl
  stream.writeRawData(&m_pData->header.aqprotocol_name[0], 20); // 378: aqprotocol_name
  stream << m_pData->header.aqprotocol_type;                   // 398: aqprotocol_type
  stream.writeRawData(&m_pData->header.patient_name[0], 30); // 400: patient_name
  stream << m_pData->header.reslice_ang1;       // 430: reslice_ang1
  stream << m_pData->header.reslice_ang2;       // 434: reslice_ang2
  stream << m_pData->header.reslice_ang3;       // 438: reslice_ang3
  stream << minSlice;            // 442: minslc
  stream << maxSlice;            // 444: maxslc
  stream << minFrame;            // 448: minfrm
  stream << maxFrame;            // 450: maxfrm
  stream << m_pData->header.scanner_maxslice;  // 452: scanner_maxslice

  memset(byte, 0, 2);
  stream.writeRawData(byte, 2);                       // 454: write the next 2 bytes

  stream << m_pData->header.rebin_type; // 456: rebin_type
  stream.writeRawData(&m_pData->header.scnOrigin[0], 16); // 458: scnOrigin
  stream.writeRawData(&m_pData->header.accNum[0], 16); // 474: accNum
  stream << m_pData->header.movementCoinc;            // 490: movementCoinc
  stream << m_pData->header.movementSing;            // 492: movementSing
  stream << m_pData->header.crbTstampPeriod;            // 494: crbTstampPeriod

  memset(byte, 0, 10);         // 496: write the next 10 bytes
  stream.writeRawData(byte, 10); 

  stream << m_pData->header.trailexists;            // 506: trailexists
  stream << m_pData->header.trailbeg;               // 508: trailbeg

  byte[0] = 0x00;
  stream.writeRawData(byte, 1);

  // now write out to our outStream
  bool result = false;
  if(m_pMedIOData->write(buffer) != -1)
  {
    philipsFile->mainHeaderWritten(*this);
    result = true;
  }

  RETURN(result);
  return result;
}

int CPhilipsMainHeader::rawDataSize() const
{
  return PHILIPS_BLOCKSIZE;
}

CMedIOHeader::Format CPhilipsMainHeader::headerFormat() const
{ 
  return CMedIOHeader::PhilipsMainHeader;
}

bool CPhilipsMainHeader::convertFrom(const CMedIOHeader* pHead1, const CMedIOHeader* pHead2) 
{
  ENTER();
  bool bResult = false;
#warning TODO: implement CPhilipsMainHeader::convertFrom()
  RETURN(false);
  return false;
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

short CPhilipsMainHeader::daycre() const
{
  return m_pData->header.dep_daycre;
}

short CPhilipsMainHeader::mocre() const
{
  return m_pData->header.dep_mocre;
}

short CPhilipsMainHeader::yrcre() const
{
  return m_pData->header.dep_yrcre;
}

short CPhilipsMainHeader::hrcre() const
{
  return m_pData->header.dep_hrcre;
}

short CPhilipsMainHeader::mincre() const
{
  return m_pData->header.dep_mincre;
}

short CPhilipsMainHeader::seccre() const
{
  return m_pData->header.dep_seccre;
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
  return static_cast<Valid_Header_Struct>(m_pData->header.petCt.valid);
}

short CPhilipsMainHeader::petct_separation() const
{
  return m_pData->header.petCt.separation;
}

short CPhilipsMainHeader::petct_landmark() const
{
  return m_pData->header.petCt.landmark;
}

time_t CPhilipsMainHeader::petct_alignment_timestamp() const
{
  return m_pData->header.petCt.alignment.timestamp;
}

short CPhilipsMainHeader::petct_alignment_zOffset() const
{
  return m_pData->header.petCt.alignment.zOffset;
}

short CPhilipsMainHeader::petct_alignment_xShift() const
{
  return m_pData->header.petCt.alignment.xShift;
}

short CPhilipsMainHeader::petct_alignment_yShift() const
{
  return m_pData->header.petCt.alignment.yShift;
}

short CPhilipsMainHeader::petct_alignment_zShift() const
{
  return m_pData->header.petCt.alignment.zShift;
}

short CPhilipsMainHeader::petct_alignment_acFlags() const
{
  return m_pData->header.petCt.alignment.acFlags;
}

short CPhilipsMainHeader::petct_alignment_xOffset() const
{
  return m_pData->header.petCt.alignment.xOffset;
}

short CPhilipsMainHeader::petct_alignment_yOffset() const
{
  return m_pData->header.petCt.alignment.yOffset;
}

short CPhilipsMainHeader::petct_alignment_axialRotation() const
{
  return m_pData->header.petCt.alignment.axialRotation;
}

short CPhilipsMainHeader::petct_alignment_horizRotation() const
{
  return m_pData->header.petCt.alignment.horizRotation;
}

short CPhilipsMainHeader::petct_alignment_vertRotation() const
{
  return m_pData->header.petCt.alignment.vertRotation;
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

void CPhilipsMainHeader::setDaycre(const short day)
{
  m_pData->header.dep_daycre = day;
}

void CPhilipsMainHeader::setMocre(const short month)
{
  m_pData->header.dep_mocre = month;
}

void CPhilipsMainHeader::setYrcre(const short year)
{
  m_pData->header.dep_yrcre = year;
}

void CPhilipsMainHeader::setHrcre(const short hour)
{
  m_pData->header.dep_hrcre = hour;
}

void CPhilipsMainHeader::setMincre(const short minute)
{
  m_pData->header.dep_mincre = minute;
}

void CPhilipsMainHeader::setSeccre(const short second)
{
  m_pData->header.dep_seccre = second;
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
  strncpy(m_pData->header.ssn, id, 10);
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
  strncpy(m_pData->header.scan_swrel, str, 6);
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
  strncpy(m_pData->header.fctrfil, str, 20);
}

void CPhilipsMainHeader::setBaselin(const char* str)
{
  strncpy(m_pData->header.baselin, str, 20);
}

void CPhilipsMainHeader::setDstpkfl(const char* str)
{
  strncpy(m_pData->header.dstpkfl, str, 20);
}

void CPhilipsMainHeader::setAqprotocol_Name(const char* str)
{
  strncpy(m_pData->header.aqprotocol_name, str, 20);
}

void CPhilipsMainHeader::setAqprotocol_Type(const CPhilipsMainHeader::Acquisition_Protocol_Type aqprotocol_type)
{
  m_pData->header.aqprotocol_type =  static_cast<qint16>(aqprotocol_type);
}

void CPhilipsMainHeader::setPatient_Name(const char* str)
{
  strncpy(m_pData->header.patient_name, str, 30);
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
  strncpy(m_pData->header.scnOrigin, str, 16);
}

void CPhilipsMainHeader::setAccNum(const char *str)
{
  strncpy(m_pData->header.accNum, str, 16);
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
  m_pData->header.petCt.valid = static_cast<qint16>(petct_valid);
}

void CPhilipsMainHeader::setPetct_separation(const short separation)
{
  m_pData->header.petCt.separation = separation;
}

void CPhilipsMainHeader::setPetct_landmark(const short landmark)
{
  m_pData->header.petCt.landmark = landmark;
}

void CPhilipsMainHeader::setPetct_alignment_timestamp(const time_t timestamp)
{
  m_pData->header.petCt.alignment.timestamp = timestamp;
}

void CPhilipsMainHeader::setPetct_alignment_zOffset(const short zOffset)
{
  m_pData->header.petCt.alignment.zOffset = zOffset;
}

void CPhilipsMainHeader::setPetct_alignment_xShift(const short xShift)
{
  m_pData->header.petCt.alignment.xShift = xShift;
}

void CPhilipsMainHeader::setPetct_alignment_yShift(const short yShift)
{
  m_pData->header.petCt.alignment.yShift = yShift;
}

void CPhilipsMainHeader::setPetct_alignment_zShift(const short zShift)
{
  m_pData->header.petCt.alignment.zShift = zShift;
}

void CPhilipsMainHeader::setPetct_alignment_acFlags(const short acFlags)
{
  m_pData->header.petCt.alignment.acFlags = acFlags;
}

void CPhilipsMainHeader::setPetct_alignment_xOffset(const short xOffset)
{
  m_pData->header.petCt.alignment.xOffset = xOffset;
}

void CPhilipsMainHeader::setPetct_alignment_yOffset(const short yOffset)
{
  m_pData->header.petCt.alignment.yOffset = yOffset;
}

void CPhilipsMainHeader::setPetct_alignment_axialRotation(const short axialRotation)
{
  m_pData->header.petCt.alignment.axialRotation = axialRotation;
}

void CPhilipsMainHeader::setPetct_alignment_horizRotation(const short horizRotation)
{
  m_pData->header.petCt.alignment.horizRotation = horizRotation;
}

void CPhilipsMainHeader::setPetct_alignment_vertRotation(const short vertRotation)
{
  m_pData->header.petCt.alignment.vertRotation = vertRotation;
}
