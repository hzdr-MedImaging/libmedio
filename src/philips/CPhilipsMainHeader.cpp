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
#include <QDateTime>

#include "CPhilipsMainHeader.h"
#include "CPhilipsFile.h"
#include "CPhilipsDirectory.h"
#include "CPhilipsDirectoryItem.h"
#include "CMedIOHeader.h"
#include "CMedIOData.h"
#include "CECAT7MainHeader.h"
#include "CECAT7SubHeaderImage.h"

#include <rtdebug.h>

#include "bswap.h"

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CPhilipsMainHeaderPrivate
{
public:
  // MainHeader structure (should be 512 bytes)
  #define MAINHEADER_SIZE 512
  #define MAINHEADER_VERSION 15
  #pragma pack(1)
  struct HeaderData
  {
    char magic_number[6];         // 000: magic number 1: 0x010000000016
    qint16 file_fmt;              // 006: file format (mainheader) version number. Refer to Mainheader Version.
    qint16 scan_geom;             // 008: encoding of the scanner geometry.
                                  //      0 MH_MODEL_240UP
                                  //      1 MH_MODEL_240
                                  //      2 MH_MODEL_300
                                  //      3 MH_MODEL_180
                                  //      4 MH_MODEL_CT
                                  //      5 MH_MODEL_288G - GPET
                                  //      6 MH_MODEL_432G
                                  //      7 MH_MODEL_420G - Allegro
                                  //      8 MH_MODEL_588C
                                  //      9 MH_MODEL_288M - APET
                                  //      10 MH_MODEL_420Z - Guardian
                                  //      11 MH_MODEL_432V - Vanguard
                                  //      12 MH_MODEL_420R - Raptor
                                  //      13 MH_MODEL_360S - Select (Popeye)
                                  //      14 MH_MODEL_DigitalPET - Solid Ѕtate PET
    qint16 hw_config;             // 010: Encoding of hardware used in acquisition. (Created in initSysConfig().)
    char magic2[4];               // 012: magic number 2: 0x0040005B
    qint16 edit_flag;             // 016: 1 to indicate that the if user has modified mainheader
    qint16 list_byte_ord;         // 018: Byte order of list mode raw data. Does not apply to image pixel data.
                                  //      The enumerated values are BYTEORDER_BIGENDIAN=0 and BYTEORDER_LITTLEENDIAN=1.
                                  //      Default is BYTEORDER_BIGENDIAN.
    char empty2[28];              // 020: empty
    char magic3[4];               // 048: magic number 3: 0x0007FFFF
    qint16 filtyp;                // 052: file type: 0 = FILTYP_UND = undefined
                                  //                 1 = FILTYP_SCN = sinogram (.scn)
                                  //                 2 = FILTYP_IMG = image (.img)
                                  //                 3 = FILTYP_VID
                                  //                 4 = FILTYP_OTHER
                                  //                 5 = FILTYP_SYN = syntegra (.syn)
                                  //                 6 = FILTYP_LIST = list mode (.list)
                                  //                 7 = FILTYP_RAW = raw mode (.raw)
    qint32 minTransXtalDiff;      // 054: Minimum difference in crystal x-coordinate space during all list-mode collections.
                                  //      Currently not used by recon.
    float tofTstampScale;         // 058: Time-Of-Flight time stamp unit in list events.
    char magic4[4];               // 062: magic number 4: 0x0020FFFF
    qint16 dep_daycre;            // 066: (Deprecated) Day of data acquisition. Note: These fields represent the
                                  //      date/time at which the acquisition process created the data file, not the
                                  //      time at which data was actually acquired. Reconstruction may use these as 
                                  //      a best guess at an acquisition time if other acquisition date time fields
                                  //      (e.g. acq_date_time) is not available. There is no other use of these fields,
                                  //      and applications should make use of  them any more. 
    qint16 dep_mocre;             // 068: (Deprecated) Month of data acquisition
    qint16 dep_yrcre;             // 070: (Deprecated) Year of data acquisition (four digit)
    qint16 dep_hrcre;             // 072: (Deprecated) Hour of data acquisition
    qint16 dep_mincre;            // 074: (Deprecated) Minute of data acquisition
    qint16 dep_seccre;            // 076: (Deprecated) Second of data acquisition
    qint16 duratn;                // 078: duration of scan in seconds
    qint16 shdtyp;                // 080: ImagIO sub-header (1) or original UCLA sub-header (0). (NOTE: PETView ImagIO
                                  //      library no longer supports the original UCLA sub-header.)
    qint16 sngpscl;               // 082: singles prescale, superceded by pscale float
    qint16 singopt;               // 084: Singles acquisition option (0=transmission only, 1=trans-ec, 2=ec only), obsoleted
    float pscale;                 // 086: Singles pre-scale, amount by which the actual singles events is scaled down
    float detectorRadius;         // 090: inscribed scanner radius (from middle of opening to detector face) in mm
    qint16 virtualXtal;           // 094: Whether virtual crystals were used in the acquisition to simulate a larger detector.
                                  //      0=no virtual crystals, 1=yes
    qint16 phiMashing;            // 096: Whether events from crystal phi values are combined:
                                  //      0 Undefined/Unknown (assumption is that no mashing was done)
                                  //      1 No mashing
                                  //      2 Events from pairs of consecutive angles have been added together
                                  //      List-mode: Meaningless, should be -1 (some old files may have set this field)
    qint16 polygonSides;          // 098: The number of detectors or PMT modules that form the many-sided polygon of the detector ring
    qint16 xtalsPerSide;          // 100: Number of detector crystals per opening (polygon) side in the transverse direction
    qint16 nXtalRows;             // 102: Number of crystal rows in the axial (Z) direction
    float crystalThickness;       // 104: Detector crystal thickness (in millimeters)
    float xXtalPitch;             // 108: Detector crystal pitch in the axial (Z) direction (in millimeters)
    float zXtalPitch;             // 112: Detector crystal pitch in the direction of the circumference of the opening (Y) (in millimeters)
    float axialFOV;               // 116: Useful axial length of the scanner (in millimeters)
    qint16 rphiType;              // 120: Type of R-Phi rebinning used for acquired data:
                                  //      0 Millimeters in R-Phi table
                                  //      1 Crystal-units in R-Phi table
    qint16 sliceType;             // 122: Type of slice rebinning used for acquired data:
                                  //      0 Slices in millimeters
                                  //      1 Slices in crystal sum format
                                  //      2 Slices encoded both axial (Z) values of an event
                                  //      3 Slices encode only axial (Z) value of one event (A) - for transmission
                                  //      4 Slices are SNGANG - for transmission
    qint16 delayType;             // 124: Type of delayed event used in acquiring the primary source of data:
                                  //      0 No delays recorded
                                  //      1 Delayed events were rebinned into a separate sinogram by hardware
                                  //      2 Delayed events were subtracted from the prompts sinogram in hardware
                                  //      3 Delayed events were subtracted from the prompts software
                                  //      4 The delay type is undefined
    char magic5[4];               // 126: magic number 5: 0x001BFFFF
    qint16 pattyp;                // 130: Patient-style acquisition (1) or a diagnostic-style test (0)
    qint16 scntyp;                // 132: Type of acquisition:
                                  //      0 Data not from an acquisition
                                  //      1 Blank acquisition for calibration (not used)
                                  //      2 Transmission data
                                  //      3 Emission data
                                  //      4 Normalization acquisition for calibration (not used)
                                  //      5 Image re-sliced by the brain oblique program, obl (not used)
                                  //      6 Cardiac 3D plot created by the bull’s eye program, bsi
                                  //      7 Image re-sliced by the cardiac oblique program, oblc
                                  //      8 Secondary image registered and saved by whole body display, wbd
                                  //      9 Emission contamination data
                                  //      10 Secondary cardiac image registered and saved by wbd
                                  //      11 CT data (Syntegra)
                                  //      12 CT data registered and saved by wbd (not used)
                                  //      13 MR data (Syntegra)
                                  //      14 MR data registered and saved by wbd (not used)
                                  //      15 Native sagittal images (transient, for DICOM Export)
                                  //      16 Native coronal images (transient, for DICOM Export)
    qint16 numray;                // 134: Sinograms: number of rays per projection
                                  //      Images: Sagittal (X) dimension of the matrix
                                  //      List-mode:  Meaningless, should be -1 (some old files had number of events in record).
    qint16 numang;                // 136: Sinograms: number of angles in collected data
                                  //      Images: Coronal (Y) dimension of the matrix
                                  //      List-mode: Meaningless, should be -1 (some old files had event size in bytes:  4, 8, 12, or 204).
    qint16 slcthk;                // 138: Slice thickness in millimeters
    qint16 dep_isotop;            // 140: Isotope used in:
                                  //      0 Unspecified/no isotope
                                  //      1 F-18
                                  //      2 O-15
                                  //      3 C-11
                                  //      4 Ga-68
                                  //      5 N-13
                                  //      6 Rb-82
                                  //      7 Cu-62
                                  //      8 Cs-137
                                  //      9 Ge-68
                                  //      10 Other
                                  //      11 Unknown
                                  //      12 Cu-64
                                  //      13 Br-76
                                  //      14 Na-22
                                  //      15 O-14
                                  //      16 Y-86
                                  //      17 Zn-62
                                  //      18 Cu-60
                                  //      19 Cu-61
                                  //      20 Ga-66
                                  //      21 Br-75
                                  //      22 Br-77
                                  //      23 I-124
                                  //      24 K-38
                                  //      25 Mn-52
                                  //      26 Tc-94m
                                  //      27 Ti.45
                                  //      No longer used except when reading files with file_fmt < 15. Do not add new values to
                                  //      this list
    float slope;                  // 142: Rescale slope. . nci/ml/cts/pix/min
    float intcpt;                 // 146: Offset value to data. For PET it is always 0. For AC files it is the offset due to
                                  //      hounsfield units (usually -1000 or -1024)
    qint16 dep_injtim;            // 150: (Deprecated) Time in seconds between administration of isotope and beginning of acquisition
    float polygonVertAt0deg;      // 152: Polygon vertex at zero degrees, where the polygon has nXtalModule sides
    qint16 nslice;                // 156: For reconstructed images: number of slices per frame.
                                  //      For list files: the total number of possible sinogram slices based on the detector
                                  //                      geometry. This number will vary based on the number of digital detector rings.
                                  //      For sinograms: the number will vary based on the number of digital detector rings,
                                  //                     as well as for dynamic and gated data.
    qint16 nframe;                // 158: number of frames. Acq calculates from scan length (scan length – fieldOfView) / stepSize + 1.
                                  //      Acquisition sets to the planned number initially, but updates with the number of
                                  //      frames actually acquired.
    qint16 bthday;                // 160: Day of patient birth date
    qint16 bthmo;                 // 162: Month of patient birth date
    qint16 bthyr;                 // 164: Year of patient birth date (four digit)
    char ssn[10];                 // 166: Short patient ID, superceded by Dpat_id
    qint16 ntilt;                 // 176: Number of tilts per slice (0 for images)
    qint16 petnum;                // 178: Account number on PETView File Manager/Management.
                                  //      For imported images (MR or CT) it contains the serial number of the source device
    char magic6[4];               // 180: magic number 6: 0x0021FFFF
    float activity;               // 184: Amount of isotope, in MBq, administered to patient at the time of isotope administration (injection_date_time)
    qint32 weight;                // 188: Patient weight (in grams)
    qint16 dep_hrinj;             // 192: (Deprecated) Hour of isotope administration
    qint16 dep_mininj;            // 194: (Deprecated) Minute of isotope administration
    float srcRadius;              // 196: Source radius in mm. Not used.
    float srcZpos;                // 200: Source Z position in mm. Not used.
    float halfLife;               // 204: Half-life of the isotope in minutes
                                  //      If Halflife is zero, and dep_isotop is set, a default value is assigned by the Imagio library functions
    float concfac;                // 208: SUV calibration factor; used in conjunction with dead-time factors to calculate SUVs in
                                  //      reconstruction. Units are nCi/ml/cpm/voxel
    float concfac_bgsub;          // 212: Same as concfac but accounts for the background subtraction correction applied in reconstruction.
                                  //      Units are nCi/ml/cpm/voxel
    float dmax;                   // 216: Field of view, i.e., diameter of region being imaged (in millimeters)
    float dline;                  // 220: Same as dmax. Diameter of the object being imaged (in mm)
    float angmax;                 // 224: Maximum axial acceptance angle
    float x0;                     // 228: Sagittal (X) coordinate of center of volume. Used by CTAC and save image registration in wbd; otherwise set to 0.
    float y0;                     // 232: Coronal (Y) coordinate of center of volume.  Used by CTAC and save image registration in wbd; otherwise set to 0.
    float z0;                     // 236: Transverse (Z) coordinator of center of volume.  Not used.
    float nevent;                 // 240: Number of raw events processed.  Used only by the sampling normalization generation program.
    float nsino;                  // 244: Number of prompt events collected by the acquisition. Reconstruction determines the number of prompt events that were valid
    qint16 eglob_low;             // 248: No longer used as of Ingenuity TF. Minimum energy accepted in the "global zone" around the event detection.
    qint16 eglob_up;              // 250: No longer used as of Ingenuity TF. Maximum energy accepted in the "global zone" around the event detection.
    qint16 eloc_low;              // 252: Minimum energy accepted in the "local zone" around the event detection.
    qint16 eloc_up;               // 254: Maximum energy accepted in the "local zone" around the event detection.
    qint16 orient_hf;             // 256: Head/feet first patient orientation:
                                  //      0 Undefined/Unknown
                                  //      1 Patient head is closest to gantry when the patient table is retracted
                                  //      2 Patient feet are closest to gantry when the patient table is retracted
    char scan_swrel[6];           // 258: Version of the software that acquired the data. (NOTE: The field should have a length of 7, 6 characters stored in the ImagIO file plus the null terminator.)
                                  //      This definition is kept for the ability to process pre-Ingenuity TF images.
                                  //      The other definition for this field was created for Ingenuity TF going forward. The other definition
                                  //      is a counter generated by the calibration subsystem to indicate the calibration revision level
                                  //      starting at 00001. NOTE: The field should have a length of 7, 6 characters stored in the ImagIO
                                  //      file plus the null terminator.
    qint16 petct_sepdist;         // 264: PET separation distance at acquisition time (in 1/10 millimeters)
    qint16 petct_landmrk;         // 266: Landmark position at acquisition time (in 1/10millimeters). Landmark returned from CT times 10,
                                  //      minus the axial field of view * 10, minus the separation value above.
    qint32 petct_align_timestamp; // 268: Date and time of alignment calibration (in seconds since January 1, 1970)
    qint16 tbl_direction;         // 272: Patient table direction during acquisition:
                                  //      0 Undefined/Unknown
                                  //      1 In: Data acquired while patient table moves from the front to the back of the scanner (CT->PET)
                                  //      2 Out: Data acquired while patient table moves from the back to the front of the scanner (PET->CT)
    qint16 orient_ps;             // 274: Patient orientation in relation to gravity:
                                  //      0 Undefined/Unknown
                                  //      1 Patient faces down
                                  //      2 Patient faces up
                                  //      3 Patient's left side is lower (left side down)
                                  //      4 Patient's right side is lower (right side down)
    qint16 petct_align_zoffset;   // 276: Not used at this time.
    qint16 petct_align_xshift;    // 278: Shift in horizontal (X) from the alignment calibration (in 1/100millimeters)
    qint16 petct_align_yshift;    // 280: Shift in vertical (Y) from the alignment calibration (in 1/100millimeters)
    qint16 petct_align_zshift;    // 282: Shift in axial (Z) from the alignment calibration (in 1/100millimeters)
    qint16 petct_align_acqflgs;   // 284: Not used at this time.
    qint16 petct_align_xoffset;   // 286: Horizontal (X) shift of the center of the CT field of view with respect to PET (in 1/100 millimeters)
    qint16 petct_align_yoffset;   // 288: Vertical (Y) shift of the center of the CT field of view with respect to PET (in 1/100 millimeters)
    qint16 petct_align_axrot;     // 290: Rotation about the axial (Z) axis (in 1/10,000 degrees)
    qint16 petct_align_horzrot;   // 292: Rotation about the horizontal (X) axis (in 1/10,000 degrees)
    qint16 petct_align_vertrot;   // 294: Rotation about vertical (Y) axis (in 1/10,000 degrees)
    float frontLeadDiameter;      // 296: The diameter of the front lead (Pb) in mm
    float backLeadDiameter;       // 300: The diameter of the back lead (Pb) in mm
    float leadSeparation;         // 304: The distance between the front and back lead (Pb) in mm
    float ndelays;                // 308: Number of delayed events collected by the acquisition. Reconstruction determines the number of prompt events that were valid
    float slcsep;                 // 312: Physical slice separation.  Always set to 1.0 for PETView files. 
                                  //      Set to the slice spacing (0x0018,0x0088) or, if that is not found,
                                  //      the slice thickness (0x0018,0x0050) if available by the DICOM translator.
                                  //      Value is 1 for list files.
    qint16 petct_valid;           // 316: Whether the file contains a valid PET/CT structure
    char fctrfil[20];             // 318: PMT gain factor calibration file used when the data was acquired. Not used for Vereos.
    char baselin[20];             // 338: Scanner baseline calibration file used when the data was acquired. Not used for Vereos.
    char dstpkfl[20];             // 358: Spatial distortion removal calibration name used when the data was acquired.
                                  //      NOTE: This is inadvertently no longer set.
    char dep_aqprotocol_name[20]; // 378: (Deprecated) Name of the protocol (settings) used to acquire the data. (Gemini TF).
                                  //      Deprecated for file_fmt >=15 For file_fmt >= 15 please see acqprotocol_name
    qint16 aqprotocol_type;       // 398: Type of acquisition for the primary source of data:
                                  //      0 Undefined/Unknown
                                  //      1 Static emission
                                  //      2 Dynamic emission
                                  //      3 Static transmission-only
                                  //      4 Gated cardiac
                                  //      5 Whole-Body emission
                                  //      6 Whole-Body transmission-only
                                  //      7 Not used
                                  //      8 Singles (transmission or emission contamination) in a combined emission/transmission acquisition
                                  //      9 Gated respiratory
                                  //      10 Gated combined cardiac and respiratory
    char patient_name[30];        // 400: Short patient name, superceded by Dpat_name.
    float reslice_ang1;           // 430: Rotation around the Sagittal (X) axis applied to the volume. No longer used.
    float reslice_ang2;           // 434: Rotation around the Coronal (Y) axis applied to the volume. No longer used.
    float reslice_ang3;           // 438: Rotation around the Transverse (Z) axis applied to the volume. No longer used.
    qint16 minslc;                // 442: For PET image files: recorded in mm, it is the position in the PET coordinate system.
                                  //                           It is the midpoint of the lowest numbered slice.
                                  //      For list files: always 0.
                                  //      In .syn files (e.g. ldCT) it is always 1, and is an ordinal number.
    qint16 maxslc;                // 444: For PET image files: recorded in mm, it is the position in the PET coordinate system.
                                  //                           It is the midpoint of the highest numbered slice.
                                  //      For list files: always 0 + (nslice – 1).
                                  //      In .syn files (e.g. ldCT) it is always equal to nslice (number of slices), and is an ordinal number.
    qint16 minfrm;                // 446: Lowest numbered frame
    qint16 maxfrm;                // 448: Highest numbered frame
    qint16 scanner_maxslice;      // 450: Maximum number of 1-mm slices in a static frame based on the useful axial extent of the scanner.
                                  //      (Set to sysConfig’s axial field of view.)
    char empty7[2];               // 452: empty
    qint16 rebin_type;            // 454: Method of data rebinning prior to storage in a sinogram:
                                  //      0 Single slice (no longer used)
                                  //      1 Multiple slice
                                  //      2 4D, multiple tilts
                                  //      3 Fourier rebinning
                                  //      No longer used.
    char scnOrigin[16];           // 456: Indicates origin of data;
                                  //      Prior to Solothis is a combination of the scanner serial number and site name
                                  //      ("patient" for scanners with no sites).
                                  //      For Vereos this is provided from the console as part of reconParams, and the value
                                  //      comes from information from the SIDS file on the console..
    char accNum[16];              // 472: Optional DICOM accession number.
    qint16 movementCoinc;         // 488: Length of table movement between adjacent bed positions for coincidence acquisition (in mm)
    qint16 movementSing;          // 490: Length of table movement between adjacent bed positions for singles acquisition (in mm)
    qint16 crbTstampPeriod;       // 492: Period in msec between adjacent time stamps in the List data.
                                  //      For Gemini TF the source of these timestamps is the Correction and Rebinning Board (CRB). On
                                  //      Solo the source of these timestamps is the PPC board. On Vereos the source of these timestamps is ???.
    char empty8[10];              // 494: empty
    qint16 trailexists;           // 504: (not used) 1= trailer exists, 0 = no trailer
    quint32 trailbeg;             // 506: (not used) # of bytes from file beginning indicating where the trailer begins
    char empty9[2];               // 510: empty
  } header;
  #pragma pack()

  // the extended header data structure
  #define EXTHEADER_SIZE (2336+5856) // 8K max size
  #pragma pack(1)
  struct ExtHeaderData
  {
    char Dpat_name[64];                   // 0000: DICOM patient name
    char Dpat_id[64];                     // 0064: DICOM patient ID
    char study_uid[64];                   // 0128: DICOM unique study identifier
    char series_uid[64];                  // 0192: DICOM unique series identifier
    char view_code[20];                   // 0256: DICOM view code value (0008,0100). Not used in Gemini or later.
    char sortproto_name[20];              // 0276: Sorter protocol name. No longer used as of the Rubik (3.5) release
    qint16 route;                         // 0296: An enumerated value for how the isotope is delivered:
                                          //       0 Undefined
                                          //       1 Intravenous
                                          //       2 Intra-arterial
                                          //       3 Oral
                                          //       4 By inhalation
                                          //       5 Intramuscular
                                          //       6 Subcutaneous
                                          //       7 Intracutaneous
                                          //       8 Intraperitoneal
                                          //       9 Intramedullary
                                          //       10 Intrathecal
                                          //       11 Intra-articular
                                          //       12 Intraepithelial
                                          //       13 Topical
                                          //       14 Transluminal
                                          //       15 Intraluminal
                                          //       16 Extraluminal
                                          //       17 Per rectum
                                          //       18 Vaginal route
                                          //       19 Other
    qint16 dep_pharm;                     // 0298: (Deprecated) Replaced by radiopharm_code_value,
                                          //       radiopharm_coding_scheme_designator, radiopharm_coding_scheme_version and
                                          //       radiopharm_code_meaning.
                                          //       An enumerated value for the pharmaceutical used in the study:
                                          //       0 Undefined
                                          //       1 Acetate (C11)
                                          //       2 Ammonia (N13)
                                          //       3 Carbon dioxide (O15)
                                          //       4 Carbon monoxide (C11)
                                          //       5 Carbon monoxide (O15)
                                          //       6 Carfentanil (C11)
                                          //       7 Fluorodeoxyglucose (F18)
                                          //       8 Fluoro-L-dopa (F18)
                                          //       9 Germanium (Ge68)
                                          //       10 Glutamate (N13)
                                          //       11 Methionine (C11)
                                          //       12 Oxygen (O15)
                                          //       13 Oxygen-water (O15)
                                          //       14 Palmitate (C11)
                                          //       15 Raclopride (C11)
                                          //       16 Rubidium chloride (Rb82)
                                          //       17 Sodium fluoride (F18)
                                          //       18 Sodium (Na22)
                                          //       19 Spiperone (F18)
                                          //       20 Thymidine/FLT (F18)
                                          //       21 Other – A radiopharmaceutical other than those enumerated here was used
                                          //          (and is specified in radiopharm_name).
                                          //       22 ATSM (Cu64)
                                          //       23 Butanol  (O15)
                                          //       24 EDTA (Ga68)
                                          //       25 Flumazenil (C11)
                                          //       26 Flumazenil (F18)
                                          //       27 Fluoroethylthyrosin (F18)
                                          //       28 Fluoromisonidazole (F18)
                                          //       29 Fluoromethane (F18)
                                          //       30 Fluorouracil (F18)
                                          //       31 Fluoro-benzothiazole (F18)
                                          //       32 Mespiperone (C11)
                                          //       33 Monoclonal Antibody (I124)
                                          //       34 PTSM (Cu62)
                                          //       35 Sodium Iodide (I124)
    char req_phys[64];                    // 0300: Physician who requested the study
    qint16 card_phstate;                  // 0364: Gated cardiac:  Indicates the physiologic state:
                                          //       0 Undefined/Unknown
                                          //       1 Resting
                                          //       2 Cardiac Stress
                                          //       3 Reinjection
                                          //       4 Redistribution
                                          //       5 Delayed Redistribution
                                          //       Not used.
    qint32 assay_date;                    // 0366: Date of radioactivity measurement. Note: this is currently set to zero and not used.
    qint32 assay_time;                    // 0370: Time of radioactivity measurement (seconds from midnight). Note: this is currently set to zero and not used.
    char series_desc[64];                 // 0374: Series description
                                          //       For List (raw) data that has been exported via DICOM, the number of segments that
                                          //       the list file was broken into (including associated calibration files) is appended to the
                                          //       Series Description.
                                          //       Acquisition fills in from what operator entered.
                                          //       For clinical reconstruction protocols: (may differ for service protocols)
                                          //       Reconstruction prepends series modifier entered by the user for reconstruction
                                          //       protocol.
                                          //       Reconstruction prepends an additional series descriptor that is protocol and image-
                                          //       series dependent (e.g. WB_CTAC, WB_NAC, PREVIEW, BR_CTAC, BR_NAC, etc.).
                                          //       For List files, DICOM Export appends the number of DICOM objects required to
                                          //       encapsulate the entire raw data
    qint16 height;                        // 0438: Patient height (in mm) (identical to DICOM Patient Size attribute). Filled in by acquisition.
    qint16 dep_abundance;                 // 0440: (Deprecated). Positron abundance ratio, in tenths of percent, 0 thru 1000, so 1000 is 100%. Filled in by acquisition.
                                          //       This parameter is replaced by positron_fraction.
    qint16 petct_realign_x;               // 0442: For cardiac realignment. Additional horizontal (x) shift of center of CT
                                          //       FOV w.r.t. PET (in 1/100mm).
    qint16 petct_realign_y;               // 0444: For cardiac realignment. Additional vertical (y) shift of center of CT
                                          //       FOV w.r.t. PET (in 1/100mm).
    qint16 petct_realign_hr;              // 0446: For cardiac realignment. Additional rotation about the horizontal (X) axis
                                          //       (in 1/1,000 degree).
    qint32 acq_date_time;                 // 0448: Date and time of acquisition (in seconds since Jan 1, 1970 UTC). Filled in by acquisition.
    qint32 study_date_time;               // 0452: Date and time Study was created (in seconds since Jan 1, 1970 UTC). Filled in by acquisition.
    qint32 injection_date_time;           // 0456: Date and time of isotope administration  (in seconds since Jan 1, 1970 UTC). Acquisition to fill in.
    qint32 file_create_date_time;         // 0460: Replaces daycre, mocre, yrcre, hrcre, mincre and seccre fields. Date and
                                          //       time that the acquisition file was created. Filled in by acquisition.
    qint16 resp_trig_loc;                 // 0464: Respiratory trigger location. Filled in by acquisition. Defined terms are:
                                          //       0 Unknown (RESPTRIGLOC_UNKNOWN)
                                          //       1 Max Inhalation (RESPTRIGLOC_MAX_INHALATION)
                                          //       2 Max Exhalation (RESPTRIGLOC_MAX_EXHALATION)
                                          //       3 User Defined (RESPTRIGLOC_MAX_EXHALATION)
    qint16 card_arrhythmia_rej_tech;      // 0466: The cardiac arrhythmia rejection techniques employed. Bit mask bits are defined as:
                                          //       0x0 = no rejection
                                          //       0x1 = Rejection based on deviation from average R-R interval
                                          //       0x2 = Rejection based on deviation from regular QRS loop
                                          //       0x4 = Rejection based on PVC criteria.
                                          //       Note: this is referred to as card_arrhythm in the memory map diagram above.
    float window_center;                  // 0468: DICOM display center value, i.e. a suggested center value to be used when displaying this image. Filled in by recon.
    float window_width;                   // 0472: DICOM display width value, i.e. a suggested window value to be used when displaying this image. Filled in by recon.
    qint16 petct_realign_zr;              // 0476: For cardiac realignment. Additional rotation about the axial (Z) axis
                                          //       (in 1/1,000 degree).
    qint16 petct_realign_vr;              // 0478: For cardiac realignment. Additional rotation about the vertical (Y) axis
                                          //       (in 1/1,000 degree).
    float positron_fraction;              // 0480: The radionuclide positron fraction (fraction of decays that are by positron emission).
                                          //       Replaces use of abundance parameter
    qint16 image_org;                     // 0484: This is used to indicate how the slices in this file are organized. It is set by reconstruction and read by
                                          //       DICOM applications to determine how to encode thiese image in DICOM. Values are:
                                          //       0 IORG_STATIC = slices constitute a single volume. Examples are WB images, or gated images that have been converted to dynamic or non-gated images.
                                          //       1 IORG_DYNAMIC = a Dynamic image
                                          //       2 IORG_CARDGATED = a Cardiac gated image
                                          //       3 IORG_RESPGATED = a respiratory gated image
                                          //       4 IORG_CARDRESPGATED = image that is both cardiac and respiratory gated.
                                          //       5 IORG_UNUSED = unused.
    qint16 pulmo_trig_of;                 // 0486: Pulmonary Trigger Offset: The delay in the pulmonary trigger from CT, in msec.
    qint16 resp_trig_threshold;           // 0488: Respiratory trigger threshold in percent of the chest expansion relative to the last respiratory peak. Filled in by acquisition.
                                          //       Note: this is referred to as resp_trig_thr in the memory map diagram above.
    qint16 resp_phase_duration;           // 0490: Respiratory gated:  Duration of this phase in percentage of the respiratory cycle time. Filled in by recon.
                                          //       Note: this is referred to as resp_phase_du in the memory map diagram above.
    qint16 resp_phase_offset;             // 0492: Respiratory gated: Offset from the trigger to the beginning of the acquisition in percent of the respiratory cycle time. Filled in by recon.
                                          //       Note: this is referred to as resp_phase_of in the memory map diagram above.
    qint16 petct_realign_z;               // 0494: For cardiac realignment. Additional (z) shift of center of CT
                                          //       FOV w.r.t. PET (in 1/100mm).
    qint16 window_units;                  // 0496: Indicates the units in which the window_center and window_width are specified:
                                          //       0 Undefined
                                          //       1 Counts
                                          //       2 Bq/ml
                                          //       3 % of max
                                          //       4 SUV
                                          //       5 Hounsfield
                                          //       Note: this is referred to as window_unit in the memory map diagram above
    char empty2[14];                      // 0498: empty
    char ref_phys[64];                    // 0512: Physician who referred the patient for the study
    char study_id[16];                    // 0576: DICOM study identifier
    qint16 pt_tbl_vers;                   // 0592: Patient table version.
    float Dslice_thick;                   // 0594: DICOM slice thickness. Same as slcthk for PET images
    char sex;                             // 0598: Patient sex, 'M', 'F', 'O'
    float table_height;                   // 0599: Table height in mm. Filled in for imported CT images, not used for PET images
    char empty4[1];                       // 0603: empty
    qint16 card_bt_rej;                   // 0604: Gated cardiac: Whether heart beat duration sorting has been applied (y/n).
                                          //       0=HDR_NO
                                          //       1=HDR_YES
    qint16 card_fr_type;                  // 0606: Gated cardiac: Description of type of framing performed: forward, backward, or for/backward by percentage.
                                          //       CARDFR_UNDEFINED = 0,
                                          //       CARDFR_FORWARD=1,
                                          //       CARDFR_BACKWARD=2,
                                          //       CARDFR_PERCENT=3.
    char Dmanuf_model[64];                // 0608: DICOM manufacturer’s model name of the equipment that produced the digital images; same as scanner_geom._str for PETView images.
    char Dimg_type[64];                   // 0672: DICOM image identification characteristics; for PETView, depends on scntyp:
                                          //       scntyp 0-4,9  => 'original\\primary'
                                          //       scntyp 5-8,10 => 'derived\\primary'
    float min_bed_pos;                    // 0736: Minimum bed position. Provided by acq., in PET coordinates. Defines leading edge of slice.
                                          //       Calculated as:
                                          //       if (TableDir is inward)
                                          //       {
                                          //         min_bed_pos = startPos;
                                          //         max_bed_pos = ((numFrames - 1) * stepSize) + startPos;
                                          //       }
                                          //       else
                                          //       {
                                          //         max_bed_pos = startPos;
                                          //         min_bed_pos = startPos - ((numFrames - 1) * stepSize);
                                          //       }
                                          //       The "startPos" is passed to HTA in the ScanGetReady message and is converted from CT
                                          //       coordinates to Pet/Allegro before trying to use it.
                                          //       Note that for inward scans, min_bed_pos is the same as the start position, while for outward scans
                                          //       the max_bed_pos is the same as the start position.
    float max_bed_pos;                    // 0740: Maximum bed position. Provided by acq. See min_bed_pos for explanation.
    qint16 der_filled;                    // 0744: Whether main header derived sub-header fields have been filled.
                                          //       0 DERIVED_NOTFILLED
                                          //       1 DERIVED_FILLED
    qint32 series_number;                 // 0746: DICOM series number; 1 for PETView images (only one image per series).
                                          //       This field is being ignored...series number is generated and stored in the Data-M database.
    qint32 dep_study_date;                // 0750: (Deprecated) Date study was created (in date_2_int() format)
    qint32 dep_study_time;                // 0754: (Deprecated) Time study was created (seconds from midnight)
    qint32 dep_acq_time;                  // 0758: (Deprecated) Time acquisition was created (seconds from midnight)
    qint16 card_slc_dir;                  // 0762: For Cardiac images: BASE_TO_APEX or APEX_TO_BASE.
                                          //       Not used for Gemini and beyond.
    char empty5[4];                       // 0764: empty
    qint32 card_skip_msec;                // 0768: Delay from acquisition start time to when data is actually processed, in msec
    qint32 card_skip_counts;              // 0772: Delay from acquisition start time to when data is actually processed, in counts. Not used
    qint32 card_dur_msec;                 // 0776: Duration of acquisition data to be processed following the card_skip_msec, in msec.
                                          //       A value of zero means process all data to the end of the acquired frame
    qint32 card_dur_counts;               // 0780: Duration of acquisition data to be processed following the card_skip_counts, in counts
    qint32 card_beats_tot;                // 0784: Total number of beats that occurred during the entire acquisition
    qint32 card_beats_acc;                // 0788: Total number of beats that are accepted by reconstruction
    qint16 card_skip_beats;               // 0792: Number of beats skipped after a detected arrhythmia. Filled in by recon. 
                                          //       Note: this is referred to as card_skip_b in the memory map diagram above.
    qint16 pvc_threshold;                 // 0794: When PVC rejection is employed, the percent of the r-r interval below which a heart beat is considered to be a PVC. Filled in by recon.
                                          //       Note: this is referred to as pvc_thresh in the memory map diagram above.
    char empty6[12];                      // 0796: empty
    qint32 dep_acq_date;                  // 0808: (Deprecated) Date raw data was acquired (in date_2_int() format)
    char empty7[4];                       // 0812: empty
    char radiopharm_name[64];             // 0816: Used to specify the name of the radiopharmaceutical used.
    char Dserial_number[16];              // 0880: System serial number + null
    char attncor_label[64];               // 0896: UID assigned to all series that are part of the same acquisition process.
                                          //       For example, the ldCT or MR images, and the PET list and images, that are acquired on a Gemini.
                                          //       Filled in in PET list file by the acquisition, copied to images by recon.
    char operator_name[64];               // 0960: The operator’s name, in DICOM format.
    char contr_bolus_agent[64];           // 1024: DICOM contrast bolus agent; for PETView images this is empty.
    char sop_uid[64];                     // 1088: DICOM SOP instance UID. Prior to file_fmt 11, this was the only sop_uid; post 11,
                                          //       the PETView image SOP Instance UID is stored in the sub-header and this is used only
                                          //       for some imported images.
    char frame_ref_uid[64];               // 1152: DICOM frame of reference UID
    char pps_file[30];                    // 1216: Name of the file containing DICOM Perform Procedure records (generated by DICOM library). Not used in Ingenuity TF or later.
    char worklist_file[30];               // 1246: Name of the file containing DICOM Worklist records (generated by DICOM library). No longer used in Solo and later versions.
    char empty9[4];                       // 1276: empty
    char recon_swrel[6];                  // 1280: Version of the software that produced the image from the acquired data. Not currently used.
    char analy_swrel[6];                  // 1286: Display or analysis software release (e.g. PetView). Set to 1.2 by the cardiac reslicing program, oblc. Not currently used.
    char recprotocol_name[19];            // 1292: Name of the protocol (settings) used to produce the image from the acquired data.
    char insinofile[19];                  // 1311: Primary source of data (sinogram) used to generate the image. Not used.
    qint16 slc_add;                       // 1330: Indicates whether or not data slices were combined when producing the image:
                                          //       0 No
                                          //       1 Yes
                                          //       Not used.
    qint16 slc_space;                     // 1332: The increment between the start of the group of slices that were combined, if any. Defined in units
                                          //       of slices, and applies only if slc_add is Yes.
                                          //       No Longer Used.
    qint16 slc_thick;                     // 1334: The thickness of the group of slices that were combined, if any. Defined in units of slices, and applies
                                          //       only if slc_add is Yes.
                                          //       No Longer Used.
    qint16 frame_add;                     // 1336: Whether data frames were combined when producing the image:
                                          //       0 No
                                          //       1 Yes
                                          //       No Longer Used.
    qint16 frame_space;                   // 1338: The increment between the start of the group of frames that were combined, if any.
                                          //       No Longer Used.
    qint16 frame_thick;                   // 1340: The thickness of the group of frames that were combined, if any.
                                          //       No Longer Used.
    qint16 fltr_type;                     // 1342: The filter back projection algorithm used in reconstruction:
                                          //       0 Undefined/Unknown
                                          //       1 None used
                                          //       2 Ramp
                                          //       3 Hanning
                                          //       4 Gaussian
                                          //       5 Butterworth
                                          //       No Longer Used.
    qint16 smoth;                         // 1344: Smooth factor used by the Hanning filtered back-projection.
    qint16 scatcorr_type;                 // 1346: Type of scatter correction applied:
                                          //       0 Undefined
                                          //       1 None
                                          //       2 Uniform background subtraction
                                          //       3 Non-Uniform background subtraction
                                          //       4 SS Simultaneous
                                          //       Not currently used.
    qint16 edge_exp;                      // 1348: Number of bins to expand edge in background subtraction. Not Used.
    qint16 bckang_avg;                    // 1350: Number of angles to average in background subtraction. Not Used.
    float bck_coeff;                      // 1352: Non-uniform background coefficient background subtraction. Not Used.
    qint16 bck_wid;                       // 1356: Number of bins for fitting tails radially in background subtraction. Not Used.
    qint16 attncor_type;                  // 1358: The attenuation correction algorithm used in reconstruction:
                                          //       0 Undefined/Unknown
                                          //       1 None
                                          //       2 Transmission
                                          //       3 Ellipse
                                          //       4 Read from a region of interest file
                                          //       5 Reproject
                                          //       6 CTAC
                                          //       Not Used. See attnstr.
    qint16 attncor_ecc;                   // 1360: Emission contamination correction used in attenuation correction:
                                          //       0 Undefined/Unknown
                                          //       1 None
                                          //       2 Measured
                                          //       3 Estimated
                                          //       No Longer Used.
    float attn_coeff;                     // 1362: Coefficient in attenuation correction (in centimeters). No Longer Used.
    char regfile[19];                     // 1366: Region of interest file name used in attenuation correction. No Longer Used.
    char proc_transinofile[19];           // 1385: Processed transmission sinogram file of attenuation coefficients used in attenuation correction. No Longer Used.
    float skull_comp;                     // 1404: Skull effect correction factor in attenuation correction. No Longer Used.
    qint16 norm_type;                     // 1408: The normalization type used in reconstruction:
                                          //       0 Undefined/Unknown
                                          //       1 None
                                          //       2 Axial
                                          //       3 Efficiency
                                          //       4 Axial efficiency
                                          //       No Longer Used.
    qint16 smp_norm;                      // 1410: Whether sampling pattern removal is done in normalization:
                                          //       0 No
                                          //       1 Yes
                                          //       No Longer Used.
    char axnfile[19];                     // 1412: Axial normalization file used in normalization. No Longer Used.
    char effnormfile[19];                 // 1431: Efficiency normalization file used in normalization. No Longer Used.
    qint16 gap_comp;                      // 1450: Whether gap compensation was performed in reconstruction:
                                          //       0 No
                                          //       1 Yes
                                          //       No Longer Used.
    qint16 algtype_em;                    // 1452: The emission algorithm used in reconstruction:
                                          //       0 Undefined/Unknown
                                          //       1 Filter Back Projection
                                          //       2 OSEM
                                          //       3 RAMLA
                                          //       4 3D RAMLA
                                          //       5 FBP_HAN
                                          //       6 FBP_BW
                                          //       7 FBP_3DFRP
                                          //       8 RAMLA_LOR
                                          //       See recon_str
    qint16 num_iter;                      // 1454: Number of iterations in gap compensation. No Longer Used.
    qint16 iter_em;                       // 1456: Number of OSEM iterations used in processing the emission data.
    qint16 subset_em;                     // 1458: Number of OSEM subsets used in processing the emission data.
    qint16 nsmooth_em;                    // 1460: OSEM smoothing used in processing the emission data. Not currently used.
    qint16 nrepeat_em;                    // 1462: Number of OSEM repetitions used in processing the emission data. Not currently used.
    qint16 bckslc_avg;                    // 1464: Number of slices to average in background subtraction. Not currently used.
    qint16 dead_corr;                     // 1466: Whether dead-time correction was applied in processing the data:
                                          //       0 No
                                          //       1 Yes
                                          //       Not currently used. See sub-header for a duplicate field.
    qint16 decay_corr;                    // 1468: Whether decay correction was applied in processing the data:
                                          //       0 No
                                          //       1 Yes
                                          //       Not currently used. See sub-header for a duplicate field.
    char transinofile[19];                // 1470: Transmission sinogram. Not Used.
    char blnksinofile[19];                // 1489: Blank scan calibration sinogram. Not Used.
    float tran_ray_fwhm;                  // 1508: Gaussian FWHM used for transverse smoothing in transmission data processing. Not Used.
    float tran_axl_fwhm;                  // 1512: Gaussian FWHM used for axial smoothing in transmission data processing. Not Used.
    qint16 surv_mask;                     // 1516: Number of rays to mask survival probability to 1 in transmission data processing. Not Used.
    qint16 preflt_type;                   // 1518: Pre-filter type used in reconstruction:
                                          //       0 Undefined/Unknown
                                          //       1 None
                                          //       2 Wiener
                                          //       3 Metz
                                          //       No Longer Used.
    qint16 postflt_type;                  // 1520: Post-filter type used in reconstruction:
                                          //       0 Undefined/Unknown
                                          //       1 None
                                          //       2 Wiener
                                          //       3 3D PET Filter (No Longer Used)
                                          //       4 Gaussian
    qint16 tr_posttyp;                    // 1522: Post-processing done in transmission data processing:
                                          //       0 Undefined/Unknown
                                          //       1 None
                                          //       2 Segmentation
                                          //       3 Remapping
                                          //       No Longer Used.
    qint16 algtype_tr;                    // 1524: Algorithm used in transmission data processing:
                                          //       0 Undefined/Unknown
                                          //       1 Filter Back Projection
                                          //       2 OSEM
                                          //       No Longer Used.
    qint16 iter_tr;                       // 1526: Number of OSEM iterations used in transmission data processing. No Longer Used.
    qint16 subset_tr;                     // 1528: Number of OSEM subsets used in transmission data processing. No Longer Used.
    qint16 nsmooth_tr;                    // 1530: OSEM smoothing used in transmission data processing. No Longer Used.
    qint16 nrepeat_tr;                    // 1532: Number of OSEM repetitions used in transmission data processing. No Longer Used.
    qint16 attn_corr_3d;                  // 1534: Whether 3D attenuation correction is done:
                                          //       0 No
                                          //       1 Yes
                                          //       No Longer Used.
    qint16 ramla_no_it;                   // 1536: Number of iterations in RAMLA used in emission data processing. No Longer Used.
    qint16 ramla_sysac;                   // 1538: System attenuation considered in RAMLA used in emission data processing. No Longer Used.
    float ramla_lambda[5];                // 1540: Relaxation parameters for RAMLA used in emission data processing. No Longer Used.
    float ramla_blrad;                    // 1560: Blob radius in RAMLA used in emission data processing. No Longer Used.
    float ramla_blalpha;                  // 1564: Blob alpha in RAMLA used in emission data processing. No Longer Used.
    float ramla_bcc_rsz;                  // 1568: Body centered cubic grid of relative size in RAMLA used in emission data processing. No Longer Used.
    qint32 recon_date_time;               // 1572: Date and time at which reconstruction started. Filled in by recon. in UTC
    qint16 gating_type;                   // 1576: For Respiratory gating, identifies the type of gated image. Filled in by recon.
                                          //       0 = GATINGTYPE_AVE_IP
                                          //       1 = GATINGTYPE_PULMO
                                          //       2 = GATINGTYPE_MIP
                                          //       3 = GATINGTYPE_MIN_IP
    char empty10[6];                      // 1578: empty
    char ref_attncor_series_uid[64];      // 1584: Series UID of the images used to perform attenuation correction (e.g. the ldCT images). Filled in by
                                          //       reconstruction.
                                          //       No longer consumed starting with Solo.
    char ref_gated_qc_image_inst_uid[64]; // 1648: The SOP Instance UID of the Secondary Capture image object that contains the picture of the cardiac waveform and trigger levels.
    char ref_raw_data_inst_uid[64];       // 1712: SOP Instance UID of the raw data that was used to create this image. Filled in by recon.
    qint32 start_table_pos_abs;           // 1776: Absolute start table position.
    qint32 start_table_pos_rel;           // 1780: Relative start table position.
    qint16 mr_valid;                      // 1784: Whether the file contains a valid PET/MR struct
    char empty11[6];                      // 1786: empty
    unsigned char coil_type[16];          // 1792: 16 byte fields indicating MR coils that are in position during the scan. Each contains a coil type number of the MR coil.
    char ref_raw_data_series_inst_uid[64];// 1808: Series Instance UID of the raw data that was used to create this image.
    char pet_software_version[19];        // 1872: Blanket PET software version number created for Ingenuity TF and going forward.
                                          //       (NOTE: The field should have a length of 20,19 characters stored in the ImagIO file plus the null terminator.)
    char empty12[13];                     // 1891: empty
    unsigned char num_tile_rings;         // 1904: Number of digital tile rings (z direction). Valid only when model is MH_MODEL_DigitalPET.
                                          //       Note: This is represented on the memory map in section A.2 as ntr
    unsigned char num_detector_modules;   // 1905: Number of digital detector modules. Valid only when model is MH_MODEL_DigitalPET.
                                          //       Note: is represented on the memory map in section A.2 as ndm.
                                          //       NOTE: Redundant with other fields and can be removed
    unsigned char num_tiles_per_module_row;// 1906: Number of tiles per detector module row (x direction). Valid only when model is
                                           //       MH_MODEL_DigitalPET.
                                           //       Note: This is represented on the memory map in section A.2 as ntmr
    char empty13[1];                      // 1907:
    float X_tile_gap;                     // 1908: Distance in mm between adjacent tiles in the x direction
    float Z_tile_gap;                     // 1912: Distance in mm between adjacent tiles in the z (axial) direction
    qint16 crystal_type;                  // 1916: Type of crystal
    char empty14[2];                      // 1918:
    qint16 psf1;                          // 1920: Identifies whether or not PSF1 was applied.
    qint16 psf1_iter;                     // 1922: Number of iterations in PSF1 processing. Valid only if psf1 is yes.
    float psf1_regularization;            // 1924: Regularization value used in PSF1 processing. Valid only if psf1 is yes. The value is in units of mm
                                          //       representing the full-width half-maximum of the sieve filter kernel.
    qint16 psf2;                          // 1928: Identifies whether or not PSF2 was applied.
    qint16 psf2_iter;                     // 1930: Number of iterations in PSF2 processing. Valid only if psf2 is yes.
    float gaussian_kernel;                // 1932: gaussian kernel width in mm with a range of (0, 12.0]
    char acqprotocol_name[64];            // 1936: Exam card name
    quint16 coinc_half_window_bins;       // 2000: For two events to be considered by the system as a coincidence event their timestamps must differ
                                          //       by less than this number of timing bins.
                                          //       Added for Vereos.
                                          //       Note: To convert from timing bins to an actual time value in picoseconds, use the tofTstampScale
                                          //       field, which is the time in picoseconds per timing bin
    char empty15[46];                     // 2002: empty
    char isotope_name[64];                // 2048: A free-form text-based name of the isotope.
    char isotope_code_value[16];          // 2112: DICOM Code Value identifying the isotope that was used.
    char isotope_coding_scheme_designator[16]; // 2128: DICOM Coding Scheme Designator that identifies the coding scheme from which the Code Value is taken.
    char isotope_coding_scheme_version[16];// 2144: Optional DICOM Coding Scheme Version. Identifies the version of the coding scheme
                                           //       from which the code value is taken. Required only if the value of
                                           //       isotope_coding_scheme_designator is not sufficient to identify the isotope Code
                                           //       Value.
    char isotope_code_meaning[64];        // 2160: DICOM Code Meaning. Code meaning for the isotope Code Value, as defined by
                                          //       Coding Scheme.
    char radiopharm_code_value[16];       // 2224: DICOM Code Value identifying the radiopharmaceutical that was used
    char radiopharm_coding_scheme_designator[16]; // 2240: DICOM Coding Scheme Designator that identifies the coding scheme from which the Code Value is taken
    char radiopharm_coding_scheme_version[16];// 2256: Optional DICOM Coding Scheme Version. Identifies the version of the coding scheme
                                              //       from which the code value is taken. Required only if the value of
                                              //       radiopharm_coding_scheme_designator is not sufficient to identify the radiopharm
                                              //       Code Value.
    char radiopharm_code_meaning[64];     // 2272: DICOM Code Meaning. Code meaning for the isotope Code Value, as defined by the Coding Scheme
    char reserved[5856];                  // 2336: RESERVED till 8K
  } extHeader;
  #pragma pack()

  int mainHeaderPosition;

  // convert functions
  bool ecat2philipsSex(const CECAT7MainHeader::Patient_Sex sex);
  bool ecat2philipsIsotope(const QString& isotop);
  bool ecat2philipsOrientation(const CECAT7MainHeader::Patient_Orientation eOrientation);
  bool ecat2philipsAcquisitionType(const CECAT7MainHeader::Acquisition_Type acqType);
  bool ecat2philipsPharm(const QString& radiopharmaceutial);
};

CPhilipsMainHeader::CPhilipsMainHeader(CPhilipsFile* file,
                                       CPhilipsMainHeader::File_Type fileType,
                                       int mainHeaderPosition)
  : CMedIOHeader(file)
{
  ENTER();

  m_pData = new CPhilipsMainHeaderPrivate();

  // this constructor creates a empty EPhilipsMainHeader
  // with prefilled data that is always the same for a philips main header
  clear();
  m_pData->mainHeaderPosition = mainHeaderPosition;
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

  // magic number 2 @ 012: 0x0040005B
  m_pData->header.magic2[0] = 0x00;
  m_pData->header.magic2[1] = 0x40;
  m_pData->header.magic2[2] = 0x00;
  m_pData->header.magic2[3] = 0x5B;

  // magic number 3 @ 048: 0x0007FFFF
  m_pData->header.magic3[0] = 0x00;
  m_pData->header.magic3[1] = 0x07;
  m_pData->header.magic3[2] = 0xFF;
  m_pData->header.magic3[3] = 0xFF;
  
  // magic number 4 @ 062: 0x0020FFFF
  m_pData->header.magic4[0] = 0x00;
  m_pData->header.magic4[1] = 0x20;
  m_pData->header.magic4[2] = 0xFF;
  m_pData->header.magic4[3] = 0xFF;

  // magic number 5 @ 126: 0x001BFFFF
  m_pData->header.magic5[0] = 0x00;
  m_pData->header.magic5[1] = 0x1B;
  m_pData->header.magic5[2] = 0xFF;
  m_pData->header.magic5[3] = 0xFF;

  // magic number 6 @ 180: 0x0021FFFF
  m_pData->header.magic6[0] = 0x00;
  m_pData->header.magic6[1] = 0x21;
  m_pData->header.magic6[2] = 0xFF;
  m_pData->header.magic6[3] = 0xFF;

  // set some default values for the new Main Header
  m_pData->header.file_fmt = MAINHEADER_VERSION;
  m_pData->header.shdtyp = CPhilipsMainHeader::UGM;
  m_pData->extHeader.series_number = 1; // always one for PETview images

  LEAVE();
}

bool CPhilipsMainHeader::load()
{
  ENTER();
  CMedIOData* mData = medIOData();

  // only go on if the device is readable at all
  if(mData == NULL ||
     mData->isReadable() == false ||
     mData->seek(m_pData->mainHeaderPosition) == false)
  {
    RETURN(false);
    return false;
  }

  // read in the main header in one read() operation
  ASSERT(sizeof(m_pData->header) == MAINHEADER_SIZE);
  if(mData->read(reinterpret_cast<char*>(&m_pData->header), sizeof(m_pData->header)) != MAINHEADER_SIZE)
  {
    RETURN(false);
    return false;
  }

  // now we also load the extended main header. However, we first have to query
  // for the directory item of the extended header so that we can set the file
  // to the right position.
  ASSERT(sizeof(m_pData->extHeader) == EXTHEADER_SIZE);
  CPhilipsFile* philipsFile = static_cast<CPhilipsFile*>(mData);
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
        if(mData->seek(extHeaderItem->dataBlock_Start()) == false ||
           mData->read((char*)&m_pData->extHeader, sizeof(m_pData->extHeader)) != EXTHEADER_SIZE)
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
    BSWAP_16(m_pData->header.list_byte_ord);
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
    BSWAP_16(m_pData->header.dep_isotop);
    BSWAP_FLT(m_pData->header.slope);
    BSWAP_FLT(m_pData->header.intcpt);
    BSWAP_16(m_pData->header.dep_injtim);
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
    BSWAP_16(m_pData->header.dep_hrinj);
    BSWAP_16(m_pData->header.dep_mininj);
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
    BSWAP_32(m_pData->header.petct_align_timestamp);
    BSWAP_16(m_pData->header.tbl_direction);
    BSWAP_16(m_pData->header.orient_ps);
    BSWAP_16(m_pData->header.petct_align_zoffset);
    BSWAP_16(m_pData->header.petct_align_xshift);
    BSWAP_16(m_pData->header.petct_align_yshift);
    BSWAP_16(m_pData->header.petct_align_zshift);
    BSWAP_16(m_pData->header.petct_align_acqflgs);
    BSWAP_16(m_pData->header.petct_align_xoffset);
    BSWAP_16(m_pData->header.petct_align_yoffset);
    BSWAP_16(m_pData->header.petct_align_axrot);
    BSWAP_16(m_pData->header.petct_align_horzrot);
    BSWAP_16(m_pData->header.petct_align_vertrot);
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
      BSWAP_16(m_pData->extHeader.dep_pharm);
      BSWAP_16(m_pData->extHeader.card_phstate);
      BSWAP_32(m_pData->extHeader.assay_date);
      BSWAP_32(m_pData->extHeader.assay_time);
      BSWAP_16(m_pData->extHeader.height);   
      BSWAP_16(m_pData->extHeader.dep_abundance);
      BSWAP_16(m_pData->extHeader.petct_realign_x);
      BSWAP_16(m_pData->extHeader.petct_realign_y);
      BSWAP_16(m_pData->extHeader.petct_realign_hr);
      BSWAP_32(m_pData->extHeader.acq_date_time);
      BSWAP_32(m_pData->extHeader.study_date_time);
      BSWAP_32(m_pData->extHeader.injection_date_time);
      BSWAP_32(m_pData->extHeader.file_create_date_time);
      BSWAP_16(m_pData->extHeader.resp_trig_loc);
      BSWAP_16(m_pData->extHeader.card_arrhythmia_rej_tech);
      BSWAP_FLT(m_pData->extHeader.window_center);
      BSWAP_FLT(m_pData->extHeader.window_width);
      BSWAP_16(m_pData->extHeader.petct_realign_zr);
      BSWAP_16(m_pData->extHeader.petct_realign_vr);
      BSWAP_FLT(m_pData->extHeader.positron_fraction);
      BSWAP_16(m_pData->extHeader.image_org);
      BSWAP_16(m_pData->extHeader.pulmo_trig_of);
      BSWAP_16(m_pData->extHeader.resp_trig_threshold);
      BSWAP_16(m_pData->extHeader.resp_phase_duration);
      BSWAP_16(m_pData->extHeader.resp_phase_offset);
      BSWAP_16(m_pData->extHeader.petct_realign_z);
      BSWAP_16(m_pData->extHeader.window_units);
      BSWAP_16(m_pData->extHeader.pt_tbl_vers);
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
      BSWAP_FLT(m_pData->extHeader.X_tile_gap);
      BSWAP_FLT(m_pData->extHeader.Z_tile_gap);
      BSWAP_16(m_pData->extHeader.crystal_type);
      BSWAP_16(m_pData->extHeader.psf1);
      BSWAP_16(m_pData->extHeader.psf1_iter);
      BSWAP_FLT(m_pData->extHeader.psf1_regularization);
      BSWAP_16(m_pData->extHeader.psf2);
      BSWAP_16(m_pData->extHeader.psf2_iter);
      BSWAP_FLT(m_pData->extHeader.gaussian_kernel);
      BSWAP_16(m_pData->extHeader.coinc_half_window_bins);
    }
  }
  
#if defined(DEBUG)
  D("philips Main Header loaded:");
  D("--------------------------");
  D("file_fmt             : %d", file_fmt());
  D("scan_geom            : %d", scan_geom());
  D("hw_config            : %d", hw_config());
  D("edit_flag            : %d", edit_flag());
  D("filtyp               : %d", filtyp());
  D("list_byte_ord        : %d", list_byte_ord());
  D("minTransXtalDiff     : %d", minTransXtalDiff());
  D("tofTstampScale       : %f", tofTstampScale());
  D("dep_daycre           : %d", dep_daycre());
  D("dep_mocre            : %d", dep_mocre());
  D("dep_yrcre            : %d", dep_yrcre());
  D("dep_hrcre            : %d", dep_hrcre());
  D("dep_mincre           : %d", dep_mincre());
  D("dep_seccre           : %d", dep_seccre());
  D("duratn               : %d", duratn());
  D("shdtyp               : %d", shdtyp());
  D("sngpscl              : %d", sngpscl());
  D("singopt              : %d", singopt());
  D("pscale               : %d", pscale());
  D("detectorRadius       : %f", detectorRadius());
  D("virtualXtal          : %d", virtualXtal());
  D("phiMashing           : %d", phiMashing());
  D("polygonSides         : %d", polygonSides());
  D("xtalsPerSide         : %d", xtalsPerSide());
  D("nXtalRows            : %d", nXtalRows());
  D("crystalThickness     : %f", crystalThickness());
  D("xXtalPitch           : %f", xXtalPitch());
  D("zXtalPitch           : %f", zXtalPitch());
  D("axialFov             : %f", axialFov());
  D("rphiType             : %d", rphiType());
  D("sliceType            : %d", sliceType());
  D("delayType            : %d", delayType());
  D("pattyp               : %d", pattyp());
  D("scntyp               : %d", scntyp());
  D("numray               : %d", numray());
  D("numang               : %d", numang());
  D("slcthk               : %d", slcthk());
  D("dep_isotop           : %d", dep_isotop());
  D("slope                : %f", slope());
  D("intcpt               : %f", intcpt());
  D("dep_injtim           : %d", dep_injtim());
  D("polygonVertAt0deg    : %f", polygonVertAt0deg());
  D("nslice               : %d", nslice());
  D("nframe               : %d", nframe());
  D("bthday               : %d", bthday());
  D("bthmo                : %d", bthmo());
  D("bthyr                : %d", bthyr());
  D("ssn                  : '%s'", ssn());
  D("ntilt                : %d", ntilt());
  D("petnum               : %d", petnum());
  D("activity             : %f", activity());
  D("weight               : %d", weight());
  D("dep_hrinj            : %d", dep_hrinj());
  D("dep_mininj           : %d", dep_mininj());
  D("srcRadius            : %f", srcRadius());
  D("srcZpos              : %f", srcZpos());
  D("halfLife             : %f", halfLife());
  D("concfac              : %f", concfac());
  D("concfac_Bgsub        : %f", concfac_Bgsub());
  D("dmax                 : %f", dmax());
  D("dline                : %f", dline());
  D("angmax               : %f", angmax());
  D("x0                   : %f", x0());
  D("y0                   : %f", y0());
  D("z0                   : %f", z0());
  D("nevent               : %f", nevent());
  D("nsino                : %f", nsino());
  D("eglob_low            : %d", eglob_low());
  D("eglob_up             : %d", eglob_up());
  D("eloc_low             : %d", eloc_low());
  D("eloc_up              : %d", eloc_up());
  D("orient_hf            : %d", orient_hf());
  D("scan_swrel           : '%s'", scan_swrel());
  D("petct_sepdist        : %d", petct_sepdist());
  D("petct_landmrk        : %d", petct_landmrk());
  D("petct_align_timestamp: %s", QDateTime::fromTime_t(petct_align_timestamp()).toString().toLatin1().constData());
  D("tbl_direction        : %d", tbl_direction());
  D("orient_ps            : %d", orient_ps());
  D("petct_align_zoffset  : %d", petct_align_zoffset());
  D("petct_align_xshift   : %d", petct_align_xshift());
  D("petct_align_yshift   : %d", petct_align_yshift());
  D("petct_align_zshift   : %d", petct_align_zshift());
  D("petct_align_acqflgs  : %d", petct_align_acqflgs());
  D("petct_align_xoffset  : %d", petct_align_xoffset());
  D("petct_align_yoffset  : %d", petct_align_yoffset());
  D("petct_align_axrot    : %d", petct_align_axrot());
  D("petct_align_horzrot  : %d", petct_align_horzrot());
  D("petct_align_vertrot  : %d", petct_align_vertrot());
  D("frontLeadDiameter    : %f", frontLeadDiameter());
  D("backLeadDiameter     : %f", backLeadDiameter());
  D("leadSeparation       : %f", leadSeparation());
  D("ndelays              : %f", ndelays());
  D("slcsep               : %f", slcsep());
  D("petct_valid          : %d", petct_valid());
  D("fctrfil              : '%s'", fctrfil());
  D("baselin              : '%s'", baselin());
  D("dstpkfl              : '%s'", dstpkfl());
  D("dep_aqprotocol_name  : '%s'", dep_aqprotocol_name());
  D("aqprotocol_type      : %d", aqprotocol_type());
  D("patient_name         : '%s'", patient_name());
  D("reslice_ang1         : %f", reslice_ang1());
  D("reslice_ang2         : %f", reslice_ang2());
  D("reslice_ang3         : %f", reslice_ang3());
  D("minslc               : %d", minslc());
  D("maxslc               : %d", maxslc());
  D("minfrm               : %d", minfrm());
  D("maxfrm               : %d", maxfrm());
  D("scanner_maxslice     : %d", scanner_maxslice());
  D("rebin_type           : %d", rebin_type());
  D("scnOrigin            : '%s'", scnOrigin());
  D("accNum               : '%s'", accNum());
  D("movementCoinc        : %d", movementCoinc());
  D("movementSing         : %d", movementSing());
  D("crbTstampPeriod      : %d", crbTstampPeriod());
  D("trailexists          : %d", trailexists());
  D("trailbeg             : %d", trailbeg());

  if(extendedHeaderFound == true)
  {
    D("philips Extended Main Header:");
    D("----------------------------");
    D("Dpat_name                  : '%s'", Dpat_name());
    D("Dpat_id                    : '%s'", Dpat_id());
    D("study_uid                  : '%s'", study_uid());
    D("series_uid                 : '%s'", series_uid());
    D("view_code                  : '%s'", view_code());
    D("sortproto_name             : '%s'", sortproto_name());
    D("route                      : %d", route());
    D("dep_pharm                  : %d", dep_pharm());
    D("req_phys                   : '%s'", req_phys());
    D("card_phstate               : %d", card_phstate());
    D("assay_date                 : %d", assay_date());
    D("assay_time                 : %d", assay_time());
    D("series_desc                : '%s'", series_desc());
    D("height                     : %d", height());
    D("dep_abundance              : %d", dep_abundance());
    D("petct_realign_x            : %d", petct_realign_x());
    D("petct_realign_y            : %d", petct_realign_y());
    D("petct_realign_hr           : %d", petct_realign_hr());
    D("acq_date_time              : %s", QDateTime::fromTime_t(acq_date_time()).toString().toLatin1().constData());
    D("study_date_time            : %s", QDateTime::fromTime_t(study_date_time()).toString().toLatin1().constData());
    D("injection_date_time        : %s", QDateTime::fromTime_t(injection_date_time()).toString().toLatin1().constData());
    D("file_create_date_time      : %s", QDateTime::fromTime_t(file_create_date_time()).toString().toLatin1().constData());
    D("resp_trig_loc              : %d", resp_trig_loc());
    D("card_arrhythmia_rej_tech   : %d", card_arrhythmia_rej_tech());
    D("window_center              : %f", window_center());
    D("window_width               : %f", window_width());
    D("petct_realign_zr           : %d", petct_realign_zr());
    D("petct_realign_vr           : %d", petct_realign_vr());
    D("positron_fraction          : %f", positron_fraction());
    D("image_org                  : %d", image_org());
    D("pulmo_trig_of              : %d", pulmo_trig_of());
    D("resp_trig_threshold        : %d", resp_trig_threshold());
    D("resp_phase_duration        : %d", resp_phase_duration());
    D("resp_phase_offset          : %d", resp_phase_offset());
    D("petct_realign_z            : %d", petct_realign_z());
    D("window_units               : %d", window_units());
    D("ref_phys                   : '%s'", ref_phys());
    D("study_id                   : '%s'", study_id());
    D("pt_tbl_vers                : %d", pt_tbl_vers());
    D("Dslice_thick               : %f", Dslice_thick());
    D("sex                        : %c", sex());
    D("table_height               : %f", table_height());
    D("card_bt_rej                : %d", card_bt_rej());
    D("card_fr_type               : %d", card_fr_type());
    D("Dmanuf_model               : '%s'", Dmanuf_model());
    D("Dimg_type                  : '%s'", Dimg_type());
    D("min_bed_pos                : %f", min_bed_pos());
    D("max_bed_pos                : %f", max_bed_pos());
    D("der_filled                 : %d", der_filled());
    D("series_number              : %d", series_number());
    D("dep_study_date             : %d", dep_study_date());
    D("dep_study_time             : %d", dep_study_time());
    D("dep_acq_time               : %d", dep_acq_time());
    D("card_slc_dir               : %d", card_slc_dir());
    D("card_skip_msec             : %d", card_skip_msec());
    D("card_skip_counts           : %d", card_skip_counts());
    D("card_dur_msec              : %d", card_dur_msec());
    D("card_dur_counts            : %d", card_dur_counts());
    D("card_beats_tot             : %d", card_beats_tot());
    D("card_beats_acc             : %d", card_beats_acc());
    D("card_skip_beats            : %d", card_skip_beats());
    D("pvc_threshold              : %d", pvc_threshold());
    D("dep_acq_date               : %d", dep_acq_date());
    D("radiopharm_name            : '%s'", radiopharm_name());
    D("Dserial_number             : '%s'", Dserial_number());
    D("attncor_label              : '%s'", attncor_label());
    D("operator_name              : '%s'", operator_name());
    D("contr_bolus_agent          : '%s'", contr_bolus_agent());
    D("sop_uid                    : '%s'", sop_uid());
    D("frame_ref_uid              : '%s'", frame_ref_uid());
    D("pps_file                   : '%s'", pps_file());
    D("worklist_file              : '%s'", worklist_file());
    D("recon_swrel                : '%s'", recon_swrel());
    D("analy_swrel                : '%s'", analy_swrel());
    D("recprotocol_name           : '%s'", recprotocol_name());
    D("insinofile                 : '%s'", insinofile());
    D("slc_add                    : %d", slc_add());
    D("slc_space                  : %d", slc_space());
    D("slc_thick                  : %d", slc_thick());
    D("frame_add                  : %d", frame_add());
    D("frame_space                : %d", frame_space());
    D("frame_thick                : %d", frame_thick());
    D("fltr_type                  : %d", fltr_type());
    D("smoth                      : %d", smoth());
    D("scatcorr_type              : %d", scatcorr_type());
    D("edge_exp                   : %d", edge_exp());
    D("bckang_avg                 : %d", bckang_avg());
    D("bck_coeff                  : %f", bck_coeff());
    D("bck_wid                    : %d", bck_wid());
    D("attncor_type               : %d", attncor_type());
    D("attncor_ecc                : %d", attncor_ecc());
    D("attn_coeff                 : %f", attn_coeff());
    D("regfile                    : '%s'", regfile());
    D("proc_transinofile          : '%s'", proc_transinofile());
    D("skull_comp                 : %f", skull_comp());
    D("norm_type                  : %d", norm_type());
    D("smp_norm                   : %d", smp_norm());
    D("axnfile                    : '%s'", axnfile());
    D("effnormfile                : '%s'", effnormfile());
    D("gap_comp                   : %d", gap_comp());
    D("algtype_em                 : %d", algtype_em());
    D("num_iter                   : %d", num_iter());
    D("iter_em                    : %d", iter_em());
    D("subset_em                  : %d", subset_em());
    D("nsmooth_em                 : %d", nsmooth_em());
    D("nrepeat_em                 : %d", nrepeat_em());
    D("bckslc_avg                 : %d", bckslc_avg());
    D("dead_corr                  : %d", dead_corr());
    D("decay_corr                 : %d", decay_corr());
    D("transinofile               : '%s'", transinofile());
    D("blnksinofile               : '%s'", blnksinofile());
    D("tran_ray_fwhm              : %f", tran_ray_fwhm());
    D("tran_axl_fwhm              : %f", tran_axl_fwhm());
    D("surv_mask                  : %d", surv_mask());
    D("preflt_type                : %d", preflt_type());
    D("postflt_type               : %d", postflt_type());
    D("tr_posttyp                 : %d", tr_posttyp());
    D("algtype_tr                 : %d", algtype_tr());
    D("iter_tr                    : %d", iter_tr());
    D("subset_tr                  : %d", subset_tr());
    D("nsmooth_tr                 : %d", nsmooth_tr());
    D("nrepeat_tr                 : %d", nrepeat_tr());
    D("attn_corr_3d               : %d", attn_corr_3d());
    D("ramla_no_it                : %d", ramla_no_it());
    D("ramla_sysac                : %d", ramla_sysac());
    D("ramla_lambda[0]            : %f", ramla_lambda(0));
    D("ramla_lambda[1]            : %f", ramla_lambda(1));
    D("ramla_lambda[2]            : %f", ramla_lambda(2));
    D("ramla_lambda[3]            : %f", ramla_lambda(3));
    D("ramla_lambda[4]            : %f", ramla_lambda(4));
    D("ramla_blrad                : %f", ramla_blrad());
    D("ramla_blalpha              : %f", ramla_blalpha());
    D("ramla_bcc_rsz              : %f", ramla_bcc_rsz());
    D("recon_date_time            : %s", QDateTime::fromTime_t(recon_date_time()).toString().toLatin1().constData());
    D("gating_type                : %d", gating_type());
    D("ref_attncor_series_uid     : '%s'", ref_attncor_series_uid());
    D("ref_gated_qc_image_inst_uid: '%s'", ref_gated_qc_image_inst_uid());
    D("ref_raw_data_inst_uid      : '%s'", ref_raw_data_inst_uid());
    D("start_table_pos_abs        : %d", start_table_pos_abs());
    D("start_table_pos_rel        : %d", start_table_pos_rel());
    D("mr_valid                   : %d", mr_valid());
    D("coil_type[0]               : %d", coil_type(0));
    D("coil_type[1]               : %d", coil_type(1));
    D("coil_type[2]               : %d", coil_type(2));
    D("coil_type[3]               : %d", coil_type(3));
    D("coil_type[4]               : %d", coil_type(4));
    D("coil_type[5]               : %d", coil_type(5));
    D("coil_type[6]               : %d", coil_type(6));
    D("coil_type[7]               : %d", coil_type(7));
    D("coil_type[8]               : %d", coil_type(8));
    D("coil_type[9]               : %d", coil_type(9));
    D("coil_type[10]              : %d", coil_type(10));
    D("coil_type[11]              : %d", coil_type(11));
    D("coil_type[12]              : %d", coil_type(12));
    D("coil_type[13]              : %d", coil_type(13));
    D("coil_type[14]              : %d", coil_type(14));
    D("ref_raw_data_series_inst_uid: '%s'", ref_raw_data_series_inst_uid());
    D("pet_software_version       : '%s'", pet_software_version());
    D("num_tile_rings             : %c", num_tile_rings());
    D("num_detector_modules       : %c", num_detector_modules());
    D("num_tiles_per_module_row   : %c", num_tiles_per_module_row());
    D("X_tile_gap                 : %f", X_tile_gap());
    D("Z_tile_gap                 : %f", Z_tile_gap());
    D("crystal_type               : %d", crystal_type());
    D("psf1                       : %d", psf1());
    D("psf1_iter                  : %d", psf1_iter());
    D("psf1_regularization        : %f", psf1_regularization());
    D("psf2                       : %d", psf2());
    D("psf2_iter                  : %d", psf2_iter());
    D("gaussian_kernel            : %f", gaussian_kernel());
    D("acqprotocol_name           : '%s'", acqprotocol_name());
    D("coinc_half_window_bins     : %d", coinc_half_window_bins());
    D("isotope_name               : '%s'", isotope_name());
    D("isotope_code_value         : '%s'", isotope_code_value());
    D("isotope_coding_scheme_designator: '%s'", isotope_coding_scheme_designator());
    D("isotope_coding_scheme_version   : '%s'", isotope_coding_scheme_version());
    D("isotope_code_meaning            : '%s'", isotope_code_meaning());
    D("radiopharm_code_value           : '%s'", radiopharm_code_value());
    D("radiopharm_coding_scheme_designator: '%s'", radiopharm_coding_scheme_designator());
    D("radiopharm_coding_scheme_version   : '%s'", radiopharm_coding_scheme_version());
    D("radiopharm_code_meaning            : '%s'", radiopharm_code_meaning());
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
  CMedIOData* mData = medIOData();

  SHOWPOINTER(mData);

  // only go on if the device is writeable at all
  if(mData == NULL ||
     mData->isWritable() == false ||
     mData->seek(0) == false)
  {
    RETURN(false);
    return false;
  }

  D("about to write main header");
  SHOWVALUE(mData->pos());

  // check that the sizes are absolutely correct
  ASSERT(sizeof(m_pData->header) == MAINHEADER_SIZE);
  ASSERT(sizeof(m_pData->extHeader) == EXTHEADER_SIZE);

  // before we can start reading out some data we have to collect some
  // out data beforehand which we use instead of the data stored in our
  // data structure (such as frames/slices/tilts etc.)
  CPhilipsFile* philipsFile = static_cast<CPhilipsFile*>(mData);
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
    BSWAP_16(header->list_byte_ord);
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
    BSWAP_16(header->dep_isotop);
    BSWAP_FLT(header->slope);
    BSWAP_FLT(header->intcpt);
    BSWAP_16(header->dep_injtim);
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
    BSWAP_16(header->dep_hrinj);
    BSWAP_16(header->dep_mininj);
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
    BSWAP_32(header->petct_align_timestamp);
    BSWAP_16(header->tbl_direction);
    BSWAP_16(header->orient_ps);
    BSWAP_16(header->petct_align_zoffset);
    BSWAP_16(header->petct_align_xshift);
    BSWAP_16(header->petct_align_yshift);
    BSWAP_16(header->petct_align_zshift);
    BSWAP_16(header->petct_align_acqflgs);
    BSWAP_16(header->petct_align_xoffset);
    BSWAP_16(header->petct_align_yoffset);
    BSWAP_16(header->petct_align_axrot);
    BSWAP_16(header->petct_align_horzrot);
    BSWAP_16(header->petct_align_vertrot);
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
    BSWAP_16(extHeader->dep_pharm);
    BSWAP_16(extHeader->card_phstate);
    BSWAP_32(extHeader->assay_date);
    BSWAP_32(extHeader->assay_time);
    BSWAP_16(extHeader->height);   
    BSWAP_16(extHeader->dep_abundance);
    BSWAP_16(extHeader->petct_realign_x);
    BSWAP_16(extHeader->petct_realign_y);
    BSWAP_16(extHeader->petct_realign_hr);
    BSWAP_32(extHeader->acq_date_time);
    BSWAP_32(extHeader->study_date_time);
    BSWAP_32(extHeader->injection_date_time);
    BSWAP_32(extHeader->file_create_date_time);
    BSWAP_16(extHeader->resp_trig_loc);
    BSWAP_16(extHeader->card_arrhythmia_rej_tech);
    BSWAP_FLT(extHeader->window_center);
    BSWAP_FLT(extHeader->window_width);
    BSWAP_16(extHeader->petct_realign_zr);
    BSWAP_16(extHeader->petct_realign_vr);
    BSWAP_FLT(extHeader->positron_fraction);
    BSWAP_16(extHeader->resp_trig_threshold);
    BSWAP_16(extHeader->resp_phase_duration);
    BSWAP_16(extHeader->resp_phase_offset);
    BSWAP_16(extHeader->petct_realign_z);
    BSWAP_16(extHeader->window_units);
    BSWAP_16(extHeader->pt_tbl_vers);
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
    BSWAP_FLT(extHeader->X_tile_gap);
    BSWAP_FLT(extHeader->Z_tile_gap);
    BSWAP_16(extHeader->crystal_type);
    BSWAP_16(extHeader->psf1);
    BSWAP_16(extHeader->psf1_iter);
    BSWAP_FLT(extHeader->psf1_regularization);
    BSWAP_16(extHeader->psf2);
    BSWAP_16(extHeader->psf2_iter);
    BSWAP_FLT(extHeader->gaussian_kernel);
    BSWAP_16(extHeader->coinc_half_window_bins);
  }
  else
  {
    header = &m_pData->header;
    extHeader = &m_pData->extHeader;
  }

  // now write out the main header to the file
  if(mData->write(reinterpret_cast<char*>(header), sizeof(m_pData->header)) == MAINHEADER_SIZE)
  {
    // now we have to check/create the extended main header part which
    // is part of the directory
    CPhilipsFile* phFile = static_cast<CPhilipsFile*>(mData);
    CPhilipsDirectory* philipsDirectory = phFile->directory();
    if(philipsDirectory != NULL)
    {
      CPhilipsDirectoryItem* extHeaderItem = philipsDirectory->extendedMainHeaderItem();
      if(extHeaderItem != NULL)
      {
        D("about to write extended main header");
        SHOWVALUE(mData->pos());

        // now we have the extendedheader diritem. so lets get the header out of it
        if(mData->seek(extHeaderItem->dataBlock_Start()) == true &&
           mData->write(reinterpret_cast<char*>(extHeader), sizeof(m_pData->extHeader)) == EXTHEADER_SIZE)
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

bool CPhilipsMainHeader::convertFrom(const CMedIOHeader* mainHeader, const CMedIOHeader* subHeader) 
{
  ENTER();
  bool bResult = false;

  // depending on the MedIOHeader format we do have to 
  // distinguish between our copy operations.
  switch(mainHeader->headerFormat())
  {
    case CMedIOHeader::ECATMainHeader:
    {
      const CECATMainHeader* eMainHeader = static_cast<const CECATMainHeader*>(mainHeader);

      // depending on the source type we have to copy either every data or just 
      // some data of the src header
      switch(eMainHeader->mainHeaderType())
      {
        case CECATMainHeader::ECAT7MainHeader:
        {
          const CECAT7MainHeader* header = static_cast<const CECAT7MainHeader*>(mainHeader);
          const CECAT7SubHeaderImage* sh = static_cast<const CECAT7SubHeaderImage*>(subHeader);

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

            case CECAT7MainHeader::AttenuationCorr:
            case CECAT7MainHeader::Normalization:
            case CECAT7MainHeader::PolarMap:
            case CECAT7MainHeader::Projection8:
            case CECAT7MainHeader::Projection16:
            case CECAT7MainHeader::Normalization_3D:
              E("unsupported main header type %d", header->file_Type());
            break;

            case CECAT7MainHeader::Unknown:
              fileType = CPhilipsMainHeader::Unknown;
            break;
          }

          setFiltyp(fileType);

          // convert the patient name (convert 'FIRSTNAME, LASTNAME' to 'LASTNAME FIRSTNAME'
          QString patientName = QString::fromLatin1(header->patient_Name());
          if(patientName.contains(','))
          {
            // split firstname/lastname
            QStringList splitted = patientName.split(',');
            
            if(splitted.size() == 2)
            {
              QString newPatientName = splitted[1].trimmed() + " " + splitted[0].trimmed();
              setPatient_name(newPatientName.toLatin1().constData());
            }
            else
            setPatient_name(header->patient_Name());
          }
          else
            setPatient_name(header->patient_Name());

          // if sex is selected as unknown in the ecat header we
          // set the patient type to test (phantom) in Philips
          if(header->patient_Sex() == CECAT7MainHeader::Sex_Unknown)
            setPattyp(CPhilipsMainHeader::Test);
          else
            setPattyp(CPhilipsMainHeader::Patient);

          // set some static information for which info in the
          // ECAT format is missing
          setShdtyp(CPhilipsMainHeader::UGM);
          setRoute(CPhilipsMainHeader::Intravenous);
          setAngmax(10.0f);

          setDuratn(sh->frame_Duration() / 1000); // ms -> s
          setSsn(header->patient_ID());
          setWeight(header->patient_Weight() * 1000.0f); // kg -> g
          setPatient_Birth_Date_Qt(header->patient_Birth_Date_Qt());
          m_pData->ecat2philipsOrientation(header->patient_Orientation());
          setNslice(header->num_Planes());
          setNframe(header->num_Frames());
          setSlcthk(header->plane_Separation() * 10.0f); // cm -> mm
          setSlcsep(1.0f); // always 1.0 for PETview files
          setDslice_thick(header->plane_Separation() * 10.0f); // cm -> mm
          m_pData->ecat2philipsIsotope(header->isotope_Name());
          setHalfLife(header->isotope_Halflife() / 60.0f); // sec -> min
          setActivity(header->dosage() / 1000000.0f); // Bq -> MBq
          setDep_Aqprotocol_name(header->study_Type());
          m_pData->ecat2philipsAcquisitionType(header->acquisition_Type());
          setScnOrigin(header->facility_Name());

          setDpat_name(QString::fromLatin1(header->patient_Name()).replace(", ", "^").toLatin1().constData());
          setSeries_desc(header->study_Description());
          setInjection_date_time_Qt(header->dose_Start_Time_Qt());
          setAcq_date_time(header->scan_Start_Time());
          setRadiopharm_name(header->radiopharmaceutical());
          setDserial_number(header->serial_Number());
          setScan_geom(header->system_Type());

          // depending on the scan_geom and operator_Name (which we reused)
          // we try to identify the right scanner type
          if(scan_geom() == 97)
          {
            setDmanuf_model("Ingenuity TF PET/MR");
            setHw_config(6769);
            setMr_valid(true);
            setPetct_valid(CPhilipsMainHeader::PETCT_Valid);
          }
          else
          {
            QString operatorName = header->operator_Name();

            if(operatorName.contains("PET/CT", Qt::CaseInsensitive))
            {
              setPetct_valid(CPhilipsMainHeader::PETCT_Valid);
              setDmanuf_model(header->operator_Name());
            }
            else if(operatorName.contains("MR", Qt::CaseInsensitive))
            {
              setMr_valid(true);
              setPetct_valid(CPhilipsMainHeader::PETCT_Valid);
              setDmanuf_model(header->operator_Name());
            }
            else if(operatorName.contains("PET", Qt::CaseInsensitive))
              setDmanuf_model(header->operator_Name());
          }

          // check if cti_Reserved() is set to mr_valid mode
          if(header->cti_Reserved(0) == 0x6d72) // hex-code for 'mr', if set mr_valid can be extracted
          {
            setMr_valid(true);
            setCoil_type(0, header->cti_Reserved(1));
            setCoil_type(1, header->cti_Reserved(2));
            setCoil_type(2, header->cti_Reserved(3));
            setCoil_type(3, header->cti_Reserved(4));
            setCoil_type(4, header->cti_Reserved(5));
          }

          m_pData->ecat2philipsPharm(header->radiopharmaceutical());
          m_pData->ecat2philipsSex(header->patient_Sex());
          setDpat_id(header->patient_ID());
          setHeight(header->patient_Height() * 10.0f); // cm -> mm
          setDep_abundance(header->branching_Fraction() * 1000.0f); // 1.0 = 100%
          setPositron_fraction(header->branching_Fraction());
          setRef_phys(header->physician_Name());
          setTable_height(header->bed_Elevation());
         
          // convert scan start time to file create and acq datetime
          setAcq_date_time_Qt(header->scan_Start_Time_Qt());
          setStudy_date_time_Qt(header->scan_Start_Time_Qt());
          setFile_create_date_time_Qt(header->scan_Start_Time_Qt());

          // set the table direction based on the bed offset being negative or positive
          if(header->bed_Offset(0) != 0)
            setTbl_direction(header->bed_Offset(0) < 0 ? CPhilipsMainHeader::Out : CPhilipsMainHeader::In);
          else 
            setTbl_direction(header->init_Bed_Position() < 0 ? CPhilipsMainHeader::Out : CPhilipsMainHeader::In);

          switch(tbl_direction())
          {
            case CPhilipsMainHeader::Out:
              setMax_bed_pos(header->init_Bed_Position() * 10.0f); // cm -> mm
            break;

            case CPhilipsMainHeader::In:
              setMin_bed_pos(header->init_Bed_Position() * 10.0f); // cm -> mm
            break;

            case CPhilipsMainHeader::UnknownDirection:
              // nothing
            break;
          }

          setAxialFov(header->distance_Scanned() * 10.0f); // cm -> mm
          setScanner_maxslice(header->distance_Scanned() * 10.0f); // cm -> mm
          setDmax(header->transaxial_FOV() * 10.0f); // cm -> mm
          setDline(header->transaxial_FOV() * 10.0f); // cm -> mm
          setPhiMashing(header->angular_Compression() == CECAT7MainHeader::Mash2 ? CPhilipsMainHeader::Mashing2 : CPhilipsMainHeader::NoMashing);
          setNumray(sh->x_Dimension());
          setNumang(sh->y_Dimension());

          bResult = true;
        }
        break;

        case CECATMainHeader::ECAT6MainHeader:
          E("unsupported main header copy");
        break;

        case CECATMainHeader::UnknownHeaderType:
          // nothing
        break;
      }
    }
    break;

    case CMedIOHeader::ECATSubHeader:
    case CMedIOHeader::PhilipsSubHeader:
      // copying a sub header into a main header doesn't make much sense, so we
      // do nothing here
    break;
   
    case CMedIOHeader::ConcordeMicroPetMainHeader:
    case CMedIOHeader::ConcordeMicroPetFrameHeader:
    case CMedIOHeader::PhilipsMainHeader:
    case CMedIOHeader::PhilipsListviewHeader:
    {
      E("medio mainheader %d conversion not implemented!", mainHeader->headerFormat());
    }
    break;

    case CMedIOHeader::Unknown:
      // for an unknown header type we do nothing
    break;
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

short CPhilipsMainHeader::file_fmt() const
{
  return m_pData->header.file_fmt;
}

short CPhilipsMainHeader::scan_geom() const
{
  return m_pData->header.scan_geom;
}

short CPhilipsMainHeader::hw_config() const
{
  return m_pData->header.hw_config;
}

short CPhilipsMainHeader::edit_flag() const
{
  return m_pData->header.edit_flag;
}

CPhilipsMainHeader::File_Type CPhilipsMainHeader::filtyp() const
{
  return static_cast<File_Type>(m_pData->header.filtyp);
}

CPhilipsMainHeader::Byte_Order CPhilipsMainHeader::list_byte_ord() const
{
  return static_cast<Byte_Order>(m_pData->header.list_byte_ord);
}

int CPhilipsMainHeader::minTransXtalDiff() const
{
  return m_pData->header.minTransXtalDiff;
}

float CPhilipsMainHeader::tofTstampScale() const
{
  return m_pData->header.tofTstampScale;
}

short CPhilipsMainHeader::dep_daycre() const
{
  return m_pData->header.dep_daycre;
}

short CPhilipsMainHeader::dep_mocre() const
{
  return m_pData->header.dep_mocre;
}

short CPhilipsMainHeader::dep_yrcre() const
{
  return m_pData->header.dep_yrcre;
}

short CPhilipsMainHeader::dep_hrcre() const
{
  return m_pData->header.dep_hrcre;
}

short CPhilipsMainHeader::dep_mincre() const
{
  return m_pData->header.dep_mincre;
}

short CPhilipsMainHeader::dep_seccre() const
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

short CPhilipsMainHeader::sngpscl() const
{
  return m_pData->header.sngpscl;
}

short CPhilipsMainHeader::singopt() const
{
  return m_pData->header.singopt;
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

CPhilipsMainHeader::Mashing_Type CPhilipsMainHeader::phiMashing() const
{
  return static_cast<CPhilipsMainHeader::Mashing_Type>(m_pData->header.phiMashing);
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

float CPhilipsMainHeader::axialFov() const
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

CPhilipsMainHeader::Isotope CPhilipsMainHeader::dep_isotop() const
{
  return static_cast<Isotope>(m_pData->header.dep_isotop);
}

float CPhilipsMainHeader::slope() const
{
  return m_pData->header.slope;
}

float CPhilipsMainHeader::intcpt() const
{
  return m_pData->header.intcpt;
}

short CPhilipsMainHeader::dep_injtim() const
{
  return m_pData->header.dep_injtim;
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

int CPhilipsMainHeader::weight() const
{
  return m_pData->header.weight;
}

short CPhilipsMainHeader::dep_hrinj() const
{
  return m_pData->header.dep_hrinj;
}

short CPhilipsMainHeader::dep_mininj() const
{
  return m_pData->header.dep_mininj;
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

short CPhilipsMainHeader::eglob_low() const
{
  return m_pData->header.eglob_low;
}

short CPhilipsMainHeader::eglob_up() const
{
  return m_pData->header.eglob_up;
}

short CPhilipsMainHeader::eloc_low() const
{
  return m_pData->header.eloc_low;
}

short CPhilipsMainHeader::eloc_up() const
{
  return m_pData->header.eloc_up;
}

CPhilipsMainHeader::Patient_Orientation_hf CPhilipsMainHeader::orient_hf() const
{
  return static_cast<Patient_Orientation_hf>(m_pData->header.orient_hf);
}

const char* CPhilipsMainHeader::scan_swrel() const
{
  return m_pData->header.scan_swrel;
}

short CPhilipsMainHeader::petct_sepdist() const
{
  return m_pData->header.petct_sepdist;
}

short CPhilipsMainHeader::petct_landmrk() const
{
  return m_pData->header.petct_landmrk;
}

time_t CPhilipsMainHeader::petct_align_timestamp() const
{
  return m_pData->header.petct_align_timestamp;
}

CPhilipsMainHeader::Table_Direction CPhilipsMainHeader::tbl_direction() const
{
  return static_cast<Table_Direction>(m_pData->header.tbl_direction);
}

CPhilipsMainHeader::Patient_Orientation_ps CPhilipsMainHeader::orient_ps() const
{
  return static_cast<Patient_Orientation_ps>(m_pData->header.orient_ps);
}

short CPhilipsMainHeader::petct_align_zoffset() const
{
  return m_pData->header.petct_align_zoffset;
}

short CPhilipsMainHeader::petct_align_xshift() const
{
  return m_pData->header.petct_align_xshift;
}

short CPhilipsMainHeader::petct_align_yshift() const
{
  return m_pData->header.petct_align_yshift;
}

short CPhilipsMainHeader::petct_align_zshift() const
{
  return m_pData->header.petct_align_zshift;
}

short CPhilipsMainHeader::petct_align_acqflgs() const
{
  return m_pData->header.petct_align_acqflgs;
}

short CPhilipsMainHeader::petct_align_xoffset() const
{
  return m_pData->header.petct_align_xoffset;
}

short CPhilipsMainHeader::petct_align_yoffset() const
{
  return m_pData->header.petct_align_yoffset;
}

short CPhilipsMainHeader::petct_align_axrot() const
{
  return m_pData->header.petct_align_axrot;
}

short CPhilipsMainHeader::petct_align_horzrot() const
{
  return m_pData->header.petct_align_horzrot;
}

short CPhilipsMainHeader::petct_align_vertrot() const
{
  return m_pData->header.petct_align_vertrot;
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

const char* CPhilipsMainHeader::dep_aqprotocol_name() const
{
  return m_pData->header.dep_aqprotocol_name;
}

CPhilipsMainHeader::Acquisition_Protocol_Type CPhilipsMainHeader::aqprotocol_type() const
{
  return static_cast<Acquisition_Protocol_Type>(m_pData->header.aqprotocol_type);
}

const char* CPhilipsMainHeader::patient_name() const
{
  return m_pData->header.patient_name;
}

float CPhilipsMainHeader::reslice_ang1() const
{
  return m_pData->header.reslice_ang1;
}

float CPhilipsMainHeader::reslice_ang2() const
{
  return m_pData->header.reslice_ang2;
}

float CPhilipsMainHeader::reslice_ang3() const
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

short CPhilipsMainHeader::scanner_maxslice() const
{
  return m_pData->header.scanner_maxslice;
}

CPhilipsMainHeader::Rebin_Type CPhilipsMainHeader::rebin_type() const
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

bool CPhilipsMainHeader::trailexists() const
{
  return m_pData->header.trailexists == 1;
}

unsigned int CPhilipsMainHeader::trailbeg() const
{
  return m_pData->header.trailbeg;
}

CPhilipsMainHeader::PETCT_Valid_Type CPhilipsMainHeader::petct_valid() const
{
  return static_cast<PETCT_Valid_Type>(m_pData->header.petct_valid);
}

void CPhilipsMainHeader::setFile_fmt(const short format)
{
  m_pData->header.file_fmt = format;
}

void CPhilipsMainHeader::setScan_geom(const short geometry)
{
  m_pData->header.scan_geom = geometry;
}

void CPhilipsMainHeader::setHw_config(const short conifg)
{
  m_pData->header.hw_config = conifg;
}

void CPhilipsMainHeader::setEdit_flag(const short eFlag)
{
  m_pData->header.edit_flag = eFlag;
}

void CPhilipsMainHeader::setFiltyp(const File_Type fType)
{
  m_pData->header.filtyp = fType;
}

void CPhilipsMainHeader::setList_byte_ord(const Byte_Order iOrder)
{
  m_pData->header.list_byte_ord = iOrder;
}

void CPhilipsMainHeader::setMinTransXtalDiff(const int minTransXtalDiff)
{
  m_pData->header.minTransXtalDiff = minTransXtalDiff;
}

void CPhilipsMainHeader::setTofTstampScale(const float tofTstampScale)
{
  m_pData->header.tofTstampScale = tofTstampScale;
}

void CPhilipsMainHeader::setDep_daycre(const short daycre)
{
  m_pData->header.dep_daycre = daycre;
}

void CPhilipsMainHeader::setDep_mocre(const short mocre)
{
  m_pData->header.dep_mocre = mocre;
}

void CPhilipsMainHeader::setDep_yrcre(const short yrcre)
{
  m_pData->header.dep_yrcre = yrcre;
}

void CPhilipsMainHeader::setDep_hrcre(const short hrcre)
{
  m_pData->header.dep_hrcre = hrcre;
}

void CPhilipsMainHeader::setDep_mincre(const short mincre)
{
  m_pData->header.dep_mincre = mincre;
}

void CPhilipsMainHeader::setDep_seccre(const short seccre)
{
  m_pData->header.dep_seccre = seccre;
}

void CPhilipsMainHeader::setDuratn(const short seconds)
{
  m_pData->header.duratn = seconds;
}

void CPhilipsMainHeader::setShdtyp(const Subheader_Type sType)
{
  m_pData->header.shdtyp = sType;
}

void CPhilipsMainHeader::setSngpscl(const short sngpscl)
{
  m_pData->header.sngpscl = sngpscl;
}

void CPhilipsMainHeader::setSingopt(const short singopt)
{
  m_pData->header.singopt = singopt;
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
  m_pData->header.virtualXtal = (virtualCrystals == true) ? 1 : 0;
}

void CPhilipsMainHeader::setPhiMashing(const CPhilipsMainHeader::Mashing_Type phiMashing)
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

void CPhilipsMainHeader::setNXtalRows(const short crystalRows)
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

void CPhilipsMainHeader::setAxialFov(const float axialFOV)
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

void CPhilipsMainHeader::setDep_isotop(const Isotope isotop)
{
  m_pData->header.dep_isotop = isotop;
}

void CPhilipsMainHeader::setSlope(const float slope)
{
  m_pData->header.slope = slope;
}

void CPhilipsMainHeader::setIntcpt(const float intercept)
{
  m_pData->header.intcpt = intercept;
}

void CPhilipsMainHeader::setDep_injtim(const short seconds)
{
  m_pData->header.dep_injtim = seconds;
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
  strncpy(m_pData->header.ssn, id, sizeof(m_pData->header.ssn)-1);
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

void CPhilipsMainHeader::setWeight(const int weight)
{
  m_pData->header.weight = weight;
}

void CPhilipsMainHeader::setDep_hrinj(const short hrinj)
{
  m_pData->header.dep_hrinj = hrinj;
}

void CPhilipsMainHeader::setDep_mininj(const short mininj)
{
  m_pData->header.dep_mininj = mininj;
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

void CPhilipsMainHeader::setEglob_low(const short eglob_low)
{
  m_pData->header.eglob_low = eglob_low;
}

void CPhilipsMainHeader::setEglob_up(const short eglob_up)
{
  m_pData->header.eglob_up = eglob_up;
}

void CPhilipsMainHeader::setEloc_low(const short eloc_low)
{
  m_pData->header.eloc_low = eloc_low;
}

void CPhilipsMainHeader::setEloc_up(const short eloc_up)
{
  m_pData->header.eloc_up = eloc_up;
}

void CPhilipsMainHeader::setOrient_hf(const CPhilipsMainHeader::Patient_Orientation_hf orient_hf)
{
  m_pData->header.orient_hf = static_cast<qint16>(orient_hf);
}

void CPhilipsMainHeader::setScan_swrel(const char* str)
{
  strncpy(m_pData->header.scan_swrel, str, sizeof(m_pData->header.scan_swrel)-1);
}

void CPhilipsMainHeader::setPetct_sepdist(const short sepdist)
{
  m_pData->header.petct_sepdist = sepdist;
}

void CPhilipsMainHeader::setPetct_landmrk(const short landmrk)
{
  m_pData->header.petct_landmrk = landmrk;
}

void CPhilipsMainHeader::setPetct_align_timestamp(const time_t tstamp)
{
  m_pData->header.petct_align_timestamp = tstamp;
}

void CPhilipsMainHeader::setTbl_direction(const CPhilipsMainHeader::Table_Direction tbl_direction)
{
  m_pData->header.tbl_direction = static_cast<qint16>(tbl_direction);
}

void CPhilipsMainHeader::setOrient_ps(const CPhilipsMainHeader::Patient_Orientation_ps orient_ps)
{
  m_pData->header.orient_ps = static_cast<qint16>(orient_ps);
}

void CPhilipsMainHeader::setPetct_align_zoffset(const short zOffset)
{
  m_pData->header.petct_align_zoffset = zOffset;
}

void CPhilipsMainHeader::setPetct_align_xshift(const short xShift)
{
  m_pData->header.petct_align_xshift = xShift;
}

void CPhilipsMainHeader::setPetct_align_yshift(const short yShift)
{
  m_pData->header.petct_align_yshift = yShift;
}

void CPhilipsMainHeader::setPetct_align_zshift(const short zShift)
{
  m_pData->header.petct_align_zshift = zShift;
}

void CPhilipsMainHeader::setPetct_align_acqflgs(const short acqFlags)
{
  m_pData->header.petct_align_acqflgs = acqFlags;
}

void CPhilipsMainHeader::setPetct_align_xoffset(const short xOffset)
{
  m_pData->header.petct_align_xoffset = xOffset;
}

void CPhilipsMainHeader::setPetct_align_yoffset(const short yOffset)
{
  m_pData->header.petct_align_yoffset = yOffset;
}

void CPhilipsMainHeader::setPetct_align_axrot(const short axialRotation)
{
  m_pData->header.petct_align_axrot = axialRotation;
}

void CPhilipsMainHeader::setPetct_align_horzrot(const short horizRotation)
{
  m_pData->header.petct_align_horzrot = horizRotation;
}

void CPhilipsMainHeader::setPetct_align_vertrot(const short vertRotation)
{
  m_pData->header.petct_align_vertrot = vertRotation;
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

void CPhilipsMainHeader::setPetct_valid(const CPhilipsMainHeader::PETCT_Valid_Type petct_valid)
{
  m_pData->header.petct_valid = static_cast<qint16>(petct_valid);
}

void CPhilipsMainHeader::setFctrfil(const char* str)
{
  strncpy(m_pData->header.fctrfil, str, sizeof(m_pData->header.fctrfil)-1);
}

void CPhilipsMainHeader::setBaselin(const char* str)
{
  strncpy(m_pData->header.baselin, str, sizeof(m_pData->header.baselin)-1);
}

void CPhilipsMainHeader::setDstpkfl(const char* str)
{
  strncpy(m_pData->header.dstpkfl, str, sizeof(m_pData->header.dstpkfl)-1);
}

void CPhilipsMainHeader::setDep_Aqprotocol_name(const char* str)
{
  strncpy(m_pData->header.dep_aqprotocol_name, str, sizeof(m_pData->header.dep_aqprotocol_name)-1);
}

void CPhilipsMainHeader::setAqprotocol_type(const CPhilipsMainHeader::Acquisition_Protocol_Type aqprotocol_type)
{
  m_pData->header.aqprotocol_type =  static_cast<qint16>(aqprotocol_type);
}

void CPhilipsMainHeader::setPatient_name(const char* str)
{
  strncpy(m_pData->header.patient_name, str, sizeof(m_pData->header.patient_name)-1);
}

void CPhilipsMainHeader::setReslice_ang1(const float reslice_ang1)
{
  m_pData->header.reslice_ang1 = reslice_ang1;
}

void CPhilipsMainHeader::setReslice_ang2(const float reslice_ang2)
{
  m_pData->header.reslice_ang2 = reslice_ang2;
}

void CPhilipsMainHeader::setReslice_ang3(const float reslice_ang3)
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

void CPhilipsMainHeader::setScanner_maxslice(const short scanner_maxslice)
{
  m_pData->header.scanner_maxslice = scanner_maxslice;
}

void CPhilipsMainHeader::setRebin_type(const Rebin_Type rebin_type)
{
  m_pData->header.rebin_type = rebin_type;
}

void CPhilipsMainHeader::setScnOrigin(const char *str)
{
  strncpy(m_pData->header.scnOrigin, str, sizeof(m_pData->header.scnOrigin)-1);
}

void CPhilipsMainHeader::setAccNum(const char *str)
{
  strncpy(m_pData->header.accNum, str, sizeof(m_pData->header.accNum)-1);
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

void CPhilipsMainHeader::setTrailbeg(const unsigned int trailbeg)
{
  m_pData->header.trailbeg = trailbeg;
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

QDateTime CPhilipsMainHeader::petct_align_timestamp_Qt() const
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  QDateTime datetime = Jan1970.addSecs(m_pData->header.petct_align_timestamp);
  return datetime.toLocalTime();
}

void CPhilipsMainHeader::setPetct_align_timestamp_Qt(const QDateTime& datetime)
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  setPetct_align_timestamp(Jan1970.secsTo(datetime.toUTC()));
}

QDateTime CPhilipsMainHeader::acq_date_time_Qt() const
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  QDateTime datetime = Jan1970.addSecs(m_pData->extHeader.acq_date_time);
  return datetime.toLocalTime();
}

void CPhilipsMainHeader::setAcq_date_time_Qt(const QDateTime& datetime)
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  setAcq_date_time(Jan1970.secsTo(datetime.toUTC()));
}

QDateTime CPhilipsMainHeader::study_date_time_Qt() const
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  QDateTime datetime = Jan1970.addSecs(m_pData->extHeader.study_date_time);
  return datetime.toLocalTime();
}

void CPhilipsMainHeader::setStudy_date_time_Qt(const QDateTime& datetime)
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  setStudy_date_time(Jan1970.secsTo(datetime.toUTC()));
}

QDateTime CPhilipsMainHeader::injection_date_time_Qt() const
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  QDateTime datetime = Jan1970.addSecs(m_pData->extHeader.injection_date_time);
  return datetime.toLocalTime();
}

void CPhilipsMainHeader::setInjection_date_time_Qt(const QDateTime& datetime)
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  setInjection_date_time(Jan1970.secsTo(datetime.toUTC()));
}

QDateTime CPhilipsMainHeader::file_create_date_time_Qt() const
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  QDateTime datetime = Jan1970.addSecs(m_pData->extHeader.file_create_date_time);
  return datetime.toLocalTime();
}

void CPhilipsMainHeader::setFile_create_date_time_Qt(const QDateTime& datetime)
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  setFile_create_date_time(Jan1970.secsTo(datetime.toUTC()));
}

QDateTime CPhilipsMainHeader::recon_date_time_Qt() const
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  QDateTime datetime = Jan1970.addSecs(m_pData->extHeader.recon_date_time);
  return datetime.toLocalTime();
}

void CPhilipsMainHeader::setRecon_date_time_Qt(const QDateTime& datetime)
{
  QDateTime Jan1970(QDate(1970, 1, 1), QTime(), Qt::UTC);
  setRecon_date_time(Jan1970.secsTo(datetime.toUTC()));
}

bool CPhilipsMainHeaderPrivate::ecat2philipsSex(const CECAT7MainHeader::Patient_Sex sex)
{
  ENTER();
  bool result = false;

  switch(sex)
  {
    case CECAT7MainHeader::Sex_Male:
      extHeader.sex = 'M';
    break;

    case CECAT7MainHeader::Sex_Female:
      extHeader.sex = 'F';
    break;

    case CECAT7MainHeader::Sex_Unknown:
      extHeader.sex = 'O';
    break;
 }

  RETURN(result);
  return result;
}

bool CPhilipsMainHeaderPrivate::ecat2philipsIsotope(const QString& isotop)
{
  ENTER();
  bool result = false;

  QString isotope = isotop.toUpper().remove(QChar('-'));
  if(isotope.contains("UNDEFINED"))
    header.dep_isotop = CPhilipsMainHeader::UndefinedIsotope;
  else if(isotope.contains("UNKNOWN"))
    header.dep_isotop = CPhilipsMainHeader::UnknownIsotope;
  else if(isotope.contains("F18"))
    header.dep_isotop = CPhilipsMainHeader::F18;
  else if(isotope.contains("O15"))
    header.dep_isotop = CPhilipsMainHeader::O15;
  else if(isotope.contains("C11"))
    header.dep_isotop = CPhilipsMainHeader::C11;
  else if(isotope.contains("GA68"))
    header.dep_isotop = CPhilipsMainHeader::GA68;
  else if(isotope.contains("N13"))
    header.dep_isotop = CPhilipsMainHeader::N13;
  else if(isotope.contains("RB82"))
    header.dep_isotop = CPhilipsMainHeader::RB82;
  else if(isotope.contains("CU62"))
    header.dep_isotop = CPhilipsMainHeader::CU62;
  else if(isotope.contains("CS137"))
    header.dep_isotop = CPhilipsMainHeader::CS137;
  else if(isotope.contains("GE68"))
    header.dep_isotop = CPhilipsMainHeader::GE68;
  else if(isotope.contains("CU64"))
    header.dep_isotop = CPhilipsMainHeader::CU64;
  else if(isotope.contains("BR76"))
    header.dep_isotop = CPhilipsMainHeader::BR76;
  else if(isotope.contains("NA22"))
    header.dep_isotop = CPhilipsMainHeader::NA22;
  else if(isotope.contains("O14"))
    header.dep_isotop = CPhilipsMainHeader::O14;
  else if(isotope.contains("Y86"))
    header.dep_isotop = CPhilipsMainHeader::Y86;
  else if(isotope.contains("ZN62"))
    header.dep_isotop = CPhilipsMainHeader::ZN62;
  else if(isotope.contains("CU60"))
    header.dep_isotop = CPhilipsMainHeader::CU60;
  else if(isotope.contains("CU61"))
    header.dep_isotop = CPhilipsMainHeader::CU61;
  else if(isotope.contains("GA66"))
    header.dep_isotop = CPhilipsMainHeader::GA66;
  else if(isotope.contains("BR75"))
    header.dep_isotop = CPhilipsMainHeader::BR75;
  else if(isotope.contains("BR77"))
    header.dep_isotop = CPhilipsMainHeader::BR77;
  else if(isotope.contains("I124"))
    header.dep_isotop = CPhilipsMainHeader::I124;
  else if(isotope.contains("K38"))
    header.dep_isotop = CPhilipsMainHeader::K38;
  else if(isotop.contains("MN52"))
    header.dep_isotop = CPhilipsMainHeader::MN52;
  else if(isotop.contains("TC94M"))
    header.dep_isotop = CPhilipsMainHeader::TC94M;
  else if(isotop.contains("TI45"))
    header.dep_isotop = CPhilipsMainHeader::TI45;
  else
    header.dep_isotop = CPhilipsMainHeader::OtherIsotope;

  strncpy(extHeader.isotope_name, isotop.toLatin1().constData(), sizeof(extHeader.isotope_name)-1);

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
  QString dimg_type = "original\\primary";

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
  strncpy(extHeader.Dimg_type, dimg_type.toLatin1().constData(), sizeof(extHeader.Dimg_type)-1);

  RETURN(result);
  return result;
}

bool CPhilipsMainHeaderPrivate::ecat2philipsPharm(const QString& rpharm)
{
  ENTER();
  bool result = false;
  CPhilipsMainHeader::Pharm_Type pharmType = CPhilipsMainHeader::UndefinedPharm;

  // the ECAT format only knows the isotope and radiopharmaceutical. thus we need to
  // search in the radiopharmaceutical description for the right pharm code used in the
  // philips format.
  if(rpharm.contains("FDG", Qt::CaseInsensitive) ||
     rpharm.contains("glucose", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Fluorodeoxyglucose_F18;
  }
  else if(rpharm.contains("DOPA", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Fluoro_L_Dopa_F18;
  }
  else if(rpharm.contains("Acetate", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Acetate_C11;
  }
  else if(rpharm.contains("Ammonia", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Ammonia_N13;
  }
  else if(rpharm.contains("CO2", Qt::CaseInsensitive) ||
          (rpharm.contains("Carbon", Qt::CaseInsensitive) &&
           rpharm.contains("Dioxide", Qt::CaseInsensitive)))
  {
    pharmType = CPhilipsMainHeader::Carbon_Dioxide_O15;
  }
  else if(rpharm.contains("CO") ||
          (rpharm.contains("Carbon", Qt::CaseInsensitive) &&
           rpharm.contains("Monoxide", Qt::CaseInsensitive)))
  {
    if(rpharm.contains("C11", Qt::CaseInsensitive) ||
       rpharm.contains("C-11", Qt::CaseInsensitive))
    {
      pharmType = CPhilipsMainHeader::Carbon_Monoxide_C11;
    }
    else if(rpharm.contains("O15", Qt::CaseInsensitive) ||
            rpharm.contains("O-15", Qt::CaseInsensitive))
    {
      pharmType = CPhilipsMainHeader::Carbon_Monoxide_O15;
    }
    else
      pharmType = CPhilipsMainHeader::OtherPharm;
  }
  else if(rpharm.contains("Carfentanil", Qt::CaseInsensitive) ||
          rpharm.contains("CFN", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Carfentanil_C11;
  }
  else if(rpharm.contains("Germanium", Qt::CaseInsensitive) ||
          rpharm.contains("GE-68", Qt::CaseInsensitive) ||
          rpharm.contains("GE68", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Germanium_GE68;
  }
  else if(rpharm.contains("Glutamate", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Glutamate_N13;
  }
  else if(rpharm.contains("Methionine", Qt::CaseInsensitive) ||
          rpharm.contains("MET", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Methionine_C11;
  }
  else if(rpharm.contains("H2O", Qt::CaseInsensitive) ||
          (rpharm.contains("Oxygen", Qt::CaseInsensitive) &&
           rpharm.contains("Water", Qt::CaseInsensitive)))
  {
    pharmType = CPhilipsMainHeader::Oxygen_Water_O15;
  }
  else if(rpharm.contains("Oxygen", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Oxygen_O15;
  }
  else if(rpharm.contains("Palmitate", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Palmitate_C11;
  }
  else if(rpharm.contains("Raclopride", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Raclopride_C11;
  }
  else if(rpharm.contains("Rubidium", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Rubidium_Chloride_RB82;
  }
  else if(rpharm.contains("NaF", Qt::CaseInsensitive) ||
          (rpharm.contains("Sodium", Qt::CaseInsensitive) &&
           rpharm.contains("Fluoride", Qt::CaseInsensitive)))
  {
    pharmType = CPhilipsMainHeader::Sodium_Fluoride_F18;
  }
  else if(rpharm.contains("Sodium", Qt::CaseInsensitive))
  {
    if(rpharm.contains("Iodide"))
      pharmType = CPhilipsMainHeader::Sodium_Iodide_I124;
    else
      pharmType = CPhilipsMainHeader::Sodium_NA22;
  }
  else if(rpharm.contains("Spiperone", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Spiperone_F18;
  }
  else if(rpharm.contains("Thymidine", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Thymidine_F18;
  }
  else if(rpharm.contains("Atsm", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Atsm_CU64;
  }
  else if(rpharm.contains("Butanol", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Butanol_O15;
  }
  else if(rpharm.contains("Edta", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Edta_GA68;
  }
  else if(rpharm.contains("Flumazenil", Qt::CaseInsensitive))
  {
    if(rpharm.contains("C11", Qt::CaseInsensitive) ||
       rpharm.contains("C-11", Qt::CaseInsensitive))
    {
      pharmType = CPhilipsMainHeader::Flumazenil_C11;
    }
    else if(rpharm.contains("F18", Qt::CaseInsensitive) ||
            rpharm.contains("F-18", Qt::CaseInsensitive))
    {
      pharmType = CPhilipsMainHeader::Flumazenil_F18;
    }
    else
      pharmType = CPhilipsMainHeader::OtherPharm;
  }
  else if(rpharm.contains("Fluoroethyltyrosin", Qt::CaseInsensitive) ||
          rpharm.contains("FET", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Fluoroethyltyrosin_F18;
  }
  else if(rpharm.contains("Fluoromisonidazole", Qt::CaseInsensitive) ||
          rpharm.contains("MISO", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Fluoromisonidazole_F18;
  }
  else if(rpharm.contains("Fluoromethane", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Fluoromethane_F18;
  }
  else if(rpharm.contains("Fluorouracil", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Fluorouracil_F18;
  }
  else if(rpharm.contains("Fluorobenzothiazole", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Fluorobenzothiazole_F18;
  }
  else if(rpharm.contains("Mespiperone", Qt::CaseInsensitive) ||
          rpharm.contains("MSP", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Mespiperone_C11;
  }
  else if(rpharm.contains("Monoclonal", Qt::CaseInsensitive) ||
          rpharm.contains("MABS", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Monoclonal_Antibody_I124;
  }
  else if(rpharm.contains("PTSM", Qt::CaseInsensitive))
  {
    pharmType = CPhilipsMainHeader::Ptsm_CU62;
  }
  else if(rpharm.isEmpty() == false)
    pharmType = CPhilipsMainHeader::OtherPharm;

  // set the header
  extHeader.dep_pharm = pharmType;
  strncpy(extHeader.radiopharm_name, rpharm.toLatin1().constData(), sizeof(extHeader.radiopharm_name)-1);

  // set the result
  result = pharmType != CPhilipsMainHeader::UndefinedPharm;

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

CPhilipsMainHeader::Pharm_Type CPhilipsMainHeader::dep_pharm() const
{
  return static_cast<Pharm_Type>(m_pData->extHeader.dep_pharm);
}

int CPhilipsMainHeader::assay_date() const
{
  return m_pData->extHeader.assay_date;
}

int CPhilipsMainHeader::assay_time() const
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

short CPhilipsMainHeader::dep_abundance() const
{
  return m_pData->extHeader.dep_abundance;
}

short CPhilipsMainHeader::petct_realign_x() const
{
  return m_pData->extHeader.petct_realign_x;
}

short CPhilipsMainHeader::petct_realign_y() const
{
  return m_pData->extHeader.petct_realign_y;
}

short CPhilipsMainHeader::petct_realign_hr() const
{
  return m_pData->extHeader.petct_realign_hr;
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

short CPhilipsMainHeader::petct_realign_zr() const
{
  return m_pData->extHeader.petct_realign_zr;
}

short CPhilipsMainHeader::petct_realign_vr() const
{
  return m_pData->extHeader.petct_realign_vr;
}

float CPhilipsMainHeader::positron_fraction() const
{
  return m_pData->extHeader.positron_fraction;
}

short CPhilipsMainHeader::image_org() const
{
  return m_pData->extHeader.image_org;
}

short CPhilipsMainHeader::pulmo_trig_of() const
{
  return m_pData->extHeader.pulmo_trig_of;
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

short CPhilipsMainHeader::petct_realign_z() const
{
  return m_pData->extHeader.petct_realign_z;
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

const char* CPhilipsMainHeader::ref_phys() const
{
  return m_pData->extHeader.ref_phys;
}

const char* CPhilipsMainHeader::study_id() const
{
  return m_pData->extHeader.study_id;
}

short CPhilipsMainHeader::pt_tbl_vers() const
{
  return m_pData->extHeader.pt_tbl_vers;
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

const char* CPhilipsMainHeader::Dmanuf_model() const
{
  return m_pData->extHeader.Dmanuf_model;
}

const char* CPhilipsMainHeader::Dimg_type() const
{
  return m_pData->extHeader.Dimg_type;
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

int CPhilipsMainHeader::series_number() const
{
  return m_pData->extHeader.series_number;
}

int CPhilipsMainHeader::dep_study_date() const
{
  return m_pData->extHeader.dep_study_date;
}

int CPhilipsMainHeader::dep_study_time() const
{
  return m_pData->extHeader.dep_study_time;
}

int CPhilipsMainHeader::dep_acq_time() const
{
  return m_pData->extHeader.dep_acq_time;
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

int CPhilipsMainHeader::dep_acq_date() const
{
  return m_pData->extHeader.dep_acq_date;
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

const char* CPhilipsMainHeader::operator_name() const
{
  return m_pData->extHeader.operator_name;
}

const char* CPhilipsMainHeader::contr_bolus_agent() const
{
  return m_pData->extHeader.contr_bolus_agent;
}

const char* CPhilipsMainHeader::sop_uid() const
{
  return m_pData->extHeader.sop_uid;
}

const char* CPhilipsMainHeader::frame_ref_uid() const
{
  return m_pData->extHeader.frame_ref_uid;
}

const char* CPhilipsMainHeader::pps_file() const
{
  return m_pData->extHeader.pps_file;
}

const char* CPhilipsMainHeader::worklist_file() const
{
  return m_pData->extHeader.worklist_file;
}

const char* CPhilipsMainHeader::recon_swrel() const
{
  return m_pData->extHeader.recon_swrel;
}

const char* CPhilipsMainHeader::analy_swrel() const
{
  return m_pData->extHeader.analy_swrel;
}

const char* CPhilipsMainHeader::recprotocol_name() const
{
  return m_pData->extHeader.recprotocol_name;
}

const char* CPhilipsMainHeader::insinofile() const
{
  return m_pData->extHeader.insinofile;
}

bool CPhilipsMainHeader::slc_add() const
{
  return m_pData->extHeader.slc_add == 1;
}

short CPhilipsMainHeader::slc_space() const
{
  return m_pData->extHeader.slc_space;
}

short CPhilipsMainHeader::slc_thick() const
{
  return m_pData->extHeader.slc_thick;
}

bool CPhilipsMainHeader::frame_add() const
{
  return m_pData->extHeader.frame_add == 1;
}

short CPhilipsMainHeader::frame_space() const
{
  return m_pData->extHeader.frame_space;
}

short CPhilipsMainHeader::frame_thick() const
{
  return m_pData->extHeader.frame_thick;
}

CPhilipsMainHeader::Fltr_Type CPhilipsMainHeader::fltr_type() const
{
  return static_cast<CPhilipsMainHeader::Fltr_Type>(m_pData->extHeader.fltr_type);
}

short CPhilipsMainHeader::smoth() const
{
  return m_pData->extHeader.smoth;
}

short CPhilipsMainHeader::scatcorr_type() const
{
  return m_pData->extHeader.scatcorr_type;
}

short CPhilipsMainHeader::edge_exp() const
{
  return m_pData->extHeader.edge_exp;
}

short CPhilipsMainHeader::bckang_avg() const
{
  return m_pData->extHeader.bckang_avg;
}

float CPhilipsMainHeader::bck_coeff() const
{
  return m_pData->extHeader.bck_coeff;
}

short CPhilipsMainHeader::bck_wid() const
{
  return m_pData->extHeader.bck_wid;
}

CPhilipsMainHeader::AttnCorr_Type CPhilipsMainHeader::attncor_type() const
{
  return static_cast<CPhilipsMainHeader::AttnCorr_Type>(m_pData->extHeader.attncor_type);
}

CPhilipsMainHeader::AttnCorrEcc_Type CPhilipsMainHeader::attncor_ecc() const
{
  return static_cast<CPhilipsMainHeader::AttnCorrEcc_Type>(m_pData->extHeader.attncor_ecc);
}

float CPhilipsMainHeader::attn_coeff() const
{
  return m_pData->extHeader.attn_coeff;
}

const char* CPhilipsMainHeader::regfile() const
{
  return m_pData->extHeader.regfile;
}

const char* CPhilipsMainHeader::proc_transinofile() const
{
  return m_pData->extHeader.proc_transinofile;
}

float CPhilipsMainHeader::skull_comp() const
{
  return m_pData->extHeader.skull_comp;
}

CPhilipsMainHeader::Norm_Type CPhilipsMainHeader::norm_type() const
{
  return static_cast<CPhilipsMainHeader::Norm_Type>(m_pData->extHeader.norm_type);
}

bool CPhilipsMainHeader::smp_norm() const
{
  return m_pData->extHeader.smp_norm == 1;
}

const char* CPhilipsMainHeader::axnfile() const
{
  return m_pData->extHeader.axnfile;
}

const char* CPhilipsMainHeader::effnormfile() const
{
  return m_pData->extHeader.effnormfile;
}

bool CPhilipsMainHeader::gap_comp() const
{
  return m_pData->extHeader.gap_comp == 1;
}

CPhilipsMainHeader::Alg_Type CPhilipsMainHeader::algtype_em() const
{
  return static_cast<CPhilipsMainHeader::Alg_Type>(m_pData->extHeader.algtype_em);
}

short CPhilipsMainHeader::num_iter() const
{
  return m_pData->extHeader.num_iter;
}

short CPhilipsMainHeader::iter_em() const
{
  return m_pData->extHeader.iter_em;
}

short CPhilipsMainHeader::subset_em() const
{
  return m_pData->extHeader.subset_em;
}

short CPhilipsMainHeader::nsmooth_em() const
{
  return m_pData->extHeader.nsmooth_em;
}

short CPhilipsMainHeader::nrepeat_em() const
{
  return m_pData->extHeader.nrepeat_em;
}

short CPhilipsMainHeader::bckslc_avg() const
{
  return m_pData->extHeader.bckslc_avg;
}

bool CPhilipsMainHeader::dead_corr() const
{
  return m_pData->extHeader.dead_corr == 1;
}

bool CPhilipsMainHeader::decay_corr() const
{
  return m_pData->extHeader.decay_corr == 1;
}

const char* CPhilipsMainHeader::transinofile() const
{
  return m_pData->extHeader.transinofile;
}

const char* CPhilipsMainHeader::blnksinofile() const
{
  return m_pData->extHeader.blnksinofile;
}

float CPhilipsMainHeader::tran_ray_fwhm() const
{
  return m_pData->extHeader.tran_ray_fwhm;
}

float CPhilipsMainHeader::tran_axl_fwhm() const
{
  return m_pData->extHeader.tran_axl_fwhm;
}

short CPhilipsMainHeader::surv_mask() const
{
  return m_pData->extHeader.surv_mask;
}

CPhilipsMainHeader::PreFlt_Type CPhilipsMainHeader::preflt_type() const
{
  return static_cast<CPhilipsMainHeader::PreFlt_Type>(m_pData->extHeader.preflt_type);
}

CPhilipsMainHeader::PostFlt_Type CPhilipsMainHeader::postflt_type() const
{
  return static_cast<CPhilipsMainHeader::PostFlt_Type>(m_pData->extHeader.postflt_type);
}

CPhilipsMainHeader::PostProc_Type CPhilipsMainHeader::tr_posttyp() const
{
  return static_cast<CPhilipsMainHeader::PostProc_Type>(m_pData->extHeader.tr_posttyp);
}

CPhilipsMainHeader::AlgTr_Type CPhilipsMainHeader::algtype_tr() const
{
  return static_cast<CPhilipsMainHeader::AlgTr_Type>(m_pData->extHeader.algtype_tr);
}

short CPhilipsMainHeader::iter_tr() const
{
  return m_pData->extHeader.iter_tr;
}

short CPhilipsMainHeader::subset_tr() const
{
  return m_pData->extHeader.subset_tr;
}

short CPhilipsMainHeader::nsmooth_tr() const
{
  return m_pData->extHeader.nsmooth_tr;
}

short CPhilipsMainHeader::nrepeat_tr() const
{
  return m_pData->extHeader.nrepeat_tr;
}

bool CPhilipsMainHeader::attn_corr_3d() const
{
  return m_pData->extHeader.attn_corr_3d == 1;
}

short CPhilipsMainHeader::ramla_no_it() const
{
  return m_pData->extHeader.ramla_no_it;
}

short CPhilipsMainHeader::ramla_sysac() const
{
  return m_pData->extHeader.ramla_sysac;
}

float CPhilipsMainHeader::ramla_lambda(const short i) const
{
  if(i >= 0 && i <= 4)
    return m_pData->extHeader.ramla_lambda[i];
  else
    return 0.0f;
}

float CPhilipsMainHeader::ramla_blrad() const
{
  return m_pData->extHeader.ramla_blrad;
}

float CPhilipsMainHeader::ramla_blalpha() const
{
  return m_pData->extHeader.ramla_blalpha;
}

float CPhilipsMainHeader::ramla_bcc_rsz() const
{
  return m_pData->extHeader.ramla_bcc_rsz;
}

time_t CPhilipsMainHeader::recon_date_time() const
{
  return m_pData->extHeader.recon_date_time;
}

short CPhilipsMainHeader::gating_type() const
{
  return m_pData->extHeader.gating_type;
}

const char* CPhilipsMainHeader::ref_attncor_series_uid() const
{
  return m_pData->extHeader.ref_attncor_series_uid;
}

const char* CPhilipsMainHeader::ref_gated_qc_image_inst_uid() const
{
  return m_pData->extHeader.ref_gated_qc_image_inst_uid;
}

const char* CPhilipsMainHeader::ref_raw_data_inst_uid() const
{
  return m_pData->extHeader.ref_raw_data_inst_uid;
}

int CPhilipsMainHeader::start_table_pos_abs() const
{
  return m_pData->extHeader.start_table_pos_abs;
}

int CPhilipsMainHeader::start_table_pos_rel() const
{
  return m_pData->extHeader.start_table_pos_rel;
}

bool CPhilipsMainHeader::mr_valid() const
{
  return m_pData->extHeader.mr_valid == 1;
}

unsigned char CPhilipsMainHeader::coil_type(const short i) const
{
  if(i >= 0 && i <= 14)
    return m_pData->extHeader.coil_type[i];
  else
    return 0;
}

const char * CPhilipsMainHeader::ref_raw_data_series_inst_uid() const
{
  return m_pData->extHeader.ref_raw_data_series_inst_uid;
}

const char * CPhilipsMainHeader::pet_software_version() const
{
  return m_pData->extHeader.pet_software_version;
}

unsigned char CPhilipsMainHeader::num_tile_rings() const
{
  return m_pData->extHeader.num_tile_rings;
}

unsigned char CPhilipsMainHeader::num_detector_modules() const
{
  return m_pData->extHeader.num_detector_modules;
}

unsigned char CPhilipsMainHeader::num_tiles_per_module_row() const
{
  return m_pData->extHeader.num_tiles_per_module_row;
}

float CPhilipsMainHeader::X_tile_gap() const
{
  return m_pData->extHeader.X_tile_gap;
}

float CPhilipsMainHeader::Z_tile_gap() const
{
  return m_pData->extHeader.Z_tile_gap;
}

short CPhilipsMainHeader::crystal_type() const
{
  return m_pData->extHeader.crystal_type;
}

short CPhilipsMainHeader::psf1() const
{
  return m_pData->extHeader.psf1;
}

short CPhilipsMainHeader::psf1_iter() const
{
  return m_pData->extHeader.psf1_iter;
}

float CPhilipsMainHeader::psf1_regularization() const
{
  return m_pData->extHeader.psf1_regularization;
}

short CPhilipsMainHeader::psf2() const
{
  return m_pData->extHeader.psf2;
}

short CPhilipsMainHeader::psf2_iter() const
{
  return m_pData->extHeader.psf2_iter;
}

float CPhilipsMainHeader::gaussian_kernel() const
{
  return m_pData->extHeader.gaussian_kernel;
}

const char * CPhilipsMainHeader::acqprotocol_name() const
{
  return m_pData->extHeader.acqprotocol_name;
}

unsigned short CPhilipsMainHeader::coinc_half_window_bins() const
{
  return m_pData->extHeader.coinc_half_window_bins;
}

const char * CPhilipsMainHeader::isotope_name() const
{
  return m_pData->extHeader.isotope_name;
}

const char * CPhilipsMainHeader::isotope_code_value() const
{
  return m_pData->extHeader.isotope_code_value;
}

const char * CPhilipsMainHeader::isotope_coding_scheme_designator() const
{
  return m_pData->extHeader.isotope_coding_scheme_designator;
}

const char * CPhilipsMainHeader::isotope_coding_scheme_version() const
{
  return m_pData->extHeader.isotope_coding_scheme_version;
}

const char * CPhilipsMainHeader::isotope_code_meaning() const
{
  return m_pData->extHeader.isotope_code_meaning;
}

const char * CPhilipsMainHeader::radiopharm_code_value() const
{
  return m_pData->extHeader.radiopharm_code_value;
}

const char * CPhilipsMainHeader::radiopharm_coding_scheme_designator() const
{
  return m_pData->extHeader.radiopharm_coding_scheme_designator;
}

const char * CPhilipsMainHeader::radiopharm_coding_scheme_version() const
{
  return m_pData->extHeader.radiopharm_coding_scheme_version;
}

const char * CPhilipsMainHeader::radiopharm_code_meaning() const
{
  return m_pData->extHeader.radiopharm_code_meaning;
}

void CPhilipsMainHeader::setDpat_name(const char* str)
{
  strncpy(m_pData->extHeader.Dpat_name, str, sizeof(m_pData->extHeader.Dpat_name)-1);
}

void CPhilipsMainHeader::setDpat_id(const char* str)
{
  strncpy(m_pData->extHeader.Dpat_id, str, sizeof(m_pData->extHeader.Dpat_id)-1);
}

void CPhilipsMainHeader::setStudy_uid(const char* str)
{
  strncpy(m_pData->extHeader.study_uid, str, sizeof(m_pData->extHeader.study_uid)-1);
}

void CPhilipsMainHeader::setSeries_uid(const char* str)
{
  strncpy(m_pData->extHeader.series_uid, str, sizeof(m_pData->extHeader.series_uid)-1);
}

void CPhilipsMainHeader::setView_code(const char* str)
{
  strncpy(m_pData->extHeader.view_code, str, sizeof(m_pData->extHeader.view_code)-1);
}

void CPhilipsMainHeader::setSortproto_name(const char* str)
{
  strncpy(m_pData->extHeader.sortproto_name, str, sizeof(m_pData->extHeader.sortproto_name)-1);
}

void CPhilipsMainHeader::setRoute(const Route_Type route)
{
  m_pData->extHeader.route = static_cast<qint16>(route);
}

void CPhilipsMainHeader::setDep_pharm(const Pharm_Type pharm)
{
  m_pData->extHeader.dep_pharm = static_cast<qint16>(pharm);
}

void CPhilipsMainHeader::setReq_phys(const char* str)
{
  strncpy(m_pData->extHeader.req_phys, str, sizeof(m_pData->extHeader.req_phys)-1);
}

void CPhilipsMainHeader::setCard_phstate(const Card_Ph_State state)
{
  m_pData->extHeader.card_phstate = state;
}

void CPhilipsMainHeader::setAssay_date(const int assay_date)
{
  m_pData->extHeader.assay_date = assay_date;
}

void CPhilipsMainHeader::setAssay_time(const int assay_time)
{
  m_pData->extHeader.assay_time = assay_time;
}

void CPhilipsMainHeader::setSeries_desc(const char* str)
{
  strncpy(m_pData->extHeader.series_desc, str, sizeof(m_pData->extHeader.series_desc)-1);
}

void CPhilipsMainHeader::setHeight(const short height)
{
  m_pData->extHeader.height = height;
}

void CPhilipsMainHeader::setDep_abundance(const short abundance)
{
  m_pData->extHeader.dep_abundance = abundance;
}

void CPhilipsMainHeader::setPetct_realign_x(const short xOffset)
{
  m_pData->extHeader.petct_realign_x = xOffset;
}

void CPhilipsMainHeader::setPetct_realign_y(const short yOffset)
{
  m_pData->extHeader.petct_realign_y = yOffset;
}

void CPhilipsMainHeader::setPetct_realign_hr(const short horizRotation)
{
  m_pData->extHeader.petct_realign_hr = horizRotation;
}

void CPhilipsMainHeader::setAcq_date_time(const time_t date_time)
{
  m_pData->extHeader.acq_date_time = date_time;

  // we also set acq_date and acq_time
  QDateTime dtime = acq_date_time_Qt();
  m_pData->extHeader.dep_acq_date = dtime.toString("yyyyMMdd").toInt(); // YYYYMMDD
  m_pData->extHeader.dep_acq_time = dtime.time().hour() * 3600 + dtime.time().minute() * 60 + dtime.time().second();
}

void CPhilipsMainHeader::setStudy_date_time(const time_t date_time)
{
  m_pData->extHeader.study_date_time = date_time;

  // we also set study_date and study_time
  QDateTime dtime = study_date_time_Qt();
  m_pData->extHeader.dep_study_date = dtime.toString("yyyyMMdd").toInt(); // YYYYMMDD
  m_pData->extHeader.dep_study_time = dtime.time().hour() * 3600 + dtime.time().minute() * 60 + dtime.time().second();
}

void CPhilipsMainHeader::setInjection_date_time(const time_t date_time)
{
  m_pData->extHeader.injection_date_time = date_time;
}

void CPhilipsMainHeader::setFile_create_date_time(const time_t date_time)
{
  m_pData->extHeader.file_create_date_time = date_time;

  // we also set study_date and study_time
  QDateTime dtime = file_create_date_time_Qt();
  m_pData->header.dep_daycre = dtime.date().day();
  m_pData->header.dep_mocre = dtime.date().month()-1;
  m_pData->header.dep_yrcre = dtime.date().year();
  m_pData->header.dep_hrcre = dtime.time().hour();
  m_pData->header.dep_mincre = dtime.time().minute();
  m_pData->header.dep_seccre = dtime.time().second();
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

void CPhilipsMainHeader::setPetct_realign_zr(const short axialRotation)
{
  m_pData->extHeader.petct_realign_zr = axialRotation;
}

void CPhilipsMainHeader::setPetct_realign_vr(const short vertRotation)
{
  m_pData->extHeader.petct_realign_vr = vertRotation;
}

void CPhilipsMainHeader::setPositron_fraction(const float fraction)
{
  m_pData->extHeader.positron_fraction = fraction;
}

void CPhilipsMainHeader::setImage_org(const short value)
{
  m_pData->extHeader.image_org = value;
}

void CPhilipsMainHeader::setPulmo_trig_of(const short value)
{
  m_pData->extHeader.pulmo_trig_of = value;
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

void CPhilipsMainHeader::setPetct_realign_z(const short zOffset)
{
  m_pData->extHeader.petct_realign_z = zOffset;
}

void CPhilipsMainHeader::setWindow_units(const CPhilipsMainHeader::Window_Units window_units)
{
  m_pData->extHeader.window_units = static_cast<qint16>(window_units);
}

void CPhilipsMainHeader::setRef_phys(const char* str)
{
  strncpy(m_pData->extHeader.ref_phys, str, sizeof(m_pData->extHeader.ref_phys)-1);
}

void CPhilipsMainHeader::setStudy_id(const char* str)
{
  strncpy(m_pData->extHeader.study_id, str, sizeof(m_pData->extHeader.study_id)-1);
}

void CPhilipsMainHeader::setPt_tbl_vers(const short value)
{
  m_pData->extHeader.pt_tbl_vers = value;
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
  m_pData->extHeader.card_bt_rej = (flag == true) ? 1 : 0;
}

void CPhilipsMainHeader::setCard_fr_type(const CPhilipsMainHeader::Card_Fr_Type type)
{
  m_pData->extHeader.card_fr_type = static_cast<qint16>(type);
}

void CPhilipsMainHeader::setDmanuf_model(const char* str)
{
  strncpy(m_pData->extHeader.Dmanuf_model, str, sizeof(m_pData->extHeader.Dmanuf_model)-1);
}

void CPhilipsMainHeader::setDimg_type(const char* str)
{
  strncpy(m_pData->extHeader.Dimg_type, str, sizeof(m_pData->extHeader.Dimg_type)-1);
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

void CPhilipsMainHeader::setSeries_number(const int number)
{
  m_pData->extHeader.series_number = number;
}

void CPhilipsMainHeader::setDep_study_date(const int date)
{
  m_pData->extHeader.dep_study_date = date;
}

void CPhilipsMainHeader::setDep_study_time(const int time)
{
  m_pData->extHeader.dep_study_time = time;
}

void CPhilipsMainHeader::setDep_acq_time(const int time)
{
  m_pData->extHeader.dep_acq_time = time;
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
  strncpy(m_pData->extHeader.radiopharm_name, str, sizeof(m_pData->extHeader.radiopharm_name)-1);
}

void CPhilipsMainHeader::setDserial_number(const char* str)
{
  strncpy(m_pData->extHeader.Dserial_number, str, sizeof(m_pData->extHeader.Dserial_number)-1);
}

void CPhilipsMainHeader::setAttncor_label(const char* str)
{
  strncpy(m_pData->extHeader.attncor_label, str, sizeof(m_pData->extHeader.attncor_label)-1);
}

void CPhilipsMainHeader::setOperator_name(const char* str)
{
  strncpy(m_pData->extHeader.operator_name, str, sizeof(m_pData->extHeader.operator_name)-1);
}

void CPhilipsMainHeader::setContr_bolus_agent(const char* str)
{
  strncpy(m_pData->extHeader.contr_bolus_agent, str, sizeof(m_pData->extHeader.contr_bolus_agent)-1);
}

void CPhilipsMainHeader::setSop_uid(const char* str)
{
  strncpy(m_pData->extHeader.sop_uid, str, sizeof(m_pData->extHeader.sop_uid)-1);
}

void CPhilipsMainHeader::setFrame_ref_uid(const char* str)
{
  strncpy(m_pData->extHeader.frame_ref_uid, str, sizeof(m_pData->extHeader.frame_ref_uid)-1);
}

void CPhilipsMainHeader::setPps_file(const char* str)
{
  strncpy(m_pData->extHeader.pps_file, str, sizeof(m_pData->extHeader.pps_file)-1);
}

void CPhilipsMainHeader::setWorklist_file(const char* str)
{
  strncpy(m_pData->extHeader.worklist_file, str, sizeof(m_pData->extHeader.worklist_file)-1);
}

void CPhilipsMainHeader::setRecon_swrel(const char* str)
{
  strncpy(m_pData->extHeader.recon_swrel, str, sizeof(m_pData->extHeader.recon_swrel)-1);
}

void CPhilipsMainHeader::setAnaly_swrel(const char* str)
{
  strncpy(m_pData->extHeader.analy_swrel, str, sizeof(m_pData->extHeader.analy_swrel)-1);
}

void CPhilipsMainHeader::setRecprotocol_name(const char* str)
{
  strncpy(m_pData->extHeader.recprotocol_name, str, sizeof(m_pData->extHeader.recprotocol_name)-1);
}

void CPhilipsMainHeader::setInsinofile(const char* str)
{
  strncpy(m_pData->extHeader.insinofile, str, sizeof(m_pData->extHeader.insinofile)-1);
}

void CPhilipsMainHeader::setSlc_add(const bool add)
{
  m_pData->extHeader.slc_add = (add == true) ? 1 : 0;
}

void CPhilipsMainHeader::setSlc_space(const short space)
{
  m_pData->extHeader.slc_space = space;
}

void CPhilipsMainHeader::setSlc_thick(const short thick)
{
  m_pData->extHeader.slc_thick = thick;
}

void CPhilipsMainHeader::setFrame_add(const bool add)
{
  m_pData->extHeader.frame_add = (add == true) ? 1 : 0;
}

void CPhilipsMainHeader::setFrame_space(const short space)
{
  m_pData->extHeader.frame_space = space;
}

void CPhilipsMainHeader::setFrame_thick(const short thick)
{
  m_pData->extHeader.frame_thick = thick;
}

void CPhilipsMainHeader::setFltr_type(const CPhilipsMainHeader::Fltr_Type type)
{
  m_pData->extHeader.fltr_type = type;
}

void CPhilipsMainHeader::setSmoth(const short smoth)
{
  m_pData->extHeader.smoth = smoth;
}

void CPhilipsMainHeader::setScatcorr_type(const short type)
{
  m_pData->extHeader.scatcorr_type = type;
}

void CPhilipsMainHeader::setEdge_exp(const short exp)
{
  m_pData->extHeader.edge_exp = exp;
}

void CPhilipsMainHeader::setBckang_avg(const short avg)
{
  m_pData->extHeader.bckang_avg = avg;
}

void CPhilipsMainHeader::setBck_coeff(const float coeff)
{
  m_pData->extHeader.bck_coeff = coeff;
}

void CPhilipsMainHeader::setBck_wid(const short wid)
{
  m_pData->extHeader.bck_wid = wid;
}

void CPhilipsMainHeader::setAttncor_type(const AttnCorr_Type type)
{
  m_pData->extHeader.attncor_type = type;
}

void CPhilipsMainHeader::setAttncor_ecc(const AttnCorrEcc_Type type)
{
  m_pData->extHeader.attncor_ecc = type;
}

void CPhilipsMainHeader::setAttn_coeff(const float coeff)
{
  m_pData->extHeader.attn_coeff = coeff;
}

void CPhilipsMainHeader::setRegfile(const char* file)
{
  strncpy(m_pData->extHeader.regfile, file, sizeof(m_pData->extHeader.regfile)-1);
}

void CPhilipsMainHeader::setProc_transinofile(const char* file)
{
  strncpy(m_pData->extHeader.transinofile, file, sizeof(m_pData->extHeader.transinofile)-1);
}
  
void CPhilipsMainHeader::setSkull_comp(const float comp)
{
  m_pData->extHeader.skull_comp = comp;
}

void CPhilipsMainHeader::setNorm_type(const Norm_Type type)
{
  m_pData->extHeader.norm_type = type;
}

void CPhilipsMainHeader::setSmp_norm(const bool norm)
{
  m_pData->extHeader.smp_norm = (norm == true) ? 1 : 0;
}

void CPhilipsMainHeader::setAxnfile(const char* file)
{
  strncpy(m_pData->extHeader.axnfile, file, sizeof(m_pData->extHeader.axnfile)-1);
}

void CPhilipsMainHeader::setEffnormfile(const char* file)
{
  strncpy(m_pData->extHeader.effnormfile, file, sizeof(m_pData->extHeader.effnormfile)-1);
}
  
void CPhilipsMainHeader::setGap_comp(const bool norm)
{
  m_pData->extHeader.gap_comp = (norm == true) ? 1 : 0;
}

void CPhilipsMainHeader::setAlgtype_em(const Alg_Type type)
{
  m_pData->extHeader.algtype_em = type;
}

void CPhilipsMainHeader::setNum_iter(const short num)
{
  m_pData->extHeader.num_iter = num;
}

void CPhilipsMainHeader::setIter_em(const short em)
{
  m_pData->extHeader.iter_em = em;
}

void CPhilipsMainHeader::setSubset_em(const short em)
{
  m_pData->extHeader.subset_em = em;
}

void CPhilipsMainHeader::setNsmooth_em(const short em)
{
  m_pData->extHeader.nsmooth_em = em;
}

void CPhilipsMainHeader::setNrepeat_em(const short em)
{
  m_pData->extHeader.nrepeat_em = em;
}

void CPhilipsMainHeader::setBckslc_avg(const short avg)
{
  m_pData->extHeader.bckslc_avg = avg;
}

void CPhilipsMainHeader::setDead_corr(const bool corr)
{
  m_pData->extHeader.dead_corr = (corr == true) ? 1 : 0;
}

void CPhilipsMainHeader::setDecay_corr(const bool decay)
{
  m_pData->extHeader.decay_corr = (decay == true) ? 1 : 0;
}

void CPhilipsMainHeader::setTransinofile(const char* file)
{
  strncpy(m_pData->extHeader.transinofile, file, sizeof(m_pData->extHeader.transinofile)-1);
}

void CPhilipsMainHeader::setBlnksinofile(const char* file)
{
  strncpy(m_pData->extHeader.blnksinofile, file, sizeof(m_pData->extHeader.blnksinofile)-1);
}

void CPhilipsMainHeader::setTran_ray_fwhm(const float fwhm)
{
  m_pData->extHeader.tran_ray_fwhm = fwhm;
}

void CPhilipsMainHeader::setTran_axl_fwhm(const float fwhm)
{
  m_pData->extHeader.tran_axl_fwhm = fwhm;
}

void CPhilipsMainHeader::setSurv_mask(const short mask)
{
  m_pData->extHeader.surv_mask = mask;
}

void CPhilipsMainHeader::setPreflt_type(const PreFlt_Type type)
{
  m_pData->extHeader.preflt_type = type;
}

void CPhilipsMainHeader::setPostflt_type(const PreFlt_Type type)
{
  m_pData->extHeader.postflt_type = type;
}

void CPhilipsMainHeader::setTr_posttyp(const PostProc_Type type)
{
  m_pData->extHeader.tr_posttyp = type;
}

void CPhilipsMainHeader::setAlgtype_tr(const AlgTr_Type type)
{
  m_pData->extHeader.algtype_tr = type;
}

void CPhilipsMainHeader::setIter_tr(const short tr)
{
  m_pData->extHeader.iter_tr = tr;
}

void CPhilipsMainHeader::setSubset_tr(const short tr)
{
  m_pData->extHeader.subset_tr = tr;
}

void CPhilipsMainHeader::setNsmooth_tr(const short tr)
{
  m_pData->extHeader.nsmooth_tr = tr;
}

void CPhilipsMainHeader::setNrepeat_tr(const short tr)
{
  m_pData->extHeader.nrepeat_tr = tr;
}

void CPhilipsMainHeader::setAttn_corr_3d(const bool attn)
{
  m_pData->extHeader.attn_corr_3d = (attn == true) ? 1 : 0;
}

void CPhilipsMainHeader::setRamla_no_it(const short it)
{
  m_pData->extHeader.ramla_no_it = it;
}

void CPhilipsMainHeader::setRamla_sysac(const short sysac)
{
  m_pData->extHeader.ramla_sysac = sysac;
}

void CPhilipsMainHeader::setRamla_lambda(const short i, const float lambda)
{
  if(i >= 0 && i <= 4)
    m_pData->extHeader.ramla_lambda[i] = lambda;
}

void CPhilipsMainHeader::setRamla_blrad(const float blrad)
{
  m_pData->extHeader.ramla_blrad = blrad;
}

void CPhilipsMainHeader::setRamla_blalpha(const float blalpha)
{
  m_pData->extHeader.ramla_blalpha = blalpha;
}

void CPhilipsMainHeader::setRamla_bcc_rsz(const float rsz)
{
  m_pData->extHeader.ramla_bcc_rsz = rsz;
}

void CPhilipsMainHeader::setRecon_date_time(const time_t time)
{
  m_pData->extHeader.recon_date_time = time;
}

void CPhilipsMainHeader::setGating_type(const short type)
{
  m_pData->extHeader.gating_type = type;
}

void CPhilipsMainHeader::setRef_attncor_series_uid(const char* str)
{
  strncpy(m_pData->extHeader.ref_attncor_series_uid, str, sizeof(m_pData->extHeader.ref_attncor_series_uid)-1);
}

void CPhilipsMainHeader::setRef_gated_qc_image_inst_uid(const char* str)
{
  strncpy(m_pData->extHeader.ref_gated_qc_image_inst_uid, str, sizeof(m_pData->extHeader.ref_gated_qc_image_inst_uid)-1);
}

void CPhilipsMainHeader::setRef_raw_data_inst_uid(const char* str)
{
  strncpy(m_pData->extHeader.ref_raw_data_inst_uid, str, sizeof(m_pData->extHeader.ref_raw_data_inst_uid)-1);
}

void CPhilipsMainHeader::setStart_table_pos_abs(const int pos)
{
  m_pData->extHeader.start_table_pos_abs = pos;
}

void CPhilipsMainHeader::setStart_table_pos_rel(const int pos)
{
  m_pData->extHeader.start_table_pos_rel = pos;
}

void CPhilipsMainHeader::setMr_valid(const bool valid)
{
  m_pData->extHeader.mr_valid = (valid == true) ? 1 : 0;
}

void CPhilipsMainHeader::setCoil_type(const short i, const unsigned char type)
{
  if(i >= 0 && i <= 14)
    m_pData->extHeader.coil_type[i] = type;
}

void CPhilipsMainHeader::setRef_raw_data_series_inst_uid(const char *str)
{
  strncpy(m_pData->extHeader.ref_raw_data_series_inst_uid, str, sizeof(m_pData->extHeader.ref_raw_data_series_inst_uid)-1);
}

void CPhilipsMainHeader::setPet_software_version(const char *str)
{
  strncpy(m_pData->extHeader.pet_software_version, str, sizeof(m_pData->extHeader.pet_software_version)-1);
}

void CPhilipsMainHeader::setNum_tile_rings(const unsigned char value)
{
  m_pData->extHeader.num_tile_rings = value;
}

void CPhilipsMainHeader::setNum_detector_modules(const unsigned char value)
{
  m_pData->extHeader.num_detector_modules = value;
}

void CPhilipsMainHeader::setNum_tiles_per_module_row(const unsigned char value)
{
  m_pData->extHeader.num_tiles_per_module_row = value;
}

void CPhilipsMainHeader::setX_tile_gap(const float value)
{
  m_pData->extHeader.X_tile_gap = value;
}

void CPhilipsMainHeader::setZ_tile_gap(const float value)
{
  m_pData->extHeader.Z_tile_gap = value;
}

void CPhilipsMainHeader::setCrystal_type(const short value)
{
  m_pData->extHeader.crystal_type = value;
}

void CPhilipsMainHeader::setPsf1(const short value)
{
  m_pData->extHeader.psf1 = value;
}

void CPhilipsMainHeader::setPsf1_iter(const short value)
{
  m_pData->extHeader.psf1_iter = value;
}

void CPhilipsMainHeader::setPsf1_regularization(const short value)
{
  m_pData->extHeader.psf1_regularization = value;
}

void CPhilipsMainHeader::setPsf2(const short value)
{
  m_pData->extHeader.psf2 = value;
}

void CPhilipsMainHeader::setPsf2_iter(const short value)
{
  m_pData->extHeader.psf2_iter = value;
}

void CPhilipsMainHeader::setGaussian_kernel(const float value)
{
  m_pData->extHeader.gaussian_kernel = value;
}

void CPhilipsMainHeader::setAcqprotocol_name(const char *str)
{
  strncpy(m_pData->extHeader.acqprotocol_name, str, sizeof(m_pData->extHeader.acqprotocol_name)-1);
}

void CPhilipsMainHeader::setCoinc_half_window_bins(unsigned short value)
{
  m_pData->extHeader.coinc_half_window_bins = value;
}

void CPhilipsMainHeader::setIsotope_name(const char *str)
{
  strncpy(m_pData->extHeader.isotope_name, str, sizeof(m_pData->extHeader.isotope_name)-1);
}

void CPhilipsMainHeader::setIsotope_code_value(const char *str)
{
  strncpy(m_pData->extHeader.isotope_code_value, str, sizeof(m_pData->extHeader.isotope_code_value)-1);
}

void CPhilipsMainHeader::setIsotope_coding_scheme_designator(const char *str)
{
  strncpy(m_pData->extHeader.isotope_coding_scheme_designator, str, sizeof(m_pData->extHeader.isotope_coding_scheme_designator)-1);
}

void CPhilipsMainHeader::setIsotope_coding_scheme_version(const char *str)
{
  strncpy(m_pData->extHeader.isotope_coding_scheme_version, str, sizeof(m_pData->extHeader.isotope_coding_scheme_version)-1);
}

void CPhilipsMainHeader::setIsotope_code_meaning(const char *str)
{
  strncpy(m_pData->extHeader.isotope_code_meaning, str, sizeof(m_pData->extHeader.isotope_code_meaning)-1);
}

void CPhilipsMainHeader::setRadiopharm_code_value(const char *str)
{
  strncpy(m_pData->extHeader.radiopharm_code_value, str, sizeof(m_pData->extHeader.radiopharm_code_value)-1);
}

void CPhilipsMainHeader::setRadiopharm_coding_scheme_designator(const char *str)
{
  strncpy(m_pData->extHeader.radiopharm_coding_scheme_designator, str, sizeof(m_pData->extHeader.radiopharm_coding_scheme_designator)-1);
}

void CPhilipsMainHeader::setRadiopharm_coding_scheme_version(const char *str)
{
  strncpy(m_pData->extHeader.radiopharm_coding_scheme_version, str, sizeof(m_pData->extHeader.radiopharm_coding_scheme_version)-1);
}

void CPhilipsMainHeader::setRadiopharm_code_meaning(const char *str)
{
  strncpy(m_pData->extHeader.radiopharm_code_meaning, str, sizeof(m_pData->extHeader.radiopharm_code_meaning)-1);
}
