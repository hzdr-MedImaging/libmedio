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

# Tests of the format independent access (pmedio.Volume, pmedio.open(),
# pmedio.create()).
#
# The expected values are independent of libmedio: the values of
# testsuite/data/<format>/<file>.json (nibabel, or documented manual values)
# and the landmarks of testsuite/data/datasets.json (DICOM patient
# coordinates of voxels of the files). Tests that need the test data are
# skipped if it is not available (e.g. in the conda package test); the data
# directory can be set with MEDIO_TEST_DATA_DIR.

import json
import os
import pathlib

import numpy as np
import pytest

import pmedio

DATA_DIR = pathlib.Path(os.environ.get("MEDIO_TEST_DATA_DIR",
                                       pathlib.Path(__file__).resolve().parents[2] / "testsuite" / "data"))

needs_data = pytest.mark.skipif(not (DATA_DIR / "datasets.json").is_file(),
                                reason="test data (testsuite/data) not available")

# files of the test datasets readable by libmedio: path, key in datasets.json, expected values
READABLE = [
    ("ecat/pet-static.v", "ecat/pet-static.v", "ecat/pet-static.v.json"),
    ("ecat/pet-dynamic.v", "ecat/pet-dynamic.v", "ecat/pet-dynamic.v.json"),
    ("philips/pet-static.img", "philips/pet-static.img", "philips/pet-static.img.json"),
    ("philips/pet-static.syn", "philips/pet-static.syn", "philips/pet-static.syn.json"),
    ("philips/pet-dynamic.img", "philips/pet-dynamic.img", "philips/pet-dynamic.img.json"),
    ("philips/pet-dynamic.syn", "philips/pet-dynamic.syn", "philips/pet-dynamic.syn.json"),
    ("nifti/pet-static.nii.gz", "nifti/pet-static.nii.gz", "nifti/pet-static.nii.gz.json"),
    ("nifti/pet-static.pair/pet-static.hdr", "nifti/pet-static.pair", "nifti/pet-static.pair.json"),
    ("nifti/pet-dynamic.nii.gz", "nifti/pet-dynamic.nii.gz", "nifti/pet-dynamic.nii.gz.json"),
    ("nifti/pet-dynamic.pair/pet-dynamic.hdr", "nifti/pet-dynamic.pair", "nifti/pet-dynamic.pair.json"),
]


# numpy data types of the ECAT7 data types (CECATSubHeader::Data_Type) and NIfTI datatypes
ECAT_TYPES = {1: np.uint8, 5: np.float32, 6: np.int16, 7: np.int32}
NIFTI_TYPES = {2: np.uint8, 4: np.int16, 8: np.int32, 16: np.float32, 64: np.float64,
               256: np.int8, 512: np.uint16, 768: np.uint32}


def load_json(name):
    with open(DATA_DIR / name) as f:
        return json.load(f)


def dataset_of(key):
    return pathlib.PurePosixPath(key).name.split(".")[0]


class Expected:
    """Expected frames of a test file: dimensions, voxel size (mm), stored
    value statistics, physical scale (physical value = stored x scale +
    intercept) and frame timing (s, None if the file has none)."""

    def __init__(self, key, json_name):
        e = load_json(json_name)
        dataset = dataset_of(key)
        ecat = load_json(f"ecat/{dataset}.v.json")
        calibration = ecat["main_header"]["calibration_factor"]
        ecat_frames = ecat["matrices"]

        self.intercept = 0.0
        if e["format"] == "ecat7":
            m = e["matrices"]
            self.dims = m[0]["dims"]
            self.dtype = ECAT_TYPES[m[0]["data_type"]]
            self.voxel_size = [10.0 * s for s in m[0]["pixel_size_cm"]]
            self.stats = [(f["raw_min"], f["raw_max"], f["raw_sum"]) for f in m]
            self.scales = [f["scale_factor"] * calibration for f in m]
            self.timing = [(f["frame_start_time_ms"] / 1000.0, f["frame_duration_ms"] / 1000.0) for f in m]
            self.orientation = "LPI"
        elif e["format"].startswith("philips"):
            # the same stored values as the ECAT7 file of the dataset (see datasets.json)
            self.dims = e["dims"]
            self.dtype = np.dtype(e["data_type"]).type
            self.voxel_size = e["pixel_size_mm"]
            self.stats = [(f["raw_min"], f["raw_max"], f["raw_sum"]) for f in e["volumes"]]
            self.scales = [f["scale_factor"] * calibration for f in ecat_frames]
            self.timing = [(f["frame_start_time_ms"] / 1000.0, f["frame_duration_ms"] / 1000.0) for f in ecat_frames]
            self.orientation = "LPI"
        else:
            h = e["header"]
            self.dims = h["dim"][1:4]
            self.dtype = NIFTI_TYPES[h["datatype"]]
            self.voxel_size = h["pixdim"][1:4]
            self.stats = [(f["raw_min"], f["raw_max"], f["raw_sum"]) for f in e["volumes"]]
            slope = h["scl_slope"] if h["scl_slope"] != 0.0 else 1.0
            self.scales = [slope] * len(self.stats)
            self.intercept = h["scl_inter"]
            self.timing = None
            if h["sform_code"] > 0:
                # nibabel: RAS affine of the sform -> LPS
                # (LPS: +x towards left, +y towards posterior, +z towards superior)
                lps = np.diag([-1.0, -1.0, 1.0]) @ np.array(h["srow"])
                code = ""
                for a in range(3):
                    r = int(np.argmax(np.abs(lps[:3, a])))
                    code += ("RL", "AP", "IS")[r][int(lps[r, a] > 0)]
                self.orientation = code
            else:
                # files without qform and sform: first axis towards patient left,
                # second towards posterior, third towards superior (CNIFTIConverter.h)
                self.orientation = "LPS"

        self.landmarks = []
        for lm in load_json("datasets.json")["datasets"][dataset].get("landmarks", []):
            if key in lm["voxel"]:
                self.landmarks.append((lm["voxel"][key], np.array(lm["world_mm"])))


def check_landmarks(volume, landmarks):
    """Distances between the landmarks (the files do not all store the
    absolute position of the cropped volume)."""
    for a in range(len(landmarks)):
        for b in range(a + 1, len(landmarks)):
            (va, wa), (vb, wb) = landmarks[a], landmarks[b]
            d = np.array(volume.position(*vb)) - np.array(volume.position(*va))
            assert d == pytest.approx(wb - wa, abs=0.1)


def ecat_landmarks(volume, dataset):
    """Landmarks of the ECAT7 file of a dataset as voxels of a volume
    without permuted axes (flipped axes are mapped)."""
    reference = "LPI"
    result = []
    for voxel, world in Expected(f"ecat/{dataset}.v", f"ecat/{dataset}.v.json").landmarks:
        v = [voxel[a] if volume.orientation[a] == reference[a] else volume.shape[a] - 1 - voxel[a] for a in range(3)]
        result.append((v, world))
    return result


#=============================================================================================
# reading the files of the test datasets

@needs_data
@pytest.mark.parametrize(("path", "key", "json_name"), READABLE, ids=[r[1] for r in READABLE])
def test_read_datasets(path, key, json_name):
    expected = Expected(key, json_name)

    with pmedio.open(DATA_DIR / path) as f:
        assert not f.writable
        assert f.filename == str((DATA_DIR / path).resolve())
        assert f.frame_numbers() == list(range(1, len(expected.stats) + 1))

        for k, n in enumerate(f.frame_numbers()):
            v = f.read_volume(n)
            assert isinstance(v, pmedio.Volume)
            assert v.shape == tuple(expected.dims)
            assert v.dtype == expected.dtype
            assert v.voxel_size == pytest.approx(expected.voxel_size, rel=1e-5)
            assert v.orientation == expected.orientation

            stored = v.stored()
            assert stored.shape == tuple(expected.dims)
            raw_min, raw_max, raw_sum = expected.stats[k]
            assert stored.min() == pytest.approx(raw_min, rel=1e-6)
            assert stored.max() == pytest.approx(raw_max, rel=1e-6)
            assert float(stored.sum(dtype=np.float64)) == pytest.approx(raw_sum, rel=1e-9)

            # physical values with the scale factor of the frame
            assert v.scale_factor == pytest.approx(expected.scales[k], rel=1e-5)
            assert v.intercept == expected.intercept
            values = v.values()
            assert values.dtype == np.float32
            assert float(values.sum(dtype=np.float64)) == pytest.approx(raw_sum * expected.scales[k] + v.intercept * stored.size,
                                                                       rel=1e-5)

            if expected.timing is not None:
                assert (v.frame_start, v.frame_duration) == pytest.approx(expected.timing[k], abs=1e-3)

            check_landmarks(v, expected.landmarks)

        # the default frame is the first one
        assert np.array_equal(f.read_volume().stored(), f.read_volume(f.frame_numbers()[0]).stored())


@needs_data
def test_read_errors(tmp_path):
    with pytest.raises(FileNotFoundError):
        pmedio.open(tmp_path / "missing.v")
    with pytest.raises(ValueError, match="not a supported image file"):
        pmedio.open(DATA_DIR / "datasets.json")
    with pytest.raises(TypeError):
        pmedio.open(42)

    f = pmedio.open(DATA_DIR / "ecat/pet-dynamic.v")
    for frame in (0, 4):
        with pytest.raises(RuntimeError, match=f"unable to read frame {frame}"):
            f.read_volume(frame)
    with pytest.raises(ValueError, match="opened for reading"):
        f.write_volume(f.read_volume(1), 1)

    f.close()
    assert f.closed
    with pytest.raises(ValueError, match="closed file"):
        f.frame_numbers()
    f.close()  # closing again is fine


#=============================================================================================
# pmedio.Volume

@pytest.mark.parametrize("dtype", [np.uint8, np.int8, np.int16, np.uint16, np.int32, np.uint32, np.float32, np.float64])
def test_volume_from_array(dtype):
    # a C ordered array: indexing [i, j, k] has to stay the same
    data = (np.arange(4 * 3 * 2) % 100).astype(dtype).reshape((4, 3, 2))
    affine = [[2.0, 0, 0, 10], [0, -3.0, 0, 20], [0, 0, -4.0, 30]]
    v = pmedio.Volume(data, affine=affine, scale_factor=0.5, intercept=1.0, frame_start=60, frame_duration=30)

    assert v.shape == (4, 3, 2)
    assert len(v) == 24
    assert v.dtype == np.dtype(dtype)
    assert np.array_equal(v.stored(), data)
    assert v.stored().dtype == np.dtype(dtype)

    values = v.values()
    assert values.dtype == (np.float64 if dtype == np.float64 else np.float32)
    assert np.allclose(values, data.astype(np.float64) * 0.5 + 1.0)

    assert np.array_equal(v.affine, np.array(affine))
    assert v.voxel_size == pytest.approx((2.0, 3.0, 4.0))
    assert v.orientation == "LAI"
    assert v.position(1, 2, 1) == pytest.approx((12.0, 14.0, 26.0))
    assert (v.frame_start, v.frame_duration) == (60.0, 30.0)
    assert repr(v) == f"<pmedio.Volume 4x3x2 {np.dtype(dtype).name} LAI>"


def test_volume_defaults_and_errors():
    v = pmedio.Volume()
    assert not v
    assert repr(v) == "<pmedio.Volume (empty)>"
    with pytest.raises(ValueError, match="no data"):
        v.stored()

    v = pmedio.Volume(np.zeros((2, 2, 2), dtype=np.int16))
    assert v
    assert np.array_equal(v.affine, np.eye(3, 4))
    assert v.scale_factor == 1.0
    assert v.intercept == 0.0

    # big endian data is converted into the byte order of this machine
    big = np.arange(8, dtype=">i2").reshape((2, 2, 2))
    assert np.array_equal(pmedio.Volume(big).stored(), big)

    with pytest.raises(TypeError, match="unsupported data type 'int64'"):
        pmedio.Volume(np.zeros((2, 2, 2), dtype=np.int64))
    with pytest.raises(TypeError, match="unsupported data type 'bool'"):
        pmedio.Volume(np.zeros((2, 2, 2), dtype=bool))
    with pytest.raises(ValueError, match="3D array"):
        pmedio.Volume(np.zeros((2, 2), dtype=np.int16))
    with pytest.raises(ValueError, match="3 x 4 or 4 x 4"):
        pmedio.Volume(np.zeros((2, 2, 2), dtype=np.int16), affine=np.eye(3))
    with pytest.raises(ValueError, match="0 0 0 1"):
        v.affine = [[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 1, 1]]

    # a 4 x 4 affine
    v.affine = [[1, 0, 0, 5], [0, 1, 0, 6], [0, 0, 1, 7], [0, 0, 0, 1]]
    assert v.position(0, 0, 0) == pytest.approx((5.0, 6.0, 7.0))


def test_volume_plane_scale_factors():
    data = np.ones((2, 2, 3), dtype=np.int16)
    v = pmedio.Volume(data)
    v.set_scale_factor(1, 2.0)
    v.set_scale_factor(2, 4.0)
    assert v.scale_factors == [1.0, 2.0, 4.0]
    with pytest.raises(ValueError, match="different scale factors"):
        _ = v.scale_factor
    assert np.array_equal(v.values()[0, 0, :], [1.0, 2.0, 4.0])
    with pytest.raises(IndexError):
        v.set_scale_factor(3, 1.0)

    v.convert_to_physical_values()
    assert v.dtype == np.float32
    assert v.scale_factors == [1.0, 1.0, 1.0]
    assert np.array_equal(v.stored()[1, 1, :], [1.0, 2.0, 4.0])

    v.scale_factor = 3.0
    assert v.scale_factors == [3.0, 3.0, 3.0]


@pytest.mark.parametrize("orientation", ["RAS", "LPI", "SAR", "IRP"])
def test_volume_reorient(orientation):
    # LPI volume with an offset; every voxel has to keep its value at its position
    data = np.arange(5 * 4 * 3, dtype=np.int32).reshape((5, 4, 3))
    affine = np.array([[2.0, 0, 0, -4], [0, 3.0, 0, 7], [0, 0, -1.5, 100]])
    v = pmedio.Volume(data, affine=affine)
    assert v.orientation == "LPI"

    assert v.reorient(orientation) is False
    assert v.orientation == orientation
    assert sorted(v.shape) == sorted(data.shape)

    inverse = np.linalg.inv(np.vstack([v.affine, [0, 0, 0, 1]]))
    for i, j, k in [(0, 0, 0), (4, 3, 2), (1, 2, 0), (3, 0, 1)]:
        p = np.append(affine @ [i, j, k, 1], 1.0)
        target = np.rint(inverse @ p)[:3].astype(int)
        assert v.stored()[tuple(target)] == data[i, j, k]

    with pytest.raises(ValueError, match="invalid orientation"):
        v.reorient("LLL")


def test_volume_reorient_oblique():
    c, s = np.cos(np.radians(10)), np.sin(np.radians(10))
    v = pmedio.Volume(np.zeros((2, 2, 2), dtype=np.int16), affine=[[c, -s, 0, 0], [s, c, 0, 0], [0, 0, 1, 0]])
    assert v.reorient("RAS") is True
    assert v.orientation == "RAS"


#=============================================================================================
# writing (pmedio.create())

TARGETS = [".v", ".nii", ".nii.gz", ".hdr", ".img", ".syn"]
SOURCES = [
    ("ecat/pet-dynamic.v", "ecat/pet-dynamic.v", "ecat/pet-dynamic.v.json"),
    ("philips/pet-static.syn", "philips/pet-static.syn", "philips/pet-static.syn.json"),
    ("nifti/pet-dynamic.nii.gz", "nifti/pet-dynamic.nii.gz", "nifti/pet-dynamic.nii.gz.json"),
]

FORMATS = {".v": pmedio.Format.ECAT, ".nii": pmedio.Format.NIFTI, ".nii.gz": pmedio.Format.NIFTI,
           ".hdr": pmedio.Format.NIFTI, ".img": pmedio.Format.Philips, ".syn": pmedio.Format.Philips}


@needs_data
@pytest.mark.parametrize("suffix", TARGETS)
@pytest.mark.parametrize(("path", "key", "json_name"), SOURCES, ids=[s[1] for s in SOURCES])
def test_write_formats(tmp_path, path, key, json_name, suffix):
    expected = Expected(key, json_name)
    out_path = tmp_path / ("out" + suffix)

    with pmedio.open(DATA_DIR / path) as source:
        with pmedio.create(out_path, like=source) as out:
            assert out.writable
            assert out.format == FORMATS[suffix]
            with pytest.raises(ValueError, match="created for writing"):
                out.read_volume(1)
            for k, n in enumerate(source.frame_numbers()):
                out.write_volume(source.read_volume(n), k + 1)

    with pmedio.open(out_path) as f:
        assert f.format == FORMATS[suffix]
        assert f.frame_numbers() == list(range(1, len(expected.stats) + 1))
        for k, n in enumerate(f.frame_numbers()):
            v = f.read_volume(n)
            assert v.shape == tuple(expected.dims)
            assert v.voxel_size == pytest.approx(expected.voxel_size, rel=1e-5)

            # the physical values of the source (one quantization step of requantized voxels)
            raw_sum = expected.stats[k][2]
            values = v.values().astype(np.float64)
            assert values.sum() == pytest.approx(raw_sum * expected.scales[k], rel=1e-4)
            assert values.max() == pytest.approx(expected.stats[k][1] * expected.scales[k], rel=1e-4)

            if expected.timing is not None:
                assert (v.frame_start, v.frame_duration) == pytest.approx(expected.timing[k], abs=1e-3)

            check_landmarks(v, ecat_landmarks(v, dataset_of(key)))


@needs_data
@pytest.mark.parametrize("suffix", TARGETS)
def test_write_numpy_volume(tmp_path, suffix):
    # a volume built from numpy in RAS order (nibabel convention) with the
    # geometry, data type and scale factor of the source: every value has to
    # keep its position in the voxel order of the target
    with pmedio.open(DATA_DIR / "ecat/pet-static.v") as source:
        v = source.read_volume(1)
        v.reorient("RAS")
        data = np.asfortranarray(v.stored()[::-1, :, ::-1])  # other values at the positions
        mine = pmedio.Volume(data, affine=v.affine, scale_factor=v.scale_factor,
                             frame_start=12.5, frame_duration=1799.25)

        with pmedio.create(tmp_path / ("out" + suffix), like=source) as out:
            out.write_volume(mine, 1)

    with pmedio.open(tmp_path / ("out" + suffix)) as f:
        w = f.read_volume(1)
        assert w.orientation in ("LPI", "LPS")
        w.reorient("RAS")
        assert np.allclose(w.affine, mine.affine, atol=1e-3)
        assert np.allclose(w.values(), mine.values(), rtol=1e-6)
        assert (w.frame_start, w.frame_duration) == pytest.approx((12.5, 1799.25), abs=1e-3)


@needs_data
@pytest.mark.parametrize("suffix", [".nii", ".nii.gz", ".hdr"])
def test_write_nifti_frame_timing(tmp_path, suffix):
    # NIfTI headers only describe equally spaced volumes: the timing of every
    # volume is stored as JSON metadata, also if the volumes are written in
    # another order and have a timing of their own
    timing = [(0.0, 30.0), (30.125, 59.75), (90.0, 90.0)]
    with pmedio.open(DATA_DIR / "ecat/pet-dynamic.v") as source:
        with pmedio.create(tmp_path / ("out" + suffix), like=source) as out:
            for n in (3, 1, 2):
                v = source.read_volume(n)
                v.frame_start, v.frame_duration = timing[n - 1]
                out.write_volume(v, n)

    with pmedio.open(tmp_path / ("out" + suffix)) as f:
        assert [(f.read_volume(n).frame_start, f.read_volume(n).frame_duration) for n in f.frame_numbers()] == timing


@needs_data
def test_write_errors(tmp_path):
    source = pmedio.open(DATA_DIR / "ecat/pet-static.v")

    # existing files are replaced only with overwrite=True
    target = tmp_path / "out.v"
    pmedio.create(target, like=source).close()
    with pytest.raises(FileExistsError):
        pmedio.create(target, like=source)
    pmedio.create(str(target), like=source, overwrite=True).close()

    with pytest.raises(ValueError, match="unknown file format"):
        pmedio.create(tmp_path / "out.xyz", like=source)
    with pytest.raises(ValueError, match="cannot be written"):
        pmedio.create(tmp_path / "out.xyz", like=source, format=pmedio.Format.ConcordeMicropet)

    out = pmedio.create(tmp_path / "out.nii", like=source)
    with pytest.raises(ValueError, match="opened for reading"):
        pmedio.create(tmp_path / "other.v", like=out)

    # the dimensions have to match the main header written
    with pytest.raises(RuntimeError, match="unable to write frame 1"):
        out.write_volume(pmedio.Volume(np.zeros((2, 2, 2), dtype=np.float32)), 1)
    out.close()

    # the format given explicitly
    with pmedio.create(tmp_path / "ecat.data", like=source, format=pmedio.Format.ECAT) as f:
        assert f.format == pmedio.Format.ECAT


def test_formats():
    assert pmedio.Format is pmedio.MedIOImage.FormatType
    assert pmedio.Format.NIFTI != pmedio.Format.ECAT
