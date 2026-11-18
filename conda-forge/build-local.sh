#!/bin/sh
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
# Builds the conda-forge recipes of this directory with rattler-build from local
# checkouts instead of the release archives, to test them before they are submitted
# to conda-forge/staged-recipes or a new version is released.
#
# usage: conda-forge/build-local.sh <librtdebug checkout> [output directory]
#        (default output directory: build-conda-forge in the libmedio checkout)
#
# The recipe of librtdebug is the one of the librtdebug checkout
# (conda.recipe/recipe.yaml), the others are the ones of this directory.
#
# The variants of conda-forge (compilers, C library, pinned versions of Qt, zlib,
# Python, ...) are taken from the package conda-forge-pinning
# (conda install -c conda-forge conda-forge-pinning) in the active environment, or
# from the file given in CONDA_FORGE_PINNING.
#

set -e

if [ $# -lt 1 ] || [ ! -f "$1/conda.recipe/recipe.yaml" ]; then
  echo "usage: $0 <librtdebug checkout> [output directory]" >&2
  exit 1
fi

here=$(cd "$(dirname "$0")" && pwd)
top=$(cd "${here}/.." && pwd)
rtdebug=$(cd "$1" && pwd)
output=${2:-${top}/build-conda-forge}

tmp=$(mktemp -d)
trap 'rm -rf "${tmp}"' EXIT

# the recipes with the local checkouts as source
for name in librtdebug libmedio pmedio r-rmedio; do
  source=${top}
  recipe=${here}/${name}/recipe.yaml
  if [ "${name}" = librtdebug ]; then
    source=${rtdebug}
    recipe=${rtdebug}/conda.recipe/recipe.yaml
  fi
  mkdir -p "${tmp}/${name}"
  sed -e "s|^  url: .*|  path: ${source}|" \
      -e "/^  sha256: /d" \
      -e "/^  # curl -sL /d" \
      "${recipe}" > "${tmp}/${name}/recipe.yaml"
done

pinning=${CONDA_FORGE_PINNING:-${CONDA_PREFIX:-}/conda_build_config.yaml}
if [ ! -f "${pinning}" ]; then
  echo "ERROR: no variant configuration of conda-forge found (${pinning})," >&2
  echo "       install conda-forge-pinning or set CONDA_FORGE_PINNING" >&2
  exit 1
fi

# the channel (conda-forge) is set by the variant configuration (channel_sources)
rattler-build build --recipe-dir "${tmp}" -m "${pinning}" --output-dir "${output}"
