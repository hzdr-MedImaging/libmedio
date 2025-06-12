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

set(PROJECT_LONGNAME "image2ecat")
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 1)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# set __MEDIO_PRIVATE__ to flag that we compile the library and
# are no users of medio
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__MEDIO_PRIVATE__")

# configure files where variables should be replaced.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/includes/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/includes/config.h)

# specify include directories
include_directories(${CMAKE_CURRENT_BINARY_DIR}/includes
                    ${CMAKE_CURRENT_SOURCE_DIR}/includes
                    ${RTDEBUG_INCLUDES}
                    ${MEDIO_INCLUDES}
)

# consider all *.ccp files found to be actual code files
file(GLOB_RECURSE CODE_FILES *.cpp)
list(REMOVE_DUPLICATES CODE_FILES)


# specify the executable we are going to build
add_executable(image2ecat ${CODE_FILES})

# define link libraries dependencies
target_link_libraries(image2ecat  Qt5::Core 
                                  ${CMAKE_PROJECT_NAME}-static
                                  ${RTDEBUG_STATIC_LIB} 
)

# set target properties so that install only installs with
# version postfix
set_target_properties(image2ecat PROPERTIES OUTPUT_NAME image2ecat-${PROJECT_VERSION})

# definition of install targets
install(TARGETS image2ecat
        RUNTIME DESTINATION bin
)

# make sure to bundle all runtime dll libs which the generated
# executable might require using the 'mingw-bundleddlls-wine" tool
if(WIN32)
  find_program(MINGW_BUNDLEDLLS_WINE "mingw-bundledlls-wine")
  if(MINGW_BUNDLEDLLS_WINE)
    install(CODE [[
      execute_process(COMMAND
        mingw-bundledlls-wine ${CMAKE_BINARY_DIR}/bin/$<TARGET_FILE_NAME:image2ecat> --copy $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin
      )
    ]])
  else()
    message("WARNING: cannot bundle DLLs (no 'mingw-bundleddlls-wine' found)")
  endif()
  find_program(MINGW_BUNDLEDLLS "mingw-bundledlls")
  if(MINGW_BUNDLEDLLS)
    install(CODE [[
      execute_process(COMMAND
        mingw-bundledlls $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/$<TARGET_FILE_NAME:image2ecat> --copy
      )
    ]])
  else()
    message("WARNING: cannot bundle DLLs (no 'mingw-bundleddlls' found)")
  endif()
endif()
