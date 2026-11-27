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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__MEDIO_PRIVATE__")

include_directories(${RTDEBUG_INCLUDES}
                    ${MEDIO_CONFIG}
                    ${MEDIO_INCLUDES}
)

add_executable(testHDF5File testHDF5File.cpp)
target_include_directories(testHDF5File PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
target_compile_definitions(testHDF5File PRIVATE MEDIO_TEST_DATA_DIR="${MEDIO_TEST_DATA_DIR}")
if(MEDIO_HAVE_HDF5)
  # the test expects the HDF5 support the build was configured with
  target_compile_definitions(testHDF5File PRIVATE MEDIO_TEST_HAVE_HDF5 MEDIO_TEST_HDF5_VERSION="${HDF5_VERSION}")
endif()
target_link_libraries(testHDF5File Qt5::Core
                                   Qt5::Test
                                   ZLIB::ZLIB
                                   ${CMAKE_PROJECT_NAME}-static
                                   ${RTDEBUG_STATIC_LIB}
)
add_test(NAME hdf5_file
         COMMAND testHDF5File
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
