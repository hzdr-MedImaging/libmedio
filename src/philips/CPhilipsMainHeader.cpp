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
      // skip 4 bytes 0x00,0x21,0xff,0xff
      float activity;           // in MBq
      float weight;
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
      char scan_swrel;


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

  setFile_Type(fileType);

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
  stream.skipRawData(12);                     // 54: skip the next 12 bytes
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

#if defined(DEBUG)
  D("philips Main Header loaded:");
  D("--------------------------");
  D("file_fmt:        : %d", m_pData->header.file_fmt);
  D("scan_geom        : %d", m_pData->header.scan_geom);
  D("hw_config        : %d", m_pData->header.hw_config);
  D("edit_flag        : %d", m_pData->header.edit_flag);
  D("filtyp           : %d", m_pData->header.filtyp);
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
  D("weight           : %f", m_pData->header.weight);
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
#endif

  RETURN(true);
  return true;
}

bool CPhilipsMainHeader::save(void) const
{
  ENTER();
#warning TODO: implement CPhilipsMainHEader::save()
  RETURN(false);
  return false;
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

short CPhilipsMainHeader::file_Format() const
{
  return m_pData->header.file_fmt;
}

short CPhilipsMainHeader::scanner_Geometry() const
{
  return m_pData->header.scan_geom;
}

short CPhilipsMainHeader::hardware_Config() const
{
  return m_pData->header.hw_config;
}

short CPhilipsMainHeader::edit_Flag() const
{
  return m_pData->header.edit_flag;
}

CPhilipsMainHeader::File_Type CPhilipsMainHeader::file_Type() const
{
  return static_cast<File_Type>(m_pData->header.filtyp);
}

short CPhilipsMainHeader::day_Created() const
{
  return m_pData->header.dep_daycre;
}

short CPhilipsMainHeader::month_Created() const
{
  return m_pData->header.dep_mocre;
}

short CPhilipsMainHeader::year_Created() const
{
  return m_pData->header.dep_yrcre;
}

short CPhilipsMainHeader::hour_Created() const
{
  return m_pData->header.dep_hrcre;
}

short CPhilipsMainHeader::minute_Created() const
{
  return m_pData->header.dep_mincre;
}

short CPhilipsMainHeader::second_Created() const
{
  return m_pData->header.dep_seccre;
}

short CPhilipsMainHeader::scan_Duration() const
{
  return m_pData->header.duratn;
}

CPhilipsMainHeader::Subheader_Type CPhilipsMainHeader::subheader_Type() const
{
  return static_cast<Subheader_Type>(m_pData->header.shdtyp);
}

short CPhilipsMainHeader::singles_PreScale_Old() const
{
  return m_pData->header.sngpscl;
}

float CPhilipsMainHeader::singles_PreScale() const
{
  return m_pData->header.pscale;
}

float CPhilipsMainHeader::detector_Radius() const
{
  return m_pData->header.detectorRadius;
}

bool CPhilipsMainHeader::virtual_Crystals() const
{
  return (m_pData->header.virtualXtal == 1);
}

short CPhilipsMainHeader::phi_Mashing() const
{
  return m_pData->header.phiMashing;
}

short CPhilipsMainHeader::polygon_Sides() const
{
  return m_pData->header.polygonSides;
}

short CPhilipsMainHeader::crystals_Per_Side() const
{
  return m_pData->header.xtalsPerSide;
}

short CPhilipsMainHeader::crystal_Rows() const
{
  return m_pData->header.nXtalRows;
}

float CPhilipsMainHeader::crystal_Thickness() const
{
  return m_pData->header.crystalThickness;
}

float CPhilipsMainHeader::x_CrystalPitch() const
{
  return m_pData->header.xXtalPitch;
}

float CPhilipsMainHeader::z_CrystalPitch() const
{
  return m_pData->header.zXtalPitch;
}

float CPhilipsMainHeader::axial_FOV() const
{
  return m_pData->header.axialFOV;
}

CPhilipsMainHeader::RPhi_Type CPhilipsMainHeader::rPhi_Type() const
{
  return static_cast<RPhi_Type>(m_pData->header.rphiType);
}

CPhilipsMainHeader::Slice_Type CPhilipsMainHeader::slice_Type() const
{
  return static_cast<Slice_Type>(m_pData->header.sliceType);
}

CPhilipsMainHeader::Delay_Type CPhilipsMainHeader::delay_Type() const
{
  return static_cast<Delay_Type>(m_pData->header.delayType);
}

CPhilipsMainHeader::Scan_Type CPhilipsMainHeader::scan_Type() const
{
  return static_cast<Scan_Type>(m_pData->header.pattyp);
}

CPhilipsMainHeader::Acquisition_Type CPhilipsMainHeader::acquisition_Type() const
{
  return static_cast<Acquisition_Type>(m_pData->header.scntyp);
}

short CPhilipsMainHeader::num_Ray() const
{
  return m_pData->header.numray;
}

short CPhilipsMainHeader::num_Ang() const
{
  return m_pData->header.numang;
}

short CPhilipsMainHeader::slice_Thickness() const
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

float CPhilipsMainHeader::calibration_Intercept() const
{
  return m_pData->header.intcpt;
}

short CPhilipsMainHeader::injection_Time() const
{
  return m_pData->header.injtim;
}

float CPhilipsMainHeader::polygon_VertexAt0Deg() const
{
  return m_pData->header.polygonVertAt0deg;
}

short CPhilipsMainHeader::num_Slices() const
{
  return m_pData->header.nslice;
}

short CPhilipsMainHeader::num_Frames() const
{
  return m_pData->header.nframe;
}

short CPhilipsMainHeader::birthdate_Day() const
{
  return m_pData->header.bthday;
}

short CPhilipsMainHeader::birthdate_Month() const
{
  return m_pData->header.bthmo;
}

short CPhilipsMainHeader::birthdate_Year() const
{
  return m_pData->header.bthyr;
}

const char* CPhilipsMainHeader::short_Patient_ID() const
{
  return m_pData->header.ssn;
}

short CPhilipsMainHeader::num_Tilts() const
{
  return m_pData->header.ntilt;
}

void CPhilipsMainHeader::setFile_Format(const short format)
{
  m_pData->header.file_fmt = format;
}

void CPhilipsMainHeader::setScanner_Geometry(const short geometry)
{
  m_pData->header.scan_geom = geometry;
}

void CPhilipsMainHeader::setHardware_Config(const short conifg)
{
  m_pData->header.hw_config = conifg;
}

void CPhilipsMainHeader::setEdit_Flag(const short eFlag)
{
  m_pData->header.edit_flag = eFlag;
}

void CPhilipsMainHeader::setFile_Type(const File_Type fType)
{
  m_pData->header.filtyp = fType;
}

void CPhilipsMainHeader::setDay_Created(const short day)
{
  m_pData->header.dep_daycre = day;
}

void CPhilipsMainHeader::setMonth_Created(const short month)
{
  m_pData->header.dep_mocre = month;
}

void CPhilipsMainHeader::setYear_Created(const short year)
{
  m_pData->header.dep_yrcre = year;
}

void CPhilipsMainHeader::setHour_Created(const short hour)
{
  m_pData->header.dep_hrcre = hour;
}

void CPhilipsMainHeader::setMinute_Created(const short minute)
{
  m_pData->header.dep_mincre = minute;
}

void CPhilipsMainHeader::setSecond_Created(const short second)
{
  m_pData->header.dep_seccre = second;
}

void CPhilipsMainHeader::setScan_Duration(const short seconds)
{
  m_pData->header.duratn = seconds;
}

void CPhilipsMainHeader::setSubheader_Type(const Subheader_Type sType)
{
  m_pData->header.shdtyp = sType;
}

void CPhilipsMainHeader::setSingles_PreScale_Old(const short preScale)
{
  m_pData->header.sngpscl = preScale;
}

void CPhilipsMainHeader::setSingles_PreScale(const float preScale)
{
  m_pData->header.pscale = preScale;
}

void CPhilipsMainHeader::setDetector_Radius(const float radius)
{
  m_pData->header.detectorRadius = radius;
}

void CPhilipsMainHeader::setVirtual_Crystals(const bool virtualCrystals)
{
  m_pData->header.virtualXtal = virtualCrystals;
}
void CPhilipsMainHeader::setPhi_Mashing(const short phiMashing)
{
  m_pData->header.phiMashing = phiMashing;
}

void CPhilipsMainHeader::setPolygon_Sides(const short polygonSides)
{
  m_pData->header.polygonSides = polygonSides;
}

void CPhilipsMainHeader::setCrystals_Per_Side(const short crystalsPerSide)
{
  m_pData->header.xtalsPerSide = crystalsPerSide;
}

void CPhilipsMainHeader::setCrystal_Rows(const short crystalRows)
{
  m_pData->header.nXtalRows = crystalRows;
}

void CPhilipsMainHeader::setCrystal_Thickness(const float crystalThickness)
{
  m_pData->header.crystalThickness = crystalThickness;
}

void CPhilipsMainHeader::setX_CrystalPitch(const float pitch)
{
  m_pData->header.xXtalPitch = pitch;
}

void CPhilipsMainHeader::setZ_CrystalPitch(const float pitch)
{
  m_pData->header.zXtalPitch = pitch;
}

void CPhilipsMainHeader::setAxial_FOV(const float axialFOV)
{
  m_pData->header.axialFOV = axialFOV;
}

void CPhilipsMainHeader::setRPhi_Type(const RPhi_Type rType)
{
  m_pData->header.rphiType = rType;
}

void CPhilipsMainHeader::setSlice_Type(const Slice_Type sType)
{
  m_pData->header.sliceType = sType;
}

void CPhilipsMainHeader::setDelay_Type(const Delay_Type dType)
{
  m_pData->header.delayType = dType;
}

void CPhilipsMainHeader::setScan_Type(const Scan_Type sType)
{
  m_pData->header.pattyp = sType;
}

void CPhilipsMainHeader::setAcquisition_Type(const Acquisition_Type aType)
{
  m_pData->header.scntyp = aType;
}

void CPhilipsMainHeader::setNum_Ray(const short num)
{
  m_pData->header.numray = num;
}

void CPhilipsMainHeader::setNum_Ang(const short num)
{
  m_pData->header.numang = num;
}

void CPhilipsMainHeader::setSlice_Thickness(const short thickness)
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

void CPhilipsMainHeader::setCalibration_Intercept(const float intercept)
{
  m_pData->header.intcpt = intercept;
}

void CPhilipsMainHeader::setInjection_Time(const short seconds)
{
  m_pData->header.injtim = seconds;
}

void CPhilipsMainHeader::setPolygon_VertexAt0Deg(const float vertex)
{
  m_pData->header.polygonVertAt0deg = vertex;
}

void CPhilipsMainHeader::setNum_Slices(const short num)
{
  m_pData->header.nslice = num;
}

void CPhilipsMainHeader::setNum_Frames(const short num)
{
  m_pData->header.nframe = num;
}

void CPhilipsMainHeader::setBirthdate_Day(const short day)
{
  m_pData->header.bthday = day;
}

void CPhilipsMainHeader::setBirthdate_Month(const short month)
{
  m_pData->header.bthmo = month;
}

void CPhilipsMainHeader::setBirthdate_Year(const short year)
{
  m_pData->header.bthyr = year;
}

void CPhilipsMainHeader::setShort_Patient_ID(const char* id)
{
  strncpy(m_pData->header.ssn, id, 10);
}

void CPhilipsMainHeader::setNum_Tilts(const short num)
{
  m_pData->header.ntilt = num;
}
