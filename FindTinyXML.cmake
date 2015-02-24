# - Find TinyXML
# Find the native TinyXML includes and library
#
#   TINYXML_FOUND        - True if TinyXML found.
#   TINYXML_INCLUDE_DIRS - where to find tinyxml.h, etc.
#   TINYXML_LIBRARIES    - List of libraries when using TinyXML.
#

if(PKG_CONFIG_FOUND)
  pkg_check_modules (TINYXML tinyxml)
  list(APPEND TINYXML_INCLUDE_DIRS ${TINYXML_INCLUDEDIR})
endif()
if(NOT TINYXML_FOUND)
  find_path( TINYXML_INCLUDE_DIRS "tinyxml.h"
             PATH_SUFFIXES "tinyxml" )

  find_library( TINYXML_LIBRARIES
                NAMES "tinyxml"
                PATH_SUFFIXES "tinyxml" )
endif()

# handle the QUIETLY and REQUIRED arguments and set TINYXML_FOUND to TRUE if
# all listed variables are TRUE
include( "FindPackageHandleStandardArgs" )
find_package_handle_standard_args(TinyXML DEFAULT_MSG TINYXML_INCLUDE_DIRS TINYXML_LIBRARIES )

mark_as_advanced(TINYXML_INCLUDE_DIRS TINYXML_LIBRARIES)
