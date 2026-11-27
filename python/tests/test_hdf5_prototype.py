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

# Tests of the HDF5 layout (docs/hdf5.md) with h5py as independent implementation:
# - the test datasets written by the prototype testsuite/hdf5/tools/medio2hdf5.py are
#   checked with testsuite/hdf5/tools/check_hdf5.py and read back with h5py; the checker
#   has to refuse files that violate the layout,
# - libmedio reads these files as h5py reads them,
# - the files libmedio writes pass the checker and are read with h5py.

import importlib.util
import json
import pathlib

import numpy as np
import pytest

import pmedio
from test_volume import DATA_DIR, READABLE, needs_data

h5py = pytest.importorskip("h5py")

TOOLS = pathlib.Path(__file__).resolve().parents[2] / "testsuite" / "hdf5" / "tools"
pytestmark = pytest.mark.skipif(not TOOLS.is_dir(), reason="testsuite/hdf5/tools is not available")


def tool(name):
    spec = importlib.util.spec_from_file_location(name, TOOLS / (name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@pytest.fixture(scope="module")
def medio2hdf5():
    return tool("medio2hdf5")


@pytest.fixture(scope="module")
def check_hdf5():
    return tool("check_hdf5")


@needs_data
@pytest.mark.parametrize(("path", "key", "json_name"), READABLE, ids=[r[1] for r in READABLE])
def test_datasets_follow_the_layout(path, key, json_name, tmp_path, medio2hdf5, check_hdf5):
    target = tmp_path / "image.h5"
    medio2hdf5.convert(DATA_DIR / path, target)
    assert [p.name for p in tmp_path.iterdir()] == ["image.h5"]
    assert check_hdf5.check(target).errors == []

    with pmedio.open(DATA_DIR / path) as source, h5py.File(target, "r") as f:
        frames = source.frame_numbers()
        attributes = source.attributes()
        data = f["image/data"]
        assert f["image/volumes/Frame"][()].tolist() == list(frames)
        assert data.shape[0] == len(frames)
        for k, n in enumerate(frames):
            volume = source.read_volume(n)
            # [z][y][x] of the file is (x, y, z) of the volume
            np.testing.assert_array_equal(data[k].transpose(2, 1, 0), volume.values())
            assert f["image/volumes/FrameTimesStart"][k] == volume.frame_start
            assert f["image/volumes/FrameDuration"][k] == volume.frame_duration
        np.testing.assert_array_equal(f["image/affine"][()], source.read_volume(frames[0]).affine)
        assert json.loads(f["header/json"][()]) == attributes
        assert ("Units" in data.attrs) == ("Units" in attributes)
        for name, value in f["header"].attrs.items():
            if isinstance(value, np.ndarray):
                assert [v.decode() if isinstance(v, bytes) else v for v in value] == attributes[name]
            else:
                assert (value.decode() if isinstance(value, bytes) else value) == attributes[name]


def hdf5_support(tmp_path):
    """True if libmedio was built with HDF5 support (it cannot open HDF5 files otherwise)"""
    path = tmp_path / "support.h5"
    with h5py.File(path, "w") as f:
        f.attrs["Format"] = "other"
    with pytest.raises(ValueError, match="is an HDF5 file") as error:
        pmedio.open(path)
    path.unlink()
    return "without HDF5 support" not in str(error.value)


# libmedio reads the files written by the prototype as h5py reads them
@needs_data
@pytest.mark.parametrize(("path", "key", "json_name"), READABLE, ids=[r[1] for r in READABLE])
def test_libmedio_reads_the_layout(path, key, json_name, tmp_path, medio2hdf5):
    if not hdf5_support(tmp_path):
        pytest.skip("libmedio was built without HDF5 support")
    target = tmp_path / "image.h5"
    medio2hdf5.convert(DATA_DIR / path, target)

    with h5py.File(target, "r") as f, pmedio.open(target) as image:
        assert image.format == pmedio.Format.HDF5
        data = f["image/data"]
        frames = f["image/volumes/Frame"][()].tolist()
        assert image.frame_numbers() == frames
        orientation = image.read_volume(frames[0]).orientation
        for k, n in enumerate(frames):
            volume = image.read_volume(n)
            values = volume.values()
            assert values.dtype == data.dtype
            np.testing.assert_array_equal(values, data[k].transpose(2, 1, 0))
            np.testing.assert_array_equal(volume.stored(), values)
            np.testing.assert_array_equal(volume.affine, f["image/affine"][()])
            assert volume.frame_start == f["image/volumes/FrameTimesStart"][k]
            assert volume.frame_duration == f["image/volumes/FrameDuration"][k]

        # the typed attributes of the file, and every key of its JSON object without one
        attributes = image.attributes()
        for name, value in f["header"].attrs.items():
            if isinstance(value, np.ndarray):
                assert attributes[name] == [v.decode() if isinstance(v, bytes) else v for v in value]
            else:
                assert attributes[name] == (value.decode() if isinstance(value, bytes) else value)
        stored = json.loads(f["header/json"][()])
        for name, value in stored.items():
            if name not in ("Frames", "NIfTI", "FrameTimesStart", "FrameDuration"):
                assert attributes[name] == value, name
        assert len(attributes["Frames"]) == len(frames)
        for k, frame in enumerate(attributes["Frames"]):
            assert frame["ActualFrameDuration"] == round(f["image/volumes/FrameDuration"][k] * 1000)
            for name in ("DecayFactor", "FrameReferenceTime"):
                if name in f["image/volumes"]:
                    assert frame[name] == pytest.approx(f["image/volumes/" + name][k], rel=1e-12)

    # the whole image at once, and converted into another format
    loaded = pmedio.load(target)
    with h5py.File(target, "r") as f:
        np.testing.assert_array_equal(loaded.data, np.transpose(f["image/data"][()], (3, 2, 1, 0)))
    converted = tmp_path / "image.nii.gz"
    loaded.save(converted)
    np.testing.assert_array_equal(pmedio.load(converted, orientation=orientation).data, loaded.data)


# files written by libmedio follow the layout: the checker (h5py, independent of libmedio)
# accepts them, and h5py reads the values, positions, timing and attributes of the source
@needs_data
@pytest.mark.parametrize(("path", "key", "json_name"), READABLE, ids=[r[1] for r in READABLE])
def test_libmedio_writes_the_layout(path, key, json_name, tmp_path, check_hdf5):
    if not hdf5_support(tmp_path):
        pytest.skip("libmedio was built without HDF5 support")
    target = tmp_path / "image.h5"

    with pmedio.open(DATA_DIR / path) as source:
        frames = source.frame_numbers()
        attributes = source.attributes()
        with pmedio.create(target, like=source) as out:
            assert out.format == pmedio.Format.HDF5
            for n in frames:
                out.write_volume(source.read_volume(n), n)
            # the file gets its name when it is complete
            assert not target.exists()
        assert [p.name for p in tmp_path.iterdir()] == ["image.h5"]

        result = check_hdf5.check(target)
        assert result.errors == []
        assert not [w for w in result.warnings if "variable-length" in w or "not part of the layout" in w]

        with h5py.File(target, "r") as f:
            assert f.attrs["Format"] == "MEDIO-IMAGE"
            assert f.attrs["Creator"].startswith("libmedio ")
            data = f["image/data"]
            assert data.dtype == np.dtype("<f4")
            assert f["image/volumes/Frame"][()].tolist() == list(frames)
            assert "Gate" not in f["image/volumes"] and "Bed" not in f["image/volumes"]
            for k, n in enumerate(frames):
                # written in the voxel order of ECAT7 with the physical values
                volume = source.read_volume(n)
                volume.reorient("LPI")
                np.testing.assert_array_equal(data[k].transpose(2, 1, 0), volume.values().astype(np.float32))
                np.testing.assert_allclose(f["image/affine"][()], volume.affine, rtol=1e-6, atol=1e-3)
                assert f["image/volumes/FrameTimesStart"][k] == pytest.approx(volume.frame_start, abs=1e-3)
                assert f["image/volumes/FrameDuration"][k] == pytest.approx(volume.frame_duration, abs=1e-3)

            # the unit in both notations, or none
            assert ("Units" in data.attrs) == ("units" in data.attrs)
            if attributes.get("Units") in ("BQML", "KBQCC"):
                assert data.attrs["Units"] == attributes["Units"]
                assert data.attrs["units"] == {"BQML": "Bq/mL", "KBQCC": "kBq/mL"}[attributes["Units"]]

            # the attributes of the dictionary are typed attributes
            typed = dict(f["header"].attrs)
            for name, value in attributes.items():
                entry = pmedio.dictionary_entry(name)
                if entry is None or entry["scope"] != "main" or entry["vr"] == "SQ":
                    continue
                stored = typed.pop(name)
                if isinstance(value, list):
                    assert list(stored) == value
                elif isinstance(value, str):
                    assert stored == value
                else:
                    assert stored == value and stored.dtype == (np.int32 if entry["vr"] in ("IS", "US", "UL") else np.float64)
            assert typed == {}

            # and all attributes are in the JSON object, with those of the frames
            stored = json.loads(f["header/json"][()])
            for name, value in attributes.items():
                if name not in ("Frames", "NIfTI", "FrameTimesStart", "FrameDuration"):
                    assert stored[name] == value, name
            assert len(stored["Frames"]) == len(frames)
            for name in ("DecayFactor", "FrameReferenceTime"):
                source_frames = attributes.get("Frames", [])
                if source_frames and all(name in frame for frame in source_frames):
                    np.testing.assert_allclose(f["image/volumes/" + name][()], [frame[name] for frame in source_frames],
                                               rtol=1e-12)

        # read again by libmedio: the attributes of the source
        with pmedio.open(target) as back:
            for name, value in attributes.items():
                if name not in ("Frames", "NIfTI", "FrameTimesStart", "FrameDuration"):
                    assert back.attributes()[name] == value, name


def test_save_and_create_hdf5(tmp_path, check_hdf5):
    if not hdf5_support(tmp_path):
        pytest.skip("libmedio was built without HDF5 support")
    rng = np.random.default_rng(1)
    values = (rng.random((5, 4, 3, 2)) * 1000).astype(np.float64)
    affine = np.array([[2.0, 0, 0, -4], [0, 2.5, 0, -3], [0, 0, -3.0, 10]])
    image = pmedio.Image(values, affine=affine, frame_start=[0, 60], frame_duration=[60, 120],
                         attributes={"PatientWeight": 72.5, "Units": "BQML", "SeriesNumber": 7})
    for name in ("image.h5", "image.hdf5", "image.hdf"):
        path = tmp_path / name
        image.save(path)
        assert check_hdf5.check(path).errors == []
        with h5py.File(path, "r") as f:
            # float64 values stay float64
            assert f["image/data"].dtype == np.dtype("<f8")
            assert f["header"].attrs["SeriesNumber"] == 7 and f["header"].attrs["SeriesNumber"].dtype == np.int32
            assert f["image/volumes/FrameDuration"][()].tolist() == [60, 120]
        back = pmedio.load(path)
        assert back.format == pmedio.Format.HDF5
        assert back.attributes["PatientWeight"] == 72.5
        assert back.frame_start == [0, 60] and back.frame_duration == [60, 120]
        # the voxels of the image are in the order libmedio writes (LPI): the same values
        np.testing.assert_array_equal(back.data, values)
        np.testing.assert_array_equal(back.affine, affine)
        path.unlink()

    # existing files are replaced only with overwrite=True, and only by a complete file
    path = tmp_path / "image.h5"
    path.write_bytes(b"existing")
    with pytest.raises(FileExistsError):
        image.save(path)
    assert path.read_bytes() == b"existing"
    image.save(path, overwrite=True)
    assert check_hdf5.check(path).errors == []
    assert sorted(p.name for p in tmp_path.iterdir()) == ["image.h5"]

    # the format given explicitly
    other = tmp_path / "image.dat"
    image.save(other, format=pmedio.Format.HDF5)
    assert pmedio.load(other).format == pmedio.Format.HDF5


# an HDF5 file gets its name when it is closed: if that fails, close() says so
@needs_data
def test_failed_completion_is_reported(tmp_path):
    if not hdf5_support(tmp_path):
        pytest.skip("libmedio was built without HDF5 support")
    volume = pmedio.Volume(np.ones((4, 3, 2), dtype=np.float32), affine=np.eye(3, 4))

    def occupy(path):
        # a directory that cannot be replaced by the new file
        path.mkdir()
        (path / "content").write_text("kept")

    # close()
    target = tmp_path / "close.h5"
    out = pmedio.create(target)
    out.write_volume(volume, 1)
    occupy(target)
    with pytest.raises(OSError, match="unable to complete .*close.h5"):
        out.close()
    assert out.closed
    out.close()   # reported once

    # leaving the with block
    target = tmp_path / "with.h5"
    with pytest.raises(OSError, match="unable to complete"):
        with pmedio.create(target) as out:
            out.write_volume(volume, 1)
            occupy(target)

    # an exception on its way is not replaced
    target = tmp_path / "exception.h5"
    with pytest.raises(KeyError):
        with pmedio.create(target) as out:
            out.write_volume(volume, 1)
            occupy(target)
            raise KeyError("other")

    # nothing but the directories is left, with their content
    assert sorted(p.name for p in tmp_path.iterdir()) == ["close.h5", "exception.h5", "with.h5"]
    for name in ("close.h5", "exception.h5", "with.h5"):
        assert [p.name for p in (tmp_path / name).iterdir()] == ["content"]

    # a file with the main header of a source but without a volume is not written
    target = tmp_path / "empty.h5"
    with pmedio.open(DATA_DIR / "ecat/pet-static.v") as source:
        with pytest.raises(OSError, match="unable to complete .*no volume"):
            with pmedio.create(target, like=source):
                pass
    assert not target.exists()

    # a file created without a source and without a volume is removed silently, like the
    # files of the other formats
    target = tmp_path / "pending.h5"
    with pmedio.create(target):
        pass
    assert not target.exists()


def test_other_hdf5_files_are_refused(tmp_path):
    if not hdf5_support(tmp_path):
        pytest.skip("libmedio was built without HDF5 support")
    path = tmp_path / "other.h5"
    with h5py.File(path, "w") as f:
        f.create_dataset("ITKImage/0/VoxelData", data=np.zeros((2, 3, 4), dtype="<f4"))
    with pytest.raises(ValueError, match="is an HDF5 file.*another layout"):
        pmedio.open(path)
    with pytest.raises(ValueError, match="is an HDF5 file"):
        pmedio.load(path)


@needs_data
def test_no_file_is_left_behind(tmp_path, medio2hdf5):
    with pytest.raises(Exception):
        medio2hdf5.convert(DATA_DIR / "datasets.json", tmp_path / "image.h5")
    assert list(tmp_path.iterdir()) == []


def valid_file(path):
    """a minimal valid file written with h5py alone"""
    text = h5py.string_dtype("utf-8")
    with h5py.File(path, "w") as f:
        f.attrs.create("Format", "MEDIO-IMAGE", dtype=text)
        f.attrs.create("FormatVersion", "0.1", dtype=text)
        f.attrs.create("Creator", "test", dtype=text)
        data = f.create_dataset("image/data", data=np.zeros((2, 3, 4, 5), dtype="<f4"), chunks=(1, 3, 4, 5),
                                shuffle=True, compression="gzip")
        data.attrs.create("Units", "BQML", dtype=text)
        data.attrs.create("units", "Bq/mL", dtype=text)
        affine = np.array([[2.0, 0, 0, -4], [0, 2.0, 0, -3], [0, 0, -3.0, 10]])
        f.create_dataset("image/affine", data=affine).attrs.create("units", "mm", dtype=text)
        f.create_dataset("image/volumes/Frame", data=np.array([1, 2], dtype="<i4"))
        for name, values in (("FrameTimesStart", [0.0, 60.0]), ("FrameDuration", [60.0, 120.0])):
            f.create_dataset("image/volumes/" + name, data=np.array(values)).attrs.create("units", "s", dtype=text)
        header = f.create_group("header")
        header.attrs["PatientWeight"] = np.float64(72.5)
        header.attrs.create("AcquisitionDateTime", "2026-06-03T17:04:56+02:00", dtype=text)


def replace(f, name, data, units=None):
    del f[name]
    dataset = f.create_dataset(name, data=data)
    if units:
        dataset.attrs.create("units", units, dtype=h5py.string_dtype("utf-8"))


def no_format(f):
    del f.attrs["Format"]


def other_major_version(f):
    f.attrs.modify("FormatVersion", "1.0")


def integer_data(f):
    del f["image/data"]
    f.create_dataset("image/data", data=np.zeros((2, 3, 4, 5), dtype="<i2"), chunks=(1, 3, 4, 5))


def three_dimensions(f):
    del f["image/data"]
    f.create_dataset("image/data", data=np.zeros((3, 4, 5), dtype="<f4"), chunks=(3, 4, 5))


def not_chunked(f):
    del f["image/data"]
    f.create_dataset("image/data", data=np.zeros((2, 3, 4, 5), dtype="<f4"))


def chunk_of_two_volumes(f):
    del f["image/data"]
    f.create_dataset("image/data", data=np.zeros((2, 3, 4, 5), dtype="<f4"), chunks=(2, 3, 4, 5))


def other_filter(f):
    del f["image/data"]
    f.create_dataset("image/data", data=np.zeros((2, 3, 4, 5), dtype="<f4"), chunks=(1, 3, 4, 5), compression="lzf")


def one_unit_only(f):
    del f["image/data"].attrs["units"]


def no_affine(f):
    del f["image/affine"]


def singular_affine(f):
    replace(f, "image/affine", np.zeros((3, 4)), "mm")


def no_frame_duration(f):
    del f["image/volumes/FrameDuration"]


def one_frame_time(f):
    replace(f, "image/volumes/FrameTimesStart", np.array([0.0]), "s")


def frame_times_in_ms(f):
    replace(f, "image/volumes/FrameTimesStart", np.array([0.0, 60000.0]), "ms")


def same_frame_twice(f):
    replace(f, "image/volumes/Frame", np.array([1, 1], dtype="<i4"))


def weight_as_text(f):
    f["header"].attrs["PatientWeight"] = "72.5"


def date_without_separators(f):
    f["header"].attrs["AcquisitionDateTime"] = "20260603170456"


def code_sequence_as_attribute(f):
    f["header"].attrs["RadionuclideCodeSequence"] = "F-18"


def json_is_no_object(f):
    f["header"].create_dataset("json", data="[1, 2", dtype=h5py.string_dtype("utf-8"))


INVALID = [no_format, other_major_version, integer_data, three_dimensions, not_chunked, chunk_of_two_volumes,
           other_filter, one_unit_only, no_affine, singular_affine, no_frame_duration, one_frame_time,
           frame_times_in_ms, same_frame_twice, weight_as_text, date_without_separators,
           code_sequence_as_attribute, json_is_no_object]


def test_checker_accepts_a_minimal_file(tmp_path, check_hdf5):
    valid_file(tmp_path / "valid.h5")
    assert check_hdf5.check(tmp_path / "valid.h5").errors == []


@pytest.mark.parametrize("change", INVALID, ids=[c.__name__ for c in INVALID])
def test_checker_refuses_invalid_files(change, tmp_path, check_hdf5):
    if change in (weight_as_text, date_without_separators, code_sequence_as_attribute):
        pytest.importorskip("pydicom")  # the checker takes the types of the attributes from pydicom
    path = tmp_path / "invalid.h5"
    valid_file(path)
    with h5py.File(path, "r+") as f:
        change(f)
    assert check_hdf5.check(path).errors != []


def test_checker_refuses_other_files(tmp_path, check_hdf5):
    path = tmp_path / "text.h5"
    path.write_text("no HDF5 file")
    assert check_hdf5.check(path).errors != []
