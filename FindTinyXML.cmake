# - Find TinyXML
# Find the native TinyXML includes and library
#
#   TINYXML_FOUND        - True if TinyXML found.
#   TINYXML_INCLUDE_DIRS - where to find tinyxml.h, etc.
#   TINYXML_LIBRARIES    - List of libraries when using TinyXML.
#

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_TINYXML tinyxml QUIET)
endif()

find_path(TINYXML_INCLUDE_DIRS NAMES tinyxml.h
                               PATHS ${PC_TINYXML_INCLUDEDIR}
                               PATH_SUFFIXES tinyxml)
find_library(TINYXML_LIBRARIES NAMES tinyxml
                               PATHS ${PC_TINYXML_LIBDIR}
                               PATH_SUFFIXES tinyxml)

include("FindPackageHandleStandardArgs")
find_package_handle_standard_args(TinyXML REQUIRED_VARS TINYXML_INCLUDE_DIRS TINYXML_LIBRARIES)

mark_as_advanced(TINYXML_INCLUDE_DIRS TINYXML_LIBRARIES)
