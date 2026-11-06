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

#ifndef CNIFTICONVERTER_H
#define CNIFTICONVERTER_H

#include <QString>

// Conversion of complete image files between ECAT7 and NIfTI-1/NIfTI-2. NIfTI
// files are single files (.nii, .nii.gz) or header/image pairs (.hdr/.img,
// .hdr.gz/.img.gz; either name of a pair can be given), depending on the file
// name. Analyze 7.5 files (.hdr/.img without NIfTI magic) are not supported.
//
// Voxel values
// - NIfTI files hold the physical values of the ECAT7 data, i.e. the stored
//   values x scale factor x calibration factor. If all frames share the same
//   scale factor the stored values are kept and the factor is written as
//   scl_slope, otherwise (or with Options::floatData) the values are stored as
//   float32.
// - ECAT7 files are written as 16 bit integers with a scale factor per frame
//   and calibration factor 1. 8/16 bit integer NIfTI data without scl_inter
//   is kept as it is, other data is requantized per frame.
//
// Geometry
// - ECAT7 voxel (i, j, k) is located at (DICOM patient coordinates LPS, mm)
//     x = x_offset + (i - (nx-1)/2) * x_pixel_size
//     y = y_offset + (j - (ny-1)/2) * y_pixel_size
//     z = init_bed_position + z_offset - k * z_pixel_size
// - ECAT7 -> NIfTI: the voxel is stored as NIfTI voxel (i, j, nz-1-k), qform
//   and sform (code 1) hold the corresponding RAS affine diag(-px, -py, pz),
//   xyzt_units are mm and s.
// - NIfTI -> ECAT7: the affine is taken from the sform, the qform or, if both
//   codes are 0, diag(-px, -py, pz) (the convention of PMOD and ROVER). The
//   voxels are reordered into the ECAT7 voxel order, whatever the orientation
//   of the NIfTI data is. Oblique data is assigned to the closest axes.
//
// Frames and metadata
// - All frames of dynamic data are converted (4D NIfTI <-> ECAT7 frames).
// - The ECAT7 main header values (incl. patient data) and the frame timing
//   (FrameTimesStart, FrameDuration in s) are stored as JSON header extension
//   (and optionally as JSON sidecar file) and restored when converting back
//   to ECAT7. NIfTI files without this metadata provide the frame timing by
//   toffset and pixdim[4] (4D data only).
//
// If a conversion fails, no output file is left behind.
class CNIFTIConverter
{
  public:
    struct Options
    {
      Options() : niftiVersion(1), floatData(false), jsonExtension(true), jsonSidecar(false) { }

      int niftiVersion;     // NIfTI version to write (1 or 2)
      bool floatData;       // always store float32 voxel values
      bool jsonExtension;   // store the metadata as JSON header extension
      bool jsonSidecar;     // also store the metadata as <name>.json next to the NIfTI file
    };

    // Convert an ECAT7 image file into a NIfTI file (.nii, .nii.gz, .hdr/.img, .hdr.gz/.img.gz).
    static bool ecatToNifti(const QString& ecatFileName, const QString& niftiFileName,
                            const Options& options = Options(), QString* error = NULL);

    // Convert a NIfTI file (.nii, .nii.gz, .hdr/.img, .hdr.gz/.img.gz) into an ECAT7 image file. The
    // metadata is taken from the JSON header extension or, if there is none,
    // from a JSON sidecar file <name>.json next to the NIfTI file.
    static bool niftiToEcat(const QString& niftiFileName, const QString& ecatFileName,
                            QString* error = NULL);
};

#endif // CNIFTICONVERTER_H
