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

# Metadata of NIfTI files written by libmedio as other tools read it: the JSON
# extension (BIDS keys) and the DICOM extension (attributes and the frame timing
# of PMOD) read with nibabel and pydicom, and NIfTI files of other tools (a
# DICOM extension like PMOD writes it, created with nibabel and pydicom) read
# with libmedio. The expected values are those of testsuite/data/<format>/<file>.json
# (nibabel), not those of libmedio.

import datetime
import json

import numpy as np
import pytest

import pmedio

from test_volume import DATA_DIR, Expected, load_json, needs_data

nib = pytest.importorskip("nibabel")
pydicom = pytest.importorskip("pydicom")


def extensions(path):
    """JSON metadata and DICOM data set of a NIfTI file read with nibabel."""
    img = nib.load(str(path))
    codes = [e.get_code() for e in img.header.extensions]
    metadata = None
    dataset = None
    for e in img.header.extensions:
        if e.get_code() == 6:
            metadata = json.loads(e.get_content())
        elif e.get_code() == 2:
            dataset = e.get_content()
    return img, codes, metadata, dataset


def dicom_datetime(iso):
    """DICOM DT of an ISO 8601 date and time with "Z"."""
    t = datetime.datetime.fromisoformat(iso.replace("Z", "+00:00"))
    return t.strftime("%Y%m%d%H%M%S") + "+0000"


def as_list(value):
    return list(value) if isinstance(value, (list, tuple, pydicom.multival.MultiValue)) else [value]


@needs_data
@pytest.mark.parametrize("suffix", [".nii", ".nii.gz", ".hdr"])
@pytest.mark.parametrize("name", ["ecat/pet-dynamic.v", "ecat/pet-static.v"])
def test_written_metadata(tmp_path, name, suffix):
    expected = Expected(name, name + ".json")
    attributes = load_json(name + ".json")["attributes"]["main"]
    out_path = tmp_path / ("out" + suffix)

    with pmedio.open(DATA_DIR / name) as source:
        with pmedio.create(out_path, like=source) as out:
            for k, n in enumerate(source.frame_numbers()):
                out.write_volume(source.read_volume(n), k + 1)

    img, codes, metadata, ds = extensions(out_path)
    assert codes == [6, 2]  # the JSON extension first, as in earlier versions

    # descrip like PMOD writes it: "<patient name> [<study type>] <study description>"
    # (descrip holds up to 79 characters)
    descrip = img.header["descrip"].tobytes().rstrip(b"\0").decode("latin-1")
    assert descrip.startswith(attributes["PatientName"] + " [")
    description = descrip[descrip.index("] ") + 2:]
    assert description and attributes["StudyDescription"].startswith(description)

    starts = [t[0] for t in expected.timing]
    durations = [t[1] for t in expected.timing]

    # JSON extension: BIDS PET keys
    assert metadata["Units"] == "Bq/mL"
    assert metadata["FrameTimesStart"] == pytest.approx(starts)
    assert metadata["FrameDuration"] == pytest.approx(durations)
    assert metadata["PatientName"] == attributes["PatientName"]
    scan = datetime.datetime.fromisoformat(attributes["AcquisitionDateTime"].replace("Z", "+00:00"))
    injection = datetime.datetime.fromisoformat(attributes["RadiopharmaceuticalStartDateTime"].replace("Z", "+00:00"))
    assert metadata["TimeZero"] == scan.strftime("%H:%M:%S")
    assert metadata["ScanStart"] == 0
    assert metadata["InjectionStart"] == pytest.approx((injection - scan).total_seconds())
    assert metadata["InjectedRadioactivity"] == pytest.approx(attributes["RadionuclideTotalDose"] / 1e6)
    assert metadata["InjectedRadioactivityUnits"] == "MBq"

    # DICOM extension (pydicom data set)
    assert ds.PatientName == attributes["PatientName"]
    assert ds.PatientID == attributes["PatientID"]
    assert ds.PatientBirthDate == attributes["PatientBirthDate"].replace("-", "")
    assert float(ds.PatientWeight) == pytest.approx(attributes["PatientWeight"])
    assert ds.StudyDescription == attributes["StudyDescription"]
    assert ds.AcquisitionDateTime == dicom_datetime(attributes["AcquisitionDateTime"])
    assert ds.Units == "BQML"
    info = ds.RadiopharmaceuticalInformationSequence[0]
    assert info.Radiopharmaceutical == attributes["Radiopharmaceutical"]
    assert float(info.RadionuclideTotalDose) == pytest.approx(attributes["RadionuclideTotalDose"])
    assert float(info.RadionuclideHalfLife) == pytest.approx(attributes["RadionuclideHalfLife"])
    assert info.RadiopharmaceuticalStartDateTime == dicom_datetime(attributes["RadiopharmaceuticalStartDateTime"])

    # frame timing in the private tags of PMOD (start times in s, durations in ms)
    assert ds[0x0055, 0x0010].value == "PMOD_1"
    assert as_list(ds[0x0055, 0x1001].value) == pytest.approx(starts)
    assert as_list(ds[0x0055, 0x1004].value) == pytest.approx([d * 1000.0 for d in durations])


@needs_data
def test_written_metadata_zero_duration(tmp_path):
    # a last frame duration of 0 ends the DICOM data set with zero bytes, which
    # nibabel removes as padding of the extension: it has to be read anyway
    timing = [(0.0, 30.0), (30.0, 60.0), (90.0, 0.0)]
    out_path = tmp_path / "out.nii"
    with pmedio.open(DATA_DIR / "ecat/pet-dynamic.v") as source:
        with pmedio.create(out_path, like=source) as out:
            for k, n in enumerate(source.frame_numbers()):
                v = source.read_volume(n)
                v.frame_start, v.frame_duration = timing[k]
                out.write_volume(v, k + 1)

    _, codes, metadata, ds = extensions(out_path)
    assert codes == [6, 2]
    assert metadata["FrameDuration"] == [30.0, 60.0, 0.0]
    assert as_list(ds[0x0055, 0x1004].value) == [30000.0, 60000.0, 0.0]
    assert ds.Units == "BQML"


def test_read_pmod_file(tmp_path):
    # a NIfTI file as PMOD writes it: no qform/sform, the patient name, study
    # type and description in descrip, units and frame timing in the DICOM extension
    starts = [0.0, 20.0, 60.0]
    durations_ms = [20000.0, 40000.0, 60000.0]
    ds = pydicom.Dataset()
    ds.add_new((0x0054, 0x1001), "CS", "BQML")
    ds.add_new((0x0055, 0x0010), "LO", "PMOD_1")
    ds.add_new((0x0055, 0x1001), "FD", starts)
    ds.add_new((0x0055, 0x1004), "FD", durations_ms)

    img = nib.Nifti1Image(np.arange(4 * 3 * 2 * 3, dtype=np.int16).reshape((4, 3, 2, 3)), None)
    img.set_qform(None, 0)
    img.set_sform(None, 0)
    img.header.set_xyzt_units("mm", "sec")
    img.header["descrip"] = b"Doe^Jane [DYNAMIC] brain PET"
    img.header.extensions.append(nib.nifti1.Nifti1DicomExtension(2, ds))
    path = tmp_path / "pmod.nii"
    nib.save(img, str(path))

    with pmedio.open(path) as f:
        a = f.attributes()
        assert a["PatientName"] == "Doe^Jane"
        assert a["StudyDescription"] == "brain PET"
        assert a["ECAT7"]["Study_Type"] == "DYNAMIC"
        assert a["Units"] == "BQML"
        volumes = [f.read_volume(n) for n in f.frame_numbers()]
        assert [v.frame_start for v in volumes] == pytest.approx(starts)
        assert [v.frame_duration for v in volumes] == pytest.approx([d / 1000.0 for d in durations_ms])
