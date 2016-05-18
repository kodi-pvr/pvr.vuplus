project(pvr.vuplus)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(p8-platform REQUIRED)
find_package(TinyXML REQUIRED)

include_directories(${kodiplatform_INCLUDE_DIRS}
                    ${p8-platform_INCLUDE_DIRS}
                    ${TINYXML_INCLUDE_DIR}
                    ${KODI_INCLUDE_DIR})

set(VUPLUS_SOURCES src/client.cpp
                   src/VuData.cpp)

set(DEPLIBS ${kodiplatform_LIBRARIES}
            ${p8-platform_LIBRARIES}
            ${TINYXML_LIBRARIES})

build_addon(pvr.vuplus VUPLUS DEPLIBS)

include(CPack)
