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

// Conversion of complete image files between ECAT7 or Philips (image .img and
// Syntegra .syn files) and NIfTI-1/NIfTI-2. NIfTI
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
//   with BIDS PET keys (and optionally as JSON sidecar file) and restored when
//   converting back to ECAT7. A DICOM header extension holds the same
//   attributes and the frame timing in the private tags of PMOD, and descrip
//   is written like PMOD writes it (see CNIFTIExtensions.h). NIfTI files
//   without the JSON metadata provide the values of a DICOM extension (e.g.
//   written by PMOD) or the frame timing of toffset and pixdim[4] (4D data only).
// - The metadata is written by the NIfTI classes themselves, so applications
//   using them directly get the same: the header save()/CNIFTIFile::
//   writeMainHeader() stores the attributes, convertFrom() of a NIfTI header
//   takes them over from the ECAT7/Philips (and the other NIfTI version)
//   headers, CNIFTIFile::writeMatrix(data, frame, start, duration) stores the
//   timing of a volume, and convertFrom() of the ECAT7 headers takes over the
//   attributes and timing of a NIfTI header.
//
// Philips image and Syntegra files
// - Philips files are converted via the ECAT7 representation of libmedio
//   (the existing Philips <-> ECAT7 header conversions), so the rules above
//   apply to them as well. Their voxel order is the one of ECAT7.
// - Philips -> NIfTI: every slice has its own scale factor (suvscl together
//   with activity, weight, half-life and injection time, or imgscl, see
//   CPhilipsSubHeaderImage::ecat_Scale_Factor()); if the slices differ in
//   their scale factor, float32 values are written. The position along z is
//   given by the bed position; the image positions img_pos_x/y of the slices
//   are not used, as their definition is not known reliably.
// - NIfTI -> Philips: 16 bit integers, one sub header per slice with the
//   scale factor of the frame (as suvscl for calibrated data, as imgscl
//   otherwise). Image files number their slices from 0, Syntegra files from 1.
//   The x/y position of the NIfTI data is not stored.
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
      bool jsonExtension;   // store the metadata as header extensions (JSON and DICOM)
      bool jsonSidecar;     // also store the metadata as <name>.json next to the NIfTI file
    };

    // Convert an ECAT7 image file into a NIfTI file (.nii, .nii.gz, .hdr/.img, .hdr.gz/.img.gz).
    static bool ecatToNifti(const QString& ecatFileName, const QString& niftiFileName,
                            const Options& options = Options(), QString* error = NULL);

    // Convert a Philips image (.img) or Syntegra (.syn) file into a NIfTI file.
    static bool philipsToNifti(const QString& philipsFileName, const QString& niftiFileName,
                               const Options& options = Options(), QString* error = NULL);

    // Convert an ECAT7 or Philips image file (identified by its content) into a NIfTI file.
    static bool toNifti(const QString& fileName, const QString& niftiFileName,
                        const Options& options = Options(), QString* error = NULL);

    // Convert a NIfTI file (.nii, .nii.gz, .hdr/.img, .hdr.gz/.img.gz) into an ECAT7 image file. The
    // metadata is taken from the JSON header extension or, if there is none,
    // from a JSON sidecar file <name>.json next to the NIfTI file.
    static bool niftiToEcat(const QString& niftiFileName, const QString& ecatFileName,
                            QString* error = NULL);

    // Convert a NIfTI file into a Philips file: a Syntegra file if the name
    // ends with .syn, an image file otherwise. The metadata is taken as for
    // niftiToEcat().
    static bool niftiToPhilips(const QString& niftiFileName, const QString& philipsFileName,
                               QString* error = NULL);
};

#endif // CNIFTICONVERTER_H
