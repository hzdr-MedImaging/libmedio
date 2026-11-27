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

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# set __MEDIO_PRIVATE__ to flag that we compile the library and
# are no users of medio
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__MEDIO_PRIVATE__")

# specify include directories
include_directories(${RTDEBUG_INCLUDES}
                    ${MEDIO_CONFIG}
                    ${MEDIO_INCLUDES}
)

# specify the executable we are going to build
add_executable(medioconvert medioconvert.cpp)

# define link libraries dependencies
target_link_libraries(medioconvert Qt5::Core
                                   ${CMAKE_PROJECT_NAME}-static
                                   ${RTDEBUG_STATIC_LIB}
)

# set target properties so that install only installs with
# version postfix
set_target_properties(medioconvert PROPERTIES OUTPUT_NAME medioconvert-${PROJECT_VERSION})

# definition of install targets
install(TARGETS medioconvert
        RUNTIME DESTINATION bin
)
medio_install_tool_link(medioconvert)

# the conversions as programs of their own: <source>2<target> are links to medioconvert
# (copies on Windows), which converts as the name it is called with says. ecat2nifti,
# nifti2ecat, philips2nifti and nifti2philips are the programs of examples/ecat2nifti and
# examples/philips2nifti; the names with hdf5 are only installed with HDF5 support.
set(MEDIOCONVERT_SOURCES ecat nifti philips interfile micropet)
set(MEDIOCONVERT_TARGETS ecat nifti philips interfile)
if(MEDIO_HAVE_HDF5)
  list(APPEND MEDIOCONVERT_SOURCES hdf5)
  list(APPEND MEDIOCONVERT_TARGETS hdf5)
endif()
set(MEDIOCONVERT_ALIASES)
foreach(source ${MEDIOCONVERT_SOURCES})
  foreach(target ${MEDIOCONVERT_TARGETS})
    set(alias "${source}2${target}")
    if(NOT source STREQUAL target AND
       NOT alias MATCHES "^(ecat2nifti|nifti2ecat|philips2nifti|nifti2philips)$")
      medio_tool_alias(medioconvert ${alias})
      list(APPEND MEDIOCONVERT_ALIASES ${alias})
    endif()
  endforeach()
endforeach()
set(MEDIOCONVERT_ALIASES ${MEDIOCONVERT_ALIASES} CACHE INTERNAL "names medioconvert is also installed with")

# make sure to bundle all runtime dll libs which the generated
# executable might require using the 'mingw-bundleddlls-wine" tool
if(WIN32)
  find_program(MINGW_BUNDLEDLLS_WINE "mingw-bundledlls-wine")
  if(MINGW_BUNDLEDLLS_WINE)
    install(CODE [[
      execute_process(COMMAND
        mingw-bundledlls-wine ${CMAKE_BINARY_DIR}/bin/$<TARGET_FILE_NAME:medioconvert> --copy $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin
      )
    ]])
  else()
    message("WARNING: cannot bundle DLLs (no 'mingw-bundleddlls-wine' found)")
  endif()
  find_program(MINGW_BUNDLEDLLS "mingw-bundledlls")
  if(MINGW_BUNDLEDLLS)
    install(CODE [[
      execute_process(COMMAND
        mingw-bundledlls $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/$<TARGET_FILE_NAME:medioconvert> --copy
      )
    ]])
  else()
    message("WARNING: cannot bundle DLLs (no 'mingw-bundleddlls' found)")
  endif()
endif()
