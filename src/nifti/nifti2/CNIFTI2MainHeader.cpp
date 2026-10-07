/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio
 
  Copyright (C) 2004-2026 hzdr.de and contributors
 
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
    http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "CNIFTI2MainHeader.h"
#include "CNIFTIFile.h"
#include "CECATMainHeader.h"
#include "CECAT7MainHeader.h"
#include "CECAT7SubHeaderImage.h"
#include "CECATSubHeader.h"
#include "CConcordeFrameHeader.h"
#include "CPhilipsMainHeader.h"
#include "CPhilipsSubHeaderImage.h"
#include "MedIOUnits.h"

#include "config.h"

#include <QDataStream>
#include <QDateTime>
#include <QFileInfo>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include <time.h>
#include <unistd.h>

#include <rtdebug.h>

#include "bswap.h"

// We define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CNIFTI2MainHeaderPrivate { // private class to add private variables to the CNIFTI2MainHeader class without exposing them in the public header file

  public:
    // MainHeader structure (540 bytes)

    #define MAINHEADER_SIZE 540
    #pragma pack(push, 1)

    struct HeaderData { // structure to represent the NIFTI-2 main header, which is 540 bytes long -> bytes of the header 

      quint32 Sizeof_Hdr;                 //   0: Sizeof_Header -> 540 for NIFTI 2 
      char    Magic[8];                   //   4: Magic -> "n+2\0" for NIFTI 2
      qint16 DataType;                    //  12: Data_Type -> defines the data type of the image data (unsigned char, signed short, float, etc.)
      qint16 Bit_Pix;                     //  14: Bit_Pix -> Number of bits/voxel
      qint64 Dim[8];                      //  16: Dim -> Data array for dimensions
      double Intent_P1;                   //  80: 1st intent parameter
      double Intent_P2;                   //  88: 2nd intent parameter
      double Intent_P3;                   //  96: 3rd intent parameter
      double Pix_Dim[8];                   // 104: Grid spacings (unit per dimension)
      qint64 Vox_Offset;                  // 168: Offset into .nii file for image data
      double Scl_Slope;                   // 176: Data scaling: slope
      double Scl_Inter;                   // 184: Data scaling: intercept/offset
      double Cal_Max;                     // 192: Max display intensity
      double Cal_Min;                     // 200: Min display intensity
      double Slice_Duration;              // 208: Time for 1 slice
      double Toffset;                     // 216: Time axis shift
      qint64 Slice_Start;                  // 224: First slice index
      qint64 Slice_End;                    // 232: Last slice index
      char   Descrip[80];                  // 240: any text you like
      char   Aux_File[24];                 // 320: auxiliary filename
      qint32 Qform_Code;                  // 344: NIFTI_XFORM_* code
      qint32 Sform_Code;                  // 348: NIFTI_XFORM_* code
      double Quatern_B;                   // 352: Quaternion b parameter
      double Quatern_C;                   // 360: Quaternion c parameter
      double Quatern_D;                   // 368: Quaternion d parameter
      double Qoffset_X;                   // 376: Quaternion x shift
      double Qoffset_Y;                   // 384: Quaternion y shift
      double Qoffset_Z;                   // 392: Quaternion z shift
      double Srow_X[4];                   // 400: 1st row affine transform
      double Srow_Y[4];                   // 432: 2nd row affine transform
      double Srow_Z[4];                   // 464: 3rd row affine transform
      qint32 Slice_Code;                   // 496: Slice timing order
      qint32 XYZT_Units;                  // 500: Units of pixdim[1..8]
      qint32 Intent_Code;                 // 504: NIFTI_INTENT_*
      char   Intent_Name[16];              // 508: Name or meaning of the data
      char   Dim_Info;                     // 524: MRI slice ordering
      char  unused_str[15];                // 525: Unused space for future expansion
    } header;

    // byte order of the file the header belongs to
    QSysInfo::Endian byteOrder;
    #pragma pack(pop)
  };

// NIfTI-2 single file (.nii) magic signature: "n+2\0" followed by "\r\n\032\n"
static const char NIFTI2_MAGIC[8] = { 'n', '+', '2', '\0', '\r', '\n', '\x1a', '\n' };

// swap the byte order of all non-char elements of a NIfTI-2 header
static void swapHeader(struct CNIFTI2MainHeaderPrivate::HeaderData& header)
{
  BSWAP_32(header.Sizeof_Hdr);
  BSWAP_16(header.DataType);
  BSWAP_16(header.Bit_Pix);

  for(int i = 0; i < 8; i++) {
    BSWAP_64(header.Dim[i]);
    BSWAP_DBL(header.Pix_Dim[i]);
  }

  BSWAP_DBL(header.Intent_P1);
  BSWAP_DBL(header.Intent_P2);
  BSWAP_DBL(header.Intent_P3);

  BSWAP_64(header.Vox_Offset);
  BSWAP_64(header.Slice_Start);
  BSWAP_64(header.Slice_End);

  BSWAP_DBL(header.Scl_Slope);
  BSWAP_DBL(header.Scl_Inter);
  BSWAP_DBL(header.Cal_Max);
  BSWAP_DBL(header.Cal_Min);
  BSWAP_DBL(header.Slice_Duration);
  BSWAP_DBL(header.Toffset);

  BSWAP_32(header.Qform_Code);
  BSWAP_32(header.Sform_Code);

  BSWAP_DBL(header.Quatern_B);
  BSWAP_DBL(header.Quatern_C);
  BSWAP_DBL(header.Quatern_D);
  BSWAP_DBL(header.Qoffset_X);
  BSWAP_DBL(header.Qoffset_Y);
  BSWAP_DBL(header.Qoffset_Z);

  for(int i = 0; i < 4; i++) {
    BSWAP_DBL(header.Srow_X[i]);
    BSWAP_DBL(header.Srow_Y[i]);
    BSWAP_DBL(header.Srow_Z[i]);
  }

  BSWAP_32(header.Slice_Code);
  BSWAP_32(header.XYZT_Units);
  BSWAP_32(header.Intent_Code);
}

//==============================================================================================
// Constructors
CNIFTI2MainHeader::CNIFTI2MainHeader(CNIFTIFile* niftiFile, CNIFTIMainHeader::Type fileType)  : CNIFTIMainHeader(niftiFile) {
  
  ENTER();

  // allocate data from our private instance class
  m_pData = new CNIFTI2MainHeaderPrivate();

  // this constructor signals us to create a empty NIFTI2MainHeader
  // with prefilled data that is always the same for a NIFTI2 main header
  // depending on the supplied fileType we have to have a different size of header field
  clear();

  //setFileType(fileType);

  LEAVE();
}
// Copy constructor
CNIFTI2MainHeader::CNIFTI2MainHeader(const CNIFTI2MainHeader& src)  : CNIFTIMainHeader(src) {
  ENTER();

  // allocate data from our private instance class
  m_pData = new CNIFTI2MainHeaderPrivate(*(src.m_pData));

  LEAVE();
}

// Default assignment operator
CNIFTI2MainHeader& CNIFTI2MainHeader::operator=(const CNIFTI2MainHeader& src) {
  ENTER();

  if(m_pData != src.m_pData) {
    memcpy(&m_pData->header, 
           &src.m_pData->header, 
           sizeof(struct CNIFTI2MainHeaderPrivate::HeaderData));
    m_pData->byteOrder = src.m_pData->byteOrder;
  }

  LEAVE();
  return *this;
}

// Destructor
CNIFTI2MainHeader::~CNIFTI2MainHeader() {
  ENTER();

  delete m_pData;

  LEAVE();
}

// Header clear method
void CNIFTI2MainHeader::clear() {
  ENTER();
  
  // clear our MainHeader structure first
  memset(&m_pData->header, 0, sizeof(struct CNIFTI2MainHeaderPrivate::HeaderData));

  // new files are written in little endian byte order
  m_pData->byteOrder = QSysInfo::LittleEndian;

  LEAVE();
}

//=============================================================================================
// load the main header from the file
bool CNIFTI2MainHeader::load(void) {

  ENTER();
  CMedIOData* mData = medIOData();

  // only go on if the device is readable at all
  if(mData == NULL ||
     mData->isReadable() == false ||
     mData->seek(0) == false) {

    RETURN(false);
    return false;

  }

  // we read in all data at once using read()
  ASSERT(sizeof(m_pData->header) == MAINHEADER_SIZE);

  if(mData->read(reinterpret_cast<char*>(&m_pData->header), sizeof(m_pData->header)) != MAINHEADER_SIZE) {
    RETURN(false);
    return false;
  }

//------------------------------------------------------------------------------------

  // detect the byte order of the file by means of the sizeof_hdr field and
  // swap all non-char elements in case it differs from the one of this machine
  bool swapped = false;
  if(m_pData->header.Sizeof_Hdr != MAINHEADER_SIZE) {
    if(bswap_32(m_pData->header.Sizeof_Hdr) != MAINHEADER_SIZE) {
      W("invalid sizeof_hdr field: %u", m_pData->header.Sizeof_Hdr);
      RETURN(false);
      return false;
    }

    swapHeader(m_pData->header);
    swapped = true;
  }

  if(swapped)
    m_pData->byteOrder = (QSysInfo::ByteOrder == QSysInfo::LittleEndian) ? QSysInfo::BigEndian : QSysInfo::LittleEndian;
  else
    m_pData->byteOrder = QSysInfo::ByteOrder;

  // some more debug output
#if defined(DEBUG)
  D("NIFTI2 Main Header loaded:");
  D("------------------------");
  D("SIZE OF HEADER          : %d",           m_pData->header.Sizeof_Hdr);
  D("DATA TYPE               : %s",           m_pData->header.Data_Type);
  D("DB NAME                 : %s",           m_pData->header.Db_Name);
  D("EXTENTS                 : %d",           m_pData->header.Extents);
  D("SESSION ERROR           : %d",           m_pData->header.Session_Error);
  D("REGULAR                 : %c",           m_pData->header.Regular);
  D("DIM INFO                : %c",           m_pData->header.Dim_Info);
  D("DIM                     : %d %d %d %d %d %d %d %d", m_pData->header.Dim[0], m_pData->header.Dim[1], m_pData->header.Dim[2], m_pData->header.Dim[3], m_pData->header.Dim[4], m_pData->header.Dim[5], m_pData->header.Dim[6], m_pData->header.Dim[7]);
  D("INTENT P1               : %f",           m_pData->header.Intent_P1);
  D("INTENT P2               : %f",           m_pData->header.Intent_P2);
  D("INTENT P3               : %f",           m_pData->header.Intent_P3);
  D("INTENT CODE             : %d",           m_pData->header.Intent_Code);
  D("DATA TYPE               : %d",           m_pData->header.Data_Type);
  D("BIT PIX                 : %d",           m_pData->header.Bit_Pix);
  D("SLICE START             : %d",           m_pData->header.Slice_Start);
  D("PIX DIM                 : %f %f %f %f %f %f %f %f", m_pData->header.Pix_Dim[0], m_pData->header.Pix_Dim[1], m_pData->header.Pix_Dim[2], m_pData->header.Pix_Dim[3], m_pData->header.Pix_Dim[4], m_pData->header.Pix_Dim[5], m_pData->header.Pix_Dim[6], m_pData->header.Pix_Dim[7]);
  D("VOX OFFSET              : %f",           m_pData->header.Vox_Offset);
  D("SCL SLOPE                : %f",           m_pData->header.Scl_Slope);    
  D("SCL INTER               : %f",           m_pData->header.Scl_Inter);
  D("SLICE END               : %d",           m_pData->header.Slice_End);
  D("SLICE CODE              : %c",           m_pData->header.Slice_Code);
  D("XYZT UNITS              : %c",           m_pData->header.XYZT_Units);
  D("CAL MAX                 : %f",           m_pData->header.Cal_Max);
  D("CAL MIN                 : %f",           m_pData->header.Cal_Min);
  D("SLICE DURATION          : %f",           m_pData->header.Slice_Duration);
  D("TOFFSET                 : %f",           m_pData->header.Toffset);
  D("GLMAX                   : %d",           m_pData->header.Glmax);
  D("GLMIN                   : %d",           m_pData->header.Glmin);
  D("DESCRIP                 : %s",           m_pData->header.Descrip);
  D("AUX FILE                : %s",           m_pData->header.Aux_File);
  D("QFORM CODE              : %d",           m_pData->header.Qform_Code);
  D("SFORM CODE              : %d",           m_pData->header.Sform_Code);
  D("QUATERN B               : %f",           m_pData->header.Quatern_B);
  D("QUATERN C               : %f",           m_pData->header.Quatern_C);
  D("QUATERN D               : %f",           m_pData->header.Quatern_D);
  D("QOFFSET X               : %f",           m_pData->header.Qoffset_X);
  D("QOFFSET Y               : %f",           m_pData->header.Qoffset_Y);
  D("QOFFSET Z               : %f",           m_pData->header.Qoffset_Z);
  D("SROW X                  : %f %f %f %f", m_pData->header.Srow_X[0], m_pData->header.Srow_X[1], m_pData->header.Srow_X[2], m_pData->header.Srow_X[3]);
  D("SROW Y                  : %f %f %f %f", m_pData->header.Srow_Y[0], m_pData->header.Srow_Y[1], m_pData->header.Srow_Y[2], m_pData->header.Srow_Y[3]);
  D("SROW Z                  : %f %f %f %f", m_pData->header.Srow_Z[0], m_pData->header.Srow_Z[1], m_pData->header.Srow_Z[2], m_pData->header.Srow_Z[3]);
  D("INTENT NAME             : %s",           m_pData->header.Intent_Name);
  D("MAGIC                   : %s",           m_pData->header.Magic);
#endif

  RETURN(true);
  return true;
}

//=============================================================================================
QTextStream& operator>>(QTextStream& stream, CNIFTI2MainHeader& mHeader) {
  ENTER();
  
  QString buf;
  while(stream.atEnd() == false){  
    buf = stream.readLine();
    if(buf.isEmpty() == false)  {
      QString typeString = buf.section(" ", 0, 0);
      QString dataString = buf.section(" ", 1);
      bool convertSuccess = true;

      if(dataString.isEmpty())
        dataString = "";

      if(typeString == "SIZEOF_HDR")
        mHeader.m_pData->header.Sizeof_Hdr = dataString.toInt(&convertSuccess);
      else if(typeString == "DESCRIP")
        strncpy(mHeader.m_pData->header.Descrip, dataString.toLatin1(), sizeof(mHeader.m_pData->header.Descrip)-1);
      else if(typeString == "DIM")
      {
        for(int i=0; i < 8 && convertSuccess; i++)
        {
          QString subString = dataString.section(" ", i, i);
          if(subString.isEmpty()) break;
          mHeader.m_pData->header.Dim[i] = subString.toLongLong(&convertSuccess);
        }
      }
      else if(typeString == "PIX_DIM")
      {
        for(int i=0; i < 8 && convertSuccess; i++)
        {
          QString subString = dataString.section(" ", i, i);
          if(subString.isEmpty()) break;
          mHeader.m_pData->header.Pix_Dim[i] = subString.toDouble(&convertSuccess);
        }
      }
      else if(typeString == "VOX_OFFSET")
        mHeader.m_pData->header.Vox_Offset = dataString.toLongLong(&convertSuccess);

      else if(typeString == "QFORM_CODE")
        mHeader.m_pData->header.Qform_Code = dataString.toInt(&convertSuccess);
      else if(typeString == "SFORM_CODE")
        mHeader.m_pData->header.Sform_Code = dataString.toInt(&convertSuccess);
      else if(typeString == "QOFFSET_X")
        mHeader.m_pData->header.Qoffset_X = dataString.toDouble(&convertSuccess);
      else if(typeString == "QOFFSET_Y")
        mHeader.m_pData->header.Qoffset_Y = dataString.toDouble(&convertSuccess);
      else if(typeString == "QOFFSET_Z")
        mHeader.m_pData->header.Qoffset_Z = dataString.toDouble(&convertSuccess);
      else if(typeString == "QUATERN_B")
        mHeader.m_pData->header.Quatern_B = dataString.toDouble(&convertSuccess);
      else if(typeString == "QUATERN_C")
        mHeader.m_pData->header.Quatern_C = dataString.toDouble(&convertSuccess);
      else if(typeString == "QUATERN_D")
        mHeader.m_pData->header.Quatern_D = dataString.toDouble(&convertSuccess);
      else if(typeString == "SROW_X")
      {
        for(int i=0; i < 4 && convertSuccess; i++)
        {
          QString subString = dataString.section(" ", i, i);
          if(subString.isEmpty()) break;
          mHeader.m_pData->header.Srow_X[i] = subString.toDouble(&convertSuccess);
        }
      }
      else if(typeString == "SROW_Y")
      {
        for(int i=0; i < 4 && convertSuccess; i++)
        {    
          QString subString = dataString.section(" ", i, i);
          if(subString.isEmpty()) break;
          mHeader.m_pData->header.Srow_Y[i] = subString.toDouble(&convertSuccess);
        }
      }
      else if(typeString == "SROW_Z")
      {
        for(int i=0; i < 4 && convertSuccess; i++)
        {
          QString subString = dataString.section(" ", i, i);
          if(subString.isEmpty()) break;
          mHeader.m_pData->header.Srow_Z[i] = subString.toDouble(&convertSuccess);
        }
      }
      else if(typeString == "INTENT_CODE")
        mHeader.m_pData->header.Intent_Code = dataString.toInt(&convertSuccess);
      else if(typeString == "DATA_TYPE")
        mHeader.m_pData->header.DataType = dataString.toShort(&convertSuccess);
      else if(typeString == "BIT_PIX")
        mHeader.m_pData->header.Bit_Pix = dataString.toShort(&convertSuccess);
      else if(typeString == "SLICE_START")
        mHeader.m_pData->header.Slice_Start = dataString.toLongLong(&convertSuccess);
      else if(typeString == "SLICE_END")
        mHeader.m_pData->header.Slice_End = dataString.toLongLong(&convertSuccess);
      else if(typeString == "SCL_SLOPE")
        mHeader.m_pData->header.Scl_Slope = dataString.toDouble(&convertSuccess);
      else if(typeString == "SCL_INTER")
        mHeader.m_pData->header.Scl_Inter = dataString.toDouble(&convertSuccess);
      else if(typeString == "CAL_MAX")
        mHeader.m_pData->header.Cal_Max = dataString.toDouble(&convertSuccess);
      else if(typeString == "CAL_MIN")
        mHeader.m_pData->header.Cal_Min = dataString.toDouble(&convertSuccess);
      else if(typeString == "SLICE_DURATION")
        mHeader.m_pData->header.Slice_Duration = dataString.toDouble(&convertSuccess);
      else if(typeString == "TOFFSET")
        mHeader.m_pData->header.Toffset = dataString.toDouble(&convertSuccess);
      else if(typeString == "MAGIC")
        strncpy(mHeader.m_pData->header.Magic, dataString.toLatin1(), sizeof(mHeader.m_pData->header.Magic)-1);
      else if(typeString == "AUX_FILE")
        strncpy(mHeader.m_pData->header.Aux_File, dataString.toLatin1(), sizeof(mHeader.m_pData->header.Aux_File)-1);
      else if(typeString == "INTENT_NAME")
        strncpy(mHeader.m_pData->header.Intent_Name, dataString.toLatin1(), sizeof(mHeader.m_pData->header.Intent_Name)-1);
      else
      {
        E("'%s' - unknown header field.", typeString.toLatin1().constData());
      }

      if(convertSuccess == false)
      {
        E("'%s' - error while converting string '%s' to a numerical value.", typeString.toLatin1().constData(), dataString.toLatin1().constData());
      }
    }
  }

  RETURN(&stream);
  return stream;
}

//=============================================================================================
bool CNIFTI2MainHeader::save(void) const {
  ENTER();

  CMedIOData* mData = medIOData();

  // Check if the device is writable at all
  if(mData == NULL ||
     mData->isWritable() == false ||
     mData->seek(0) == false) {
    RETURN(false);
    return false;
  }

  SHOWVALUE(mData->pos());

  ASSERT(sizeof(m_pData->header) == MAINHEADER_SIZE);

  CNIFTIFile* niftiFile = static_cast<CNIFTIFile*>(mData);

  // work on a copy so that the fields required for a valid file
  // do not modify the header data itself
  struct CNIFTI2MainHeaderPrivate::HeaderData header;
  memcpy(&header, &m_pData->header, sizeof(header));

  // mandatory fields of a single file (.nii) NIfTI-2 header
  header.Sizeof_Hdr = MAINHEADER_SIZE;
  memcpy(header.Magic, NIFTI2_MAGIC, sizeof(header.Magic));
  if(header.Vox_Offset == 0)
    header.Vox_Offset = MAINHEADER_SIZE + 4;

//------------------------------------------------------------------------------------
  // the header is written in the byte order of its file
  if(m_pData->byteOrder != QSysInfo::ByteOrder)
    swapHeader(header);

//------------------------------------------------------------------------------------
  // Write out the main header to the file
  bool result = false;
  if(mData->write(reinterpret_cast<char*>(&header), sizeof(header)) == MAINHEADER_SIZE) {
    niftiFile->mainHeaderWritten(*this);
    result = true;
  }

  RETURN(result);
  return result;
}

//=============================================================================================
int CNIFTI2MainHeader::rawDataSize() const { 
  return 540; // 540 bytes for NIFTI2 main header
}

//=============================================================================================
CNIFTI2MainHeader::HeaderType CNIFTI2MainHeader::mainHeaderType() const { 
  return (CNIFTIMainHeader::HeaderType)2;
}

//=============================================================================================
// Convert from a MedIOHeader to a NIFTI2MainHeader
bool CNIFTI2MainHeader::convertFrom(const CMedIOHeader* mainHeader, const CMedIOHeader* subHeader) {
  ENTER();
  bool bResult = false;

  // Clear existing NIFTI header data before converting
  clear();

  // Depending on the MedIOHeader format we distinguish the copy operations
  switch(mainHeader->headerFormat()) {

    // Conversion from ECAT main header to NIFTI2 main header
    case CMedIOHeader::ECATMainHeader:  
    {
      const CECATMainHeader* eMainHeader = static_cast<const CECATMainHeader*>(mainHeader);

      // To extract the initial bed offset from the ECAT header (in centimeters)
      float initBedPosition = 0.0f;
      if (eMainHeader != NULL) {
          
          const CECAT7MainHeader* e7MainHeader = dynamic_cast<const CECAT7MainHeader*>(eMainHeader);
          if (e7MainHeader != NULL) {
              initBedPosition = e7MainHeader->init_Bed_Position();
          }
      }
      
      const CECAT7SubHeaderImage* eSubHeader = NULL;
      if (subHeader != NULL) {
          eSubHeader = static_cast<const CECAT7SubHeaderImage*>(subHeader);
      }

      // 1. Initialize mandatory structural fields for NIFTI-2 (540 bytes)
      clear(); 
      m_pData->header.Sizeof_Hdr = 540;
      m_pData->header.Vox_Offset = 544; // 540 bytes header + 4 bytes extension marker

      // NIFTI-2 magic signature (8 bytes: "n+2\0\r\n\x1a\n")
      memcpy(m_pData->header.Magic, NIFTI2_MAGIC, sizeof(m_pData->header.Magic));

      //double bedOffsetMm = static_cast<double>(initBedPosition) * 10.0;
      
      // 2. Map spatial and temporal dimensions (64-bit integers in NIFTI-2)
      qint64 numFrames = 1;

      if (eMainHeader != NULL && eMainHeader->num_Frames() > 1) {
          numFrames = eMainHeader->num_Frames();
      }

      // Static image -> 3D
      // Dynamic image -> 4D
      m_pData->header.Dim[0] = (numFrames > 1) ? 4 : 3;

      // Temporal dimension
      m_pData->header.Dim[4] = numFrames;

      // Unused dimensions
      m_pData->header.Dim[5] = 1;
      m_pData->header.Dim[6] = 1;
      m_pData->header.Dim[7] = 1;

      // Default spacing for temporal / unused dimensions
      m_pData->header.Pix_Dim[4] = 1.0;
      m_pData->header.Pix_Dim[5] = 1.0;
      m_pData->header.Pix_Dim[6] = 1.0;
      m_pData->header.Pix_Dim[7] = 1.0;      
      
      m_pData->header.Qform_Code = 1;
      m_pData->header.Sform_Code = 1;
      m_pData->header.Pix_Dim[0] = 1.0; // qfac (double precision)
      //m_pData->header.Qoffset_Z = bedOffsetMm;

      m_pData->header.Quatern_B = 0.0;
      m_pData->header.Quatern_C = 0.0;
      m_pData->header.Quatern_D = 1.0;

      if (eSubHeader != NULL) {
          m_pData->header.Dim[1] = eSubHeader->x_Dimension();
          m_pData->header.Dim[2] = eSubHeader->y_Dimension();
          m_pData->header.Dim[3] = eSubHeader->z_Dimension();
          
          double px = static_cast<double>(eSubHeader->x_Pixel_Size()) * 10.0;
          double py = static_cast<double>(eSubHeader->y_Pixel_Size()) * 10.0;
          double pz = static_cast<double>(eSubHeader->z_Pixel_Size()) * 10.0;
          
          m_pData->header.Pix_Dim[1] = px;
          m_pData->header.Pix_Dim[2] = py;
          m_pData->header.Pix_Dim[3] = pz;
          
          // Preserve the ECAT scale factor in the NIfTI header
          m_pData->header.Scl_Slope = static_cast<double>(eSubHeader->scale_Factor());

          m_pData->header.Scl_Inter = 0.0;

          // Extraction of native reconstruction offsets from the ECAT subheader,
          double xOffsetMm = 0.0;
          double yOffsetMm = 0.0;
          double zOffsetMm = static_cast<double>(initBedPosition) * 10.0;

          // ECAT7 stores offsets in centimeters -> convert to millimeters
          xOffsetMm = static_cast<double>(eSubHeader->x_Offset()) * 10.0;
          yOffsetMm = static_cast<double>(eSubHeader->y_Offset()) * 10.0;

          double subZOffset = static_cast<double>(eSubHeader->z_Offset()) * 10.0;

          // Prefer the subheader Z offset when available
          if (subZOffset != 0.0) {
              zOffsetMm = subZOffset;
          }

          // Qform translation
          m_pData->header.Qoffset_X = xOffsetMm;
          m_pData->header.Qoffset_Y = yOffsetMm;
          m_pData->header.Qoffset_Z = zOffsetMm;

          // =============================================================================================================
          // Sform matrix

          // X axis inverted
          m_pData->header.Srow_X[0] = -px; m_pData->header.Srow_X[1] = 0.0; m_pData->header.Srow_X[2] = 0.0; m_pData->header.Srow_X[3] = xOffsetMm;
          // Y axis inverted
          m_pData->header.Srow_Y[0] = 0.0; m_pData->header.Srow_Y[1] = -py; m_pData->header.Srow_Y[2] = 0.0; m_pData->header.Srow_Y[3] = yOffsetMm;
          // Z axis mapping
          m_pData->header.Srow_Z[0] = 0.0; m_pData->header.Srow_Z[1] = 0.0; m_pData->header.Srow_Z[2] = pz; m_pData->header.Srow_Z[3] = zOffsetMm;

          // 3. Map data type
          short dt = eSubHeader->data_Type();
          if (dt == CECATSubHeader::SunShort || dt == CECATSubHeader::VAX_Ix2) {
              m_pData->header.DataType = 4; // 16-bit signed integer
              m_pData->header.Bit_Pix = 16;
          } else if (dt == CECATSubHeader::IEEEFloat || dt == CECATSubHeader::VAX_Rx4) {
              m_pData->header.DataType = 16; // 32-bit float
              m_pData->header.Bit_Pix = 32;
          }
      }

      bResult = true; 
    }
    break;

    // Conversion from Concorde MicroPET main header to NIFTI2 main header
    case CMedIOHeader::ConcordeMicroPetMainHeader:
    {
      const CConcordeMainHeader* head = static_cast<const CConcordeMainHeader*>(mainHeader);
      const CConcordeFrameHeader* frame = static_cast<const CConcordeFrameHeader*>(subHeader);

      m_pData->header.Sizeof_Hdr = 540;
      m_pData->header.Vox_Offset = 544.0;
      memcpy(m_pData->header.Magic, NIFTI2_MAGIC, sizeof(m_pData->header.Magic));

      m_pData->header.Dim[0] = 3; 
      m_pData->header.Dim[1] = head->xDimension();
      m_pData->header.Dim[2] = head->yDimension();
      m_pData->header.Dim[3] = head->zDimension();

      m_pData->header.Pix_Dim[0] = 1.0; 
      m_pData->header.Pix_Dim[1] = static_cast<double>(head->pixelSize());
      m_pData->header.Pix_Dim[2] = static_cast<double>(head->pixelSize());
      m_pData->header.Pix_Dim[3] = static_cast<double>(head->axialPlaneSize());

      m_pData->header.Qform_Code = 1;
      m_pData->header.Quatern_B = 0.0;
      m_pData->header.Quatern_C = 0.0;
      m_pData->header.Quatern_D = 0.0;
      
      if(frame) {
          m_pData->header.Qoffset_X = 0.0; 
          m_pData->header.Qoffset_Y = static_cast<double>(frame->verticalBedOffset());
          m_pData->header.Qoffset_Z = static_cast<double>(frame->bedOffset());
      }

      strncpy(m_pData->header.Descrip, head->study().toLatin1().constData(), sizeof(m_pData->header.Descrip)-1);
      bResult = true;
    }
    break;

    // Conversion from Philips main header to NIFTI2 main header
    case CMedIOHeader::PhilipsMainHeader:
    {
      const CPhilipsMainHeader* head = static_cast<const CPhilipsMainHeader*>(mainHeader);
      const CPhilipsSubHeaderImage* subHead = static_cast<const CPhilipsSubHeaderImage*>(subHeader);

      m_pData->header.Sizeof_Hdr = 540;
      m_pData->header.Vox_Offset = 544.0;
      memcpy(m_pData->header.Magic, NIFTI2_MAGIC, sizeof(m_pData->header.Magic));

      m_pData->header.Dim[0] = 4; 
      m_pData->header.Dim[3] = head->nslice();
      m_pData->header.Dim[4] = head->nframe();

      m_pData->header.Pix_Dim[0] = 1.0; 
      m_pData->header.Pix_Dim[3] = static_cast<double>(head->Dslice_thick()) / 10.0;

      m_pData->header.Qform_Code = 1;
      
      if(subHead) {
          m_pData->header.Qoffset_Z = subHead->Dslice_loc() != 0.0f ? 
                                      static_cast<double>(subHead->Dslice_loc()) / 10.0 : 
                                      static_cast<double>(subHead->img_pos_z()) / 10.0;
      }

      strncpy(m_pData->header.Descrip, head->series_desc(), sizeof(m_pData->header.Descrip)-1);
      bResult = true;
    }
    break;

    case CMedIOHeader::Unknown:
    case CMedIOHeader::ECATSubHeader:
    case CMedIOHeader::PhilipsSubHeader:
    case CMedIOHeader::ConcordeMicroPetFrameHeader:
    case CMedIOHeader::PhilipsListviewHeader:
      E("medio mainheader %d conversion not implemented or invalid!", mainHeader->headerFormat());
    break;
  }

  RETURN(bResult);
  return bResult;
}
  
 
//=============================================================================================
// Clone
CMedIOHeader* CNIFTI2MainHeader::clone() const
{
  ENTER();
  CNIFTI2MainHeader* pNewHeader = new CNIFTI2MainHeader(*this);
  RETURN(pNewHeader);
  return pNewHeader;
}

//=============================================================================================
// CNIFTI2MainHeader accessors

// Getters
qint32 CNIFTI2MainHeader::sizeof_Hdr(void) const {
  return m_pData->header.Sizeof_Hdr;
}

qint64 CNIFTI2MainHeader::dim(const short index) const {
  if(index >= 0 && index <= 7) return m_pData->header.Dim[index];
  return 0;
}

double CNIFTI2MainHeader::pix_Dim(const short index) const {
  if(index >= 0 && index <= 7) return m_pData->header.Pix_Dim[index];
  return 0.0;
}

qint64 CNIFTI2MainHeader::vox_Offset(void) const { 
  return m_pData->header.Vox_Offset; 
}

qint32 CNIFTI2MainHeader::qform_Code(void) const { 
  return m_pData->header.Qform_Code; 
}

qint32 CNIFTI2MainHeader::sform_Code(void) const { 
  return m_pData->header.Sform_Code; 
}

double CNIFTI2MainHeader::qoffset_X(void) const { 
  return m_pData->header.Qoffset_X; 
}

double CNIFTI2MainHeader::qoffset_Y(void) const { 
  return m_pData->header.Qoffset_Y; 
}

double CNIFTI2MainHeader::qoffset_Z(void) const { 
  return m_pData->header.Qoffset_Z; 
}

const char* CNIFTI2MainHeader::descrip(void) const {
  return m_pData->header.Descrip;
}

const char* CNIFTI2MainHeader::magic(void) const {
  return m_pData->header.Magic;
}

qint16 CNIFTI2MainHeader::bit_Pix(void) const { 
  return m_pData->header.Bit_Pix; 
}

double CNIFTI2MainHeader::srow_X(const short index) const {
  if(index >= 0 && index < 4)
    return m_pData->header.Srow_X[index];
  return 0;
}

double CNIFTI2MainHeader::srow_Y(const short index) const {
  if(index >= 0 && index < 4)
    return m_pData->header.Srow_Y[index];
  return 0;
}

double CNIFTI2MainHeader::srow_Z(const short index) const {
  if(index >= 0 && index < 4)
    return m_pData->header.Srow_Z[index];
  return 0;
}

double CNIFTI2MainHeader::scl_Slope(void) const { 
  return m_pData->header.Scl_Slope; 
}

double CNIFTI2MainHeader::scl_Inter(void) const { 
  return m_pData->header.Scl_Inter; 
}

qint16 CNIFTI2MainHeader::dataType(void) const { 
  return m_pData->header.DataType; 
}

double CNIFTI2MainHeader::intent_P1(void) const { 
  return m_pData->header.Intent_P1; 
}

double CNIFTI2MainHeader::intent_P2(void) const { 
  return m_pData->header.Intent_P2; 
}

double CNIFTI2MainHeader::intent_P3(void) const { 
  return m_pData->header.Intent_P3; 
}

double CNIFTI2MainHeader::cal_Max(void) const { 
  return m_pData->header.Cal_Max; 
}

double CNIFTI2MainHeader::cal_Min(void) const { 
  return m_pData->header.Cal_Min; 
}
double CNIFTI2MainHeader::slice_Duration(void) const { 
  return m_pData->header.Slice_Duration; 
}

double CNIFTI2MainHeader::toffset(void) const { 
  return m_pData->header.Toffset; 
}

qint64 CNIFTI2MainHeader::slice_Start(void) const { 
  return m_pData->header.Slice_Start; 
}

qint64 CNIFTI2MainHeader::slice_End(void) const { 
  return m_pData->header.Slice_End; 
}

const char* CNIFTI2MainHeader::aux_File(void) const { 
  return m_pData->header.Aux_File; 
}

double CNIFTI2MainHeader::quatern_B(void) const { 
  return m_pData->header.Quatern_B; 
}

double CNIFTI2MainHeader::quatern_C(void) const { 
  return m_pData->header.Quatern_C; 
}

double CNIFTI2MainHeader::quatern_D(void) const { 
  return m_pData->header.Quatern_D; 
}

qint32 CNIFTI2MainHeader::slice_Code(void) const { 
  return m_pData->header.Slice_Code; 
}

qint32 CNIFTI2MainHeader::xyzt_Units(void) const { 
  return m_pData->header.XYZT_Units; 
}

qint32 CNIFTI2MainHeader::intent_Code(void) const { 
  return m_pData->header.Intent_Code; 
}

const char* CNIFTI2MainHeader::intent_Name(void) const { 
  return m_pData->header.Intent_Name; 
}

char CNIFTI2MainHeader::dim_Info(void) const { 
  return m_pData->header.Dim_Info; 
}

//=============================================================================================
// Setter Methods
void CNIFTI2MainHeader::setSizeof_Hdr(const qint32 size) {
  m_pData->header.Sizeof_Hdr = size;
}

void CNIFTI2MainHeader::setDim(const short index, const qint64 value) {
  if(index >= 0 && index <= 7) m_pData->header.Dim[index] = value;
}

void CNIFTI2MainHeader::setPix_Dim(const short index, const double value) {
  if(index >= 0 && index <= 7) m_pData->header.Pix_Dim[index] = value;
}

void CNIFTI2MainHeader::setQform_Code(const qint32 code) { m_pData->header.Qform_Code = code; }
void CNIFTI2MainHeader::setSform_Code(const qint32 code) { m_pData->header.Sform_Code = code; }
void CNIFTI2MainHeader::setQoffset_X(const double offset) { m_pData->header.Qoffset_X = offset; }
void CNIFTI2MainHeader::setQoffset_Y(const double offset) { m_pData->header.Qoffset_Y = offset; }
void CNIFTI2MainHeader::setQoffset_Z(const double offset) { m_pData->header.Qoffset_Z = offset; }
void CNIFTI2MainHeader::setQuatern_B(const double val) { m_pData->header.Quatern_B = val; }
void CNIFTI2MainHeader::setQuatern_C(const double val) { m_pData->header.Quatern_C = val; }
void CNIFTI2MainHeader::setQuatern_D(const double val) { m_pData->header.Quatern_D = val; }

void CNIFTI2MainHeader::setDescrip(const char* desc) {
  strncpy(m_pData->header.Descrip, desc, sizeof(m_pData->header.Descrip)-1);
  m_pData->header.Descrip[sizeof(m_pData->header.Descrip)-1] = '\0'; // Ensure null-termination
}

void CNIFTI2MainHeader::setMagic(const char* magic) {
  // the NIfTI-2 magic consists of a 4 byte identifier ("n+2\0" or "ni2\0")
  // followed by the fixed sequence "\r\n\032\n"
  memset(m_pData->header.Magic, 0, sizeof(m_pData->header.Magic));
  strncpy(m_pData->header.Magic, magic, 3);
  memcpy(m_pData->header.Magic+4, NIFTI2_MAGIC+4, 4);
}

void CNIFTI2MainHeader::setScl_Slope(const double slope) { m_pData->header.Scl_Slope = slope; }
void CNIFTI2MainHeader::setScl_Inter(const double inter) { m_pData->header.Scl_Inter = inter; }

void CNIFTI2MainHeader::setDataType(const qint16 dataType) { m_pData->header.DataType = dataType; }
void CNIFTI2MainHeader::setBit_Pix(const qint16 bitPix) { m_pData->header.Bit_Pix = bitPix; }
void CNIFTI2MainHeader::setIntent_P1(const double p1) { m_pData->header.Intent_P1 = p1; }
void CNIFTI2MainHeader::setIntent_P2(const double p2) { m_pData->header.Intent_P2 = p2; }
void CNIFTI2MainHeader::setIntent_P3(const double p3) { m_pData->header.Intent_P3 = p3; }
void CNIFTI2MainHeader::setCal_Max(const double max) { m_pData->header.Cal_Max = max; }
void CNIFTI2MainHeader::setCal_Min(const double min) { m_pData->header.Cal_Min = min; }
void CNIFTI2MainHeader::setSlice_Duration(const double duration) { m_pData->header.Slice_Duration = duration; }
void CNIFTI2MainHeader::setToffset(const double toffset) { m_pData->header.Toffset = toffset; }
void CNIFTI2MainHeader::setSlice_Start(const qint64 start) { m_pData->header.Slice_Start = start; }
void CNIFTI2MainHeader::setSlice_End(const qint64 end) { m_pData->header.Slice_End = end; }

void CNIFTI2MainHeader::setAux_File(const char* auxFile) {
  strncpy(m_pData->header.Aux_File, auxFile, sizeof(m_pData->header.Aux_File)-1);
  m_pData->header.Aux_File[sizeof(m_pData->header.Aux_File)-1] = '\0';
}

void CNIFTI2MainHeader::setSlice_Code(const qint32 code) { m_pData->header.Slice_Code = code; }
void CNIFTI2MainHeader::setXyzt_Units(const qint32 units) { m_pData->header.XYZT_Units = units; }
void CNIFTI2MainHeader::setIntent_Code(const qint32 code) { m_pData->header.Intent_Code = code; }

void CNIFTI2MainHeader::setIntent_Name(const char* intentName) {
  strncpy(m_pData->header.Intent_Name, intentName, sizeof(m_pData->header.Intent_Name)-1);
  m_pData->header.Intent_Name[sizeof(m_pData->header.Intent_Name)-1] = '\0';
}

void CNIFTI2MainHeader::setDim_Info(const char dimInfo) { m_pData->header.Dim_Info = dimInfo; }

void CNIFTI2MainHeader::setSrow_X(const short index, const double value) {
  if(index >= 0 && index < 4) m_pData->header.Srow_X[index] = value;
}
void CNIFTI2MainHeader::setSrow_Y(const short index, const double value) {
  if(index >= 0 && index < 4) m_pData->header.Srow_Y[index] = value;
}
void CNIFTI2MainHeader::setSrow_Z(const short index, const double value) {
  if(index >= 0 && index < 4) m_pData->header.Srow_Z[index] = value;
}

void CNIFTI2MainHeader::setVox_Offset(const qint64 offset) { m_pData->header.Vox_Offset = offset; }
//=============================================================================================
QTextStream& operator<<(QTextStream& stream, const CNIFTI2MainHeader& mHeader) {
  ENTER();

  stream << qSetRealNumberPrecision(6)
         << "SIZEOF_HDR " << mHeader.m_pData->header.Sizeof_Hdr << Qt::endl
         << "MAGIC "      << mHeader.m_pData->header.Magic      << Qt::endl
         << "DATA_TYPE "  << mHeader.m_pData->header.DataType   << Qt::endl
         << "BIT_PIX "    << mHeader.m_pData->header.Bit_Pix    << Qt::endl
         << "DESCRIP "    << mHeader.m_pData->header.Descrip    << Qt::endl
         << "VOX_OFFSET " << mHeader.m_pData->header.Vox_Offset << Qt::endl;

  stream << "DIM";
  for(int i=0; i < 8; i++)
    stream << " " << mHeader.m_pData->header.Dim[i];
  stream << Qt::endl;

  stream << "PIX_DIM";
  for(int i=0; i < 8; i++)
    stream << " " << mHeader.m_pData->header.Pix_Dim[i];
  stream << Qt::endl;

  stream << "INTENT_P1 "  << mHeader.m_pData->header.Intent_P1 << Qt::endl
         << "INTENT_P2 "  << mHeader.m_pData->header.Intent_P2 << Qt::endl
         << "INTENT_P3 "  << mHeader.m_pData->header.Intent_P3 << Qt::endl
         << "INTENT_CODE " << mHeader.m_pData->header.Intent_Code << Qt::endl
         << "INTENT_NAME " << mHeader.m_pData->header.Intent_Name << Qt::endl;

  stream << "SCL_SLOPE "  << mHeader.m_pData->header.Scl_Slope << Qt::endl
         << "SCL_INTER "  << mHeader.m_pData->header.Scl_Inter << Qt::endl
         << "CAL_MAX "    << mHeader.m_pData->header.Cal_Max << Qt::endl
         << "CAL_MIN "    << mHeader.m_pData->header.Cal_Min << Qt::endl
         << "SLICE_START " << mHeader.m_pData->header.Slice_Start << Qt::endl
         << "SLICE_END "  << mHeader.m_pData->header.Slice_End << Qt::endl
         << "SLICE_CODE " << mHeader.m_pData->header.Slice_Code << Qt::endl
         << "SLICE_DURATION " << mHeader.m_pData->header.Slice_Duration << Qt::endl
         << "TOFFSET "    << mHeader.m_pData->header.Toffset << Qt::endl
         << "XYZT_UNITS " << mHeader.m_pData->header.XYZT_Units << Qt::endl
         << "DIM_INFO "   << (int)mHeader.m_pData->header.Dim_Info << Qt::endl;

  stream << "QFORM_CODE " << mHeader.m_pData->header.Qform_Code << Qt::endl
         << "SFORM_CODE " << mHeader.m_pData->header.Sform_Code << Qt::endl
         << "QOFFSET_X "  << mHeader.m_pData->header.Qoffset_X  << Qt::endl
         << "QOFFSET_Y "  << mHeader.m_pData->header.Qoffset_Y  << Qt::endl
         << "QOFFSET_Z "  << mHeader.m_pData->header.Qoffset_Z  << Qt::endl
         << "QUATERN_B "  << mHeader.m_pData->header.Quatern_B  << Qt::endl
         << "QUATERN_C "  << mHeader.m_pData->header.Quatern_C  << Qt::endl
         << "QUATERN_D "  << mHeader.m_pData->header.Quatern_D  << Qt::endl;
  
  stream << "SROW_X";
  for(int i=0; i < 4; i++)
    stream << " " << mHeader.m_pData->header.Srow_X[i];
  stream << Qt::endl;

  stream << "SROW_Y";
  for(int i=0; i < 4; i++)
    stream << " " << mHeader.m_pData->header.Srow_Y[i];
  stream << Qt::endl;

  stream << "SROW_Z";
  for(int i=0; i < 4; i++)
    stream << " " << mHeader.m_pData->header.Srow_Z[i]; 
  stream << Qt::endl;
  
  stream << "AUX_FILE "    << mHeader.m_pData->header.Aux_File    << Qt::endl;

  RETURN(&stream);
  return stream;
}

//=============================================================================================
// Byte order

QSysInfo::Endian CNIFTI2MainHeader::byteOrder(void) const {
  return m_pData->byteOrder;
}

void CNIFTI2MainHeader::setByteOrder(QSysInfo::Endian order) {
  m_pData->byteOrder = order;
}

//=============================================================================================
// Header extension

// size in bytes of the header extension holding the given JSON metadata
int CNIFTI2MainHeader::headerExtensionSize(const QJsonObject& json) const {
  return jsonExtensionSize(json);
}

// Write the JSON metadata as header extension directly after the NIfTI-2 main header.
// vox_offset has to account for the extension (see headerExtensionSize()).
bool CNIFTI2MainHeader::writeHeaderExtension(CNIFTIFile& niftiFile, const QJsonObject& json) {
  return writeJsonExtension(niftiFile, MAINHEADER_SIZE, json, m_pData->byteOrder != QSysInfo::ByteOrder);
}

// Read the JSON metadata from the header extensions of the NIfTI-2 file
QJsonObject CNIFTI2MainHeader::readHeaderExtension(CNIFTIFile& file) const {
  return readJsonExtension(file, MAINHEADER_SIZE, static_cast<qint64>(m_pData->header.Vox_Offset),
                           m_pData->byteOrder != QSysInfo::ByteOrder);
}
