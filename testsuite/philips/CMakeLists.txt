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

add_executable(testPhilipsDatasets testPhilipsDatasets.cpp)

target_include_directories(testPhilipsDatasets PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
target_compile_definitions(testPhilipsDatasets PRIVATE MEDIO_TEST_DATA_DIR="${MEDIO_TEST_DATA_DIR}")

target_link_libraries(testPhilipsDatasets Qt5::Core
                                          Qt5::Test
                                          ${CMAKE_PROJECT_NAME}-static
                                          ${RTDEBUG_STATIC_LIB}
)

add_test(NAME philips_datasets
         COMMAND testPhilipsDatasets
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(testPhilipsMatrix testPhilipsMatrix.cpp)

target_include_directories(testPhilipsMatrix PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)
target_compile_definitions(testPhilipsMatrix PRIVATE MEDIO_TEST_DATA_DIR="${MEDIO_TEST_DATA_DIR}")

target_link_libraries(testPhilipsMatrix Qt5::Core
                                        Qt5::Test
                                        ${CMAKE_PROJECT_NAME}-static
                                        ${RTDEBUG_STATIC_LIB}
)

add_test(NAME philips_matrix
         COMMAND testPhilipsMatrix
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
