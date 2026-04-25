add_executable(WDBtoPKTRunner 
  "WDBtoPKT.cs")
 
set_target_properties(WDBtoPKTRunner
  PROPERTIES
    DOTNET_SDK "Microsoft.NET.Sdk"
    DOTNET_TARGET_FRAMEWORK "net10.0"
    VS_GLOBAL_Platforms "x64"
    VS_GLOBAL_PlatformTarget "x64")

target_link_libraries(WDBtoPKTRunner
  PUBLIC
    WDBtoPKT)

install(TARGETS WDBtoPKTRunner DESTINATION .)
