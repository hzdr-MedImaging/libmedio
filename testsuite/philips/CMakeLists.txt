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

# Qt5::Test is required for the QtTest framework (QTest::qExec, QVERIFY, …);
# the component is requested in the top-level find_package(Qt5 …) call.

# specify include directories
include_directories(${RTDEBUG_INCLUDES}
                    ${MEDIO_CONFIG}
                    ${MEDIO_INCLUDES}
)

# collect all source files
file(GLOB_RECURSE CODE_FILES *.cpp)
list(REMOVE_DUPLICATES CODE_FILES)

# specify the executable we are going to build
add_executable(testPhilipsFormat ${CODE_FILES})

# define link libraries dependencies
target_link_libraries(testPhilipsFormat Qt5::Core
                                        Qt5::Test
                                        ${CMAKE_PROJECT_NAME}-static
                                        ${RTDEBUG_STATIC_LIB}
)

# Optional CTest registration: set PHILIPS_TEST_FILE to the path of a
# Philips PET/Syntegra data file to enable the test.
#
# Example:
#   cmake -DPHILIPS_TEST_FILE=/path/to/scan.hdr ...
#   make && ctest -R philips
#
set(PHILIPS_TEST_FILE "" CACHE FILEPATH
    "Path to a Philips PET/Syntegra data file used by the philips_format CTest")

if(PHILIPS_TEST_FILE)
  add_test(NAME philips_format
           COMMAND testPhilipsFormat ${PHILIPS_TEST_FILE}
  )
else()
  message(STATUS "PHILIPS_TEST_FILE not set — testPhilipsFormat will be built but not registered as a CTest test")
endif()
