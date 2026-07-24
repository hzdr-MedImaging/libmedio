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
#include "CNIFTI1MainHeader.h"
#include "CNIFTIFile.h"
#include "CECATMainHeader.h"
#include "CConcordeFrameHeader.h"
#include "CPhilipsMainHeader.h"
#include "CPhilipsSubHeaderImage.h"
#include "MedIOUnits.h"

#include "config.h"

#include <QDataStream>
#include <QDateTime>
#include <QFileInfo>
#include <QTextStream>

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

      quint16 Dim[8];                     //  40: Dim (8) -> data array for dimensions
      float   Intent_P1;                  //  56: Intent_P1

      float   Intent_P2;                  //  60: Intent_P2

      float   Intent_P3;                  //  64: Intent_P3

      quint16 Intent_Code;                //  68: Intent_Code 
      quint16 DataType;                  //  70: Data_Type -> defines the data type of the image data (unsigned char, signed short, float, etc.)
      quint16 Bit_Pix;                    //  72: Bit_Pix
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
    #pragma pack(pop) // restore the previous packing alignment
};



//==============================================================================================
// Header extension
// after the main header, the extension is 4 bytes long 

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

  LEAVE();
}

//=============================================================================================
// load the main header from the file
bool CNIFTI1MainHeader::load(void) {

  D("Dimensione struct HeaderData: %lu", sizeof(m_pData->header));
  D("MAINHEADER_SIZE definito: %d", MAINHEADER_SIZE);
std::cout << "blah2:" << MAINHEADER_SIZE << ":" << sizeof(m_pData->header) << std::endl;
  ENTER();
  CMedIOData* mData = medIOData();

  // only go on if the device is readable at all
  if(mData == NULL ||
     mData->isReadable() == false ||
     mData->seek(0) == false) {
std::cout << "blah3" << std::endl;
    RETURN(false);
    return false;

  }

  // we read in all data at once using read()
  ASSERT(sizeof(m_pData->header) == MAINHEADER_SIZE);

  if(mData->read(reinterpret_cast<char*>(&m_pData->header), sizeof(m_pData->header)) != MAINHEADER_SIZE) {
    std::cout << "blah4" << std::endl;
    RETURN(false);
    return false;
  }

//------------------------------------------------------------------------------------

  // now that we have streamed in all data in one run we
  // have to take care of correct endianness in the non-char
  // entries in the header structure in case this is a little endian
  // machine

  if(QSysInfo::ByteOrder != QSysInfo::LittleEndian) {
    // we only swap non-char elements of the header
    BSWAP_32(m_pData->header.Sizeof_Hdr);
    BSWAP_32(m_pData->header.Extents);
    BSWAP_16(m_pData->header.Session_Error);
    BSWAP_16(m_pData->header.Dim[0]);
    BSWAP_16(m_pData->header.Dim[1]);
    BSWAP_16(m_pData->header.Dim[2]);
    BSWAP_16(m_pData->header.Dim[3]);
    BSWAP_16(m_pData->header.Dim[4]);
    BSWAP_16(m_pData->header.Dim[5]);
    BSWAP_16(m_pData->header.Dim[6]);
    BSWAP_16(m_pData->header.Dim[7]);

    BSWAP_FLT(m_pData->header.Intent_P1);
    BSWAP_FLT(m_pData->header.Intent_P2);
    BSWAP_FLT(m_pData->header.Intent_P3);
    BSWAP_16(m_pData->header.Intent_Code);
    BSWAP_16(m_pData->header.DataType);
    BSWAP_16(m_pData->header.Bit_Pix);
    BSWAP_16(m_pData->header.Slice_Start);
    BSWAP_FLT(m_pData->header.Pix_Dim[0]);
    BSWAP_FLT(m_pData->header.Pix_Dim[1]);
    BSWAP_FLT(m_pData->header.Pix_Dim[2]);
    BSWAP_FLT(m_pData->header.Pix_Dim[3]);
    BSWAP_FLT(m_pData->header.Pix_Dim[4]);
    BSWAP_FLT(m_pData->header.Pix_Dim[5]);
    BSWAP_FLT(m_pData->header.Pix_Dim[6]);
    BSWAP_FLT(m_pData->header.Pix_Dim[7]);
    BSWAP_FLT(m_pData->header.Vox_Offset);
    BSWAP_FLT(m_pData->header.Scl_Slope);
    BSWAP_FLT(m_pData->header.Scl_Inter);
    BSWAP_16(m_pData->header.Slice_End);
    BSWAP_FLT(m_pData->header.Cal_Max);
    BSWAP_FLT(m_pData->header.Cal_Min);
    BSWAP_FLT(m_pData->header.Slice_Duration);
    BSWAP_FLT(m_pData->header.Toffset);
    BSWAP_32(m_pData->header.Glmax);
    BSWAP_32(m_pData->header.Glmin);

    BSWAP_16(m_pData->header.Qform_Code);
    BSWAP_16(m_pData->header.Sform_Code);
    BSWAP_FLT(m_pData->header.Quatern_B);
    BSWAP_FLT(m_pData->header.Quatern_C);
    BSWAP_FLT(m_pData->header.Quatern_D);
    BSWAP_FLT(m_pData->header.Qoffset_X);
    BSWAP_FLT(m_pData->header.Qoffset_Y);
    BSWAP_FLT(m_pData->header.Qoffset_Z);
    BSWAP_FLT(m_pData->header.Srow_X[0]);
    BSWAP_FLT(m_pData->header.Srow_X[1]); 
    BSWAP_FLT(m_pData->header.Srow_X[2]);
    BSWAP_FLT(m_pData->header.Srow_X[3]);
    BSWAP_FLT(m_pData->header.Srow_Y[0]);
    BSWAP_FLT(m_pData->header.Srow_Y[1]);
    BSWAP_FLT(m_pData->header.Srow_Y[2]);
    BSWAP_FLT(m_pData->header.Srow_Y[3]);
    BSWAP_FLT(m_pData->header.Srow_Z[0]);
    BSWAP_FLT(m_pData->header.Srow_Z[1]); 
    BSWAP_FLT(m_pData->header.Srow_Z[2]);
    BSWAP_FLT(m_pData->header.Srow_Z[3]);
  }

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
  
  struct CNIFTI1MainHeaderPrivate::HeaderData* header = NULL;

//------------------------------------------------------------------------------------
  if(QSysInfo::ByteOrder != QSysInfo::LittleEndian) {
    header = new CNIFTI1MainHeaderPrivate::HeaderData;

    // Copy the current header data to manipulate it safely
    memcpy(header, &m_pData->header, sizeof(m_pData->header));

    // Apply the same byte-swapping logic used in load()
    BSWAP_32(header->Sizeof_Hdr);
    BSWAP_32(header->Extents);
    BSWAP_16(header->Session_Error);
    for(int i=0; i<8; i++) {
        BSWAP_16(header->Dim[i]);
        BSWAP_FLT(header->Pix_Dim[i]);
    }
    BSWAP_FLT(header->Intent_P1);
    BSWAP_FLT(header->Intent_P2);
    BSWAP_FLT(header->Intent_P3);
    BSWAP_16(header->Intent_Code);
    BSWAP_16(header->DataType);
    BSWAP_16(header->Bit_Pix);
    BSWAP_16(header->Slice_Start);
    BSWAP_FLT(header->Vox_Offset);
    BSWAP_FLT(header->Scl_Slope);
    BSWAP_FLT(header->Scl_Inter);
    BSWAP_16(header->Slice_End);
    BSWAP_FLT(header->Cal_Max);
    BSWAP_FLT(header->Cal_Min);
    BSWAP_FLT(header->Slice_Duration);
    BSWAP_FLT(header->Toffset);
    BSWAP_32(header->Glmax);
    BSWAP_32(header->Glmin);

    BSWAP_16(header->Qform_Code);
    BSWAP_16(header->Sform_Code);
    BSWAP_FLT(header->Quatern_B);
    BSWAP_FLT(header->Quatern_C);
    BSWAP_FLT(header->Quatern_D);
    BSWAP_FLT(header->Qoffset_X);
    BSWAP_FLT(header->Qoffset_Y);
    BSWAP_FLT(header->Qoffset_Z);

    for(int i=0; i<4; i++) {
        BSWAP_FLT(header->Srow_X[i]);
        BSWAP_FLT(header->Srow_Y[i]);
        BSWAP_FLT(header->Srow_Z[i]);
    }
  }
  else {
    header = &m_pData->header;
  }

//------------------------------------------------------------------------------------
  // Write out the main header to the file
  bool result = false;
  if(mData->write(reinterpret_cast<char*>(header), sizeof(m_pData->header)) == MAINHEADER_SIZE) {
    niftiFile->mainHeaderWritten(*this);
    result = true;
  }

  // Delete the temporary byte-swapped header structure
  if(QSysInfo::ByteOrder != QSysInfo::BigEndian)
    delete header;

  RETURN(result);
  return result;
}

//=============================================================================================
int CNIFTI1MainHeader::rawDataSize() const { 
  return 348; // Size of the NIFTI-1 main header in bytes
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
    case CMedIOHeader::ECATMainHeader: 
    {
      const CECATMainHeader* eMainHeader = static_cast<const CECATMainHeader*>(mainHeader);
      
      // Default configurations for NIFTI converted from ECAT
      m_pData->header.Dim[0] = 3; // 3D volume by default
      m_pData->header.Qform_Code = 1; // Scanner anatomical coordinates
      m_pData->header.Sform_Code = 0; // No standard space alignment by default
      m_pData->header.Pix_Dim[0] = 1.0f; // qfac = 1 (right-handed)

      // Note: Full ECAT to NIFTI spatial mapping requires extracting 
      // voxel sizes and matrix dimensions from ECAT subheaders, 
      // which vary depending on ECAT6 or ECAT7 sub-types.
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

//=============================================================================================
QTextStream& operator<<(QTextStream& stream, const CNIFTI1MainHeader& mHeader) {
  ENTER();

  stream << qSetRealNumberPrecision(6)
         << "SIZEOF_HDR " << mHeader.m_pData->header.Sizeof_Hdr << Qt::endl
         << "DATA_TYPE_STR "  << mHeader.m_pData->header.Data_Type  << Qt::endl
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

  stream << "QFORM_CODE " << mHeader.m_pData->header.Qform_Code << Qt::endl
         << "SFORM_CODE " << mHeader.m_pData->header.Sform_Code << Qt::endl
         << "QOFFSET_X "  << mHeader.m_pData->header.Qoffset_X  << Qt::endl
         << "QOFFSET_Y "  << mHeader.m_pData->header.Qoffset_Y  << Qt::endl
         << "QOFFSET_Z "  << mHeader.m_pData->header.Qoffset_Z  << Qt::endl;

  stream << "QUATERN_B " << mHeader.m_pData->header.Quatern_B << Qt::endl
         << "QUATERN_C " << mHeader.m_pData->header.Quatern_C << Qt::endl
         << "QUATERN_D " << mHeader.m_pData->header.Quatern_D << Qt::endl;
  stream << "MAGIC " << mHeader.m_pData->header.Magic << Qt::endl;


  RETURN(&stream);
  return stream;
}