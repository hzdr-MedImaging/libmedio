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

#include "CNIFTI1MainHeader.h"
#include "CNIFTIFile.h"
#include "CNIFTI2MainHeader.h"
#include "CNIFTIHeaderCopy.h"
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
class CNIFTI1MainHeaderPrivate { // private class to add private variables to the CNIFTI1MainHeader class without exposing them in the public header file

  public:
    // MainHeader structure (348 bytes)

    #define MAINHEADER_SIZE 348
    #pragma pack(push, 1) // to ensure that the structure is packed without any padding bytes, which is important for reading/writing binary data from/to files

    struct HeaderData { // structure to represent the NIFTI-1 main header, which is 348 bytes long -> bytes of the header 

      quint32 Sizeof_Hdr;                 //   0: Sizeof_Header -> 348 for NIFTI 1 
      char    Data_Type[10];              //   4: Data_Type -> unused
      char    Db_Name[18];                //  14: Db_Name -> unused
      quint32 Extents;                    //  32: Extents -> unused
      quint16 Session_Error;              //  36: Session_Error -> unused
      char    Regular;                    //  38: Regular -> unused
      char    Dim_Info;                   //  39: Dim_Info -> MRI slice ordering

      quint16 Dim[8];                     //  40: Dim (8) -> data array for dimensions => number of voxels in each dimension (x, y, z, time, etc.)
      float   Intent_P1;                  //  56: Intent_P1

      float   Intent_P2;                  //  60: Intent_P2

      float   Intent_P3;                  //  64: Intent_P3

      quint16 Intent_Code;                //  68: Intent_Code 
      quint16 DataType;                  //  70: Data_Type -> defines the data type of the image data (unsigned char, signed short, float, etc.)
      quint16 Bit_Pix;                    //  72: Bit_Pix -> Number of bits/voxel (dimension of the data type in bits)
      quint16 Slice_Start;                //  74: Slice_Start -> first slice index (0, ..., N-1) for 3D+time data
      float   Pix_Dim[8];                 //  76: Pix_Dim (8)
      float   Vox_Offset;                 // 108: Vox_Offset -> offset into .nii file for image data
      float   Scl_Slope;                  // 112: Scl_Slope
      float   Scl_Inter;                  // 116: Scl_Inter
      quint16 Slice_End;                  // 120: Slice_End
      char    Slice_Code;                 // 122: Slice_Code
      char    XYZT_Units;                 // 123: XYZT_Units
      float   Cal_Max;                    // 124: Cal_Max
      float   Cal_Min;                    // 128: Cal_Min
      float   Slice_Duration;             // 132: Slice_Duration
      float   Toffset;                    // 136: Toffset
      quint32 Glmax;                      // 140: Glmax
      quint32 Glmin;                      // 144: Glmin

      char    Descrip[80];                // 148: Descrip
      char    Aux_File[24];               // 228: Aux_File
      quint16 Qform_Code;                 // 252: Qform_Code
      quint16 Sform_Code;                 // 253: Sform_Code
      float   Quatern_B;                  // 254: Quatern_B
      float   Quatern_C;                  // 258: Quatern_C
      float   Quatern_D;                  // 262: Quatern_D
      float   Qoffset_X;                  // 266: Qoffset_X
      float   Qoffset_Y;                  // 270: Qoffset_Y
      float   Qoffset_Z;                  // 274: Qoffset_Z

      float   Srow_X[4];                  // 278: Srow_X (4)
      float   Srow_Y[4];                  // 294: Srow_Y (4)
      float   Srow_Z[4];                  // 310: Srow_Z (4)
      char    Intent_Name[16];            // 326: Intent_Name
      char    Magic[4];                   // 342: Magic
    } header;

    // byte order of the file the header belongs to
    QSysInfo::Endian byteOrder;
    #pragma pack(pop) // restore the previous packing alignment
};

// NIfTI-1 single file (.nii) magic signature
static const char NIFTI1_MAGIC[4] = { 'n', '+', '1', '\0' };

// swap the byte order of all non-char elements of a NIfTI-1 header
static void swapHeader(struct CNIFTI1MainHeaderPrivate::HeaderData& header)
{
  BSWAP_32(header.Sizeof_Hdr);
  BSWAP_32(header.Extents);
  BSWAP_16(header.Session_Error);

  for(int i = 0; i < 8; i++) {
    BSWAP_16(header.Dim[i]);
    BSWAP_FLT(header.Pix_Dim[i]);
  }

  BSWAP_FLT(header.Intent_P1);
  BSWAP_FLT(header.Intent_P2);
  BSWAP_FLT(header.Intent_P3);
  BSWAP_16(header.Intent_Code);
  BSWAP_16(header.DataType);
  BSWAP_16(header.Bit_Pix);
  BSWAP_16(header.Slice_Start);
  BSWAP_FLT(header.Vox_Offset);
  BSWAP_FLT(header.Scl_Slope);
  BSWAP_FLT(header.Scl_Inter);
  BSWAP_16(header.Slice_End);
  BSWAP_FLT(header.Cal_Max);
  BSWAP_FLT(header.Cal_Min);
  BSWAP_FLT(header.Slice_Duration);
  BSWAP_FLT(header.Toffset);
  BSWAP_32(header.Glmax);
  BSWAP_32(header.Glmin);

  BSWAP_16(header.Qform_Code);
  BSWAP_16(header.Sform_Code);
  BSWAP_FLT(header.Quatern_B);
  BSWAP_FLT(header.Quatern_C);
  BSWAP_FLT(header.Quatern_D);
  BSWAP_FLT(header.Qoffset_X);
  BSWAP_FLT(header.Qoffset_Y);
  BSWAP_FLT(header.Qoffset_Z);

  for(int i = 0; i < 4; i++) {
    BSWAP_FLT(header.Srow_X[i]);
    BSWAP_FLT(header.Srow_Y[i]);
    BSWAP_FLT(header.Srow_Z[i]);
  }
}


//==============================================================================================
// Constructors
CNIFTI1MainHeader::CNIFTI1MainHeader(CNIFTIFile* niftiFile, CNIFTIMainHeader::Type fileType)  : CNIFTIMainHeader(niftiFile) {
  
  ENTER();

  // allocate data from our private instance class
  m_pData = new CNIFTI1MainHeaderPrivate();

  // this constructor signals us to create a empty NIFTI1MainHeader
  // with prefilled data that is always the same for a NIFTI1 main header
  // depending on the supplied fileType we have to have a different size of header field
  clear();

  //setFileType(fileType);

  LEAVE();
}
// Copy constructor
CNIFTI1MainHeader::CNIFTI1MainHeader(const CNIFTI1MainHeader& src)  : CNIFTIMainHeader(src) {
  ENTER();

  // allocate data from our private instance class
  m_pData = new CNIFTI1MainHeaderPrivate(*(src.m_pData));

  LEAVE();
}

// Default assignment operator
CNIFTI1MainHeader& CNIFTI1MainHeader::operator=(const CNIFTI1MainHeader& src) {
  ENTER();

  if(m_pData != src.m_pData) {
    memcpy(&m_pData->header, 
           &src.m_pData->header, 
           sizeof(struct CNIFTI1MainHeaderPrivate::HeaderData));
    m_pData->byteOrder = src.m_pData->byteOrder;
  }

  LEAVE();
  return *this;
}

// Destructor
CNIFTI1MainHeader::~CNIFTI1MainHeader() {
  ENTER();

  delete m_pData;

  LEAVE();
}

// Header clear method
void CNIFTI1MainHeader::clear() {
  ENTER();
  
  // clear our MainHeader structure first
  memset(&m_pData->header, 0, sizeof(struct CNIFTI1MainHeaderPrivate::HeaderData));

  // new files are written in little endian byte order
  m_pData->byteOrder = QSysInfo::LittleEndian;

  LEAVE();
}

//=============================================================================================
// load the main header from the file
bool CNIFTI1MainHeader::load(void) {

  D("Dimensione struct HeaderData: %lu", sizeof(m_pData->header));
  D("MAINHEADER_SIZE definito: %d", MAINHEADER_SIZE);
  //std::cout << "blah2:" << MAINHEADER_SIZE << ":" << sizeof(m_pData->header) << std::endl;
  ENTER();
  CMedIOData* mData = medIOData();

  // only go on if the device is readable at all
  if(mData == NULL ||
     mData->isReadable() == false ||
     mData->seek(0) == false) {
      //std::cout << "blah3" << std::endl;
      RETURN(false);
      return false;
  }

  // we read in all data at once using read()
  ASSERT(sizeof(m_pData->header) == MAINHEADER_SIZE);

  if(mData->read(reinterpret_cast<char*>(&m_pData->header), sizeof(m_pData->header)) != MAINHEADER_SIZE) {
    //std::cout << "blah4" << std::endl;
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
  D("NIFTI1 Main Header loaded:");
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
QTextStream& operator>>(QTextStream& stream, CNIFTI1MainHeader& mHeader) {
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
      else if(typeString == "DATA_TYPE_STR") // Use a different name to avoid matching the numeric Data_Type
        strncpy(mHeader.m_pData->header.Data_Type, dataString.toLatin1(), sizeof(mHeader.m_pData->header.Data_Type)-1);
      else if(typeString == "DESCRIP")
        strncpy(mHeader.m_pData->header.Descrip, dataString.toLatin1(), sizeof(mHeader.m_pData->header.Descrip)-1);
      else if(typeString == "DIM")
      {
        for(int i=0; i < 8 && convertSuccess; i++)
        {
          QString subString = dataString.section(" ", i, i);
          if(subString.isEmpty()) break;
          mHeader.m_pData->header.Dim[i] = subString.toShort(&convertSuccess);
        }
      }
      else if(typeString == "PIX_DIM")
      {
        for(int i=0; i < 8 && convertSuccess; i++)
        {
          QString subString = dataString.section(" ", i, i);
          if(subString.isEmpty()) break;
          mHeader.m_pData->header.Pix_Dim[i] = subString.toFloat(&convertSuccess);
        }
      }
      else if(typeString == "VOX_OFFSET")
        mHeader.m_pData->header.Vox_Offset = dataString.toFloat(&convertSuccess);
      
      else if(typeString == "QFORM_CODE")
        mHeader.m_pData->header.Qform_Code = dataString.toShort(&convertSuccess);
      else if(typeString == "SFORM_CODE")
        mHeader.m_pData->header.Sform_Code = dataString.toShort(&convertSuccess);
      else if(typeString == "QUATERN_B")
        mHeader.m_pData->header.Quatern_B = dataString.toFloat(&convertSuccess);
      else if(typeString == "QUATERN_C")
        mHeader.m_pData->header.Quatern_C = dataString.toFloat(&convertSuccess);
      else if(typeString == "QUATERN_D")
        mHeader.m_pData->header.Quatern_D = dataString.toFloat(&convertSuccess);
      else if(typeString == "QOFFSET_X")
        mHeader.m_pData->header.Qoffset_X = dataString.toFloat(&convertSuccess);
      else if(typeString == "QOFFSET_Y")
        mHeader.m_pData->header.Qoffset_Y = dataString.toFloat(&convertSuccess);
      else if(typeString == "QOFFSET_Z")
        mHeader.m_pData->header.Qoffset_Z = dataString.toFloat(&convertSuccess);
      else if(typeString == "SROW_X")
      {
        for(int i=0; i < 4 && convertSuccess; i++)
        {
          QString subString = dataString.section(" ", i, i);
          if(subString.isEmpty()) break;
          mHeader.m_pData->header.Srow_X[i] = subString.toFloat(&convertSuccess);
        }
      }
      else if(typeString == "SROW_Y")
      {
        for(int i=0; i < 4 && convertSuccess; i++)
        {
          QString subString = dataString.section(" ", i, i);
          if(subString.isEmpty()) break;
          mHeader.m_pData->header.Srow_Y[i] = subString.toFloat(&convertSuccess);
        }
      }
      else if(typeString == "SROW_Z")
      {
        for(int i=0; i < 4 && convertSuccess; i++)
        {
          QString subString = dataString.section(" ", i, i);
          if(subString.isEmpty()) break;
          mHeader.m_pData->header.Srow_Z[i] = subString.toFloat(&convertSuccess);
        }
      }
      else if(typeString == "MAGIC")
        strncpy(mHeader.m_pData->header.Magic, dataString.toLatin1(), sizeof(mHeader.m_pData->header.Magic)-1);
      else
      {
        E("'%s' - unknown header field.", typeString.toLatin1().constData());
        convertSuccess = false;
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
bool CNIFTI1MainHeader::save(void) const {
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
  struct CNIFTI1MainHeaderPrivate::HeaderData header;
  memcpy(&header, &m_pData->header, sizeof(header));

  // mandatory fields of a single file (.nii) NIfTI-1 header
  header.Sizeof_Hdr = MAINHEADER_SIZE;
  memcpy(header.Magic, NIFTI1_MAGIC, sizeof(header.Magic));
  if(header.Vox_Offset == 0.0f)
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
int CNIFTI1MainHeader::rawDataSize() const { 
  return 348; // Size of the NIFTI-1 writeMatrix(const QByteArray& matrixData) (emain header in bytes
}

//=============================================================================================
CNIFTI1MainHeader::HeaderType CNIFTI1MainHeader::mainHeaderType() const { 
  return (CNIFTIMainHeader::HeaderType)1;
}

//=============================================================================================
// Convert from a MedIOHeader to a NIFTI1MainHeader
bool CNIFTI1MainHeader::convertFrom(const CMedIOHeader* mainHeader, const CMedIOHeader* subHeader) {
  ENTER();
  bool bResult = false;

  // Clear existing NIFTI header data before converting
  clear();

  // Depending on the MedIOHeader format we distinguish the copy operations
  switch(mainHeader->headerFormat()) {

    // Conversion from ECAT main header to NIFTI1 main header
    case CMedIOHeader::ECATMainHeader: {

      const CECATMainHeader* eMainHeader = static_cast<const CECATMainHeader*>(mainHeader);
      // Exctract the offset of the bed position from the ECAT main header, if available
      float initBedPosition = 0.0f;
      if (eMainHeader != NULL) {
          //m_pData->header.Dim[4] = eMainHeader->num_Frames();
          
          const CECAT7MainHeader* e7MainHeader = dynamic_cast<const CECAT7MainHeader*>(eMainHeader);

          if (e7MainHeader != NULL) {
              initBedPosition = e7MainHeader->init_Bed_Position();
          }
      }

      const CECAT7SubHeaderImage* eSubHeader = NULL;
      if (subHeader != NULL) {
          eSubHeader = static_cast<const CECAT7SubHeaderImage*>(subHeader);
      }

      // 1. Initialize the mandatory structural fields of the NIfTI header
      clear(); 
      m_pData->header.Sizeof_Hdr = 348;
      m_pData->header.Vox_Offset = 352.0f;
      memcpy(m_pData->header.Magic, NIFTI1_MAGIC, sizeof(m_pData->header.Magic));

      // 2. Map spatial and temporal dimensions

      short numFrames = 1;

      if (eMainHeader != NULL && eMainHeader->num_Frames() > 1) {
          numFrames = eMainHeader->num_Frames();
      }

      // Static PET image -> 3D
      // Dynamic PET image -> 4D
      m_pData->header.Dim[0] = (numFrames > 1) ? 4 : 3;

      m_pData->header.Dim[4] = numFrames;

      // Unused dimensions must be 1
      m_pData->header.Dim[5] = 1;
      m_pData->header.Dim[6] = 1;
      m_pData->header.Dim[7] = 1;

      // Default spacing for unused/temporal dimensions
      m_pData->header.Pix_Dim[4] = 1.0f;
      m_pData->header.Pix_Dim[5] = 1.0f;
      m_pData->header.Pix_Dim[6] = 1.0f;
      m_pData->header.Pix_Dim[7] = 1.0f;


      // =============================================================================================================
      // Set Qform and Sform codes to indicate scanner anatomical coordinates and affine transformation, respectively.
      m_pData->header.Qform_Code = 1; // Scanner anatomical (Method 2)
      m_pData->header.Sform_Code = 1; // Affine Transformation (Method 3)
      //m_pData->header.Qoffset_Z = initBedPosition * 10.0f;

      // =============================================================================================================
      // Qform matrix Method 2: Scanner anatomical coordinates
      //m_pData->header.Pix_Dim[0] = 1.0f; // qfac --> 1.0 for right-handed coordinate system
      m_pData->header.Pix_Dim[0] = 1.0f;  // for left-handed coordinate system (ECAT uses left-handed coordinates)
      
      // Rotation of X and Y: Quaternion [a, b, c, d] for 180-degree rotation around Z-axis
      m_pData->header.Quatern_B = 0.0f;
      m_pData->header.Quatern_C = 0.0f;
      m_pData->header.Quatern_D = 1.0f;
      // =============================================================================================================

      if (eSubHeader != NULL) {
          m_pData->header.Dim[1] = eSubHeader->x_Dimension();
          m_pData->header.Dim[2] = eSubHeader->y_Dimension();
          m_pData->header.Dim[3] = eSubHeader->z_Dimension();
          
          float px = eSubHeader->x_Pixel_Size() * 10.0f; // mm
          float py = eSubHeader->y_Pixel_Size() * 10.0f; 
          float pz = eSubHeader->z_Pixel_Size() * 10.0f; 
          
          m_pData->header.Pix_Dim[1] = px;
          m_pData->header.Pix_Dim[2] = py;
          m_pData->header.Pix_Dim[3] = pz;

          // Extract the scale factor slc_slope from ECAT subheader and pass it to the NIfTI header
          m_pData->header.Scl_Slope = eSubHeader->scale_Factor();
          
          // Extraction of native reconstruction offsets from the ECAT subheader for absolute bit-identity
          float xOffsetMm = 0.0f;
          float yOffsetMm = 0.0f;
          float zOffsetMm = initBedPosition * 10.0f;

          if (eSubHeader != NULL) {
              // ECAT7 stores offsets in centimeters, convert them to millimeters (* 10.0f)
              xOffsetMm = eSubHeader->x_Offset() * 10.0f;
              yOffsetMm = eSubHeader->y_Offset() * 10.0f;
              float subZOffset = eSubHeader->z_Offset() * 10.0f;
              if (subZOffset != 0.0f) {
                  zOffsetMm = subZOffset;
              }
          }

          m_pData->header.Qoffset_X = xOffsetMm;
          m_pData->header.Qoffset_Y = yOffsetMm;
          m_pData->header.Qoffset_Z = zOffsetMm;

          // =============================================================================================================
          // Sform matrix Method 3: Affine transformation using the native reconstruction offsets

          // X axis inverted
          m_pData->header.Srow_X[0] = -px;  m_pData->header.Srow_X[1] = 0.0f; m_pData->header.Srow_X[2] = 0.0f; m_pData->header.Srow_X[3] = xOffsetMm; 
          // Y axis inverted
          m_pData->header.Srow_Y[0] = 0.0f; m_pData->header.Srow_Y[1] = -py; m_pData->header.Srow_Y[2] = 0.0f; m_pData->header.Srow_Y[3] = yOffsetMm; 
          // Z axis mapping
          m_pData->header.Srow_Z[0] = 0.0f; m_pData->header.Srow_Z[1] = 0.0f; m_pData->header.Srow_Z[2] = pz; m_pData->header.Srow_Z[3] = zOffsetMm;
          // =============================================================================================================
          
          // 3. Map the data type (Create a mapping from ECAT data types to NIfTI data types)
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

    // Conversion from Concorde MicroPET main header to NIFTI1 main header
    case CMedIOHeader::ConcordeMicroPetMainHeader:
    {
      const CConcordeMainHeader* head = static_cast<const CConcordeMainHeader*>(mainHeader);
      const CConcordeFrameHeader* frame = static_cast<const CConcordeFrameHeader*>(subHeader);

      m_pData->header.Dim[0] = 3; // 3D dimensions
      m_pData->header.Dim[1] = head->xDimension(); // Assuming these getters exist in CConcordeMainHeader
      m_pData->header.Dim[2] = head->yDimension();
      m_pData->header.Dim[3] = head->zDimension();

      m_pData->header.Pix_Dim[0] = 1.0f; // qfac
      m_pData->header.Pix_Dim[1] = head->pixelSize();
      m_pData->header.Pix_Dim[2] = head->pixelSize();
      m_pData->header.Pix_Dim[3] = head->axialPlaneSize(); // Z plane separation
 
      // Set Qform to scanner anatomical and apply translation offset
      m_pData->header.Qform_Code = 1;
      m_pData->header.Quatern_B = 0.0f;
      m_pData->header.Quatern_C = 0.0f;
      m_pData->header.Quatern_D = 0.0f;
      
      // Calculate spatial offsets (DICOM translation logic)
      if(frame) {
          m_pData->header.Qoffset_X = 0.0f; 
          m_pData->header.Qoffset_Y = frame->verticalBedOffset();
          m_pData->header.Qoffset_Z = frame->bedOffset();
      }

      strncpy(m_pData->header.Descrip, head->study().toLatin1().constData(), sizeof(m_pData->header.Descrip)-1);
      bResult = true;
    }
    break;

    // Conversion from Philips main header to NIFTI1 main header
    case CMedIOHeader::PhilipsMainHeader:
    {
      const CPhilipsMainHeader* head = static_cast<const CPhilipsMainHeader*>(mainHeader);
      const CPhilipsSubHeaderImage* subHead = static_cast<const CPhilipsSubHeaderImage*>(subHeader);

      m_pData->header.Dim[0] = 4; // 3D + Time (Frames)
      m_pData->header.Dim[3] = head->nslice();
      m_pData->header.Dim[4] = head->nframe();

      m_pData->header.Pix_Dim[0] = 1.0f; // qfac
      m_pData->header.Pix_Dim[3] = head->Dslice_thick() / 10.0f; // mm -> cm

      m_pData->header.Qform_Code = 1;
      
      if(subHead) {
          m_pData->header.Qoffset_Z = subHead->Dslice_loc() != 0.0f ? 
                                      subHead->Dslice_loc() / 10.0f : 
                                      subHead->img_pos_z() / 10.0f;
      }

      strncpy(m_pData->header.Descrip, head->series_desc(), sizeof(m_pData->header.Descrip)-1);
      bResult = true;
    }
    break;

    // Conversion from a NIfTI-1/NIfTI-2 main header
    case CMedIOHeader::NIFTIMainHeader:
    {
      const CNIFTIMainHeader* niftiHeader = static_cast<const CNIFTIMainHeader*>(mainHeader);

      if(niftiHeader->mainHeaderType() == CNIFTIMainHeader::NIFTI1MainHeader) {
        *this = *static_cast<const CNIFTI1MainHeader*>(niftiHeader);

        // a converted header always describes a new (little endian) file
        setByteOrder(QSysInfo::LittleEndian);
        bResult = true;
      }
      else if(niftiHeader->mainHeaderType() == CNIFTIMainHeader::NIFTI2MainHeader) {
        bResult = copyNIFTIHeaderFields(*this, *static_cast<const CNIFTI2MainHeader*>(niftiHeader), 32767);

        if(bResult == false)
          E("NIfTI-2 header values exceed the limits of a NIfTI-1 header");
      }
    }
    break;

    // Conversion from unknown or unsupported main header types to NIFTI1 main header is not supported
    case CMedIOHeader::Unknown:
    case CMedIOHeader::ECATSubHeader:
    case CMedIOHeader::PhilipsSubHeader:
    case CMedIOHeader::ConcordeMicroPetFrameHeader:
    case CMedIOHeader::PhilipsListviewHeader:
      // Conversion from these sub-types to a NIFTI main header is not supported
      E("medio mainheader %d conversion not implemented or invalid!", mainHeader->headerFormat());
    break;
  }

  RETURN(bResult);
  return bResult;
}


//=============================================================================================
// Clone
CMedIOHeader* CNIFTI1MainHeader::clone() const
{
  ENTER();
  CNIFTI1MainHeader* pNewHeader = new CNIFTI1MainHeader(*this);
  RETURN(pNewHeader);
  return pNewHeader;
}

//=============================================================================================
// CNIFTI1MainHeader accessors

// Getters
qint32 CNIFTI1MainHeader::sizeof_Hdr(void) const {
  return m_pData->header.Sizeof_Hdr;
}

short CNIFTI1MainHeader::dim(const short index) const {
  if(index >= 0 && index <= 7)
    return m_pData->header.Dim[index];
  return 0;
}

float CNIFTI1MainHeader::pix_Dim(const short index) const {
  if(index >= 0 && index <= 7)
    return m_pData->header.Pix_Dim[index];
  return 0.0f;
}

float CNIFTI1MainHeader::vox_Offset(void) const {
  return m_pData->header.Vox_Offset;
}

short CNIFTI1MainHeader::qform_Code(void) const {
  return m_pData->header.Qform_Code;
}

short CNIFTI1MainHeader::sform_Code(void) const {
  return m_pData->header.Sform_Code;
}

float CNIFTI1MainHeader::qoffset_X(void) const { return m_pData->header.Qoffset_X; }
float CNIFTI1MainHeader::qoffset_Y(void) const { return m_pData->header.Qoffset_Y; }
float CNIFTI1MainHeader::qoffset_Z(void) const { return m_pData->header.Qoffset_Z; }

const char* CNIFTI1MainHeader::descrip(void) const {
  return m_pData->header.Descrip;
}

const char* CNIFTI1MainHeader::magic(void) const {
  return m_pData->header.Magic;
}

short CNIFTI1MainHeader::bit_Pix(void) const {
  return m_pData->header.Bit_Pix;
} 

float CNIFTI1MainHeader::srow_X(const short index) const {
  if(index >= 0 && index < 4)
    return m_pData->header.Srow_X[index];
  return 0;
} 

float CNIFTI1MainHeader::srow_Y(const short index) const {
  if(index >= 0 && index < 4)
    return m_pData->header.Srow_Y[index];
  return 0;
}

float CNIFTI1MainHeader::srow_Z(const short index) const {
  if(index >= 0 && index < 4)
    return m_pData->header.Srow_Z[index];
  return 0;
}

float CNIFTI1MainHeader::scl_Slope(void) const {
  return m_pData->header.Scl_Slope;
}

float CNIFTI1MainHeader::scl_Inter(void) const {
  return m_pData->header.Scl_Inter;
}

const char* CNIFTI1MainHeader::data_Type(void) const { 
  return m_pData->header.Data_Type; 
}

const char* CNIFTI1MainHeader::db_Name(void) const { 
  return m_pData->header.Db_Name; 
}

quint32 CNIFTI1MainHeader::extents(void) const { 
  return m_pData->header.Extents; 
}

short CNIFTI1MainHeader::session_Error(void) const { 
  return m_pData->header.Session_Error; 
}

char CNIFTI1MainHeader::regular(void) const { 
  return m_pData->header.Regular; 
}

char CNIFTI1MainHeader::dim_Info(void) const { 
  return m_pData->header.Dim_Info; 
}

float CNIFTI1MainHeader::intent_P1(void) const { 
  return m_pData->header.Intent_P1; 
}

float CNIFTI1MainHeader::intent_P2(void) const { 
  return m_pData->header.Intent_P2; 
}

float CNIFTI1MainHeader::intent_P3(void) const { 
  return m_pData->header.Intent_P3; 
}

short CNIFTI1MainHeader::intent_Code(void) const { 
  return m_pData->header.Intent_Code; 
}

short CNIFTI1MainHeader::dataType(void) const { 
  return m_pData->header.DataType; 
}

short CNIFTI1MainHeader::slice_Start(void) const { 
  return m_pData->header.Slice_Start; 
}

short CNIFTI1MainHeader::slice_End(void) const { 
  return m_pData->header.Slice_End; 
}

char CNIFTI1MainHeader::slice_Code(void) const { 
  return m_pData->header.Slice_Code;
}

char CNIFTI1MainHeader::xyzt_Units(void) const { 
  return m_pData->header.XYZT_Units; 
}

float CNIFTI1MainHeader::cal_Max(void) const { 
  return m_pData->header.Cal_Max; 
}

float CNIFTI1MainHeader::cal_Min(void) const { 
  return m_pData->header.Cal_Min; 
}

float CNIFTI1MainHeader::slice_Duration(void) const { 
  return m_pData->header.Slice_Duration; 
}

float CNIFTI1MainHeader::toffset(void) const { 
  return m_pData->header.Toffset; 
}

quint32 CNIFTI1MainHeader::glmax(void) const { 
  return m_pData->header.Glmax; 
}

quint32 CNIFTI1MainHeader::glmin(void) const { 
  return m_pData->header.Glmin; 
}

const char* CNIFTI1MainHeader::aux_File(void) const { 
  return m_pData->header.Aux_File; 
}

const char* CNIFTI1MainHeader::intent_Name(void) const { 
  return m_pData->header.Intent_Name;
}

float CNIFTI1MainHeader::quatern_B(void) const { 
  return m_pData->header.Quatern_B; 
}

float CNIFTI1MainHeader::quatern_C(void) const { 
  return m_pData->header.Quatern_C; 
}

float CNIFTI1MainHeader::quatern_D(void) const { 
  return m_pData->header.Quatern_D; 
}

//=============================================================================================
// Setter Methods
void CNIFTI1MainHeader::setSizeof_Hdr(const qint32 size) {
  m_pData->header.Sizeof_Hdr = size;
}

void CNIFTI1MainHeader::setDim(const short index, const short value) {
  if(index >= 0 && index <= 7)
    m_pData->header.Dim[index] = value;
}

void CNIFTI1MainHeader::setPix_Dim(const short index, const float value) {
  if(index >= 0 && index <= 7)
    m_pData->header.Pix_Dim[index] = value;
}

void CNIFTI1MainHeader::setQform_Code(const short code) {
  m_pData->header.Qform_Code = code;
}

void CNIFTI1MainHeader::setSform_Code(const short code) {
  m_pData->header.Sform_Code = code;
}

void CNIFTI1MainHeader::setQoffset_X(const float offset) { m_pData->header.Qoffset_X = offset; }
void CNIFTI1MainHeader::setQoffset_Y(const float offset) { m_pData->header.Qoffset_Y = offset; }
void CNIFTI1MainHeader::setQoffset_Z(const float offset) { m_pData->header.Qoffset_Z = offset; }

void CNIFTI1MainHeader::setQuatern_B(const float val) { m_pData->header.Quatern_B = val; }
void CNIFTI1MainHeader::setQuatern_C(const float val) { m_pData->header.Quatern_C = val; }
void CNIFTI1MainHeader::setQuatern_D(const float val) { m_pData->header.Quatern_D = val; }

void CNIFTI1MainHeader::setDescrip(const char* desc) {
  strncpy(m_pData->header.Descrip, desc, sizeof(m_pData->header.Descrip)-1);
  m_pData->header.Descrip[sizeof(m_pData->header.Descrip)-1] = '\0'; // Ensure null-termination
}

void CNIFTI1MainHeader::setMagic(const char* magic) {
  strncpy(m_pData->header.Magic, magic, sizeof(m_pData->header.Magic)-1);
  m_pData->header.Magic[sizeof(m_pData->header.Magic)-1] = '\0'; // Ensure null-termination
}

void CNIFTI1MainHeader::setScl_Slope(float slope) {
  m_pData->header.Scl_Slope = slope;
}

void CNIFTI1MainHeader::setScl_Inter(float inter) {
  m_pData->header.Scl_Inter = inter;
}

void CNIFTI1MainHeader::setData_Type(const char* dataType) {
  strncpy(m_pData->header.Data_Type, dataType, sizeof(m_pData->header.Data_Type)-1);
  m_pData->header.Data_Type[sizeof(m_pData->header.Data_Type)-1] = '\0';
}

void CNIFTI1MainHeader::setDb_Name(const char* dbName) {
  strncpy(m_pData->header.Db_Name, dbName, sizeof(m_pData->header.Db_Name)-1);
  m_pData->header.Db_Name[sizeof(m_pData->header.Db_Name)-1] = '\0';
}

void CNIFTI1MainHeader::setExtents(const quint32 extents) { m_pData->header.Extents = extents; }

void CNIFTI1MainHeader::setSession_Error(const short error) { m_pData->header.Session_Error = error; }

void CNIFTI1MainHeader::setRegular(const char regular) { m_pData->header.Regular = regular; }

void CNIFTI1MainHeader::setDim_Info(const char dimInfo) { m_pData->header.Dim_Info = dimInfo; }

void CNIFTI1MainHeader::setIntent_P1(const float p1) { m_pData->header.Intent_P1 = p1; }

void CNIFTI1MainHeader::setIntent_P2(const float p2) { m_pData->header.Intent_P2 = p2; }

void CNIFTI1MainHeader::setIntent_P3(const float p3) { m_pData->header.Intent_P3 = p3; }

void CNIFTI1MainHeader::setIntent_Code(const short code) { m_pData->header.Intent_Code = code; }

void CNIFTI1MainHeader::setDataType(const short dataType) { m_pData->header.DataType = dataType; }

void CNIFTI1MainHeader::setBit_Pix(const short bitPix) { m_pData->header.Bit_Pix = bitPix; }

void CNIFTI1MainHeader::setSlice_Start(const short start) { m_pData->header.Slice_Start = start; }

void CNIFTI1MainHeader::setVox_Offset(const float offset) { m_pData->header.Vox_Offset = offset; }

void CNIFTI1MainHeader::setSlice_End(const short end) { m_pData->header.Slice_End = end; }

void CNIFTI1MainHeader::setSlice_Code(const char code) { m_pData->header.Slice_Code = code; }

void CNIFTI1MainHeader::setXyzt_Units(const char units) { m_pData->header.XYZT_Units = units; }

void CNIFTI1MainHeader::setCal_Max(const float max) { m_pData->header.Cal_Max = max; }

void CNIFTI1MainHeader::setCal_Min(const float min) { m_pData->header.Cal_Min = min; }

void CNIFTI1MainHeader::setSlice_Duration(const float duration) { m_pData->header.Slice_Duration = duration; }

void CNIFTI1MainHeader::setToffset(const float toffset) { m_pData->header.Toffset = toffset; }

void CNIFTI1MainHeader::setGlmax(const quint32 glmax) { m_pData->header.Glmax = glmax; }

void CNIFTI1MainHeader::setGlmin(const quint32 glmin) { m_pData->header.Glmin = glmin; }

void CNIFTI1MainHeader::setAux_File(const char* auxFile) {
  strncpy(m_pData->header.Aux_File, auxFile, sizeof(m_pData->header.Aux_File)-1);
  m_pData->header.Aux_File[sizeof(m_pData->header.Aux_File)-1] = '\0';
}

void CNIFTI1MainHeader::setIntent_Name(const char* intentName) {
  strncpy(m_pData->header.Intent_Name, intentName, sizeof(m_pData->header.Intent_Name)-1);
  m_pData->header.Intent_Name[sizeof(m_pData->header.Intent_Name)-1] = '\0';
}

void CNIFTI1MainHeader::setSrow_X(const short index, const float value) {
  if(index >= 0 && index < 4) m_pData->header.Srow_X[index] = value;
}
void CNIFTI1MainHeader::setSrow_Y(const short index, const float value) {
  if(index >= 0 && index < 4) m_pData->header.Srow_Y[index] = value;
}

void CNIFTI1MainHeader::setSrow_Z(const short index, const float value) {
  if(index >= 0 && index < 4) m_pData->header.Srow_Z[index] = value;
}
//=============================================================================================
QTextStream& operator<<(QTextStream& stream, const CNIFTI1MainHeader& mHeader) {
  ENTER();

  stream << "SIZEOF_HDR " << mHeader.m_pData->header.Sizeof_Hdr << Qt::endl
         << "DATA_TYPE_STR " << mHeader.m_pData->header.Data_Type  << Qt::endl
         << "DB_NAME "    << mHeader.m_pData->header.Db_Name     << Qt::endl 
         << "EXTENTS "    << mHeader.m_pData->header.Extents     << Qt::endl
         << "SESSION_ERROR " << mHeader.m_pData->header.Session_Error << Qt::endl
         << "REGULAR "    << (int)mHeader.m_pData->header.Regular     << Qt::endl
         << "DIM_INFO "   << (int)mHeader.m_pData->header.Dim_Info    << Qt::endl;

  stream << "DIM";
  for(int i=0; i < 8; i++)
    stream << " " << mHeader.m_pData->header.Dim[i];
  stream << Qt::endl;
  
  stream << "INTENT_P1 "  << mHeader.m_pData->header.Intent_P1   << Qt::endl
         << "INTENT_P2 "  << mHeader.m_pData->header.Intent_P2   << Qt::endl
         << "INTENT_P3 "  << mHeader.m_pData->header.Intent_P3   << Qt::endl
         << "INTENT_CODE " << mHeader.m_pData->header.Intent_Code << Qt::endl
         << "DATA_TYPE "  << mHeader.m_pData->header.DataType   << Qt::endl
         << "BIT_PIX "    << mHeader.m_pData->header.Bit_Pix     << Qt::endl
         << "SLICE_START " << mHeader.m_pData->header.Slice_Start << Qt::endl;

  stream << "PIX_DIM";
  for(int i=0; i < 8; i++)
    stream << " " << mHeader.m_pData->header.Pix_Dim[i];
  stream << Qt::endl;

  stream << "VOX_OFFSET " << mHeader.m_pData->header.Vox_Offset << Qt::endl
         << "SCL_SLOPE "  << mHeader.m_pData->header.Scl_Slope  << Qt::endl
         << "SCL_INTER "  << mHeader.m_pData->header.Scl_Inter  << Qt::endl
         << "SLICE_END "  << mHeader.m_pData->header.Slice_End  << Qt::endl
         << "SLICE_CODE " << (int)mHeader.m_pData->header.Slice_Code << Qt::endl
         << "XYZT_UNITS " << (int)mHeader.m_pData->header.XYZT_Units << Qt::endl
         << "CAL_MAX "    << mHeader.m_pData->header.Cal_Max    << Qt::endl
         << "CAL_MIN "    << mHeader.m_pData->header.Cal_Min    << Qt::endl
         << "SLICE_DURATION " << mHeader.m_pData->header.Slice_Duration << Qt::endl
         << "TOFFSET "    << mHeader.m_pData->header.Toffset    << Qt::endl
         << "GLMAX "      << mHeader.m_pData->header.Glmax      << Qt::endl
         << "GLMIN "      << mHeader.m_pData->header.Glmin      << Qt::endl
         << "DESCRIP "    << mHeader.m_pData->header.Descrip    << Qt::endl
         << "AUX_FILE "   << mHeader.m_pData->header.Aux_File   << Qt::endl
         << "QFORM_CODE " << mHeader.m_pData->header.Qform_Code << Qt::endl
         << "SFORM_CODE " << mHeader.m_pData->header.Sform_Code << Qt::endl
         << "QUATERN_B "  << mHeader.m_pData->header.Quatern_B  << Qt::endl
         << "QUATERN_C "  << mHeader.m_pData->header.Quatern_C  << Qt::endl
         << "QUATERN_D "  << mHeader.m_pData->header.Quatern_D  << Qt::endl
         << "QOFFSET_X "  << mHeader.m_pData->header.Qoffset_X  << Qt::endl
         << "QOFFSET_Y "  << mHeader.m_pData->header.Qoffset_Y  << Qt::endl
         << "QOFFSET_Z "  << mHeader.m_pData->header.Qoffset_Z  << Qt::endl
         << "SROW_X "     << mHeader.m_pData->header.Srow_X[0] << " " << mHeader.m_pData->header.Srow_X[1] << " " << mHeader.m_pData->header.Srow_X[2] << " " << mHeader.m_pData->header.Srow_X[3] << Qt::endl
         << "SROW_Y "     << mHeader.m_pData->header.Srow_Y[0] << " " << mHeader.m_pData->header.Srow_Y[1] << " " << mHeader.m_pData->header.Srow_Y[2] << " " << mHeader.m_pData->header.Srow_Y[3] << Qt::endl
         << "SROW_Z "     << mHeader.m_pData->header.Srow_Z[0] << " " << mHeader.m_pData->header.Srow_Z[1] << " " << mHeader.m_pData->header.Srow_Z[2] << " " << mHeader.m_pData->header.Srow_Z[3] << Qt::endl
         << "INTENT_NAME " << mHeader.m_pData->header.Intent_Name << Qt::endl
         << "MAGIC "      << mHeader.m_pData->header.Magic << Qt::endl;

  RETURN(&stream);
  return stream;
}

//=============================================================================================
// Byte order

QSysInfo::Endian CNIFTI1MainHeader::byteOrder(void) const {
  return m_pData->byteOrder;
}

void CNIFTI1MainHeader::setByteOrder(QSysInfo::Endian order) {
  m_pData->byteOrder = order;
}

//=============================================================================================
// Header extension

// size in bytes of the header extension holding the given JSON metadata
int CNIFTI1MainHeader::headerExtensionSize(const QJsonObject& json) const {
  return jsonExtensionSize(json);
}

// Write the JSON metadata as header extension directly after the NIfTI-1 main header.
// vox_offset has to account for the extension (see headerExtensionSize()).
bool CNIFTI1MainHeader::writeHeaderExtension(CNIFTIFile& niftiFile, const QJsonObject& json) {
  return writeJsonExtension(niftiFile, MAINHEADER_SIZE, json, m_pData->byteOrder != QSysInfo::ByteOrder);
}

// Read the JSON metadata from the header extensions of the NIfTI-1 file
QJsonObject CNIFTI1MainHeader::readHeaderExtension(CNIFTIFile& file) const {
  return readJsonExtension(file, MAINHEADER_SIZE, static_cast<qint64>(m_pData->header.Vox_Offset),
                           m_pData->byteOrder != QSysInfo::ByteOrder);
}
