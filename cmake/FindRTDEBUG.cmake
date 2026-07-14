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

# - Try to find RTDEBUG
# Once done this will define
#
#  RTDEBUG_FOUND      - True if RTDEBUG was found.
#  RTDEBUG_INCLUDES   - Where to find CMedio.h, etc.
#  RTDEBUG_STATIC_LIB - List of static libraries when using RTDEBUG
#  RTDEBUG_SHARED_LIB - List of shared libraries when using RTDEBUG
#  RTDEBUG_DEFINITIONS - Compiler switches required for using RTDEBUG

if(RTDEBUG_INCLUDES)
  set(RTDEBUG_FIND_QUIETLY TRUE)
endif(RTDEBUG_INCLUDES)

find_path(RTDEBUG_INCLUDES CRTDebug.h
          PATH_SUFFIXES include/rtdebug
          NO_CMAKE_SYSTEM_PATH
)

find_library(RTDEBUG_STATIC_LIB NAMES librtdebug${CMAKE_STATIC_LIBRARY_SUFFIX}
             PATH_SUFFIXES lib
             NO_CMAKE_SYSTEM_PATH
)
find_library(RTDEBUG_SHARED_LIB NAMES librtdebug${CMAKE_SHARED_LIBRARY_SUFFIX} rtdebug
             PATH_SUFFIXES lib
             NO_CMAKE_SYSTEM_PATH
)

# set the global DEBUG define while compiling
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

# handle the QUIETLY and REQUIRED arguments and set RTDEBUG_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
set(RTDEBUG_LIBS ${RTDEBUG_STATIC_LIB} ${RTDEBUG_SHARED_LIB})
find_package_handle_standard_args(RTDEBUG DEFAULT_MSG RTDEBUG_LIBS RTDEBUG_INCLUDES)

mark_as_advanced(RTDEBUG_STATIC_LIB RTDEBUG_SHARED_LIB RTDEBUG_INCLUDES)
