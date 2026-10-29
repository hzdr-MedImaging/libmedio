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

"""Generate the expected values (<file>.json) of the test data files.

The data files are read with nibabel (https://nipy.org/nibabel) as an
independent implementation, so that libmedio is not validated against
itself. See ../README.md for the layout of the test data.

Without arguments, the expected values of all data files which do not have
a <file>.json yet are generated. Files can also be given explicitly (path
relative to the test data directory or absolute).

Supported formats:
  ecat/   ECAT7 image files (file types Image8/16, Volume8/16)
  nifti/  single file NIfTI-1/NIfTI-2 (.nii, .nii.gz) and header/image
          pairs (directory holding <name>.hdr and <name>.img)
  raw/    raw voxel data, described by the "layout" entry of the file in
          datasets.json (dims, data_type, byte_order)
"""

import argparse
import json
import os
import sys

import numpy as np
import nibabel as nib
from nibabel import ecat
from nibabel.volumeutils import array_from_file

DATA_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# entries of a format directory which are no data files
NON_DATA_FILES = ('PLACEHOLDER.md', 'README.md')

ECAT_BLOCK_SIZE = 512

# ECAT7 file types holding image sub headers
ECAT7_IMAGE_FILE_TYPES = (2, 6, 7, 10)  # Image16, Volume8, Volume16, Image8

# ECAT7 data types stored in little endian (VAX) byte order
ECAT7_VAX_DATA_TYPES = (2, 3, 4)


def number(value):
    # convert numpy scalars to plain JSON numbers
    value = np.asarray(value).item()
    return value if isinstance(value, int) else float(value)


def stats(raw):
    # statistics of stored (unscaled) voxel values
    if np.issubdtype(raw.dtype, np.integer):
        raw_sum = int(raw.astype(np.int64).sum())
    else:
        raw_sum = float(raw.astype(np.float64).sum())
    return {'raw_min': number(raw.min()), 'raw_max': number(raw.max()), 'raw_sum': raw_sum}


#------------------------------------------------------------------------------
# ECAT7

def ecat7_matrix_index(matrix_id):
    # decode an ECAT7 matrix id (as defined by the ECAT7 file format)
    return {
        'frame': matrix_id & 0x1FF,
        'plane': ((matrix_id >> 16) & 0xFF) + (((matrix_id >> 9) & 0x3) << 8),
        'gate': (matrix_id >> 24) & 0x3F,
        'bed': (matrix_id >> 12) & 0xF,
        'data': ((matrix_id >> 9) & 0x4) | ((matrix_id >> 30) & 0x3),
    }


def expected_ecat(path, file_info):
    img = ecat.load(path)
    hdr = img.header
    sub = img.get_subheaders()
    mlist = img.get_mlist()

    file_type = int(hdr['file_type'])
    if file_type not in ECAT7_IMAGE_FILE_TYPES:
        raise ValueError(f'ECAT7 file type {file_type} is not supported by this script, '
                         'please write the expected values by hand')

    expected = {
        'generator': f'nibabel {nib.__version__}',
        'format': 'ecat7',
        'main_header': {
            'file_type': file_type,
            'num_frames': int(hdr['num_frames']),
            'num_planes': int(hdr['num_planes']),
            'calibration_factor': number(hdr['ecat_calibration_factor']),
            'data_units': hdr['data_units'].item().decode('latin-1').rstrip('\0 '),
        },
        'matrices': [],
    }

    for i, entry in enumerate(mlist):
        if entry[0] <= 0:
            continue

        sh = sub.subheaders[i]
        code = int(sh['data_type'])
        dtype = np.dtype(ecat.data_type_codes.dtype[code])
        dtype = dtype.newbyteorder('<' if code in ECAT7_VAX_DATA_TYPES else '>')
        shape = (int(sh['x_dimension']), int(sh['y_dimension']), int(sh['z_dimension']))

        # stored voxel values without any scaling or reorientation
        raw = array_from_file(shape, dtype, sub.fileobj, offset=int(entry[1]) * ECAT_BLOCK_SIZE)

        matrix = ecat7_matrix_index(int(entry[0]))
        matrix.update({
            'data_type': code,
            'dims': list(shape),
            'pixel_size_cm': [number(sh['x_pixel_size']), number(sh['y_pixel_size']), number(sh['z_pixel_size'])],
            'scale_factor': number(sh['scale_factor']),
            'frame_start_time_ms': int(sh['frame_start_time']),
            'frame_duration_ms': int(sh['frame_duration']),
        })
        matrix.update(stats(raw))
        expected['matrices'].append(matrix)

    return expected


#------------------------------------------------------------------------------
# NIfTI

def expected_nifti(path, file_info):
    if os.path.isdir(path):
        # header/image pair stored in a dataset directory
        headers = [f for f in os.listdir(path) if f.endswith('.hdr')]
        if len(headers) != 1:
            raise ValueError('a NIfTI pair directory has to contain exactly one .hdr file')
        img = nib.load(os.path.join(path, headers[0]))
        if type(img) not in (nib.Nifti1Pair, nib.Nifti2Pair):
            raise ValueError('not a NIfTI-1/NIfTI-2 header/image pair')
        fmt = 'nifti2-pair' if isinstance(img, nib.Nifti2Pair) else 'nifti1-pair'
    else:
        img = nib.load(path)
        if type(img) not in (nib.Nifti1Image, nib.Nifti2Image):
            raise ValueError('not a single file NIfTI-1/NIfTI-2 file')
        fmt = 'nifti2' if isinstance(img, nib.Nifti2Image) else 'nifti1'

    hdr = img.header
    ndim = int(hdr['dim'][0])

    expected = {
        'generator': f'nibabel {nib.__version__}',
        'format': fmt,
        'header': {
            'dim': [int(v) for v in hdr['dim'][:ndim + 1]],
            'pixdim': [number(v) for v in hdr['pixdim'][:ndim + 1]],
            'datatype': int(hdr['datatype']),
            'bitpix': int(hdr['bitpix']),
            # nibabel moves the scaling into the data object on load
            'scl_slope': number(img.dataobj.slope),
            'scl_inter': number(img.dataobj.inter),
            'qform_code': int(hdr['qform_code']),
            'sform_code': int(hdr['sform_code']),
            'quatern': [number(hdr['quatern_b']), number(hdr['quatern_c']), number(hdr['quatern_d'])],
            'qoffset': [number(hdr['qoffset_x']), number(hdr['qoffset_y']), number(hdr['qoffset_z'])],
            'srow': [[number(v) for v in hdr[f'srow_{axis}']] for axis in 'xyz'],
            'xyzt_units': int(hdr['xyzt_units']),
            'byte_order': 'big' if hdr.endianness == '>' else 'little',
        },
        'volumes': [],
    }

    # stored (unscaled) voxel values per 3D volume
    raw = np.asanyarray(img.dataobj.get_unscaled())
    volumes = raw.reshape(raw.shape[:3] + (-1,), order='F') if raw.ndim > 3 else raw[..., np.newaxis]
    for v in range(volumes.shape[3]):
        expected['volumes'].append(stats(volumes[..., v]))

    return expected


#------------------------------------------------------------------------------
# raw voxel data

RAW_DATA_TYPES = ('uint8', 'int8', 'uint16', 'int16', 'uint32', 'int32', 'float32', 'float64')


def expected_raw(path, file_info):
    layout = file_info.get('layout')
    if not layout:
        raise ValueError('raw files need a "layout" entry (dims, data_type, byte_order) in datasets.json')
    if layout.get('data_type') not in RAW_DATA_TYPES:
        raise ValueError(f'unsupported raw data type {layout.get("data_type")} (supported: {", ".join(RAW_DATA_TYPES)})')

    dtype = np.dtype(layout['data_type']).newbyteorder('>' if layout.get('byte_order') == 'big' else '<')
    dims = [int(d) for d in layout['dims']]
    size = os.path.getsize(path)
    if size != int(np.prod(dims)) * dtype.itemsize:
        raise ValueError(f'file size {size} does not match the layout {dims} x {dtype.itemsize} bytes')

    # first dimension varies fastest
    raw = np.fromfile(path, dtype=dtype).reshape(dims, order='F')
    volumes = raw.reshape(raw.shape[:3] + (-1,), order='F') if raw.ndim > 3 else raw[..., np.newaxis]

    return {
        'generator': f'numpy {np.__version__}',
        'format': 'raw',
        'layout': layout,
        'volumes': [stats(volumes[..., v]) for v in range(volumes.shape[3])],
    }


GENERATORS = {
    'ecat': expected_ecat,
    'nifti': expected_nifti,
    'raw': expected_raw,
}


#------------------------------------------------------------------------------

def data_files(data_dir):
    # all data files/directories of the format directories (relative paths)
    result = []
    for fmt in sorted(os.listdir(data_dir)):
        fmt_dir = os.path.join(data_dir, fmt)
        if fmt == 'tools' or not os.path.isdir(fmt_dir):
            continue
        for name in sorted(os.listdir(fmt_dir)):
            if name.startswith('.') or name.endswith('.json') or name in NON_DATA_FILES:
                continue
            result.append(f'{fmt}/{name}')
    return result


def listed_files(data_dir):
    # file path -> (dataset name, file entry)
    with open(os.path.join(data_dir, 'datasets.json'), encoding='utf-8') as f:
        datasets = json.load(f).get('datasets', {})
    return {path: (name, info) for name, dataset in datasets.items()
            for path, info in dataset.get('files', {}).items()}


def process(data_dir, rel_path, listed):
    fmt = rel_path.split('/')[0]
    if fmt not in GENERATORS:
        raise ValueError(f'no generator for format directory "{fmt}" (supported: {", ".join(GENERATORS)}), '
                         'please write the expected values by hand')

    dataset = os.path.basename(rel_path).split('.')[0]
    listed_dataset, file_info = listed.get(rel_path, (None, {}))
    if listed_dataset != dataset:
        print(f'{rel_path}: warning: not listed under dataset "{dataset}" in datasets.json')

    expected = GENERATORS[fmt](os.path.join(data_dir, rel_path), file_info)
    with open(os.path.join(data_dir, rel_path + '.json'), 'w', encoding='utf-8') as f:
        json.dump(expected, f, indent=2, ensure_ascii=False)
        f.write('\n')
    print(f'{rel_path}.json written')


def remove_placeholders(data_dir, listed):
    # remove the PLACEHOLDER.md of format directories whose files all exist
    for fmt in {path.split('/')[0] for path in listed}:
        placeholder = os.path.join(data_dir, fmt, 'PLACEHOLDER.md')
        complete = all(os.path.exists(os.path.join(data_dir, p)) for p in listed if p.startswith(fmt + '/'))
        if complete and os.path.exists(placeholder):
            os.remove(placeholder)
            print(f'{fmt}/PLACEHOLDER.md removed')


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('files', nargs='*', metavar='FILE', help='data files (default: all without .json)')
    parser.add_argument('--force', action='store_true', help='regenerate existing expected values')
    parser.add_argument('--data-dir', default=DATA_DIR, help='test data directory (default: %(default)s)')
    args = parser.parse_args()

    data_dir = os.path.abspath(args.data_dir)
    listed = listed_files(data_dir)

    if args.files:
        files = [os.path.relpath(os.path.abspath(f) if os.path.exists(f) else os.path.join(data_dir, f), data_dir)
                 for f in args.files]
        files = [f.replace(os.sep, '/') for f in files]
    else:
        files = [f for f in data_files(data_dir)
                 if args.force or not os.path.exists(os.path.join(data_dir, f + '.json'))]

    failed = False
    for rel_path in files:
        try:
            process(data_dir, rel_path, listed)
        except Exception as e:  # report all files before failing
            print(f'{rel_path}: error: {e}', file=sys.stderr)
            failed = True

    remove_placeholders(data_dir, listed)

    pending = [p for p in listed if not os.path.exists(os.path.join(data_dir, p))]
    for path in pending:
        print(f'{path}: listed in datasets.json but missing')

    with open(os.path.join(data_dir, 'datasets.json'), encoding='utf-8') as f:
        if 'TODO' in f.read():
            print('datasets.json: note: still contains TODO entries')

    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
