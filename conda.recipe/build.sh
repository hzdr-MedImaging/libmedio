#!/bin/bash
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

set -e

# CMAKE_ARGS of conda-build selects the compilers and the search paths of the
# host environment; libmedio is installed into PREFIX_PATH
# (AR and RANLIB are given with their full path: CMAKE_ARGS can name them
# without directory, which CMake takes relative to the working directory)
cmake -S . -B build-conda ${CMAKE_ARGS} \
      -DCMAKE_AR="$(command -v "${AR}")" -DCMAKE_RANLIB="$(command -v "${RANLIB}")" \
      -DCMAKE_BUILD_TYPE=Release -DPREFIX_PATH="${PREFIX}"
cmake --build build-conda --parallel "${CPU_COUNT:-2}"
cmake --install build-conda
