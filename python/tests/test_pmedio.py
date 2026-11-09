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

import os

import pmedio
import pytest

DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'testsuite', 'data')

def test_constructors():
  a = pmedio.MedIOImage(1,1,1,1)
  assert a.ndim == 4
  assert a.shape == (1,1,1,1)
  a[0,0,0,0] = 123.456
  assert a[0,0,0,0] == pytest.approx(123.456, 3)



def test_attributes():
  # attributes of an ECAT7 file (see testsuite/data/ecat/pet-dynamic.v.json)
  with pmedio.open(os.path.join(DATA, 'ecat', 'pet-dynamic.v')) as f:
    m = f.attributes()
  assert m['PatientWeight'] == pytest.approx(8.63)
  assert m['RadionuclideTotalDose'] == pytest.approx(60780000)
  assert m['AcquisitionDateTime'] == '2026-06-03T17:04:56Z'
  assert m['ECAT7']['Patient_Weight'] == pytest.approx(8.63)
  assert 'NIfTI' not in m
  assert len(m['Frames']) == 3
  assert m['Frames'][0]['DecayFactor'] == pytest.approx(1.00158)

  # Philips file of the same data
  with pmedio.open(os.path.join(DATA, 'philips', 'pet-dynamic.img')) as f:
    p = f.attributes()
  assert p['PatientWeight'] == pytest.approx(8.63)
  assert p['Philips']['weight'] == 8630


def test_attributes_of_created_files(tmp_path):
  # a NIfTI file created like a Philips file stores the Philips attributes
  with pmedio.open(os.path.join(DATA, 'philips', 'pet-dynamic.img')) as f:
    source = f.attributes()
    with pmedio.create(tmp_path / 'philips.nii', like=f) as out:
      for k, frame in enumerate(f.frame_numbers()):
        out.write_volume(f.read_volume(frame), k + 1)
  with pmedio.open(tmp_path / 'philips.nii') as f:
    written = f.attributes()
  assert written['PatientWeight'] == pytest.approx(source['PatientWeight'])
  assert written['Philips'] == source['Philips']
  assert len(written['FrameTimesStart']) == 3


def test_dictionary_entry():
  k = pmedio.dictionary_entry('(0010,1030)')
  assert k['keyword'] == 'PatientWeight'
  assert k['tag'] == '(0010,1030)'
  assert k['vr'] == 'DS'
  assert k['unit'] == 'kg'
  assert k['sequence'] is None
  assert k['scope'] == 'main'
  assert pmedio.dictionary_entry('RadionuclideTotalDose')['sequence'] == '(0054,0016)'
  assert pmedio.dictionary_entry('Unknown') is None
