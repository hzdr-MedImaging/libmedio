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

cmake_minimum_required(VERSION 3.15...3.28)
project(pmedio LANGUAGES CXX)

# set the cmake modules path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# set the search+install prefix
if(NOT PREFIX_PATH OR PREFIX_PATH STREQUAL "")
  if(MSYS)
    set(PREFIX_PATH "C:/MinGW/msys/1.0/local/petlib")
  else()
    set(PREFIX_PATH "/usr/local/petlib")
  endif()
endif()

if(EXISTS ${PREFIX_PATH})
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${PREFIX_PATH})
  set(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} ${PREFIX_PATH})
else()
  message("WARNING: selected PREFIX path '${PREFIX_PATH}' does not exist. "
          "Select the prefix libmedio is installed in with -DPREFIX_PATH=... "
          "(pip install: environment variable BUILD_PREFIX)")
endif()

message("Using Modules Path: ${CMAKE_MODULE_PATH}")
message("Using Search Path:  ${CMAKE_PREFIX_PATH}")
message("Using Root Path: ${CMAKE_FIND_ROOT_PATH}")

# nanobind needs the Python found by FindPython (select it with -DPython_EXECUTABLE=...).
# Its CMake files are part of the Python package nanobind of this interpreter, unless
# nanobind_DIR is given.
#
# PMEDIO_STABLE_ABI builds the module for the stable ABI of Python (pmedio.abi3.so),
# which works with Python 3.12 and all newer versions. scikit-build-core requests it
# for the wheels (wheel.py-api of pyproject.toml) by setting SKBUILD_SABI_COMPONENT.
if(SKBUILD_SABI_COMPONENT)
  set(PMEDIO_STABLE_ABI_DEFAULT ON)
else()
  set(PMEDIO_STABLE_ABI_DEFAULT OFF)
endif()
option(PMEDIO_STABLE_ABI "build the module for the stable ABI of Python (3.12 or newer)" ${PMEDIO_STABLE_ABI_DEFAULT})

if(PMEDIO_STABLE_ABI)
  find_package(Python 3.12 COMPONENTS Interpreter Development.Module Development.SABIModule REQUIRED)
  set(PMEDIO_MODULE_OPTIONS STABLE_ABI)
else()
  find_package(Python 3.10 COMPONENTS Interpreter Development.Module REQUIRED)
  set(PMEDIO_MODULE_OPTIONS)
endif()
if(NOT nanobind_DIR)
  execute_process(COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
                  OUTPUT_STRIP_TRAILING_WHITESPACE
                  OUTPUT_VARIABLE nanobind_ROOT
                  RESULT_VARIABLE NANOBIND_CMAKE_DIR_RESULT)
  if(NOT NANOBIND_CMAKE_DIR_RESULT EQUAL 0)
    message(FATAL_ERROR "nanobind is not installed for ${Python_EXECUTABLE} (pip install nanobind)")
  endif()
endif()
find_package(nanobind CONFIG REQUIRED)
find_package(MEDIO REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core)
find_package(ZLIB REQUIRED)

# the static libmedio needs the HDF5 library if it was built with HDF5 support
# (libmedio_HAS_HDF5 of its CMake package)
find_package(libmedio CONFIG QUIET)
set(PMEDIO_HDF5_LIBS)
if(libmedio_HAS_HDF5)
  find_package(HDF5 REQUIRED COMPONENTS C)
  set(PMEDIO_HDF5_LIBS ${HDF5_C_LIBRARIES})
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${MEDIO_INCLUDES}
)

# the bindings need C++17 (nanobind), which nanobind_add_module() selects; the
# small nanobind library is built together with and linked into the module
nanobind_add_module(pmedio NB_STATIC ${PMEDIO_MODULE_OPTIONS}
                           src/pmedio.cpp
                           src/medio.cpp
                           src/volume.cpp)

# define link libraries dependencies
target_link_libraries(pmedio PRIVATE
                      Qt5::Core
                      ${MEDIO_STATIC_LIB}
                      ZLIB::ZLIB
                      ${PMEDIO_HDF5_LIBS}
)

# The version of the package (pmedio.__version__) is the one of pyproject.toml:
# scikit-build-core passes it, a plain CMake build reads it from the file.
if(SKBUILD_PROJECT_VERSION)
  set(PMEDIO_VERSION "${SKBUILD_PROJECT_VERSION}")
else()
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/pyproject.toml" PMEDIO_VERSION_LINE
       REGEX "^version = \"[^\"]+\"" LIMIT_COUNT 1)
  string(REGEX REPLACE "^version = \"([^\"]+)\".*" "\\1" PMEDIO_VERSION "${PMEDIO_VERSION_LINE}")
endif()
if(NOT PMEDIO_VERSION)
  message(FATAL_ERROR "no version found in pyproject.toml")
endif()
message("Using pmedio version: ${PMEDIO_VERSION}")
target_compile_definitions(pmedio
  PRIVATE VERSION_INFO=${PMEDIO_VERSION}
)

# the module is the content of the Python package (wheel)
install(TARGETS pmedio LIBRARY DESTINATION .)

# Type stubs of the module for editors and type checkers (mypy, pyright). They are
# generated from the built module, which is imported for it, and installed as the
# stub package pmedio-stubs next to the module (PEP 561: a module that is a single
# file gets its stubs from <name>-stubs/__init__.pyi).
option(PMEDIO_STUBS "generate type stubs (pmedio-stubs) from the built module" ON)
if(PMEDIO_STUBS)
  nanobind_add_stub(pmedio_stubs
    MODULE pmedio
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/pmedio-stubs/__init__.pyi"
    PYTHON_PATH $<TARGET_FILE_DIR:pmedio>
    PATTERN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/pmedio.stubs.pat"
    DEPENDS pmedio)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/pmedio-stubs/__init__.pyi" DESTINATION pmedio-stubs)
endif()

# Set some better optimization options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=generic")
# the warnings are errors for the bindings, but not for the sources of nanobind
target_compile_options(pmedio PRIVATE -Wall -Wextra -Werror)
# Append to CMake's per-configuration defaults so that Release keeps -O3 -DNDEBUG,
# RelWithDebInfo keeps -O2 -g -DNDEBUG and Debug keeps -g (plus -DDEBUG from
# FindRTDEBUG, where used).
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
