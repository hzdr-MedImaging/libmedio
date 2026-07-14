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

# - Try to find COREG
# Once done this will define
#
#  COREG_FOUND      - True if COREG was found.
#  COREG_INCLUDES   - Where to find coreg.hh, dataset.hh, basics.hh, etc.
#  COREG_STATIC_LIB - Static library for linking against COREG
#  COREG_SHARED_LIB - Shared library for linking against COREG

if(COREG_INCLUDES)
  set(COREG_FIND_QUIETLY TRUE)
endif(COREG_INCLUDES)

find_path(COREG_INCLUDES coreg.hh
          PATH_SUFFIXES include/coreg
          NO_CMAKE_SYSTEM_PATH
)

find_library(COREG_STATIC_LIB NAMES libcoreg${CMAKE_STATIC_LIBRARY_SUFFIX}
             PATH_SUFFIXES lib
             NO_CMAKE_SYSTEM_PATH
)
find_library(COREG_SHARED_LIB NAMES libcoreg${CMAKE_SHARED_LIBRARY_SUFFIX} coreg
             PATH_SUFFIXES lib
             NO_CMAKE_SYSTEM_PATH
)

# handle the QUIETLY and REQUIRED arguments and set COREG_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
set(COREG_LIBS ${COREG_STATIC_LIB} ${COREG_SHARED_LIB})
find_package_handle_standard_args(COREG DEFAULT_MSG COREG_LIBS COREG_INCLUDES)

mark_as_advanced(COREG_STATIC_LIB COREG_SHARED_LIB COREG_INCLUDES)
