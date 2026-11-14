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
add_executable(testNIFTIFormat testNIFTIFormat.cpp)

target_include_directories(testNIFTIFormat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)

# reference NIfTI files (written with nibabel, see data/generate_fixtures.py)
target_compile_definitions(testNIFTIFormat PRIVATE
                           NIFTI_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

# define link libraries dependencies
target_link_libraries(testNIFTIFormat Qt5::Core
                                      Qt5::Test
                                      ZLIB::ZLIB
                                      ${CMAKE_PROJECT_NAME}-static
                                      ${RTDEBUG_STATIC_LIB}
)

# Register as a CTest test; temporary files are created in the system
# temporary directory.
add_test(NAME nifti_format
         COMMAND testNIFTIFormat
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(testNIFTIConversion testNIFTIConversion.cpp)

target_include_directories(testNIFTIConversion PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
target_compile_definitions(testNIFTIConversion PRIVATE MEDIO_TEST_DATA_DIR="${MEDIO_TEST_DATA_DIR}")

target_link_libraries(testNIFTIConversion Qt5::Core
                                          Qt5::Test
                                          ZLIB::ZLIB
                                          ${CMAKE_PROJECT_NAME}-static
                                          ${RTDEBUG_STATIC_LIB}
)

add_test(NAME nifti_conversion
         COMMAND testNIFTIConversion
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# conversion of an image larger than 2 GB: needs about 3.3 GB of temporary
# disk space and 1.5 GB of memory (ctest -LE large skips it)
add_executable(testNIFTILargeImage testNIFTILargeImage.cpp)

target_link_libraries(testNIFTILargeImage Qt5::Core
                                          Qt5::Test
                                          ZLIB::ZLIB
                                          ${CMAKE_PROJECT_NAME}-static
                                          ${RTDEBUG_STATIC_LIB}
)

add_test(NAME nifti_large_image
         COMMAND testNIFTILargeImage
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_tests_properties(nifti_large_image PROPERTIES LABELS large)

add_executable(testNIFTIDatasets testNIFTIDatasets.cpp)

target_include_directories(testNIFTIDatasets PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
target_compile_definitions(testNIFTIDatasets PRIVATE MEDIO_TEST_DATA_DIR="${MEDIO_TEST_DATA_DIR}")

target_link_libraries(testNIFTIDatasets Qt5::Core
                                        Qt5::Test
                                        ZLIB::ZLIB
                                        ${CMAKE_PROJECT_NAME}-static
                                        ${RTDEBUG_STATIC_LIB}
)

add_test(NAME nifti_datasets
         COMMAND testNIFTIDatasets
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
