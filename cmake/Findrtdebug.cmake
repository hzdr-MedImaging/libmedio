# - Try to find librtdebug
# Once done this will define
#  LIBRTDEBUG_FOUND - System has librtdebug
#  LIBRTDEBUG_INCLUDE_DIRS - The librtdebug include directories
#  LIBRTDEBUG_LIBRARIES - The libraries needed to use librtdebug
#  LIBRTDEBUG_DEFINITIONS - Compiler switches required for using librtdebug

# only search for librtdebug in debug mode
if(CMAKE_BUILD_TYPE MATCHES Debug)

  message(STATUS "Searching for librtdebug package...")

  find_path(LIBRTDEBUG_INCLUDE_DIR CRTDebug.h
            PATH_SUFFIXES include/rtdebug
  )

  message(STATUS ${LIBRTDEBUG_INCLUDE_DIR})
  find_library(LIBRTDEBUG_STATIC_LIBRARY NAMES librtdebug.a rtdebug
               PATH_SUFFIXES lib
  )
  find_library(LIBRTDEBUG_SHARED_LIBRARY NAMES librtdebug.so rtdebug
               PATH_SUFFIXES lib
  )

  message(STATUS ${LIBRTDEBUG_STATIC_LIBRARY})
  message(STATUS ${LIBRTDEBUG_SHARED_LIBRARY})
  set(LIBRTDEBUG_INCLUDE_DIRS ${LIBRTDEBUG_INCLUDE_DIR})

  # set the global DEBUG define while compiling
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

  include(FindPackageHandleStandardArgs)

  # handle the QUIETLY and REQUIRED arguments and set LIBRTDEBUG_FOUND to TRUE
  # if all listed variables are TRUE
  find_package_handle_standard_args(librtdebug DEFAULT_MSG
                                    LIBRTDEBUG_STATIC_LIBRARY 
                                    LIBRTDEBUG_SHARED_LIBRARY
                                    LIBRTDEBUG_INCLUDE_DIR)

  mark_as_advanced(LIBRTDEBUG_INCLUDE_DIR LIBRTDEBUG_STATIC_LIBRARY LIBRTDEBUG_SHARED_LIBRARY)

endif()
