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
                   src/Enigma2.cpp
                   src/enigma2/Admin.cpp
                   src/enigma2/Channels.cpp
                   src/enigma2/ChannelGroups.cpp
                   src/enigma2/Epg.cpp
                   src/enigma2/RecordingReader.cpp
                   src/enigma2/Recordings.cpp
                   src/enigma2/Settings.cpp
                   src/enigma2/StreamReader.cpp
                   src/enigma2/Timers.cpp
                   src/enigma2/TimeshiftBuffer.cpp
                   src/enigma2/data/AutoTimer.cpp
                   src/enigma2/data/BaseEntry.cpp
                   src/enigma2/data/Channel.cpp
                   src/enigma2/data/ChannelGroup.cpp
                   src/enigma2/data/EpgEntry.cpp
                   src/enigma2/data/RecordingEntry.cpp
                   src/enigma2/data/Timer.cpp
                   src/enigma2/extract/EpgEntryExtractor.cpp
                   src/enigma2/extract/GenreIdMapper.cpp
                   src/enigma2/extract/GenreRytecTextMapper.cpp
                   src/enigma2/extract/ShowInfoExtractor.cpp
                   src/enigma2/utilities/CurlFile.cpp
                   src/enigma2/utilities/FileUtils.cpp
                   src/enigma2/utilities/Logger.cpp
                   src/enigma2/utilities/WebUtils.cpp)

set(VUPLUS_HEADERS src/client.h
                   src/Enigma2.h
                   src/enigma2/Admin.h
                   src/enigma2/Channels.h
                   src/enigma2/ChannelGroups.h
                   src/enigma2/Epg.h
                   src/enigma2/IStreamReader.h
                   src/enigma2/RecordingReader.h
                   src/enigma2/Recordings.h
                   src/enigma2/Settings.h
                   src/enigma2/StreamReader.h
                   src/enigma2/Timers.h
                   src/enigma2/TimeshiftBuffer.h
                   src/enigma2/data/AutoTimer.h
                   src/enigma2/data/BaseEntry.h
                   src/enigma2/data/Channel.h
                   src/enigma2/data/ChannelGroup.h
                   src/enigma2/data/EpgEntry.h
                   src/enigma2/data/RecordingEntry.h
                   src/enigma2/data/Timer.h
                   src/enigma2/extract/IExtractor.h
                   src/enigma2/extract/EpgEntryExtractor.h
                   src/enigma2/extract/EpisodeSeasonPattern.h
                   src/enigma2/extract/GenreIdMapper.h
                   src/enigma2/extract/GenreRytecTextMapper.h
                   src/enigma2/extract/ShowInfoExtractor.h
                   src/enigma2/utilities/CurlFile.h
                   src/enigma2/utilities/DeviceInfo.h
                   src/enigma2/utilities/DeviceSettings.h
                   src/enigma2/utilities/LocalizedString.h
                   src/enigma2/utilities/UpdateState.h
                   src/enigma2/utilities/FileUtils.h
                   src/enigma2/utilities/Logger.h
                   src/enigma2/utilities/WebUtils.h)

set(DEPLIBS ${kodiplatform_LIBRARIES}
            ${p8-platform_LIBRARIES}
            ${TINYXML_LIBRARIES})

build_addon(pvr.vuplus VUPLUS DEPLIBS)

include(CPack)
