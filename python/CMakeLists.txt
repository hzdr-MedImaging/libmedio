cmake_minimum_required(VERSION 3.4...3.18)
project(pmedio)

# set the cmake modules path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# set the search+install prefix
if(NOT PREFIX_PATH OR PREFIX_PATH STREQUAL "")
  if(MSYS)
    set(PREFIX_PATH "C:/MinGW/msys/1.0/local/petlib")
  else()
    set(PREFIX_PATH "/usr/local/petlib")
  endif()
endif()

if(EXISTS ${PREFIX_PATH})
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${PREFIX_PATH})
  set(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} ${PREFIX_PATH})
else()
  message("WARNING: selected PREFIX path '${PREFIX_PATH}' does not exist")
endif()

message("Using Modules Path: ${CMAKE_MODULE_PATH}")
message("Using Search Path:  ${CMAKE_PREFIX_PATH}")
message("Using Root Path: ${CMAKE_FIND_ROOT_PATH}")

#set(pybind11_DIR "/usr/local/lib/python3.10/dist-packages/pybind11/share/cmake/pybind11")

find_package(pybind11 REQUIRED)
find_package(MEDIO REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core)

include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${MEDIO_INCLUDES}
)

pybind11_add_module(pmedio src/pmedio.cpp
                           src/medio.cpp)

# define link libraries dependencies
target_link_libraries(pmedio PRIVATE
                      Qt5::Core
                      ${MEDIO_STATIC_LIB}
)

# EXAMPLE_VERSION_INFO is defined by setup.py and passed into the C++ code as a
# define (VERSION_INFO) here.
target_compile_definitions(pmedio
  PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO}
)

# Set some better optimization options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=generic -std=c++11 -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_RELEASE, "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO, "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3 -g -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG, "${CMAKE_CXX_FLAGS_RELEASE} -O0")
