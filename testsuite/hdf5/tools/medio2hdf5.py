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

# Prototype of the HDF5 layout of docs/hdf5.md (MEDIO-IMAGE): converts an
# image file of any format libmedio reads into an HDF5 file of this layout,
# with pmedio and h5py. It is meant for trying out and discussing the layout
# before libmedio itself reads and writes it; it is not part of the library.
#
# Usage: python3 medio2hdf5.py input output.h5

import json
import os
import sys
import tempfile

import h5py
import numpy as np

import pmedio

FORMAT = "MEDIO-IMAGE"
FORMAT_VERSION = "0.1"
MAX_CHUNK_BYTES = 1 << 20
STRING = h5py.string_dtype("utf-8")

# DICOM Units (and the further terms of libmedio) in UCUM notation
UCUM = {
    "BQML": "Bq/mL",
    "KBQCC": "kBq/mL",
    "CNTS": "{counts}",
    "CPS": "{counts}/s",
    "NONE": "1",
    "CM2": "cm2",
    "PCNT": "%",
    "GML": "g/mL{SUVbw}",
}

# frame attributes of libmedio stored as datasets of /image/volumes
FRAME_DATASETS = {"DecayFactor": "1", "DeadTimeFactor": "1", "FrameReferenceTime": "ms", "TriggerTime": "ms"}


def set_attribute(obj, name, value):
    """an attribute with the HDF5 type of the layout for a JSON value"""
    entry = pmedio.dictionary_entry(name)
    vr = entry["vr"] if entry else None
    values = value if isinstance(value, list) else [value]
    if vr in ("DS", "FD", "FL"):
        data = np.array(values, dtype="<f8")
    elif vr in ("IS", "US", "UL", "SS", "SL"):
        data = np.array(values, dtype="<i4")
    else:
        data = np.array([str(v) for v in values], dtype=object)
    if isinstance(value, list):
        obj.attrs.create(name, data, dtype=STRING if data.dtype == object else data.dtype)
    elif data.dtype == object:
        obj.attrs.create(name, data[0], dtype=STRING)
    else:
        obj.attrs.create(name, data[0])


def write_header(f, attributes):
    header = f.create_group("header")
    for name, value in attributes.items():
        entry = pmedio.dictionary_entry(name)
        if entry is None or entry["scope"] != "main":
            continue  # native sections, frame attributes and unknown keys: JSON only
        if entry["vr"] == "SQ":
            if isinstance(value, dict):
                group = header.create_group(name)
                for key, item in value.items():
                    group.attrs.create(key, str(item), dtype=STRING)
        else:
            set_attribute(header, name, value)
    header.create_dataset("json", data=json.dumps(attributes, ensure_ascii=False), dtype=STRING)


def write_file(path, volumes, frames, attributes):
    """volumes: pmedio.Volume of every frame, frames: their frame numbers"""
    arrays = [np.ascontiguousarray(v.values().transpose(2, 1, 0)) for v in volumes]  # (z, y, x)
    if len(set(a.shape for a in arrays)) != 1:
        raise ValueError("the frames do not have the same dimensions")
    dtype = "<f8" if arrays[0].dtype == np.float64 else "<f4"
    nz, ny, nx = arrays[0].shape
    planes = max(1, min(nz, MAX_CHUNK_BYTES // (nx * ny * np.dtype(dtype).itemsize)))
    affines = np.array([v.affine for v in volumes], dtype="<f8")
    frame_attributes = attributes.get("Frames", [])

    with h5py.File(path, "w", libver=("earliest", "v108")) as f:
        f.attrs.create("Format", FORMAT, dtype=STRING)
        f.attrs.create("FormatVersion", FORMAT_VERSION, dtype=STRING)
        f.attrs.create("Creator", "medio2hdf5.py (prototype, h5py %s)" % h5py.__version__, dtype=STRING)

        image = f.create_group("image")
        data = image.create_dataset("data", shape=(len(arrays), nz, ny, nx), dtype=dtype,
                                    chunks=(1, planes, ny, nx), shuffle=True, compression="gzip")
        for k, a in enumerate(arrays):
            data[k] = a
        units = attributes.get("Units")
        if units is not None:
            if units not in UCUM:
                raise ValueError("no UCUM notation known for the units %s" % units)
            data.attrs.create("Units", units, dtype=STRING)
            data.attrs.create("units", UCUM[units], dtype=STRING)

        image.create_dataset("affine", data=affines[0]).attrs.create("units", "mm", dtype=STRING)
        table = image.create_group("volumes")
        if not np.allclose(affines, affines[0], rtol=1e-6, atol=1e-3):  # as CMedIOVolume::hasSameGeometry()
            table.create_dataset("affine", data=affines).attrs.create("units", "mm", dtype=STRING)
        table.create_dataset("Frame", data=np.array(frames, dtype="<i4"))
        for name, values in (("FrameTimesStart", [v.frame_start for v in volumes]),
                             ("FrameDuration", [v.frame_duration for v in volumes])):
            table.create_dataset(name, data=np.array(values, dtype="<f8")).attrs.create("units", "s", dtype=STRING)
        for name, unit in FRAME_DATASETS.items():
            if len(frame_attributes) == len(volumes) and all(name in a for a in frame_attributes):
                values = np.array([a[name] for a in frame_attributes], dtype="<f8")
                table.create_dataset(name, data=values).attrs.create("units", unit, dtype=STRING)

        write_header(f, attributes)


def convert(source, target):
    """converts an image file into an HDF5 file; no file is left behind on failure"""
    with pmedio.open(source) as f:
        frames = list(f.frame_numbers())
        volumes = [f.read_volume(n) for n in frames]
        attributes = f.attributes()
    directory = os.path.dirname(os.path.abspath(target))
    handle, temporary = tempfile.mkstemp(suffix=".tmp", prefix=os.path.basename(target) + ".", dir=directory)
    os.close(handle)
    try:
        write_file(temporary, volumes, frames, attributes)
        os.replace(temporary, target)
    except BaseException:
        os.remove(temporary)
        raise


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("usage: medio2hdf5.py input output.h5")
    convert(sys.argv[1], sys.argv[2])
