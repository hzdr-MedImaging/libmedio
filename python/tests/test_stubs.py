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

# Tests of the type stubs of pmedio (pmedio-stubs/__init__.pyi), which are
# generated from the module when it is built: the stubs are valid Python and
# declare everything the module provides. That they describe the interface
# correctly is checked with a type checker and python/tests/typing/usage.py.
# The tests are skipped if the module was built without stubs
# (-DPMEDIO_STUBS=OFF).

import ast
import inspect
import pathlib

import pytest

import pmedio

STUBS = pathlib.Path(pmedio.__file__).parent / "pmedio-stubs" / "__init__.pyi"

needs_stubs = pytest.mark.skipif(not STUBS.is_file(), reason="built without type stubs")


def declared(body):
    """Names declared by the statements of a module or class of the stubs"""
    names = {}
    for node in body:
        if isinstance(node, (ast.FunctionDef, ast.ClassDef)):
            names.setdefault(node.name, node)
        elif isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name):
            names.setdefault(node.target.id, node)
        elif isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name):
                    names.setdefault(target.id, node)
    return names


def public(obj):
    return sorted(name for name in vars(obj) if not name.startswith("_"))


@needs_stubs
def test_stubs_declare_the_module():
    module = declared(ast.parse(STUBS.read_text(encoding="utf-8")).body)

    # functions, classes and constants of the module
    assert public(pmedio)
    for name in public(pmedio) + ["__version__"]:
        assert name in module, name

    # members of the classes, including the nested classes with the constants
    classes = [(pmedio.MedIO, module["MedIO"]), (pmedio.MedIOImage, module["MedIOImage"]),
               (pmedio.Volume, module["Volume"]), (pmedio.File, module["File"]), (pmedio.Image, module["Image"])]
    for cls, node in classes:
        members = declared(node.body)
        assert public(cls), cls
        for name in public(cls):
            assert name in members, f"{cls.__name__}.{name}"
            if inspect.isclass(getattr(cls, name)):
                nested = declared(members[name].body)
                for constant in public(getattr(cls, name)):
                    assert constant in nested, f"{cls.__name__}.{name}.{constant}"
    for name in ("__init__", "__getitem__", "__setitem__"):
        assert name in declared(module["MedIOImage"].body), name
    for name in ("__enter__", "__exit__"):
        assert name in declared(module["File"].body), name


@needs_stubs
def test_stubs_are_precise():
    # the signatures that are given explicitly, as the generated ones only say "object"
    text = STUBS.read_text(encoding="utf-8")
    for signature in (
        "def open(path: str | os.PathLike[str]) -> File:",
        "def create(path: str | os.PathLike[str], like: File | None = None, format: str | None = None, "
        "overwrite: bool | None = False, frames: int | None = None, "
        "attributes: dict[str, Any] | None = None) -> File:",
        "def dictionary_entry(keyword_or_tag: str) -> dict[str, Any] | None:",
        "def load(path: str | os.PathLike[str], frames: Sequence[int] | None = None, "
        "orientation: str | None = None) -> Image:",
        "def save(path: str | os.PathLike[str], image: Image | numpy.typing.ArrayLike, like: File | None = None, "
        "format: str | None = None, overwrite: bool | None = False) -> None:",
        "def frame_start(self, values: Sequence[float], /) -> None:",
        "def __init__(self, img: numpy.typing.ArrayLike, format: str = 'ecat', dimType: str = 'frames') -> None:",
        "def shape(self, /) -> tuple[int, int, int, int]:",
        "def mhead(self, /) -> dict[str, Any]:",
        "def shead(self, /) -> list[dict[str, Any]]:",
        "def stored(self) -> numpy.typing.NDArray[Any]:",
        "def attributes(self) -> dict[str, Any]:",
        "def __enter__(self) -> File:",
        "__version__: str",
    ):
        assert signature in text, signature
    assert "-> object" not in text
