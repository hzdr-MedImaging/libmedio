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

# Tests of pmedio.MedIOImage, pmedio.read() and pmedio.write().
#
# They describe the behaviour of the Python interface itself (constructors,
# conversion of arguments, numpy and buffer access, exceptions), so that it
# stays the same when the implementation of the bindings changes. The expected
# values of the test data are independent of libmedio (testsuite/data/<format>/
# <file>.json, generated with nibabel); tests that need the test data are
# skipped if it is not available. The data directory can be set with
# MEDIO_TEST_DATA_DIR.

import datetime
import json
import os
import pathlib
import time

import numpy as np
import pytest

import pmedio

DATA_DIR = pathlib.Path(os.environ.get("MEDIO_TEST_DATA_DIR",
                                       pathlib.Path(__file__).resolve().parents[2] / "testsuite" / "data"))

needs_data = pytest.mark.skipif(not (DATA_DIR / "datasets.json").is_file(),
                                reason="test data (testsuite/data) not available")

MedIOImage = pmedio.MedIOImage

SHAPE = (2, 3, 4, 5)


def ramp(shape=SHAPE, dtype=np.float32):
    """Array in the voxel order of MedIOImage (x runs fastest) with a different value in every voxel"""
    return np.arange(np.prod(shape), dtype=dtype).reshape(shape, order="F")


@pytest.fixture(params=["UTC", "Europe/Berlin", "Asia/Tokyo", "America/Los_Angeles"])
def timezone(request, monkeypatch):
    """Runs a test in several time zones, independent of the time zone of the machine"""
    if not hasattr(time, "tzset"):
        pytest.skip("time.tzset() not available")
    monkeypatch.setenv("TZ", request.param)
    time.tzset()
    yield request.param
    monkeypatch.undo()
    time.tzset()


#-----------------------------------------------------------------------------------------------
# module and class layout

def test_module_layout():
    assert pmedio.__doc__ == "libmedio python interface"
    assert isinstance(pmedio.__version__, str)
    for name in ("File", "Format", "MedIO", "MedIOImage", "Volume", "create", "dictionary_entry", "open",
                 "read", "write"):
        assert hasattr(pmedio, name), name
    for name in ("DataType", "DimType", "FormatType", "dimType", "filename", "format", "mhead", "nbytes",
                 "ndim", "read", "shape", "shead", "size", "tdim", "toarray", "write", "xdim", "ydim", "zdim"):
        assert hasattr(MedIOImage, name), name


def test_library_information():
    assert pmedio.MedIO.name() == "libmedio"
    major, minor = pmedio.MedIO.majorVersion(), pmedio.MedIO.minorVersion()
    assert type(major) is int and type(minor) is int
    assert pmedio.MedIO.version().startswith(f"{major}.{minor}")
    assert pmedio.MedIO.qtVersion().startswith(f"{pmedio.MedIO.qtMajorVersion()}.{pmedio.MedIO.qtMinorVersion()}.")
    assert type(pmedio.MedIO.qtBuildVersion()) is int
    assert type(pmedio.MedIO.buildDate()) is str
    assert type(pmedio.MedIO.copyright()) is str


def test_constants():
    # formats, dimension types and data types are passed and returned as strings, the
    # constants are these strings
    assert MedIOImage.FormatType is pmedio.Format
    assert pmedio.Format.Unknown == "unknown"
    assert pmedio.Format.ConcordeMicropet == "micropet"
    assert pmedio.Format.ECAT == "ecat"
    assert pmedio.Format.Philips == "philips"
    assert pmedio.Format.PhilipsBin == "philipsbin"
    assert pmedio.Format.NIFTI == "nifti"
    assert MedIOImage.DimType.UNKNOWN == "unknown"
    assert MedIOImage.DimType.FRAMES == "frames"
    assert MedIOImage.DimType.GATES == "gates"
    assert MedIOImage.DimType.BEDS == "beds"
    assert MedIOImage.DimType.DATA == "data"
    assert MedIOImage.DataType.FLOAT == "float"
    assert MedIOImage.DataType.SHORT == "short"
    assert MedIOImage.DataType.LONG == "int"
    assert MedIOImage.DataType.BYTE == "byte"


#-----------------------------------------------------------------------------------------------
# constructors

def test_construct_from_dimensions():
    img = MedIOImage(*SHAPE)
    assert img.shape == SHAPE
    assert type(img.shape) is tuple
    assert (img.xdim, img.ydim, img.zdim, img.tdim) == SHAPE
    assert img.ndim == 4
    assert img.size == 120
    assert img.nbytes == 480
    assert img.format == "ecat"
    assert img.dimType == "frames"
    assert img.filename == ""
    assert not np.asarray(img).any()

    assert MedIOImage(xdim=2, ydim=3, zdim=4, tdim=5, format="ecat").shape == SHAPE
    assert MedIOImage(2, 3, 4, 5, pmedio.Format.ECAT).format == pmedio.Format.ECAT

    # all four dimensions are required
    with pytest.raises(TypeError):
        MedIOImage(2, 3, 4)
    with pytest.raises(TypeError):
        MedIOImage(2.5, 3, 4, 5)
    # only ECAT images can be created
    with pytest.raises(RuntimeError, match="only ECAT supported"):
        MedIOImage(2, 3, 4, 5, "nifti")


@pytest.mark.parametrize("dtype", [np.float32, np.float64, np.int16, np.uint8, np.int64])
@pytest.mark.parametrize("order", ["F", "C"])
def test_construct_from_array(dtype, order):
    # arrays of all data types and memory layouts are converted to float32
    a = np.asarray(ramp(dtype=dtype), order=order)
    img = MedIOImage(a)
    assert img.shape == SHAPE
    assert img.format == "ecat"
    assert img.dimType == "frames"
    values = np.asarray(img)
    assert values.dtype == np.float32
    assert np.array_equal(values, ramp())

    # the image holds a copy of the data
    a[...] = 0
    assert np.array_equal(np.asarray(img), ramp())


def test_construct_from_array_shapes():
    a = ramp()
    # a 3D array is a single volume
    img = MedIOImage(a[:, :, :, 0])
    assert img.shape == (2, 3, 4, 1)
    assert np.array_equal(np.asarray(img)[..., 0], a[..., 0])
    # not contiguous in memory
    img = MedIOImage(a[::-1, :, ::2, 1:4])
    assert img.shape == (2, 3, 2, 3)
    assert np.array_equal(np.asarray(img), a[::-1, :, ::2, 1:4])
    # nested lists are converted like numpy does
    img = MedIOImage([[[1.0, 2.0], [3.0, 4.0]]])
    assert img.shape == (1, 2, 2, 1)
    assert np.asarray(img)[0, 1, 0, 0] == 3.0
    # objects with the buffer protocol
    assert np.array_equal(np.asarray(MedIOImage(memoryview(a))), a)

    with pytest.raises(RuntimeError, match="only 3 and 4 dimensional data"):
        MedIOImage(a[:, :, 0, 0])
    with pytest.raises(RuntimeError, match="only 3 and 4 dimensional data"):
        MedIOImage(a[:, 0, 0, 0])
    with pytest.raises(ValueError):
        MedIOImage(b"abcd")
    # scalars and arrays with more than 4 dimensions
    for data in (np.float32(1.0), np.zeros(()), 1.5, np.zeros((2, 2, 2, 2, 2), dtype=np.float32)):
        with pytest.raises(RuntimeError, match="only 3 and 4 dimensional data"):
            MedIOImage(data)
    with pytest.raises((RuntimeError, TypeError)):
        MedIOImage(None)


def test_construct_arguments():
    a = ramp()
    # format, dimension type (and data type) are given as strings, only the first
    # letter counts and upper/lower case does not matter
    for name in ("ecat", "ECAT", "e", "Ecat7"):
        assert MedIOImage(a, name).format == "ecat"
    assert MedIOImage(a, format="ecat", dimType="frames").dimType == "frames"
    for name, expected in (("frames", "frames"), ("F", "frames"), ("gates", "gates"), ("g", "gates"),
                           ("beds", "beds"), ("data", "data"), ("unknown", "unknown")):
        assert MedIOImage(a, "ecat", name).dimType == expected
    assert MedIOImage(a, pmedio.Format.ECAT, MedIOImage.DimType.GATES).dimType == MedIOImage.DimType.GATES

    with pytest.raises(RuntimeError, match="could not convert 'xyz' to valid format type"):
        MedIOImage(a, "xyz")
    with pytest.raises(RuntimeError, match="could not convert 'x' to valid dimension type"):
        MedIOImage(a, dimType="x")
    with pytest.raises(RuntimeError, match="only ECAT supported"):
        MedIOImage(a, "nifti")
    # numbers are no formats or dimension types
    with pytest.raises(TypeError):
        MedIOImage(a, 2)
    with pytest.raises(TypeError):
        MedIOImage(a, "ecat", 102)


def test_construct_copy():
    img = MedIOImage(ramp(), "ecat", "gates")
    img.filename = "image.v"
    copy = MedIOImage(img)
    assert copy.shape == SHAPE
    assert copy.dimType == "gates"
    assert copy.filename == "image.v"
    assert np.array_equal(np.asarray(copy), ramp())
    # the voxels are copied, the headers are shared
    copy[0, 0, 0, 0] = 42.0
    assert img[0, 0, 0, 0] == 0.0
    assert copy.mhead is img.mhead
    assert copy.shead is img.shead


#-----------------------------------------------------------------------------------------------
# headers and attributes

def test_default_headers():
    img = MedIOImage(ramp())
    assert type(img.mhead) is dict
    assert type(img.shead) is list
    assert len(img.shead) == 5

    m = img.mhead
    assert m["magic_number"] == "MATRIX72V"
    assert m["isotope_name"] == "F-18"
    assert m["isotope_halflife"] == pytest.approx(6586.2)
    assert m["ecat_calibration_factor"] == 1.0
    assert (m["num_planes"], m["num_frames"], m["num_gates"], m["num_bed_pos"]) == (4, 5, 0, 0)
    assert type(m["sw_version"]) is int
    assert type(m["patient_weight"]) is float
    assert type(m["patient_name"]) is str
    for key in ("scan_start_time", "patient_birth_date", "dose_start_time"):
        assert type(m[key]) is datetime.datetime
        assert m[key].tzinfo is None

    for s in img.shead:
        assert type(s) is dict
        assert (s["x_dimension"], s["y_dimension"], s["z_dimension"]) == (2, 3, 4)
        assert (s["x_pixelsize"], s["y_pixelsize"], s["z_pixelsize"]) == (1.0, 1.0, 1.0)
        assert type(s["frame_duration"]) is int

    m = MedIOImage(ramp(), dimType="gates").mhead
    assert (m["num_planes"], m["num_frames"], m["num_gates"], m["num_bed_pos"]) == (4, 0, 5, 0)
    m = MedIOImage(ramp(), dimType="beds").mhead
    assert (m["num_planes"], m["num_frames"], m["num_gates"], m["num_bed_pos"]) == (4, 0, 0, 5)


def test_attributes():
    img = MedIOImage(ramp())
    # the same objects are returned every time, changes of the headers are kept
    assert img.mhead is img.mhead
    assert img.shead is img.shead
    img.mhead["patient_name"] = "Test"
    assert img.mhead["patient_name"] == "Test"

    mhead = {"patient_name": "Other"}
    img.mhead = mhead
    assert img.mhead is mhead
    shead = [{}]
    img.shead = shead
    assert img.shead is shead
    img.filename = "image.v"
    assert img.filename == "image.v"
    img.filename = "Bär/图像.v"
    assert img.filename == "Bär/图像.v"

    with pytest.raises(TypeError):
        img.mhead = 5
    with pytest.raises(TypeError):
        img.shead = {}
    with pytest.raises(TypeError):
        img.filename = 5
    for name in ("ndim", "xdim", "ydim", "zdim", "tdim", "dimType", "format", "shape", "size", "nbytes"):
        with pytest.raises(AttributeError):
            setattr(img, name, getattr(img, name))


#-----------------------------------------------------------------------------------------------
# access to the voxels

def test_items():
    a = ramp()
    img = MedIOImage(a)
    # the dimensions differ, so that a wrong index is noticed
    for index in np.ndindex(*SHAPE):
        assert img[index] == a[index], index
    assert type(img[1, 2, 3, 4]) is float

    img[1, 2, 3, 4] = -7.5
    assert img[1, 2, 3, 4] == -7.5
    assert np.asarray(img)[1, 2, 3, 4] == -7.5
    a[1, 2, 3, 4] = -7.5
    assert np.array_equal(np.asarray(img), a)
    img[0, 0, 0, 1] = np.float32(3.0)
    img[0, 0, 0, 2] = 4
    assert (img[0, 0, 0, 1], img[0, 0, 0, 2]) == (3.0, 4.0)

    for index in ((2, 0, 0, 0), (0, 3, 0, 0), (0, 0, 4, 0), (0, 0, 0, 5),
                  (-1, 0, 0, 0), (0, -1, 0, 0), (0, 0, -1, 0), (0, 0, 0, -1)):
        with pytest.raises(IndexError):
            img[index]
        with pytest.raises(IndexError):
            img[index] = 1.0
    assert np.array_equal(np.asarray(img)[..., 3:], a[..., 3:])

    with pytest.raises(TypeError):
        img[0, 0, 0]
    with pytest.raises(TypeError):
        img[0]
    with pytest.raises(TypeError):
        img[0, 0, 0, 0] = "a"


def test_buffer():
    a = ramp()
    img = MedIOImage(a)

    # numpy uses the memory of the image
    view = np.asarray(img)
    assert view.dtype == np.float32
    assert view.shape == SHAPE
    assert view.flags.f_contiguous
    assert view.flags.writeable
    assert not view.flags.owndata
    assert np.array_equal(view, a)
    view[1, 2, 3, 4] = -1.0
    assert img[1, 2, 3, 4] == -1.0
    img[0, 1, 2, 3] = -2.0
    assert view[0, 1, 2, 3] == -2.0

    # a copy does not
    copy = np.array(img)
    copy[0, 0, 0, 0] = 99.0
    assert img[0, 0, 0, 0] == 0.0
    assert np.asarray(img, dtype=np.float64).dtype == np.float64

    # buffer protocol
    m = memoryview(img)
    assert m.format == "f"
    assert m.itemsize == 4
    assert m.ndim == 4
    assert m.shape == SHAPE
    assert m.strides == (4, 8, 24, 96)
    assert m.nbytes == 480
    assert not m.readonly
    assert m.f_contiguous

    # the image lives as long as it is used by an array
    view = np.asarray(MedIOImage(a))
    assert np.array_equal(view, a)

    # numpy functions take the image
    assert np.sum(MedIOImage(a), dtype=np.float64) == a.sum(dtype=np.float64)


def test_toarray():
    a = ramp()
    img = MedIOImage(a)
    for args in ((), (False,), (True,)):
        values = img.toarray(*args)
        assert type(values) is np.ndarray
        assert values.dtype == np.float32
        assert values.shape == SHAPE
        assert values.flags.f_contiguous
        assert values.flags.writeable
        assert np.array_equal(values, a)
    assert np.array_equal(img.toarray(copy=False), a)

    # a copy is independent of the image and outlives it
    values = img.toarray(copy=True)
    values[0, 0, 0, 0] = 99.0
    assert img[0, 0, 0, 0] == 0.0
    del img
    assert values[1, 2, 3, 4] == a[1, 2, 3, 4]

    with pytest.raises(TypeError):
        MedIOImage(a).toarray("yes")


#-----------------------------------------------------------------------------------------------
# reading and writing

def test_write_read(tmp_path):
    a = ramp()
    img = MedIOImage(a)
    name = str(tmp_path / "image.v")

    written = pmedio.write(img, name)
    assert type(written) is MedIOImage
    assert written.filename == name
    assert written.shape == SHAPE
    assert os.path.getsize(name) > a.nbytes

    read = pmedio.read(name)
    assert type(read) is MedIOImage
    assert read.filename == name
    assert read.shape == SHAPE
    assert read.ndim == 4
    assert read.format == "ecat"
    assert read.dimType == "frames"
    assert np.array_equal(np.asarray(read), a)
    assert len(read.shead) == 5
    assert read.mhead["num_frames"] == 5
    assert read.mhead["num_planes"] == 4
    assert read.shead[0]["x_dimension"] == 2

    # the file names are strings
    with pytest.raises(TypeError):
        pmedio.read(5)
    with pytest.raises(TypeError):
        pmedio.read(name.encode())
    with pytest.raises(TypeError):
        pmedio.write(img, 5)
    with pytest.raises((RuntimeError, TypeError)):
        pmedio.write(None, name)
    with pytest.raises(RuntimeError, match="invalid/unsupported file"):
        pmedio.read(str(tmp_path / "missing.v"))
    with pytest.raises(TypeError):
        pmedio.read(name, "1")
    with pytest.raises(TypeError):
        pmedio.read(name, [1.5])


def test_write_overwrite(tmp_path):
    name = str(tmp_path / "image.v")
    pmedio.write(MedIOImage(ramp()), name)
    with pytest.raises(RuntimeError, match="already exists"):
        pmedio.write(MedIOImage(ramp()), name)
    with pytest.raises(RuntimeError, match="already exists"):
        pmedio.write(ramp(), name, overwrite=False)
    assert np.array_equal(np.asarray(pmedio.read(name)), ramp())

    pmedio.write(MedIOImage(ramp() + 1), name, True)
    assert np.array_equal(np.asarray(pmedio.read(name)), ramp() + 1)
    pmedio.write(ramp() + 2, name, overwrite=True)
    assert np.array_equal(np.asarray(pmedio.read(name)), ramp() + 2)


@pytest.mark.parametrize("data_type, code", [("float", 5), ("short", 6), ("int", 7), ("byte", 1),
                                             ("FLOAT", 5), ("s", 6)])
def test_write_data_types(tmp_path, data_type, code):
    # the values of the ramp (0 ... 119) can be stored exactly with all data types
    a = ramp()
    name = str(tmp_path / "image.v")
    pmedio.write(MedIOImage(a), fileName=name, overwrite=False, dimType="frames", dataType=data_type,
                 format="ecat")
    read = pmedio.read(fileName=name, volumes=[], rows=[], cols=[], planes=[])
    assert np.array_equal(np.asarray(read), a)
    assert read.shead[0]["data_type"] == code


@pytest.mark.parametrize("dtype", [np.float32, np.float64, np.int16])
@pytest.mark.parametrize("order", ["F", "C"])
def test_write_array(tmp_path, dtype, order):
    # pmedio.write() takes arrays of all data types and memory layouts
    a = np.asarray(ramp(dtype=dtype), order=order)
    name = str(tmp_path / "image.v")
    written = pmedio.write(a, name, False, "frames", "float", "ecat")
    assert type(written) is MedIOImage
    assert written.filename == name
    read = pmedio.read(name)
    assert read.shape == SHAPE
    assert np.array_equal(np.asarray(read), ramp())

    # a 3D array is a single frame
    pmedio.write(a[..., 2], name, overwrite=True)
    read = pmedio.read(name)
    assert read.shape == (2, 3, 4, 1)
    assert np.array_equal(np.asarray(read)[..., 0], ramp()[..., 2])


def test_write_gates(tmp_path):
    a = ramp()
    name = str(tmp_path / "image.v")
    pmedio.write(a, name, dimType="gates")
    read = pmedio.read(name)
    assert read.dimType == "gates"
    assert read.mhead["num_gates"] == 5
    assert np.array_equal(np.asarray(read), a)


def test_write_errors(tmp_path):
    a = ramp()
    name = tmp_path / "image.v"
    with pytest.raises(RuntimeError, match="could not convert 'q' to valid data type"):
        pmedio.write(MedIOImage(a), str(name), dataType="q")
    with pytest.raises(RuntimeError, match="could not convert 'x' to valid dimension type"):
        pmedio.write(a, str(name), dimType="x")
    with pytest.raises(RuntimeError, match="could not convert 'xyz' to valid format type"):
        pmedio.write(a, str(name), format="xyz")
    with pytest.raises(TypeError):
        pmedio.write(a, str(name), dataType=5)
    with pytest.raises(TypeError):
        pmedio.write(a, str(name), overwrite="yes")
    # nothing is left behind
    assert list(tmp_path.iterdir()) == []


def test_image_methods(tmp_path):
    a = ramp()
    first = str(tmp_path / "first.v")
    second = str(tmp_path / "second.v")
    pmedio.write(a, first)
    img = pmedio.read(first)

    # write(filename) writes a new file ...
    written = img.write(second)
    assert type(written) is MedIOImage
    assert written.filename == second
    assert np.array_equal(np.asarray(pmedio.read(second)), a)
    with pytest.raises(RuntimeError, match="already exists"):
        img.write(second)
    img.write(filename=second, overwrite=True)

    # ... write() without name overwrites the file of the image
    img = pmedio.read(first)
    np.asarray(img)[...] += 1
    assert img.write().filename == first
    assert np.array_equal(np.asarray(pmedio.read(first)), a + 1)

    # read() reads the file again
    np.asarray(img)[...] = 0
    assert np.array_equal(np.asarray(img.read()), a + 1)


def test_header_round_trip(tmp_path):
    name = str(tmp_path / "image.v")
    img = MedIOImage(ramp())
    img.mhead["patient_name"] = "Test^Name"
    img.mhead["patient_id"] = "4711"
    img.mhead["patient_weight"] = 70.5
    img.mhead["radiopharmaceutical"] = "Ga-68 DOTATOC"
    img.mhead["isotope_halflife"] = 4062.6
    img.mhead["study_description"] = "Größe"
    for k, s in enumerate(img.shead):
        s["frame_start"] = 1000 * k
        s["frame_duration"] = 1000
        s["x_pixelsize"] = 0.25
    img.write(name)

    read = pmedio.read(name)
    assert read.mhead["patient_name"] == "Test^Name"
    assert read.mhead["patient_id"] == "4711"
    assert read.mhead["patient_weight"] == 70.5
    assert read.mhead["radiopharmaceutical"] == "Ga-68 DOTATOC"
    assert read.mhead["isotope_halflife"] == pytest.approx(4062.6)
    assert [s["frame_start"] for s in read.shead] == [0, 1000, 2000, 3000, 4000]
    assert [s["frame_duration"] for s in read.shead] == [1000] * 5
    assert read.shead[2]["x_pixelsize"] == 0.25

    # values of a wrong type are rejected
    img.mhead["patient_weight"] = "heavy"
    with pytest.raises((RuntimeError, TypeError)):
        img.write(str(tmp_path / "invalid.v"))


def test_header_times(tmp_path, timezone):
    # the times of the headers are local times without time zone (like datetime.fromtimestamp())
    scan = datetime.datetime(2020, 5, 17, 12, 34, 56)
    dose = datetime.datetime(2020, 1, 17, 23, 0, 0)
    name = str(tmp_path / f"image-{timezone.replace('/', '-')}.v")
    img = MedIOImage(ramp())
    assert img.mhead["scan_start_time"] == datetime.datetime.fromtimestamp(0)
    img.mhead["scan_start_time"] = scan
    img.mhead["dose_start_time"] = dose
    img.write(name)

    read = pmedio.read(name)
    assert read.mhead["scan_start_time"] == scan
    assert read.mhead["dose_start_time"] == dose
    assert read.mhead["scan_start_time"].tzinfo is None

    # the file stores the corresponding UTC time
    with pmedio.open(name) as f:
        attributes = f.attributes()
    utc = datetime.timezone.utc
    assert attributes["AcquisitionDateTime"] == scan.astimezone(utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    assert attributes["RadiopharmaceuticalStartDateTime"] == dose.astimezone(utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def expected_values(path):
    with open(DATA_DIR / (path + ".json")) as f:
        return json.load(f)


@needs_data
@pytest.mark.parametrize("path", ["ecat/pet-static.v", "ecat/pet-dynamic.v"])
def test_read_dataset_headers(path, timezone):
    expected = expected_values(path)
    matrices = expected["matrices"]

    img = pmedio.read(str(DATA_DIR / path))
    assert img.format == "ecat"
    assert img.dimType == "frames"
    assert img.shape == (*matrices[0]["dims"], len(matrices))
    assert img.mhead["num_planes"] == expected["main_header"]["num_planes"]
    assert img.mhead["patient_weight"] == pytest.approx(expected["attributes"]["main"]["PatientWeight"])
    utc = datetime.timezone.utc
    assert (img.mhead["scan_start_time"].astimezone(utc).strftime("%Y-%m-%dT%H:%M:%SZ")
            == expected["attributes"]["main"]["AcquisitionDateTime"])
    assert len(img.shead) == len(matrices)
    for s, m in zip(img.shead, matrices):
        assert s["frame_start"] == m["frame_start_time_ms"]
        assert s["frame_duration"] == m["frame_duration_ms"]
        assert s["data_type"] == m["data_type"]


@needs_data
@pytest.mark.parametrize("path", [
    "ecat/pet-static.v",
    # known problem: MedIOImage stores the frames in reverse order (the last frame of the
    # file is img[..., 0]), while img.shead is in the order of the file
    pytest.param("ecat/pet-dynamic.v", marks=pytest.mark.xfail(strict=True, reason="frames in reverse order")),
])
def test_read_dataset_values(path):
    expected = expected_values(path)
    calibration = expected["main_header"]["calibration_factor"]

    values = np.asarray(pmedio.read(str(DATA_DIR / path)))
    for k, m in enumerate(expected["matrices"]):
        scale = m["scale_factor"] * calibration
        frame = values[..., k]
        assert frame.sum(dtype=np.float64) == pytest.approx(m["raw_sum"] * scale, rel=1e-5)
        assert frame.min() == pytest.approx(m["raw_min"] * scale, rel=1e-5)
        assert frame.max() == pytest.approx(m["raw_max"] * scale, rel=1e-5)


@needs_data
@pytest.mark.parametrize("path", ["philips/pet-dynamic.img", "nifti/pet-dynamic.nii.gz"])
def test_read_unsupported(path):
    # MedIOImage only reads ECAT files, the other formats are read with pmedio.open()
    with pytest.raises(RuntimeError, match="unsupported data format"):
        pmedio.read(str(DATA_DIR / path))
