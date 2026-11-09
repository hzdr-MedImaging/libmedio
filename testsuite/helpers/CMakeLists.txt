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

add_executable(testBswap testBswap.cpp)

target_link_libraries(testBswap Qt5::Core
                                Qt5::Test
                                ${RTDEBUG_STATIC_LIB}
)

# misaligned accesses abort the test (instead of passing silently on x86)
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_FLAGS "-fsanitize=alignment -fno-sanitize-recover=alignment")
set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=alignment")
check_cxx_source_compiles("int main() { return 0; }" MEDIO_HAVE_ALIGNMENT_SANITIZER)
unset(CMAKE_REQUIRED_FLAGS)
unset(CMAKE_REQUIRED_LINK_OPTIONS)
if(MEDIO_HAVE_ALIGNMENT_SANITIZER)
  target_compile_options(testBswap PRIVATE -fsanitize=alignment -fno-sanitize-recover=alignment)
  target_link_options(testBswap PRIVATE -fsanitize=alignment)
endif()

add_test(NAME bswap_matrix
         COMMAND testBswap
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# benchmark of bswap_matrix(): without sanitizers, with the optimization of the
# build type (only Release builds give meaningful times)
add_executable(testBswapPerformance testBswapPerformance.cpp)

target_link_libraries(testBswapPerformance Qt5::Core
                                           Qt5::Test
                                           ${RTDEBUG_STATIC_LIB}
)

# every benchmark runs once in the test suite (checks it and its results);
# for meaningful times run it without -iterations, see testBswapPerformance.cpp
add_test(NAME bswap_performance
         COMMAND testBswapPerformance -iterations 1
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_tests_properties(bswap_performance PROPERTIES LABELS "performance")
