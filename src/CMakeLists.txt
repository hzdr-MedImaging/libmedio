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

# configure files where variables should be replaced.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)

# specify include directories
set(MEDIO_LIB_INCLUDES  ${CMAKE_CURRENT_SOURCE_DIR}/ecat
                        ${CMAKE_CURRENT_SOURCE_DIR}/ecat/ecat6
                        ${CMAKE_CURRENT_SOURCE_DIR}/ecat/ecat7
                        ${CMAKE_CURRENT_SOURCE_DIR}/nifti
                        ${CMAKE_CURRENT_SOURCE_DIR}/nifti/nifti1
                        ${CMAKE_CURRENT_SOURCE_DIR}/nifti/nifti2
                        ${CMAKE_CURRENT_SOURCE_DIR}/micropet
                        ${CMAKE_CURRENT_SOURCE_DIR}/philips
                        ${CMAKE_CURRENT_SOURCE_DIR}/helpers
                        ${CMAKE_CURRENT_SOURCE_DIR}/include/rtdebug )

set(MEDIO_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
                   ${CMAKE_CURRENT_SOURCE_DIR}/include/stubs
                   ${MEDIO_LIB_INCLUDES} PARENT_SCOPE)

set(MEDIO_LIB_CONFIG  ${CMAKE_CURRENT_BINARY_DIR}/include
                      ${CMAKE_CURRENT_SOURCE_DIR}/include )

set(MEDIO_CONFIG ${MEDIO_LIB_CONFIG} PARENT_SCOPE)

include_directories(${RTDEBUG_INCLUDES}
                    ${MEDIO_LIB_CONFIG}
                    ${MEDIO_LIB_INCLUDES})

# specify the public includes to be installed as well
set(PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/CMedIO.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/CMedIOHeader.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/CMedIOData.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/CMedIODataFactory.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/CMedIOVolume.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/CMedIOHeaderDictionary.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/ecat/CECATDirectory.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/ecat/CECATDirectoryItem.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/ecat/CECATFile.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/ecat/CECATMainHeader.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/ecat/CECATSubHeader.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/ecat/ecat6/CECAT6MainHeader.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/ecat/ecat7/CECAT7MainHeader.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/ecat/ecat7/CECAT7SubHeaderAttenCorr.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/ecat/ecat7/CECAT7SubHeaderImage.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/ecat/ecat7/CECAT7SubHeaderNorm.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/ecat/ecat7/CECAT7SubHeaderNorm3D.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/ecat/ecat7/CECAT7SubHeaderPolarMap.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/ecat/ecat7/CECAT7SubHeaderScan.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/ecat/ecat7/CECAT7SubHeaderScan3D.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/nifti/CNIFTIMainHeader.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/nifti/nifti1/CNIFTI1MainHeader.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/nifti/nifti2/CNIFTI2MainHeader.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/nifti/CNIFTIFile.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/nifti/CNIFTIConverter.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/micropet/CConcordeMainHeader.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/micropet/CConcordeMainHeaderSinogram.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/micropet/CConcordeMainHeaderImage.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/micropet/CConcordeFrameHeader.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/micropet/CConcordeFile.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/micropet/CConcordeSinogram.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/micropet/CConcordeImage.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/philips/CPhilipsFile.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/philips/CPhilipsListviewHeader.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/philips/CPhilipsBinFile.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/philips/CPhilipsMainHeader.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/philips/CPhilipsDirectory.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/philips/CPhilipsDirectoryItem.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/philips/CPhilipsSubHeader.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/philips/CPhilipsSubHeaderImage.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/philips/CPhilipsSubHeaderSinogram.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/philips/CPhilipsSubHeaderSyntegra.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/philips/CPhilipsSubHeaderListmode.h
                  ${CMAKE_CURRENT_SOURCE_DIR}/helpers/MedIOUnits.h
)

# consider all files in the stubs directory
file(GLOB STUB_FILES LIST_DIRECTORIES false ${CMAKE_CURRENT_SOURCE_DIR}/include/stubs/*)

# consider all *.ccp files found to be actual code files
file(GLOB_RECURSE CODE_FILES *.cpp)

# specify that we are creating a shared and static library at the
# same time
add_library(${CMAKE_PROJECT_NAME}-static STATIC ${CODE_FILES})
add_library(${CMAKE_PROJECT_NAME}-shared SHARED ${CODE_FILES})
set_target_properties(${CMAKE_PROJECT_NAME}-static PROPERTIES OUTPUT_NAME medio 
                                                              VERSION ${PROJECT_VERSION} 
                                                              SOVERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(${CMAKE_PROJECT_NAME}-shared PROPERTIES OUTPUT_NAME medio 
                                                              VERSION ${PROJECT_VERSION} 
                                                              SOVERSION ${PROJECT_VERSION_MAJOR})

# define link libraries dependencies. Within this build tree, all of them are passed
# on to the targets linking the libraries (as before). The CMake package of the
# installation (see below) only passes on what users of the installed libraries need:
# Qt5::Core, whose headers the public headers include, and for the static library
# also the libraries it was linked with (librtdebug is added by libmedioConfig.cmake).
set(MEDIO_BUILD_ONLY_STATIC_LIBS)
foreach(lib ${RTDEBUG_STATIC_LIB} ${DEPEND_STATIC_LIB})
  list(APPEND MEDIO_BUILD_ONLY_STATIC_LIBS "$<BUILD_INTERFACE:${lib}>")
endforeach()
set(MEDIO_BUILD_ONLY_SHARED_LIBS)
foreach(lib Threads::Threads ZLIB::ZLIB ${RTDEBUG_SHARED_LIB} ${DEPEND_STATIC_LIB})
  list(APPEND MEDIO_BUILD_ONLY_SHARED_LIBS "$<BUILD_INTERFACE:${lib}>")
endforeach()

target_link_libraries(${CMAKE_PROJECT_NAME}-static PUBLIC
                      Qt5::Core
                      Threads::Threads
                      ZLIB::ZLIB
                      ${MEDIO_BUILD_ONLY_STATIC_LIBS}
)

target_link_libraries(${CMAKE_PROJECT_NAME}-shared PUBLIC
                      Qt5::Core
                      ${MEDIO_BUILD_ONLY_SHARED_LIBS}
)

# the include directory of the installed public headers for users of the CMake package
target_include_directories(${CMAKE_PROJECT_NAME}-static INTERFACE $<INSTALL_INTERFACE:include/medio>)
target_include_directories(${CMAKE_PROJECT_NAME}-shared INTERFACE $<INSTALL_INTERFACE:include/medio>)
set_target_properties(${CMAKE_PROJECT_NAME}-static PROPERTIES EXPORT_NAME medio-static)
set_target_properties(${CMAKE_PROJECT_NAME}-shared PROPERTIES EXPORT_NAME medio)

# definition of install targets
install(TARGETS ${CMAKE_PROJECT_NAME}-static
        EXPORT libmedioStaticTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        COMPONENT library
)
install(TARGETS ${CMAKE_PROJECT_NAME}-shared
        EXPORT libmedioTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        COMPONENT library
)
install(FILES ${PUBLIC_HEADER} 
              ${STUB_FILES}
        DESTINATION include/medio
)

# CMake package (find_package(libmedio), targets libmedio::medio and
# libmedio::medio-static) and pkg-config file (medio.pc) of the installation. The
# targets of the two libraries are exported separately, so that the package also works
# where only the shared library is installed (e.g. conda-forge).
include(CMakePackageConfigHelpers)
set(MEDIO_CMAKE_INSTALL_DIR lib/cmake/libmedio)
set(MEDIO_LIB_INSTALL_DIR lib)
set(MEDIO_STATIC_LIB_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}medio${CMAKE_STATIC_LIBRARY_SUFFIX})
install(EXPORT libmedioTargets
        NAMESPACE libmedio::
        DESTINATION ${MEDIO_CMAKE_INSTALL_DIR}
        COMPONENT library
)
install(EXPORT libmedioStaticTargets
        NAMESPACE libmedio::
        DESTINATION ${MEDIO_CMAKE_INSTALL_DIR}
        COMPONENT library
)
configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/libmedioConfig.cmake.in
                              ${CMAKE_CURRENT_BINARY_DIR}/libmedioConfig.cmake
                              INSTALL_DESTINATION ${MEDIO_CMAKE_INSTALL_DIR}
                              PATH_VARS MEDIO_LIB_INSTALL_DIR
)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/libmedioConfigVersion.cmake
                                 VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY SameMajorVersion
)
configure_file(${PROJECT_SOURCE_DIR}/cmake/medio.pc.in
               ${CMAKE_CURRENT_BINARY_DIR}/medio.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libmedioConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/libmedioConfigVersion.cmake
        DESTINATION ${MEDIO_CMAKE_INSTALL_DIR}
        COMPONENT library
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/medio.pc
        DESTINATION lib/pkgconfig
        COMPONENT library
)
