cmake_minimum_required(VERSION 2.8.12)
project(Rmedio)

# set the cmake modules path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# set the search+install prefix
if(NOT PREFIX_PATH)
  if(MSYS)
    set(PREFIX_PATH "C:/MinGW/msys/1.0/local/petlib")
  else()
    set(PREFIX_PATH "/usr/local/petlib")
  endif()
endif()

if(EXISTS ${PREFIX_PATH})
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${PREFIX_PATH})
else()
  message("WARNING: selected PREFIX path '${PREFIX_PATH}' does not exist")
endif()

message("Using Search Path: ${CMAKE_PREFIX_PATH}")

find_package(Qt5 REQUIRED COMPONENTS Core)
find_package(R REQUIRED)
find_package(MEDIO REQUIRED)

if(APPLE)
  # make sure to enable rpath handling for OSX
  set(CMAKE_MACOSX_RPATH 1)
  if(EXISTS /opt/local/libexec/qt5/include)
    set(DEPEND_INCLUDE_DIR "/opt/local/libexec/qt5/include")
  endif()
endif()

message("R libraries: ${R_LIBRARIES}")
message("R include dirs: ${R_INCLUDE_DIR} ${Rcpp_INCLUDE_DIR}")

include(GenerateExportHeader)
include_directories(${CMAKE_CURRENT_BINARY_DIR}
                    ${MEDIO_INCLUDES}
                    ${R_INCLUDE_DIR}
                    ${Rcpp_INCLUDE_DIR}
                    ${DEPEND_INCLUDE_DIR}
)

add_library(Rmedio SHARED src/CRECATFile.cpp src/readEcat.cpp src/writeEcat.cpp)
target_link_libraries(Rmedio
                      Qt5::Core
                      ${MEDIO_STATIC_LIB}
                      ${R_LIBRARIES}
)

generate_export_header(Rmedio)

install(TARGETS Rmedio DESTINATION lib)
