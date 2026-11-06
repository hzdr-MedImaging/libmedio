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

#ifndef CNIFTIHEADERCOPY_H
#define CNIFTIHEADERCOPY_H

// private helper (not installed) used by the NIfTI-1/NIfTI-2 main header
// classes to convert between both header versions.

#include <QtGlobal>

// Copy all header fields of a NIfTI-1/NIfTI-2 header into another one. Fields
// which are file specific (sizeof_hdr, magic, vox_offset, byte order) are left
// untouched. Returns false if a value does not fit into the destination header
// (NIfTI-1 limits dimensions and slice indices to 16 bit).
template <class DST, class SRC>
static bool copyNIFTIHeaderFields(DST& dst, const SRC& src, qint64 maxValue)
{
  for(short i=0; i < 8; i++) {
    if(src.dim(i) > maxValue)
      return false;
  }

  if(src.slice_Start() > maxValue || src.slice_End() > maxValue)
    return false;

  for(short i=0; i < 8; i++) {
    dst.setDim(i, src.dim(i));
    dst.setPix_Dim(i, src.pix_Dim(i));
  }

  dst.setDataType(src.dataType());
  dst.setBit_Pix(src.bit_Pix());
  dst.setDim_Info(src.dim_Info());

  dst.setIntent_P1(src.intent_P1());
  dst.setIntent_P2(src.intent_P2());
  dst.setIntent_P3(src.intent_P3());
  dst.setIntent_Code(src.intent_Code());
  dst.setIntent_Name(src.intent_Name());

  dst.setScl_Slope(src.scl_Slope());
  dst.setScl_Inter(src.scl_Inter());
  dst.setCal_Max(src.cal_Max());
  dst.setCal_Min(src.cal_Min());

  dst.setSlice_Start(src.slice_Start());
  dst.setSlice_End(src.slice_End());
  dst.setSlice_Code(src.slice_Code());
  dst.setSlice_Duration(src.slice_Duration());
  dst.setToffset(src.toffset());
  dst.setXyzt_Units(src.xyzt_Units());

  dst.setDescrip(src.descrip());
  dst.setAux_File(src.aux_File());

  dst.setQform_Code(src.qform_Code());
  dst.setSform_Code(src.sform_Code());
  dst.setQuatern_B(src.quatern_B());
  dst.setQuatern_C(src.quatern_C());
  dst.setQuatern_D(src.quatern_D());
  dst.setQoffset_X(src.qoffset_X());
  dst.setQoffset_Y(src.qoffset_Y());
  dst.setQoffset_Z(src.qoffset_Z());

  for(short i=0; i < 4; i++) {
    dst.setSrow_X(i, src.srow_X(i));
    dst.setSrow_Y(i, src.srow_Y(i));
    dst.setSrow_Z(i, src.srow_Z(i));
  }

  return true;
}

#endif // CNIFTIHEADERCOPY_H
