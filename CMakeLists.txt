cmake_minimum_required(VERSION 3.8)

if(NOT EXISTS "${CMAKE_BINARY_DIR}/dep/vcpkg/scripts/buildsystems/vcpkg.cmake")
  file(DOWNLOAD https://github.com/microsoft/vcpkg/archive/master.tar.gz "${CMAKE_BINARY_DIR}/dep/vcpkg.zip")
  file(ARCHIVE_EXTRACT
    INPUT "${CMAKE_BINARY_DIR}/dep/vcpkg.zip"
    DESTINATION "${CMAKE_BINARY_DIR}/dep/"
  )
  file(RENAME "${CMAKE_BINARY_DIR}/dep/vcpkg-master" "${CMAKE_BINARY_DIR}/dep/vcpkg")
endif()

if(CMAKE_TOOLCHAIN_FILE)
  set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE CACHE INTERNAL ${CMAKE_TOOLCHAIN_FILE})
endif()
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_BINARY_DIR}/dep/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

project("WDBtoPKT")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 1)
set(CMAKE_CXX_EXTENSIONS_DEFAULT 0)

find_package(fmt CONFIG REQUIRED)

# Add source to this project's executable.
add_executable(WDBtoPKT
  "ByteBuffer/ByteBuffer.cpp"
  "ByteBuffer/ByteBuffer.h"
  "WDBtoPKT.cpp")

target_link_libraries(WDBtoPKT
  PRIVATE
    fmt::fmt-header-only)

install(TARGETS WDBtoPKT DESTINATION .)
