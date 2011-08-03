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
      qint16 File_Format;       // file format (mainheader) version number
      qint16 Scanner_Geometry;  // encoding of the scanner geometry
      qint16 Hardware_Config;   // encoding of hardware used in acquisition
      qint16 Edit_Flag;         // 1 to indicat that the user has modified mainheader
      qint16 File_Type;
      qint16 Day_Created;
      qint16 Month_Created;
      qint16 Year_Created;
      qint16 Hour_Created;
      qint16 Minute_Created;
      qint16 Second_Created;
      qint16 Scan_Duration;     // duration of scan in seconds
      qint16 Subheader_Type;    // ImageIO subheader (1) or old subheader format (0)
      qint16 Singles_PreScale_Old; // singles prescale, superceded by Singles_Prescale (float)
      // qint16 Singles_Acquisition_Option; // 0=transmission only, 1=trans-ec, 2=ec only (obsoleted)
      float Singles_PreScale;      // amount by which the actual singles events is scaled down
      float Detector_Radius;       // inscribed scanner radius (from middle of opening to
                                   // detector face) in mm
      qint16 Virtual_Crystals; /* whether virtual crystals were used in the acquisition
                                  to simulate a larger detector */
      qint16 Phi_Mashing;       /* whether events from crystal phi values are combined
                                   6 Undefined/Unknown (assumption is no mashing)
                                   7 No mashing
                                   8 Events from pairs of consecutive angles have been added together */
      qint16 Polygon_Sides;    /* Number of "sides" in the scanner opening
                                  (may refer to detectors or PMT modules */
      qint16 Crystals_Per_Side; // Number of detectors per opening (polygon) side
      qint16 Crystal_Rows;      // Number of crystal rows in the axial (Z) direction
      float Crystal_Thickness;  // Detetctor crystal thickness in mm
      float X_CrystalPitch;     // X crystal pitch in mm
      float Z_CrystalPitch;     // Z crystal pitch in mm
      float Axial_FOV;          // in mm
      qint16 RPhi_Type;         // 0 Real, 1 Crystal

      qint16 Num_Frames;        // number of frames
      qint16 Num_Slices;        // number of slices per frame
      qint16 Num_Tilts;         // number of tilts per frame
    } header;
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
  stream >> m_pData->header.File_Format;      // 6: File_Format
  stream >> m_pData->header.Scanner_Geometry; // 8: Scanner_Geometry
  stream >> m_pData->header.Hardware_Config;  // 10: Hardware_Config
  stream.skipRawData(4);                      // 12: skip the next 4 bytes
  stream >> m_pData->header.Edit_Flag;        // 16: Edit_Flag
  stream.skipRawData(34);                     // 18: skip the next 34 bytes
  stream >> m_pData->header.File_Type;        // 52: File_Type
  stream.skipRawData(12);                     // 54: skip the next 12 bytes
  stream >> m_pData->header.Day_Created;      // 66: Day_Created
  stream >> m_pData->header.Month_Created;    // 68: Month_Created
  stream >> m_pData->header.Year_Created;     // 70: Year_Created
  stream >> m_pData->header.Hour_Created;     // 72: Hour_Created
  stream >> m_pData->header.Minute_Created;   // 74: Minute_Created
  stream >> m_pData->header.Second_Created;   // 76: Second_Created
  stream >> m_pData->header.Scan_Duration;    // 78: Scan_Duration
  stream >> m_pData->header.Subheader_Type;   // 80: Subheader_Type
  stream >> m_pData->header.Singles_PreScale_Old; // 82: Singles_Prescale_Old
  // stream >> m_pData->header.Singles_Acquisition_Option; // 84: Singles_Acquisition_Option
  stream.skipRawData(2);                      // 84: skip singopt (obsoleted)
  stream >> m_pData->header.Singles_PreScale; // 86: Singles_Prescale
  stream >> m_pData->header.Detector_Radius;  // 90: Detector_Radius
  stream >> m_pData->header.Virtual_Crystals; // 94: Virtual_Crystals
  stream >> m_pData->header.Phi_Mashing;      // 96: Phi_Mashing
  stream >> m_pData->header.Polygon_Sides;    // 98: Polygon_Sides
  stream >> m_pData->header.Crystals_Per_Side; // 100: Crystals_Per_Side
  stream >> m_pData->header.Crystal_Rows;      // 102: Crystal_Rows
  stream >> m_pData->header.Crystal_Thickness; // 104: Crystal_Thickness
  stream >> m_pData->header.X_CrystalPitch;    // 108: X_CrystalPitch
  stream >> m_pData->header.Z_CrystalPitch;    // 112: Z_CrystalPitch
  stream >> m_pData->header.Axial_FOV;         // 116: Axial_FOV
  stream >> m_pData->header.RPhi_Type;         // 120: RPhi_Type


#if defined(DEBUG)
  D("philips Main Header loaded:");
  D("--------------------------");
  D("File_Format:              : %d", m_pData->header.File_Format);
  D("Scanner_Geometry          : %d", m_pData->header.Scanner_Geometry);
  D("Hardware_Config           : %d", m_pData->header.Hardware_Config);
  D("Edit_Flag                 : %d", m_pData->header.Edit_Flag);
  D("File_Type                 : %d", m_pData->header.File_Type);
  D("Day_Created               : %d", m_pData->header.Day_Created);
  D("Month_Created             : %d", m_pData->header.Month_Created);
  D("Year_Created              : %d", m_pData->header.Year_Created);
  D("Hour_Created              : %d", m_pData->header.Hour_Created);
  D("Minute_Created            : %d", m_pData->header.Minute_Created);
  D("Second_Created            : %d", m_pData->header.Second_Created);
  D("Scan_Duration             : %d", m_pData->header.Scan_Duration);
  D("Subheader_Type            : %d", m_pData->header.Subheader_Type);
  D("Singles_PreScale_Old      : %d", m_pData->header.Singles_PreScale_Old);
  D("Singles_PreScale          : %f", m_pData->header.Singles_PreScale);
  D("Detector_Radius           : %f", m_pData->header.Detector_Radius);
  D("Virtual_Crystals          : %d", m_pData->header.Virtual_Crystals);
  D("Phi_Mashing               : %d", m_pData->header.Phi_Mashing);
  D("Polygon_Sides             : %d", m_pData->header.Polygon_Sides);
  D("Crystals_Per_Side         : %d", m_pData->header.Crystals_Per_Side);
  D("Crystal_Rows              : %d", m_pData->header.Crystal_Rows);
  D("Crystal_Thickness         : %f", m_pData->header.Crystal_Thickness);
  D("X_CrystalPitch            : %f", m_pData->header.X_CrystalPitch);
  D("Z_CrystalPitch            : %f", m_pData->header.Z_CrystalPitch);
  D("Axial_FOV                 : %f", m_pData->header.Axial_FOV);
  D("RPhi_Type                 : %d", m_pData->header.RPhi_Type);
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
  return CMedIOHeader::ECATMainHeader;
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
  return m_pData->header.Hardware_Config & CPhilipsMainHeaderPrivate::PETMR;
}

short CPhilipsMainHeader::file_Format() const
{
  return m_pData->header.File_Format;
}

short CPhilipsMainHeader::scanner_Geometry() const
{
  return m_pData->header.Scanner_Geometry;
}

short CPhilipsMainHeader::hardware_Config() const
{
  return m_pData->header.Hardware_Config;
}

short CPhilipsMainHeader::edit_Flag() const
{
  return m_pData->header.Edit_Flag;
}

CPhilipsMainHeader::File_Type CPhilipsMainHeader::file_Type() const
{
  return static_cast<File_Type>(m_pData->header.File_Type);
}

short CPhilipsMainHeader::day_Created() const
{
  return m_pData->header.Day_Created;
}

short CPhilipsMainHeader::month_Created() const
{
  return m_pData->header.Month_Created;
}

short CPhilipsMainHeader::year_Created() const
{
  return m_pData->header.Year_Created;
}

short CPhilipsMainHeader::hour_Created() const
{
  return m_pData->header.Hour_Created;
}

short CPhilipsMainHeader::minute_Created() const
{
  return m_pData->header.Minute_Created;
}

short CPhilipsMainHeader::second_Created() const
{
  return m_pData->header.Second_Created;
}

short CPhilipsMainHeader::scan_Duration() const
{
  return m_pData->header.Scan_Duration;
}

CPhilipsMainHeader::Subheader_Type CPhilipsMainHeader::subheader_Type() const
{
  return static_cast<Subheader_Type>(m_pData->header.Subheader_Type);
}

short CPhilipsMainHeader::singles_PreScale_Old() const
{
  return m_pData->header.Singles_PreScale_Old;
}

float CPhilipsMainHeader::singles_PreScale() const
{
  return m_pData->header.Singles_PreScale;
}

float CPhilipsMainHeader::detector_Radius() const
{
  return m_pData->header.Detector_Radius;
}

bool CPhilipsMainHeader::virtual_Crystals() const
{
  return (m_pData->header.Virtual_Crystals == 1);
}

short CPhilipsMainHeader::phi_Mashing() const
{
  return m_pData->header.Phi_Mashing;
}

short CPhilipsMainHeader::polygon_Sides() const
{
  return m_pData->header.Polygon_Sides;
}

short CPhilipsMainHeader::crystals_Per_Side() const
{
  return m_pData->header.Crystals_Per_Side;
}

short CPhilipsMainHeader::crystal_Rows() const
{
  return m_pData->header.Crystal_Rows;
}

float CPhilipsMainHeader::crystal_Thickness() const
{
  return m_pData->header.Crystal_Thickness;
}

float CPhilipsMainHeader::x_CrystalPitch() const
{
  return m_pData->header.X_CrystalPitch;
}

float CPhilipsMainHeader::z_CrystalPitch() const
{
  return m_pData->header.Z_CrystalPitch;
}

float CPhilipsMainHeader::axial_FOV() const
{
  return m_pData->header.Axial_FOV;
}

CPhilipsMainHeader::RPhi_Type CPhilipsMainHeader::rPhi_Type() const
{
  return static_cast<RPhi_Type>(m_pData->header.RPhi_Type);
}

short CPhilipsMainHeader::num_Frames() const
{
  return m_pData->header.Num_Frames;
}

short CPhilipsMainHeader::num_Slices() const
{
  return m_pData->header.Num_Slices;
}

short CPhilipsMainHeader::num_Tilts() const
{
  return m_pData->header.Num_Tilts;
}

void CPhilipsMainHeader::setFile_Format(const short format)
{
  m_pData->header.File_Format = format;
}

void CPhilipsMainHeader::setScanner_Geometry(const short geometry)
{
  m_pData->header.Scanner_Geometry = geometry;
}

void CPhilipsMainHeader::setHardware_Config(const short conifg)
{
  m_pData->header.Hardware_Config = conifg;
}

void CPhilipsMainHeader::setEdit_Flag(const short eFlag)
{
  m_pData->header.Edit_Flag = eFlag;
}

void CPhilipsMainHeader::setFile_Type(const File_Type fType)
{
  m_pData->header.File_Type = fType;
}

void CPhilipsMainHeader::setDay_Created(const short day)
{
  m_pData->header.Day_Created = day;
}

void CPhilipsMainHeader::setMonth_Created(const short month)
{
  m_pData->header.Month_Created = month;
}

void CPhilipsMainHeader::setYear_Created(const short year)
{
  m_pData->header.Year_Created = year;
}

void CPhilipsMainHeader::setHour_Created(const short hour)
{
  m_pData->header.Hour_Created = hour;
}

void CPhilipsMainHeader::setMinute_Created(const short minute)
{
  m_pData->header.Minute_Created = minute;
}

void CPhilipsMainHeader::setSecond_Created(const short second)
{
  m_pData->header.Second_Created = second;
}

void CPhilipsMainHeader::setScan_Duration(const short seconds)
{
  m_pData->header.Scan_Duration = seconds;
}

void CPhilipsMainHeader::setSubheader_Type(const Subheader_Type sType)
{
  m_pData->header.Subheader_Type = sType;
}

void CPhilipsMainHeader::setSingles_PreScale_Old(const short preScale)
{
  m_pData->header.Singles_PreScale_Old = preScale;
}

void CPhilipsMainHeader::setSingles_PreScale(const float preScale)
{
  m_pData->header.Singles_PreScale = preScale;
}

void CPhilipsMainHeader::setDetector_Radius(const float radius)
{
  m_pData->header.Detector_Radius = radius;
}

void CPhilipsMainHeader::setVirtual_Crystals(const bool virtualCrystals)
{
  m_pData->header.Virtual_Crystals = virtualCrystals;
}
void CPhilipsMainHeader::setPhi_Mashing(const short phiMashing)
{
  m_pData->header.Phi_Mashing = phiMashing;
}

void CPhilipsMainHeader::setPolygon_Sides(const short polygonSides)
{
  m_pData->header.Polygon_Sides = polygonSides;
}

void CPhilipsMainHeader::setCrystals_Per_Side(const short crystalsPerSide)
{
  m_pData->header.Crystals_Per_Side = crystalsPerSide;
}

void CPhilipsMainHeader::setCrystal_Rows(const short crystalRows)
{
  m_pData->header.Crystal_Rows = crystalRows;
}

void CPhilipsMainHeader::setCrystal_Thickness(const float crystalThickness)
{
  m_pData->header.Crystal_Thickness = crystalThickness;
}

void CPhilipsMainHeader::setX_CrystalPitch(const float pitch)
{
  m_pData->header.X_CrystalPitch = pitch;
}

void CPhilipsMainHeader::setZ_CrystalPitch(const float pitch)
{
  m_pData->header.Z_CrystalPitch = pitch;
}

void CPhilipsMainHeader::setAxial_FOV(const float axialFOV)
{
  m_pData->header.Axial_FOV = axialFOV;
}

void CPhilipsMainHeader::setRPhi_Type(const RPhi_Type rType)
{
  m_pData->header.RPhi_Type = rType;
}
