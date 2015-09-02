#/* -*- mode: c++; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil; -*-
# * vim:set ts=2 sw=2 expandtab: *********************************************
# *
# * libmedio - C++ I/O Library for loading/saving medical data formats
# * Copyright (C) 2006-2015 Jens Maus <mail@jens-maus.de>
# *
# * This library is free software; you can redistribute it and/or
# * modify it under the terms of the GNU Lesser General Public
# * License as published by the Free Software Foundation; either
# * version 2.1 of the License, or (at your option) any later version.
# *
# * This library is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# * Lesser General Public License for more details.
# *
# * You should have received a copy of the GNU Lesser General Public
# * License along with this library; if not, write to the Free Software
# * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
# *
# * $Id: README 496 2010-01-11 09:45:23Z langner $
# *
# **************************************************************************/

# set __MEDIO_PRIVATE__ to flag that we compile the library and
# are no users of medio
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__MEDIO_PRIVATE__")

# configure files where variables should be replaced.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)

# specify include directories
set(MEDIO_LIB_INCLUDES  ${CMAKE_CURRENT_BINARY_DIR}/include
                        ${CMAKE_CURRENT_SOURCE_DIR}/include
                        ${CMAKE_CURRENT_SOURCE_DIR}/ecat
                        ${CMAKE_CURRENT_SOURCE_DIR}/ecat/ecat6
                        ${CMAKE_CURRENT_SOURCE_DIR}/ecat/ecat7
                        ${CMAKE_CURRENT_SOURCE_DIR}/micropet
                        ${CMAKE_CURRENT_SOURCE_DIR}/philips
                        ${CMAKE_CURRENT_SOURCE_DIR}/helpers
                        ${CMAKE_CURRENT_SOURCE_DIR}/include/rtdebug )

set(MEDIO_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} 
                   ${MEDIO_LIB_INCLUDES} PARENT_SCOPE)

include_directories(${MEDIO_LIB_INCLUDES}
                    ${RTDEBUG_INCLUDES}
)

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
file(GLOB STUB_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/stubs/*)

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
                      ${RTDEBUG_STATIC_LIB}
)

target_link_libraries(${CMAKE_PROJECT_NAME}-shared 
                      Qt5::Core 
                      ${RTDEBUG_SHARED_LIB}
                      ${DEPEND_STATIC_LIB}
)

# definition of install targets
install(TARGETS ${CMAKE_PROJECT_NAME}-static
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        COMPONENT library
)
install(TARGETS ${CMAKE_PROJECT_NAME}-shared
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        COMPONENT library
)
install(FILES ${PUBLIC_HEADER} 
              ${STUB_FILES}
        DESTINATION include/medio
)
