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

# Tests of Image.to_nibabel() and Image.from_nibabel(): pmedio images as nibabel images
# and back. The expected values are the synthetic images themselves and nibabel, which
# is independent of libmedio (its RAS affines, its reading and writing of NIfTI files).

import sys

import numpy as np
import pytest

import pmedio
from test_volume import DATA_DIR, needs_data

nib = pytest.importorskip("nibabel")

# LPS affine of a synthetic image and the RAS affine nibabel uses for it
AFFINE = np.array([[-2.0, 0.0, 0.0, 10.0], [0.0, -2.5, 0.0, -20.0], [0.0, 0.0, 3.0, -30.0]])
RAS = np.diag([-1.0, -1.0, 1.0, 1.0]) @ np.vstack([AFFINE, [0, 0, 0, 1]])
SHAPE = (5, 4, 3)


def synthetic(frames=3, dtype=np.int16, duration=(30.0, 30.0, 30.0)):
    data = (np.arange(int(np.prod(SHAPE)) * frames).reshape((*SHAPE, frames), order="F") + 1).astype(dtype)
    return pmedio.Image(data, affine=AFFINE, frame_start=[60.0 * k for k in range(frames)],
                        frame_duration=list(duration[:frames]), attributes={"PatientName": "Test^Name"})


#=============================================================================================
# to_nibabel()

def test_to_nibabel():
    image = synthetic()
    n = image.to_nibabel()
    assert isinstance(n, nib.Nifti1Image)
    assert n.shape == (*SHAPE, 3)
    assert n.get_data_dtype() == np.int16
    assert np.array_equal(np.asanyarray(n.dataobj), image.data)
    assert np.shares_memory(np.asanyarray(n.dataobj), image.data)
    assert np.allclose(n.affine, RAS)
    # scanner coordinates, mm and s, the frame duration as fourth voxel size
    assert int(n.header["qform_code"]) == 1 and int(n.header["sform_code"]) == 1
    assert n.header.get_xyzt_units() == ("mm", "sec")
    assert n.header.get_zooms() == pytest.approx((2.0, 2.5, 3.0, 30.0))
    # the same position of every voxel: nibabel RAS = pmedio LPS with x and y negated
    for voxel in ([0, 0, 0], [4, 3, 2], [1, 2, 1]):
        ras = n.affine @ [*voxel, 1]
        lps = image.affine @ [*voxel, 1]
        assert np.allclose(ras[:3], lps * [-1, -1, 1])


def test_to_nibabel_frames():
    # one frame: a 3D image
    n = synthetic(frames=1).to_nibabel()
    assert n.shape == SHAPE
    assert n.header.get_zooms() == pytest.approx((2.0, 2.5, 3.0))
    # frames of different durations: no frame duration in the header
    n = synthetic(duration=(30.0, 60.0, 90.0)).to_nibabel()
    assert n.header.get_zooms()[3] == 1.0
    # float data keeps its type
    assert synthetic(dtype=np.float32).to_nibabel().get_data_dtype() == np.float32


def test_to_nibabel_saved_by_nibabel(tmp_path):
    # a NIfTI file written by nibabel holds the values at the positions of the image
    image = synthetic(dtype=np.float32)
    path = tmp_path / "nibabel.nii.gz"
    nib.save(image.to_nibabel(), str(path))
    back = pmedio.load(path)
    assert np.array_equal(back.data, image.data)
    assert np.allclose(back.affine, image.affine, atol=1e-4)
    # the same as the file pmedio.save() writes, read with nibabel
    pmedio.save(tmp_path / "pmedio.nii.gz", image)
    written = nib.as_closest_canonical(nib.load(str(tmp_path / "pmedio.nii.gz")))
    converted = nib.as_closest_canonical(image.to_nibabel())
    assert np.array_equal(written.get_fdata(), converted.get_fdata())
    assert np.allclose(written.affine, converted.affine, atol=1e-4)


@needs_data
def test_to_nibabel_dataset():
    # the voxels in RAS order of nibabel's canonical image are those of load(orientation="RAS")
    path = DATA_DIR / "ecat/pet-dynamic.v"
    canonical = nib.as_closest_canonical(pmedio.load(path).to_nibabel())
    ras = pmedio.load(path, orientation="RAS")
    assert np.array_equal(canonical.get_fdata(), ras.data)
    assert np.allclose(canonical.affine[:3], ras.affine * np.array([[-1], [-1], [1]]), atol=1e-4)


#=============================================================================================
# from_nibabel()

def test_from_nibabel():
    data = (np.arange(int(np.prod(SHAPE)) * 2).reshape((*SHAPE, 2)) * 1.5).astype(np.float32)
    image = pmedio.Image.from_nibabel(nib.Nifti1Image(data, RAS), frame_start=[0.0, 60.0],
                                      frame_duration=[60.0, 60.0], attributes={"PatientWeight": 70.0})
    assert isinstance(image, pmedio.Image)
    assert np.array_equal(image.data, data)
    assert np.allclose(image.affine, AFFINE)
    assert image.frame_start == [0.0, 60.0] and image.frame_duration == [60.0, 60.0]
    assert image.attributes == {"PatientWeight": 70.0}
    assert image.format is None and image.filename is None

    # 3D data is one frame, without timing
    image = pmedio.Image.from_nibabel(nib.Nifti1Image(data[..., 0], RAS))
    assert image.shape == (*SHAPE, 1)
    assert image.frame_start == [0.0] and image.frame_duration == [0.0]

    # other nibabel images (Analyze, NIfTI-2)
    for klass in (nib.AnalyzeImage, nib.Nifti2Image):
        image = pmedio.Image.from_nibabel(klass(data, RAS))
        assert np.array_equal(image.data, data), klass
    assert np.allclose(pmedio.Image.from_nibabel(nib.Nifti2Image(data, RAS)).affine, AFFINE)


def test_from_nibabel_scaled(tmp_path):
    # the physical values: the scaling of the file applied
    data = np.arange(int(np.prod(SHAPE)), dtype=np.int16).reshape(SHAPE)
    n = nib.Nifti1Image(data, RAS)
    n.header.set_data_dtype(np.int16)
    n.header.set_slope_inter(0.5, 10.0)
    nib.save(n, str(tmp_path / "scaled.nii"))
    image = pmedio.Image.from_nibabel(nib.load(str(tmp_path / "scaled.nii")))
    assert np.allclose(image.data[..., 0], data * 0.5 + 10.0)


def test_nibabel_roundtrip(tmp_path):
    image = synthetic()
    back = pmedio.Image.from_nibabel(image.to_nibabel(), image.frame_start, image.frame_duration, image.attributes)
    assert np.array_equal(back.data, image.data)
    assert np.allclose(back.affine, image.affine)
    assert back.attributes == image.attributes
    # and into a file of another format: the synthetic image is in RAS voxel order
    back.save(tmp_path / "image.v")
    written = pmedio.load(tmp_path / "image.v", orientation="RAS")
    assert np.array_equal(written.data, image.data)
    assert np.allclose(written.affine, image.affine, atol=1e-3)
    assert written.attributes["PatientName"] == "Test^Name"


def test_from_nibabel_errors():
    with pytest.raises(TypeError, match="nibabel image"):
        pmedio.Image.from_nibabel(np.zeros(SHAPE))
    with pytest.raises(ValueError, match="3D"):
        pmedio.Image.from_nibabel(nib.Nifti1Image(np.zeros((2, 2, 2, 2, 2), dtype=np.float32), np.eye(4)))
    with pytest.raises(ValueError, match="one value per frame"):
        pmedio.Image.from_nibabel(nib.Nifti1Image(np.zeros((*SHAPE, 2), dtype=np.float32), np.eye(4)),
                                  frame_start=[0.0])


def test_without_nibabel(monkeypatch):
    # nibabel is optional: without it, to_nibabel() says how to get it
    monkeypatch.setitem(sys.modules, "nibabel", None)
    with pytest.raises(ImportError, match="pip install nibabel"):
        synthetic().to_nibabel()
