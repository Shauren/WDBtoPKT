cmake_minimum_required(VERSION 3.24)

project("WDBtoPKT" LANGUAGES CXX CSharp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 1)
set(CMAKE_CXX_EXTENSIONS_DEFAULT 0)

set(WDB_TO_PKT_WOWPACKETPARSER_DIRECTORY "" CACHE FILEPATH "Location of WowPacketParser.dll")
if(NOT EXISTS "${WDB_TO_PKT_WOWPACKETPARSER_DIRECTORY}")
  message(FATAL_ERROR "WDB_TO_PKT_WOWPACKETPARSER_DIRECTORY does not point to valid WowPacketParser.dll")
endif()

string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

add_subdirectory(converter)
add_subdirectory(runner)
