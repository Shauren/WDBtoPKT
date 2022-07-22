cmake_minimum_required(VERSION 3.22)

project("WDBtoPKT" LANGUAGES CXX CSharp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 1)
set(CMAKE_CXX_EXTENSIONS_DEFAULT 0)

set(WDB_TO_PKT_WOWPACKETPARSER_DIRECTORY "" CACHE FILEPATH "Location of WowPacketParser.dll")
if(NOT EXISTS "${WDB_TO_PKT_WOWPACKETPARSER_DIRECTORY}")
  message(FATAL_ERROR "WDB_TO_PKT_WOWPACKETPARSER_DIRECTORY does not point to valid WowPacketParser.dll")
endif()

string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

# Add source to this project's executable.
add_library(WDBtoPKT SHARED
  "ByteBuffer/ByteBuffer.cpp"
  "ByteBuffer/ByteBuffer.h"
  "WDBtoPKT.cpp")

target_compile_options(WDBtoPKT
  PRIVATE
    /EHa)

set_target_properties(WDBtoPKT
  PROPERTIES
    COMMON_LANGUAGE_RUNTIME "netcore"
    DOTNET_TARGET_FRAMEWORK "net6.0"
    VS_DOTNET_REFERENCES "${WDB_TO_PKT_WOWPACKETPARSER_DIRECTORY}"
    VS_GLOBAL_CLRSupport "NetCore"
    VS_GLOBAL_ResolveNuGetPackages "False") # nuget is unable to restore packages in C++/CLI projects but tries anyway (and fails)

set(CMAKE_VS_NUGET_PACKAGE_RESTORE OFF)

add_executable(WDBtoPKTRunner 
  "WDBtoPKT.cs")
 
set_target_properties(WDBtoPKTRunner
  PROPERTIES
    DOTNET_SDK "Microsoft.NET.Sdk"
    DOTNET_TARGET_FRAMEWORK "net6.0"
    VS_GLOBAL_PlatformTarget "x64")

target_link_libraries(WDBtoPKTRunner
  PUBLIC
    WDBtoPKT)

install(TARGETS WDBtoPKT DESTINATION .)
