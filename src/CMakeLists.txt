#
# libmedio - C++ I/O Library for loading/saving medical data formats
#            https://github.com/hzdr-MedImaging/libmedio
#
# Copyright (C) 2004-2025 hzdr.de and contributors
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
                        ${CMAKE_CURRENT_SOURCE_DIR}/micropet
                        ${CMAKE_CURRENT_SOURCE_DIR}/philips
                        ${CMAKE_CURRENT_SOURCE_DIR}/helpers
                        ${CMAKE_CURRENT_SOURCE_DIR}/include/rtdebug )

set(MEDIO_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} 
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

# define link libraries dependencies
target_link_libraries(${CMAKE_PROJECT_NAME}-static 
                      Qt5::Core 
                      Threads::Threads
                      ${RTDEBUG_STATIC_LIB}
                      ${DEPEND_STATIC_LIB}
)

target_link_libraries(${CMAKE_PROJECT_NAME}-shared 
                      Qt5::Core 
                      Threads::Threads
                      ${RTDEBUG_SHARED_LIB}
                      ${DEPEND_STATIC_LIB}
)

# definition of install targets
install(TARGETS ${CMAKE_PROJECT_NAME}-static
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        COMPONENT library
)
install(TARGETS ${CMAKE_PROJECT_NAME}-shared
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        COMPONENT library
)
install(FILES ${PUBLIC_HEADER} 
              ${STUB_FILES}
        DESTINATION include/medio
)
