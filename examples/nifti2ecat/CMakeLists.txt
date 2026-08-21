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

set(PROJECT_LONGNAME "nifti2ecat")
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
# version 1.0.0: initial release => nifti2ecat-1.0.0

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# set __MEDIO_PRIVATE__ to flag that we compile the library and
# are no users of medio
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__MEDIO_PRIVATE__")   



# specify include directories
include_directories(
                    ${RTDEBUG_INCLUDES}
                    ${MEDIO_INCLUDES}
)

# consider all *.ccp files found to be actual code files
file(GLOB_RECURSE CODE_FILES *.cpp)
list(REMOVE_DUPLICATES CODE_FILES)

# specify the executable we are going to build
# Executable creation passing all *.cpp files found in the current directory to the add_executable command
add_executable(nifti2ecat nifti2ecat.cpp)

# define link libraries dependencies
target_link_libraries(nifti2ecat  Qt5::Core 
                                ${CMAKE_PROJECT_NAME}-static
                                ${RTDEBUG_STATIC_LIB} 
)

# set target properties so that install only installs with
# version postfix
set_target_properties(nifti2ecat PROPERTIES OUTPUT_NAME nifti2ecat-${PROJECT_VERSION})

# definition of install targets
install(TARGETS nifti2ecat 
        RUNTIME DESTINATION bin
)

# make sure to bundle all runtime dll libs which the generated 
# executable might require using the 'mingw-bundleddlls-wine" tool
if(WIN32)
    find_program(MINGW_BUNDLE_DLLS_EXECUTABLE mingw-bundleddlls-wine)
    if(MINGW_BUNDLE_DLLS_EXECUTABLE)
        install(CODE [[
            execute_process(COMMAND 
            mingw-bundleddlls-wine ${CMAKE_BINARY_DIR}/bin/$<TARGET_FILE_NAME:nifti2ecat> --copy $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin
            )
        ]])
    else()
      message(WARNING "mingw-bundleddlls-wine not found, unable to bundle runtime dlls for nifti2ecat executable")
    endif()

    find_program(MINGW_BUNDLEDLLS "mingw-bundledlls")
    if(MINGW_BUNDLEDLLS)
        install(CODE [[
            execute_process(COMMAND
            mingw-bundledlls $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/$<TARGET_FILE_NAME:nifti2ecat> --copy
            )
        ]])
    else()
        message(WARNING "mingw-bundledlls not found, unable to bundle runtime dlls for nifti2ecat executable")
    endif()
endif()