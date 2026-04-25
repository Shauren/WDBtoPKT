set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 1)
set(CMAKE_CXX_EXTENSIONS_DEFAULT 0)

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
    DOTNET_TARGET_FRAMEWORK "net10.0"
    VS_DOTNET_REFERENCES "${WDB_TO_PKT_WOWPACKETPARSER_DIRECTORY}"
    VS_GLOBAL_ResolveNuGetPackages "False") # nuget is unable to restore packages in C++/CLI projects but tries anyway (and fails)

install(TARGETS WDBtoPKT DESTINATION .)
