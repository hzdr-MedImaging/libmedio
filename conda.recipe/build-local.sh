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
# Builds the conda-forge recipes of libmedio (conda.recipe/recipe.yaml), pmedio
# (python/conda.recipe/recipe.yaml) and r-rmedio (r/conda.recipe/recipe.yaml) with
# rattler-build from local checkouts instead of the release archives, to test them
# before they are submitted to conda-forge/staged-recipes or a new version is
# released, or to build packages for a local channel.
#
# usage: conda.recipe/build-local.sh <librtdebug checkout or channel> [output directory]
#        (default output directory: build-conda in the libmedio checkout)
#
# librtdebug, which libmedio needs for building, comes from
# - a librtdebug checkout: its recipe (conda.recipe/recipe.yaml) is built as well, or
# - a conda channel holding the librtdebug package, e.g. the extracted archive of a
#   librtdebug release (librtdebug-<version>-conda-<platform>.tar.gz): its packages are
#   copied into the output directory, which is a channel of the build.
#
# The variants of conda-forge (compilers, C library, pinned versions of Qt, zlib,
# Python, ...) are taken from the package conda-forge-pinning
# (conda install -c conda-forge conda-forge-pinning) in the active environment, or
# from the file given in CONDA_FORGE_PINNING.
#

set -e

if [ $# -lt 1 ] || [ ! -d "$1" ]; then
  echo "usage: $0 <librtdebug checkout or channel> [output directory]" >&2
  exit 1
fi

here=$(cd "$(dirname "$0")" && pwd)
top=$(cd "${here}/.." && pwd)
rtdebug=$(cd "$1" && pwd)
output=${2:-${top}/build-conda}

names="libmedio pmedio r-rmedio"
if [ -f "${rtdebug}/conda.recipe/recipe.yaml" ]; then
  names="librtdebug ${names}"
elif [ -f "${rtdebug}/noarch/repodata.json" ]; then
  echo "* librtdebug from the channel ${rtdebug}"
  mkdir -p "${output}"
  cp -R "${rtdebug}/." "${output}/"
else
  echo "ERROR: ${rtdebug} is neither a librtdebug checkout (conda.recipe/recipe.yaml)" >&2
  echo "       nor a conda channel (noarch/repodata.json)" >&2
  exit 1
fi

tmp=$(mktemp -d)
trap 'rm -rf "${tmp}"' EXIT

# the recipes with the local checkouts as source
for name in ${names}; do
  source=${top}
  case ${name} in
    librtdebug) source=${rtdebug}; recipe=${rtdebug}/conda.recipe/recipe.yaml ;;
    libmedio)   recipe=${top}/conda.recipe/recipe.yaml ;;
    pmedio)     recipe=${top}/python/conda.recipe/recipe.yaml ;;
    r-rmedio)   recipe=${top}/r/conda.recipe/recipe.yaml ;;
  esac
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
