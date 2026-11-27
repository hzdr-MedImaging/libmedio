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

# Checks HDF5 files against the layout of docs/hdf5.md (MEDIO-IMAGE).
#
# The checker only uses h5py and numpy (and pydicom for the types of the
# header attributes, if it is installed), not libmedio, so that it can serve
# as independent implementation of the layout.
#
# Usage: python3 check_hdf5.py file.h5 [...]
#        exit status 1 if a file has errors; warnings are printed only

import json
import re
import sys

import h5py
import numpy as np

FORMAT = "MEDIO-IMAGE"
MAJOR = 0
MAX_CHUNK_BYTES = 1 << 20

# /image/volumes: name -> (kind, unit, required)
VOLUME_DATASETS = {
    "Frame": ("i", None, True),
    "Gate": ("i", None, False),
    "Bed": ("i", None, False),
    "FrameTimesStart": ("f", "s", True),
    "FrameDuration": ("f", "s", True),
    "FrameReferenceTime": ("f", "ms", False),
    "DecayFactor": ("f", "1", False),
    "DeadTimeFactor": ("f", "1", False),
    "TriggerTime": ("f", "ms", False),
    "TablePosition": ("f", "mm", False),
}

# header attributes recommended for quantitative use
RECOMMENDED = ["AcquisitionDateTime", "DecayCorrection", "DecayCorrectionDateTime", "CorrectedImage",
               "RadionuclideHalfLife", "RadionuclideTotalDose", "RadiopharmaceuticalStartDateTime",
               "PatientWeight"]
RECOMMENDED_GROUPS = ["RadionuclideCodeSequence"]
CODE_ATTRIBUTES = ["CodeValue", "CodingSchemeDesignator", "CodingSchemeVersion", "CodeMeaning"]

DATETIME = re.compile(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(\.\d+)?(Z|[+-]\d{2}:\d{2})?$")
DATE = re.compile(r"^\d{4}-\d{2}-\d{2}$")


def vr_of(keyword):
    """DICOM value representation of a keyword (None: unknown or pydicom is missing)"""
    try:
        from pydicom.datadict import dictionary_VR, tag_for_keyword
    except ImportError:
        return None
    tag = tag_for_keyword(keyword)
    return dictionary_VR(tag) if tag is not None else None


def kind_of_vr(vr):
    if vr in ("DS", "FD", "FL"):
        return "f"
    if vr in ("IS", "US", "UL", "SS", "SL"):
        return "i"
    if vr == "SQ":
        return "group"
    return "s" if vr else None


class Result:
    def __init__(self):
        self.errors = []
        self.warnings = []

    def error(self, text):
        self.errors.append(text)

    def warning(self, text):
        self.warnings.append(text)


def is_string(h5type):
    return isinstance(h5type, h5py.h5t.TypeStringID)


def text(value):
    return value.decode("utf-8") if isinstance(value, bytes) else value


def check_string(result, where, type_id, space_shape=None):
    """True if the type is a string; warns about encodings libmedio does not write"""
    if not is_string(type_id):
        result.error("%s is no string" % where)
        return False
    if not type_id.is_variable_str() or type_id.get_cset() != h5py.h5t.CSET_UTF8:
        result.warning("%s is no variable-length UTF-8 string" % where)
    return True


def string_attribute(result, obj, name, where, required=True):
    if name not in obj.attrs:
        if required:
            result.error("%s: attribute %s is missing" % (where, name))
        return None
    attribute = obj.attrs.get_id(name)
    if attribute.shape != ():
        result.error("%s: attribute %s is no scalar" % (where, name))
        return None
    if not check_string(result, "%s: attribute %s" % (where, name), attribute.get_type()):
        return None
    return text(obj.attrs[name])


def check_root(result, f):
    if string_attribute(result, f, "Format", "/") != FORMAT:
        result.error("/: Format is not %s" % FORMAT)
        return False
    version = string_attribute(result, f, "FormatVersion", "/")
    if version is None or not re.match(r"^\d+\.\d+$", version):
        result.error("/: FormatVersion is not <major>.<minor>")
        return False
    if int(version.split(".")[0]) != MAJOR:
        result.error("/: major version %s is not supported" % version)
        return False
    if "Creator" not in f.attrs:
        result.warning("/: attribute Creator is missing")
    return True


def check_data(result, f):
    """the number of volumes (None if there is no valid dataset)"""
    data = f.get("image/data")
    if not isinstance(data, h5py.Dataset):
        result.error("/image/data is missing")
        return None
    if data.ndim != 4:
        result.error("/image/data has %d instead of 4 dimensions" % data.ndim)
        return None
    if data.dtype not in (np.dtype("<f4"), np.dtype("<f8")):
        result.error("/image/data has the data type %s instead of little-endian float32 or float64" % data.dtype)
    filters = set(data.id.get_create_plist().get_filter(k)[0] for k in range(data.id.get_create_plist().get_nfilters()))
    if not filters <= {h5py.h5z.FILTER_SHUFFLE, h5py.h5z.FILTER_DEFLATE}:
        result.error("/image/data uses filters other than shuffle and deflate")
    if data.chunks is None:
        result.error("/image/data is not chunked")
    else:
        nv, nz, ny, nx = data.shape
        cv, cz, cy, cx = data.chunks
        if cv != 1:
            result.error("/image/data: a chunk spans %d volumes" % cv)
        if (cy, cx) != (ny, nx):
            result.error("/image/data: a chunk does not hold whole planes")
        if cz > 1 and cz * cy * cx * data.dtype.itemsize > MAX_CHUNK_BYTES:
            result.error("/image/data: a chunk of several planes has more than 1 MiB")
    units_dicom = string_attribute(result, data, "Units", "/image/data", required=False)
    units_ucum = string_attribute(result, data, "units", "/image/data", required=False)
    if (units_dicom is None) != (units_ucum is None):
        result.error("/image/data: only one of the attributes Units and units is present")
    elif units_dicom is None:
        result.warning("/image/data: the unit of the values is unknown (no attributes Units and units)")
    return data.shape[0]


def check_affine(result, dataset, where, shape):
    if not isinstance(dataset, h5py.Dataset):
        result.error("%s is missing" % where)
        return
    if dataset.shape != shape or dataset.dtype != np.dtype("<f8"):
        result.error("%s is no little-endian float64 dataset of the dimensions %s" % (where, list(shape)))
        return
    if string_attribute(result, dataset, "units", where) not in (None, "mm"):
        result.error("%s: units is not mm" % where)
    m = dataset[()].reshape(-1, 3, 4)
    if not np.all(np.isfinite(m)):
        result.error("%s has values that are not finite" % where)
    elif np.any(np.abs(np.linalg.det(m[:, :, :3])) < 1e-12):
        result.error("%s is singular" % where)


def check_volumes(result, f, volumes):
    group = f.get("image/volumes")
    if not isinstance(group, h5py.Group):
        result.error("/image/volumes is missing")
        return
    for name, (kind, unit, required) in VOLUME_DATASETS.items():
        where = "/image/volumes/" + name
        dataset = group.get(name)
        if dataset is None:
            if required:
                result.error("%s is missing" % where)
            continue
        expected = np.dtype("<i4") if kind == "i" else np.dtype("<f8")
        if not isinstance(dataset, h5py.Dataset) or dataset.shape != (volumes,) or dataset.dtype != expected:
            result.error("%s is no %s dataset with one value per volume" % (where, expected.name))
            continue
        if unit is not None and string_attribute(result, dataset, "units", where) not in (None, unit):
            result.error("%s: units is not %s" % (where, unit))
        if kind == "f" and not np.all(np.isfinite(dataset[()])):
            result.error("%s has values that are not finite" % where)
    if "affine" in group:
        check_affine(result, group["affine"], "/image/volumes/affine", (volumes, 3, 4))
        if isinstance(f.get("image/affine"), h5py.Dataset) and f["image/affine"].shape == (3, 4) \
                and group["affine"].shape == (volumes, 3, 4) and not np.array_equal(group["affine"][0], f["image/affine"][()]):
            result.error("/image/affine is not the affine of the first volume")
    for name in group:
        if name not in VOLUME_DATASETS and name != "affine":
            result.warning("/image/volumes/%s is not part of the layout" % name)

    def column(name, default):
        dataset = group.get(name)
        if isinstance(dataset, h5py.Dataset) and dataset.shape == (volumes,):
            return dataset[()]
        return np.full(volumes, default)

    index = np.stack([column("Frame", 1), column("Gate", 1), column("Bed", 0)], axis=1)
    if np.any(index[:, :2] < 1) or np.any(index[:, 2] < 0):
        result.error("/image/volumes: Frame and Gate start with 1, Bed with 0")
    if len(np.unique(index, axis=0)) != volumes:
        result.error("/image/volumes: several volumes have the same frame, gate and bed")
    duration = column("FrameDuration", 0.0)
    if np.any(duration < 0):
        result.error("/image/volumes/FrameDuration has negative values")


def check_header_attribute(result, group, name, where):
    attribute = group.attrs.get_id(name)
    kind = kind_of_vr(vr_of(name))
    if kind is None:
        result.warning("%s: attribute %s is no known DICOM keyword" % (where, name))
        return
    if kind == "group":
        result.error("%s: %s is a code sequence and has to be a group" % (where, name))
        return
    if kind == "s":
        if not check_string(result, "%s: attribute %s" % (where, name), attribute.get_type()):
            return
        values = np.atleast_1d(group.attrs[name])
        vr = vr_of(name)
        for value in values:
            value = text(value)
            if vr == "DT" and not DATETIME.match(value):
                result.error("%s: %s = %r is no ISO 8601 date and time" % (where, name, value))
            elif vr == "DT" and not re.search(r"(Z|[+-]\d{2}:\d{2})$", value):
                result.warning("%s: %s = %r has no UTC offset" % (where, name, value))
            elif vr == "DA" and not DATE.match(value):
                result.error("%s: %s = %r is no date YYYY-MM-DD" % (where, name, value))
    else:
        expected = np.dtype("<f8") if kind == "f" else np.dtype("<i4")
        if attribute.dtype != expected:
            result.error("%s: attribute %s is %s instead of %s" % (where, name, attribute.dtype, expected.name))
    if len(attribute.shape) > 1:
        result.error("%s: attribute %s has more than one dimension" % (where, name))


def typed_header(group):
    """the typed attributes of /header as JSON compatible dict"""
    result = {}
    for name in group.attrs:
        value = group.attrs[name]
        if isinstance(value, np.ndarray):
            result[name] = [text(v) if isinstance(v, (bytes, str)) else v.item() for v in value]
        else:
            result[name] = text(value) if isinstance(value, (bytes, str)) else value.item()
    for name in group:
        if isinstance(group[name], h5py.Group):
            result[name] = {key: text(group[name].attrs[key]) for key in group[name].attrs}
    return result


def check_header(result, f):
    header = f.get("header")
    if not isinstance(header, h5py.Group):
        result.warning("/header is missing")
        return
    for name in header.attrs:
        check_header_attribute(result, header, name, "/header")
    for name in header:
        child = header[name]
        where = "/header/" + name
        if isinstance(child, h5py.Group):
            if kind_of_vr(vr_of(name)) not in ("group", None):
                result.error("%s is no code sequence" % where)
            for key in child.attrs:
                if key not in CODE_ATTRIBUTES:
                    result.warning("%s: attribute %s is not part of the layout" % (where, key))
                else:
                    string_attribute(result, child, key, where)
            if "CodeValue" not in child.attrs and "CodeMeaning" not in child.attrs:
                result.error("%s has neither CodeValue nor CodeMeaning" % where)
        elif name != "json":
            result.warning("%s is not part of the layout" % where)
    for name in RECOMMENDED:
        if name not in header.attrs:
            result.warning("/header: recommended attribute %s is missing" % name)
    for name in RECOMMENDED_GROUPS:
        if name not in header:
            result.warning("/header: recommended group %s is missing" % name)
    data = f.get("image/data")
    if isinstance(data, h5py.Dataset) and "Units" in data.attrs and "Units" in header.attrs \
            and text(data.attrs["Units"]) != text(header.attrs["Units"]):
        result.error("Units of /image/data and of /header differ")

    if "json" in header:
        dataset = header["json"]
        if not isinstance(dataset, h5py.Dataset) or dataset.shape != () \
                or not check_string(result, "/header/json", dataset.id.get_type()):
            result.error("/header/json is no scalar string dataset")
            return
        try:
            attributes = json.loads(text(dataset[()]))
        except ValueError as e:
            result.error("/header/json is no JSON: %s" % e)
            return
        if not isinstance(attributes, dict):
            result.error("/header/json is no JSON object")
            return
        for name, value in typed_header(header).items():
            if name in attributes and attributes[name] != value:
                result.warning("/header: %s = %r differs from the JSON value %r (the typed value is valid)"
                               % (name, value, attributes[name]))


def check(path):
    """checks a file and returns a Result (errors: the file does not follow the layout)"""
    result = Result()
    try:
        f = h5py.File(path, "r")
    except OSError as e:
        result.error("no HDF5 file: %s" % e)
        return result
    with f:
        if not check_root(result, f):
            return result
        volumes = check_data(result, f)
        check_affine(result, f.get("image/affine"), "/image/affine", (3, 4))
        if volumes is not None:
            check_volumes(result, f, volumes)
        check_header(result, f)
    return result


def main(paths):
    failed = False
    for path in paths:
        result = check(path)
        for line in result.warnings:
            print("%s: warning: %s" % (path, line))
        for line in result.errors:
            print("%s: error: %s" % (path, line))
        print("%s: %s" % (path, "FAILED" if result.errors else "ok"))
        failed = failed or bool(result.errors)
    return 1 if failed else 0


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit("usage: check_hdf5.py file.h5 [...]")
    sys.exit(main(sys.argv[1:]))
