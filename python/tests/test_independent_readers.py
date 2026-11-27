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

# The files libmedio writes, read with implementations independent of libmedio: every
# image test file is converted into NIfTI, ECAT7 and Interfile with the format independent
# interface (main header of the source, every frame as volume) and read with nibabel
# (NIfTI, ECAT7) and with the own Interfile parser of testsuite/data/tools/make_expected.py.
# The values and positions have to be those of the source as libmedio reads it.

import importlib.util

import numpy as np
import pytest

import pmedio
from test_volume import DATA_DIR, READABLE, needs_data

nib = pytest.importorskip("nibabel")
ecat = pytest.importorskip("nibabel.ecat")

pytestmark = needs_data

TARGETS = {".nii.gz": "nifti", ".v": "ecat", ".hv": "interfile"}
# every readable test file into every target of another format
CASES = [(path, key, json_name, suffix) for path, key, json_name in READABLE for suffix, directory in TARGETS.items()
         if key.split("/")[0] != directory]


def make_expected():
    """the own parsers of testsuite/data/tools/make_expected.py (independent of libmedio)"""
    path = DATA_DIR / "tools" / "make_expected.py"
    spec = importlib.util.spec_from_file_location("make_expected", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def convert(source, target):
    with pmedio.open(source) as f:
        out = pmedio.create(target, like=f, overwrite=True)
        for k, n in enumerate(f.frame_numbers()):
            out.write_volume(f.read_volume(n), k + 1)
        out.close()


def source_values_at(source, affine_lps, shape):
    """the values of the source at the positions of the voxels of another grid (every
    voxel of that grid has to be a voxel of the source)"""
    sa = np.vstack([source.affine, [0, 0, 0, 1]])
    a = np.vstack([np.asarray(affine_lps)[:3], [0, 0, 0, 1]])
    m = np.linalg.inv(sa) @ a
    idx = np.stack(np.meshgrid(*[np.arange(s) for s in shape], indexing="ij"), -1).reshape(-1, 3)
    src = idx @ m[:3, :3].T + m[:3, 3]
    assert np.abs(src - np.round(src)).max() < 1e-3, "the voxels are not on the grid of the source"
    r = np.round(src).astype(int)
    assert np.all((r >= 0) & (r < np.array(source.data.shape[:3]))), "voxels outside of the source"
    return source.data[r[:, 0], r[:, 1], r[:, 2], :].reshape(*shape, -1)


def assert_values(actual, expected):
    scale = max(float(np.abs(expected).max()), 1e-30)
    assert float(np.abs(actual.astype(np.float64) - expected).max()) <= 1e-6 * scale


@pytest.mark.parametrize(("path", "key", "json_name", "suffix"), CASES, ids=[f"{c[1]}->{c[3]}" for c in CASES])
def test_written_files(request, tmp_path, path, key, json_name, suffix):
    source_path = DATA_DIR / path
    source = pmedio.load(source_path)
    frames = source.data.shape[3]
    target = tmp_path / ("image" + suffix)
    convert(source_path, target)

    if suffix == ".nii.gz":
        # nibabel: every voxel at its position (RAS affine of nibabel -> LPS)
        im = nib.load(str(target))
        data = np.asarray(im.get_fdata(dtype=np.float64))
        data = data.reshape(*data.shape[:3], -1)
        assert data.shape[3] == frames
        lps = np.diag([-1.0, -1.0, 1.0, 1.0]) @ im.affine
        assert_values(data, source_values_at(source, lps, data.shape[:3]))

    elif suffix == ".v":
        # nibabel: the stored values x scale factor x calibration factor of every frame in
        # the voxel order of ECAT7 (LPI; nibabel flips them by patient_orientation for display),
        # the voxel size and the timing of the sub headers
        im = ecat.load(str(target))
        header, subheaders = im.header, im.get_subheaders()
        request.addfinalizer(subheaders.fileobj.close)
        assert subheaders.get_nframes() == frames
        lpi = pmedio.load(source_path, orientation="LPI")
        code = int(header["patient_orientation"])
        for f in range(frames):
            raw = np.asarray(subheaders.raw_data_from_fileobj(f)).astype(np.float64)
            if code in (1, 3, 5, 7):
                raw = raw[::-1, ::-1, ::-1]
            elif code in (0, 2, 4, 6):
                raw = raw[:, ::-1, ::-1]
            sh = subheaders.subheaders[f]
            values = raw * float(sh["scale_factor"]) * float(header["ecat_calibration_factor"])
            assert_values(values, lpi.data[..., f].astype(np.float64))
            sizes = [10.0 * float(sh[k]) for k in ("x_pixel_size", "y_pixel_size", "z_pixel_size")]
            assert sizes == pytest.approx([abs(float(lpi.affine[i][i])) for i in range(3)], rel=1e-5)
            assert float(sh["frame_start_time"]) / 1000.0 == pytest.approx(source.frame_start[f], abs=1e-3)
            assert float(sh["frame_duration"]) / 1000.0 == pytest.approx(source.frame_duration[f], abs=1e-3)

    else:
        # the own Interfile parser: matrix, voxel size, timing, the sum of the physical
        # values of every frame and the LPS position of the first voxel (STIR convention)
        expected = make_expected().expected_interfile(str(target), {})
        assert len(expected["frames"]) == frames
        first = np.asarray(source.affine)
        for f, e in enumerate(expected["frames"]):
            assert e["dims"] == list(source.data.shape[:3])
            assert e["pixel_size_mm"] == pytest.approx([float(np.linalg.norm(first[:, i])) for i in range(3)], rel=1e-5)
            assert e["duration_s"] == pytest.approx(source.frame_duration[f], abs=1e-3)
            total = e["raw_sum"] * e["rescale_slope"] + e["rescale_offset"] * int(np.prod(e["dims"]))
            assert total == pytest.approx(float(source.data[..., f].astype(np.float64).sum()), rel=1e-5)
            # the first voxel of the file is a voxel of the source at the same position
            position = np.array(e["first_voxel_lps_mm"])
            index = np.linalg.solve(first[:, :3], position - first[:, 3])
            assert np.abs(index - np.round(index)).max() < 1e-3
            corner = [0 if v < 1 else s - 1 for v, s in zip(np.round(index), source.data.shape[:3], strict=True)]
            assert list(np.round(index).astype(int)) == corner, "the first voxel is no corner of the source"
