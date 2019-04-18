#/* -*- mode: c++; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil; -*-
# * vim:set ts=2 sw=2 expandtab: *********************************************
# *
# * libmedio - C++ I/O Library for loading/saving medical data formats
# * Copyright (C) 2006-2015 Jens Maus <mail@jens-maus.de>
# *
# * This library is free software; you can redistribute it and/or
# * modify it under the terms of the GNU Lesser General Public
# * License as published by the Free Software Foundation; either
# * version 2.1 of the License, or (at your option) any later version.
# *
# * This library is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# * Lesser General Public License for more details.
# *
# * You should have received a copy of the GNU Lesser General Public
# * License along with this library; if not, write to the Free Software
# * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
# *
# * $Id: README 496 2010-01-11 09:45:23Z langner $
# *
# **************************************************************************/

cmake_minimum_required(VERSION 2.8.11)

# Default configuration values. These must be before the project command or
# they won't work in Windows.
# If no build type is specified, default to "Release"
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
      "None Debug Release RelWithDebInfo MinSizeRel"
      FORCE)
endif()

# define the project and the version number
project(libmedio)
set(PROJECT_LONGNAME "libmedio")
set(PROJECT_VERSION_MAJOR 3)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 1)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

# change output directory to top-level 'bin' dir.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Global CMake options
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

# set the cmake modules path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# In case we are cross compiling we have to setup
# everything for different search paths
if(CMAKE_CROSS_COMPILING)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
endif()

# set the search+install prefix
if(NOT PREFIX_PATH)
  if(MSYS)
    set(PREFIX_PATH "C:/MinGW/msys/1.0/local/petlib")
  else()
    set(PREFIX_PATH "/usr/local/petlib")
  endif()
endif()

if(EXISTS ${PREFIX_PATH})
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${PREFIX_PATH})
  set(CMAKE_INSTALL_PREFIX ${PREFIX_PATH})
else()
  message("WARNING: selected PREFIX path '${PREFIX_PATH}' does not exist")
endif()

message("Using Search Path: ${CMAKE_PREFIX_PATH}")
message("Using Install Prefix: ${CMAKE_INSTALL_PREFIX}")

# Configure third party packages
find_package(Qt5 REQUIRED COMPONENTS Core)
find_package(Threads REQUIRED)
find_package(RTDEBUG REQUIRED)

# add some special libraries in case we are build for windows
# WARNING: This seems to be a workaround because Qt5 itself should
# include them?!?!
if(WIN32 AND CMAKE_CROSS_COMPILING)
  if(${CMAKE_C_COMPILER} MATCHES "static-gcc")
    set(DEPEND_STATIC_LIB ${Qt5_STATIC_LIB_DEPENDENCIES})
    set(RTDEBUG_SHARED_LIB ${RTDEBUG_STATIC_LIB})
  endif()
endif()

# Set some better optimization options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=generic -std=c++11 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE, "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO, "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3 -g -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG, "${CMAKE_CXX_FLAGS_RELEASE} -O0")

# enable @rpath in the install name for any shared library being built
# note: it is planned that a future version of CMake will enable this by default
set(CMAKE_MACOSX_RPATH 1)

add_subdirectory(src)
add_subdirectory(examples/img2ecat)
add_subdirectory(examples/bin2txt)
add_subdirectory(examples/ecat/assembleBeds)
add_subdirectory(examples/ecat/ecatutil)
add_subdirectory(examples/ecat/ecatCheck)
