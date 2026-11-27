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

# Use of the pmedio interface that is checked with a static type checker
# against the type stubs of the package (pmedio-stubs), e.g.
#
#   mypy --strict python/tests/typing/usage.py
#
# The file is not run and is no test of pytest. The lines marked with
# "type: ignore[...]" are mistakes the stubs have to detect: with
# --warn-unused-ignores (part of --strict) the check fails if they do not.

import os
import pathlib
from typing import Any

import numpy as np
import numpy.typing as npt

import pmedio


def read_write_image(source: str, target: str) -> float:
    img: pmedio.MedIOImage = pmedio.read(source, volumes=[1, 2])
    shape: tuple[int, int, int, int] = img.shape
    weight: Any = img.mhead["patient_weight"]
    frame_starts: list[Any] = [s["frame_start"] for s in img.shead]
    voxel: float = img[0, 0, 0, 0]
    img[0, 0, 0, 0] = voxel + 1.0
    values: npt.NDArray[np.float32] = img.toarray()
    view: npt.NDArray[Any] = np.asarray(img)
    name: str = pmedio.write(img, target, overwrite=True, dataType="short").filename
    written: pmedio.MedIOImage = pmedio.write(values, target, True, "frames", "float", pmedio.Format.ECAT)
    created = pmedio.MedIOImage(np.zeros((2, 3, 4, 5), dtype=np.float32), format="ecat", dimType="frames")
    empty = pmedio.MedIOImage(2, 3, 4, 5)
    copy = pmedio.MedIOImage(created)
    return float(view.sum()) + shape[3] + len(frame_starts) + len(name) + written.size + empty.nbytes + copy.ndim + float(weight)


def convert(source: pathlib.Path, target: "os.PathLike[str]") -> list[str]:
    orientations: list[str] = []
    with pmedio.open(source) as f:
        attributes: dict[str, Any] = f.attributes()
        frames: list[int] = f.frame_numbers()
        with pmedio.create(target, like=f, format=pmedio.Format.NIFTI, overwrite=True) as out:
            for k, frame in enumerate(frames):
                volume: pmedio.Volume = f.read_volume(frame)
                stored: npt.NDArray[Any] = volume.stored()
                dims: tuple[int, int, int] = volume.shape
                position: tuple[float, float, float] = volume.position(0.0, 0.0, 0.0)
                affine: npt.NDArray[np.float64] = volume.affine
                volume.affine = np.eye(4)
                volume.scale_factor = 2.0 * volume.scale_factors[0]
                oblique: bool = volume.reorient("RAS")
                orientations.append(volume.orientation + str(oblique) + str(dims) + str(position) + str(affine.shape))
                out.write_volume(pmedio.Volume(stored, affine=volume.affine, frame_start=volume.frame_start), k + 1)
        orientations.append(str(attributes.get("PatientWeight")) + f.filename + f.format + str(f.writable))
    entry: dict[str, Any] | None = pmedio.dictionary_entry("PatientWeight")
    if entry is not None:
        orientations.append(str(entry["unit"]))
    hdf5: str | None = pmedio.hdf5_version()
    if hdf5 is not None:
        orientations.append(hdf5.split(".")[0])
    return orientations


def create_without_source(target: pathlib.Path) -> dict[str, Any]:
    data: npt.NDArray[np.float32] = np.zeros((4, 4, 2), dtype=np.float32)
    attributes: dict[str, Any] = {"PatientWeight": 72.5, "PatientName": "Test^Name"}
    with pmedio.create(target, frames=2, attributes=attributes, overwrite=True) as out:
        for k in range(2):
            out.write_volume(pmedio.Volume(data, affine=np.eye(3, 4), frame_start=60.0 * k), k + 1)
        written: dict[str, Any] = out.attributes()
    # the context manager does not suppress exceptions: written is always set here
    return written


def load_save(source: str, target: pathlib.Path) -> float:
    image: pmedio.Image = pmedio.load(source, frames=[1, 2], orientation="RAS")
    data: npt.NDArray[Any] = image.data
    affine: npt.NDArray[np.float64] = image.affine
    shape: tuple[int, int, int, int] = image.shape
    timing: list[float] = image.frame_start + image.frame_duration
    attributes: dict[str, Any] = image.attributes
    loaded_from: str | None = image.filename
    file_format: str | None = image.format
    image.data = data * 2.0
    image.affine = np.eye(4)
    image.frame_start = [0.0, 60.0]
    image.frame_duration = (60.0, 60.0)
    image.attributes = {"PatientWeight": 72.5}
    volume: pmedio.Volume = image.volume(0)
    image.save(target, format=pmedio.Format.NIFTI, overwrite=True)
    pmedio.save(target, image, overwrite=True)
    pmedio.save(target, np.zeros((4, 4, 2), dtype=np.int16), overwrite=True)
    with pmedio.open(source) as like:
        pmedio.save(target, pmedio.Image(data, affine=affine, frame_start=timing[:shape[3]]), like=like, overwrite=True)
    return float(len(image)) + len(attributes) + len(str(loaded_from)) + len(str(file_format)) + volume.shape[0]


def nibabel_images(image: pmedio.Image) -> pmedio.Image:
    converted = image.to_nibabel()
    ras = converted.affine  # optional in nibabel
    weight = float(ras[0, 0]) if ras is not None else 0.0
    return pmedio.Image.from_nibabel(converted, frame_start=image.frame_start, frame_duration=[60.0] * len(image),
                                     attributes={"PatientWeight": weight})


def library() -> str:
    major: int = pmedio.MedIO.majorVersion()
    return f"{pmedio.MedIO.name()} {pmedio.MedIO.version()} {major} {pmedio.__version__}"


def mistakes(img: pmedio.MedIOImage, volume: pmedio.Volume, f: pmedio.File) -> None:
    pmedio.read(5)  # type: ignore[arg-type]
    pmedio.read("image.v", volumes="1")  # type: ignore[arg-type]
    pmedio.write(img, "image.v", dataType=5)  # type: ignore[call-overload]
    pmedio.open(5)  # type: ignore[arg-type]
    pmedio.create("image.v", like="other.v")  # type: ignore[arg-type]
    pmedio.create("image.v", frames="2")  # type: ignore[arg-type]
    pmedio.create("image.v", attributes=["PatientWeight"])  # type: ignore[arg-type]
    pmedio.load("image.v", frames=1)  # type: ignore[arg-type]
    pmedio.Image.from_nibabel(np.zeros((2, 2, 2)))  # type: ignore[arg-type]
    pmedio.save("image.v", np.zeros((2, 2, 2)), like="other.v")  # type: ignore[arg-type]
    pmedio.Image(np.zeros((2, 2, 2))).filename = "image.v"  # type: ignore[misc]
    pmedio.Image(np.zeros((2, 2, 2))).frame_start = 0.0  # type: ignore[assignment]
    img.xdim = 5  # type: ignore[misc]
    img.shape = (1, 2, 3, 4)  # type: ignore[misc]
    img[0, 0, 0] = 1.0  # type: ignore[index]
    img.filename = 5  # type: ignore[assignment]
    volume.reorient(5)  # type: ignore[arg-type]
    volume.set_scale_factor("1", 2.0)  # type: ignore[arg-type]
    f.read_volume("1")  # type: ignore[arg-type]
    f.write_volume(img)  # type: ignore[arg-type]
    name: int = f.filename  # type: ignore[assignment]
    del name
    pmedio.missing_function()  # type: ignore[attr-defined]
