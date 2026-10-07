#!/usr/bin/env python3
#
# libmedio - C++ I/O Library for loading/saving medical data formats
#            https://github.com/hzdr-MedImaging/libmedio
#
# Copyright (C) 2004-2026 hzdr.de and contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Generates the NIfTI reference files used by testNIFTIFormat. The files are
# written with nibabel (https://nipy.org/nibabel) as an independent NIfTI
# implementation and only contain synthetic data.
#
# Usage: python3 generate_fixtures.py [output directory]
#
# The voxel values, affine and header fields must match the expectations
# in ../testNIFTIFormat.cpp.

import gzip
import json
import os
import sys

import nibabel as nib
import numpy as np

OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))

# affine used for all files (qform and sform)
AFFINE = np.array([[-2.0, 0.0, 0.0,  10.0],
                   [ 0.0, 2.5, 0.0, -20.0],
                   [ 0.0, 0.0, 3.0, -30.0],
                   [ 0.0, 0.0, 0.0,   1.0]])

# JSON metadata stored as header extension (code 6)
JSON_META = {"Isotope_Name": "F-18", "Dosage": 300.5, "Frame_Duration": 600}


def pattern(shape, dtype):
    # voxel value of linear (Fortran order) index i
    i = np.arange(int(np.prod(shape)))
    if np.issubdtype(dtype, np.floating):
        v = i * 0.25 - 2.0
    elif np.issubdtype(dtype, np.unsignedinteger):
        v = (i * 7) % 50
    else:
        v = (i * 7) % 50 - 10
    return v.astype(dtype).reshape(shape, order='F')


def make(cls, shape, dtype, byteorder='<', extensions=False):
    hdr = cls.header_class()
    if byteorder == '>':
        hdr = hdr.as_byteswapped('>')
    img = cls(pattern(shape, dtype), AFFINE, header=hdr)
    img.set_data_dtype(np.dtype(dtype).newbyteorder(byteorder))
    img.set_qform(AFFINE, 1)
    img.set_sform(AFFINE, 1)
    img.header.set_xyzt_units('mm', 'sec')
    img.header['scl_slope'] = 0.5
    img.header['scl_inter'] = 1.0
    img.header['descrip'] = b'libmedio test fixture'
    if extensions:
        img.header.extensions.append(nib.nifti1.Nifti1Extension('comment', b'plain text comment'))
        img.header.extensions.append(nib.nifti1.Nifti1Extension(6, json.dumps(JSON_META).encode()))
    return img


def save(img, name):
    path = os.path.join(OUT, name)
    if name.endswith('.gz'):
        # write deterministic gzip files (no timestamp)
        raw = img.to_bytes()
        with open(path, 'wb') as f:
            f.write(gzip.compress(raw, mtime=0))
    else:
        nib.save(img, path)
    print('written', path)


N1, N2 = nib.Nifti1Image, nib.Nifti2Image

save(make(N1, (4, 3, 2),    np.int16),                   'n1_le_int16.nii')
save(make(N1, (4, 3, 2, 2), np.float32, '>'),            'n1_be_float32_4d.nii')
save(make(N1, (4, 3, 2),    np.uint8),                   'n1_le_uint8.nii.gz')
save(make(N1, (4, 3, 2),    np.int16, extensions=True),  'n1_le_int16_ext.nii')
save(make(N2, (4, 3, 2),    np.float64),                 'n2_le_float64.nii')
save(make(N2, (4, 3, 2),    np.int32, '>', True),        'n2_be_int32_ext.nii')
save(make(N2, (4, 3, 2, 2), np.int16),                   'n2_le_int16_4d.nii.gz')

# files which must not be identified as (single file) NIfTI
nib.save(nib.AnalyzeImage(pattern((4, 3, 2), np.int16), AFFINE), os.path.join(OUT, 'analyze.img'))
nib.save(N1(pattern((4, 3, 2), np.int16), AFFINE), os.path.join(OUT, 'n1_pair.img'))
print('written analyze.hdr/.img and n1_pair.hdr/.img')
