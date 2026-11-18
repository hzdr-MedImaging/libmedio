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

# Tests of pmedio.create() without a source file: files of every writable
# format made from volumes alone, and the header attributes given to create().
#
# The volumes are synthetic, so their values, positions and timing are the
# expected values. ECAT7 and NIfTI files are also read with nibabel, which is
# independent of libmedio; Philips files can only be read back with libmedio.

import io
import os
import pathlib

import numpy as np
import pytest

import pmedio

# RAS voxel order (the nibabel convention), which is the order of none of the formats
AFFINE = np.array([[-2.0, 0.0, 0.0, 10.0], [0.0, -2.5, 0.0, -20.0], [0.0, 0.0, 3.0, -30.0]])
SHAPE = (5, 4, 3)
TIMING = [(0.0, 30.0), (60.0, 31.0), (120.0, 32.5)]
FORMATS = {".v": pmedio.Format.ECAT, ".nii": pmedio.Format.NIFTI, ".nii.gz": pmedio.Format.NIFTI,
           ".hdr": pmedio.Format.NIFTI, ".img": pmedio.Format.Philips, ".syn": pmedio.Format.Philips}

ATTRIBUTES = {
    "PatientName": "Test^Name",
    "PatientID": "4711",
    "PatientWeight": 72.5,
    "PatientSize": 1.8,
    "Radiopharmaceutical": "FDG",
    "RadionuclideTotalDose": 150000000.0,
    "RadionuclideHalfLife": 6586.2,
    "AcquisitionDateTime": "2026-06-03T17:04:56Z",
}


def stored(k):
    """stored values of frame k: int16, different in every voxel and frame"""
    return (np.arange(int(np.prod(SHAPE))).reshape(SHAPE, order="F") + 1 + 100 * k).astype(np.int16)


def frame(k, scale=0.25):
    start, duration = TIMING[k]
    return pmedio.Volume(stored(k), affine=AFFINE, scale_factor=scale, frame_start=start, frame_duration=duration)


def write(path, n, **arguments):
    with pmedio.create(path, **arguments) as f:
        for k in range(n):
            f.write_volume(frame(k), k + 1)


def read_ras(path):
    """volumes of the file in RAS voxel order"""
    with pmedio.open(path) as f:
        volumes = [f.read_volume(n) for n in f.frame_numbers()]
        assert f.frame_numbers() == list(range(1, len(volumes) + 1))
    for v in volumes:
        v.reorient("RAS")
    return volumes


def nibabel_image(path):
    nib = pytest.importorskip("nibabel")
    if str(path).endswith(".v"):
        # nibabel keeps an ECAT7 file open, so it gets the content of the file
        holder = nib.FileHolder(fileobj=io.BytesIO(pathlib.Path(path).read_bytes()))
        return nib.ecat.EcatImage.from_file_map({"header": holder, "image": holder})
    return nib.load(str(path))


@pytest.mark.parametrize("suffix", list(FORMATS))
@pytest.mark.parametrize("frames", [1, 3])
def test_create_without_source(tmp_path, suffix, frames):
    path = tmp_path / ("new" + suffix)
    write(path, frames, frames=frames)

    with pmedio.open(path) as f:
        assert f.format == FORMATS[suffix]
    volumes = read_ras(path)
    assert len(volumes) == frames
    for k, v in enumerate(volumes):
        assert v.shape == SHAPE
        assert np.array_equal(v.values(), frame(k).values()), k
        assert (v.frame_start, v.frame_duration) == pytest.approx(TIMING[k], abs=1e-3)
        expected = AFFINE.copy()
        if suffix in (".img", ".syn"):
            # Philips files store no position within the planes: they are centered
            expected[0, 3] = -(SHAPE[0] - 1) / 2 * AFFINE[0, 0]
            expected[1, 3] = -(SHAPE[1] - 1) / 2 * AFFINE[1, 1]
        assert np.allclose(v.affine, expected, atol=1e-3), k


@pytest.mark.parametrize("suffix", [".nii", ".nii.gz", ".hdr"])
def test_create_nifti_nibabel(tmp_path, suffix):
    # one frame: the stored values and scaling of the volume
    one = tmp_path / ("one" + suffix)
    write(one, 1)
    nib = pytest.importorskip("nibabel")
    image = nibabel_image(one)
    assert image.shape == SHAPE
    assert image.get_data_dtype() == np.int16
    assert image.dataobj.slope == 0.25
    # in the voxel order of the volume (RAS), as nibabel reorders the file
    canonical = nib.as_closest_canonical(image)
    assert np.array_equal(canonical.get_fdata(), stored(0) * 0.25)
    lps_to_ras = np.diag([-1.0, -1.0, 1.0, 1.0])
    assert np.allclose(canonical.affine, lps_to_ras @ np.vstack([AFFINE, [0, 0, 0, 1]]), atol=1e-4)

    # several frames with scale factors of their own: the physical values as float32
    several = tmp_path / ("several" + suffix)
    with pmedio.create(several, frames=3) as f:
        for k in range(3):
            f.write_volume(frame(k, scale=0.25 * (k + 1)), k + 1)
    image = nibabel_image(several)
    assert image.shape == (*SHAPE, 3)
    assert image.get_data_dtype() == np.float32
    canonical = nib.as_closest_canonical(image)
    for k in range(3):
        assert np.array_equal(canonical.get_fdata()[..., k], stored(k) * 0.25 * (k + 1)), k


def test_create_ecat_nibabel(tmp_path):
    path = tmp_path / "new.v"
    write(path, 3, attributes=ATTRIBUTES)
    image = nibabel_image(path)
    assert image.shape == (*SHAPE, 3)
    header = image.header
    assert header["patient_name"].item().decode().strip("\x00") == "Test^Name"
    assert header["patient_id"].item().decode().strip("\x00") == "4711"
    assert float(header["patient_weight"]) == pytest.approx(72.5)
    assert float(header["isotope_halflife"]) == pytest.approx(6586.2, rel=1e-6)
    subheaders = image.get_subheaders()
    calibration = float(header["ecat_calibration_factor"])
    from nibabel.volumeutils import array_from_file, native_code
    for k in range(3):
        # the voxels in the order of the file (nibabel reorders them depending on
        # patient_orientation): x towards left, y towards posterior, planes from superior
        # to inferior, i.e. all three axes reversed compared to RAS
        dtype = subheaders._get_data_dtype(k)
        if subheaders._header.endianness is not native_code:
            dtype = dtype.newbyteorder(subheaders._header.endianness)
        raw = array_from_file(subheaders.get_shape(k), dtype, subheaders.fileobj, offset=subheaders._get_frame_offset(k))
        values = raw[::-1, ::-1, ::-1] * calibration * float(subheaders.subheaders[k]["scale_factor"])
        assert np.allclose(values, frame(k).values()), k
        assert int(subheaders.subheaders[k]["frame_start_time"]) == TIMING[k][0] * 1000
        assert int(subheaders.subheaders[k]["frame_duration"]) == TIMING[k][1] * 1000


@pytest.mark.parametrize("suffix", list(FORMATS))
def test_create_attributes(tmp_path, suffix):
    path = tmp_path / ("new" + suffix)
    with pmedio.create(path, attributes=ATTRIBUTES) as f:
        # the main header is made from the first volume
        with pytest.raises(ValueError, match="no volume written yet"):
            f.attributes()
        f.write_volume(frame(0), 1)
        written = f.attributes()
    with pmedio.open(path) as f:
        read = f.attributes()
    for key, value in ATTRIBUTES.items():
        assert written[key] == pytest.approx(value) if isinstance(value, float) else written[key] == value, key
        assert read[key] == pytest.approx(value) if isinstance(value, float) else read[key] == value, key


def test_create_attributes_with_source(tmp_path):
    source = pathlib.Path(__file__).resolve().parents[2] / "testsuite" / "data" / "ecat" / "pet-static.v"
    if not source.is_file():
        pytest.skip("test data not available")
    with pmedio.open(source) as like:
        original = like.attributes()
        with pmedio.create(tmp_path / "new.nii", like=like, attributes={"PatientWeight": 99.0}) as out:
            assert out.attributes()["PatientWeight"] == 99.0
            out.write_volume(like.read_volume(), 1)
        with pytest.raises(ValueError, match="number of frames is taken from the source"):
            pmedio.create(tmp_path / "frames.v", like=like, frames=2)
    with pmedio.open(tmp_path / "new.nii") as f:
        attributes = f.attributes()
    assert attributes["PatientWeight"] == 99.0
    assert attributes["PatientName"] == original["PatientName"]


def test_create_attribute_values(tmp_path):
    # numpy scalars are stored as the numbers they hold
    path = tmp_path / "new.v"
    with pmedio.create(path, attributes={"PatientWeight": np.float32(70.25), "PatientSize": np.float64(1.75)}) as f:
        f.write_volume(frame(0), 1)
    with pmedio.open(path) as f:
        assert f.attributes()["PatientWeight"] == 70.25
        assert f.attributes()["PatientSize"] == 1.75


@pytest.mark.parametrize("attributes, error", [
    ({"PatientWeight": "heavy"}, ValueError),
    ({"ECAT7": {"No_Such_Field": 1}}, ValueError),
    ({"PatientWeight": object()}, TypeError),
    (["PatientWeight"], TypeError),
    ("PatientWeight", TypeError),
])
def test_create_invalid_attributes(tmp_path, attributes, error):
    # reported before the file is created
    path = tmp_path / "new.v"
    with pytest.raises(error):
        pmedio.create(path, attributes=attributes)
    assert not path.exists()


@pytest.mark.parametrize("frames, error", [(0, ValueError), (-1, ValueError), (32768, ValueError),
                                           (True, TypeError), (2.0, TypeError), ("2", TypeError)])
def test_create_invalid_frames(tmp_path, frames, error):
    path = tmp_path / "new.nii"
    with pytest.raises(error):
        pmedio.create(path, frames=frames)
    assert not path.exists()


def test_create_nifti_frames(tmp_path):
    # a NIfTI file holds the number of frames given to create()
    path = tmp_path / "new.nii"
    with pmedio.create(path) as f:
        f.write_volume(frame(0), 1)
        with pytest.raises(ValueError, match="created for 1 frame"):
            f.write_volume(frame(1), 2)
    assert len(read_ras(path)) == 1

    with pmedio.create(path, frames=2, overwrite=True) as f:
        for n in (0, 3, -2):
            with pytest.raises(ValueError, match="does not exist"):
                f.write_volume(frame(0), n)
        f.write_volume(frame(1), 2)
        f.write_volume(frame(0), -1)   # frame 1
    assert [v.frame_start for v in read_ras(path)] == [TIMING[0][0], TIMING[1][0]]


@pytest.mark.parametrize("suffix", [".nii", ".nii.gz", ".hdr"])
def test_create_nifti_incomplete(tmp_path, suffix):
    # a NIfTI file without all of its frames is removed
    path = tmp_path / ("new" + suffix)
    with pytest.raises(ValueError, match="only 2 of the 3 frames"):
        with pmedio.create(path, frames=3) as f:
            f.write_volume(frame(0), 1)
            f.write_volume(frame(2), 3)
    assert list(tmp_path.iterdir()) == []

    f = pmedio.create(path, frames=2)
    f.write_volume(frame(1), 2)
    with pytest.raises(ValueError, match="only 1 of the 2 frames"):
        f.close()
    f.close()
    assert f.closed
    assert list(tmp_path.iterdir()) == []

    # an exception that is on its way is not replaced
    with pytest.raises(KeyError):
        with pmedio.create(path, frames=2) as f:
            f.write_volume(frame(0), 1)
            raise KeyError("other")
    assert list(tmp_path.iterdir()) == []


@pytest.mark.parametrize("suffix", [".nii", ".nii.gz", ".hdr"])
def test_create_nifti_incomplete_with_source(tmp_path, suffix):
    # a NIfTI file created from a source file has the frames of the source: without all
    # of them it would be truncated (nibabel: "could the file be damaged?"), so it is removed
    source = pathlib.Path(__file__).resolve().parents[2] / "testsuite" / "data" / "ecat" / "pet-dynamic.v"
    if not source.is_file():
        pytest.skip("test data not available")
    path = tmp_path / ("new" + suffix)
    with pmedio.open(source) as like:
        assert len(like.frame_numbers()) == 3
        with pytest.raises(ValueError, match="only 1 of the 3 frames"):
            with pmedio.create(path, like=like) as f:
                f.write_volume(like.read_volume(1), 1)
        assert list(tmp_path.iterdir()) == []
        with pmedio.create(path, like=like) as f:
            with pytest.raises(ValueError, match="frame 4 does not exist"):
                f.write_volume(like.read_volume(1), 4)
            for n in like.frame_numbers():
                f.write_volume(like.read_volume(n), n)
    assert nibabel_image(path).get_fdata().shape[3] == 3


@pytest.mark.parametrize("suffix", list(FORMATS))
def test_create_without_volume(tmp_path, suffix):
    # a file without a volume has no main header either: it is removed
    path = tmp_path / ("new" + suffix)
    with pmedio.create(path):
        pass
    assert list(tmp_path.iterdir()) == []

    f = pmedio.create(path)
    f.close()
    assert list(tmp_path.iterdir()) == []

    with pytest.raises(KeyError):
        with pmedio.create(path):
            raise KeyError("other")
    assert list(tmp_path.iterdir()) == []

    # an existing file is kept until a volume is written
    path.write_bytes(b"old")
    with pytest.raises(FileExistsError):
        pmedio.create(path)
    assert path.read_bytes() == b"old"


def test_create_errors(tmp_path):
    with pytest.raises(ValueError, match="cannot be written"):
        pmedio.create(tmp_path / "new.data", format=pmedio.Format.ConcordeMicropet)
    with pytest.raises(TypeError, match="has to be a pmedio.File"):
        pmedio.create(tmp_path / "new.v", like="other.v")
    with pmedio.create(tmp_path / "new.v") as f:
        with pytest.raises(ValueError, match="no data"):
            f.write_volume(pmedio.Volume(), 1)
        f.write_volume(frame(0), 1)
    with pmedio.open(tmp_path / "new.v") as f:
        assert f.frame_numbers() == [1]
    assert sorted(os.listdir(tmp_path)) == ["new.v"]
