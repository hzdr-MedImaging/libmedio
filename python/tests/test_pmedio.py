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
  m = pmedio.attributes(os.path.join(DATA, 'ecat', 'pet-dynamic.v'))
  assert m['PatientWeight'] == pytest.approx(8.63)
  assert m['RadionuclideTotalDose'] == pytest.approx(60780000)
  assert m['AcquisitionDateTime'] == '2026-06-03T17:04:56Z'
  assert m['ECAT7']['Patient_Weight'] == pytest.approx(8.63)
  assert 'NIfTI' not in m
  assert len(m['Frames']) == 3
  assert m['Frames'][0]['DecayFactor'] == pytest.approx(1.00158)

  # Philips file of the same data
  p = pmedio.attributes(os.path.join(DATA, 'philips', 'pet-dynamic.img'))
  assert p['PatientWeight'] == pytest.approx(8.63)
  assert p['Philips']['weight'] == 8630


def test_dictionary_entry():
  k = pmedio.dictionaryEntry('(0010,1030)')
  assert k['keyword'] == 'PatientWeight'
  assert k['tag'] == '(0010,1030)'
  assert k['vr'] == 'DS'
  assert k['unit'] == 'kg'
  assert k['sequence'] is None
  assert pmedio.dictionaryEntry('RadionuclideTotalDose')['sequence'] == '(0054,0016)'
  assert pmedio.dictionaryEntry('Unknown') is None


def test_read_volume():
  v = pmedio.readVolume(os.path.join(DATA, 'ecat', 'pet-static.v'))
  assert v['values'].shape == (33, 33, 37)
  assert v['affine'].shape == (4, 4)
  assert v['orientation'] == 'LPI'
  assert v['frameDuration'] == pytest.approx(1800.0)
