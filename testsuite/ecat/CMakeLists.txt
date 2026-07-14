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
add_executable(readwritematrix readwritematrix.cpp)

# define link libraries dependencies
target_link_libraries(readwritematrix Qt5::Core
                                      ${CMAKE_PROJECT_NAME}-static
                                      ${RTDEBUG_STATIC_LIB}
)

# Register as a CTest test; the test writes temporary files into its
# working directory so we point it at the cmake binary output dir.
add_test(NAME ecat_readwritematrix
         COMMAND readwritematrix
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
