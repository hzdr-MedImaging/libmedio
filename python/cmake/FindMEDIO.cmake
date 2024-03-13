# - Try to find MEDIO
# Once done this will define
#
#  MEDIO_FOUND      - True if MEDIO was found.
#  MEDIO_INCLUDES   - Where to find CMedio.h, etc.
#  MEDIO_STATIC_LIB - List of static libraries when using MEDIO
#  MEDIO_SHARED_LIB - List of shared libraries when using MEDIO
#  MEDIO_DEFINITIONS - Compiler switches required for using MEDIO

if(MEDIO_INCLUDES)
  set(MEDIO_FIND_QUIETLY TRUE)
endif(MEDIO_INCLUDES)

find_path(MEDIO_INCLUDES
  NAMES CMedIO.h 
  PATH_SUFFIXES include/medio
  ONLY_CMAKE_FIND_ROOT_PATH
)

find_library(MEDIO_STATIC_LIB
  NAMES libmedio${CMAKE_STATIC_LIBRARY_SUFFIX}
  PATH_SUFFIXES lib
  ONLY_CMAKE_FIND_ROOT_PATH
)
find_library(MEDIO_SHARED_LIB
  NAMES libmedio${CMAKE_SHARED_LIBRARY_SUFFIX}
  PATH_SUFFIXES lib
  ONLY_CMAKE_FIND_ROOT_PATH
)

# handle the QUIETLY and REQUIRED arguments and set MEDIO_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
set(MEDIO_LIBS ${MEDIO_STATIC_LIB} ${MEDIO_SHARED_LIB})
find_package_handle_standard_args(MEDIO DEFAULT_MSG MEDIO_LIBS MEDIO_INCLUDES)

mark_as_advanced(MEDIO_STATIC_LIB MEDIO_SHARED_LIB MEDIO_INCLUDES)
