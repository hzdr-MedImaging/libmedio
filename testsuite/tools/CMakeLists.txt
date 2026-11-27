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

# ecat2nifti/nifti2ecat and philips2nifti/nifti2philips, which were programs of their own
# and are names of medioConvert now (see examples/medioConvert, cmake/MedioToolLink.cmake):
# their help and conversions in both directions
add_test(NAME converter_names
         COMMAND ${CMAKE_COMMAND}
                 -DECAT2NIFTI=$<TARGET_FILE_DIR:medioConvert>/ecat2nifti-${PROJECT_VERSION}${CMAKE_EXECUTABLE_SUFFIX}
                 -DPHILIPS2NIFTI=$<TARGET_FILE_DIR:medioConvert>/philips2nifti-${PROJECT_VERSION}${CMAKE_EXECUTABLE_SUFFIX}
                 -DDATA=${MEDIO_TEST_DATA_DIR}
                 -DWORK=${CMAKE_CURRENT_BINARY_DIR}/converter_names
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/ConverterNames.cmake
)

# medioConvert and the names <source>2<target> it is installed with, run on the test data
include_directories(${RTDEBUG_INCLUDES}
                    ${MEDIO_CONFIG}
                    ${MEDIO_INCLUDES}
)
add_executable(testMedioConvert testMedioConvert.cpp)
add_dependencies(testMedioConvert medioConvert)
target_include_directories(testMedioConvert PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
target_compile_definitions(testMedioConvert PRIVATE __MEDIO_PRIVATE__
                                                    MEDIO_TEST_DATA_DIR="${MEDIO_TEST_DATA_DIR}"
                                                    MEDIO_TOOL_DIR="$<TARGET_FILE_DIR:medioConvert>"
                                                    MEDIO_TOOL_VERSION="${PROJECT_VERSION}")
target_link_libraries(testMedioConvert Qt5::Core
                                       Qt5::Test
                                       ZLIB::ZLIB
                                       ${CMAKE_PROJECT_NAME}-static
                                       ${RTDEBUG_STATIC_LIB}
)
add_test(NAME medioConvert
         COMMAND testMedioConvert
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
