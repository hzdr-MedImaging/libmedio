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

"""Generate the "expected" block of a test dataset (see ../README.md).

The data file of the dataset is read with nibabel (https://nipy.org/nibabel)
as an independent implementation, so that libmedio is not validated against
itself. dataset.json is created if it does not exist, otherwise only its
"expected" block is replaced.

Usage: make_expected.py DATASET_DIR [DATASET_DIR ...]

Supported formats (first directory level below testsuite/data):
  ecat7  ECAT7 image files (file types Image8/16, Volume8/16)
"""

import argparse
import glob
import json
import os
import sys

import numpy as np
import nibabel as nib
from nibabel import ecat
from nibabel.volumeutils import array_from_file

ECAT_BLOCK_SIZE = 512

# ECAT7 file types holding image sub headers
ECAT7_IMAGE_FILE_TYPES = (2, 6, 7, 10)  # Image16, Volume8, Volume16, Image8

# ECAT7 data types stored in little endian (VAX) byte order
ECAT7_VAX_DATA_TYPES = (2, 3, 4)


def ecat7_matrix_index(matrix_id):
    # decode an ECAT7 matrix id (as defined by the ECAT7 file format)
    return {
        'frame': matrix_id & 0x1FF,
        'plane': ((matrix_id >> 16) & 0xFF) + (((matrix_id >> 9) & 0x3) << 8),
        'gate': (matrix_id >> 24) & 0x3F,
        'bed': (matrix_id >> 12) & 0xF,
        'data': ((matrix_id >> 9) & 0x4) | ((matrix_id >> 30) & 0x3),
    }


def number(value):
    # convert numpy scalars to plain JSON numbers
    value = np.asarray(value).item()
    return value if isinstance(value, int) else float(value)


def expected_ecat7(path):
    img = ecat.load(path)
    hdr = img.header
    sub = img.get_subheaders()
    mlist = img.get_mlist()

    file_type = int(hdr['file_type'])
    if file_type not in ECAT7_IMAGE_FILE_TYPES:
        raise ValueError(f'ECAT7 file type {file_type} is not supported by this script, '
                         'please add the expected values manually')

    expected = {
        'generator': f'nibabel {nib.__version__}',
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
        raw_sum = raw.astype(np.float64).sum()
        if np.issubdtype(dtype, np.integer):
            raw_sum = int(raw.astype(np.int64).sum())

        matrix = ecat7_matrix_index(int(entry[0]))
        matrix.update({
            'data_type': code,
            'dims': list(shape),
            'pixel_size_cm': [number(sh['x_pixel_size']), number(sh['y_pixel_size']), number(sh['z_pixel_size'])],
            'scale_factor': number(sh['scale_factor']),
            'frame_start_time_ms': int(sh['frame_start_time']),
            'frame_duration_ms': int(sh['frame_duration']),
            'raw_min': number(raw.min()),
            'raw_max': number(raw.max()),
            'raw_sum': number(raw_sum),
        })
        expected['matrices'].append(matrix)

    return expected


GENERATORS = {
    'ecat7': (expected_ecat7, ('*.v',)),
}


def process(dataset_dir):
    dataset_dir = os.path.abspath(dataset_dir)
    fmt = os.path.basename(os.path.dirname(dataset_dir))
    if fmt not in GENERATORS:
        raise ValueError(f'unsupported format directory "{fmt}" (supported: {", ".join(GENERATORS)})')
    generator, patterns = GENERATORS[fmt]

    json_path = os.path.join(dataset_dir, 'dataset.json')
    if os.path.exists(json_path):
        with open(json_path, encoding='utf-8') as f:
            dataset = json.load(f)
    else:
        candidates = sorted(c for p in patterns for c in glob.glob(os.path.join(dataset_dir, p)))
        if len(candidates) != 1:
            raise ValueError('dataset.json missing and the data file cannot be determined '
                             f'(candidates: {candidates}); please create dataset.json with a "file" entry')
        dataset = {'description': 'TODO', 'file': os.path.basename(candidates[0])}

    if 'file' not in dataset:
        raise ValueError(f'{json_path}: "file" entry missing')

    for reference in dataset.get('references', []):
        if not os.path.isfile(os.path.join(dataset_dir, reference.get('file', ''))):
            raise ValueError(f'reference file "{reference.get("file")}" listed in dataset.json does not exist')

    dataset.pop('expected', None)
    dataset['expected'] = generator(os.path.join(dataset_dir, dataset['file']))

    with open(json_path, 'w', encoding='utf-8') as f:
        json.dump(dataset, f, indent=2, ensure_ascii=False)
        f.write('\n')

    print(f'{json_path}: {len(dataset["expected"].get("matrices", []))} matrices')

    # the dataset is complete now, remove the placeholder files
    for placeholder in glob.glob(os.path.join(dataset_dir, '**', 'PLACEHOLDER.md'), recursive=True):
        os.remove(placeholder)
        print(f'removed {placeholder}')

    if 'TODO' in json.dumps({k: v for k, v in dataset.items() if k != 'expected'}):
        print(f'{json_path}: note: still contains TODO entries')


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('datasets', nargs='+', metavar='DATASET_DIR')
    args = parser.parse_args()

    failed = False
    for dataset_dir in args.datasets:
        try:
            process(dataset_dir)
        except Exception as e:  # report all datasets before failing
            print(f'{dataset_dir}: error: {e}', file=sys.stderr)
            failed = True

    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
