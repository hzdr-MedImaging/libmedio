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

# Tests of pmedio.load(), pmedio.save() and pmedio.Image: all frames of a file
# of any format as one 4D array.
#
# The files of the test datasets are checked against the expected values of
# testsuite/data (nibabel, or documented manual values, see test_volume.py),
# NIfTI files also against nibabel directly. Synthetic images are their own
# expected values.

import io
import pathlib

import numpy as np
import pytest

import pmedio
from test_volume import DATA_DIR, READABLE, Expected, load_json, needs_data

# RAS voxel order (the nibabel convention), which is the order of none of the formats
AFFINE = np.array([[-2.0, 0.0, 0.0, 10.0], [0.0, -2.5, 0.0, -20.0], [0.0, 0.0, 3.0, -30.0]])
SHAPE = (5, 4, 3)
FRAME_START = [0.0, 60.0, 120.0]
FRAME_DURATION = [30.0, 31.0, 32.5]
FORMATS = {".v": pmedio.Format.ECAT, ".nii": pmedio.Format.NIFTI, ".nii.gz": pmedio.Format.NIFTI,
           ".hdr": pmedio.Format.NIFTI, ".img": pmedio.Format.Philips, ".syn": pmedio.Format.Philips}
ATTRIBUTES = {"PatientName": "Test^Name", "PatientID": "4711", "PatientWeight": 72.5, "Radiopharmaceutical": "FDG"}


def synthetic(dtype=np.int16, frames=3):
    """image of distinct values in every voxel and frame, in RAS voxel order"""
    data = np.arange(int(np.prod(SHAPE)) * frames).reshape((*SHAPE, frames), order="F") + 1
    return pmedio.Image(data.astype(dtype), affine=AFFINE, frame_start=FRAME_START[:frames],
                        frame_duration=FRAME_DURATION[:frames], attributes=ATTRIBUTES)


def expected_affine(suffix):
    affine = AFFINE.copy()
    if suffix in (".img", ".syn"):
        # Philips files store no position within the planes: they are centered
        affine[0, 3] = -(SHAPE[0] - 1) / 2 * AFFINE[0, 0]
        affine[1, 3] = -(SHAPE[1] - 1) / 2 * AFFINE[1, 1]
    return affine


def files_in(directory):
    return sorted(p.name for p in pathlib.Path(directory).iterdir())


#=============================================================================================
# load()

@needs_data
@pytest.mark.parametrize(("path", "key", "json_name"), READABLE, ids=[r[1] for r in READABLE])
def test_load_datasets(path, key, json_name):
    expected = Expected(key, json_name)
    image = pmedio.load(DATA_DIR / path)

    frames = len(expected.stats)
    assert isinstance(image, pmedio.Image)
    assert image.shape == (*expected.dims, frames)
    assert len(image) == frames
    assert image.data.dtype == np.float32
    # absolute, the same file (resolve() would also resolve symbolic links, which the name keeps)
    assert pathlib.Path(image.filename).is_absolute()
    assert pathlib.Path(image.filename).samefile(DATA_DIR / path)
    file_format = load_json(json_name)["format"]
    assert image.format == (pmedio.Format.ECAT if file_format == "ecat7" else
                            pmedio.Format.NIFTI if file_format.startswith("nifti") else pmedio.Format.Philips)

    # the physical values of every frame: stored values x scale + intercept
    for k in range(frames):
        raw_sum = expected.stats[k][2]
        size = int(np.prod(expected.dims))
        assert float(image.data[..., k].sum(dtype=np.float64)) == pytest.approx(
            raw_sum * expected.scales[k] + expected.intercept * size, rel=1e-5), k
    if expected.timing is not None:
        assert image.frame_start == pytest.approx([t[0] for t in expected.timing], abs=1e-3)
        assert image.frame_duration == pytest.approx([t[1] for t in expected.timing], abs=1e-3)

    # the same as the volumes of pmedio.open()
    with pmedio.open(DATA_DIR / path) as f:
        assert image.attributes == f.attributes()
        for k, n in enumerate(f.frame_numbers()):
            v = f.read_volume(n)
            assert np.array_equal(image.data[..., k], v.values()), k
            assert np.array_equal(image.affine, v.affine)
            assert (image.frame_start[k], image.frame_duration[k]) == (v.frame_start, v.frame_duration)
            assert v.orientation == expected.orientation


@needs_data
@pytest.mark.parametrize("path", ["nifti/pet-dynamic.nii.gz", "nifti/pet-dynamic.pair/pet-dynamic.hdr",
                                  "nifti/pet-static.nii.gz", "nifti/pet-static.pair/pet-static.hdr"])
def test_load_nifti_nibabel(path):
    nib = pytest.importorskip("nibabel")
    reference = nib.load(str(DATA_DIR / path))
    # the voxels in the order of the file
    image = pmedio.load(DATA_DIR / path)
    data = reference.get_fdata()
    if data.ndim == 3:
        data = data[..., np.newaxis]
    assert image.shape == data.shape
    assert np.allclose(image.data, data, rtol=1e-6, atol=1e-6 * np.abs(data).max())
    if reference.header["sform_code"] > 0 or reference.header["qform_code"] > 0:
        # nibabel: RAS affine -> LPS (files without qform and sform have none:
        # nibabel and libmedio assume different voxel orders for them)
        lps = np.diag([-1.0, -1.0, 1.0, 1.0]) @ reference.affine
        assert np.allclose(image.affine, lps[:3], atol=1e-3)


@needs_data
def test_load_frames_and_orientation():
    path = DATA_DIR / "ecat/pet-dynamic.v"
    image = pmedio.load(path)
    assert image.shape[3] == 3

    selected = pmedio.load(path, frames=[3, 1])
    assert selected.shape[3] == 2
    assert np.array_equal(selected.data[..., 0], image.data[..., 2])
    assert np.array_equal(selected.data[..., 1], image.data[..., 0])
    assert selected.frame_start == [image.frame_start[2], image.frame_start[0]]
    assert selected.frame_duration == [image.frame_duration[2], image.frame_duration[0]]
    assert len(pmedio.load(path, frames=(np.int64(2),))) == 1

    # the voxels reordered like Volume.reorient(): the same positions
    ras = pmedio.load(path, orientation="RAS")
    with pmedio.open(path) as f:
        v = f.read_volume(2)
    v.reorient("RAS")
    assert np.array_equal(ras.data[..., 1], v.values())
    assert np.array_equal(ras.affine, v.affine)
    # ECAT7: LPI voxel order, i.e. all three axes reversed
    assert np.array_equal(ras.data, image.data[::-1, ::-1, ::-1, :])


@needs_data
def test_load_errors(tmp_path):
    path = DATA_DIR / "ecat/pet-dynamic.v"
    with pytest.raises(ValueError, match="frame 4 does not exist"):
        pmedio.load(path, frames=[1, 4])
    with pytest.raises(ValueError, match="frame 0 does not exist"):
        pmedio.load(path, frames=[0])
    with pytest.raises(ValueError, match="frame 65537 does not exist"):
        pmedio.load(path, frames=[65537])
    with pytest.raises(TypeError, match="frame numbers"):
        pmedio.load(path, frames=[1.0])
    with pytest.raises(TypeError, match="frame numbers"):
        pmedio.load(path, frames=[True])
    with pytest.raises(ValueError, match="no frames"):
        pmedio.load(path, frames=[])
    with pytest.raises(ValueError, match="invalid orientation"):
        pmedio.load(path, orientation="XYZ")
    with pytest.raises(OSError):
        pmedio.load(tmp_path / "missing.v")


def test_load_frames_of_other_geometry(tmp_path):
    # frames of different dimensions cannot be one 4D array
    path = tmp_path / "geometry.v"
    with pmedio.create(path, frames=2) as f:
        f.write_volume(pmedio.Volume(np.ones(SHAPE, dtype=np.int16), affine=AFFINE), 1)
        f.write_volume(pmedio.Volume(np.ones((6, 4, 3), dtype=np.int16), affine=AFFINE), 2)
    with pytest.raises(ValueError, match="another geometry.*pmedio.open"):
        pmedio.load(path)
    # one frame at a time
    assert pmedio.load(path, frames=[2]).shape == (6, 4, 3, 1)


#=============================================================================================
# save()

@pytest.mark.parametrize("suffix", list(FORMATS))
@pytest.mark.parametrize("frames", [1, 3])
def test_save_formats(tmp_path, suffix, frames):
    image = synthetic(frames=frames)
    path = tmp_path / ("image" + suffix)
    pmedio.save(path, image)

    back = pmedio.load(path, orientation="RAS")
    assert back.format == FORMATS[suffix]
    assert back.shape == (*SHAPE, frames)
    assert np.array_equal(back.data, image.data)
    assert np.allclose(back.affine, expected_affine(suffix), atol=1e-3)
    assert back.frame_start == pytest.approx(FRAME_START[:frames], abs=1e-3)
    assert back.frame_duration == pytest.approx(FRAME_DURATION[:frames], abs=1e-3)
    for key, value in ATTRIBUTES.items():
        assert back.attributes[key] == value, key

    # the stored data type: integers as integers (NIfTI files of several frames: float32)
    with pmedio.open(path) as f:
        dtype = f.read_volume(1).dtype
    if suffix in (".nii", ".nii.gz", ".hdr") and frames > 1:
        assert dtype == np.float32
    else:
        assert dtype == np.int16


@pytest.mark.parametrize("suffix", [".nii", ".nii.gz", ".hdr"])
def test_save_nifti_nibabel(tmp_path, suffix):
    nib = pytest.importorskip("nibabel")
    image = synthetic(dtype=np.float32)
    path = tmp_path / ("image" + suffix)
    image.save(path)
    reference = nib.as_closest_canonical(nib.load(str(path)))
    assert reference.shape == (*SHAPE, 3)
    assert np.array_equal(reference.get_fdata(), image.data)
    lps_to_ras = np.diag([-1.0, -1.0, 1.0, 1.0])
    assert np.allclose(reference.affine, lps_to_ras @ np.vstack([AFFINE, [0, 0, 0, 1]]), atol=1e-4)


def test_save_ecat_nibabel(tmp_path):
    nib = pytest.importorskip("nibabel")
    image = synthetic()
    path = tmp_path / "image.v"
    image.save(path)
    holder = nib.FileHolder(fileobj=io.BytesIO(path.read_bytes()))
    reference = nib.ecat.EcatImage.from_file_map({"header": holder, "image": holder})
    assert reference.shape == (*SHAPE, 3)
    subheaders = reference.get_subheaders()
    for k in range(3):
        assert int(subheaders.subheaders[k]["frame_start_time"]) == FRAME_START[k] * 1000
        assert int(subheaders.subheaders[k]["frame_duration"]) == FRAME_DURATION[k] * 1000
    # nibabel orders the voxels by patient_orientation; the sum is independent of it
    assert reference.get_fdata().sum() == pytest.approx(float(image.data.sum()), rel=1e-6)
    assert reference.header["patient_name"].item().decode().strip("\x00") == "Test^Name"


@needs_data
@pytest.mark.parametrize("source", ["ecat/pet-dynamic.v", "nifti/pet-dynamic.nii.gz", "philips/pet-dynamic.img"])
@pytest.mark.parametrize("suffix", list(FORMATS))
def test_save_datasets(tmp_path, source, suffix):
    # every dataset in every format: the same physical values, positions and timing
    image = pmedio.load(DATA_DIR / source, orientation="RAS")
    path = tmp_path / ("image" + suffix)
    image.save(path)
    back = pmedio.load(path, orientation="RAS")
    assert back.shape == image.shape
    assert np.array_equal(back.data, image.data)
    if suffix in (".img", ".syn"):
        # Philips files store no position within the planes
        assert np.allclose(back.affine[:, :3], image.affine[:, :3], atol=1e-3)
    else:
        assert np.allclose(back.affine, image.affine, atol=1e-3)
    assert back.frame_start == image.frame_start
    assert back.frame_duration == image.frame_duration
    assert back.attributes["PatientName"] == image.attributes["PatientName"]


@needs_data
def test_save_like(tmp_path):
    source = DATA_DIR / "ecat/pet-dynamic.v"
    image = pmedio.load(source)
    image.attributes = {"PatientWeight": 99.0}
    with pmedio.open(source) as like:
        original = like.attributes()
        pmedio.save(tmp_path / "like.nii", image, like=like)
        with pytest.raises(ValueError):
            # the source has 3 frames
            pmedio.save(tmp_path / "frames.nii", pmedio.Image(image.data[..., :1]), like=like)
    assert not (tmp_path / "frames.nii").exists()
    attributes = pmedio.load(tmp_path / "like.nii").attributes
    assert attributes["PatientWeight"] == 99.0
    assert attributes["PatientName"] == original["PatientName"]


def test_save_array(tmp_path):
    # an array: identity affine, no timing
    data = np.arange(60, dtype=np.int16).reshape(SHAPE, order="F")
    pmedio.save(tmp_path / "array.nii", data)
    back = pmedio.load(tmp_path / "array.nii")
    assert back.shape == (*SHAPE, 1)
    assert np.array_equal(back.data[..., 0], data)
    assert np.allclose(back.affine, np.eye(3, 4), atol=1e-6)
    # array-like data (a list of float becomes float64)
    pmedio.save(tmp_path / "list.v", (data * 0.5).tolist(), format=pmedio.Format.ECAT)
    # ECAT7 stores the planes from superior to inferior: the identity affine is the LPS voxel order
    back = pmedio.load(tmp_path / "list.v", orientation="LPS")
    assert np.array_equal(back.data[..., 0], data * 0.5)
    assert np.allclose(back.affine, np.eye(3, 4), atol=1e-6)


def test_save_overwrite(tmp_path):
    path = tmp_path / "image.v"
    synthetic(frames=1).save(path)
    with pytest.raises(FileExistsError):
        synthetic().save(path)
    assert len(pmedio.load(path)) == 1
    synthetic().save(path, overwrite=True)
    assert len(pmedio.load(path)) == 3


@pytest.mark.parametrize(("header", "image"), [("pair.hdr", "pair.img"), ("PAIR.HDR", "PAIR.IMG"),
                                               ("pair.hdr.gz", "pair.img.gz"),
                                               # an existing image file in the other letter case is
                                               # the image file of the pair (see CNIFTIFile)
                                               ("mixed.hdr", "mixed.IMG")])
def test_save_nifti_pair_overwrite(tmp_path, header, image):
    # the image file of a NIfTI header/image pair, e.g. a Philips file of the same name,
    # is not replaced without overwrite=True
    (tmp_path / image).write_bytes(b"other data")
    with pytest.raises(FileExistsError, match="the image file of"):
        synthetic().save(tmp_path / header)
    assert (tmp_path / image).read_bytes() == b"other data"
    assert not (tmp_path / header).exists()
    synthetic().save(tmp_path / header, overwrite=True)
    assert len(pmedio.load(tmp_path / header)) == 3


def test_save_nifti_pair_keeps_philips_image(tmp_path):
    philips = tmp_path / "image.img"
    synthetic().save(philips)
    with pytest.raises(FileExistsError):
        synthetic(frames=1).save(tmp_path / "image.hdr")
    assert pmedio.load(philips).format == pmedio.Format.Philips
    # a .hdr file of another format has no image file: the Philips file stays
    synthetic(frames=1).save(tmp_path / "image.hdr", format=pmedio.Format.ECAT)
    assert pmedio.load(philips).format == pmedio.Format.Philips
    assert pmedio.load(tmp_path / "image.hdr").format == pmedio.Format.ECAT


def test_save_errors(tmp_path):
    image = synthetic()
    with pytest.raises(ValueError):
        pmedio.save(tmp_path / "image.v", image, format="unknown")
    image.attributes = {"PatientWeight": "heavy"}
    with pytest.raises(ValueError):
        image.save(tmp_path / "attributes.nii")
    with pytest.raises(TypeError):
        pmedio.save(tmp_path / "image.v", np.zeros(SHAPE, dtype=np.complex64))
    with pytest.raises(TypeError):
        pmedio.save(tmp_path / "image.v", np.zeros(SHAPE, dtype=bool))
    with pytest.raises(ValueError):
        pmedio.save(tmp_path / "image.v", np.zeros((5, 4), dtype=np.int16))
    # nothing is left behind
    assert files_in(tmp_path) == []


#=============================================================================================
# Image

def test_image():
    image = synthetic()
    assert image.shape == (*SHAPE, 3)
    assert len(image) == 3
    assert image.data.dtype == np.int16
    assert image.affine.dtype == np.float64 and image.affine.shape == (3, 4)
    assert np.array_equal(image.affine, AFFINE)
    assert image.frame_start == FRAME_START
    assert image.frame_duration == FRAME_DURATION
    assert image.attributes == ATTRIBUTES
    assert image.format is None and image.filename is None
    assert repr(image) == "<pmedio.Image 5x4x3x3 int16>"

    # the attributes are a copy
    attributes = dict(ATTRIBUTES)
    image.attributes = attributes
    attributes["PatientID"] = "0815"
    assert image.attributes["PatientID"] == "4711"

    # one frame as volume
    v = image.volume(1)
    assert isinstance(v, pmedio.Volume)
    assert np.array_equal(v.values(), image.data[..., 1])
    assert np.array_equal(v.affine, AFFINE)
    assert (v.frame_start, v.frame_duration) == (FRAME_START[1], FRAME_DURATION[1])
    with pytest.raises(IndexError):
        image.volume(3)

    # a 4 x 4 affine, defaults
    square = np.vstack([AFFINE, [0, 0, 0, 1]])
    assert np.array_equal(pmedio.Image(image.data, affine=square).affine, AFFINE)
    default = pmedio.Image(np.zeros(SHAPE, dtype=np.float32))
    assert default.shape == (*SHAPE, 1)
    assert np.array_equal(default.affine, np.eye(3, 4))
    assert default.frame_start == [0.0] and default.frame_duration == [0.0]
    assert default.attributes == {}


@needs_data
def test_image_repr_of_loaded():
    image = pmedio.load(DATA_DIR / "ecat/pet-static.v")
    assert repr(image) == f"<pmedio.Image {'x'.join(str(n) for n in image.shape)} float32 '{image.filename}'>"


def test_image_setters():
    image = synthetic()
    # data of the same number of frames keeps the timing
    image.data = image.data * 2
    assert image.frame_start == FRAME_START
    # data of another number of frames resets it
    image.data = image.data[..., :2]
    assert image.frame_start == [0.0, 0.0] and image.frame_duration == [0.0, 0.0]
    image.data = np.zeros(SHAPE, dtype=np.uint8)
    assert image.shape == (*SHAPE, 1)

    image.frame_start = [5.0]
    image.frame_duration = np.array([10.0])
    assert (image.frame_start, image.frame_duration) == ([5.0], [10.0])
    with pytest.raises(ValueError, match="one value per frame"):
        image.frame_start = [1.0, 2.0]

    image.affine = np.vstack([AFFINE, [0, 0, 0, 1]])
    assert np.array_equal(image.affine, AFFINE)
    with pytest.raises(ValueError):
        image.affine = np.eye(2)

    image.attributes = None
    assert image.attributes == {}
    with pytest.raises(TypeError):
        image.attributes = [("PatientName", "x")]


def test_image_errors():
    with pytest.raises(ValueError, match="3D"):
        pmedio.Image(np.zeros((5, 4), dtype=np.int16))
    with pytest.raises(ValueError, match="frames"):
        pmedio.Image(np.zeros((*SHAPE, 0), dtype=np.int16))
    with pytest.raises(TypeError, match="unsupported data type"):
        pmedio.Image(np.zeros(SHAPE, dtype=bool))
    with pytest.raises(ValueError, match="one value per frame"):
        pmedio.Image(np.zeros((*SHAPE, 2), dtype=np.int16), frame_start=[0.0])
    with pytest.raises(ValueError):
        pmedio.Image(np.zeros(SHAPE, dtype=np.int16), affine=np.eye(3))
