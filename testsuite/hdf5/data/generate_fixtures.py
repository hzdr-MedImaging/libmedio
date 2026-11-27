#!/usr/bin/env python3
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

# Generates the HDF5 fixtures of the tests of CHDF5File and expected.json with
# the values libmedio has to read from them, with h5py and numpy only (not
# with libmedio): small synthetic images of the layout of docs/hdf5.md with
# the features the test datasets do not have (gates, bed positions, typed
# header attributes, strings as other programs write them) and files that
# violate the layout.
#
# Usage: python3 generate_fixtures.py [output directory]

import json
import os
import sys

import h5py
import numpy as np

OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))

TEXT = h5py.string_dtype("utf-8")
NX, NY, NZ = 4, 3, 2
AFFINE = np.array([[2.0, 0.0, 0.0, -3.0], [0.0, 2.5, 0.0, -2.5], [0.0, 0.0, -3.0, 10.0]])


def values(volumes, dtype):
    """voxel (x, y, z) of volume v has the value 1000 v + 100 z + 10 y + x + 0.5"""
    v, z, y, x = np.meshgrid(np.arange(volumes), np.arange(NZ), np.arange(NY), np.arange(NX), indexing="ij")
    return (1000 * v + 100 * z + 10 * y + x + 0.5).astype(dtype)


def text(obj, name, value):
    obj.attrs.create(name, value, dtype=TEXT)


def minimal(f, volumes=2, dtype="<f4", layout=True):
    """the required objects of the layout"""
    if layout:
        text(f, "Format", "MEDIO-IMAGE")
        text(f, "FormatVersion", "0.1")
    f.create_dataset("image/data", data=values(volumes, dtype), chunks=(1, NZ, NY, NX), shuffle=True, compression="gzip")
    text(f.create_dataset("image/affine", data=AFFINE), "units", "mm")
    f.create_dataset("image/volumes/Frame", data=np.arange(1, volumes + 1, dtype="<i4"))
    for name, data in (("FrameTimesStart", 60.0 * np.arange(volumes)), ("FrameDuration", np.full(volumes, 60.0))):
        text(f.create_dataset("image/volumes/" + name, data=data), "units", "s")


def replace(f, name, data):
    del f[name]
    return f.create_dataset(name, data=data)


def gated(f):
    """two frames (numbers 1 and 3) with two gates each, typed header attributes and JSON"""
    minimal(f, volumes=4)
    text(f, "Creator", "generate_fixtures.py")
    text(f["image/data"], "Units", "BQML")
    text(f["image/data"], "units", "Bq/mL")
    volumes = f["image/volumes"]
    replace(f, "image/volumes/Frame", np.array([1, 1, 3, 3], dtype="<i4"))
    volumes.create_dataset("Gate", data=np.array([1, 2, 1, 2], dtype="<i4"))
    text(replace(f, "image/volumes/FrameTimesStart", np.array([0.0, 0.0, 90.5, 90.5])), "units", "s")
    text(replace(f, "image/volumes/FrameDuration", np.array([30.0, 30.0, 60.25, 60.25])), "units", "s")
    text(volumes.create_dataset("DecayFactor", data=np.array([1.01, 1.01, 1.05, 1.05])), "units", "1")
    text(volumes.create_dataset("FrameReferenceTime", data=np.array([14900.0, 14900.0, 120100.0, 120100.0])), "units", "ms")
    text(volumes.create_dataset("TriggerTime", data=np.array([0.0, 400.0, 0.0, 400.0])), "units", "ms")

    header = f.create_group("header")
    header.attrs["PatientWeight"] = np.float64(72.5)
    header.attrs["RadionuclideTotalDose"] = np.float64(3.01e8)
    header.attrs["SeriesNumber"] = np.int32(4)
    text(header, "PatientName", "Müller^Hans")
    text(header, "AcquisitionDateTime", "2026-06-03T17:04:56+02:00")
    text(header, "DecayCorrectionDateTime", "2026-06-03T17:04:56+02:00")
    text(header, "DecayCorrection", "START")
    text(header, "Units", "BQML")
    header.attrs.create("CorrectedImage", np.array(["DECY", "ATTN"], dtype=object), dtype=TEXT)
    code = header.create_group("RadionuclideCodeSequence")
    text(code, "CodeValue", "C-111A1")
    text(code, "CodingSchemeDesignator", "SRT")
    text(code, "CodeMeaning", "^18^Fluorine")
    # the typed attributes are the valid ones: weight and units of the JSON differ
    stored = {"PatientWeight": 70.0, "Units": "CNTS", "StudyDescription": "gated study",
              "ECAT7": {"Study_Type": "GATED"}, "(0029,1010)": "private",
              "Frames": [{"DecayFactor": 9.0, "ECAT7": {"Annotation": "volume %d" % k}} for k in range(4)]}
    header.create_dataset("json", data=json.dumps(stored), dtype=TEXT)

    main = {"PatientWeight": 72.5, "RadionuclideTotalDose": 3.01e8, "SeriesNumber": 4,
            "PatientName": "Müller^Hans", "AcquisitionDateTime": "2026-06-03T17:04:56+02:00",
            "DecayCorrectionDateTime": "2026-06-03T17:04:56+02:00", "DecayCorrection": "START",
            "Units": "BQML", "CorrectedImage": ["DECY", "ATTN"],
            "RadionuclideCodeSequence": {"CodeValue": "C-111A1", "CodingSchemeDesignator": "SRT",
                                         "CodeMeaning": "^18^Fluorine"},
            "StudyDescription": "gated study", "ECAT7": {"Study_Type": "GATED"}, "(0029,1010)": "private"}
    frames = [{"ActualFrameDuration": [30000, 30000, 60250, 60250][k], "DecayFactor": [1.01, 1.01, 1.05, 1.05][k],
               "FrameReferenceTime": [14900, 14900, 120100, 120100][k], "TriggerTime": [0, 400, 0, 400][k],
               "ECAT7": {"Annotation": "volume %d" % k}} for k in range(4)]
    return main, frames


def beds(f):
    """two bed positions with their own affine, big-endian float64 values, strings of
    fixed length, no header and no units"""
    f.attrs.create("Format", np.bytes_("MEDIO-IMAGE"))
    f.attrs.create("FormatVersion", np.bytes_("0.3"))
    f.create_dataset("image/data", data=values(2, ">f8"), chunks=(1, 1, NY, NX))
    f.create_dataset("image/affine", data=AFFINE.astype(">f8")).attrs.create("units", np.bytes_("mm"))
    second = AFFINE.copy()
    second[2, 3] -= 150.0
    f.create_dataset("image/volumes/affine", data=np.array([AFFINE, second]))
    f.create_dataset("image/volumes/Frame", data=np.array([1, 1], dtype="<i8"))
    f.create_dataset("image/volumes/Bed", data=np.array([0, 1], dtype="<u2"))
    f.create_dataset("image/volumes/FrameTimesStart", data=np.array([0, 120], dtype="<i4"))
    f.create_dataset("image/volumes/FrameDuration", data=np.array([120.0, 120.0], dtype="<f4"))
    f.create_dataset("image/volumes/TablePosition", data=np.array([-617.0, -767.0]))
    frames = [{"ActualFrameDuration": 120000, "TablePosition": [-617, -767][k]} for k in range(2)]
    return {}, frames


def other_layout(f):
    f.create_dataset("ITKImage/0/VoxelData", data=values(1, "<f4")[0])


def no_format(f):
    minimal(f, layout=False)


def other_format(f):
    minimal(f)
    text(f, "Format", "NeXus")


def other_major_version(f):
    minimal(f)
    text(f, "FormatVersion", "1.0")


def invalid_version(f):
    minimal(f)
    text(f, "FormatVersion", "first")


def no_data(f):
    minimal(f)
    del f["image/data"]


def integer_data(f):
    minimal(f)
    replace(f, "image/data", values(2, "<i2"))


def three_dimensions(f):
    minimal(f)
    replace(f, "image/data", values(1, "<f4")[0])


def one_unit_only(f):
    minimal(f)
    text(f["image/data"], "Units", "BQML")


def no_affine(f):
    minimal(f)
    del f["image/affine"]


def affine_of_other_size(f):
    minimal(f)
    replace(f, "image/affine", np.eye(4))


def no_frame_duration(f):
    minimal(f)
    del f["image/volumes/FrameDuration"]


def one_frame_time(f):
    minimal(f)
    replace(f, "image/volumes/FrameTimesStart", np.array([0.0]))


def frame_zero(f):
    minimal(f)
    replace(f, "image/volumes/Frame", np.array([0, 1], dtype="<i4"))


def same_frame_twice(f):
    minimal(f)
    replace(f, "image/volumes/Frame", np.array([1, 1], dtype="<i4"))


def negative_duration(f):
    minimal(f)
    replace(f, "image/volumes/FrameDuration", np.array([60.0, -1.0]))


def text_as_decay_factor(f):
    minimal(f)
    f.create_dataset("image/volumes/DecayFactor", data=np.array(["1.0", "1.1"], dtype=object), dtype=TEXT)


def json_is_no_object(f):
    minimal(f)
    f.create_dataset("header/json", data="[1, 2", dtype=TEXT)


VALID = {"gated.h5": (gated, {}), "beds.h5": (beds, {"userblock_size": 512})}
INVALID = {
    "other-layout.h5": (other_layout, "another layout"),
    "no-format.h5": (no_format, "another layout"),
    "other-format.h5": (other_format, "another layout"),
    "other-major-version.h5": (other_major_version, "version 1.0"),
    "invalid-version.h5": (invalid_version, "FormatVersion"),
    "no-data.h5": (no_data, "/image/data"),
    "integer-data.h5": (integer_data, "float32"),
    "three-dimensions.h5": (three_dimensions, "[volume][z][y][x]"),
    "one-unit-only.h5": (one_unit_only, "Units"),
    "no-affine.h5": (no_affine, "/image/affine"),
    "affine-of-other-size.h5": (affine_of_other_size, "/image/affine"),
    "no-frame-duration.h5": (no_frame_duration, "FrameDuration"),
    "one-frame-time.h5": (one_frame_time, "FrameTimesStart"),
    "frame-zero.h5": (frame_zero, "/image/volumes/Frame"),
    "same-frame-twice.h5": (same_frame_twice, "same frame"),
    "negative-duration.h5": (negative_duration, "FrameDuration"),
    "text-as-decay-factor.h5": (text_as_decay_factor, "DecayFactor"),
    "json-is-no-object.h5": (json_is_no_object, "/header/json"),
}


def read_back(path, main, frames):
    """the content of a valid file as h5py reads it"""
    with h5py.File(path, "r") as f:
        data = f["image/data"]
        count = data.shape[0]
        table = f["image/volumes"]

        def column(name, default):
            return [int(v) for v in table[name][()]] if name in table else [default] * count

        affines = table["affine"][()] if "affine" in table else np.array([f["image/affine"][()]] * count)
        index = list(zip(column("Frame", 1), column("Gate", 1), column("Bed", 0)))
        first = (min(i[1] for i in index), min(i[2] for i in index))
        units = data.attrs["Units"] if "Units" in data.attrs else None
        return {
            "valid": True,
            "version": f.attrs["FormatVersion"].decode() if isinstance(f.attrs["FormatVersion"], bytes) else f.attrs["FormatVersion"],
            "dimensions": [data.shape[3], data.shape[2], data.shape[1]],
            "datatype": "float64" if data.dtype.itemsize == 8 else "float32",
            "units": units.decode() if isinstance(units, bytes) else units,
            "first_gate_frames": [i[0] for i in index if (i[1], i[2]) == first],
            "attributes": main,
            "volumes": [{"frame": index[k][0], "gate": index[k][1], "bed": index[k][2],
                         "start": float(table["FrameTimesStart"][k]), "duration": float(table["FrameDuration"][k]),
                         "affine": affines[k].astype(float).tolist(),
                         # x running fastest
                         "values": data[k].astype(float).ravel().tolist(),
                         "attributes": frames[k]} for k in range(count)],
        }


def main():
    files = {}
    for name, (make, options) in VALID.items():
        path = os.path.join(OUT, name)
        with h5py.File(path, "w", libver=("earliest", "v108"), **options) as f:
            attributes, frames = make(f)
        files[name] = read_back(path, attributes, frames)
    for name, (make, reason) in INVALID.items():
        with h5py.File(os.path.join(OUT, name), "w", libver=("earliest", "v108")) as f:
            make(f)
        files[name] = {"valid": False, "reason": reason}

    expected = {"generator": "manual and h5py (generate_fixtures.py)", "files": files}
    with open(os.path.join(OUT, "expected.json"), "w", encoding="utf-8") as f:
        json.dump(expected, f, indent=1, ensure_ascii=False)
        f.write("\n")


if __name__ == "__main__":
    main()
