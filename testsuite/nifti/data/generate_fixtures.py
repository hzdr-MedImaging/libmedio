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

# Generates the NIfTI reference files (single files and header/image pairs)
# used by testNIFTIFormat. The files are
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
import pydicom

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


def save_pair(img, name, compressed=False, offset=0):
    # header/image pair <name>.hdr/<name>.img (compressed: .hdr.gz/.img.gz);
    # offset > 0 stores the voxel data at this vox_offset of the image file
    hdr_path = os.path.join(OUT, name + '.hdr')
    img_path = os.path.join(OUT, name + '.img')
    if offset:
        img.header['vox_offset'] = offset
    nib.save(img, hdr_path)
    if offset:
        # nibabel writes the data at vox_offset and leaves the gap empty
        with open(img_path, 'rb') as f:
            data = f.read()
        assert len(data) == offset + img.get_data_dtype().itemsize * int(np.prod(img.shape))
        with open(img_path, 'wb') as f:
            f.write(b'libmedio gap ' + b'.' * (offset - 13) + data[offset:])
    if compressed:
        for path in (hdr_path, img_path):
            with open(path, 'rb') as f:
                raw = f.read()
            os.remove(path)
            with open(path + '.gz', 'wb') as f:
                f.write(gzip.compress(raw, mtime=0))
    print('written', name, '(header/image pair)')


N1, N2 = nib.Nifti1Image, nib.Nifti2Image
P1, P2 = nib.Nifti1Pair, nib.Nifti2Pair

save(make(N1, (4, 3, 2),    np.int16),                   'n1_le_int16.nii')
save(make(N1, (4, 3, 2, 2), np.float32, '>'),            'n1_be_float32_4d.nii')
save(make(N1, (4, 3, 2),    np.uint8),                   'n1_le_uint8.nii.gz')
save(make(N1, (4, 3, 2),    np.int16, extensions=True),  'n1_le_int16_ext.nii')
save(make(N2, (4, 3, 2),    np.float64),                 'n2_le_float64.nii')
save(make(N2, (4, 3, 2),    np.int32, '>', True),        'n2_be_int32_ext.nii')
save(make(N2, (4, 3, 2, 2), np.int16),                   'n2_le_int16_4d.nii.gz')

# header/image pairs ("ni1"/"ni2")
save_pair(make(P1, (4, 3, 2),    np.int16, extensions=True), 'p1_le_int16_ext')
save_pair(make(P2, (4, 3, 2, 2), np.float32, '>'),           'p2_be_float32_4d')
save_pair(make(P1, (4, 3, 2),    np.uint8),                  'p1_le_uint8', compressed=True)
save_pair(make(P1, (4, 3, 2),    np.int16),                  'p1_le_int16_offset', offset=48)

# Analyze 7.5 header/image pair, which must not be identified as NIfTI
nib.save(nib.AnalyzeImage(pattern((4, 3, 2), np.int16), AFFINE), os.path.join(OUT, 'analyze.img'))
print('written analyze.hdr/.img')

# minimal NIfTI-1 pair without extensions written by nibabel from a single file image
nib.save(N1(pattern((4, 3, 2), np.int16), AFFINE), os.path.join(OUT, 'n1_pair.img'))
print('written n1_pair.hdr/.img')


# metadata in a DICOM header extension (code 2), written with pydicom
# (https://pydicom.github.io) as nibabel's Nifti1DicomExtension

# a file like those written by PMOD: no qform/sform, descrip
# "<patient name> [<study type>] <study description>" and a DICOM extension
# holding the units and the frame timing in the private tags of PMOD
# (start times in s, durations in ms)
PMOD_STARTS = [0.0, 15.0, 45.0]
PMOD_DURATIONS_MS = [15000.0, 30000.0, 45000.0]


def pmod_dataset():
    ds = pydicom.Dataset()
    ds.add_new((0x0054, 0x1001), 'CS', 'BQML')
    ds.add_new((0x0055, 0x0010), 'LO', 'PMOD_1')
    ds.add_new((0x0055, 0x1001), 'FD', PMOD_STARTS)
    ds.add_new((0x0055, 0x1004), 'FD', PMOD_DURATIONS_MS)
    return ds


def make_pmod():
    img = N1(pattern((4, 3, 2, 3), np.int16), None)
    img.set_qform(None, 0)
    img.set_sform(None, 0)
    img.header.set_xyzt_units('mm', 'sec')
    img.header['pixdim'][1:4] = [2.0, 2.5, 3.0]
    img.header['descrip'] = b'Test^Patient [DYNAMIC\\IMA] PET test study'
    img.header.extensions.append(nib.nifti1.Nifti1DicomExtension(2, pmod_dataset()))
    return img


# a big endian file with a DICOM extension holding attributes (incl. sequences
# and a Latin-1 patient name) and a JSON extension: the JSON metadata comes
# first, the DICOM extension provides the values the JSON metadata does not have.
# The data set is big endian like the file (nibabel writes it so with
# parent_hdr, but reads DICOM extensions as little endian, as libmedio writes them).
DICOM_JSON = {"PatientWeight": 70.5, "StudyDescription": "from JSON"}


def full_dataset():
    ds = pydicom.Dataset()
    ds.SpecificCharacterSet = 'ISO_IR 100'
    ds.AcquisitionDateTime = '20240227141344'
    ds.StudyDescription = 'from DICOM'
    ds.PatientName = 'Müller^Hans'
    ds.PatientID = 'ID-0815'
    ds.PatientBirthDate = '19700102'
    ds.PatientSex = 'M'
    ds.PatientWeight = '80'
    ds.PatientSize = '1.78'
    code = pydicom.Dataset()
    code.CodeValue = 'C-111A1'
    code.CodingSchemeDesignator = 'SRT'
    code.CodeMeaning = '^18^Fluorine'
    info = pydicom.Dataset()
    info.Radiopharmaceutical = 'FDG'
    info.RadiopharmaceuticalStartDateTime = '20240227131500'
    info.RadionuclideTotalDose = '370000000'
    info.RadionuclideHalfLife = '6586.2'
    info.RadionuclideCodeSequence = pydicom.Sequence([code])
    ds.RadiopharmaceuticalInformationSequence = pydicom.Sequence([info])
    ds.Units = 'BQML'
    ds.DecayCorrection = 'START'
    ds.add_new((0x0055, 0x0010), 'LO', 'PMOD_1')
    ds.add_new((0x0055, 0x1001), 'FD', [0.0, 60.0])
    ds.add_new((0x0055, 0x1004), 'FD', [60000.0, 120000.0])
    return ds


def make_dicom_be():
    img = make(N1, (4, 3, 2, 2), np.float32, '>')
    img.header.extensions.append(nib.nifti1.Nifti1Extension(6, json.dumps(DICOM_JSON).encode()))
    img.header.extensions.append(nib.nifti1.Nifti1DicomExtension(2, full_dataset(), parent_hdr=img.header))
    return img


save(make_pmod(), 'pmod_le_int16_4d.nii')
save(make_dicom_be(), 'n1_be_float32_dicom.nii')
